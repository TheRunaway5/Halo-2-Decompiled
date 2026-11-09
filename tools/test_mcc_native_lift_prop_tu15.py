#!/usr/bin/env python3
"""Exercise a genuine authored inactive prop through a named-marker MCC lift.

The sole gameplay input is public set_object_position, which owns source B95D0.
Read-only native entry/return probes attribute marker, velocity and wake work to
the exact field/prop. No velocity, island, membership or activation is supplied.
"""
import argparse
import json
import math
import os
from pathlib import Path
import re
import signal
import struct
import subprocess
import time
import test_mcc_gravlift_contact_audit_tu15 as audit

ROOT = audit.ROOT
SCRIPT = r'''set pagination off
set confirm off
set print thread-events off
set debuginfod enabled off
set breakpoint pending off
set $world=(void*)0
set $memory=(void*)0
set $ready=0
python
import gdb,json,struct,math
plan=CASE_PLAN
stats_layout=STATS_LAYOUT
kernel_symbols=KERNEL_SYMBOLS
phase=None;phase_tick=0;done=False;active=False;in_input=False
samples=[];inputs=[];events=[];errors=[];throws=[];unavailable=[];native={};phase_native={};initial={};prop=None;field=None
returns={};velocity_depth=0;listener_depth=0;before_stats={};after_stats={};entry_diagnostics={}
def reg(name):
 value=int(gdb.parse_and_eval('$'+name))
 return value&0xffffffff if name in ('eax','edi','esi','edx','ecx','r8d','r9d')else value
def raw(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def word(p):return struct.unpack('<I',raw(p,4))[0]
def source(p,n):
 if not 0<p<2**32 or not 0<n<=32768 or p+n>2**32:raise RuntimeError('Invalid bounded source read')
 host=int(gdb.parse_and_eval("((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,%d,%d)"%(p,n)))
 return raw(host,n)
def u32(p):return struct.unpack('<I',source(p,4))[0]
def vec(p):return list(struct.unpack('<fff',source(p,12)))
def body_host(datum):
 return int(gdb.parse_and_eval("((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,%d)"%datum))
def component_for(datum):
 host=body_host(datum);physical=word(host+0xb4)
 if physical==0xffffffff:raise RuntimeError('Actual object has no physical component')
 pool=u32(0x52be5c);capacity=u32(pool+0x20);stride=u32(pool+0x24);index=physical&65535
 if index>=capacity or stride!=160:raise RuntimeError('Actual physical pool layout/generation differs')
 component=u32(pool+0x44)+index*stride
 if u32(component+8)!=datum:raise RuntimeError('Actual physical component owner changed')
 return host,component
def prop_state():
 host,component=component_for(prop['object'])
 if word(host)!=plan['prop']['authored']['definition']:raise RuntimeError('Actual prop definition changed')
 count=struct.unpack('<i',source(component+0x74,4))[0]
 if count<=plan['prop']['slot'] or count>128:raise RuntimeError('Actual prop signed body slot changed')
 body=u32(u32(component+0x70)+plan['prop']['slot']*0x60+0x40);motion=u32(body+0x3c);island=u32(body+0x44)
 if source(body+0x40,1)!=b'\0':raise RuntimeError('Declared dynamic prop became static')
 return {'object':prop['object'],'component':component,'body':body,'motion':motion,'island':island,
  'island_active':source(island+0x29,1)[0]if island else None,
  'position':vec(motion+0xb0),'object_position':list(struct.unpack('<fff',raw(host+0x64,12))),
  'linear_velocity':vec(motion+0x40),'angular_velocity':vec(motion+0x50)}
def capture_stats():
 pointer=int(gdb.parse_and_eval('((void*(*)(unsigned long))malloc)(%d)'%stats_layout['size']))
 gdb.set_convenience_variable('stats',gdb.Value(pointer))
 try:
  gdb.execute("call ((void(*)(void*,void*)) 'halo2::PhysicsLiveWorldTu15::native_physics_frame_activity() const')($stats,$world)",to_string=True)
  result={}
  for name,offset in stats_layout.items():
   if name=='size':continue
   result[name]=list(struct.unpack('<QQ',raw(pointer+offset,16)))if name in ('native_lift_velocity_attempts','native_lift_velocity_completed','native_lift_velocity_observed_stores')else word64(pointer+offset)
  return result
 finally:
  gdb.execute("call ((void(*)(void*)) 'halo2::NativeTu15PhysicsFrameExecutionStats::~NativeTu15PhysicsFrameExecutionStats()')($stats)",to_string=True)
  gdb.execute('call ((void(*)(void*))free)($stats)',to_string=True)
def word64(p):return struct.unpack('<Q',raw(p,8))[0]
def select():
 global prop,field,initial,active,phase,phase_tick,before_stats
 pool=int(gdb.parse_and_eval("((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::object_pool() const')($world)"))
 count=word(pool+0x38);headers=source(word(pool+0x44),count*12)
 if count>2048 or word(pool+0x24)!=12:raise RuntimeError('Actual object pool layout differs')
 found={'prop':[],'field':[]}
 for index in range(count):
  h=headers[index*12:index*12+12];salt=struct.unpack_from('<H',h)[0]
  if not salt or not struct.unpack_from('<I',h,8)[0]:continue
  datum=(salt<<16)|index;host=body_host(datum);definition=word(host)
  for key,expected,point in [('prop',plan['prop']['authored']['definition'],plan['prop']['authored']['position']),('field',plan['field']['definition_datum'],plan['field']['placement_position'])]:
   if definition!=expected:continue
   position=list(struct.unpack('<fff',raw(host+0x64,12)))
   if sum((a-b)**2 for a,b in zip(position,point))<1e-8:found[key].append({'object':datum,'definition':definition,'position':position,'type':h[3]})
 if len(found['prop'])!=1 or len(found['field'])!=1:raise RuntimeError('Authored prop/field requires unique exact definition and initial position; no nearest-object match')
 prop,field=found['prop'][0],found['field'][0]
 if prop['type']!=11:raise RuntimeError('Declared prop is not actual crate type11')
 initial=prop_state()
 if initial['island_active']!=0:raise RuntimeError('Actual authored prop was not inactive before declared input')
 before_stats=capture_stats()
 print('PROP_LIFT_PROGRESS='+json.dumps({'phase':'initial','prop':prop,'inactive':initial['island_active']}),flush=True)
 phase='entry';phase_tick=0;active=True;apply_input(plan['field']['entrance'])
def apply_input(point):
 global in_input
 before=prop_state();xy=struct.unpack('<d',struct.pack('<ff',*point[:2]))[0]
 gdb.set_convenience_variable('input_xy',gdb.Value(xy));gdb.set_convenience_variable('input_z',gdb.Value(float(point[2])))
 gdb.set_convenience_variable('prop_datum',gdb.Value(prop['object']))
 in_input=True
 try:gdb.execute("call ((void(*)(void*,unsigned int,double,float)) 'halo2::PhysicsLiveWorldTu15::set_object_position(unsigned int, halo2::Vec3)')($world,$prop_datum,$input_xy,$input_z)",to_string=True)
 finally:in_input=False
 inputs.append({'phase':phase,'position':point,'before':before,'after':prop_state()})
def observe():
 global phase,phase_tick,active,done,after_stats
 if not active:return
 state=prop_state();host,component=component_for(field['object']);header=u32(component+0x94);matched=[]
 if header:
  count=u32(header+4)
  if count>4096:raise RuntimeError('Actual field membership extent exceeds bounded profile')
  if count:
   rows=source(u32(header),count*8)
   for i in range(count):
    row=rows[i*8:i*8+8]
    if row[0]==plan['field']['membership_phantom_index'] and struct.unpack_from('<I',row,4)[0]==prop['object']:matched.append(row.hex())
 if not all(math.isfinite(x)for key in ('position','object_position','linear_velocity','angular_velocity')for x in state[key]):raise RuntimeError('Nonfinite actual retained motion')
 samples.append({'phase':phase,'tick':phase_tick,**state,'membership_rows':matched});phase_tick+=1
 if phase_tick>=(plan['leave_ticks']if phase=='leave'else plan['contact_ticks']):
  print('PROP_LIFT_PROGRESS='+json.dumps({'phase':phase,'completed_ticks':phase_tick,'native':phase_native.get(phase,{})}),flush=True)
  if phase=='entry':phase='leave';point=plan['prop']['authored']['position']
  elif phase=='leave':phase='reentry';point=plan['field']['entrance']
  else:after_stats=capture_stats();active=False;done=True;return
  phase_tick=0;apply_input(point)
class Returned(gdb.FinishBreakpoint):
 def __init__(self,kind,data):
  super().__init__(gdb.newest_frame(),internal=True);self.kind=kind;self.data=data;returns[self.number]=self
  self.commands='silent\npython native_return(%d)\ncontinue\n'%self.number
 def out_of_scope(self):errors.append('Native call unwound before return: '+self.kind)
def count(key):
 native[key]=native.get(key,0)+1
 counts=phase_native.setdefault(phase,{})
 counts[key]=counts.get(key,0)+1
def native_return(number):
 global velocity_depth,listener_depth
 call=returns.pop(number)
 try:
  kind=call.kind;count(kind+'.returns');event={'kind':kind,'phase':phase,'input':in_input,**call.data}
  if kind=='marker':
   # At an exact symbol-entry FinishBreakpoint, optimized debug metadata may
   # omit return_value. AX16 is the actual declared native ABI return.
   event['count']=reg('eax')&65535
   if event['count']:
    b=source(event['destination']+0x38,52);event['world_matrix_hex']=b.hex();event['forward']=list(struct.unpack_from('<fff',b,4))
  elif kind.startswith('velocity'):
   velocity_depth-=1;event['state']=prop_state();event['after_hex']=source(event['destination'],16).hex()
   if event['after_hex']!=event['before_hex']:count('mutation.'+kind.split('.')[-1])
  elif kind=='listener':listener_depth-=1
  elif kind=='factory303c10':event['agent']=reg('eax')
  if kind in ('marker','factory303c10') or sum(e['kind']==kind for e in events)<24:events.append(event)
 except (gdb.error,RuntimeError,MemoryError,ValueError)as error:errors.append('native return: '+str(error))
def retained_velocity_write():
 if active and velocity_depth:
  kind='store.'+('angular'if reg('esi')==initial['motion']+0x50 else'linear');count(kind)
  if sum(e['kind']==kind for e in events)<12:events.append({'kind':kind,'phase':phase,'address':reg('esi'),'bytes':reg('rcx'),'written_hex':raw(reg('rdx'),16).hex()})
def native_entry(kind):
 global velocity_depth,listener_depth
 if not active or prop is None:return
 try:
   if kind!='velocity':
    entries=entry_diagnostics.setdefault(kind,[])
    if len(entries)<32:entries.append({'phase':phase,'input':in_input,'esi':reg('esi'),'edx':reg('edx'),'ecx':reg('ecx'),'r8d':reg('r8d'),'r9d':reg('r9d')})
   data={}
   if kind=='marker':
    if reg('esi')!=field['object'] or reg('ecx')!=plan['field']['force']['marker']:return
    data={'object':reg('esi'),'marker':reg('ecx'),'destination':reg('r9d'),'capacity':reg('r8d')&65535}
   elif kind=='velocity':
    if reg('esi')!=initial['component']:return
    angular=bool(reg('ecx'));kind+='.'+('angular'if angular else'linear')
    destination=prop_state()['motion']+(0x50 if angular else 0x40)
    data={'component':reg('esi'),'slot':reg('edx'),'destination':destination,'before_hex':source(destination,16).hex()}
    velocity_depth+=1
   elif kind=='wake':
    if reg('esi')!=initial['body']:return
    data={'body':reg('esi')}
   elif kind=='listener':
    island=reg('esi');body_count=u32(island+0x40)
    if body_count>2048:raise RuntimeError('Listener body extent exceeds bounded profile')
    rows=source(u32(island+0x3c),body_count*4)if body_count else b''
    if initial['body']not in struct.unpack('<'+'I'*body_count,rows):return
    data={'island':island}
    listener_depth+=1
   elif kind=='factory303c10':
    if not listener_depth or reg('esi')!=0:return
    data={'factory_enum':reg('esi')}
   count(kind+'.entries');Returned(kind,data)
 except (gdb.error,RuntimeError,MemoryError,ValueError)as error:errors.append('native entry: '+str(error))
for name,kind in [('native_campaign_object_marker_request_tu15','marker'),('native_tu15_set_phantom_body_velocity','velocity'),('native_tu15_activate_physics_body','wake'),('native_tu15_recreate_island_agents','listener'),('native_tu15_create_contact_agent','factory303c10')]:
 if not kernel_symbols.get(name):unavailable.append(name)
 for symbol in kernel_symbols.get(name,[]):
  try:
   # Break at the genuine symbol entry, before a prologue may reuse volatile
   # argument registers. Function-name line breakpoints are insufficient.
   bp=gdb.Breakpoint('*'+symbol,internal=True);bp.commands='silent\npython native_entry(%r)\ncontinue\n'%kind
  except gdb.error:unavailable.append(symbol)
class Throw(gdb.Breakpoint):
 def stop(self):
  if active:throws.append({'phase':phase,'input':in_input,'backtrace':gdb.execute('bt 12',to_string=True)[-10000:]})
  return False
Throw('__cxa_throw',internal=True)
def tick():
 global active
 try:
  if reg('ready')==3:select()
  elif reg('ready')>3:observe()
 except (gdb.error,RuntimeError,MemoryError,ValueError)as error:errors.append(str(error));active=False
end
break halo2::MultiplayerObjectRuntime::step_objects
commands
silent
if $world == 0
set $world=((void*(*)(void*)) 'halo2::MultiplayerObjectRuntime::tu15_world()')((void*)$rdi)
set $memory=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::memory()')($world)
end
continue
end
break halo2::MultiplayerMatch::step
commands
silent
if $world != 0
set $ready=$ready+1
python tick()
end
continue
end
run
if $_isvoid($_exitcode)
bt 16
else
printf "PROP_LIFT_CHILD_EXIT=%d\n",$_exitcode
end
python
print('PROP_LIFT_RESULT='+json.dumps({'done':done,'prop':prop,'field':field,'initial':initial,'inputs':inputs,'samples':samples,'native_calls':native,'phase_native_calls':phase_native,'native_events':events,'before_stats':before_stats,'after_stats':after_stats,'entry_diagnostics':entry_diagnostics,'errors':errors,'throws':throws,'unavailable':unavailable}),flush=True)
end
'''


