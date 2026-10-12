#!/usr/bin/env python3
"""Execute complete original voice pause, envelope and common-stop CPU owners.

Expected stores, device command words and calls come from unchanged original
instructions. Kernel time/timers/DPC, parent buffer stop, forced pending retirement and FIFO/MMIO backing
are declared ABI inputs, not physical Xbox devices or audible parity. Native
code supplies no original expected values. No genuine emulator case selected.
"""
import argparse,atexit,ctypes,hashlib,json,os,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda b,i=0:struct.unpack_from('<I',b,i)[0]
put=lambda b,i,v:struct.pack_into('<I',b,i,v&0xffffffff)
A=dict(voice=0x1001000,device=0x1002000,previous=0x1003000,next=0x1004000,slots=0x1005000,
 data=0x100d000,mapping=0x100e000,group_registers=0x38699c,counter=0x387110,slot_pointer=0x3872b0,mapping_pointer=0x3872c0,
 descriptor=0x1006000,parent=0x1007000,buffer=0x1008000,buffer_data=0x1009000,vtable=0x41727c,mmio_status=0xfe801100,mmio_registers=0xfe802000,mmio_commands=0xfe820000)
B=dict(group_register_table='0038699c',force_counter_address='00387110',slot_mapping_pointer_address='003872b0',voice_mapping_pointer_address='003872c0',voice_vtable='0041727c',status_method='003829f7',arm_method='00382ad9',cancel_method='00382b08')
SPEC=dict(pause=(0x3811fa,1),envelope=(0x381122,0),stop=(0x381ef8,1))
SITES={0x384abd:('stall_execution',1),0x384ae2:('stall_execution',1),0x3810b1:('insert_queue_dpc',3),0x3811b2:('query_system_time',1),0x37f13a:('query_system_time',1),0x381faf:('query_system_time',1),0x37eb30:('cancel_timer',1),0x37eb3d:('remove_queue_dpc',1),0x37eb5d:('set_timer',4),0x37f1db:('set_timer',4)}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pairing',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 path=ROOT/'assets/disc/default.xbe';assert sha(path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';image=Xbe(path);code=_Code(image,'DSOUND')
 layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n');subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
 host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0,extra_declared_regions=[(0xfe800000,0x30000)]);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
 guards={};mmio={};visited=set();rows=[]
 for entry in(0x384a36,0x380ee1,0x3810bd,0x3829f7,0x3811a6,0x381e0e,0x37f407,0x3808e4,0x3824ea,0x379c60,0x382ad9,0x37f245,0x37f11a,0x37eae2,0x381ef8,0x3811fa,0x381122,0x37ed0f,0x37ed36,0x3808be,0x382b08,0x37f068,0x37eb07):
  for line in(args.pairing/'instructions'/f'{entry:08x}-base.asm').read_text().splitlines():
   pc,raw,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(raw)==host.read(pc,i.size)
   if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in(0x379c90,0x379cb2):guards[pc]=i.operands[0].imm
   for op in i.operands:
    if op.type==3 and op.mem.disp&0xff000000==0xfe000000:mmio[pc]=op
 stack,stop=0x100f000,0x3000000
 def measure(label,kind,state,arguments=(),times=(),actions=()):
  state={name:raw for name,raw in state.items() if name!="slots"}
  entry,argc=SPEC[kind];assert len(arguments)==argc
  for name,raw in state.items():
   # DSOUND contains both executable instructions and these separately named
   # original data cells. Its coarse section guard deliberately rejects them;
   # permit only this fixed data allowlist, never a source instruction address.
   if A[name]<0x1000000:
    assert name in('group_registers','counter','slot_pointer','mapping_pointer','vtable')
    assert os.pwrite(host.memory_fd,raw,A[name])==len(raw)
   else:host.write(A[name],raw)
  host.write(stack-0x800,bytes([0x5b])*0x804);host.write(stack,struct.pack('<'+'I'*(argc+1),stop,*arguments));host.continuing=False;inputs=dict(rcx=A['voice']);stores=[];writes=[];reads=[];locks=[];calls=[];clock=0;done=set()
  def action(event,value):
   for n,a in enumerate(actions):
    if n not in done and a.get(event)==value:host.write(a['address'],bytes.fromhex(a['hex']));done.add(n)
  def collect(changed):
   for item in changed:
    pc,address,size,before,after=item
    if address>=0xfe800000:
     assert size==4;writes.append(dict(pc=f'{pc:08x}',address=f'{address:08x}',value=f'{after:08x}'));action('write',len(writes))
    elif any(A[n]<=address and address+size<=A[n]+len(raw)for n,raw in state.items()):stores.append(item);action('pc',f'{pc:08x}')
    else:assert stack-0x1000<=address and address+size<=stack+0x100,(label,'Unretained store',item)
  for _ in range(20000):
   sites={int(a['pc'],16)+code.at(int(a['pc'],16)).size for n,a in enumerate(actions)if'pc'in a and n not in done}
   boundaries={stop,*guards,*mmio,*SITES,0x382dd2,0x3810bd,*sites}
   result,changed,executed=host.run_traced(entry,stack,code,0,0x100000000,stop_addresses=boundaries,**inputs);inputs={};visited.update(executed);collect(changed);pc=result.rip
   if pc==stop:break
   host.continuing=True
   if pc in sites:entry=pc;continue
   if pc in guards:locks.append(f'{guards[pc]:08x}');action('guard',len(locks));entry=pc+code.at(pc).size;continue
   if pc in mmio:
    op=mmio[pc];reg=lambda r:0 if not r else getattr(result,{'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}[code.at(pc).reg_name(r)])&0xffffffff
    address=(reg(op.mem.base)+reg(op.mem.index)*op.mem.scale+op.mem.disp)&0xffffffff;assert 0xfe800000<=address<0xfe830000
    is_read=bool(op.access&1);value=int.from_bytes(host.read(address,op.size),'little')
    result,changed,executed=host.run_traced(pc,stack,code,0,0x100000000,stop_addresses=(pc+code.at(pc).size,));visited.update(executed);collect(changed)
    if is_read:reads.append(dict(pc=f'{pc:08x}',address=f'{address:08x}',size=op.size,value=f'{value:08x}'));action('read',len(reads))
    entry=result.rip;continue
   if pc==0x3810bd:
    name='force_pending_voice';calls.append(dict(name=name,arguments=[f'{result.rcx&0xffffffff:08x}'],time=None));action('callback',name)
    result.rip=u32(host.read(result.rsp,4));result.rsp+=4;host.trace(13,0,ctypes.addressof(result));entry=result.rip;continue
   if pc==0x382dd2:
    values=[result.rcx&0xffffffff,u32(host.read(result.rsp+4,4))];name='stop_buffer';calls.append(dict(name=name,arguments=[f'{v:08x}'for v in values],time=None));action('callback',name)
    result.rax=0;result.rip=u32(host.read(result.rsp,4));result.rsp+=8;host.trace(13,0,ctypes.addressof(result));entry=result.rip;continue
   name,count=SITES[pc];values=list(struct.unpack('<'+'I'*count,host.read(result.rsp,count*4)));time=None
   if name=='query_system_time':
    assert clock<len(times),(label,'Undeclared clock');time=times[clock];clock+=1;host.write(values[0],struct.pack('<Q',time));values=[]
   calls.append(dict(name=name,arguments=[f'{v:08x}'for v in values],time=f'{time:016x}'if time is not None else None));action('callback',name)
   if name=='stall_execution':action('stall',sum(c['name']==name for c in calls))
   if name=='query_system_time':action('clock',clock)
   result.rsp+=count*4;host.trace(13,0,ctypes.addressof(result));entry=pc+code.at(pc).size
  else:raise RuntimeError((label,'Retirement boundary limit'))
  assert result.rsp==stack+4+argc*4 and len(done)==len(actions)and clock==len(times),(label,hex(result.rsp),done,len(actions),clock,len(times))
  rows.append(dict(label=label,kind=kind,genuine=False,arguments=[f'{v:08x}'for v in arguments],times=[f'{t:016x}'for t in times],actions=list(actions),calls=calls,locks=locks,reads=reads,writes=writes,result=f'{result.rax&0xffffffff:08x}',regions=[dict(name=n,address=f'{A[n]:08x}',before_hex=raw.hex(),after_hex=host.read(A[n],len(raw)).hex())for n,raw in state.items()],stores=[dict(pc=f'{p:08x}',address=f'{a:08x}',size=size,before=f'{before:0{size*2}x}',after=f'{after:0{size*2}x}')for p,a,size,before,after in stores]))
 def seed(group=0,count=1,previous=False,next=False,flags=0x8001,current=None,last=None):
  state={n:bytearray(size)for n,size in dict(voice=0x158,device=0x540,previous=0x158,next=0x158,slots=0x8000,data=0x100,mapping=0x40,counter=4,slot_pointer=4,mapping_pointer=4,mmio_status=1,mmio_registers=0x80,mmio_commands=0x308,descriptor=0xc0,parent=0x30,buffer=0x100,buffer_data=0x100).items()};state['group_registers']=bytearray(image.read(A['group_registers'],36));state['vtable']=bytearray(image.read(A['vtable'],40))
  put(state['voice'],0,A['vtable']);put(state['voice'],8,A['device']);put(state['voice'],0x80,A['data']);put(state['voice'],0x68,A['mapping']);put(state['voice'],0x6c,4);struct.pack_into('<H',state['voice'],0x12,flags);state['voice'][0x64]=count;state['voice'][0x65]=group
  put(state['slot_pointer'],0,A['slots']);put(state['mapping_pointer'],0,A['mapping']);state['mapping'][:]=bytes([0x80])*64
  head=A['device']+0x488+group*8;members=(['previous']if previous else[])+['voice']+(['next']if next else[]);links=[A[n]+0x4c for n in members]
  for i,n in enumerate(members):
   put(state[n],0x4c,links[i+1]if i+1<len(links)else head);put(state[n],0x50,links[i-1]if i else head)
  put(state['device'],head-A['device'],links[0]);put(state['device'],head-A['device']+4,links[-1]);state['previous'][0x64]=2;state['next'][0x64]=1
  for n,slots in [('previous',[1,2]),('voice',list(range(3,3+count))),('next',[9])]:
   for i,slot in enumerate(slots):struct.pack_into('<H',state[n],0xc+i*2,slot)
  order=([1,2]if previous else[])+list(range(3,3+count))+([9]if next else[])
  for i,slot in enumerate(order):put(state['slots'],slot*128+0x7c,0xa5a50000|(order[i+1]if i+1<len(order)else 0xffff))
  for at in(0x4a0,0x4a8,0x4b0):put(state['device'],at,A['device']+at);put(state['device'],at+4,A['device']+at)
  for i in range(6):at=0x88+i*32;put(state['voice'],at,A['voice']+at);put(state['voice'],at+4,A['voice']+at);put(state['voice'],at+8,1)
  put(state['voice'],0x14,0xa380a55a)
  put(state['voice'],0x70,A['descriptor']);put(state['buffer'],0x80,A['buffer_data']);put(state['buffer'],0x44,A['buffer']+0x44);put(state['buffer'],0x48,A['buffer']+0x44)
  put(state['mmio_commands'],0x10,0x100);put(state['mmio_registers'],0,0x18)
  for g in range(3):
   put(state['mmio_registers'],0x54+g*12,order[0]if order else 0xffff);put(state['mmio_registers'],0x58+g*12,3 if current is None else current);put(state['mmio_registers'],0x5c+g*12,3 if last is None else last)
  return state
 for flags in(0,1,3,0x43,0x107,0x407,0x847,0xffff):
  for requested in(0,4,0x40,0x400,0x444,0xffffffff):
   for count in(0,1,3):
    for total in(0,27,29,0xffffffff):
     state=seed(count=count,flags=flags);put(state['device'],0x53c,total)
     measure(f'pause_flags{flags:04x}_request{requested:08x}_count{count}_total{total}','pause',state,[requested])
 for flags in(0,1,3,0x43,0x807,0xffff):
  for count in(0,1,2,3):
   measure(f'envelope_flags{flags:04x}_count{count}','envelope',seed(count=count,flags=flags))
 for flags in(0,1,3,0x43,0x107,0x407,0x847,0x8000,0x8001,0xffff):
  for group in(0,2,3):
   for count in(0,1,3):
    state=seed(group=group,count=count,flags=flags)
    measure(f'stop_flags{flags:04x}_group{group}_count{count}','stop',state,[0],times=[100]if flags&3==3 else[])
 for parent in('none','empty','eligible','active_peer'):
  state=seed(flags=3)
  if parent!='none':put(state['descriptor'],0xb0,A['parent'])
  if parent in('eligible','active_peer'):put(state['parent'],0x20,A['buffer']);put(state['buffer_data'],8,0x82000)
  if parent=='active_peer':put(state['buffer'],0x44,A['previous']+0x4c);put(state['previous'],0x4c,A['buffer']+0x44);state['previous'][2]=3
  measure(f'stop_parent_{parent}','stop',state,[0],times=[123])
 for flags in(0,1,3,0x43,0x8001):
  state=seed(flags=flags)
  active=flags&3==3
  times=[100,200]if active else[200]if flags&0x8000 else[]
  actions=[dict(clock=2 if active else 1,address=A['voice']+0x12,hex='0100')]if active or flags&0x8000 else[]
  measure(f'stop_wait_flags{flags:04x}','stop',state,[1],times=times,actions=actions)
 state=seed(flags=3,count=3)
 measure('envelope_reloads_count_after_flags','envelope',state,actions=[dict(pc='0038114e',address=A['voice']+0x64,hex='01')])
 state=seed(flags=3,count=3)
 measure('pause_reloads_count_after_first_command','pause',state,[4],actions=[dict(write=1,address=A['voice']+0x64,hex='01')])
 state=seed(flags=3,count=3)
 measure('stop_reloads_count_after_first_command','stop',state,[0],times=[100],actions=[dict(write=1,address=A['voice']+0x64,hex='01')])
 state=seed(flags=3,count=1)
 measure('stop_reloads_format_between_commands','stop',state,[0],times=[100],actions=[dict(write=1,address=A['voice']+0x14,hex='78563412')])
 state=seed(flags=3,count=1);put(state['parent'],0x20,A['buffer']);put(state['buffer_data'],8,0x82000)
 measure('stop_parent_published_by_clock','stop',state,[0],times=[100],actions=[dict(clock=1,address=A['descriptor']+0xb0,hex=struct.pack('<I',A['parent']).hex())])
 state=seed(flags=3,count=1);put(state['mmio_commands'],0x10,0)
 measure('stop_fifo_becomes_ready','stop',state,[0],times=[100],actions=[dict(read=1,address=A['mmio_commands']+0x10,hex='0c000000')])
 state=seed(flags=0x8001)
 measure('stop_wait_forced_retirement','stop',state,[1],times=[0x4c4b40],actions=[dict(callback='force_pending_voice',address=A['voice']+0x12,hex='0100')])
 state=seed(flags=1);notification=A['voice']+0x88;head=A['device']+0x4a8
 put(state['voice'],0x90,2);put(state['voice'],0x88,head);put(state['voice'],0x8c,head);put(state['device'],0x4a8,notification);put(state['device'],0x4ac,notification)
 measure('stop_cancels_pending_timer','stop',state,[0])
 listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited));(args.output/'executed-original.asm').write_text(listing)
 (args.output/'original.json').write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_VOICE_STOP_V1',boundary=__doc__,bindings=B,aliases=A,cases=rows),indent=2)+'\n')
 report=dict(boundary=__doc__,tool_sha256=sha(__file__),runner_sha256=sha(ROOT/'tools/original_audio_cpu.py'),image_sha256=sha(path),pairing_report_sha256=sha(args.pairing/'report.json'),cases=len(rows),genuine=0,ordered_stores=sum(len(r['stores'])for r in rows),device_reads=sum(len(r['reads'])for r in rows),device_words=sum(len(r['writes'])for r in rows),platform_calls=sum(len(r['calls'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[]);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
