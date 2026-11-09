#!/usr/bin/env python3
"""Read genuine canonical MCC body/motion before-input metadata; no pose or force input."""
import argparse
import json
import math
import os
from pathlib import Path
import re
import signal
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
set $ready_ticks=0
set $captured=0
python
import gdb,json,struct
records=[];islands={};worlds={};errors=[];throws=[];capture_throws=[];captured=False;capturing=False;last_read=None
maximum=MAX_BODIES
def value(name):return int(gdb.parse_and_eval('$'+name))
def raw(p,n):return bytes(gdb.selected_inferior().read_memory(p,n))
def word(p):return struct.unpack('<I',raw(p,4))[0]
def source(p,n):
 global last_read
 last_read={'source':p,'bytes':n}
 if not 0<p<2**32 or not 0<n<=32768 or p+n>2**32:raise RuntimeError('Unqualified bounded source read extent')
 expression="((void*(*)(void*,unsigned int,unsigned long)) 'halo2::SourceRuntimeMemory::view(unsigned int, unsigned long) const')($memory,%d,%d)"%(p,n)
 pointer=int(gdb.parse_and_eval(expression));return raw(pointer,n)
def u32(b,p):return struct.unpack_from('<I',b,p)[0]
def snapshot_world(token):
 if not token or token in worlds:return
 b=source(token,0x110);item={'identity':token,'bytes_hex':b.hex(),'arrays':{}}
 for label,offset,count_offset in [('active',8,0xc),('inactive',0x14,0x18),('listeners',0x78,0x7c),('deferred',0x108,0x10c)]:
  pointer,count=u32(b,offset),u32(b,count_offset)
  if count>2048:raise RuntimeError('World '+label+' count exceeds bounded metadata profile')
  entry={'pointer':pointer,'count':count}
  if count:
   rows=source(pointer,count*4);entry['bytes_hex']=rows.hex()
   if label=='listeners':
    entry['owners']=[]
    for offset in range(count):
     listener=u32(rows,offset*4)
     if listener:
      table_bytes=source(listener,4);table=u32(table_bytes,0);owner_bytes=source(table+4,4)
      entry['owners'].append({'listener':listener,'vtable_hex':table_bytes.hex(),'vtable':table,'method_cell':table+4,'method_hex':owner_bytes.hex(),'owner':u32(owner_bytes,0)})
  item['arrays'][label]=entry
 worlds[token]=item
def snapshot_island(token):
 if not token or token in islands:return
 b=source(token,0x44);item={'identity':token,'bytes_hex':b.hex(),'active':b[0x29],'body_count':u32(b,0x40)}
 if item['body_count']>2048:raise RuntimeError('Island body count exceeds bounded metadata profile')
 if item['body_count']:item['body_list_hex']=source(u32(b,0x3c),item['body_count']*4).hex()
 header=source(token+0x68,12);pair_count=struct.unpack_from('<i',header,4)[0]
 if not 0<=pair_count<=256:raise RuntimeError('Island pair count exceeds bounded metadata profile')
 item.update(pair_header_hex=header.hex(),pair_count=pair_count,pair_list=u32(header,0))
 if pair_count:
  pairs=source(u32(header,0),pair_count*16);item['pair_rows_hex']=pairs.hex();item['missing_agent_dispatch']=[]
  # These are exact original constant getters, also admitted by the native
  # activation type service. An unknown getter is recorded, never executed or
  # assigned an inferred type.
  classes={0xa9520:23,0xab4b0:24,0xa9be0:19,0xadb50:20,0xad1d0:17,
    0x2e8cf0:4,0x31dd70:5,0x31edf0:6,0xad710:10,0xae690:21,
    0xad740:1,0x31d520:2,0xa3850:0,0x31ede0:7,0xad390:8,0xaf940:25}
  world=u32(source(token+0x20,4),0);context=u32(source(world+0xcc,4),0);dispatch=u32(source(context,4),0)
  for ordinal in range(pair_count):
   pair=pairs[ordinal*16:ordinal*16+16]
   if u32(pair,8):continue
   row={'ordinal':ordinal,'context':context,'dispatch':dispatch,'payloads':[]}
   for at in (0,4):
    payload=u32(pair,at)-0x10;shape=u32(source(payload,4),0);table=u32(source(shape,4),0);cell=source(table+20,4);owner=u32(cell,0)
    row['payloads'].append({'identity':payload,'shape':shape,'vtable':table,'getter_cell':table+20,'getter_hex':cell.hex(),'getter_owner':owner,'shape_class':classes.get(owner)})
   kinds=[p['shape_class']for p in row['payloads']]
   if all(k is not None and 0<=k<32 for k in kinds):
    cell=dispatch+((kinds[0]<<5)+kinds[1])*4+0x18c;raw_factory=source(cell,4)
    row.update(factory_cell=cell,factory_hex=raw_factory.hex(),factory_owner=u32(raw_factory,0))
   else:row['limit']='Unknown actual source getter; factory selection is unqualified'
   item['missing_agent_dispatch'].append(row)
 islands[token]=item
