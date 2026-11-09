#!/usr/bin/env python3
"""Bounded canonical MCC campaign field contact scan on an exact native ELF.

Use tools/run_test.py. The only gameplay mutation is a declared real B95D0
position input after canonical campaign player birth. Scripts, device power,
BSP admission, object birth and native guards remain authoritative. A field
without observed source membership is never reported as contact-qualified.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import re
import signal
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[1]

def sha(path):
 with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()

def save(path,value):
 temp=path.with_suffix(path.suffix+'.tmp');temp.write_text(json.dumps(value,indent=2)+'\n');temp.replace(path)

def layout(source,inventory,scratch):
 names=('include/halo2/simulation.hpp','include/halo2/collision.hpp',
     'include/halo2/campaign_source_runtime.hpp','include/halo2/physics_live_world_tu15.hpp',
     'include/halo2/native_tu15_physics_frame_residuals.hpp','include/halo2/source_runtime_memory.hpp',
     'src/campaign/campaign_source_runtime.cpp','src/gameplay/multiplayer/multiplayer_object_runtime.cpp',
     'src/app/main.cpp','include/halo2/content.hpp','include/halo2/native_tu15_support_carrier.hpp',
     'include/halo2/native_tu15_support_carrier_handoff.hpp')
 pins={n:sha(source/n)for n in names}
 if any(inventory.get(n)!=h for n,h in pins.items()):raise ValueError('Campaign ABI/source owners do not match the exact runtime inventory')
 fields={'unit_source':'offsetof(Unit,source_datum)','unit_position':'offsetof(Unit,body)+offsetof(Body,position)',
     'unit_velocity':'offsetof(Unit,body)+offsetof(Body,velocity)','simulation_player':'offsetof(Simulation,player_)',
     'simulation_tick':'offsetof(Simulation,ticks_)','simulation_bsp':'offsetof(Simulation,bsp_)',
     'placement_type':'offsetof(ScenarioObject,type)','placement_index':'offsetof(ScenarioObject,placement)',
     'placement_definition':'offsetof(ScenarioObject,definition)',
     'stats_size':'sizeof(NativeTu15PhysicsFrameExecutionStats)',
     'native_lift_scalar_calls':'offsetof(NativeTu15PhysicsFrameExecutionStats,native_lift_scalar_calls)',
     'native_lift_observation_refusals':'offsetof(NativeTu15PhysicsFrameExecutionStats,native_lift_observation_refusals)'}
 fields.update(carrier_request_size='sizeof(NativeTu15SupportCarrierRequest)',carrier_component='offsetof(NativeTu15SupportCarrierRequest,component_datum)')
 code='#include "halo2/campaign_source_runtime.hpp"\n#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include "halo2/native_tu15_support_carrier.hpp"\n#include <cstddef>\n#include <iostream>\nint main(){using namespace halo2;std::cout<<"{"'
 for i,(name,expr)in enumerate(fields.items()):code+='<<"'+(','if i else'')+'\\"'+name+'\\":"<<('+expr+')'
 code+='<<"}\\n";}\n';cpp=scratch/'campaign-abi.cpp';binary=scratch/'campaign-abi';cpp.write_text(code)
 command=['c++','-std=c++20','-fno-access-control','-Wno-invalid-offset','-I'+str(source/'include'),'-I/usr/include/SDL2',str(cpp),'-o',str(binary)]
 result=subprocess.run(command,capture_output=True,text=True,timeout=90)
 if result.returncode:raise RuntimeError('Campaign ABI compilation failed: '+result.stderr[-8000:])
 return dict(layout=json.loads(subprocess.check_output([str(binary)],text=True)),source_hashes=pins,
     compiler_command=command,compiler_source=code,scope='Read-only equivalent of inlined Simulation player/tick/BSP getters, cross-checked against actual adopt_unit/bind_player seams.')

SCRIPT=r'''set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
set $runtime=(void*)0
set $world=(void*)0
set $simulation=(void*)0
set $body=(void*)0
set $unit=4294967295
set $bootstrapped=0
set $scan=0
set $do_input=0
set $do_stats=0
set $active=0
set $audit_failed=0
python
import gdb,json,struct,math
plan=PLAN
layout=LAYOUT
requested_bsp=REQUESTED_BSP
entry_ticks=ENTRY_TICKS
leave_ticks=LEAVE_TICKS
startup_ticks=STARTUP_TICKS
cases=[];fields={};errors=[];throws=[];native_throws=[];binds=[];births=[];done=False;active=None;index=0
carrier_calls={};carrier_handoffs={};carrier_events=[];carrier_unavailable=[]
phase=None;phase_count=0;last_tick=None;spawn=None;stats_label='';neutral_last=None;neutral_completed=0
def v(name):return int(gdb.parse_and_eval('$'+name))
def setv(name,x):gdb.set_convenience_variable(name,gdb.Value(x))
def raw(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def guarded(operation):
 try:operation()
 except(gdb.error,MemoryError,RuntimeError,ValueError,struct.error)as error:
  message=str(error)
  if len(errors)<20:errors.append(message)
  if active is not None:active['status']='audit-readback-error';active['errors'].append(message)
  for name in('active','do_input','do_stats','ready'):setv(name,0)
  setv('audit_failed',1);setv('scan',2048)
def word(p):return struct.unpack('<I',raw(p,4))[0]
def u16(p):return struct.unpack('<H',raw(p,2))[0]
def vec(p):return list(struct.unpack('<fff',raw(p,12)))
def tick():return struct.unpack('<Q',raw(v('simulation')+layout['simulation_tick'],8))[0]
def bsp():return struct.unpack('<Q',raw(v('simulation')+layout['simulation_bsp'],8))[0]
def bound():
 binds.append({'player':v('player'),'unit':v('unit'),'runtime':v('runtime')})
def adopted():
 p=int(gdb.parse_and_eval('$rsi'))
 if word(p+layout['unit_source'])==v('unit'):setv('body',p)
class BirthReturn(gdb.FinishBreakpoint):
 def __init__(self,identity):
  super().__init__(gdb.newest_frame(),internal=True);self.identity=identity
 def stop(self):
  row=dict(self.identity);row['object']=int(gdb.parse_and_eval('$eax'))&0xffffffff
  if row['object']!=0xffffffff:births.append(row)
  return False
class BirthEntry(gdb.Breakpoint):
 def stop(self):
  try:
   pointer=int(gdb.parse_and_eval('$rsi'))
   identity={'type':word(pointer+layout['placement_type']),'index':word(pointer+layout['placement_index']),
    'definition':word(pointer+layout['placement_definition'])}
   BirthReturn(identity)
  except(gdb.error,MemoryError,ValueError)as error:errors.append('Canonical authored birth capture: '+str(error))
  return False
BirthEntry('halo2::PhysicsLiveWorldTu15::create_scenario_object',internal=True)
def boot():
 global spawn
 if v('body')!=v('simulation')+layout['simulation_player']:raise RuntimeError('Actual adopt_unit did not identify the canonical Simulation player')
 if word(v('body')+layout['unit_source'])!=v('unit'):raise RuntimeError('Actual bound player unit differs from adopted Unit')
 pool=v('object_pool')
 if word(pool+0x24)!=12 or word(pool+0x38)>2048:raise RuntimeError('Campaign object pool ABI differs')
 spawn=vec(v('body')+layout['unit_position'])
def ready():
 global neutral_last,neutral_completed
 now=tick()
 if neutral_last is not None:
  if now==neutral_last+1:neutral_completed+=1
  elif now!=neutral_last:neutral_completed=0
 neutral_last=now
 setv('ready',int(neutral_completed>=2 and now>=startup_ticks and bsp()==requested_bsp))
def choose_header():
 p=v('headers')+v('scan')*12;salt=u16(p)
 setv('valid',int(salt!=0 and word(p+8)!=0));setv('object',(salt<<16)|v('scan'))
def remember():
 p=v('source_body');definition=word(p)
 if definition in {c['definition_datum']for c in plan}:
  fields.setdefault(definition,[]).append({'object':v('object'),'body':p,'position':vec(p+0x64)})
def prepare(point):
 gdb.set_convenience_variable('input_xy',gdb.Value(struct.unpack('<d',struct.pack('<ff',*point[:2]))[0]))
 gdb.set_convenience_variable('input_z',gdb.Value(float(point[2])));setv('do_input',1)
def block(status,message):
 active['status']=status;active['errors'].append(message);next_case()
def next_case():
 global index,active,phase,phase_count,last_tick,stats_label,done
 setv('active',0);setv('do_input',0);setv('do_stats',0)
 while index<len(plan):
  row=plan[index];index+=1
  active={'id':row['id'],'case':row,'unit':v('unit'),'status':'unvisited','samples':[],'phases':{},'errors':[],'inputs':[]}
  cases.append(active)
  if bsp()!=requested_bsp:
   active['status']='campaign-bsp-changed';active['errors'].append('Authored scripts changed BSP; no input supplied in another BSP');continue
  identities=[b for b in births if b['type']==row['placement_type']and b['index']==row['placement']['index']and b['definition']==row['definition_datum']]
  if not identities:
   active['status']='authored-object-absent';active['errors'].append('No actual canonical create_scenario_object return established this exact authored placement');continue
  identity=identities[-1];active['authored_birth']=identity
  candidates=fields.get(row['definition_datum'],[])
  target=next((c for c in candidates if c['object']==identity['object']),None)
  if target is None:
   active['status']='authored-object-absent';active['errors'].append('No genuine current field object with the authored definition was observed');continue
  distance=math.dist(target['position'],row['placement_position']);active['field']=target;active['placement_distance']=distance
  if distance>.25:
   active['status']='field-pose-unresolved';active['errors'].append('Current definition instance does not match the authored initial placement');continue
  source_header=v('headers')+(target['object']&65535)*12
  if u16(source_header)!=(target['object']>>16)or word(source_header+8)==0:
   active['status']='field-lifecycle-changed';active['errors'].append('Authored field retired before its probe');continue
  physical=word(target['body']+0xb4);active['physical_component']=physical
  if physical==0xffffffff:
   active['status']='field-component-absent';active['errors'].append('Current field has no native physics component');continue
  header=v('physical_header');slot=physical&65535
  if slot>=word(header+0x20)or word(header+0x24)!=0xa0:raise RuntimeError('Campaign physical component pool ABI differs')
  setv('component_address',word(header+0x44)+slot*0xa0)
  phase='entry';phase_count=0;last_tick=tick();stats_label='entry-before';active['status']='running'
  prepare(row['entrance']);setv('active',1);return
 done=True;active=None
def stats():
 p=v('stats');active['phases'][stats_label]={'native_lift_scalar_calls':list(struct.unpack('<4Q',raw(p+layout['native_lift_scalar_calls'],32))),
  'native_lift_observation_refusals':struct.unpack('<Q',raw(p+layout['native_lift_observation_refusals'],8))[0]}
def input_done():
 position=vec(v('native_body')+0x64);expected=spawn if phase=='leave'else active['case']['entrance']
 active['inputs'].append({'phase':phase,'tick':tick(),'requested':expected,'source_position':position})
 if math.dist(position,expected)>.1:active['errors'].append('The genuine B95D0 input did not produce the requested source location')
 setv('do_input',0)
def observe():
 global phase_count,last_tick,stats_label
 if active is None or v('do_input'):return
 now=tick()
 if now==last_tick:return
 if now<last_tick:block('campaign-clock-reverted','Actual campaign tick reverted/checkpoint lifecycle changed');return
 if now!=last_tick+1:block('campaign-tick-gap','Observation lost a completed canonical tick');return
 last_tick=now
 if bsp()!=requested_bsp:block('campaign-bsp-changed','Authored scripts changed the active BSP during this field probe');return
 if v('unit')!=active['unit']or word(v('body')+layout['unit_source'])!=active['unit']:
  block('player-lifecycle-changed','Canonical campaign player generation changed');return
 p=v('component');physical=active['physical_component']
 if u16(p)!=(physical>>16)or word(p+8)!=active['field']['object']:
  block('field-lifecycle-changed','Selected component salt/field owner changed');return
 source_header=v('headers')+(active['field']['object']&65535)*12
 if u16(source_header)!=(active['field']['object']>>16)or word(source_header+8)==0:
  block('field-lifecycle-changed','Exact authored field object retired during its probe');return
 if word(v('native_body')+0x14)!=0xffffffff:
  block('entrant-parented','Authored campaign currently parents/seats the entrant; no detach was supplied');return
 position=vec(v('body')+layout['unit_position']);velocity=vec(v('body')+layout['unit_velocity'])
 native_position=vec(v('native_body')+0x64);native_velocity=vec(v('native_body')+0x88)
 if not all(math.isfinite(x)for x in position+velocity+native_position+native_velocity):raise RuntimeError('Nonfinite completed campaign source/adopted pose')
 if math.dist(position,native_position)>.001 or math.dist(velocity,native_velocity)>.001:
  active['pose_transition']={'tick':now,'adopted_position':position,'source_position':native_position,
   'adopted_velocity':velocity,'source_velocity':native_velocity}
  block('campaign-pose-readback-transition','Completed outer tick contains an adopted/source pose transition; script/teleport/checkpoint timing is not qualified');return
 matched=[]
 for n in range(v('member_count')):
  row=raw(v('members')+n*8,8)
  if row[0]==active['case']['membership_phantom_index']and struct.unpack_from('<I',row,4)[0]==active['unit']:
   matched.append({'row_hex':row.hex(),'references':row[2],'other_body_slot':row[1],'field_body_slot':row[3]})
 field_body=active['field']['body'];field_position=vec(field_body+0x64)
 field_forward=vec(field_body+0x70);field_up=vec(field_body+0x7c)
 if not all(math.isfinite(x)for x in field_position+field_forward+field_up):raise RuntimeError('Nonfinite genuine field root transform')
 active['samples'].append({'phase':phase,'tick':now,'bsp':bsp(),'position':position,'velocity':velocity,
  'field_root_position':field_position,'field_root_forward':field_forward,'field_root_up':field_up,
  'selected_field_member':bool(matched),'membership_rows':matched})
 phase_count+=1
 if phase_count>=(leave_ticks if phase=='leave'else entry_ticks):stats_label=phase+'-after';setv('do_stats',1)
def advance():
 global phase,phase_count,last_tick,stats_label
 setv('do_stats',0)
 if phase=='entry':
  phase='leave';phase_count=0;last_tick=tick();stats_label='leave-before';prepare(spawn)
 elif phase=='leave':
  phase='reentry';phase_count=0;last_tick=tick();stats_label='reentry-before';prepare(active['case']['entrance'])
 else:active['status']='completed';next_case()
class CarrierReturned(gdb.FinishBreakpoint):
 def __init__(self,case,phase,component):
  super().__init__(gdb.newest_frame(),internal=True);self.case=case;self.phase=phase;self.component=component
 def stop(self):
  key=self.case+':'+self.phase;counts=carrier_calls[key];counts['returns']+=1
  success=bool(int(gdb.parse_and_eval('$eax'))&255);counts['positive_returns']+=int(success)
  if len(carrier_events)<12:carrier_events.append({'case':self.case,'phase':self.phase,'component_datum':self.component,'positive_return':success})
  return False
 def out_of_scope(self):errors.append('Actual native184EC0 support owner unwound before return')
class CarrierHandoffReturned(gdb.FinishBreakpoint):
 def __init__(self,frame,key):super().__init__(frame,internal=True);self.key=key
 def stop(self):
  counts=carrier_handoffs[self.key];counts['returns']+=1;counts['positive_returns']+=int(bool(int(gdb.parse_and_eval('$eax'))&255));return False
 def out_of_scope(self):errors.append('Actual native184EC0 handoff unwound before return')
class CarrierEntry(gdb.Breakpoint):
 def stop(self):
  if active is None:return False
  try:
   if layout['carrier_request_size']!=32 or layout['carrier_component']!=0:raise RuntimeError('Sealed support carrier input ABI changed')
   request=int(gdb.parse_and_eval('$rsi'));component=struct.unpack('<I',raw(request,4))[0]
   if component!=active.get('physical_component'):return False
   key=active['id']+':'+str(phase);counts=carrier_calls.setdefault(key,{'entries':0,'returns':0,'positive_returns':0});counts['entries']+=1
   parent=gdb.newest_frame().older()
   if parent is None or 'native_tu15_handoff_support_carrier'not in (parent.name()or''):raise RuntimeError('Actual native184EC0 core has no authenticated handoff parent')
   counts=carrier_handoffs.setdefault(key,{'entries':0,'returns':0,'positive_returns':0});counts['entries']+=1;CarrierHandoffReturned(parent,key)
   CarrierReturned(active['id'],str(phase),component)
  except (gdb.error,RuntimeError,MemoryError,ValueError)as error:errors.append('Native184EC0 probe: '+str(error))
  return False
try:CarrierEntry('halo2::native_tu15_update_support_carrier',internal=True)
except gdb.error:carrier_unavailable.append('native_tu15_update_support_carrier')
class Throws(gdb.Breakpoint):
 def stop(self):
  try:
   pointer=struct.unpack('<Q',raw(int(gdb.parse_and_eval('$rdi'))+8,8))[0]
   message=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(length=1024,errors='replace').split(chr(0))[0]
   item={'message':message,'case':active['id']if active else None,'tick':tick()if v('simulation')else None}
   if 'TU1.5'in message or 'world owner'in message or 'Native memory pressure'in message:
    if len(native_throws)<20:
     item['backtrace']=gdb.execute('bt 16',to_string=True)[-12000:];native_throws.append(item)
   elif len(throws)<20:throws.append(item)
  except (gdb.error,MemoryError,ValueError):pass
  return False
Throws('__cxa_throw',internal=True)
end
define capture_activity
set $stats=(void*)malloc(STATS_SIZE)
call ((void(*)(void*,void*)) 'halo2::PhysicsLiveWorldTu15::native_physics_frame_activity() const')($stats,$world)
python guarded(stats)
call ((void(*)(void*)) 'halo2::NativeTu15PhysicsFrameExecutionStats::~NativeTu15PhysicsFrameExecutionStats()')($stats)
call ((void(*)(void*))free)($stats)
end
break halo2::MultiplayerObjectRuntime::bind_player
commands
silent
set $runtime=(void*)$rdi
set $player=(unsigned int)$esi
set $unit=(unsigned int)$edx
python guarded(bound)
continue
end
break halo2::CampaignSourceRuntime::adopt_unit
commands
silent
python guarded(adopted)
continue
end
break halo2::CampaignSourceRuntime::step
commands
silent
set $simulation=(void*)$rsi
if $runtime != 0 && $body != 0 && $unit != 4294967295 && $audit_failed == 0
if $bootstrapped == 0
set $world=((void*(*)(void*)) 'halo2::MultiplayerObjectRuntime::tu15_world()')($runtime)
set $memory=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::memory()')($world)
set $object_pool=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::object_pool() const')($world)
set $headers=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)((char*)$object_pool+0x44)),*((unsigned int*)((char*)$object_pool+0x38))*12)
set $physical_global=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,0x52be5c,4)
set $physical_header=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)$physical_global),0x4c)
python guarded(boot)
set $bootstrapped=1
end
if $bootstrapped == 1
python guarded(ready)
if $ready
while $scan < *((unsigned int*)((char*)$object_pool+0x38))
python guarded(choose_header)
if $valid
set $source_body=((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,$object)
python guarded(remember)
end
set $scan=$scan+1
end
set $bootstrapped=2
python guarded(next_case)
end
end
if $active
set $native_body=((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,$unit)
if $do_input == 0
set $member_header=*((unsigned int*)((char*)$component+0x94))
set $member_count=0
set $members=(void*)0
if $member_header != 0
set $mh=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$member_header,8)
set $member_count=*((unsigned int*)((char*)$mh+4))
if $member_count > 4096
echo Invalid campaign membership extent\n
quit 2
end
if $member_count > 0
set $members=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,*((unsigned int*)$mh),$member_count*8)
end
end
python guarded(observe)
if $active && $do_stats
capture_activity
python guarded(advance)
end
end
if $active && $do_input
set $component=((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,$component_address,0xa0)
if *((unsigned int*)((char*)$native_body+0x14)) != 4294967295
python guarded(lambda:block('entrant-parented','Canonical player is attached/seated; no position input or detach was supplied'))
else
capture_activity
call ((void(*)(void*,unsigned int,double,float)) 'halo2::PhysicsLiveWorldTu15::set_object_position(unsigned int, halo2::Vec3)')($world,$unit,$input_xy,$input_z)
set $native_body=((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,$unit)
python guarded(input_done)
end
end
end
end
continue
end
run
if $_isvoid($_exitcode)
bt 16
else
printf "CAMPAIGN_FIELD_CHILD_EXIT=%d\n",$_exitcode
end
python
print('CAMPAIGN_FIELD_RESULT='+json.dumps({'cases':cases,'done':done,'errors':errors,'caught_other_exceptions':throws,'native_exceptions':native_throws,'bind_events':binds,'authored_births':births,'neutral_completed_ticks':neutral_completed,'bootstrapped':v('bootstrapped'),'native_support_carrier_calls':carrier_calls,'native_support_carrier_handoffs':carrier_handoffs,'native_support_carrier_events':carrier_events,'native_support_carrier_unavailable':carrier_unavailable}),flush=True)
end
'''

def evaluate(row,entry_ticks,leave_ticks):
 samples=row.pop('samples');errors=list(row['errors']);phases={}
 for phase,required in(('entry',entry_ticks),('leave',leave_ticks),('reentry',entry_ticks)):
  selected=[s for s in samples if s['phase']==phase];members=sum(s['selected_field_member']for s in selected)
  before=row['phases'].get(phase+'-before',{});after=row['phases'].get(phase+'-after',{})
  scalar=sum(after.get('native_lift_scalar_calls',[]))-sum(before.get('native_lift_scalar_calls',[]))if before and after else None
  phases[phase]={'completed_ticks':len(selected),'required_ticks':required,'selected_member_ticks':members,'aggregate_world_scalar_calls':scalar}
  if after.get('native_lift_observation_refusals',0)!=before.get('native_lift_observation_refusals',0):errors.append('Native lift observation refusal changed')
 finite_complete=row['status']=='completed'and all(phases[p]['completed_ticks']>=n for p,n in(('entry',entry_ticks),('leave',leave_ticks),('reentry',entry_ticks)))and not errors
 contact=finite_complete and phases['entry']['selected_member_ticks']>0 and phases['reentry']['selected_member_ticks']>0 and phases['leave']['selected_member_ticks']<leave_ticks
 status=row['status']
 if finite_complete and not contact:
  status='authored-biped-filter-unconfirmed'if row['case']['force_flags']&32 else'contact-not-observed-at-authored-pose'
 row.update(status=status,phases=phases,errors=errors,finite_completed_probe=finite_complete,
     contact_qualified=contact,passed=contact,completed_sample_count=len(samples),
     samples_sha256=hashlib.sha256(json.dumps(samples,separators=(',',':')).encode()).hexdigest(),
     sample_examples=[s for phase in('entry','leave','reentry')for s in([x for x in samples if x['phase']==phase][:1]+[x for x in samples if x['phase']==phase][-1:])],
     force_attributed_to_selected_rule=False,lift_travel_qualified=False)
 return row

def world(args,source,cases,map_name,bsp,abi,scratch):
 map_path=args.maps/(map_name+'.map');pin=sha(map_path)
 if any(c['map_sha256']!=pin for c in cases):raise ValueError('Campaign map differs from census')
 replacements={'PLAN':repr(cases),'LAYOUT':repr(abi['layout']),'REQUESTED_BSP':str(bsp),
     'ENTRY_TICKS':str(args.entry_ticks),'LEAVE_TICKS':str(args.leave_ticks),'STARTUP_TICKS':str(args.startup_ticks),'STATS_SIZE':str(abi['layout']['stats_size'])}
 script=SCRIPT
 for key,value in replacements.items():script=script.replace(key,value)
 gdb=scratch/'campaign-fields.gdb';gdb.write_text(script)
 ticks=min(args.max_world_ticks,args.startup_ticks+len(cases)*(args.entry_ticks*2+args.leave_ticks+3)+4)
 command=['gdb','--batch','-x',str(gdb),'--args',str(args.runtime),'--campaign','--map',str(map_path),
     '--shared-maps',str(args.maps),'--bsp',str(bsp),'--simulate',str(max(ticks,args.startup_ticks+4)),'--mute']
 env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')}
 env.update(HALO2_TEST_SCRATCH=str(scratch),SDL_AUDIODRIVER='dummy',
     XDG_CONFIG_HOME=str(scratch/'config'),XDG_DATA_HOME=str(scratch/'data'),XDG_CACHE_HOME=str(scratch/'cache'))
 log=scratch/'runtime.log';started=time.monotonic()
 with log.open('w')as output:
  process=subprocess.Popen(command,cwd=source,env=env,stdout=output,stderr=subprocess.STDOUT,start_new_session=True)
  try:code=process.wait(timeout=args.timeout_seconds)
  except subprocess.TimeoutExpired:
   os.killpg(process.pid,signal.SIGTERM)
   try:process.wait(timeout=5)
   except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
   code=124
 text=log.read_text(errors='replace');matches=re.findall(r'^CAMPAIGN_FIELD_RESULT=(.*)$',text,re.MULTILINE)
 observed=json.loads(matches[-1])if matches else{}
 child=re.findall(r'CAMPAIGN_FIELD_CHILD_EXIT=(\d+)',text);child=int(child[-1])if child else None
 fatal=[line for line in text.splitlines()if line.startswith(('error:','Script fault:'))]
 statuses=re.findall(r'^.*\btick=\d+.*$',text,re.MULTILINE)
 if 'BSP has no authored player start'in text:blocker='bsp-no-authored-player-start'
 elif any(x.startswith('Script fault:')for x in fatal):blocker='authored-script-fault'
 elif code==124:blocker='bounded-world-timeout'
 elif any('Native memory pressure'in x for x in fatal):blocker='native-memory-pressure-blocked'
 elif any('world owner missing'in x for x in fatal):blocker='native-owner-startup-or-world-failure'
 elif child is None:blocker='audit-debugger-or-native-stop'
 elif child!=0:blocker='native-world-or-startup-failure'
 elif observed.get('errors'):blocker='audit-readback-error'
 elif not observed.get('bind_events'):blocker='canonical-player-not-bound'
 elif observed.get('bootstrapped',0)<2:blocker='campaign-tick-bsp-or-presentation-blocked'
 elif observed.get('done'):blocker=None
 else:blocker='world-budget-ended-before-case'
 rows=[evaluate(row,args.entry_ticks,args.leave_ticks)for row in observed.get('cases',[])]
 visited={r['id']for r in rows}
 for case in cases:
  if case['id']not in visited:rows.append(dict(id=case['id'],case=case,status=blocker,passed=False,contact_qualified=False,finite_completed_probe=False,errors=['Declared field was not visited'],completed_sample_count=0))
 for row in rows:
  if row['status']=='running':
   row['status']=blocker or'world-ended-during-probe';row['errors'].append('Canonical world stopped before the declared probe phases completed')
  row['clean_world_exit']=code==0 and child==0 and not fatal and not observed.get('errors')
  if not row['clean_world_exit']:row['passed']=False;row['contact_qualified']=False
 result=dict(map=map_name,bsp=bsp,runtime_sha256=args.runtime_sha256,map_sha256=pin,command=command,
     gdb_script=script,gdb_exit_code=code,child_exit_code=child,seconds=round(time.monotonic()-started,3),
     declared_case_count=len(cases),cases=rows,observed={k:v for k,v in observed.items()if k!='cases'},
     fatal_errors=fatal,last_status=statuses[-3:],blocker=blocker,
     failure_context=('during-field-probe'if any(r.get('completed_sample_count',0)for r in rows)else'canonical-startup-or-script-progression')if fatal or child!=0 else None,
     startup_probe_completed=child==0 and code==0 and not fatal,passed=bool(cases)and all(r['passed']for r in rows),
     scope='Canonical authored campaign construction; source membership and completed finite body/adopt readback only. No force attribution, directional travel, script progression or general parity claim.')
 return result,(('\n'.join(fatal)+'\n'+text[-24000:])[-32000:])

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in('runtime','source-root','source-hashes','maps','census','membership-adapter','report-output'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--runtime-sha256',required=True);p.add_argument('--baseline-mechanics-only',action='store_true')
 p.add_argument('--map',action='append');p.add_argument('--bsp',action='append',type=int)
 p.add_argument('--case-id',action='append',help='Exact authored case identity; repeat to select several cases')
 p.add_argument('--entry-ticks',type=int,default=90);p.add_argument('--leave-ticks',type=int,default=30)
 p.add_argument('--startup-ticks',type=int,default=12);p.add_argument('--max-world-ticks',type=int,default=20000)
 p.add_argument('--timeout-seconds',type=float,default=180);args=p.parse_args()
 if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
 for key in('entry_ticks','leave_ticks','startup_ticks','max_world_ticks'):
  if not 1<=getattr(args,key)<=50000:p.error('Require bounded positive tick counts')
 if not math.isfinite(args.timeout_seconds)or not 1<=args.timeout_seconds<=600:p.error('Require finite1..600second world timeout')
 args.runtime=args.runtime.resolve(strict=True);args.maps=args.maps.resolve(strict=True);source=args.source_root.resolve(strict=True)
 if sha(args.runtime)!=args.runtime_sha256:p.error('Exact runtime ELF pin differs')
 if args.runtime.name!='halo2_native':p.error('Require the exact native game ELF')
 args.source_hashes=args.source_hashes.resolve(strict=True)
 cache=args.runtime.parent/'CMakeCache.txt';qualification=args.runtime.parent/'qualification.json'
 if (args.source_hashes.parent==args.runtime.parent and cache.is_file()
     and 'CMAKE_HOME_DIRECTORY:INTERNAL='+str(source)in cache.read_text().splitlines()
     and args.runtime.parent.parent.name.startswith('halo2-test-')):runtime_kind='owned_frozen_build_diagnostic'
 elif qualification.is_file():
  qualified=json.loads(qualification.read_text())
  if qualified.get('passed')is not True or qualified.get('runtime_sha256')!=args.runtime_sha256:p.error('Qualified candidate runtime pin differs')
  runtime_kind='qualified_candidate'
 else:p.error('Require a passed candidate or explicit matching active owned source/CMake/inventory pair')
 inventory=json.loads(args.source_hashes.read_text());summary=json.loads((args.census/'summary.json').read_text())
 adapter=json.loads(args.membership_adapter.read_text());membership={row['case_id']:row for row in adapter['cases']}
 maps=[m['map']for m in summary['maps']if m['cache_type']==0 and(not args.map or m['map']in args.map)]
 if args.map and set(args.map)!=set(maps):p.error('Every requested map must be an available canonical campaign census map')
 if args.case_id and (len(set(args.case_id))!=len(args.case_id)or any(c not in membership or membership[c]['map']not in maps for c in args.case_id)):
  p.error('Every requested case must be a unique available authored campaign case in the selected maps')
 args.report_output.mkdir(parents=True,exist_ok=True);report=dict(schema='H2_MCC_CAMPAIGN_FIELD_CONTACT_AUDIT_V1',
     runtime=str(args.runtime),runtime_sha256=args.runtime_sha256,runtime_kind=runtime_kind,
     source=str(source),source_inventory_sha256=sha(args.source_hashes),
     membership_adapter=str(args.membership_adapter.resolve()),membership_adapter_sha256=sha(args.membership_adapter),
     map_filter=args.map,bsp_filter=args.bsp,case_filter=args.case_id,
     baseline_mechanics_only=args.baseline_mechanics_only,tool_sha256=sha(__file__),map_count=len(maps),worlds=[],
     scan_completed=False,passed=False,publication_executed=False,retained_large_artifacts=[])
 (args.report_output/'audit-tool.py.txt').write_bytes(Path(__file__).read_bytes())
 report['publication_files_before_sha256']={name:sha(ROOT/name)for name in('Halo2-Launcher','Halo2-Title-Launcher','gameplay-runtime-selection.json','frontend-runtime-selection.json')if(ROOT/name).is_file()}
 save(args.report_output/'report.json',report)
 with tempfile.TemporaryDirectory(prefix='campaign-field-scan-',dir=Path(os.environ['HALO2_TEST_SCRATCH']))as directory:
  scratch=Path(directory);abi=layout(source,inventory,scratch);save(args.report_output/'abi-layout.json',abi)
  for name in maps:
   detail_path=args.census/(name+'.json');detail=json.loads(detail_path.read_text());groups={0:[]}
   for row in detail['cases']:
    if args.case_id and row['case_id']not in args.case_id:continue
    joined=membership[row['case_id']]
    if (not joined.get('membership_identity_verified')or not joined['membership_identity'].get('force_join_verified')
        or joined['membership_phantom_index']!=joined['wrapper_user_data_low']
        or joined['membership_phantom_index']!=joined['primitive_material_index']
        or joined['map_sha256']!=detail['map_sha256']or joined['definition_datum']!=row['definition_datum']
        or not 0<=joined['membership_phantom_index']<=255):raise ValueError('Every authored case requires a verified native material/wrapper membership identity')
    point=row.get('world_geometry',{}).get('inside_candidate');origin=row['instance'].get('origin_bsp',-1)
    case=dict(id=row['case_id'],map=name,map_sha256=detail['map_sha256'],definition_datum=row['definition_datum'],
        definition=row['definition'],placement=row['instance'],placement_position=row['instance']['position'],
        entrance=point,phantom_index=row['phantom_index'],membership_phantom_index=joined['membership_phantom_index'],membership_join=joined,
        placement_type={'bipd':0,'vehi':1,'weap':2,'eqip':3,'scen':6,'mach':7,'ctrl':8,'lifi':9,'ssce':10,'bloc':11,'crea':12}[row['instance']['block']],
        force_flags=(row.get('force')or{}).get('flags',0),force_index=joined['force_index'],
        classification=row.get('classification'),coverage_limits=row.get('coverage_limits',[]))
    if point is None or len(point)!=3 or not all(math.isfinite(x)for x in point):case['invalid_authored_point']=True
    groups.setdefault(max(0,origin),[]).append(case)
   for bsp,cases in sorted(groups.items()):
    if args.bsp and bsp not in args.bsp:continue
    if args.case_id and not cases:continue
    valid=[c for c in cases if not c.get('invalid_authored_point')];folder=scratch/(name+'-'+str(bsp));folder.mkdir()
    try:result,tail=world(args,source,valid,name,bsp,abi,folder)
    except (OSError,ValueError,RuntimeError,subprocess.SubprocessError)as error:
     result=dict(map=name,bsp=bsp,passed=False,cases=[dict(id=c['id'],status='audit-mechanics-error',passed=False,contact_qualified=False,finite_completed_probe=False)for c in cases],audit_error=str(error));tail=str(error)
    for c in cases:
     if c.get('invalid_authored_point'):result['cases'].append(dict(id=c['id'],status='authored-pose-unresolved',passed=False,contact_qualified=False,finite_completed_probe=False))
    result['census_detail_sha256']=sha(detail_path);label=name+'-bsp'+str(bsp)
    save(args.report_output/(label+'.json'),result);(args.report_output/(label+'.log')).write_text(tail)
    report['worlds'].append({k:v for k,v in result.items()if k not in('gdb_script','cases')})
    report['worlds'][-1]['case_status_counts']={s:sum(r['status']==s for r in result['cases'])for s in sorted({r['status']for r in result['cases']})}
    report['worlds'][-1]['case_count']=len(result['cases']);report['worlds'][-1]['contact_qualified_cases']=sum(r.get('contact_qualified',False)for r in result['cases'])
    report['worlds'][-1]['finite_completed_probes']=sum(r.get('finite_completed_probe',False)for r in result['cases'])
    report['attempted_maps']=sorted({w['map']for w in report['worlds']});save(args.report_output/'report.json',report)
    print(json.dumps({'map':name,'bsp':bsp,'cases':len(cases),'contact_qualified':report['worlds'][-1]['contact_qualified_cases'],'blocker':result.get('blocker'), 'fatal_errors':result.get('fatal_errors',[])[:1]}),flush=True)
 report.update(scan_completed=True,declared_cases=sum(w['case_count']for w in report['worlds']),
     contact_qualified_cases=sum(w['contact_qualified_cases']for w in report['worlds']),finite_completed_probes=sum(w['finite_completed_probes']for w in report['worlds']),
     runtime_unchanged=sha(args.runtime)==args.runtime_sha256,owned_scratch_removed=not scratch.exists(),
     publication_files_unchanged=all((ROOT/name).is_file()and sha(ROOT/name)==pin for name,pin in report['publication_files_before_sha256'].items()),
     attempted_map_count=len(report.get('attempted_maps',[])),expected_map_count=len(maps))
 report['requested_case_filter_resolved']=not args.case_id or report['declared_cases']==len(args.case_id)
 report['passed']=not args.baseline_mechanics_only and report['runtime_unchanged']and report['requested_case_filter_resolved']and bool(report['declared_cases'])and report['contact_qualified_cases']==report['declared_cases']
 save(args.report_output/'report.json',report);print(json.dumps({k:report[k]for k in('scan_completed','passed','map_count','declared_cases','contact_qualified_cases','finite_completed_probes')}))
 return 0 if report['scan_completed']and report['runtime_unchanged']and report['requested_case_filter_resolved']else 1

if __name__=='__main__':raise SystemExit(main())
