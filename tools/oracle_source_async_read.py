#!/usr/bin/env python3
"""Original read-task and completion arithmetic with explicit phase inputs.

Complete request prefixes and whole callers over retained banks stop before real kernel
instructions. Post-I/O and post-callback controls are separate source phases
with explicit external outcomes; no kernel call is emulated or claimed to
return, and no original completion/readiness timeline is manufactured.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,re,subprocess,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,TASK,OUT,DONE,NODE,STACK,STOP=0x20000000,0x20001000,0x20002000,0x20003000,0x20004000,0x200fe000,0x200ff000
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,required=True);a=p.parse_args()
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};dirty=set();seen={};stops=set();calls=[];boundary=[]
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
 def event(pc):
  return dict(entry=f'{pc:08X}',registers={name:u.reg_read(reg)for name,reg in[
   ('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),
   ('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI)]},
   stack=words(read(u.reg_read(UC_X86_REG_ESP)+4,32)))
 def missing(g,access,address,size,value,data):
  try:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(at)
   return True
  except ValueError as error:
   boundary.append(dict(kind='uncaptured memory',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',reason=str(error)))
   g.emu_stop();return False
 def code(g,pc,size,data):
  try:original=x.read(pc,size)
  except (ValueError,KeyError):
   boundary.append(dict(kind='actual kernel entry; not executed',**event(pc)));g.emu_stop();return
  assert read(pc,size)==original,(hex(pc),'changed original instruction');seen[pc]=size
  if pc in[0x120ba0,0x1a0f10,0x1208b0,0x1209c0,0x1a0e70,0x120c40,0x120a30,0x2d2488,0x2d2241,0x2d6246,0x213760]:calls.append(event(pc))
  if pc in stops:g.emu_stop()
 def writing(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for at in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(at)
 u.hook_add(UC_HOOK_MEM_UNMAPPED,missing);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,writing)
 ga,gl=(int(v,16)for v in re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',s.report['registers_before']).groups())
 read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,val in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,16),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_FS,32)]:u.reg_write(reg,val)
 def reset():
  for at in dirty:u.mem_write(at,pages[at])
  dirty.clear();stops.clear();calls.clear();boundary.clear()
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.mem_write(STACK-64,packed([0]*32));u.mem_write(STACK,packed([STOP]));u.mem_write(DONE,b'\x7f')
 def run(entry):
  try:u.emu_start(entry,STOP,count=1000000)
  except UcError:
   if not boundary:raise
  assert u.reg_read(UC_X86_REG_EIP)==STOP or u.reg_read(UC_X86_REG_EIP)in stops or boundary
 cases=[];outputs=[];counts=collections.Counter();request_rows=[];rng=random.Random(0x1a0e70)
 allocator_report=ROOT/'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json'
 parent=json.loads(allocator_report.read_text())
 added=uint(s.read(0x4e28f0,4));counter=uint(s.read(0x4e0364,4))
 for record in parent['records']:
  event_row=next(c for c in record['source_calls']if c['entry']=='001A0F10');st=event_row['stack']
  request_rows.append([0,st[0],event_row['edx'],st[1],st[2],st[5],event_row['ebx'],st[4],st[3],added,counter])
 for _ in range(512):request_rows.append([0,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),
  rng.getrandbits(32),OUT if rng.randrange(2)else 0,DONE,rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32)])
 for row in request_rows:
  reset();put(0x4e28f0,packed([row[9]]));put(0x4e0364,packed([row[10]]))
  put(row[6],b'\x7f');u.reg_write(UC_X86_REG_EDX,row[2]);u.reg_write(UC_X86_REG_EBX,row[6])
  u.mem_write(STACK,packed([STOP,row[1],row[3],row[4],row[8],row[7],row[5]]));stops.add(0x1208b0);run(0x1a0f10)
  actual=words(read(STACK-0x28,40))+[uint(read(0x4e28f0,4)),row[6],read(row[6],1)[0],uint(read(0x4e0364,4))]
  assert not boundary;cases.append(row);outputs.append(actual);counts['request_prefix']+=1
  # Distinct admitted post-acquisition phase. The free-list callee is not made
  # to return; its required leased node is an explicit input to this phase.
  initial=[rng.getrandbits(32)for _ in range(16)];initial[1]=0xffffffff;initial[15]=0
  task=actual[:10];put(NODE,packed(initial));u.reg_write(UC_X86_REG_EAX,NODE)
  u.reg_write(UC_X86_REG_ESP,u.reg_read(UC_X86_REG_ESP)+4);stops.clear();stops.add(0x1209c0)
  run(0x1a0f75);assert not boundary
  bound=words(read(NODE,64));assert bound[15]==0;bound[15]=0xffffffff
  initial[15]=0xffffffff
  cases.append([5]+initial+task+[row[7],row[8],0x1a0e70,row[6]])
  outputs.append(bound);counts['read_bind_owned']+=1
 for _ in range(256):
  task=[rng.getrandbits(32)for _ in range(10)]
  priority,category,callback,done=[rng.getrandbits(32),rng.getrandbits(32),rng.getrandbits(32),DONE]
  supplied_added,supplied_counter=rng.getrandbits(32),rng.getrandbits(32)
  reset();put(TASK,packed(task));put(0x4e28f0,packed([supplied_added]));put(0x4e0364,packed([supplied_counter]))
  u.reg_write(UC_X86_REG_EBX,done);u.mem_write(STACK,packed([STOP,priority,TASK,category,callback]))
  stops.add(0x1208b0);run(0x120ba0);assert not boundary
  actual=[uint(read(0x4e28f0,4)),done,read(done,1)[0],uint(read(0x4e0364,4))]+words(read(TASK,40))
  cases.append([4]+task+[priority,category,callback,done,supplied_added,supplied_counter])
  outputs.append(actual);counts['generic_request_prefix']+=1
  initial=[rng.getrandbits(32)for _ in range(16)];initial[1]=0xffffffff;initial[15]=0
  put(NODE,packed(initial));u.reg_write(UC_X86_REG_EAX,NODE)
  u.reg_write(UC_X86_REG_ESP,u.reg_read(UC_X86_REG_ESP)+4);stops.clear();stops.add(0x1209c0)
  run(0x120bb5);assert not boundary
  bound=words(read(NODE,64));assert bound[15]==0;bound[15]=0xffffffff;initial[15]=0xffffffff
  cases.append([5]+initial+task+[priority,category,callback,done])
  outputs.append(bound);counts['generic_bind_owned']+=1
 tasks=[];limits=[0,1,0x200,0x8000,uint(s.read(0x46e488,4)),0xffffffff]
 for total,processed in[(0,0),(1,0),(1,1),(2,1),(0xffffffff,0),(0xffffffff,0xffffffff),
   (0,1),(0x80000000,0),(0,0xffffffff)]:
  for limit in limits:
   for out in[0,OUT]:tasks.append(([72,0xfffffff0,total,0xfffffff0,out,processed,0xaabbccdd,0xdeadbeef,3,4],limit))
 for _ in range(1024):
  task=[rng.getrandbits(32)for _ in range(10)];task[4]=OUT if rng.randrange(2)else 0
  tasks.append((task,rng.getrandbits(32)))
 for task,limit in tasks:
  reset();u.mem_write(TASK,packed(task));put(0x46e488,packed([limit]));u.mem_write(STACK,packed([STOP,TASK]));stops.add(0x2d2488)
  run(0x1a0e70);assert not boundary
  seek_stack=words(read(u.reg_read(UC_X86_REG_ESP)+4,16))
  assert seek_stack[0]==task[0]and seek_stack[1]==task[3]and seek_stack[3]==0
  assert uint(read(seek_stack[2],4))==0
  requested=u.reg_read(UC_X86_REG_EDI)
  # Independent controlled post-seek phase. No SetFilePointer call is made to return.
  reset();u.mem_write(TASK,packed(task));u.reg_write(UC_X86_REG_ESI,TASK);u.reg_write(UC_X86_REG_EDI,requested)
  u.reg_write(UC_X86_REG_ESP,STACK-12);u.mem_write(STACK-12,packed([0,0,0,STOP,0]));stops.add(0x2d2241)
  run(0x1a0eb2);assert not boundary
  read_stack=words(read(u.reg_read(UC_X86_REG_ESP)+4,20))
  assert read_stack[0]==task[0]and read_stack[2]==requested and read_stack[4]==0
  assert uint(read(read_stack[3],4))==0
  cases.append([1]+task+[limit,0,0,0]);outputs.append([seek_stack[0],seek_stack[1],read_stack[1],read_stack[2]]);counts['read_prepare']+=1
  for seek,transfer in[(0,0),(0,requested),(0,requested-1 if requested else 0),
    (0xffffffff,0),(0,rng.getrandbits(32))]:
   reset();u.mem_write(TASK,packed(task));u.mem_write(OUT,packed([0xcafebabe]));u.reg_write(UC_X86_REG_ESI,TASK)
   u.reg_write(UC_X86_REG_EDI,requested);u.reg_write(UC_X86_REG_ESP,STACK-12)
   u.mem_write(STACK-12,packed([0,0,0,STOP,transfer]));entry=0x1a0ee8 if seek==0xffffffff else 0x1a0ecb
   run(entry);assert not boundary
   mutated=words(read(TASK,40));write_present=int(seek!=0xffffffff and task[4]!=0)
   actual=mutated+[u.reg_read(UC_X86_REG_EAX),write_present,uint(read(OUT,4))if write_present else 0,uint(read(OUT,4))]
   cases.append([2]+task+[limit,seek,transfer,0xcafebabe]);outputs.append(actual);counts['read_finish']+=1
 for result in[0,1,2,3,0xffffffff,0x80000000,0x7fffffff]:
  for done in[0,DONE]:
   for initial in[0,0x7f,0xff]:
    reset();u.mem_write(DONE,bytes([initial]));node=[0]*16;node[14]=done;u.mem_write(NODE,packed(node))
    u.reg_write(UC_X86_REG_ESI,NODE);u.reg_write(UC_X86_REG_EAX,result);u.reg_write(UC_X86_REG_ESP,STACK-8)
    stops.update([0x120a30,0x2d6246]);run(0x120c7c);assert not boundary
    pc=u.reg_read(UC_X86_REG_EIP);remove=int(pc==0x120a30);signal=int(pc==0x2d6246)
    if signal:assert uint(read(u.reg_read(UC_X86_REG_ESP)+8,4))==1
    write_present=int(result==1 and done!=0)
    cases.append([3,result,done,initial]);outputs.append([remove,signal,write_present,done if write_present else 0,read(DONE,1)[0]])
    counts['callback_effect']+=1
 diagnostics=[]
 for entry in[0x1a0f10,0x1a0e70,0x120c40]:
  reset()
  if entry==0x1a0f10:
   row=request_rows[0];put(row[6],b'\x7f');u.reg_write(UC_X86_REG_EDX,row[2]);u.reg_write(UC_X86_REG_EBX,row[6])
   u.mem_write(STACK,packed([STOP,row[1],row[3],row[4],row[8],row[7],row[5]]))
  elif entry==0x1a0e70:
   u.mem_write(TASK,packed([request_rows[0][1],request_rows[0][2],request_rows[0][3],request_rows[0][4],0,0,0,0,0,0]));u.mem_write(STACK,packed([STOP,TASK]))
  run(entry);assert boundary
  diagnostics.append(dict(entry=f'{entry:08X}',completed=False,boundary=boundary.copy(),source_calls=calls.copy()))
 reset()
 for at,initial in pages.items():assert bytes(u.mem_read(at,4096))==initial
 encoded='\n'.join(' '.join(f'{v:08X}'for v in row)for row in cases)+'\n'
 native=subprocess.run([str(a.binary.resolve()),'--probe'],input=encoded,text=True,capture_output=True,check=True)
 actual=[list(int(v,16)for v in line.split())for line in native.stdout.splitlines()];assert len(actual)==len(outputs)
 for i,(native_row,original)in enumerate(zip(actual,outputs)):assert native_row==original,(i,cases[i],native_row,original)
 fixture=ROOT/'tests/fixtures/retail_source_async_read.tsv';fixture.write_text('# External read/callback outcomes are controlled phase inputs, never a source I/O timeline.\n'+
  '\n'.join(' '.join(f'{v:08X}'for v in row)+' | '+' '.join(f'{v:08X}'for v in out)for row,out in zip(cases,outputs))+'\n')
 report=dict(boundary=__doc__,source_xbe_sha256=PIN,snapshot_ram_sha256=sha(s.ram),snapshot_manifest_sha256=sha((s.directory/'export.json').read_bytes()),
  tool_sha256=sha(Path(__file__).read_bytes()),helper_sha256=sha((ROOT/'src/source_async_jobs.cpp').read_bytes()),
  header_sha256=sha((ROOT/'include/halo2/source_async_jobs.hpp').read_bytes()),
  test_sha256=sha((ROOT/'tests/source_async_read_tests.cpp').read_bytes()),fixture_sha256=sha(fixture.read_bytes()),
  allocator_report_sha256=sha(allocator_report.read_bytes()),native_binary_sha256=sha(a.binary.read_bytes()),
  cases=len(cases),checked_words=sum(len(r)for r in outputs),groups=dict(counts),actual_allocator_requests=168,
  literal_chunk_limit=uint(s.read(0x46e488,4)),whole_caller_diagnostics=diagnostics,
  unique_instructions=len(seen),instruction_sha256=sha(b''.join(packed([pc])+x.read(pc,n)for pc,n in sorted(seen.items()))),
  restored_pages_checked=len(pages),fpcw='023F',mxcsr='1F80',
  controlled_external_phase_inputs=['leased-node request binding1A0F75/120BB5; no allocator result supplied',
   'post-successful-seek ReadFile argument producer1A0EB2','supplied read count in post-ReadFile1A0ECB','failed-seek tail1A0EE8','callback return at120C7C'],
  services_replaced=[],source_instruction_patches=[],readiness_timeline_reconstructed=False)
 path=ROOT/'analysis/multiplayer-shading/source-async-read-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
 fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({key:report[key]for key in['cases','checked_words','groups','literal_chunk_limit','unique_instructions','restored_pages_checked']},indent=2))
if __name__=='__main__':main()
