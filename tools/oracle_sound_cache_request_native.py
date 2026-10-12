#!/usr/bin/env python3
"""Unchanged cache owner composed with genuine host queue/IO boundaries.

Native XAPI mutex/semaphore and explicit owned shared-map binding are named
platform adaptations. No original thread/TLS/status/driver/admission is supplied.
"""
import argparse,json,struct,threading,hashlib,mmap,ctypes
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
import oracle_sound_create_native as prior
from oracle_mp_title_update_display import ROOT,OLD,Xbe,_Code,sha,save,encoded
from oracle_smg_firing_1_1 import STACK,STOP
BASE=ROOT/'analysis/multiplayer/integration/sound-cache-request-native'
CREATE=ROOT/'analysis/multiplayer/integration/sound-create-native'
PRIOR_PIN='6483852f56ef577383d3fd80edb7edec16a942f0067d654742e9370c95026318'
QUEUE=0x4e7924;QUEUE_SIZE=0x25a0;DESCRIPTOR=0x5098a0;CURRENT=0x518000
class Boundary(Exception):pass
def Capture(*args):
 g=Uc(*args);Capture.instance=g;g.frames=[]
 def capture(_g,a,n,u):
  if a==0x21a960 and len(g.frames)<3:
   g.frames.append((g.context_save(),[(lo,bytes(g.mem_read(lo,hi-lo+1)))for lo,hi,_ in g.mem_regions()]))
 g.hook_add(UC_HOOK_CODE,capture);return g

