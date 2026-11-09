#!/usr/bin/env python3
"""Audit authored MCC lift contacts on an exact existing runtime; never install it.

Run through tools/run_test.py. A case JSON has a ``cases`` list with id, map,
definition_datum, definition, placement_position, entrance and leave_position.
Positive, filtered and nonlift fields have separate assertions. A declared
birth can create an initially absent authored placement through the genuine
public Match API. Source admission, membership, force and ticks are real.
``--entrants 2`` requires declared ``second_entrance`` inputs and proves both
distinct full player generations share the selected field membership.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write_json(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(json.dumps(value, indent=2) + '\n')
    temporary.replace(path)


def owned_session_members(session):
    """Read only members of the session created for our own subprocess."""
    result = []
    if Path('/proc').is_dir():
        for entry in Path('/proc').iterdir():
            if not entry.name.isdecimal():continue
            try:
                fields = (entry / 'stat').read_text().rsplit(')', 1)[1].split()
                if int(fields[3]) == session and fields[0] != 'Z':
                    result.append((int(entry.name), fields[19]))
            except (OSError, ValueError, IndexError):pass
    return result


def stop_owned_session(process, grace=5):
    # GDB's inferior may have its own process group within the new session.
    # Retire that session too, rather than leaving it beside deleted scratch.
    def send(number):
        members = owned_session_members(process.pid)
        try:os.killpg(process.pid, number)
        except ProcessLookupError:pass
        for pid, started in members:
            try:
                fields = Path('/proc', str(pid), 'stat').read_text().rsplit(')', 1)[1].split()
                if int(fields[3]) == process.pid and fields[19] == started:
                    os.kill(pid, number)
            except (OSError, ValueError, IndexError):pass
    if process.pid == os.getsid(0):raise RuntimeError('Cannot retire the audit owner session')
    send(signal.SIGTERM)
    try:process.wait(timeout=grace)
    except subprocess.TimeoutExpired:pass
    send(signal.SIGKILL)
    process.wait()
    deadline=time.monotonic()+2
    while owned_session_members(process.pid):
        if time.monotonic()>=deadline:raise RuntimeError('Owned audit session did not retire')
        time.sleep(.01)


def interrupted(number, _frame):
    # A second owner cancellation must not interrupt the child cleanup.
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    raise SystemExit(128 + number)


def abi_layout(source, runtime, scratch, inventory_path=None):
    inventory = json.loads((inventory_path or runtime.parent / 'source-hashes.json').read_text())
    headers = ['include/halo2/multiplayer.hpp', 'include/halo2/simulation.hpp',
               'include/halo2/collision.hpp', 'include/halo2/physics_live_world_tu15.hpp',
               'include/halo2/native_tu15_physics_frame_residuals.hpp']
    pins = {name: digest(source / name) for name in headers}
    if any(inventory.get(name) != pin for name, pin in pins.items()):
        raise ValueError('Audit ABI headers must match the exact runtime source inventory')
    fields = ['native_lift_scalar_calls', 'native_lift_observation_refusals',
              'native_pair_creation_calls', 'native_convex_agent_calls',
              'native_pair_deletion_calls', 'native_pair_deletion_refusals',
              'native_wrapped_factory_calls']
    stats_header=(source/'include/halo2/native_tu15_physics_frame_residuals.hpp').read_text()
    for name in ('native_lift_velocity_attempts','native_lift_velocity_completed',
                 'native_lift_velocity_observed_stores','native_lift_velocity_refusals'):
        if name in stats_header:fields.append(name)
    code = '#include "halo2/multiplayer.hpp"\n#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include <cstddef>\n#include <iostream>\n'
    values = [('player_current', 'offsetof(MultiplayerPlayer,source_current_object)'),
              ('unit_source', 'offsetof(Unit,source_datum)'),
              ('unit_position', 'offsetof(Unit,body)+offsetof(Body,position)'),
              ('unit_velocity', 'offsetof(Unit,body)+offsetof(Body,velocity)'),
              ('stats_size', 'sizeof(NativeTu15PhysicsFrameExecutionStats)')]
    values += [(name, 'offsetof(NativeTu15PhysicsFrameExecutionStats,' + name + ')') for name in fields]
    code += 'int main(){using namespace halo2;std::cout<<"{"'
    for index, (name, expression) in enumerate(values):
        code += '<<"' + (',' if index else '') + '\\"' + name + '\\":"<<(' + expression + ')'
    code += '<<"}\\n";}\n'
    cpp, binary = scratch / 'abi.cpp', scratch / 'abi'
    cpp.write_text(code)
    command = ['c++', '-std=c++20', '-Wno-invalid-offset', '-I' + str(source / 'include'),
               '-I/usr/include/SDL2', str(cpp), '-o', str(binary)]
    compiled = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=90)
    if compiled.returncode:
        raise RuntimeError('ABI compilation failed: ' + compiled.stdout[-6000:])
    layout = json.loads(subprocess.check_output([str(binary)], text=True, timeout=10))
    return {'layout': layout, 'headers_sha256': pins, 'compiler_command': command,
            'compiler_source': code, 'compiler_source_sha256': hashlib.sha256(code.encode()).hexdigest()}


GDB_SCRIPT = r'''set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
set breakpoint pending off
set $world=(void*)0
set $bootstrapped=0
set $scan=0
set $do_input=0
set $active=0
set $do_stats=0
set $needs_birth=0
set $want_capture=0
set $unit2=4294967295
set $skip_body_probe=0
set $read_retired_body0=0
set $read_retired_body1=0
set $do_lifecycle_stats=0
python
import gdb,json,math,struct
plan=CASE_PLAN
layout=ABI_LAYOUT
cases=[]
fields={}
errors=[]
throws=[]
all_throws=[]
source_captures=[]
kernel_entries={}
unavailable_kernel_symbols=[]
world_entries=0
index=0
phase=None
phase_tick=0
active=None
done=False
stats_label=''
leave_policy=LEAVE_POLICY
entrant_count=ENTRANT_COUNT
def value(name):return int(gdb.parse_and_eval('$'+name))
def setv(name,value):gdb.set_convenience_variable(name,gdb.Value(value))
def raw(address,count):return bytes(gdb.selected_inferior().read_memory(address,count))
def word(address):return struct.unpack('<I',raw(address,4))[0]
def vector(address):return list(struct.unpack('<fff',raw(address,12)))
def bootstrap():
 global headers,high,physical_header,spawn_point,spawn_point2
 pool=value('object_pool');headers=value('headers');high=word(pool+0x38)
 if high>2048 or word(pool+0x24)!=12:raise RuntimeError('Actual object pool layout is not TU1.5')
 physical_header=value('physical_header')
 spawn_point=vector(value('body')+layout['unit_position'])
 if entrant_count==2:spawn_point2=vector(value('body2')+layout['unit_position'])
def choose_header():
 index=value('scan');header=headers+index*12;salt=struct.unpack('<H',raw(header,2))[0]
 valid=salt!=0 and word(header+8)!=0
 setv('valid',int(valid));setv('object',(salt<<16)|index)
def remember_object():
 body=value('source_body');definition=word(body)
 if definition in {row['definition_datum'] for row in plan}:
  fields.setdefault(definition,[]).append({'object':value('object'),'body':body,'position':vector(body+0x64)})
def next_case():
 global index,phase,phase_tick,active,done,stats_label
 setv('active',0);setv('do_input',0)
 while index<len(plan):
  row=plan[index];index+=1
  candidates=fields.get(row['definition_datum'],[])
  target=min(candidates,key=lambda c:sum((a-b)**2 for a,b in zip(c['position'],row['placement_position'])),default=None)
  if target is not None and math.sqrt(sum((a-b)**2 for a,b in zip(target['position'],row['placement_position'])))>row.get('placement_tolerance',.25):
   target=None
  active={'id':row['id'],'case':row,'phases':{},'samples':[],'errors':[],'field':target,'unit':value('unit'),
    'leave_policy':leave_policy,'leave_position_used':list(spawn_point if leave_policy=='canonical-spawn'else row['leave_position'])}
  if entrant_count==2:
   active['second_unit']=value('unit2')
   active['second_leave_position_used']=list(spawn_point2 if leave_policy=='canonical-spawn'else row.get('second_leave_position',row['leave_position']))
   if active['second_unit']==active['unit']:raise RuntimeError('Two entrants must have distinct actual full source generations')
  cases.append(active)
  if target is None and row.get('allow_declared_birth'):
   active['status']='awaiting-declared-authored-birth';setv('birth_index',row['content_placement_index'])
   setv('needs_birth',1);setv('active',1);return
  if target is None:
   active['errors'].append('Authored force placement is absent from the genuine canonical object pool')
   active['status']='not-instantiated';continue
  if configure_active(target):return
 done=True;active=None
def configure_active(target):
 global phase,phase_tick,stats_label
 row=active['case'];active['field']=target
 distance=math.sqrt(sum((a-b)**2 for a,b in zip(target['position'],row['placement_position'])))
 active['placement_distance']=distance
 if distance>row.get('placement_tolerance',.25):
  active['errors'].append('No genuine field object matches this authored placement position')
  active['status']='placement-unresolved';return False
 physical=word(target['body']+0xb4)
 if physical==0xffffffff:
  active['errors'].append('Authored field has no genuine physical component')
  active['status']='component-unavailable';return False
 pindex=physical&65535;capacity=word(physical_header+0x20)
 if pindex>=capacity:raise RuntimeError('Actual field physical component index escapes its pool')
 setv('component_address',word(physical_header+0x44)+pindex*word(physical_header+0x24))
 phase='entry';phase_tick=0;active['status']='running';stats_label='entry-before'
 prepare_input(row['entrance']);setv('active',1);return True
def finish_birth():
 object=value('object');active['declared_birth']={'content_placement_index':active['case']['content_placement_index'],'object':object}
 target=next((c for c in fields.get(active['case']['definition_datum'],[])if c['object']==object),None)
 if target is None:raise RuntimeError('Declared authored source birth did not publish its actual field object')
 setv('needs_birth',0)
 if not configure_active(target):next_case()
def prepare_input(point):
 xy=struct.unpack('<d',struct.pack('<ff',*point[:2]))[0]
 gdb.set_convenience_variable('input_xy',gdb.Value(xy));gdb.set_convenience_variable('input_z',gdb.Value(float(point[2])))
 if entrant_count==2:
  point2=active['second_leave_position_used']if phase=='leave'else active['case']['second_entrance']
  xy2=struct.unpack('<d',struct.pack('<ff',*point2[:2]))[0]
  gdb.set_convenience_variable('input2_xy',gdb.Value(xy2));gdb.set_convenience_variable('input2_z',gdb.Value(float(point2[2])))
 setv('do_input',1)
def capture_stats():
 address=value('stats');sample={}
 for name,offset in layout.items():
  if name.startswith('native_'):
   count=4 if name=='native_lift_scalar_calls'else 2 if name in('native_lift_velocity_attempts',
    'native_lift_velocity_completed','native_lift_velocity_observed_stores')else 1
   values=struct.unpack('<'+'Q'*count,raw(address+offset,8*count));sample[name]=list(values)if count>1 else values[0]
 active['phases'][stats_label]=sample
def mark_input():
 active.setdefault('inputs',[]).append({'phase':phase,'point':active['leave_position_used']if phase=='leave'else active['case']['entrance'],'unit':value('unit')})
 if entrant_count==2:active['inputs'].append({'phase':phase,'point':active['second_leave_position_used']if phase=='leave'else active['case']['second_entrance'],'unit':value('unit2'),'slot':1})
 setv('do_input',0)
def source_generation(unit):
 slot=unit&65535
 if unit==0xffffffff or slot>=word(value('object_pool')+0x20):
  raise RuntimeError('Captured entrant generation escapes the actual source pool')
 header=raw(value('headers')+slot*12,12);salt=struct.unpack_from('<H',header,0)[0]
 body=struct.unpack_from('<I',header,8)[0]
 return {'unit':unit,'header_hex':header.hex(),'salt':salt,'body_address':body,
   'same_generation_allocated':salt==(unit>>16) and body!=0}
def authenticate_initial_generations():
 for slot in range(entrant_count):
  if not source_generation(value('unit2'if slot else'unit'))['same_generation_allocated']:
   raise RuntimeError('Initial player generation is absent from the actual source pool')
def prepare_readback():
 global stats_label
 setv('skip_body_probe',0);setv('read_retired_body0',0);setv('read_retired_body1',0)
 if active.get('lifecycle'):
  setv('skip_body_probe',1)
  active['lifecycle']['continued_world_entries']=world_entries-active['lifecycle']['world_entries_at_event']
  if active['lifecycle']['continued_world_entries']>=90:
   stats_label='lifecycle-after';setv('do_lifecycle_stats',1)
  return
 states=[]
 for slot in range(entrant_count):
  expected=active['second_unit'if slot else'unit'];current=value('unit2'if slot else'unit')
  state=source_generation(expected);state.update(slot=slot,current_player_unit=current)
  states.append(state)
 if any(not state['same_generation_allocated'] or state['current_player_unit']!=state['unit']for state in states):
  active['lifecycle']={'phase':phase,'completed_body_samples_before_event':len(active['samples']),
    'world_entries_at_event':world_entries,'continued_world_entries':0,'entrants':states,
    'probe_policy':'Stop Unit/body queries after a full source generation changes; only still-allocated old bodies receive one read-only vitality observation.'}
  setv('skip_body_probe',1)
  for state in states:
   if state['same_generation_allocated']:
    setv('read_retired_body'+str(state['slot']),1)
    setv('retired_source_body'+str(state['slot']),state['body_address'])
def record_lifecycle_vitality(slot):
 pointer=value('retired_body_pointer'+str(slot));health,shield=struct.unpack('<ff',raw(pointer+0xec,8))
 state=active['lifecycle']['entrants'][slot]
 state['native_body_fraction']=health;state['native_shield_fraction']=shield
 state['native_depleted_body']=math.isfinite(health) and health<=0
def finish_lifecycle():
 global active
 active['status']='native-lifecycle-completed';active=None;setv('active',0);setv('do_lifecycle_stats',0)
def entrant_observation(slot):
 unit=value('unit2'if slot else'unit');body=value('body2'if slot else'body')
 if unit!=active['second_unit'if slot else'unit'] or word(body+layout['unit_source'])!=unit:
  raise RuntimeError('Entrant '+str(slot)+' full source unit generation changed'if slot else'Entrant full source unit generation changed')
 count=value('member_count');members=value('members');matched=[]
 for n in range(count):
  row=raw(members+n*8,8)
  if row[0]==active['case'].get('membership_phantom_index',active['case']['phantom_index']) and struct.unpack_from('<I',row,4)[0]==unit:
   matched.append({'row_hex':row.hex(),'phantom_index':row[0],'other_body_slot':row[1],
     'references':row[2],'field_body_slot':row[3],'object':unit})
 position=vector(body+layout['unit_position']);velocity=vector(body+layout['unit_velocity'])
 if not all(math.isfinite(x)for x in position+velocity):raise RuntimeError('Nonfinite actual Match body observation')
 return {'slot':slot,'unit':unit,'position':position,'velocity':velocity,
   'selected_field_member':bool(matched),'matched_membership_rows':matched}
def observe():
 global phase_tick,phase,stats_label
 if active is None or value('do_input'):return
 try:
  component=value('component')
  if word(component+8)!=active['field']['object']:raise RuntimeError('Selected field component changed source owner')
  sample={'phase':phase,'tick':phase_tick,**entrant_observation(0)}
  if entrant_count==2:sample['second_entrant']=entrant_observation(1)
  active['samples'].append(sample)
  phase_tick+=1
  limit=30 if phase=='leave'else 90
  if phase_tick>=limit:
   stats_label=phase+'-after';setv('do_stats',1)
 except (gdb.error,RuntimeError,MemoryError,ValueError)as error:
  active['errors'].append(str(error));stats_label=phase+'-after';setv('do_stats',1)
def advance():
 global phase,phase_tick,stats_label
 setv('do_stats',0)
 if phase=='entry':phase='leave'
 elif phase=='leave':phase='reentry'
 else:
  active['status']='completed';next_case();return
 phase_tick=0;stats_label=phase+'-before'
 prepare_input(active['leave_position_used']if phase=='leave'else active['case']['entrance'])
def record_throw():
  setv('want_capture',0)
  message='';type_name=''
  try:
   address=int(gdb.parse_and_eval('$rdi'));pointer=struct.unpack('<Q',raw(address+8,8))[0]
   message=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(length=1024,errors='replace').split(chr(0))[0]
  except (gdb.error,MemoryError,ValueError):pass
  try:type_name=gdb.execute('info symbol $rsi',to_string=True).strip()
  except gdb.error:pass
  note={'case':active['id']if active else None,'phase':phase,'message':message,'type':type_name}
  if len(all_throws)<128:all_throws.append(note)
  if 'TU1.5'in message or 'native lift'in message.lower():
   note['backtrace']=gdb.execute('bt 18',to_string=True)[-16000:]
   throws.append(note);print('GRAVLIFT_NATIVE_THROW='+json.dumps(note),flush=True)
  if CAPTURE_ENABLED and active and not source_captures and 'live world owner missing'in message:setv('want_capture',1)
def record_source_capture(address):
 pointer=value('capture_pointer');data=raw(pointer,12);table=word(pointer)
 source_captures.append({'case':active['id'],'phase':phase,'source_address':address,
   'bytes_hex':data.hex(),'words':list(struct.unpack('<III',data)),'vtable':table})
 setv('capture_vtable',table)
def record_vtable_owner():
 note=source_captures[-1];note['vtable_plus4_owner']=word(value('vtable_pointer'))
 target=note['words'][2]
 setv('capture_follow',int(note['vtable_plus4_owner']==0x30b960 and target not in(0,0xffffffff,note['source_address'])))
 setv('capture_address',target)
 print('GRAVLIFT_SOURCE_CAPTURE='+json.dumps(note),flush=True)
class Throws(gdb.Breakpoint):
 def stop(self):
  record_throw()
  return bool(value('want_capture'))
Throws('__cxa_throw',internal=True)
class KernelEntry(gdb.Breakpoint):
 def __init__(self,name):
  super().__init__('halo2::'+name,internal=True);self.owner_name=name
 def stop(self):
  name=self.owner_name
  if name=='native_tu15_destroy_contact_agent':
   # SysV second integer input is the typed native destructor owner. This
   # read-only entry probe distinguishes the actual2F35F0 route from siblings.
   kind=int(gdb.parse_and_eval('$esi'))&0xffffffff
   if kind!=3:return False
   name+='.owner_2f35f0'
  kernel_entries[name]=kernel_entries.get(name,0)+1
  if active:
   counts=active.setdefault('native_kernel_entries',{});counts[name]=counts.get(name,0)+1
  return False
for name in('native_tu15_query_reversed_sphere_capsule_overlap','native_tu15_collect_object_overlap_pair',
 'native_tu15_detach_light_attachment','native_tu15_get_conveyor_constraint_info',
 'native_tu15_destroy_contact_agent'):
 try:KernelEntry(name)
 except gdb.error:unavailable_kernel_symbols.append(name)
end
define capture_activity
set $stats=(void*)malloc(STATS_SIZE)
call ((void(*)(void*,void*)) 'halo2::PhysicsLiveWorldTu15::native_physics_frame_activity() const')($stats,$world)
python capture_stats()
call ((void(*)(void*)) 'halo2::NativeTu15PhysicsFrameExecutionStats::~NativeTu15PhysicsFrameExecutionStats()')($stats)
call ((void(*)(void*))free)($stats)
end
break halo2::MultiplayerObjectRuntime::step_objects
commands
silent
python world_entries+=1
if $world == 0
set $runtime=(void*)$rdi
set $world=((void*(*)(void*)) 'halo2::MultiplayerObjectRuntime::tu15_world()')($runtime)
end
continue
end
break halo2::MultiplayerMatch::step
commands
silent
set $match=(void*)$rdi
if $world != 0
set $player=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::player(unsigned int) const')($match,0)
set $unit=*((unsigned int*)((char*)$player+PLAYER_CURRENT))
set $entrants_ready=($unit != 4294967295)
if ENTRANT_COUNT == 2
set $player2=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::player(unsigned int) const')($match,1)
set $unit2=*((unsigned int*)((char*)$player2+PLAYER_CURRENT))
set $entrants_ready=($entrants_ready && $unit2 != 4294967295)
end
if $entrants_ready || ($bootstrapped && $active)
if $bootstrapped == 0
set $memory=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::memory()')($world)
set $object_pool=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::object_pool() const')($world)
set $headers=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)((char*)$object_pool+0x44)),*((unsigned int*)((char*)$object_pool+0x38))*12)
set $physical_global=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,0x52be5c,4)
set $physical_header=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)$physical_global),0x4c)
python authenticate_initial_generations()
set $body=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::unit(unsigned int) const')($match,0)
if ENTRANT_COUNT == 2
set $body2=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::unit(unsigned int) const')($match,1)
end
python bootstrap()
while $scan < *((unsigned int*)((char*)$object_pool+0x38))
python choose_header()
if $valid
set $source_body=((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,$object)
python remember_object()
end
set $scan=$scan+1
end
set $bootstrapped=1
python next_case()
end
if $active
if $needs_birth == 2
python finish_birth()
end
if $needs_birth == 1
set $object=((unsigned int(*)(void*,unsigned long)) 'halo2::MultiplayerMatch::create_world_object(unsigned long)')($match,$birth_index)
set $source_body=((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,$object)
python remember_object()
set $needs_birth=2
end
if $needs_birth == 0
if $do_input == 0
python prepare_readback()
if $read_retired_body0
set $retired_body_pointer0=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$retired_source_body0,0x110)
python record_lifecycle_vitality(0)
end
if $read_retired_body1
set $retired_body_pointer1=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$retired_source_body1,0x110)
python record_lifecycle_vitality(1)
end
if $do_lifecycle_stats
capture_activity
python finish_lifecycle()
end
if $skip_body_probe == 0
set $body=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::unit(unsigned int) const')($match,0)
if ENTRANT_COUNT == 2
set $body2=((void*(*)(void*,unsigned int)) 'halo2::MultiplayerMatch::unit(unsigned int) const')($match,1)
end
set $member_header=*((unsigned int*)((char*)$component+0x94))
set $member_count=0
set $members=(void*)0
if $member_header != 0
set $mh=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$member_header,8)
set $member_count=*((unsigned int*)((char*)$mh+4))
if $member_count > 4096
echo Invalid field member extent\n
quit 2
end
if $member_count > 0
set $members=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)$mh),$member_count*8)
end
end
python observe()
if $do_stats
capture_activity
python advance()
end
end
end
if $active && $do_input
set $component=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$component_address,0xa0)
capture_activity
call ((void(*)(void*,unsigned int,double,float)) 'halo2::PhysicsLiveWorldTu15::set_object_position(unsigned int, halo2::Vec3)')($world,$unit,$input_xy,$input_z)
if ENTRANT_COUNT == 2
call ((void(*)(void*,unsigned int,double,float)) 'halo2::PhysicsLiveWorldTu15::set_object_position(unsigned int, halo2::Vec3)')($world,$unit2,$input2_xy,$input2_z)
end
python mark_input()
end
end
end
end
end
continue
end
run
if $want_capture
SOURCE_CAPTURE_COMMANDS
continue
end
if $_isvoid($_exitcode)
bt 18
else
printf "GRAVLIFT_CHILD_EXIT=%d\n",$_exitcode
end
python
print('GRAVLIFT_AUDIT_RESULT='+json.dumps({'cases':cases,'done':done,'native_exceptions':throws,'all_exceptions':all_throws,'source_captures':source_captures,'native_kernel_entries':kernel_entries,'unavailable_kernel_symbols':unavailable_kernel_symbols,'native_world_entries':world_entries,'errors':errors}),flush=True)
end
'''


def evaluate(row):
    errors = list(row.get('errors', []))
    phases = {}
    entrant_count = 2 if 'second_unit' in row else 1
    if entrant_count == 2 and row['unit'] == row['second_unit']:
        errors.append('Two entrants did not have distinct actual full source generations')
    for name in ('entry', 'leave', 'reentry'):
        samples = [x for x in row['samples'] if x['phase'] == name]
        before, after = row['phases'].get(name + '-before', {}), row['phases'].get(name + '-after', {})
        scalar = (sum(after['native_lift_scalar_calls']) - sum(before['native_lift_scalar_calls'])
                  if 'native_lift_scalar_calls' in before and 'native_lift_scalar_calls' in after else None)
        velocity = (sum(after['native_lift_velocity_completed'])-sum(before['native_lift_velocity_completed'])
                    if 'native_lift_velocity_completed' in before and 'native_lift_velocity_completed' in after else None)
        assertion = row['case'].get('travel_assertion', 'contact_motion')
        phases[name] = {'samples': len(samples),
                        'world_scalar_calls_delta': scalar, 'world_native_velocity_completed_delta':velocity,
                        'travel_assertion': assertion, 'entrants': []}
        if len(samples) < (30 if name == 'leave' else 90):
            errors.append(name + ': insufficient actual completed ticks')
        for slot in range(entrant_count):
            observations = [s if slot == 0 else s['second_entrant'] for s in samples
                            if slot == 0 or 'second_entrant' in s]
            point_name = 'second_entrance' if slot else 'entrance'
            leave_name = 'second_leave_position_used' if slot else 'leave_position_used'
            point = row.get(leave_name,row['case']['leave_position']) if name=='leave' else row['case'][point_name]
            peak = max((s['position'][2] for s in observations), default=point[2])
            speed = max((s['velocity'][2] for s in observations), default=0)
            member = sum(s['selected_field_member'] for s in observations)
            member_speed = max((math.sqrt(sum(v*v for v in s['velocity']))
                                for s in observations if s['selected_field_member']), default=0)
            metrics = {'slot': slot, 'unit': row['second_unit' if slot else 'unit'],
                       'samples': len(observations), 'selected_member_samples': member,
                       'rise': peak - point[2], 'maximum_upward_velocity': speed,
                       'maximum_speed_while_selected_member': member_speed}
            phases[name]['entrants'].append(metrics)
            # Preserve the original one-entrant report fields for existing readers.
            if slot == 0:phases[name].update({k:v for k,v in metrics.items() if k not in ('slot','unit','samples')})
            label = name + ': entrant ' + str(slot) + ': '
            if len(observations) != len(samples):errors.append(label + 'missing actual body observations')
            if 'authored_ignores_bipeds_bit5' in row['case'].get('exclusion_reasons', []):
                if member:errors.append(label + 'authored biped exclusion admitted the exact entrant/phantom membership')
            elif name == 'leave':
                if observations and member == len(observations):errors.append(label + 'never left the selected field membership')
            elif row['case']['classification'] != 'actual_force_lift':
                # Filtering can follow geometric collection. No scalar-attribution
                # or travel claim is made for other and filtered fields.
                pass
            elif not member or scalar is None or scalar <= 0 or member_speed <= .05:
                errors.append(label + 'selected membership, scalar execution and entrant motion were not all observed')
            elif velocity is not None and velocity<=0:
                errors.append(label + 'no completed native lift velocity call was observed')
            elif assertion == 'upward' and (peak - point[2] <= row['case'].get('minimum_rise', .5) or speed <= 0):
                errors.append(label + 'declared upward travel was not observed')
            elif assertion not in ('contact_motion', 'upward'):
                errors.append(label + 'unsupported travel assertion')
        if entrant_count == 2:
            simultaneous = sum(s['selected_field_member'] and s.get('second_entrant',{}).get('selected_field_member',False) for s in samples)
            phases[name]['simultaneous_selected_member_samples'] = simultaneous
            if name != 'leave' and row['case']['classification'] == 'actual_force_lift' and not simultaneous:
                errors.append(name + ': the two genuine entrants never shared exact selected field membership')
        if after.get('native_lift_observation_refusals', 0) != before.get('native_lift_observation_refusals', 0):
            errors.append(name + ': lift observation refusal counter changed')
        if after.get('native_lift_velocity_refusals',0)!=before.get('native_lift_velocity_refusals',0):
            errors.append(name + ': native lift velocity refusal counter changed')
    samples = row.pop('samples')
    row['completed_sample_count'] = len(samples)
    row['completed_samples_sha256'] = hashlib.sha256(json.dumps(samples, separators=(',', ':')).encode()).hexdigest()
    examples = []
    for name in ('entry', 'leave', 'reentry'):
        segment = [s for s in samples if s['phase'] == name]
        if segment:
            chosen = [segment[0], segment[-1], max(segment, key=lambda s: s['position'][2])]
            chosen += [s for s in segment if s['selected_field_member']][:2]
            examples.extend(s for n, s in enumerate(chosen) if s not in chosen[:n])
    row['sample_examples'] = examples
    selected_contact = all(any((s if slot==0 else s['second_entrant'])['selected_field_member']
                              for s in samples if slot==0 or 'second_entrant'in s)for slot in range(entrant_count))
    shared_contact = entrant_count==1 or any(s['selected_field_member'] and
        s.get('second_entrant',{}).get('selected_field_member',False)for s in samples)
    life = row.get('lifecycle',{})
    native_release = any(not state.get('same_generation_allocated',True) or
                         (state.get('current_player_unit')!=state.get('unit') and state.get('native_depleted_body',False))
                         for state in life.get('entrants',[]))
    lifecycle_errors=[]
    if life:
        if not selected_contact:lifecycle_errors.append('Exact selected contact was not observed for every entrant')
        if not shared_contact:lifecycle_errors.append('The two entrants never shared exact selected contact')
        if not native_release:lifecycle_errors.append('No retired full generation or depleted released native body was confirmed')
        if life.get('continued_world_entries',0)<90:lifecycle_errors.append('Fewer than90 actual World entries followed the native lifecycle event')
        before=row['phases'].get('entry-before',{});after=row['phases'].get('lifecycle-after',{})
        if not after:lifecycle_errors.append('Missing actual native activity after the continued World phases')
        for counter in('native_lift_observation_refusals','native_lift_velocity_refusals'):
            if after.get(counter,0)!=before.get(counter,0):lifecycle_errors.append(counter+' changed during the lifecycle window')
        lifecycle_errors.extend(row.get('errors',[]))
    safe_lifecycle = row.get('status')=='native-lifecycle-completed' and bool(life) and not lifecycle_errors
    complete = row.get('status')=='completed' and not errors
    disposition = ('completed_force_cycle' if complete and row['case']['classification']=='actual_force_lift' else
                   'completed_filtered_or_nonforce_input_cycle' if complete else
                   'safe_native_contact_lifecycle' if safe_lifecycle else 'unvisited_or_blocked')
    return {'passed': complete,
            'id': row['id'], 'status': row.get('status'), 'phases': phases,
            'entered_slots': list(range(entrant_count)),
            'disposition':disposition,'selected_contact_observed':selected_contact,
            'shared_selected_contact_observed':shared_contact and selected_contact,
            'lifecycle_crash_safety_errors':lifecycle_errors,
            'native_contact_lifecycle_qualified':safe_lifecycle,
            'crash_safety_passed':complete or safe_lifecycle,
            'force_cycle_qualified':complete and row['case']['classification']=='actual_force_lift',
            'acceptance_scope': ('Exact selected contact followed by authenticated native death/retirement,90 continued World phases and clean exit; sustained travel/leave/reentry remain unqualified.' if safe_lifecycle else
                'Selected source membership, aggregate scalar execution, entrant motion and clean leave/reentry; marker direction is separate.' if row['case']['classification']=='actual_force_lift'
                else 'Declared biped position through authored nonlift/filtered field geometry, 90+30+90 actual ticks and clean source lifetime; scalar attribution/filter parity is not asserted.'),
            'force_attributed_to_selected_rule': False,
            'scalar_observation_scope': 'Per-world aggregate; it cannot alone attribute force to this entrant or force rule.',
            'lift_travel_qualified': row['case'].get('travel_assertion') == 'upward' and not errors,
            'errors': errors, 'observed': row}


def run_world(runtime, maps, source, cases, players, rendered, layout, scratch, timeout, output, capture_addresses=(), collector_chain=False, leave_policy='canonical-spawn', entrant_count=1):
    map_path=maps/(cases[0]['map']+'.map')
    map_pin=digest(map_path)
    if any(row.get('map_sha256')!=map_pin for row in cases):
        raise ValueError('Selected MCC map bytes differ from the authored census: '+cases[0]['map'])
    script = (GDB_SCRIPT.replace('CASE_PLAN', repr(cases)).replace('ABI_LAYOUT', repr(layout['layout']))
              .replace('LEAVE_POLICY',repr(leave_policy)).replace('ENTRANT_COUNT',str(entrant_count)))
    captures=[]
    for address in capture_addresses:
        captures.extend(['set $capture_address='+str(address),'set $capture_depth=0','set $capture_follow=1',
            'while $capture_follow && $capture_depth < '+('8'if collector_chain else'1'),
            "set $capture_pointer=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$capture_address,12)",
            "python record_source_capture(value('capture_address'))",
            "set $vtable_pointer=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$capture_vtable+4,4)",
            'python record_vtable_owner()','set $capture_depth=$capture_depth+1','end'])
    script=script.replace('CAPTURE_ENABLED',repr(bool(capture_addresses))).replace('SOURCE_CAPTURE_COMMANDS','\n'.join(captures))
    script = script.replace('STATS_SIZE', str(layout['layout']['stats_size'])).replace('PLAYER_CURRENT', str(layout['layout']['player_current']))
    gdb_file = scratch / 'audit.gdb';gdb_file.write_text(script)
    # A genuine death may end the body cycle. Keep the actual World running
    # for90 more phases without querying a stale body or supplying a respawn.
    ticks = len(cases) * 310 + 20
    command = [str(runtime), '--multiplayer', '--map', str(maps / (cases[0]['map'] + '.map')),
               '--shared-maps', str(maps), '--local-players', str(players), '--mp-starting-weapon', '5',
               '--mute', '--mp-source-object-world', str(scratch / 'world')]
    if rendered:
        command += ['--viewport', '640', '480', '--render-size', '640', '480', '--graphics-mode', 'Original', '--frames', str(ticks * 2 + 200)]
    else: command += ['--simulate', str(ticks)]
    command = ['gdb', '--batch', '-x', str(gdb_file), '--args', *command]
    if rendered:command = ['xvfb-run', '-a', '-s', '-screen 0 960x720x24 -nolisten tcp', *command]
    environment = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), SDL_VIDEODRIVER='x11',
        HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'pipeline'), MESA_SHADER_CACHE_DIR=str(scratch / 'mesa'),
        __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia'), XDG_CACHE_HOME=str(scratch / 'cache'),
        XDG_CONFIG_HOME=str(scratch / 'config'), XDG_DATA_HOME=str(scratch / 'data'))
    started = time.monotonic();log = scratch / 'runtime.log'
    with log.open('w') as stream:
        process = subprocess.Popen(command, cwd=source, env=environment, stdout=stream, stderr=subprocess.STDOUT, start_new_session=True)
        try: code = process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            code = 124
        finally:stop_owned_session(process)
    text = log.read_text(errors='replace')
    result_rows = re.findall(r'^GRAVLIFT_AUDIT_RESULT=(.*)$', text, re.MULTILINE)
    observed = json.loads(result_rows[-1]) if result_rows else {}
    child_rows = re.findall(r'GRAVLIFT_CHILD_EXIT=(\d+)', text)
    child = int(child_rows[-1]) if child_rows else None
    rows = [evaluate(row) for row in observed.get('cases', [])]
    active_exceptions=[note for note in observed.get('native_exceptions',[]) if note.get('case') is not None]
    for row in rows:
        if code != 0 or child != 0 or active_exceptions:
            row['passed'] = False;row['errors'].append('Native world did not exit cleanly without native exceptions')
            row['crash_safety_passed']=False;row['force_cycle_qualified']=False
            row['native_contact_lifecycle_qualified']=False
            row['disposition']=('actual_native_fatal' if child not in(None,0) else
                                'native_exception_during_contact' if active_exceptions else 'audit_or_world_incomplete')
    report = {'map': cases[0]['map'], 'players': players, 'entered_slots':list(range(entrant_count)), 'rendered': rendered, 'command': command,
              'map_sha256': map_pin, 'map_unchanged': digest(map_path)==map_pin,
              'gdb_exit_code': code, 'child_exit_code': child, 'seconds': time.monotonic() - started,
              'cases': rows, 'observed': {k:v for k,v in observed.items() if k!='cases'},
              'active_case_native_exceptions': active_exceptions,
              'caught_pre_case_exceptions': [note for note in observed.get('all_exceptions',[]) if note.get('case') is None],
              'native_kernel_entries': observed.get('native_kernel_entries',{}),
              'native_kernel_observation_scope':'Function-entry observations in this real world, including startup; per-case windows are retained separately, without per-entrant force attribution.',
              'gdb_script_sha256': hashlib.sha256(script.encode()).hexdigest(),
              'attempted_case_ids': [c['id'] for c in cases],
              'crash_safety_passed':bool(rows)and all(r['crash_safety_passed']for r in rows),
              'passed': bool(rows) and len(rows) == len(cases) and all(r['passed'] for r in rows)}
    report['fatal_errors'] = [line for line in text.splitlines() if line.startswith('error:') or 'received signal' in line]
    report['diagnostic_tail'] = text[-12000:] if code != 0 or child != 0 else ''
    if not report['map_unchanged']:report['passed']=False;report['crash_safety_passed']=False
    write_json(output, report)
    output.with_suffix('.log').write_text(text[-64000:])
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--owned-build-inventory',type=Path,help='Diagnostic alternative: explicit source-hashes.json from an active owned frozen CMake build; does not qualify or install it')
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--case-json', type=Path, required=True)
    parser.add_argument('--source-root', type=Path, default=ROOT)
    parser.add_argument('--report-output', type=Path, required=True)
    parser.add_argument('--players', type=int, choices=(1, 2), action='append')
    parser.add_argument('--entrants',type=int,choices=(1,2),default=1,
                        help='Move one entrant, or both genuine local players using explicit second_entrance inputs')
    parser.add_argument('--map', action='append')
    parser.add_argument('--case', action='append', help='Exact census case ID')
    parser.add_argument('--rendered', action='store_true')
    parser.add_argument('--positive-only', action='store_true', help='Explicitly restrict to biped-admitted directional fields')
    parser.add_argument('--create-initially-absent', action='store_true', help='Declare genuine births for authored absent placements, using exact canonical content indices')
    parser.add_argument('--capture-source-address', action='append', type=lambda value:int(value,0), default=[],
                        help='Diagnostic only: retain12 actual source bytes and vtable+4 on the first missing-owner throw')
    parser.add_argument('--capture-collector-chain',action='store_true',help='Follow actual30B960 adapters through+8, at most8nodes')
    parser.add_argument('--leave-policy',choices=('canonical-spawn','census'),default='canonical-spawn',help='Use the real initial player spawn for a safe outside-field leave input, or the declared census point')
    parser.add_argument('--fresh-world-per-case',action='store_true',help='Isolate lifecycle-sensitive cases in independent canonical worlds')
    parser.add_argument('--timeout-seconds', type=float, default=300)
    args = parser.parse_args()
    for number in (signal.SIGINT, signal.SIGTERM):signal.signal(number, interrupted)
    if args.entrants == 2 and args.players and 1 in args.players:parser.error('Two entrants require a two-player canonical world')
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run this audit through tools/run_test.py')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True);source = args.source_root.resolve(strict=True)
    if runtime.name != 'halo2_native':parser.error('Select the exact native game ELF, never a launcher')
    if args.owned_build_inventory:
        args.owned_build_inventory=args.owned_build_inventory.resolve(strict=True)
        cache=runtime.parent/'CMakeCache.txt'
        if (args.owned_build_inventory.parent!=runtime.parent or not cache.is_file() or
                'CMAKE_HOME_DIRECTORY:INTERNAL='+str(source) not in cache.read_text().splitlines() or
                not runtime.parent.parent.name.startswith('halo2-test-')):
            parser.error('Owned diagnostics require the declared source/inventory/CMake build pair in run_test.py storage')
        runtime_kind='owned_frozen_build_diagnostic'
    else:
        if not(runtime.parent/'qualification.json').is_file():parser.error('Select an existing qualified MCC runtime or explicitly declared owned build')
        qualification=json.loads((runtime.parent/'qualification.json').read_text())
        if qualification.get('passed') is not True or qualification.get('runtime_sha256')!=digest(runtime):
            parser.error('The selected runtime must match its passed qualification ELF pin')
        runtime_kind='qualified_candidate'
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds <= 0:parser.error('A finite positive timeout is required')
    if any(address<0 or address+12>2**32 for address in args.capture_source_address):parser.error('Source captures must fit the original address space')
    args.report_output.mkdir(parents=True, exist_ok=True)
    input_bytes = args.case_json.read_bytes()
    (args.report_output / 'declared-cases.json').write_bytes(input_bytes)
    (args.report_output / 'audit-tool.py').write_bytes(Path(__file__).read_bytes())
    data = json.loads(input_bytes);declared = data['cases'] if isinstance(data, dict) else data
    selected = [c for c in declared if (not args.map or c['map'] in args.map) and (not args.case or c['id'] in args.case)]
    included = [c for c in selected if not args.positive_only or c.get('classification') == 'actual_force_lift']
    excluded = [c for c in selected if c not in included]
    ids = [c['id'] for c in included]
    if len(set(ids)) != len(ids):parser.error('Census case IDs must be unique')
    for row in included:
        if not re.fullmatch(r'[a-z0-9_]+',row['map']):parser.error('Census maps must be direct MCC map stems')
        position_names = ['placement_position', 'entrance', 'leave_position']
        if args.entrants == 2:position_names.append('second_entrance')
        if 'second_leave_position' in row:position_names.append('second_leave_position')
        for name in position_names:
            if name not in row or len(row[name]) != 3 or not all(math.isfinite(x) for x in row[name]):parser.error('Census '+name+' must be an explicit finite Vec3 input')
        if isinstance(row['definition_datum'], str):row['definition_datum'] = int(row['definition_datum'], 0)
        if args.create_initially_absent and 'authored_not_automatically_placed_or_never_placed' in row.get('exclusion_reasons', []):
            if not isinstance(row.get('content_placement_index'), int):parser.error('Declared birth requires an exact authored content placement index')
            row['allow_declared_birth'] = True
    pins = {name: digest(ROOT / name) for name in ('Halo2-Launcher', 'Halo2-Title-Launcher', 'gameplay-runtime-selection.json', 'frontend-runtime-selection.json') if (ROOT / name).is_file()}
    runtime_pin = digest(runtime)
    report = {'schema': 'H2_MCC_AUTHORED_GRAVLIFT_CONTACT_AUDIT_V1', 'passed': False,
              'runtime': str(runtime), 'runtime_sha256': runtime_pin,'runtime_kind':runtime_kind,
              'source_inventory':str(args.owned_build_inventory or runtime.parent/'source-hashes.json'),
              'source_inventory_sha256':digest(args.owned_build_inventory or runtime.parent/'source-hashes.json'),
              'case_json_sha256': hashlib.sha256(input_bytes).hexdigest(),
              'tool_sha256': digest(Path(__file__)), 'excluded_cases': excluded, 'worlds': [],
              'publication_files_before_sha256': pins, 'publication_executed': False,
              'owned_scratch': str(owner), 'retained_large_artifacts': [],
              'entered_slots':list(range(args.entrants)),
              'other_local_players':('Canonical neutral controls; simultaneous entry is a separate coverage gate.' if args.entrants==1 else
                    'Both distinct full native player generations receive declared position inputs; simultaneous exact membership is required on entry and reentry.'),
              'scope': 'Canonical MCC worlds; genuine position inputs and explicitly declared authored births; exact field/phantom/entrant source membership and aggregate native lift scalar activity, with actual completed tick readback. No supplied force, velocity, admission or host pose overwrite.'}
    summary = args.report_output / 'report.json';write_json(summary, report)
    with tempfile.TemporaryDirectory(prefix='mcc-lift-audit-', dir=owner) as directory:
        scratch = Path(directory);layout = abi_layout(source, runtime, scratch,args.owned_build_inventory)
        write_json(args.report_output / 'abi-layout.json', layout)
        maps = list(dict.fromkeys(m for m in args.map if any(c['map']==m for c in included))) if args.map else list(dict.fromkeys(c['map'] for c in included))
        for map_name in maps:
            for players in list(dict.fromkeys(args.players or ([2] if args.entrants==2 else [1,2]))):
                remaining = [c for c in included if c['map'] == map_name]
                attempt = 0
                while remaining:
                    attempt += 1
                    label = map_name + '-' + str(players) + '-' + str(attempt)
                    case_scratch = scratch / label;case_scratch.mkdir()
                    batch=remaining[:1]if args.fresh_world_per_case else remaining
                    try:
                        if digest(runtime)!=runtime_pin:raise RuntimeError('Declared runtime ELF changed before this world; no new ABI is admitted')
                        result = run_world(runtime, args.maps.resolve(strict=True), source, batch, players,
                            args.rendered, layout, case_scratch, args.timeout_seconds, args.report_output / (label + '.json'),args.capture_source_address,args.capture_collector_chain,args.leave_policy,args.entrants)
                    except (OSError,ValueError,RuntimeError,subprocess.SubprocessError) as error:
                        result={'map':map_name,'players':players,'rendered':args.rendered,'passed':False,
                            'cases':[],'gdb_exit_code':None,'child_exit_code':None,'fatal_errors':[],
                            'audit_execution_error':str(error),'attempted_case_ids':[c['id']for c in batch]}
                        write_json(args.report_output/(label+'.json'),result)
                    compact={key:value for key,value in result.items()if key not in('cases','observed','diagnostic_tail')}
                    compact['cases']=[{key:value for key,value in row.items()if key!='observed'}for row in result['cases']]
                    compact['world_report']=str(args.report_output/(label+'.json'));compact['world_report_sha256']=digest(args.report_output/(label+'.json'))
                    report['worlds'].append(compact);write_json(summary, report)
                    print(json.dumps({'map': map_name, 'players': players, 'attempt': attempt, 'passed': result['passed'],
                        'completed': len(result['cases']), 'requested': len(remaining), 'gdb_exit_code': result['gdb_exit_code']}), flush=True)
                    completed = {row['id'] for row in result['cases']}
                    if not completed:
                        report.setdefault('unreached_cases', []).append({'id': remaining[0]['id'], 'map': map_name,
                            'players': players, 'reason': 'Native startup or audit bootstrap failed; see world report/log'})
                        completed.add(remaining[0]['id'])
                    remaining = [c for c in remaining if c['id'] not in completed]
    report['publication_files_unchanged'] = all(digest(ROOT / name) == pin for name, pin in pins.items())
    report['runtime_unchanged'] = digest(runtime) == runtime_pin
    report['private_audit_directory_removed'] = not scratch.exists()
    report['passed'] = bool(included) and all(w['passed'] for w in report['worlds']) and not report.get('unreached_cases') and report['publication_files_unchanged'] and report['runtime_unchanged']
    report['crash_safety_passed'] = bool(included) and all(w.get('crash_safety_passed',False) for w in report['worlds']) and not report.get('unreached_cases') and report['publication_files_unchanged'] and report['runtime_unchanged']
    write_json(summary, report)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
