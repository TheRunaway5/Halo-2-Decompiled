#!/usr/bin/env python3
"""Complete original213760/1A0F10 producer before real free-list acquisition.

No kernel result, wait completion, file bytes or original ready timeline is
supplied. Separate D rows are immutable authored-file bytes for real native
I/O tests, not claimed original kernel reads. The original allocator call
records supply168 literal requests; generated controls supply caller inputs.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,re,subprocess,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,DONE=0x20000000,0x200fe000,0x200ff000,0x20003000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def fnv(raw):
 value=14695981039346656037
 for byte in raw:value=((value^byte)*1099511628211)&0xffffffffffffffff
 return value
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);a=p.parse_args()
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();seen={};boundaries=[];task_base=[None];lease_stop=[False]
 def page(address):
  at=address&~4095
  if at not in pages:
   raw=s.read(at,4096);u.mem_map(at,4096);u.mem_write(at,raw);pages[at]=raw
 def read(address,size):
  for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:page(at)
  return bytes(u.mem_read(address,size))
 def put(address,raw):
  read(address,len(raw));u.mem_write(address,raw)
  for at in range(address&~4095,((address+len(raw)-1)&~4095)+4096,4096):
   if not SYNTH<=at<SYNTH+0x100000:dirty.add(at)
 def missing(g,access,address,size,value,data):
  try:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
   return True
  except ValueError as error:
   boundaries.append(dict(kind='uncaptured memory',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',reason=str(error)));g.emu_stop();return False
 def code(g,pc,size,data):
  try:original=x.read(pc,size)
  except (ValueError,KeyError):
   boundaries.append(dict(kind='actual kernel entry; not executed',entry=f'{pc:08X}',stack=words(read(g.reg_read(UC_X86_REG_ESP)+4,16))));g.emu_stop();return
  assert read(pc,size)==original;seen[pc]=size
  if pc==0x1a0f10:task_base[0]=g.reg_read(UC_X86_REG_ESP)-0x28
  if pc==0x1208b0 and lease_stop[0]:g.emu_stop()
 def writing(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups());read(ga,gl+1)
 u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,value in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,value)
 active=uint(s.read(0x55aca8,4));addresses=[0x557c90+active*0x804,0x558c98,0x557c90,0x558494]
 handles=[uint(s.read(at,4))for at in addresses];assert handles==[76,72,64,68]
 allocator_path=ROOT/'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json'
 parent=json.loads(allocator_path.read_text());rows=[];actual_resources=set();rng=random.Random(0x213760)
 for record in parent['records']:
  event=next(row for row in record['source_calls']if row['entry']=='00213760');st=event['stack']
  rows.append([event['ecx'],st[0],st[1],st[2],st[3],st[4],st[5]]+handles+
   [uint(s.read(0x4e28f0,4)),uint(s.read(0x4e0364,4))])
  actual_resources.add((event['ecx'],st[0],st[1]))
 for index in range(1024):
  rows.append([rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),DONE,
   rng.getrandbits(32),rng.getrandbits(32)]+[rng.getrandbits(32)for _ in range(4)]+[rng.getrandbits(32),rng.getrandbits(32)])
 for location in range(4):
  for offset in[0,1,0x1ff,0x200,0x3ffffffe,0x3fffffff]:
   rows.append([(location<<30)|offset,0,0,0,DONE,3,7]+handles+[0xffffffff,0xffffffff])
 def reset():
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear();boundaries.clear();task_base[0]=None
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 def setup(row):
  reset()
  for at,value in zip(addresses,row[7:11]):put(at,packed([value]))
  put(0x4e28f0,packed([row[11]]));put(0x4e0364,packed([row[12]]));put(row[4],b'\x7f')
  u.reg_write(UC_X86_REG_ECX,row[0]);u.mem_write(STACK,packed([STOP,row[1],row[2],row[3],row[4],row[5],row[6]]))
 outputs=[]
 callback_store=x.read(0x1a0f90,7)
 assert callback_store==bytes.fromhex('C74034700E1A00')
 inspected_callback=uint(callback_store[3:7])
 lease_stop[0]=True
 for row in rows:
  setup(row);u.emu_start(0x213760,STOP,count=1000000);assert not boundaries and u.reg_read(UC_X86_REG_EIP)==0x1208b0
  assert task_base[0] is not None
  task=words(read(task_base[0],40))
  boundary_stack=u.reg_read(UC_X86_REG_ESP)
  priority=uint(read(boundary_stack+0x48,4));category=uint(read(boundary_stack+0x44,4))
  done=u.reg_read(UC_X86_REG_EBX)
  assert (priority,category,done)==(row[6],row[5],row[4])
  outputs.append(task+[priority,category,inspected_callback,done,uint(read(0x4e28f0,4)),done,read(done,1)[0],uint(read(0x4e0364,4))])
 diagnostics=[];lease_stop[0]=False
 for row in rows[:4]:
  setup(row)
  try:u.emu_start(0x213760,STOP,count=1000000)
  except UcError:
   if not boundaries:raise
  assert boundaries and boundaries[0]['entry']=='80020F5A'
  diagnostics.append(dict(input=row,boundary=boundaries.copy()))
 reset()
 for at,raw in pages.items():assert bytes(u.mem_read(at,4096))==raw
 encoded='\n'.join(' '.join(f'{v:08X}'for v in row)for row in rows)+'\n'
 native=subprocess.run([str(a.binary.resolve()),'--probe'],input=encoded,text=True,capture_output=True,check=True)
 actual=[list(int(v,16)for v in line.split())for line in native.stdout.splitlines()];assert actual==outputs
 names=['zanzibar.map','mainmenu.map','shared.map','single_player_shared.map'];resource_rows=[];resource_meta=[]
 def resource(location,size,buffer):
  path=ROOT/'assets/disc/maps'/names[location>>30];offset=location&0x3fffffff
  with path.open('rb')as file:file.seek(offset);data=file.read(size)
  resource_rows.append(f'D {location:08X} {size:08X} {buffer:08X} | {len(data):08X} {fnv(data):016X}')
  resource_meta.append(dict(location=f'{location:08X}',size=size,buffer=f'{buffer:08X}',file=str(path.relative_to(ROOT)),
   actual_authored_bytes=len(data),sha256=sha(data),source_kernel_read_claimed=False))
 for location,size,buffer in sorted(actual_resources):resource(location,size,buffer)
 for kind,name in enumerate(names):
  path=ROOT/'assets/disc/maps'/name;size=path.stat().st_size
  resource(kind<<30,512,0x20004000);resource((kind<<30)|(size-7),64,0x20004000)
  resource((kind<<30)|size,64,0x20004000)
 fixture=ROOT/'tests/fixtures/retail_source_bitmap_async_read.tsv'
 fixture.write_text('# R: observed prefix/boundary words plus source-inspected callback store; D: actual authored bytes, not original kernel outcomes.\n'+
  '\n'.join('R '+' '.join(f'{v:08X}'for v in row)+' | '+' '.join(f'{v:08X}'for v in out)for row,out in zip(rows,outputs))+
  '\n'+'\n'.join(resource_rows)+'\n')
 report=dict(boundary=__doc__,source_xbe_sha256=PIN,snapshot_ram_sha256=sha(s.ram),snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),
  tool_sha256=sha(Path(__file__).read_bytes()),source_sha256=sha((ROOT/'src/rendering/source_bitmap_async_read.cpp').read_bytes()),
  header_sha256=sha((ROOT/'include/halo2/source_bitmap_async_read.hpp').read_bytes()),test_sha256=sha((ROOT/'tests/source_bitmap_async_read_tests.cpp').read_bytes()),
  async_helper_sha256=sha((ROOT/'src/platform/source_async_jobs.cpp').read_bytes()),async_header_sha256=sha((ROOT/'include/halo2/source_async_jobs.hpp').read_bytes()),
  read_protocol_report_sha256=sha((ROOT/'analysis/multiplayer-shading/source-async-read-original.json').read_bytes()),
  fixture_sha256=sha(fixture.read_bytes()),native_binary_sha256=sha(a.binary.read_bytes()),allocator_report_sha256=sha(allocator_path.read_bytes()),
  cases=len(rows),checked_words=sum(len(row)for row in outputs),actual_allocator_requests=168,immutable_reads=len(resource_rows),
  word_authority=dict(executed_prefix_output_indices=list(range(10))+[14,16,17],
   observed_boundary_input_indices=[10,11,13,15],source_inspected_output_indices=[12],
   observed_prefix_and_boundary_words=len(rows)*17,source_inspected_callback_words=len(rows),
   boundary_abi=dict(entry='001208B0',priority='stack+48',category='stack+44',done='EBX'),
   callback_assignment=dict(instruction='001A0F90',bytes=callback_store.hex().upper(),
    executed_in_this_oracle=False,source_callback=f'{inspected_callback:08X}',
    complete_owned_bind_proof='source-async-read-original.json; 1A0F75 owned-node binding, no allocator result supplied')),
  native_file_rows=resource_meta,actual_file_bank=dict(active_index=active,addresses=[f'{v:08X}'for v in addresses],tokens=handles),
  whole_caller_diagnostics=diagnostics,unique_instructions=len(seen),restored_pages_checked=len(pages),fpcw='023F',mxcsr='1F80',
  services_replaced=[],source_instruction_patches=[],native_residency_activation=False,original_completion_timeline_claimed=False)
 (ROOT/'analysis/multiplayer-shading/source-bitmap-async-read-original.json').write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({key:report[key]for key in['cases','checked_words','actual_allocator_requests','immutable_reads','unique_instructions','restored_pages_checked']},indent=2))
if __name__=='__main__':main()