def evaluate(observed, plan):
    errors = list(observed.get('errors', []))
    if not observed.get('done'):
        errors.append('Declared entry/leave/reentry loop did not complete')
    if observed.get('throws'):
        errors.append('Actual prop loop threw')
    phases = {}
    for name in ('entry', 'leave', 'reentry'):
        rows = [s for s in observed.get('samples', []) if s['phase'] == name]
        members = [s for s in rows if s['membership_rows']]
        phases[name] = dict(ticks=len(rows), selected_member_ticks=len(members),
                            maximum_member_speed=max((math.sqrt(sum(v*v for v in s['linear_velocity'])) for s in members), default=0),
                            maximum_member_up_velocity=max((s['linear_velocity'][2] for s in members), default=0))
        if len(rows) != (plan['leave_ticks'] if name == 'leave' else plan['contact_ticks']):
            errors.append(name + ': completed tick coverage missing')
        if name == 'leave':
            if any(s['membership_rows'] for s in rows[-10:]):
                errors.append('Exact prop remained in selected field after declared leave')
        elif not members or phases[name]['maximum_member_up_velocity'] <= .05:
            errors.append(name + ': exact prop membership and upward retained motion not proven')
    calls = observed.get('native_calls', {})
    for key in ('velocity.linear', 'velocity.angular', 'wake', 'listener', 'marker', 'factory303c10'):
        if not calls.get(key + '.entries') or calls.get(key + '.entries') != calls.get(key + '.returns'):
            errors.append('Missing genuine completed native ' + key)
    for phase in ('entry', 'reentry'):
        counts=observed.get('phase_native_calls',{}).get(phase,{})
        for kind in ('linear','angular'):
            if not counts.get('velocity.'+kind+'.returns'):
                errors.append(phase+': exact prop native '+kind+' completion missing')
        if not counts.get('mutation.linear'):
            errors.append(phase+': no actual retained linear velocity mutation across native call')
    markers = [e for e in observed.get('native_events', []) if e['kind'] == 'marker' and e.get('count', 0) > 0]
    if not markers:
        errors.append('Actual nonzero authored field marker lookup did not return a marker')
    if not any(e['kind']=='factory303c10' and e.get('agent') for e in observed.get('native_events',[])):
        errors.append('Actual selected prop native303C10 factory did not return a real agent')
    before,after=observed.get('before_stats',{}),observed.get('after_stats',{})
    for name in ('native_lift_velocity_refusals','native_lift_observation_refusals','native_pair_deletion_refusals'):
        if name not in before or name not in after or after[name]!=before[name]:
            errors.append('Missing or nonzero actual refusal delta: '+name)
    return dict(passed=not errors, errors=errors, phases=phases,
                named_marker_returns=len(markers), native_calls=calls)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('runtime', 'source-root', 'inventory', 'plan', 'report-output'):
        parser.add_argument('--' + key, required=True, type=Path)
    parser.add_argument('--timeout-seconds', type=float, default=240)
    parser.add_argument('--contact-ticks', type=int, default=25)
    parser.add_argument('--leave-ticks', type=int, default=15)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not math.isfinite(args.timeout_seconds) or args.timeout_seconds <= 0:
        parser.error('Require a finite positive timeout')
    if not 20 <= args.contact_ticks <= 180 or not 10 <= args.leave_ticks <= 90:
        parser.error('Require bounded genuine contact/leave tick budgets')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime, source, inventory, plan_path = (p.resolve(strict=True) for p in (args.runtime, args.source_root, args.inventory, args.plan))
    cache = runtime.parent / 'CMakeCache.txt'
    if inventory.parent != runtime.parent or 'CMAKE_HOME_DIRECTORY:INTERNAL='+str(source) not in cache.read_text().splitlines():
        parser.error('Require the exact owned source/build/inventory cohort')
    plan = json.loads(plan_path.read_text());map_path = Path(plan['map'])
    plan.update(contact_ticks=args.contact_ticks,leave_ticks=args.leave_ticks)
    if not plan['passed'] or audit.digest(map_path) != plan['map_sha256']:
        parser.error('Authored gate input changed')
    pins = {str(p): audit.digest(p) for p in (runtime, inventory, plan_path, map_path, Path(__file__))}
    publication = {name: audit.digest(ROOT/name) for name in ('Halo2-Launcher','Halo2-Title-Launcher','gameplay-runtime-selection.json','frontend-runtime-selection.json') if (ROOT/name).is_file()}
    # Compile the same header ABI check used by the broad live audit. This
    # probe reads no private host struct fields without an inventory seal.
    args.report_output = args.report_output.resolve();args.report_output.mkdir(parents=True, exist_ok=False)
    try:abi = audit.abi_layout(source, runtime, scratch, inventory)
    except (ValueError,RuntimeError) as error:
        audit.write_json(args.report_output/'report.json',dict(passed=False,executed=False,error=str(error),input_pins=pins,retained_large_artifacts=[]))
        raise
    fields=['native_lift_velocity_attempts','native_lift_velocity_completed','native_lift_velocity_observed_stores',
            'native_lift_velocity_refusals','native_lift_observation_refusals','native_pair_deletion_refusals']
    code='#include "halo2/native_tu15_physics_frame_residuals.hpp"\n#include <cstddef>\n#include <iostream>\nint main(){using T=halo2::NativeTu15PhysicsFrameExecutionStats;std::cout<<"{\\\"size\\\":"<<sizeof(T)'
    for name in fields:code+='<<",\\\"'+name+'\\\":"<<offsetof(T,'+name+')'
    code+='<<"}\\n";}\n'
    cpp,binary=scratch/'stats.cpp',scratch/'stats';cpp.write_text(code)
    compiled=subprocess.run(['c++','-std=c++20','-Wno-invalid-offset','-I'+str(source/'include'),str(cpp),'-o',str(binary)],capture_output=True,text=True)
    if compiled.returncode:raise RuntimeError('Stats ABI compile failed: '+compiled.stderr[-3000:])
    stats_layout=json.loads(subprocess.check_output([str(binary)],text=True));abi['prop_stats_layout']=stats_layout;abi['prop_stats_compiler_source']=code
    audit.write_json(args.report_output/'abi-layout.json', abi)
    names=['native_campaign_object_marker_request_tu15','native_tu15_set_phantom_body_velocity','native_tu15_activate_physics_body','native_tu15_recreate_island_agents','native_tu15_create_contact_agent']
    symbols={name:[] for name in names}
    for line in subprocess.check_output(['nm','--defined-only',str(runtime)],text=True).splitlines():
        words=line.split()
        if len(words)!=3 or words[1]not in ('T','t') or '.'in words[2]:continue
        for name in names:
            if re.match(r'^_ZN5halo2'+str(len(name))+name,words[2]):symbols[name].append(words[2])
    abi['raw_entry_symbols']=symbols
    audit.write_json(args.report_output/'abi-layout.json',abi)
    script = scratch/'prop.gdb';script.write_text(SCRIPT.replace('CASE_PLAN',repr(plan)).replace('STATS_LAYOUT',repr(stats_layout)).replace('KERNEL_SYMBOLS',repr(symbols)))
    command = ['gdb','--batch','-x',str(script),'--args',str(runtime),'--multiplayer','--map',str(map_path),'--shared-maps',str(map_path.parent),'--local-players','1','--mp-starting-weapon','5','--simulate',str(10+2*args.contact_ticks+args.leave_ticks),'--mute','--mp-source-object-world',str(scratch/'world')]
    environment = {k:v for k,v in os.environ.items() if not k.startswith('HALO2_')}
    environment.update(HALO2_TEST_SCRATCH=str(scratch), XDG_CACHE_HOME=str(scratch/'cache'), XDG_CONFIG_HOME=str(scratch/'config'), XDG_DATA_HOME=str(scratch/'data'))
    started=time.monotonic();log=scratch/'gdb.log'
    with log.open('w') as stream:
        process=subprocess.Popen(command,cwd=source,env=environment,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
        try:code=process.wait(timeout=args.timeout_seconds)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid,signal.SIGTERM)
            try:process.wait(timeout=5)
            except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
            code=124
    text=log.read_text(errors='replace');matches=re.findall(r'^PROP_LIFT_RESULT=(.*)$',text,re.MULTILINE)
    observed=json.loads(matches[-1]) if matches else {}
    child=re.findall(r'PROP_LIFT_CHILD_EXIT=(\d+)',text);child=int(child[-1]) if child else None
    result=evaluate(observed,plan)
    result.update(runtime_sha256=pins[str(runtime)],input_pins=pins,command=command,
                  seconds=round(time.monotonic()-started,3),gdb_exit_code=code,child_exit_code=child,
                  observed=observed,owned_scratch=str(scratch),no_outcome_writes=True,
                  supplied_gameplay_input='public set_object_position only',retained_large_artifacts=[],
                  contact_ticks=args.contact_ticks,leave_ticks=args.leave_ticks,
                  store_scope='Exact selected native entry/return16B retained pre/post views; linear mutation required. Original physical qualification owns exact write order and unchanged-value stores.',
                  lifetime_scope='Bounded native composition gate: the authored power module is destructible. Leave uses its genuine initial safe authored position before the later natural despawn observed in the90tick diagnostic.')
    result['inputs_unchanged']=all(audit.digest(Path(p))==v for p,v in pins.items())
    result['publication_unchanged']=all(audit.digest(ROOT/p)==v for p,v in publication.items())
    result['passed']=bool(result['passed'] and code==0 and child==0 and result['inputs_unchanged'] and result['publication_unchanged'])
    (args.report_output/'gdb.log').write_text(text[-30000:]);audit.write_json(args.report_output/'report.json',result)
    print(json.dumps({k:result[k] for k in ('passed','errors','seconds','native_calls')}))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
