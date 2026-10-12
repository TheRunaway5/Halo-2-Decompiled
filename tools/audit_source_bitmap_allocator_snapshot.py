#!/usr/bin/env python3
"""Audit unchanged 12D5A0 over immutable real bitmap/cache/async banks.

Only caller arguments and a disposable CPU stack are supplied. Real cache,
bitmap, pool, asynchronous, and fence-RAM state comes from the paused snapshot.
Stop before an unknown kernel instruction, uncaptured access, or hardware
kick/wait entry. No service, busy status, allocation or I/O result is replaced.
This is a diagnostic call on retained state, not original live scheduling.
"""
from pathlib import Path
import collections,hashlib,json,re,struct,sys
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,words,packed
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP=0x20000000,0x200fe000,0x200ff000
WATCH=[0x12d5a0,0x13d370,0x12d1a0,0x12d160,0x13d830,0x12d200,0x3fb710,0x16b840,0x16b910,0x16ba40,
 0x12d6d0,0x213760,0x1a0f10,0x1208b0,0x1209c0,0x2d63fa,0x2d62e4,0x2d62c4,0x3fac30,0x3fad50]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert sha(image.data)==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000)
 pages={};dirty=set();seen={};calls=[];boundary=[];writes=[];allocator_result=[];all_calls=collections.Counter()
 busy_trace=[];busy_handle=[None];busy_header_inputs={}
 def page(address):
  p=address&~4095
  if p not in pages:
   raw=snapshot.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw
 def read(address,size):
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=p<SYNTH+0x100000:page(p)
  return bytes(u.mem_read(address,size))
 def get(address):return uint(read(address,4))
 def event(entry):
  sp=u.reg_read(UC_X86_REG_ESP)
  return dict(entry=f'{entry:08X}',eax=u.reg_read(UC_X86_REG_EAX),ecx=u.reg_read(UC_X86_REG_ECX),edx=u.reg_read(UC_X86_REG_EDX),
   esi=u.reg_read(UC_X86_REG_ESI),edi=u.reg_read(UC_X86_REG_EDI),ebx=u.reg_read(UC_X86_REG_EBX),stack=words(read(sp+4,24)))
 def unmapped(g,access,address,size,value,data):
  try:
   for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(p)
   return True
  except ValueError as error:
   boundary.append(dict(kind='uncaptured memory/MMIO',pc=f'{g.reg_read(UC_X86_REG_EIP):08X}',address=f'{address:08X}',size=size,access=access,reason=str(error)))
   g.emu_stop();return False
 def code(g,pc,size,data):
  try:original=image.read(pc,size)
  except (ValueError,KeyError):
   boundary.append(dict(kind='non-XBE kernel/service entry',**event(pc)));g.emu_stop();return
  assert read(pc,size)==original,(hex(pc),'changed original instruction');seen[pc]=size
  if pc==0x12d5d8:allocator_result.append(g.reg_read(UC_X86_REG_EAX))
  if pc==0x12d1a0:busy_handle[0]=get(g.reg_read(UC_X86_REG_ESP)+4)
  if pc in[0x12d1d8,0x12d1f0]:busy_trace.append([busy_handle[0],g.reg_read(UC_X86_REG_EAX)&255])
  if pc==0x3fb710:
   header=g.reg_read(UC_X86_REG_ECX)
   if header not in busy_header_inputs:busy_header_inputs[header]=read(header,20).hex()
  if pc in WATCH:
   all_calls[f'{pc:08X}']+=1
   if pc not in[0x12d1a0,0x3fb710]:calls.append(event(pc))
  if pc in[0x3fac30,0x3fad50]:
   boundary.append(dict(kind='hardware kick/wait entry',**event(pc)));g.emu_stop()
 def write(g,access,address,size,value,data):
  if not SYNTH<=address<SYNTH+0x100000:
   for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):dirty.add(p)
   writes.append(dict(address=f'{address:08X}',size=size,value=f'{value&((1<<(size*8))-1):X}'))
 u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code);u.hook_add(UC_HOOK_MEM_WRITE,write)
 gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);ga,gl=(int(v,16)for v in gdtr.groups())
 read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,selector in[(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
 manager=get(0x4e6464);pool=get(manager+0x64);images=get(0x4e6454)
 banks={}
 for label,pointer,size in[('manager',manager,0x70),('physical_pool_header',pool,0x4c),
  ('physical_rows',get(pool+0x44),512*24),('physical_occupied',get(pool+0x48),64),
  ('image_pool_header',images,0x4c),('image_rows',get(images+0x44),512*40),('image_occupied',get(images+0x48),64)]:
  raw=read(pointer,size);banks[label]=dict(pointer=f'{pointer:08X}',bytes=size,sha256=sha(raw),header_hex=raw.hex()if size<=0x70 else None)
 initial_globals={f'{p:08X}':read(p,n).hex()for p,n in[(0x4e6460,4),(0x4e6484,4),(0x4e6479,1),(0x4e6488,4),
  (0x4e0358,4),(0x4e035c,4),(0x4e28e8,4),(0x4e28ec,4),(0x4e28f0,4),(0x407488,4),(0x55aca8,4),(0x557c90,4),(0x558494,4),(0x558c98,4)]}
 parent_path=ROOT/'analysis/multiplayer-shading/source-bitmap-cpu-selection-original.json';parent=json.loads(parent_path.read_text())
 candidates=[]
 for case in parent['records'][:64]:
  state=case['input'];pointer=state['record'];raw=snapshot.read(pointer,116);assert raw.hex()==state['raw_record_hex']
  for index in range(3):
   location,size,handle=uint(raw,0x1c+index*4),uint(raw,0x34+index*4),uint(raw,0x28+index*4)
   if handle==0xffffffff and location!=0xffffffff and size and(index==0 or size>0x400):candidates.append((pointer,index,location,size))
 assert len(candidates)==42
 records=[]
 for pointer,index,location,size in candidates:
  for wait,force in[(0,0),(0,1),(1,0),(1,1)]:
   for p in dirty:u.mem_write(p,pages[p])
   dirty.clear();calls.clear();boundary.clear();writes.clear();allocator_result.clear();busy_trace.clear();busy_handle[0]=None
   for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
   u.reg_write(UC_X86_REG_EAX,wait);u.reg_write(UC_X86_REG_ECX,pointer);u.reg_write(UC_X86_REG_ESP,STACK)
   u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   u.mem_write(STACK,packed([STOP,index,force,2]));before=read(pointer,116)
   try:u.emu_start(0x12d5a0,STOP,count=2000000)
   except UcError:
    if not boundary:raise
   pc=u.reg_read(UC_X86_REG_EIP);assert pc==STOP or boundary,hex(pc)
   after=read(pointer,116);handle=uint(after,0x28+index*4)
   changed_pages=[dict(pointer=f'{p:08X}',before_sha256=sha(pages[p]),after_sha256=sha(bytes(u.mem_read(p,4096))))for p in sorted(dirty)if bytes(u.mem_read(p,4096))!=pages[p]]
   row=dict(bitmap_record=f'{pointer:08X}',partition=index,location=f'{location:08X}',authored_bytes=size,wait=wait,force=force,priority=2,
    before_record_hex=before.hex(),after_record_hex=after.hex(),allocator_return=allocator_result.copy(),
    completed=not boundary,return_al=u.reg_read(UC_X86_REG_EAX)&255 if not boundary else None,
    source_calls=calls.copy(),busy_callback_trace=busy_trace.copy(),boundary=boundary.copy(),source_writes=writes.copy(),changed_pages=changed_pages,
    image_row_hex=read(get(images+0x44)+(handle&65535)*40,40).hex()if handle!=0xffffffff else None)
   records.append(row)
  print('actual allocator record',f'{pointer:08X}',index,records[-1]['boundary'][0]['kind']if records[-1]['boundary']else'complete',flush=True)
 failure_controls=[]
 for age in[2,5]:
  for p in dirty:u.mem_write(p,pages[p])
  dirty.clear();calls.clear();boundary.clear();writes.clear();allocator_result.clear();busy_trace.clear();busy_handle[0]=None
  capacity=get(manager+0x30)<<get(manager+0x34);size=capacity+1
  for reg in[UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.mem_write(STACK,packed([STOP,manager,size,age]));before=read(manager,0x70)
  u.emu_start(0x13d370,STOP,count=2000000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP and not boundary and u.reg_read(UC_X86_REG_EAX)==0xffffffff
  assert u.reg_read(UC_X86_REG_ESP)==STACK+16
  failure_controls.append(dict(label='explicit request one byte beyond the literal retained capacity',size=size,age=age,
    returned_handle=u.reg_read(UC_X86_REG_EAX),before_manager_hex=before.hex(),after_manager_hex=read(manager,0x70).hex(),
    source_calls=calls.copy(),busy_callback_trace=busy_trace.copy(),source_writes=writes.copy()))
 for p in dirty:u.mem_write(p,pages[p])
 for label,bank in banks.items():assert sha(read(int(bank['pointer'],16),bank['bytes']))==bank['sha256'],label
 boundaries=collections.Counter(b['kind']+':'+b.get('entry',b.get('pc',''))for r in records for b in r['boundary'])
 assert boundaries=={'non-XBE kernel/service entry:80020F5A':168}
 assert uint(image.read(0x411654,4))==0x800000ea and uint(snapshot.read(0x411654,4))==0x80020f5a
 assert all(r['allocator_return']and r['allocator_return'][0]!=0xffffffff and bytes.fromhex(r['image_row_hex'])[3]==0 for r in records)
 device=get(0x407488);fence_pointer=get(device+0x30)
 report=dict(boundary=__doc__,source_xbe_sha256=PIN,tool_sha256=sha(Path(__file__).read_bytes()),snapshot_ram_sha256=sha(snapshot.ram),
  snapshot_manifest_sha256=sha((snapshot.directory/'export.json').read_bytes()),cpu_selection_report_sha256=sha(parent_path.read_bytes()),
  fpcw='023F',mxcsr='1F80',supplied_control_configuration=True,real_banks=banks,initial_globals=initial_globals,
  candidates=42,cases=len(records),complete_cases=sum(r['completed']for r in records),direct_failure_controls=len(failure_controls),failure_controls=failure_controls,boundaries=dict(boundaries),
  source_calls=dict(all_calls),busy_header_inputs={f'{p:08X}':raw for p,raw in sorted(busy_header_inputs.items())},
  busy_fence_RAM=dict(device=f'{device:08X}',queued_fence=get(device+0x2c),completed_fence_pointer=f'{fence_pointer:08X}',completed_fence=get(fence_pointer)),
  kernel_export_reference=dict(file='tools/reference/halo2-native-reference/scripts/vendor/xbox_kernel_exports.py',
    sha256=sha((ROOT/'tools/reference/halo2-native-reference/scripts/vendor/xbox_kernel_exports.py').read_bytes())),
  read_only_source_regions={label:dict(start=f'{start:08X}',end=f'{end:08X}',sha256=sha(image.read(start,end-start)))for label,start,end in[
    ('separate timed pump, not called by12D5A0',0x12d400,0x12d51e),('async worker done producer',0x120c40,0x120cde),('actual read callback',0x1a0e70,0x1a0f09)]},
  unique_instructions=len(seen),instruction_sha256=sha(b''.join(packed([pc])+image.read(pc,n)for pc,n in sorted(seen.items()))),
  abi={'12D5A0':{'registers':{'ECX':'bitmap record','AL':'wait boolean'},'stack':['partition','force boolean','priority'],'returns':'AL; RET C hex'},
    '13D370':{'stack':['physical manager','authored bytes','age parameter: 2 when wait or force, else 5'],'returns':'salted physical-block handle or NONE; RET C hex'},
    '213760':{'registers':{'ECX':'authored file location'},'stack':['sector-aligned bytes','source physical address','NULL bytes-read output','image+3 done pointer','category3','priority (7 when wait and not force)']},
    'kernel_boundary':{'entry':'80020F5A','pinned_IAT':'411654','pinned_ordinal':'EA','symbol':'NtWaitForSingleObjectEx','source_wrapper':'2D62E4',
      'arguments':['free-list mutex handle4','WaitMode1 (UserMode)','Alertable0','TimeoutNULL']}},
  rules={'reservation':'13D370 executes candidate-ring selection, the original busy and delete/eviction callbacks, overlapping-block eviction, salted physical-pool allocation and linked-list insertion. No replacement candidate, slot, busy value, or address is supplied.',
    'registered_state_callback':'12D160 is registered at manager+28 by the initializer; the verified13D370 paths do not invoke it. Registration is not execution evidence.',
    'physical_address':'(physical block+8 << manager+34) + 4E6460. Retained manager uses 4096-byte pages and ranking state2; its current linked list, age limits and salted pools are exact snapshot inputs.',
    'pending_header':'12D5A0 creates the same-salt image slot, clears ready+3 and flagbit0, writes partition+4/owner+10 and bitmap handle+28/address+40, writes bitmap+54 for partition0, then builds 12D6D0 header. It does not set readiness true.',
    'queue_boundary':'213760 chooses the actual open file from location bits30..31; 1A0F10 constructs the read task, increments4E28F0 and clears done. 1208B0 reaches the real mutex wait before consuming a free job node. Kernel completion, queued job ID, 12D5A0 final priority/load-counter stores and successful return are not supplied.',
    'readiness_producer':'Read-only source: async worker120C79 calls the job callback. Only callback result1 with nonNULL done pointer stores done=1 at120C8C. The read callback1A0E70 returns1 for complete or short transfer; its file seek/read kernel services and worker scheduling remain unresolved here.',
    'minimum_seam':'A bounded CPU port can expose physical reservation/eviction plus pending image/header state over explicit exact manager/pool/bitmap banks and the proved busy inputs. Async job submission must remain a distinct unresolved service boundary; allocation cannot imply ready pixels or a completed job. No native residency activation is proposed by this report.'},
  services_replaced=[],source_instruction_patches=[],records=records)
 output=ROOT/'analysis/multiplayer-shading/source-bitmap-allocator-snapshot-original.json';output.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report[k]for k in['candidates','cases','complete_cases','direct_failure_controls','boundaries','source_calls','unique_instructions']},indent=2))
if __name__=='__main__':main()
