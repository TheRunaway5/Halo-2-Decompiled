#!/usr/bin/env python3
"""Unchanged original queue/worker with declared real native sync/file services.

This executes no kernel/XAPI wrapper status/TLS/worker creation. Owned Python
mutex/semaphore/file objects implement named call-entry adaptations; the worker
root runs synchronously, and stops BEFORE a wait with no permit is entered.
Complete raw original state, ordered stores and provider calls are retained.
"""
import argparse,csv,hashlib,json,struct,sys,threading,os
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,sha,save,encoded,Xbe,_Code,_trace
from research_mp_original_data_u5 import norm
BASE=ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-worker'
BANK,EXTENT=0x4e7924,0x25a0
BUFFER_SIZE=0x50000
CURRENT,DONE,COUNT,BUFFER=0x518000,0xa103000,0xa103010,0x82947000
STACK,STOP=0xa100800,0xa200000
MAP=ROOT/'assets/disc/maps/lockout.map'
MAP_PIN='14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
def pack(v):return struct.pack('<'+'I'*len(v),*(q&0xffffffff for q in v))
def word(b):return int.from_bytes(b,'little')
class End(Exception):pass

def comparison(out):
 base=Xbe(ROOT/'assets/disc/default.xbe');old=Xbe(OLD);codes=[_Code(x,'.text')for x in(base,old)];bodies=[];data=[]
 for entry,image,code in zip((0x1a0e70,0x1a47d0),(base,old),codes):
  f=_trace(code,entry,{entry});ins=[code.at(p)for p in sorted(f.code)];positions={i.address:n for n,i in enumerate(ins)}
  bodies.append([norm(i,image,positions)for i in ins]);name='base'if entry==0x1a0e70 else'tu11';save(out/'instructions'/f'file-callback-{name}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
  data.append(dict(image=name,entry=f'{entry:08x}',rows=len(ins),direct_calls=[dict(site=f'{i.address:08x}',target=f'{i.operands[0].imm:08x}')for i in ins if i.mnemonic=='call'],chunk_limit_address='0046e488'if name=='base'else'00475970'))
 assert bodies[0]==bodies[1] and len(bodies[0])==59
 for row,image in zip(data,(base,old)):row['chunk_limit_raw4_hex']=image.read(int(row['chunk_limit_address'],16),4).hex()
 wrappers=[]
 for be,te in ((0x2d2488,0x2d88c8),(0x2d2241,0x2d8681)):
  body=[];records=[]
  for name,entry,image in (('base',be,base),('tu11',te,old)):
   code=_Code(image,'.text');f=_trace(code,entry,{entry});ins=[code.at(p)for p in sorted(f.code)];positions={i.address:n for n,i in enumerate(ins)}
   body.append([norm(i,image,positions)for i in ins]);save(out/'instructions'/f'{be:08x}-{name}.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
   absolute=[]
   for i in ins:
    for op in i.operands:
     if op.type==3 and not op.mem.base and not op.mem.index and any(q.va<=op.mem.disp<q.va+q.vsize for q in image.sections):
      absolute.append(dict(site=f'{i.address:08x}',address=f'{op.mem.disp:08x}',bytes=op.size,raw_hex=image.read(op.mem.disp,op.size).hex()))
   records.append(dict(image=name,entry=f'{entry:08x}',rows=len(ins),absolute_image_operands=absolute,direct_calls=[dict(site=f'{i.address:08x}',target=f'{i.operands[0].imm:08x}')for i in ins if i.mnemonic=='call'and i.operands[0].type==2]))
  assert body[0]==body[1];wrappers.append(dict(full_normalized_instruction_branch_topology_equal=True,images=records))
 assert data[0]['chunk_limit_raw4_hex']==data[1]['chunk_limit_raw4_hex']=='00000400'
 return dict(full_normalized_59_instruction_branch_topology_equal=True,bodies=data,complete_file_wrappers=wrappers,base_source_sha256=sha(ROOT/'assets/disc/default.xbe'),reuse='Only read-task arithmetic/real owned-file service reused. Queue comparator identity remains selected TU callback1A47D0, never base literal1A0E70.')

def execute(name,entry,configure,kind,callback_result=None):
 image=Xbe(OLD);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 for lo,n in((0xa100000,0x10000),(STOP,4096),(BUFFER,BUFFER_SIZE)):g.mem_map(lo,n)
 original=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/animation-async-construction/prefix-controls/report.json').read_text())
 raw=bytes.fromhex(original['cases'][0]['after_hex']);g.mem_write(BANK,raw)
 # Explicit host alias table backed by actual objects; source never claims to
 # have created original Xbox handles. All waits below acquire real ownership.
 mutexes={0xf3000001:threading.RLock(),0xf3000002:threading.RLock()}
 sem=threading.BoundedSemaphore(150)
 for _ in range(150):sem.acquire()
 aliases=(0xf3000001,0xf3000002,0xf3000003)
 for at,v in zip((0x4e7928,0x4e792c,0x4e7930),aliases):g.mem_write(at,pack([v]))
 g.mem_write(CURRENT,pack([0]));g.mem_write(DONE,b'\x7f');g.mem_write(COUNT,pack([0xdeadbeef]));g.mem_write(BUFFER,b'\xa5'*BUFFER_SIZE)
 g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_EFLAGS,2);g.mem_write(STACK,pack([STOP]));stores=[];calls=[];events=[];seen={};stop={};remaining=0;file=None
 def u(at):return word(bytes(g.mem_read(at,4)))
 def w(at,v):g.mem_write(at,pack([v]))
 def node(i,priority,callback,filetoken=0xf4000001,offset=0xbb4800,total=0x7a00,next=0):
  p=0x4e7938+i*64;words=[priority,0xffffffff,filetoken,BUFFER,total,offset,COUNT,0,0x11223344,0x55667788,0x99aabbcc,0xddeeff00,3,callback,DONE,next];g.mem_write(p,pack(words));return p
 def service_return(result,pop):
  sp=g.reg_read(UC_X86_REG_ESP);ret=u(sp);g.reg_write(UC_X86_REG_EAX,result&0xffffffff);g.reg_write(UC_X86_REG_ESP,sp+4+pop);g.reg_write(UC_X86_REG_EIP,ret)
 context=dict(g=g,u=u,w=w,node=node,sem=sem)
 arg=configure(context)
 remaining=1 if entry==0x122360 else 0
 if kind=='file':file=MAP.open('rb')
 elif kind=='file_nonseek':
  rd,wr=os.pipe();os.close(wr);file=os.fdopen(rd,'rb')
 before=dict(bank_hex=bytes(g.mem_read(BANK,EXTENT)).hex(),current_hex=bytes(g.mem_read(CURRENT,4)).hex(),done_hex=bytes(g.mem_read(DONE,1)).hex(),count_hex=bytes(g.mem_read(COUNT,4)).hex(),buffer_hex=bytes(g.mem_read(BUFFER,BUFFER_SIZE)).hex())
 if entry==0x19d8c0:
  g.mem_write(STACK,pack([STOP,0x7a00,BUFFER,COUNT,DONE,7,3]));g.reg_write(UC_X86_REG_ECX,arg)
  w(0x5627b0,3)
  for at,v in [(0x55f798+3*0x804,0xf4000001),(0x5607a0,0xf4000002),(0x55f798,0xf4000003),(0x55ff9c,0xf4000004)]:w(at,v)
 if entry in(0x122020,):g.mem_write(STACK,pack([STOP,arg]))
 if entry==0x1220e0:g.reg_write(UC_X86_REG_EAX,arg)
 if entry==0x122150:g.reg_write(UC_X86_REG_ESI,arg)
 def call(row):
  calls.append(row);events.append(dict(call=row))
 def write(_g,a,at,n,v,_):
  if BANK<=at<BANK+EXTENT or at in(CURRENT,DONE,COUNT)or BUFFER<=at<BUFFER+0x10000:
   stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{at:08x}',bytes=n,before=bytes(g.mem_read(at,n)).hex(),written=(v&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
   events.append(dict(store=stores[-1]))
 def hook(_g,pc,n,_):
  nonlocal remaining
  assert bytes(g.mem_read(pc,n))==image.read(pc,n);seen[pc]=n;sp=g.reg_read(UC_X86_REG_ESP)
  if pc==0x2dc9c8:
   h,ms=u(sp+4),u(sp+8);assert ms==0xffffffff
   if h==aliases[2]:
    if not sem.acquire(False):stop.update(kind='native wait has no actual permit; source post-wait stores not entered',entry=f'{pc:08x}',handle=f'{h:08x}');g.emu_stop();return
    remaining-=1
   else:mutexes[h].acquire()
   call(['wait',h,ms]);service_return(g.reg_read(UC_X86_REG_EAX),8)
  elif pc==0x2dc892:
   h=u(sp+4);mutexes[h].release();call(['release',h]);service_return(g.reg_read(UC_X86_REG_EAX),4)
  elif pc==0x2dc814:
   h,c,out=u(sp+4),u(sp+8),u(sp+12);assert h==aliases[2]and c==1
   prev=remaining;sem.release();remaining+=1;w(out,prev);call(['signal',h,c,prev]);service_return(g.reg_read(UC_X86_REG_EAX),12)
  elif pc==0x2d84c4:
   stop.update(kind='empty free-list native yield required; no fabricated available node',entry=f'{pc:08x}');g.emu_stop()
  elif pc==0x1a47d0 and callback_result is not None:
   call(['callback_control',pc,u(sp+4),callback_result]);service_return(callback_result,4)
  elif pc==0x2d88c8:
   assert file is not None;h,offset,high,method=[u(sp+i)for i in(4,8,12,16)];assert h==0xf4000001 and u(high)==0 and method==0
   try:file.seek(offset);result=file.tell()&0xffffffff
   except OSError:result=0xffffffff
   call(['seek',h,offset,0,result]);service_return(result,16)
  elif pc==0x2d8681:
   assert file is not None;h,b,nread,out,overlap=[u(sp+i)for i in(4,8,12,16,20)];assert h==0xf4000001 and overlap==0
   actual=file.read(nread);g.mem_write(b,actual);w(out,len(actual));call(['read',h,b,nread,len(actual),hashlib.sha256(actual).hexdigest()]);service_return(g.reg_read(UC_X86_REG_EAX),20)
  elif pc==0x1a47d0:call(['callback',pc,u(sp+4)])
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
 try:g.emu_start(entry,STOP,count=200000)
 finally:
  if file:file.close()
 after=dict(bank_hex=bytes(g.mem_read(BANK,EXTENT)).hex(),current_hex=bytes(g.mem_read(CURRENT,4)).hex(),done_hex=bytes(g.mem_read(DONE,1)).hex(),count_hex=bytes(g.mem_read(COUNT,4)).hex(),buffer_hex=bytes(g.mem_read(BUFFER,BUFFER_SIZE)).hex())
 assert stop or g.reg_read(UC_X86_REG_EIP)==STOP
 return dict(name=name,entry=f'{entry:08x}',argument=arg,before=before,after=after,stores=stores,services=calls,events=events,stop=stop,returned_eax=g.reg_read(UC_X86_REG_EAX),kind=kind,callback_control=callback_result,reached={f'{p:08x}':image.read(p,n).hex()for p,n in sorted(seen.items())})

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 for path,pin in PINS.items():assert sha(path)==pin
 assert sha(MAP)==MAP_PIN
 cases=[]
 def empty(c):return 0
 cases.append(execute('actual_initialized_free_lease',0x121fd0,empty,'queue'))
 def no_free(c):c['w'](0x4e9eb8,0);return 0
 cases.append(execute('declared_empty_free_boundary',0x121fd0,no_free,'queue'))
 def insertion(c,order,counter=0):
  ps=[c['node'](i,p,callback,f,off)for i,(p,callback,f,off)in enumerate(order)]
  for x,y in zip(ps,ps[1:]):c['w'](x+60,y)
  c['w'](0x4e9ebc,ps[0]if ps else 0);c['w'](0x4e9eb8,0x4e7938+149*64);c['w'](0x4e7934,counter)
  return c['node'](148,0,0x1a47d0)
 for location in range(4):cases.append(execute('declared_file_bank_request_'+str(location),0x19d8c0,lambda c,l=location:l*0x40000000+0xbb4800,'request'))
 for label,order in [('empty',[]),('lower',[(-1,0x1a47d0,3,5)]),('equal',[(0,0x1a47d0,1,5),(0,0x1a47d0,2,8),(0,0x1a47d0,3,9)]),('nonread',[(0,0,1,5),(0,0,2,8)])]:
  cases.append(execute('publish_'+label,0x1220e0,lambda c,o=order:insertion(c,o,0xffffffff),'queue'))
 for index in(0,1,2):
  def unlink(c,index=index):
   ps=[c['node'](i,0,0x1a47d0)for i in range(3)]
   for x,y in zip(ps,ps[1:]):c['w'](x+60,y)
   c['w'](0x4e9ebc,ps[0]);return ps[index]
  cases.append(execute('unlink_'+str(index),0x122150,unlink,'queue'))
 def worker(c,total=0x7a00,offset=0xbb4800):
  p=c['node'](0,3,0x1a47d0,total=total,offset=offset);c['w'](0x4e9ebc,p);c['w'](0x4e9eb8,p+64);c['sem'].release();return p
 for label,total,off in [('actual_lockout_descriptor',0x7a00,0xbb4800),('actual_eof_short_read',64,MAP.stat().st_size-13),('actual_past_eof_read',64,MAP.stat().st_size+100),('actual_multichunk_caller_control',0x40010,0xbb4800),('actual_zero_count',0,0),('actual_seek_low_sentinel',64,0xffffffff)]:
  cases.append(execute(label,0x122360,lambda c,t=total,o=off:worker(c,t,o),'file'))
 cases.append(execute('actual_owned_pipe_failed_seek',0x122360,worker,'file_nonseek'))
 # These are separately labelled callback interface controls, not actual file
 # outcomes or arbitrary positive world/residency services.
 for result in(2,0xffffffff):cases.append(execute('explicit_callback_control_'+str(result),0x122360,worker,'callback_control',result))
 # One retry control ends at a declared observation boundary before a second
 # callback: handled separately in next extension, no artificial permit loss.
 report=dict(schema='animation-async-worker-original-v1',tool_sha256=sha(__file__),source_pins=PINS,cache_sha256=MAP_PIN,base_tu_file_callback=comparison(a.output),cases=cases,
   provider='Actual Python RLock/bounded semaphore and read-only Lockout stream. Explicit retained aliases are host adaptation inputs, not recovered Xbox handles/TLS. Worker root runs on calling host thread; no source worker creation claimed. Native no-permit stop occurs before a source wait result/store.',
   unconsumed_provider_return_registers='Wait/release/ReadFile EAX is preserved, not a fabricated Xbox status/BOOL; source consumers do not read it. SetFilePointer low result and actual bytes output are consumed and retained.',
   boundaries=['Queue raw bank starts from immutable actual1221B0 prefix output; only named native alias binding is supplied.','Queue list/counter and callback-return variants are separately explicit caller controls.','File request size/offset in first case come from actual frozen resource descriptor request; native token/path binding is explicit, not an original open handle producer.','No resource loaded byte, IO readiness, Unit/body fields, source TLS/SEH or kernel status supplied.'],instruction_patches=[],resource_success=[])
 save(a.output/'report.json',encoded(report));print(json.dumps(dict(report=str(a.output/'report.json'),sha256=sha(a.output/'report.json'),cases=len(cases),stores=sum(len(c['stores'])for c in cases))))
if __name__=='__main__':main()