def capture():
 global captured,zero,capturing
 capturing=True
 gdb.execute("set $memory=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::memory()')($world)",to_string=True)
 gdb.execute("set $object_pool=((void*(*)(void*)) 'halo2::PhysicsLiveWorldTu15::object_pool() const')($world)",to_string=True)
 pool=value('object_pool');count=word(pool+0x38);stride=word(pool+0x24)
 if stride!=12 or count>2048:raise RuntimeError('Actual object pool is not the qualified TU1.5 layout')
 headers=source(word(pool+0x44),count*12)if count else b''
 physical_token=u32(source(0x52be5c,4),0);physical=source(physical_token,0x4c)
 physical_capacity,physical_stride,physical_data=u32(physical,0x20),u32(physical,0x24),u32(physical,0x44)
 if physical_capacity>8192 or not 0xa0<=physical_stride<=0x1000:raise RuntimeError('Actual physical component pool is outside the bounded source layout')
 pointer=source(0x474d04,4);zero={'pointer_hex':pointer.hex(),'identity':u32(pointer,0),'vector_hex':source(u32(pointer,0),12).hex()}
 for index in range(count):
  header=headers[index*12:index*12+12];salt=struct.unpack_from('<H',header)[0]
  if not salt or not u32(header,8):continue
  datum=(salt<<16)|index
  try:
   expression="((void*(*)(void*,unsigned int)) 'halo2::PhysicsLiveWorldTu15::object_body(unsigned int) const')($world,%d)"%datum
   host=int(gdb.parse_and_eval(expression));definition=word(host);physical_datum=word(host+0xb4)
   if physical_datum==0xffffffff:continue
   component_index=physical_datum&65535
   if component_index>=physical_capacity:raise RuntimeError('Full object physical datum escapes its real pool')
   component_token=physical_data+component_index*physical_stride;component=source(component_token,0xa0)
   if u32(component,8)!=datum:raise RuntimeError('Physical component source owner/generation differs')
   bodies=struct.unpack_from('<i',component,0x74)[0]
   if not 0<=bodies<=128:raise RuntimeError('Actual body count exceeds the bounded signed-slot profile')
   slots=source(u32(component,0x70),bodies*0x60)if bodies else b''
   for slot in range(bodies):
    if len(records)>=maximum:raise RuntimeError('Declared body capture maximum reached')
    row=slots[slot*0x60:slot*0x60+0x60];body_token=u32(row,0x40)
    if not body_token:continue
    body=source(body_token,0x78);motion_token=u32(body,0x3c)
    record={'object':datum,'object_type':header[3],'definition':definition,'component_datum':physical_datum,
      'component':component_token,'component_hex':component.hex(),'slot':slot,'slot_hex':row.hex(),
      'body':body_token,'body_hex':body.hex(),'static_flag':body[0x40],
      'world':u32(body,8),'island':u32(body,0x44),'motion':motion_token}
    if not record['static_flag'] and motion_token:
     motion=source(motion_token,0x80);table=u32(motion,0)
     # A runtime vtable may consist of several separately owned cells. Read
     # the two genuine setter operands, never a speculative table span.
     linear=source(table+0x50,4);angular=source(table+0x54,4)
     record.update(motion_hex=motion.hex(),motion_vtable=table,
       setter_cells=[{'address':table+0x50,'hex':linear.hex()},{'address':table+0x54,'hex':angular.hex()}],
       linear_setter=u32(linear,0),angular_setter=u32(angular,0),setter_query_required=True)
     policy=u32(body,0x54)
     if policy:
      table_bytes=source(policy,4);table=u32(table_bytes,0);owner_bytes=source(table+0x1c,4)
      record['policy']={'identity':policy,'vtable_hex':table_bytes.hex(),'vtable':table,'kind_method_cell':table+0x1c,'kind_method_hex':owner_bytes.hex(),'kind_owner':u32(owner_bytes,0),
        'size_hex':source(policy+4,2).hex(),'references_hex':source(policy+6,2).hex()}
     snapshot_island(record['island']);snapshot_world(record['world'])
    else:record.update(setter_query_required=False,motion_capture_reason='Original body flag gates both setters before motion/island/virtual reads')
    records.append(record)
  except (gdb.error,RuntimeError,MemoryError,ValueError)as error:
   errors.append({'object':datum,'type':header[3],'read':last_read,'error':str(error)})
   raise
 captured=True
 capturing=False
