#!/usr/bin/env python3
"""Execute unchanged original whole buffered/streamed attachment CPU owners.

Three genuine Xbox-in-xemu calls must agree first. Source helpers, pool lists,
reference counting, buffer assignment and cursor packing execute unchanged.
Kernel guards/pinning/physical/FIFO/MMIO, allocation and stop/drain boundaries
are explicit captured or declared ABI inputs; host DSP/output is not executed.
No expected value is supplied by native code or a native formula.
"""
import argparse,atexit,ctypes,hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code
from oracle_sound_sdk_commit import FragmentCpu
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
u32=lambda b,i=0:struct.unpack_from('<I',b,i)[0]
put=lambda b,i,v:struct.pack_into('<I',b,i,v&0xffffffff)
A=dict(voice=0x1001000,data=0x1002000,device=0x1003000,format_data=0x1003800,replacement_data=0x1003a00,node_pool=0x1004004,available_count=0x386af8,stream_samples_pointer=0x387330,sample=0x1200000,stream_sample=0x1800000)
M=A['device']+0x60
SPEC=dict(buffer=(0x382b7e,0),stream=(0x382be3,0),cursor=(0x3824ee,1),detach=(0x382560,0),sample_bytes=(0x37f43e,1),prepare_buffer=(0x382cd4,0),prepare_stream=(0x382cd4,0))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--pairing',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 path=ROOT/'assets/disc/default.xbe';assert sha(path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';image=Xbe(path);codes=[_Code(image,name)for name in ('DSOUND','.text')]
 class Code:
  def at(self,address):return next((c.at(address)for c in codes if c.inside(address)),None)
 code=Code()
 captures=[json.loads(p.read_text())for p in sorted(args.capture.glob('*/report.json'))];assert len(captures)==3 and {r['path']for r in captures}=={'allocated','cached','stream'}
 for r in captures:assert r['original_whole_attachment_returned_normally']and not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
 layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n');subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
 host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
 locks={};device={};visited=set();rows=[]
 for listing in (args.pairing/'instructions').glob('*-base.asm'):
  for line in listing.read_text().splitlines():
   pc,raw,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(raw)==host.read(pc,i.size)
   if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in (0x379c90,0x379cb2):locks[pc]=i.operands[0].imm
   if i.mnemonic=='mov'and i.operands[0].type==3 and i.operands[0].mem.disp&0xff000000==0xfe000000:device[pc]=i.operands[0].mem.disp&0xffffffff
 kernels={0x380431:('lock_pages',3),0x380443:('physical_address',1),0x380494:('lock_pages',3)};stack,stop=0x100f000,0x3000000
 def normalize(r,field):
  regions={x['name']:bytearray.fromhex(x[field])for x in r['regions']};links=[(int(x['address'],16),x['size'],A[x['name']])for x in r['regions']];base=int(r['page_base'],16);stream_base=u32(bytes.fromhex(next(x for x in r['regions']if x['name']=='stream_samples_pointer')['before_hex']))
  def alias(v):
   for address,size,target in links:
    if address<=v<address+size:return target+v-address
   if r['kind']=='buffer'and base<=v<base+(r['pages']<<12):return A['sample']+v-base
   if stream_base<=v<stream_base+0x20000:return A['stream_sample']+v-stream_base
   return v
  voice=regions['voice']
  for at in (8,0x70,0x80,0x84):put(voice,at,alias(u32(voice,at)))
  for name in ('data','format_data'):
   if name in regions:put(regions[name],0xbc,alias(u32(regions[name],0xbc)))
  manager=memoryview(regions['device'])[0x60:0x80]
  for at in (4,8,0xc,0x10,0x14,0x18,0x1c):put(manager,at,alias(u32(manager,at)))
  pool=regions['node_pool']
  for node in range(0,len(pool),20):
   for at in (0,4,0x10):put(pool,node+at,alias(u32(pool,node+at)))
  put(regions['stream_samples_pointer'],0,alias(u32(regions['stream_samples_pointer'])));return regions,alias
 def measure(label,kind,state,arguments=(),physical=(),allocation=None,stop_status=0,actual=None,context=(0x23f,0x1fa0),actions=()):
  host.control_word,host.mxcsr=context;entry,argc=SPEC[kind];assert len(arguments)==argc
  for name,raw in state.items():
   assert A[name]+len(raw)<=0x100e800 or A[name]<0x1000000
   if A[name]<0x1000000:assert os.pwrite(host.memory_fd,raw,A[name])==len(raw)
   else:host.write(A[name],raw)
  host.write(stack-0x800,bytes([0x5b])*0x804);host.write(stack,struct.pack('<'+'I'*(argc+1),stop,*arguments));host.continuing=False
  action_sites={int(a['pc'],16)+code.at(int(a['pc'],16)).size for a in actions if 'pc'in a};done=set();stores=[];writes=[];guards=[];calls=[];inputs=dict(rcx=A['voice']);execute=True;physical_index=0;lock_count=0
  boundaries={stop,*locks,*device,*kernels,0x37d952,0x37d9ba,0x382dd2,0x381e0e,0x380453,*action_sites}
  def apply(**event):
   for index,a in enumerate(actions):
    if index not in done and all(a.get(k)==v for k,v in event.items()):host.write(a['address'],bytes.fromhex(a['hex']));done.add(index)
  for _ in range(4096):
   result,changed,executed=host.run_traced(entry,stack,code,0,0x3000000,stop_addresses=tuple(pc for pc in boundaries if not execute or pc!=entry),**inputs);visited.update(executed);inputs={};execute=False
   for v in changed:
    if any(A[n]<=v[1]and v[1]+v[2]<=A[n]+len(raw)for n,raw in state.items()):stores.append(v)
    else:assert stack-0x1000<=v[1]and v[1]+v[2]<=stack+0x100,(label,'Unretained original store',v)
   pc=result.rip
   if pc==stop:break
   host.continuing=True
   if pc in action_sites:
    for a in actions:
     if 'pc'in a and int(a['pc'],16)+code.at(int(a['pc'],16)).size==pc:apply(pc=a['pc'])
    entry=pc;execute=True;continue
   if pc in locks:
    guards.append(f'{locks[pc]:08x}')
    if locks[pc]==0x379c90:lock_count+=1;apply(guard=lock_count)
    entry=pc+code.at(pc).size;continue
   if pc==0x380453:entry=0x380460;continue
   if pc in kernels:
    name,count=kernels[pc];values=list(struct.unpack('<'+'I'*count,host.read(result.rsp,count*4)));value=None
    if name=='physical_address':assert physical_index<len(physical),(label,'Undeclared physical address');value=physical[physical_index];physical_index+=1;result.rax=value
    calls.append(dict(name=name,arguments=[f'{v:08x}'for v in values],result=value));result.rsp+=count*4;host.trace(13,0,ctypes.addressof(result));entry=pc+code.at(pc).size;continue
   if pc in (0x37d952,0x37d9ba,0x382dd2,0x381e0e):
    assert not actual,(label,'Genuine unexpected unobserved callee')
    name,count={0x37d952:('allocate',3),0x37d9ba:('release_allocation',1),0x382dd2:('stop',1),0x381e0e:('wait_pending',0)}[pc]
    values=list(struct.unpack('<'+'I'*count,host.read(result.rsp+4,count*4)))if count else[]
    if pc in (0x382dd2,0x381e0e):values.insert(0,result.rcx&0xffffffff)
    value=None
    if name=='allocate':assert allocation is not None;value=allocation;result.rax=value
    if name=='stop':value=stop_status;result.rax=value
    calls.append(dict(name=name,arguments=[f'{v:08x}'for v in values],result=value));apply(callback=name)
    result.rip=u32(host.read(result.rsp,4));result.rsp+=4+count*4;host.trace(13,0,ctypes.addressof(result));entry=result.rip;execute=True;continue
   assert pc in device
   instruction=code.at(pc);reg=instruction.reg_name(instruction.operands[1].reg);value=getattr(result,{'eax':'rax','ecx':'rcx','edx':'rdx','ebx':'rbx','edi':'rdi','esi':'rsi'}[reg])&0xffffffff
   writes.append(dict(address=f'{device[pc]:08x}',value=f'{value:08x}'));entry=pc+instruction.size
  else:raise RuntimeError('Original attachment boundary limit')
  assert result.rsp==stack+4+argc*4 and len(done)==len(actions)and physical_index==len(physical),(label,hex(result.rsp),done,len(actions),physical_index,len(physical))
  after={name:host.read(A[name],len(raw)).hex()for name,raw in state.items()};returned=result.rax&0xffffffff
  if actual:
   expected,alias=normalize(actual,'after_hex');assert after=={n:b.hex()for n,b in expected.items()},(label,'Genuine retained output',[(n,next((i for i,(a,b)in enumerate(zip(bytes.fromhex(after[n]),raw))if a!=b),None))for n,raw in expected.items()if after[n]!=raw.hex()])
   if kind=='buffer':assert returned==0==int(actual['actual_return_eax'],16)
   assert writes==[{k:w[k]for k in ('address','value')}for w in actual['observed_device_writes']]
   expected_calls=[dict(name=c['name'],arguments=[f'{alias(int(v,16)):08x}'for v in c['arguments']],result=int(c['actual_return_eax'],16)if c['name']=='physical_address'else None)for c in actual['kernel_calls']];assert calls==expected_calls
  rows.append(dict(label=label,kind=kind,genuine=bool(actual),arguments=[f'{v:08x}'for v in arguments],physical=list(physical),allocation=allocation,stop_status=stop_status,context=list(context),actions=list(actions),calls=calls,guards=guards,writes=writes,result=f'{returned:08x}',regions=[dict(name=n,address=f'{A[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],stores=[dict(pc=f'{pc:08x}',address=f'{addr:08x}',size=size,before=f'{before:0{size*2}x}',after=f'{value:0{size*2}x}')for pc,addr,size,before,value in stores]))
 for r in captures:
  state,alias=normalize(r,'before_hex');physical=[int(c['actual_return_eax'],16)for c in r['kernel_calls']if c['name']=='physical_address'];measure('genuine_'+r['path'],r['kind'],state,physical=physical,actual=r,context=(r['registers']['fctrl'],r['registers']['mxcsr']))
 def seed(counts=(8,),references=(0,),flags=0,voice_flags=1):
  total=sum(counts);state=dict(voice=bytearray(0x158),data=bytearray(0xe0),device=bytearray(0x540),available_count=bytearray(4),node_pool=bytearray([0x71])*((total+1)*20),stream_samples_pointer=bytearray(4))
  v=state['voice'];put(v,8,A['device']);put(v,0x70,A['data']);put(v,0x80,A['data']);put(v,0x84,A['node_pool']);struct.pack_into('<H',v,0x12,voice_flags)
  d=state['data'];put(d,8,flags);struct.pack_into('<H',d,0xc,1);put(d,0x14,4);put(d,0xbc,A['sample']+0x123);put(d,0xc0,64);put(d,0xc4,0);put(d,0xd4,0xd);put(state['stream_samples_pointer'],0,A['stream_sample'])
  m=memoryview(state['device'])[0x60:0x80];pool=state['node_pool'];base=A['node_pool'];put(m,0,0x417240);put(m,0x14,base);put(m,0x1c,A['available_count']);index=0
  for head,indices in ((M+4,list(range(len(counts)))),(M+0xc,list(range(len(counts),total+1)))):
   nodes=[base+i*20 for i in indices];put(m,head-M,nodes[0]if nodes else head);put(m,head-M+4,nodes[-1]if nodes else head)
   for j,i in enumerate(indices):put(pool,i*20,nodes[j+1]if j+1<len(nodes)else head);put(pool,i*20+4,nodes[j-1]if j else head)
  available=0;largest=None
  for i,(count,refs)in enumerate(zip(counts,references)):
   struct.pack_into('<HH',pool,i*20+8,index,count);put(pool,i*20+0xc,refs);put(pool,i*20+0x10,A['sample']+index*0x1000)
   if not refs:
    available+=count
    if largest is None or count>counts[largest]:largest=i
   index+=count
  put(m,0x18,base+largest*20 if largest is not None else 0);put(state['available_count'],0,available);return state
 rng=random.Random(0x382b7e)
 for tag in (1,0x69,0xfffe):
  for stride in (0,1,4,0x10001,0xffffffff):
   for samples in (0,1,32,63,64,65,0xffffffff):
    state=seed();state['format_data']=bytearray(0xe0);put(state['voice'],0x70,A['format_data']);struct.pack_into('<H',state['format_data'],0xc,tag);put(state['format_data'],0x14,stride)
    measure(f'sample_tag{tag:04x}_stride{stride:08x}_samples{samples:08x}','sample_bytes',state,[samples])
 for flags in (0,0x80000,0x100000,0x180000):
  for cursor in (0,1,0xfff,0x1000,0xfffffff0,0xffffffff):
   for index in (0,7,0xffff):
    state=seed(flags=flags);struct.pack_into('<H',state['node_pool'],8,index);measure(f'cursor_flags{flags:08x}_index{index}_offset{cursor:08x}','cursor',state,[cursor])
 for offset in (0,1,0xfff):
  for size in (0,1,64,0x1000,0x1001,0xffffffff):
   state=seed();put(state['data'],0xbc,A['sample']+offset);put(state['data'],0xc0,size);pages=(((offset+size+0xfff)&0xffffffff)>>12)
   measure(f'buffer_offset{offset}_bytes{size:08x}','buffer',state,physical=[rng.getrandbits(32)for _ in range(pages)]if pages<=8 else[])
 for kind in ('buffer','stream'):
  for flags in (0x100,0x101,0xffff):measure(f'{kind}_already_{flags:04x}',kind,seed(voice_flags=flags))
 for refs in (1,2,0xffffffff):measure(f'buffer_cached_refs{refs:08x}','buffer',seed(references=(refs,)))
 for status in (0,1,0x80004005,0xffffffff):
  state=seed(references=(1,));put(state['data'],0xbc,A['sample']+0x10000);measure(f'buffer_failure_stop{status:08x}','buffer',state,stop_status=status)
  for flags in (1,0x101):measure(f'detach_flags{flags:04x}_stop{status:08x}','detach',seed(references=(1,),voice_flags=flags),stop_status=status)
 for tag in (1,0x69):
  for flags in (0x100000,0x80100000):
   for stride in (0,4,0xffffffff):
    for index in (0xa,0xb,0xd,0xffffffff):
     state=seed(flags=flags);struct.pack_into('<H',state['data'],0xc,tag);put(state['data'],0x14,stride);put(state['data'],0xd4,index)
     measure(f'stream_tag{tag:04x}_flags{flags:08x}_stride{stride:08x}_index{index:08x}','stream',state)
 for allocation in (0,A['sample']):
  state=seed(flags=0x100000);put(state['data'],0xbc,0);put(state['data'],0xd4,0xb);put(state['stream_samples_pointer'],0,0);measure(f'stream_allocate_{allocation:08x}','stream',state,allocation=allocation)
 for kind,flags in (('prepare_buffer',0x80000000),('prepare_stream',0x100000)):
  state=seed(flags=flags);put(state['data'],0xbc,0 if kind=='prepare_stream'else A['sample']+0x123);measure(kind,kind,state,physical=[0x3ac4000]if kind=='prepare_buffer'else[])
 # Declared observer/guard/provider mutations verify actual source rereads.
 for kind,pc in (('buffer',0x382bb5),('stream',0x37c8f0)):
  state=seed(flags=0x100000 if kind=='stream'else 0);state['replacement_data']=bytearray(state['data']);put(state['replacement_data'],0xc4,0x321);put(state['replacement_data'],8,0)
  measure(f'{kind}_reload_data_after_store',kind,state,physical=[0x12345000]if kind=='buffer'else[],actions=[dict(pc=f'{pc:08x}',address=A['voice']+0x80,hex=struct.pack('<I',A['replacement_data']).hex())])
 state=seed();state['replacement_data']=bytearray(state['data']);put(state['replacement_data'],8,0x100000);put(state['replacement_data'],0xbc,0x12345678)
 measure('cursor_reload_after_guard','cursor',state,[0x999],actions=[dict(guard=1,address=A['voice']+0x80,hex=struct.pack('<I',A['replacement_data']).hex())])
 state=seed(references=(1,),voice_flags=0x101);measure('detach_reload_after_stop','detach',state,actions=[dict(callback='stop',address=A['voice']+0x12,hex='0100')])
 state=seed(references=(1,),voice_flags=0x101);measure('detach_reload_after_drain','detach',state,actions=[dict(callback='wait_pending',address=A['voice']+0x84,hex='00000000')])
 state=seed(flags=0x100000);measure('stream_reload_size_after_release','stream',state,actions=[dict(callback='release_allocation',address=A['data']+0xc0,hex='78563412')])
 state=seed(references=(1,));put(state['data'],0xbc,A['sample']+0x10000)
 measure('buffer_failure_branch_keeps_returned_entry','buffer',state,actions=[dict(pc='00382bb5',address=A['voice']+0x84,hex=struct.pack('<I',A['node_pool']).hex())])
 state=seed(flags=0x100000)
 measure('buffer_success_branch_keeps_returned_entry','buffer',state,physical=[0x12345000],actions=[dict(pc='00382bb5',address=A['voice']+0x84,hex='00000000')])
 state=seed(references=(1,),voice_flags=0x101)
 measure('detach_reload_after_flag_store','detach',state,actions=[dict(pc='003824ea',address=A['voice']+0x84,hex='00000000')])
 state=seed(flags=0x100000);state['replacement_data']=bytearray(state['data']);put(state['replacement_data'],0xc4,0x321);put(state['replacement_data'],0xbc,0x12345678)
 measure('stream_reload_after_free_callback','stream',state,actions=[dict(callback='release_allocation',address=A['voice']+0x80,hex=struct.pack('<I',A['replacement_data']).hex())])
 listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited));(args.output/'executed-original.asm').write_text(listing)
 (args.output/'original.json').write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_VOICE_ATTACHMENT_V1',boundary=__doc__,aliases=A,cases=rows),indent=2)+'\n')
 report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={p.parent.name:sha(p)for p in sorted(args.capture.glob('*/report.json'))},cases=len(rows),genuine=3,ordered_stores=sum(len(r['stores'])for r in rows),device_words=sum(len(r['writes'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[]);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
