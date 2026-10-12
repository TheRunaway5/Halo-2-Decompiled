"""Observe whole original voice notification arm/cancel and pending polling.

Original source helpers, kernel time/timer/DPC owners and device guards execute
normally. Hardware breakpoints observe without guest code,
memory, register or callee-result writes. Kernel side effects are retained as
external inputs for later CPU comparison; no native timer/DSP/output claim.
"""
import hashlib,json,os,struct,sys
from pathlib import Path
import gdb
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
image_path=ROOT/'assets/disc/default.xbe';sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=Xbe(image_path);inferior=gdb.selected_inferior();read=lambda a,n:bytes(inferior.read_memory(a,n));word=lambda a:struct.unpack('<I',read(a,4))[0];reg=lambda n:int(gdb.parse_and_eval('$'+n))&0xffffffff
bp=lambda pc:gdb.Breakpoint(f'*0x{pc:x}',type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
def reach(pc,expected_sp):
 b=bp(pc)
 for _ in range(4096):
  gdb.execute('continue',to_string=True);assert reg('eip')==pc
  if reg('esp')==expected_sp:b.delete();return
  if _<5:print(f'Return activation mismatch at{pc:08x}: expected ESP{expected_sp:08x}, actual{reg("esp"):08x}',flush=True)
 b.delete();raise RuntimeError('Original activation return not reached')
def capture():
 output=Path(os.environ['H2_SOUND_NOTIFICATIONS_OUTPUT']);output.mkdir(parents=True,exist_ok=False)
 metadata=ROOT/'analysis/audio-spatial-20261011/sdk-notification-observer-code-v1.json';instructions=json.loads(metadata.read_text());specs={0x382ad9:('arm_immediate',4),0x382b08:('cancel',1),0x3811a6:('pending',0)};remaining=dict(specs);reports=[]
 for attempt in range(2048):
  observers=[bp(pc)for pc in remaining];gdb.execute('continue',to_string=True);entry=reg('eip')
  for b in observers:b.delete()
  assert entry in remaining;kind,count=remaining[entry];voice=reg('ecx');stack=reg('esp');caller=word(stack);arguments=list(struct.unpack('<'+'I'*count,read(stack+4,count*4)))if count else[]
  if kind in ('arm_immediate','cancel')and(arguments[0]>=6 or not word(voice+0x90+(arguments[0]<<5))&1 or kind=='cancel'and not word(voice+0x90+(arguments[0]<<5))&2)or kind=='pending'and int.from_bytes(read(voice+0x12,2),'little')&0x8001!=0x8001:gdb.execute('stepi',to_string=True);continue
  directory=output/kind;directory.mkdir();regions=[]
  def backing(name,address,size):
   if any(int(r['address'],16)<=address and address+size<=int(r['address'],16)+r['size']for r in regions):return
   assert not any(address<int(r['address'],16)+r['size']and int(r['address'],16)<address+size for r in regions),'Partially overlapping retained extent'
   regions.append(dict(name=name,address=f'{address:08x}',size=size,before_hex=read(address,size).hex()))
  device=word(voice+8);backing('voice',voice,0x158);backing('device',device,0x540)
  for head in (device+0x4a8,device+0x4b0):
   node=word(head);seen=set()
   while node!=head:
    assert node not in seen and len(seen)<128;seen.add(node);backing('list_node_'+str(len(regions)),node,32);node=word(node)
  verified=[]
  for pc,v in instructions.items():
   raw=bytes.fromhex(v['bytes']);assert image.read(int(pc,16),len(raw))==read(int(pc,16),len(raw));verified.append(dict(pc=pc,bytes=v['bytes']))
  report=dict(boundary=__doc__,kind=kind,entry=f'{entry:08x}',voice=f'{voice:08x}',arguments=[f'{a:08x}'for a in arguments],image_sha256=sha(image_path),tool_sha256=sha(__file__),metadata_sha256=sha(metadata),registers={n:reg(n)for n in ('eax','ebx','ecx','edx','esi','edi','ebp','esp','eip','eflags','fctrl','mxcsr')},regions=regions,unchanged_instructions=verified,kernel_calls=[],guards=[],instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[])
  (directory/'entry.json').write_text(json.dumps(report,indent=2)+'\n')
  def snapshot():return[read(int(r['address'],16),r['size'])for r in regions]
  def effects(before):
   out=[]
   for r,old in zip(regions,before):
    after=read(int(r['address'],16),r['size'])
    if after!=old:out.append(dict(region=r['name'],before_hex=old.hex(),after_hex=after.hex()))
   return out
  if kind in ('arm_immediate','cancel'):
   before_guard,after_guard,before_unlock,after_unlock=(0x37f255,0x37f25a,0x37f27f,0x37f284)if kind=='arm_immediate'else(0x37f078,0x37f07d,0x37f0a1,0x37f0a6)
   reach(before_guard,stack-28);before=snapshot();reach(after_guard,stack-28);report['guards'].append(dict(owner='00379c90',effects=effects(before)))
   active_flags=word(voice+0x90+(arguments[0]<<5));assert active_flags&1 or not active_flags&2,'Immediate record changed into a timed queued record'
   report['queued_after_guard']=bool(active_flags&2)
   reach(before_unlock,stack-28);before=snapshot();reach(after_unlock,stack-28);report['guards'].append(dict(owner='00379cb2',effects=effects(before)))
  else:
   reach(0x3811b2,stack-20);clock_output=word(reg('esp'));before=snapshot();reach(0x3811b8,stack-16);now=int.from_bytes(read(clock_output,8),'little')
   report['kernel_calls'].append(dict(name='query_system_time',time_hex=read(clock_output,8).hex(),actual_return_eax=f'{reg("eax"):08x}',effects=effects(before)))
   flags=int.from_bytes(read(voice+0x12,2),'little');limit=(int.from_bytes(read(voice+0x78,8),'little')+0x4c4b40)&0xffffffffffffffff
   assert flags&0x8001==0x8001 and (now^(1<<63))<(limit^(1<<63)),'Pending call no longer follows the retained waiting path'
  reach(caller,stack+4+count*4)
  for r in regions:r['after_hex']=read(int(r['address'],16),r['size']).hex()
  report.update(actual_return_pc=f'{caller:08x}',actual_return_eax=f'{reg("eax"):08x}',original_whole_call_returned_normally=True,original_kernel_calls_executed_normally=True,native_or_physical_DSP_claimed=False)
  (directory/'report.json').write_text(json.dumps(report,indent=2)+'\n');reports.append(dict(kind=kind,report_sha256=sha(directory/'report.json')));del remaining[entry]
  (output/'report.json').write_text(json.dumps(dict(boundary=__doc__,captures=reports,complete=not remaining,original_cold_boot=True,instruction_patches=[],guest_memory_writes=[],guest_register_writes=[],supplied_callee_returns=[]),indent=2)+'\n');print(f'Original {kind} returned normally; {len(report["kernel_calls"])} actual kernel calls',flush=True)
  if not remaining:return
 raise RuntimeError('Original notification call set not reached')
