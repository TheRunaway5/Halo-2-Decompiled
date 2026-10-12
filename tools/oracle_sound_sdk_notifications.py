#!/usr/bin/env python3
"""Run unchanged original notification queue, timer and pending CPU owners.

Original instructions supply every expected CPU store/result and platform call.
Kernel time/timer/DPC and forced-stop owners are explicit captured or declared
ABI inputs. Captured asynchronous guard/kernel effects remain external inputs,
not writes attributed to the tested owner or native timer/DSP/output proof.
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
A=dict(voice=0x1001000,device=0x1002000,notification=0x1003000,peer0=0x1003100,peer1=0x1003200,peer2=0x1003300,descriptor=0x1004000,mapping=0x1005000,vtable=0x41727c)
SPEC=dict(arm=(0x37f245,1,'device'),cancel=(0x37f068,1,'device'),arm_immediate=(0x382ad9,4,'voice'),voice_arm=(0x382ad9,4,'voice'),voice_cancel=(0x382b08,1,'voice'),pending=(0x3811a6,0,'voice'),drain=(0x381e0e,0,'voice'),mapping=(0x37f407,1,'descriptor'))
SITES={0x3811b2:('query_system_time',1),0x37f13a:('query_system_time',1),0x37eb30:('cancel_timer',1),0x37eb3d:('remove_queue_dpc',1),0x37eb5d:('set_timer',4),0x37f1db:('set_timer',4)}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--capture',type=Path);p.add_argument('--pairing',type=Path,required=True);p.add_argument('--output',type=Path,required=True);args=p.parse_args();args.output.mkdir(parents=True,exist_ok=False)
 image_path=ROOT/'assets/disc/default.xbe';assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';image=Xbe(image_path);code=_Code(image,'DSOUND');captures=[json.loads(p.read_text())for p in sorted(args.capture.glob('*/report.json'))]if args.capture else[]
 for r in captures:assert r['original_whole_call_returned_normally']and not any(r[k]for k in ('instruction_patches','guest_memory_writes','guest_register_writes','supplied_callee_returns'))
 for r in captures:
  if r['kind']=='cancel':r['kind']='voice_cancel'
  for i,region in enumerate(r['regions']):
   if region['name'].startswith('list_node_'):A.setdefault(region['name'],0x1006000+i*0x100)
 layout=args.output/'user-layout.c';layout.write_text('#include <stddef.h>\n#include <sys/user.h>\n_Static_assert(offsetof(struct user,u_debugreg)==848,"Unsupported ptrace ABI");\nint main(void){return 0;}\n');subprocess.run(['cc',str(layout),'-o',str(args.output/'user-layout')],check=True);subprocess.run([str(args.output/'user-layout')],check=True)
 host=FragmentCpu(image,args.output/'original32.elf',0x23f,0x1fa0);atexit.register(host.close);host.trace(6,host.debug_offset+7*8,0);visited=set();rows=[];locks={}
 for entry in (0x3811a6,0x381e0e,0x382b08,0x382ad9,0x37f068,0x37f245,0x37f11a,0x37eb07,0x37eae2,0x37f407,0x379c60):
  for line in (args.pairing/'instructions'/f'{entry:08x}-base.asm').read_text().splitlines():
   pc,raw,*_=line.split();pc=int(pc,16);i=code.at(pc);assert i.bytes==bytes.fromhex(raw)==host.read(pc,i.size)
   if i.mnemonic=='call'and i.operands[0].type==2 and i.operands[0].imm in(0x379c90,0x379cb2):locks[pc]=i.operands[0].imm
 stack,stop=0x100f000,0x3000000
 def normalize(r,name,raw):
  raw=bytearray(raw);links=[(int(x['address'],16),x['size'],A[x['name']])for x in r['regions']]
  def alias(value):
   for address,size,target in links:
    if address<=value<address+size:return target+value-address
   return value
  offsets=[]
  if name=='voice':offsets=[8,*[0x88+i*32+j for i in range(6)for j in(0,4,0xc)]]
  if name=='device':offsets=[0x4a8,0x4ac,0x4b0,0x4b4]
  if name.startswith('list_node_'):offsets=[0,4,0xc]
  for at in offsets:put(raw,at,alias(u32(raw,at)))
  return raw
 def actual_state(r,field):return{x['name']:normalize(r,x['name'],bytes.fromhex(x[field]))for x in r['regions']}
 def actual_effects(r,call):
  return[dict(region=x['region'],before_hex=normalize(r,x['region'],bytes.fromhex(x['before_hex'])).hex(),after_hex=normalize(r,x['region'],bytes.fromhex(x['after_hex'])).hex())for x in call['effects']]
 def measure(label,kind,state,arguments=(),times=(),actions=(),actual=None):
  entry,argc,owner=SPEC[kind];assert argc==len(arguments);host.continuing=False
  for name,raw in state.items():
   assert A[name]+len(raw)<0x100e800 or A[name]<0x1000000
   if A[name]<0x1000000:assert os.pwrite(host.memory_fd,raw,A[name])==len(raw)
   else:host.write(A[name],raw)
  host.write(stack-0x800,bytes([0x5b])*0x804);host.write(stack,struct.pack('<'+'I'*(argc+1),stop,*arguments));inputs=dict(rcx=A[owner]);execute=True;stores=[];calls=[];guards=[];done=set();clock=0;guard_index=0
  sites={int(a['pc'],16)+code.at(int(a['pc'],16)).size for a in actions if'pc'in a};boundaries={stop,*locks,*SITES,0x3810bd,*sites}
  def action(event,value):
   for index,a in enumerate(actions):
    if index not in done and a.get(event)==value:host.write(a['address'],bytes.fromhex(a['hex']));done.add(index)
  def effects(values):
   for e in values:
    before=bytes.fromhex(e['before_hex']);after=bytes.fromhex(e['after_hex']);address=A[e['region']];assert host.read(address,len(before))==before,(label,'External input state differs',e['region'])
    for i,(b,a)in enumerate(zip(before,after)):
     if a!=b:host.write(address+i,bytes([a]))
  for _ in range(4096):
   result,changed,executed=host.run_traced(entry,stack,code,0,0x3000000,stop_addresses=tuple(pc for pc in boundaries if not execute or pc!=entry),**inputs);visited.update(executed);inputs={};execute=False
   for store in changed:
    if any(A[n]<=store[1]and store[1]+store[2]<=A[n]+len(raw)for n,raw in state.items()):stores.append(store)
    else:assert stack-0x1000<=store[1]and store[1]+store[2]<=stack+0x100,(label,'Unretained original store',store)
   pc=result.rip
   if pc==stop:break
   host.continuing=True
   if pc in sites:
    for a in actions:
     if'pc'in a and int(a['pc'],16)+code.at(int(a['pc'],16)).size==pc:action('pc',a['pc'])
    entry=pc;execute=True;continue
   if pc in locks:
    g=dict(owner=f'{locks[pc]:08x}',effects=[])
    if actual:
     captured=actual['guards'][len(guards)];assert captured['owner']==g['owner'];g['effects']=actual_effects(actual,captured);effects(g['effects'])
    guards.append(g);guard_index+=1;action('guard',guard_index);entry=pc+code.at(pc).size;continue
   if pc==0x3810bd:
    assert not actual,(label,'Unobserved original force owner');calls.append(dict(name='force_pending_voice',arguments=[f'{result.rcx&0xffffffff:08x}'],time=None,effects=[]));action('callback','force_pending_voice');result.rip=u32(host.read(result.rsp,4));result.rsp+=4;host.trace(13,0,ctypes.addressof(result));entry=result.rip;execute=True;continue
   name,count=SITES[pc];values=list(struct.unpack('<'+'I'*count,host.read(result.rsp,count*4)));time=None;external=[]
   if name=='query_system_time':
    assert clock<len(times),(label,'Undeclared clock');time=times[clock];clock+=1;host.write(values[0],struct.pack('<Q',time));values=[]
   if actual:
    captured=actual['kernel_calls'][len(calls)];assert captured['name']==name;external=actual_effects(actual,captured);effects(external);result.rax=int(captured['actual_return_eax'],16)
   calls.append(dict(name=name,arguments=[f'{v:08x}'for v in values],time=f'{time:016x}'if time is not None else None,effects=external));action('callback',name);action('clock',clock)if name=='query_system_time'else None
   result.rsp+=4*count;host.trace(13,0,ctypes.addressof(result));entry=pc+code.at(pc).size
  else:raise RuntimeError('Notification original boundary limit')
  assert result.rsp==stack+4+argc*4 and len(done)==len(actions)and clock==len(times),(label,hex(result.rsp),done,len(actions),clock,len(times))
  after={n:host.read(A[n],len(raw)).hex()for n,raw in state.items()}
  if actual:
   expected=actual_state(actual,'after_hex');assert after=={n:raw.hex()for n,raw in expected.items()},(label,'Genuine whole retained output',[(n,next((i for i,(a,b)in enumerate(zip(bytes.fromhex(after[n]),raw))if a!=b),None))for n,raw in expected.items()if after[n]!=raw.hex()])
   assert len(guards)==len(actual['guards'])and len(calls)==len(actual['kernel_calls'])
  rows.append(dict(label=label,kind=kind,genuine=bool(actual),arguments=[f'{v:08x}'for v in arguments],times=[f'{t:016x}'for t in times],actions=list(actions),calls=calls,guards=guards,result=f'{result.rax&0xffffffff:08x}',regions=[dict(name=n,address=f'{A[n]:08x}',before_hex=raw.hex(),after_hex=after[n])for n,raw in state.items()],stores=[dict(pc=f'{p:08x}',address=f'{a:08x}',size=size,before=f'{b:0{size*2}x}',after=f'{v:0{size*2}x}')for p,a,size,b,v in stores]))
 for r in captures:
  times=[int.from_bytes(bytes.fromhex(c['time_hex']),'little')for c in r['kernel_calls']if c['name']=='query_system_time'];measure('genuine_'+r['kind'],r['kind'],actual_state(r,'before_hex'),[int(a,16)for a in r['arguments']],times,actual=r)
 def seed(flags=0,when=0,peers=(),immediate=False):
  state=dict(voice=bytearray(0x158),device=bytearray(0x540),notification=bytearray(32),vtable=bytearray(image.read(0x41727c,40)));put(state['voice'],0,0x41727c);put(state['voice'],8,A['device']);struct.pack_into('<H',state['voice'],0x12,1)
  for i in range(6):
   base=0x88+i*32;put(state['voice'],base,A['voice']+base);put(state['voice'],base+4,A['voice']+base)
  record=state['notification'];put(record,0,A['notification']);put(record,4,A['notification']);put(record,8,flags);put(record,0x18,when);put(record,0x1c,when>>32)
  for offset in(0x4a8,0x4b0):put(state['device'],offset,A['device']+offset);put(state['device'],offset+4,A['device']+offset)
  head=A['device']+(0x4b0 if immediate else 0x4a8);names=[f'peer{i}'for i in range(len(peers))]
  if flags&2:names.insert(1 if peers else 0,'notification')
  nodes=[A[n]for n in names]
  for i,name in enumerate(names):
   if name!='notification':
    raw=state.setdefault(name,bytearray(32));time=peers[int(name[-1])];put(raw,0x18,time);put(raw,0x1c,time>>32);put(raw,8,3 if immediate else 2)
   raw=state[name];put(raw,0,nodes[i+1]if i+1<len(nodes)else head);put(raw,4,nodes[i-1]if i else head)
  put(state['device'],head-A['device'],nodes[0]if nodes else head);put(state['device'],head-A['device']+4,nodes[-1]if nodes else head);return state
 mask=(1<<64)-1
 for flags in(0,1,2,3,0x80000000,0xffffffff):
  for when in(0,1,99,100,101,0xffffffff,0x100000000,0x7fffffffffffffff,0x8000000000000000,mask,mask-99):
   for now in(100,0xffffffffffffff00):
    state=seed(flags,when,immediate=bool(flags&1));measure(f'arm_flags{flags:08x}_when{when:016x}_now{now:016x}','arm',state,[A['notification']],[now]if not flags&1 and when else[])
 for peers in((),(90,),(100,100),(50,100,150),(150,100,50)):
  for when in(80,100,120,200):
   state=seed(0,when,peers);measure(f'sorted_peers{peers}_when{when}','arm',state,[A['notification']],[60])
 for flags in(0,1,2,3,0xffffffff):
  for peers in((),(100,),(50,100,150)):
   measure(f'cancel_flags{flags:08x}_peers{peers}','cancel',seed(flags,120,peers,bool(flags&1)),[A['notification']])
 # Head cancellation and re-arming exercise timer refresh, cancellation and DPC order.
 for peers in((),(100,200)):
  state=seed(2,80,peers)
  if peers:
   head=A['device']+0x4a8;put(state['device'],0x4a8,A['notification']);put(state['notification'],4,head);put(state['notification'],0,A['peer0']);put(state['peer0'],4,A['notification']);put(state['peer0'],0,A['peer1']);put(state['peer1'],4,A['peer0']);put(state['peer1'],0,head);put(state['device'],0x4ac,A['peer1'])
  measure(f'cancel_head_peers{peers}','cancel',state,[A['notification']])
 for flags in(0,1,0x8000,0x8001,0xffff):
  for previous in(0,0xffffffff,0x7fffffffffffffff,0xfffffffffffffff0):
   limit=(previous+0x4c4b40)&mask
   for delta in(-1,0,1):
    state=seed();struct.pack_into('<H',state['voice'],0x12,flags);struct.pack_into('<Q',state['voice'],0x78,previous)
    measure(f'pending_flags{flags:04x}_previous{previous:016x}_delta{delta}','pending',state,times=[(limit+delta)&mask])
 for flags in(0,1,0x8000):
  state=seed();struct.pack_into('<H',state['voice'],0x12,flags);measure(f'drain_skip_flags{flags:04x}','drain',state)
 state=seed();struct.pack_into('<H',state['voice'],0x12,0x8001);measure('drain_repeated_clock','drain',state,times=[1,2],actions=[dict(clock=2,address=A['voice']+0x12,hex='0100')])
 state=seed();struct.pack_into('<H',state['voice'],0x12,0x8001);measure('drain_force_clears_pending','drain',state,times=[0x4c4b40],actions=[dict(callback='force_pending_voice',address=A['voice']+0x12,hex='0100')])
 for index in(0,1,5,31):
  for flag in(0,1,2,0x7f,0x80,0xff):
   state=seed();state['descriptor']=bytearray(struct.pack('<I',A['mapping']));state['mapping']=bytearray([0xa5])*0x200;state['mapping'][index*16+15]=flag;measure(f'mapping_index{index}_flag{flag:02x}','mapping',state,[index])
 for index in(0,1,5):
  state=seed();put(state['voice'],0x90+index*32,1);measure(f'voice_arm_slot{index}','voice_arm',state,[index,0x1234,0x5678,0xaabbccdd]);measure(f'voice_cancel_slot{index}','voice_cancel',state,[index])
 state=seed(0,200,(100,));measure('reload_after_clock','arm',state,[A['notification']],[60],actions=[dict(callback='query_system_time',address=A['notification']+0x18,hex='2c010000')])
 state=seed(2,200);measure('reload_flags_after_timer_cancel','cancel',state,[A['notification']],actions=[dict(callback='cancel_timer',address=A['notification']+8,hex='81aabbcc')])
 # The external input changes the retained tail after the first insertion store.
 # Keep that peer backed so the original's subsequent store is observable.
 state=seed(1);state['peer0']=bytearray(32)
 measure('reload_previous_after_insert_store','arm',state,[A['notification']],actions=[dict(pc='0037eaf2',address=A['device']+0x4b4,hex=struct.pack('<I',A['peer0']).hex())])
 state=seed(1);state['peer0']=bytearray(32)
 measure('reload_next_after_insert_store','arm',state,[A['notification']],actions=[dict(pc='0037eafa',address=A['notification'],hex=struct.pack('<I',A['peer0']).hex())])
 state=seed(3,0,(100,),True);state['peer2']=bytearray(32)
 measure('reload_previous_after_detach_store','cancel',state,[A['notification']],actions=[dict(pc='00379c6d',address=A['notification']+4,hex=struct.pack('<I',A['peer2']).hex())])
 state=seed(2,200,(100,250))
 measure('reload_time_after_nested_cancel','arm',state,[A['notification']],[60],actions=[dict(guard=3,address=A['notification']+0x18,hex=struct.pack('<I',300).hex())])
 state=seed(0,200)
 measure('reload_flags_after_timer_arm','arm',state,[A['notification']],[60],actions=[dict(callback='set_timer',address=A['notification']+8,hex='80aabbcc')])
 state=seed();struct.pack_into('<H',state['voice'],0x12,0x8001)
 measure('reload_pending_flags_after_clock','pending',state,times=[0x4c4b40],actions=[dict(callback='query_system_time',address=A['voice']+0x12,hex='0100')])
 state=seed();struct.pack_into('<H',state['voice'],0x12,0x8001)
 measure('pending_signed_wrap_negative','pending',state,times=[0x8000000000000000])
 state=seed();struct.pack_into('<H',state['voice'],0x12,0x8001);struct.pack_into('<Q',state['voice'],0x78,0x7fffffffffffffff)
 measure('pending_signed_wrap_positive','pending',state,times=[1])
 listing=''.join(f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'for pc in sorted(visited));(args.output/'executed-original.asm').write_text(listing)
 (args.output/'original.json').write_text(json.dumps(dict(schema='ORIGINAL_SOUND_SDK_NOTIFICATIONS_V1',boundary=__doc__,aliases=A,bindings=dict(voice_vtable='0041727c',cancel_method='00382b08',arm_method='00382ad9'),cases=rows),indent=2)+'\n')
 report=dict(boundary=__doc__,tool_sha256=sha(__file__),image_sha256=sha(image_path),pairing_report_sha256=sha(args.pairing/'report.json'),capture_pins={p.parent.name:sha(p)for p in sorted(args.capture.glob('*/report.json'))}if args.capture else{},cases=len(rows),genuine=len(captures),ordered_stores=sum(len(r['stores'])for r in rows),platform_calls=sum(len(r['calls'])for r in rows),executed_instructions=len(visited),fixture_sha256=sha(args.output/'original.json'),executed_listing_sha256=sha(args.output/'executed-original.asm'),original_host_kernel_execution=False,original_host_DSP_execution=False,instruction_patches=[]);(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