class Throws(gdb.Breakpoint):
 def stop(self):
  if capturing:
   message=''
   try:
    pointer=struct.unpack('<Q',raw(int(gdb.parse_and_eval('$rdi'))+8,8))[0]
    message=gdb.Value(pointer).cast(gdb.lookup_type('char').pointer()).string(length=1024,errors='replace').split(chr(0))[0]
   except (gdb.error,MemoryError,ValueError):pass
   capture_throws.append({'read':last_read,'message':message,'stack':gdb.execute('bt 8',to_string=True)[-6000:]})
  elif len(throws)<8:throws.append(gdb.execute('bt 8',to_string=True)[-6000:])
  return False
Throws('__cxa_throw',internal=True)
end
break halo2::MultiplayerObjectRuntime::step_objects
commands
silent
if $world == 0
set $runtime=(void*)$rdi
set $world=((void*(*)(void*)) 'halo2::MultiplayerObjectRuntime::tu15_world()')($runtime)
end
continue
end
break halo2::MultiplayerMatch::step
commands
silent
if $world != 0 && $captured == 0
set $ready_ticks=$ready_ticks+1
if $ready_ticks >= 3
python
try:capture()
except (gdb.error,RuntimeError,MemoryError,ValueError)as error:errors.append({'scope':'bootstrap','error':str(error)})
end
set $captured=1
end
end
continue
end
run
if $_isvoid($_exitcode)
bt 16
else
printf "LIFT_BODY_CHILD_EXIT=%d\n",$_exitcode
end
python
print('LIFT_BODY_METADATA='+json.dumps({'captured':captured,'records':records,'islands':list(islands.values()),'worlds':list(worlds.values()),'zero':globals().get('zero'),'errors':errors,'startup_caught_throws':throws,'capture_throws':capture_throws,'completed_outer_ticks_before_read':max(0,value('ready_ticks')-1)}),flush=True)
end
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runtime', 'source-root', 'maps', 'report-output'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--owned-build-inventory', type=Path)
    parser.add_argument('--map', action='append', default=[])
    parser.add_argument('--maximum-bodies', type=int, default=128)
    parser.add_argument('--timeout-seconds', type=float, default=180)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if not 1<=args.maximum_bodies<=256 or not math.isfinite(args.timeout_seconds) or args.timeout_seconds<=0:parser.error('Require bounded positive capture limits')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    runtime, source, maps = (path.resolve(strict=True) for path in (args.runtime,args.source_root,args.maps))
    inventory = (args.owned_build_inventory or runtime.parent / 'source-hashes.json').resolve(strict=True)
    if runtime.name!='halo2_native':parser.error('Use an exact game ELF')
    if args.owned_build_inventory:
        cache=runtime.parent/'CMakeCache.txt'
        if inventory.parent!=runtime.parent or not cache.is_file() or 'CMAKE_HOME_DIRECTORY:INTERNAL='+str(source) not in cache.read_text().splitlines() or not runtime.parent.parent.name.startswith('halo2-test-'):parser.error('Require the real owned source/build/inventory cohort')
    else:
        qualification=json.loads((runtime.parent/'qualification.json').read_text())
        if not qualification.get('passed')or qualification.get('runtime_sha256')!=audit.digest(runtime):parser.error('Runtime must match its real qualification pin')
    args.report_output=args.report_output.resolve();args.report_output.mkdir(parents=True,exist_ok=False)
    abi = audit.abi_layout(source,runtime,scratch,inventory)
    audit.write_json(args.report_output/'abi-layout.json',abi)
    pin=audit.digest(runtime);results=[]
    publication={name:audit.digest(ROOT/name) for name in ('Halo2-Launcher','Halo2-Title-Launcher','gameplay-runtime-selection.json','frontend-runtime-selection.json') if(ROOT/name).is_file()}
    for name in args.map or ['lockout','coagulation','headlong']:
        if not re.fullmatch('[a-z0-9_]+',name):parser.error('Map must be a direct MCC stem')
        owned=scratch/name;owned.mkdir();map_path=maps/(name+'.map');map_pin=audit.digest(map_path)
        script=SCRIPT.replace('MAX_BODIES',str(args.maximum_bodies));path=owned/'metadata.gdb';path.write_text(script)
        command=['gdb','--batch','-x',str(path),'--args',str(runtime),'--multiplayer','--map',str(map_path),'--shared-maps',str(maps),'--local-players','1','--mp-starting-weapon','5','--simulate','8','--mute','--mp-source-object-world',str(owned/'world')]
        environment={key:value for key,value in os.environ.items()if not key.startswith('HALO2_')}
        environment.update(HALO2_TEST_SCRATCH=str(owned),XDG_CACHE_HOME=str(owned/'cache'),XDG_CONFIG_HOME=str(owned/'config'),XDG_DATA_HOME=str(owned/'data'))
        started=time.monotonic();log=owned/'metadata.log'
        with log.open('w')as stream:
            process=subprocess.Popen(command,cwd=source,env=environment,stdout=stream,stderr=subprocess.STDOUT,start_new_session=True)
            try:code=process.wait(timeout=args.timeout_seconds)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid,signal.SIGTERM)
                try:process.wait(timeout=5)
                except subprocess.TimeoutExpired:os.killpg(process.pid,signal.SIGKILL);process.wait()
                code=124
        text=log.read_text(errors='replace');observations=re.findall(r'^LIFT_BODY_METADATA=(.*)$',text,re.MULTILINE);observed=json.loads(observations[-1])if observations else{}
        exits=re.findall(r'LIFT_BODY_CHILD_EXIT=(\d+)',text);child=int(exits[-1])if exits else None
        result=dict(map=name,map_sha256=map_pin,runtime_sha256=pin,command=command,seconds=round(time.monotonic()-started,3),gdb_exit_code=code,child_exit_code=child,observed=observed,
            passed=code==0 and child==0 and observed.get('captured')and bool(observed.get('records'))and not observed.get('errors')and not observed.get('capture_throws'))
        result['inputs_unchanged']=audit.digest(map_path)==map_pin and audit.digest(runtime)==pin
        result['passed']=bool(result['passed']and result['inputs_unchanged'])
        audit.write_json(args.report_output/(name+'.json'),result);(args.report_output/(name+'.log')).write_text(text[-16000:]);results.append(result)
    report=dict(passed=bool(results)and all(row['passed']for row in results),scope=__doc__,source_inventory_sha256=audit.digest(inventory),runtime_sha256=pin,
        worlds=[{'map':r['map'],'passed':r['passed'],'bodies':len(r['observed'].get('records',[]))}for r in results],
        publication_unchanged=all(audit.digest(ROOT/name)==value for name,value in publication.items()),
        no_manual_input=True,tool_sha256=audit.digest(Path(__file__)),retained_large_artifacts=[],
        retention='Bounded authentic source words/hex are required before-input references; no whole raw banks, binaries, archive copies, render captures or publication retained.',
        limit='Canonical neutral startup bodies only. This is not a force contact, marker pose, activation outcome, native integration or all-map parity acceptance.')
    audit.write_json(args.report_output/'report.json',report)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