def execute(out):
 assert sha(CREATE/'final-manifest.json')==PRIOR_PIN
 frozen=json.loads((CREATE/'final-manifest.json').read_text())
 for group in('owned_current_files','evidence_files','original_inputs'):
  for rel,q in frozen[group].items():assert sha(Path(rel)if Path(rel).is_absolute()else ROOT/rel)==q['sha256'],rel
 report=json.loads((CREATE/'measured/report.json').read_text());prep=report['preparation'];maprow=next(m for m in report['maps']if m['image']=='old'and m['map']['path'].endswith('/lockout.map'))
 image=Xbe(OLD);code=_Code(image,'.text');original=prior.Uc;prior.Uc=Capture
 try:reexecuted=prior.run('old',OLD,maprow['map'],json.loads((CREATE/'paired-v2/report.json').read_text()),prep,out/'create-reexecution')
 finally:prior.Uc=original
 g=Capture.instance;assert len(g.frames)==3
 assert reexecuted==dict(maprow,listing=reexecuted['listing'])
 # Disable all inherited observation/service hooks; this independent scope
 # retains actual captured source frames and installs only named native APIs.
 # Unicorn public hook_del needs handles. Reinstantiate and copy exact whole
 # captured regions/context instead; no source field or instruction patch.
 cases=[];listings=set();file=ROOT/'assets/disc/maps/shared.map';assert file.is_file()
 for index,(context,memory)in enumerate(g.frames):
  h=Uc(UC_ARCH_X86,UC_MODE_32)
  for lo,raw in memory:h.mem_map(lo,len(raw));h.mem_write(lo,raw)
  h.context_restore(context)
  def u(at,n=4):return int.from_bytes(h.mem_read(at,n),'little')
  def w(at,v,n=4):h.mem_write(at,int(v&((1<<(8*n))-1)).to_bytes(n,'little'))
  def raw(a,n):return bytes(h.mem_read(a,n)).hex()
  sp=h.reg_read(UC_X86_REG_ESP);sound,chunk,flags=[u(sp+k)for k in(4,8,12)];ret=u(sp)
  physical_base=int(prep['roots']['cache_base'],16);allocator=int(prep['roots']['cache_allocator'],16)
  physical_size=u(allocator+0x30)<<(u(allocator+0x34)&31)
  assert physical_base%4096==0 and physical_size%4096==0
  physical=mmap.mmap(-1,physical_size,access=mmap.ACCESS_WRITE);host=(ctypes.c_char*physical_size).from_buffer(physical)
  h.mem_map_ptr(physical_base,physical_size,3,ctypes.addressof(host))
  assert flags==2 and u(chunk)>>30==2
  # Explicit host file alias bound to the actual shared.map stream. Original
  # open-handle production is NOT claimed. Only the used shared bank is bound.
  filetoken=0xf4100001;w(0x55f798,filetoken)
  aliases=(0xf3100001,0xf3100002,0xf3100003);mutex={};sem=None;permits=0
  phase='constructor';events=[];stages=[];stop=None;seen=set();io=[]
  retained=[(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))for q in reexecuted['regions']]
  retained+= [(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))for q in prep['regions'][2:]]
  retained +=[(physical_base,physical_size),(QUEUE,QUEUE_SIZE),(DESCRIPTOR,12),(CURRENT,4),(0x55f798,4)]
  def snapshot():return [dict(address=f'{a:08x}',raw_hex=raw(a,n))for a,n in retained]
  def event(q):events.append(q)
  def service(name,args):event(dict(service=name,arguments=[f'{q:08x}'for q in args]))
  def native_return(pop,result=None):
   at=h.reg_read(UC_X86_REG_ESP);to=u(at)
   if result is not None:h.reg_write(UC_X86_REG_EAX,result&0xffffffff)
   h.reg_write(UC_X86_REG_ESP,at+pop+4);h.reg_write(UC_X86_REG_EIP,to)
  def writes(_h,k,a,n,v,_u):
   if any(b<=a and a+n<=b+s for b,s in retained):
    event(dict(store=dict(instruction=f'{h.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}')))
  stream=file.open('rb')
  def hook(_h,a,n,_u):
   nonlocal sem,permits,stop
   i=code.at(a)
   if i:assert i.bytes==image.read(a,n)==bytes(h.mem_read(a,n));seen.add(a)
   esp=h.reg_read(UC_X86_REG_ESP)
   if phase=='constructor'and a==0x2dc83a:
    args=[u(esp+k)for k in(4,8,12)];assert args==[0,0,0]
    alias=aliases[len(mutex)];mutex[alias]=threading.RLock();service('native_create_mutex',args+[alias]);native_return(12,alias)
   elif phase=='constructor'and a==0x2dc7b9:
    args=[u(esp+k)for k in(4,8,12,16)];assert args==[0,0,150,0]
    sem=threading.BoundedSemaphore(150)
    for _ in range(150):assert sem.acquire(False)
    service('native_create_semaphore',args+[aliases[2]]);native_return(16,aliases[2])
   elif phase=='constructor'and a==0x2d8606:
    stop=dict(service='native worker/TLS/lifetime unavailable',entry=f'{a:08x}',arguments=[f'{u(esp+k):08x}'for k in(4,8,12,16,20,24)],result_supplied=False);raise Boundary()
   elif phase in('create_cache','worker')and a==0x2dc9c8:
    handle,ms=u(esp+4),u(esp+8);assert ms==0xffffffff
    if handle==aliases[2]:
     if not sem.acquire(False):stop=dict(service='actual native semaphore has no permit; no wait result supplied',entry=f'{a:08x}',handle=f'{handle:08x}');raise Boundary()
     permits-=1
    else:mutex[handle].acquire()
    service('native_wait_acquired',[handle,ms]);native_return(8)
   elif phase in('create_cache','worker')and a==0x2dc892:
    handle=u(esp+4);mutex[handle].release();service('native_release_mutex',[handle]);native_return(4)
   elif phase in('create_cache','worker')and a==0x2dc814:
    handle,count,output=u(esp+4),u(esp+8),u(esp+12);assert handle==aliases[2]and count==1
    previous=permits;sem.release();permits+=1;w(output,previous);service('native_release_semaphore',[handle,count,previous]);native_return(12)
   elif phase=='worker'and a==0x2d88c8:
    token,offset,high,method=[u(esp+k)for k in(4,8,12,16)];assert token==filetoken and u(high)==0 and method==0
    stream.seek(offset);actual=stream.tell()&0xffffffff;service('native_file_seek',[token,offset,0,actual]);native_return(16,actual)
   elif phase=='worker'and a==0x2d8681:
    token,buffer,size,output,overlap=[u(esp+k)for k in(4,8,12,16,20)];assert token==filetoken and overlap==0
    data=stream.read(size);h.mem_write(buffer,data);w(output,len(data));service('native_file_read',[token,buffer,size,len(data)]);io.append(dict(offset=stream.tell()-len(data),size=len(data),sha256=hashlib.sha256(data).hexdigest()));native_return(20)
   elif phase=='create_cache'and a==ret:raise Boundary()
   elif 0x80000000<=a<0x80001000:stop=dict(service='unprovided actual kernel request',entry=f'{a:08x}');raise Boundary()
  h.hook_add(UC_HOOK_CODE,hook);h.hook_add(UC_HOOK_MEM_WRITE,writes)
  def stage(name,entry,setup=None):
   nonlocal stop
   before=snapshot();start=len(events);stop=None
   if setup:setup()
   try:h.emu_start(entry,STOP,count=1000000)
   except Boundary:pass
   result=dict(name=name,entry=f'{entry:08x}',before=before,after=snapshot(),events=events[start:],stop=stop,returned=h.reg_read(UC_X86_REG_EIP)==STOP,eax=f'{h.reg_read(UC_X86_REG_EAX):08x}')
   stages.append(result);return result
  def constructor_frame():w(STACK-0x8000,STOP);h.reg_write(UC_X86_REG_ESP,STACK-0x8000)
  r=stage('complete_queue_constructor_prefix',0x1221b0,constructor_frame);assert r['stop']and not r['returned']
  phase='create_cache';h.context_restore(context)
  r=stage('actual_cache_request_publishes_read',0x21a960);assert not r['returned']and r['stop']is None and r['eax']=='00000001'
  assert u(chunk+8)!=0xffffffff;cache_entry=u(int(prep['roots']['cache_pool'],16)+0x44)+(u(chunk+8)&0xffff)*16
  assert u(cache_entry+2,1)==0 and permits==1
  phase='worker'
  def worker_frame():w(STACK-0x8000,STOP);h.reg_write(UC_X86_REG_ESP,STACK-0x8000)
  r=stage('real_owned_file_worker_completion',0x122360,worker_frame);assert r['stop']and u(cache_entry+2,1)==1 and permits==0
  phase='cache_controls'
  for fl,expected in((0,2),(4,6),(5,6)):
   def request_frame(fl=fl):h.mem_write(STACK-0x8000,struct.pack('<IIII',STOP,sound,chunk,fl));h.reg_write(UC_X86_REG_ESP,STACK-0x8000)
   r=stage('actual_completed_cache_flags_'+str(fl),0x21a960,request_frame);assert r['returned']and int(r['eax'],16)==expected
  stream.close();listings|=seen;h.mem_unmap(physical_base,physical_size);del host;physical.close()
  cases.append(dict(scenario=reexecuted['cases'][index]['scenario'],sound=f'{sound:08x}',chunk=f'{chunk:08x}',state={k:prep['roots'][k]for k in('cache_pool','cache_allocator','cache_base')},
    constants=dict(delete_callback_identity='0021aaf0',busy_callback_identity='0021aab0'),stages=stages,io=io,
    boundaries=['Unchanged actual127750 cache-entry frame reexecuted from frozen constructors/map/rawABI composition; original admission remains false.',
      'Actual1221B0 prefix owns all queue stores and actual native mutex/semaphore aliases; it stops before original worker/TLS/lifetime, never supplying thread success.',
      'The source-created physical buffer/capacity has actual new anonymous zero-filled host backing; no cache field/readiness is changed. Actual shared.map stream alias is an explicit native file-bank binding. No original1A4250 file-open owner/kernelhandle production is claimed.',
      'Original immortal worker is executed synchronously on caller thread and stops at the next genuinely unavailable semaphore permit. No source shutdown/wait result is supplied.']))
 rows=[code.at(a)for a in sorted(listings)];save(out/'instructions/executed-old.asm',''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows).encode())
 r=dict(schema='sound-cache-request-native-original-v1',tool_sha256=sha(__file__),prior_manifest_sha256=PRIOR_PIN,paired_report_sha256=sha(BASE/'paired/report.json'),image_sha256=sha(OLD),shared_file=dict(path=str(file),sha256=sha(file)),
  cases=cases,source_admitted=False,voice_or_playback=False,instruction_patches=[],supplied_ready_or_completion=[],native_platform_adaptation=True)
 save(out/'report.json',encoded(r));return r
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=execute(a.output.resolve());print(json.dumps(dict(cases=len(r['cases']),sha256=sha(a.output/'report.json'))))
