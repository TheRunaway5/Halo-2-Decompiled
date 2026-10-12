#!/usr/bin/env python3
"""Execute unchanged original sample-page CPU owners against genuine calls.

Pool/search/split/merge/reference instructions execute unchanged. Both whole
Xbox-in-xemu acquisition captures must agree first, including actual device
words. Host kernel lock/pin/physical-address/FIFO/MMIO and allocation boundaries
are explicit captured or declared ABI inputs, not original host kernel/DSP
execution. No expected value comes from native code or a native formula.
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
A=dict(manager=0x1001000,available_count=0x386af8,node_pool=0x1003004,sample=0x1200000,secondary_count=0x1002000)
SPEC=dict(construct=(0x38025a,0),initialize=(0x38029c,1),acquire=(0x3806be,2),map=(0x380419,3),release=(0x38085d,1))
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path,required=True);p.add_argument('--pairing',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 path=ROOT/'assets/disc/default.xbe';assert sha(path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';image=Xbe(path);code=_Code(image,'DSOUND')
 captures=[json.loads(p.read_text())for p in sorted(args.capture.glob('*/report.json'))];assert len(captures)==2 and {r['path']for r in captures}=={'allocated','cached'}
 for r in captures:assert r['original_whole_acquisition_returned_normally']and not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
 layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n');subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
 host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0)
 locks={};device={};visited=set();rows=[]
 for entry in (0x38025a,0x38029c,0x3806be,0x380327,0x379c60,0x3801fd,0x380419,0x38085d,0x38074d,0x3803f8,0x37e612,0x380486):
  for line in (args.pairing/'instructions'/f'{entry:08x}-base.asm').read_text().splitlines():
   pc,raw,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(raw)==host.read(pc,i.size)
   if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in (0x379c90,0x379cb2):locks[pc]=i.operands[0].imm
   if i.mnemonic=='mov'and i.operands[0].type==3 and i.operands[0].mem.disp&0xff000000==0xfe000000:device[pc]=i.operands[0].mem.disp&0xffffffff
 assert image.read(0x4116d8,8).hex()=='ad000080af000080'
 kernels={0x380431:('lock_pages',3),0x380443:('physical_address',1),0x380494:('lock_pages',3)};stack,stop=0x100f000,0x3000000
 def normalize(r,field):
  regions={x['name']:bytearray.fromhex(x[field])for x in r['regions']};links=[(int(x['address'],16),x['size'],A[x['name']])for x in r['regions']];base=int(r['page_base'],16)
  def alias(v):
   if base<=v<base+(r['pages']<<12):return A['sample']+v-base
   for address,size,target in links:
    if address<=v<address+size:return target+v-address
   return v
  manager=regions['manager']
  for at in (4,8,0xc,0x10,0x14,0x18,0x1c):put(manager,at,alias(u32(manager,at)))
  pool=regions['node_pool']
  for node in range(0,len(pool),20):
   for at in (0,4,0x10):put(pool,node+at,alias(u32(pool,node+at)))
  return regions,alias
 def measure(label,kind,state,arguments,physical=(),allocation=None,actual=None,context=(0x23f,0x1fa0),actions=()):
  host.control_word,host.mxcsr=context;entry,argc=SPEC[kind];assert len(arguments)==argc
  for name,raw in state.items():
   if A[name]<0x1000000:assert os.pwrite(host.memory_fd,raw,A[name])==len(raw)
   else:host.write(A[name],raw)
  host.write(stack-0x800,bytes([0x5b])*0x804);host.write(stack,struct.pack('<'+'I'*(argc+1),stop,*arguments));host.continuing=False
  action_sites={int(a['pc'],16)+code.at(int(a['pc'],16)).size for a in actions if 'after_unlock' not in a};done=set();stores=[];writes=[];guards=[];calls=[];inputs=dict(rcx=A['manager']);execute=True;physical_index=0
  boundaries={stop,*locks,*device,*kernels,0x37d952,0x380453,*action_sites}
  for _ in range(4096):
   result,changed,executed=host.run_traced(entry,stack,code,0,0x3000000,stop_addresses=tuple(pc for pc in boundaries if not execute or pc!=entry),**inputs);visited.update(executed);inputs={};execute=False
   for v in changed:
    if any(A[n]<=v[1]and v[1]+v[2]<=A[n]+len(raw)for n,raw in state.items()):stores.append(v)
    else:assert stack-0x1000<=v[1]and v[1]+v[2]<=stack+0x100,(label,'Unretained original store',v)
   pc=result.rip
   if pc==stop:break
   host.continuing=True
   if pc in action_sites:
    for index,a in enumerate(actions):
     if index not in done and 'after_unlock' not in a and int(a['pc'],16)+code.at(int(a['pc'],16)).size==pc:host.write(a['address'],bytes.fromhex(a['hex']));done.add(index)
    entry=pc;execute=True;continue
   if pc in locks:
    guards.append(f'{locks[pc]:08x}')
    for index,a in enumerate(actions):
     if index not in done and a.get('after_unlock')==1 and pc==0x380721:host.write(a['address'],bytes.fromhex(a['hex']));done.add(index)
    entry=pc+code.at(pc).size;continue
   if pc==0x380453:entry=0x380460;continue
   if pc in kernels:
    name,count=kernels[pc];arguments32=list(struct.unpack('<'+'I'*count,host.read(result.rsp,count*4)));value=None
    if name=='physical_address':assert physical_index<len(physical);value=physical[physical_index];physical_index+=1;result.rax=value
    calls.append(dict(name=name,arguments=[f'{v:08x}'for v in arguments32],result=value));result.rsp+=count*4;host.trace(13,0,ctypes.addressof(result));entry=pc+code.at(pc).size;continue
   if pc==0x37d952:
    assert allocation is not None;arguments32=list(struct.unpack('<3I',host.read(result.rsp+4,12)));calls.append(dict(name='allocate',arguments=[f'{v:08x}'for v in arguments32],result=allocation));result.rax=allocation;result.rip=u32(host.read(result.rsp,4));result.rsp+=16;host.trace(13,0,ctypes.addressof(result));entry=result.rip;execute=True;continue
   assert pc in device
   instruction=code.at(pc);name={'edi':'rdi','esi':'rsi'}[instruction.reg_name(instruction.operands[1].reg)];value=getattr(result,name)&0xffffffff
   writes.append(dict(address=f'{device[pc]:08x}',value=f'{value:08x}'));entry=pc+instruction.size
  else:raise RuntimeError('Original sample-page boundary limit')
  assert result.rsp==stack+4+argc*4 and len(done)==len(actions) and physical_index==len(physical),label
  after={name:host.read(A[name],len(raw)).hex()for name,raw in state.items()};returned=result.rax&0xffffffff
  if actual:
   expected,alias=normalize(actual,'after_hex');assert after=={n:b.hex()for n,b in expected.items()},(label,'genuine retained output')
   assert returned==alias(int(actual['actual_return_eax'],16))and writes==[{k:w[k]for k in ('address','value')}for w in actual['observed_device_writes']],(label,hex(returned),hex(alias(int(actual['actual_return_eax'],16))),writes)
   expected_calls=[dict(name=c['name'],arguments=[f'{alias(int(v,16)):08x}'for v in c['arguments']],result=int(c['actual_return_eax'],16)if c['name']=='physical_address'else None)for c in actual['kernel_calls']];assert calls==expected_calls
  rows.append(dict(label=label,kind=kind,genuine=bool(actual),arguments=[f'{v:08x}'for v in arguments],physical=list(physical),allocation=allocation,context=list(context),actions=list(actions),calls=calls,guards=guards,writes=writes,result=f'{returned:08x}',regions=[dict(name=n,address=f'{A[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],stores=[dict(pc=f'{pc:08x}',address=f'{addr:08x}',size=size,before=f'{before:0{size*2}x}',after=f'{value:0{size*2}x}')for pc,addr,size,before,value in stores]))
 for r in captures:
  state,alias=normalize(r,'before_hex');physical=[int(c['actual_return_eax'],16)for c in r['kernel_calls']if c['name']=='physical_address'];measure('genuine_'+r['path'],'acquire',state,[alias(int(r['buffer'],16)),r['bytes']],physical,actual=r,context=(r['registers']['fctrl'],r['registers']['mxcsr']))
 def seed(counts=(8,),references=(0,)):
  total=sum(counts);assert total>=len(counts);state=dict(manager=bytearray([0x5b])*32,available_count=bytearray(4),node_pool=bytearray([0x71])*((total+1)*20));m=state['manager'];pool=state['node_pool'];base=A['node_pool'];put(m,0,0x417240);put(m,0x14,base);put(m,0x1c,A['available_count']);index=0
  for head,indices in ((A['manager']+4,list(range(len(counts)))),(A['manager']+0xc,list(range(len(counts),total+1)))):
   nodes=[base+i*20 for i in indices];put(m,head-A['manager'],nodes[0]if nodes else head);put(m,head-A['manager']+4,nodes[-1]if nodes else head)
   for j,i in enumerate(indices):put(pool,i*20,nodes[j+1]if j+1<len(nodes)else head);put(pool,i*20+4,nodes[j-1]if j else head)
  available=0;largest=None
  for i,(count,refs)in enumerate(zip(counts,references)):
   struct.pack_into('<HH',pool,i*20+8,index,count);put(pool,i*20+0xc,refs);put(pool,i*20+0x10,A['sample']+index*0x1000)
   if not refs:
    available+=count
    if largest is None or count>counts[largest]:largest=i
   index+=count
  put(m,0x18,base+largest*20 if largest is not None else 0);put(state['available_count'],0,available);return state
 rng=random.Random(0x3806be)
 # Each physical word is an explicit raw kernel ABI input in declared cases.
 def physical(count):return [rng.getrandbits(32)for _ in range(count)]
 for fill in (0,0x5b,0xff):
  state=seed();state['manager']=bytearray([fill])*32;measure(f'construct_fill{fill}','construct',state,[])
 for count in (0,1,2,3,7,31,255,256,1023,0x10000,0x10003):
  for ok in (False,True):
   state=seed();low=count&0xffff;state['node_pool']=bytearray((low+1)*20)
   # Initialize requires the constructor's empty rings. Existing seeded links
   # would refer beyond a smaller newly returned allocation, an invalid input.
   for at in (4,8):put(state['manager'],at,A['manager']+4)
   for at in (0xc,0x10):put(state['manager'],at,A['manager']+0xc)
   measure(f'initialize_count{count}_success{int(ok)}','initialize',state,[count],allocation=A['node_pool']if ok else 0)
 for offset in (0,1,0xfff):
  for size in (0,1,64,0xfff,0x1000,0x1001,0x7fff,0xffffffff):
   state=seed();pages=(((offset+size+0xfff)&0xffffffff)>>12);words=physical(pages)if pages<=8 else[]
   measure(f'acquire_offset{offset}_bytes{size:08x}','acquire',state,[A['sample']+offset,size],words)
 for counts in ((1,2,5),(3,2,3),(2,4,2),(1,1,6)):
  for references in ((0,0,0),(1,0,1),(0,1,0),(1,1,1)):
   for requested in (1,2,3,4,8,9):
    state=seed(counts,references);available=any(not ref and count>=requested for count,ref in zip(counts,references));words=physical(requested)if available else[]
    # Avoid matching existing bases; this case selects only from free entries.
    measure(f'bestfit_counts{counts}_refs{references}_pages{requested}','acquire',state,[A['sample']+0x10000,requested*0x1000],words)
 for references in (1,2,0xffffffff,0x80000000):
  for offset in (0,1,0x800):
   measure(f'cached_refs{references:08x}_offset{offset}','acquire',seed((1,7),(references,0)),[A['sample']+offset,64])
 for first in (0,1,7,0xffff,0xffffffff):
  for count in (0,1,2,7):
   measure(f'map_first{first:08x}_pages{count}','map',seed(),[first,A['sample']+0x123,count],physical(count))
 for counts in ((8,),(1,2,5),(3,2,3),(2,1,1,4)):
  for index in range(len(counts)):
   for pattern in (0,1,2,3):
    refs=[(pattern>>(i%2))&1 for i in range(len(counts))];refs[index]=1;measure(f'release_counts{counts}_index{index}_pattern{pattern}','release',seed(counts,refs),[A['node_pool']+index*20])
 for refs in (0,2,0xffffffff):measure(f'release_reference{refs:08x}','release',seed((8,),(refs,)),[A['node_pool']])
 for pc,address,raw in ((0x379c6d,A['node_pool']+0xa,struct.pack('<H',3)),(0x3803be,A['manager']+0x18,struct.pack('<I',0)),(0x3803b8,A['manager']+0x1c,struct.pack('<I',A['secondary_count']))):
  state=seed();state['secondary_count']=bytearray(struct.pack('<I',100));measure(f'reload_{pc:08x}','acquire',state,[A['sample'],64],physical(1),actions=[dict(pc=f'{pc:08x}',address=address,hex=raw.hex())])
 measure('reload_after_outer_unlock','acquire',seed(),[A['sample'],64],physical(1),actions=[dict(after_unlock=1,address=A['node_pool']+8,hex='3412')])
 listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited));(args.output/'executed-original.asm').write_text(listing)
 (args.output/'original.json').write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_SAMPLE_PAGES_V1',boundary=__doc__,aliases=A,cases=rows),indent=2)+'\n')
 report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={p.parent.name:sha(p)for p in sorted(args.capture.glob('*/report.json'))},cases=len(rows),genuine=2,ordered_stores=sum(len(r['stores'])for r in rows),device_words=sum(len(r['writes'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[]);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
