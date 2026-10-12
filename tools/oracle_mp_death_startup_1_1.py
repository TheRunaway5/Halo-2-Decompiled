#!/usr/bin/env python3
"""Execute original physical/async/arena/object startup from the original image.

No game bank snapshot, constructor field, allocation cursor, pool validity or
completion byte is imported. Actual anonymous host pages and real temporary
file open/seek/truncate outcomes cross named platform boundaries. Mutex and
semaphore objects are real host resources. The reference emulator uses an
explicit cooperative thread schedule: main yields run the complete original
worker until its next unavailable semaphore wait. This is a scheduler input,
not an Xbox thread timing/TLS/priority contract or live Match/world proof.
The priority boundary attempts the native operation on an owned host thread;
its real failure is allowed because the source caller ignores that BOOL.
"""
import argparse,hashlib,json,mmap,os,struct,tempfile,threading
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 a.output.mkdir(parents=True,exist_ok=False)
 source=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03';assert sha(source)==PIN
 image=Xbe(source);code=_Code(image,'.text');g=Uc(UC_ARCH_X86,UC_MODE_32)
 g.mem_map(0,0x810000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 g.mem_map(0x80000000,4096)
 main_stack,worker_stack,stop=0x60000000,0x60100000,0x60200000
 for p in (main_stack,worker_stack):g.mem_map(p,0x10000)
 g.mem_map(stop,4096)
 regs=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
 def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
 def w(p,v,n=4):g.mem_write(p,v.to_bytes(n,'little'))
 def string(p):
  raw=bytearray()
  while True:
   b=u(p+len(raw),1)
   if not b:return raw.decode('ascii')
   raw.append(b);assert len(raw)<4096
 def ret(n,eax=None):
  sp=g.reg_read(R.UC_X86_REG_ESP);target=u(sp)
  if eax is not None:g.reg_write(R.UC_X86_REG_EAX,eax&0xffffffff)
  g.reg_write(R.UC_X86_REG_ESP,sp+4+n);g.reg_write(R.UC_X86_REG_EIP,target)
 services=[];executed=set();site=[0];store_count=[0];role=['main'];paused=[False];yielded=[False];worker=[None]
 sync={};semaphores={};files={};physical=[];next_sync=[0xf3000001];next_file=[0xf4000001]
 priority_ready=threading.Event();priority_stop=threading.Event();priority_id=[None]
 def priority_task():priority_id[0]=threading.get_native_id();priority_ready.set();priority_stop.wait()
 native_priority_thread=threading.Thread(target=priority_task);native_priority_thread.start();priority_ready.wait()
 resource_directory=tempfile.TemporaryDirectory(prefix='halo2-source-death-startup-')
 output=(a.output/'stores.bin').open('wb')
 def record(p,n,value):
  if main_stack<=p<main_stack+0x10000 or worker_stack<=p<worker_stack+0x10000:return
  assert n in (1,2,4,8)
  output.write(struct.pack('<IIBQQ',site[0],p,n,u(p,n),value&((1<<(n*8))-1)));store_count[0]+=1
 def write(uc,access,p,n,value,user):record(p,n,value)
 def hook(uc,p,n,user):
  sp=g.reg_read(R.UC_X86_REG_ESP)
  args=lambda count:[u(sp+4+4*i)for i in range(count)]
  if p==0x800000a6:
   size,low,high,alignment,protection=args(5);assert (size,low,high,alignment,protection)==(0x30e4000,0x61000,0x3144fff,0,4)
   pages=mmap.mmap(-1,size);base=low|0x80000000;g.mem_map(base,size);g.mem_write(base,pages[:]);physical.append(pages)
   services.append(dict(kind='physical_allocate',arguments=args(5),result=base));ret(20,base);return
  if p==0x800000b6:
   base,size,protection=args(3);assert protection==4 and base>=0x80061000 and base+size<=0x83145000
   # Anonymous pages already have actual host read/write protection. The
   # native comparison additionally invokes mprotect on this retained range.
   services.append(dict(kind='physical_protect',arguments=args(3)));ret(12);return
  if p==0x2dc83a:
   assert args(3)==[0,0,0];token=next_sync[0];next_sync[0]+=1;sync[token]=threading.RLock()
   services.append(dict(kind='mutex_create',result=token));ret(12,token);return
  if p==0x2dc7b9:
   assert args(4)==[0,0,150,0];token=next_sync[0];next_sync[0]+=1;semaphores[token]=threading.BoundedSemaphore(150)
   for _ in range(150):assert semaphores[token].acquire(False)
   services.append(dict(kind='semaphore_create',result=token));ret(16,token);return
  if p==0x2d8606:
   assert args(6)==[0,0x4000,0x122350,0,0,0]
   context=g.context_save()
   for q in regs:g.reg_write(q,0)
   g.reg_write(R.UC_X86_REG_ESP,worker_stack+0xf000);g.reg_write(R.UC_X86_REG_EFLAGS,2)
   g.reg_write(R.UC_X86_REG_EIP,0x122350);worker[0]=g.context_save();g.context_restore(context)
   services.append(dict(kind='worker_create',entry=0x122350,result=0xf5000001));ret(24,0xf5000001);return
  if p==0x2d83f6:
   assert args(2)==[0xf5000001,1]
   try:os.setpriority(os.PRIO_PROCESS,priority_id[0],-1);result=1;error=0
   except OSError as e:result=0;error=e.errno
   services.append(dict(kind='native_priority_attempt',result=result,error=error));ret(8,result);return
  if p==0x2dc9c8:
   token,milliseconds=args(2);assert milliseconds==0xffffffff
   if token in sync:assert sync[token].acquire(False)
   else:
    assert token in semaphores and role[0]=='worker'
    if not semaphores[token].acquire(False):paused[0]=True;g.emu_stop();return
   ret(8,0);return
  if p==0x2dc892:
   token,=args(1);sync[token].release();ret(4,1);return
  if p==0x2dc814:
   token,count,previous=args(3);assert count==1 and token in semaphores
   semaphores[token].release();w(previous,0);ret(12,1);return
  if p==0x2d84c4:
   assert role[0]=='main';ret(0);yielded[0]=True;g.emu_stop();return
  if p==0x327f10:
   destination,capacity,fmt,arguments=args(4);pattern=string(fmt)
   assert pattern in ('n:\\savegame%d.bin','cluster %s','%s cluster','%s reference'), (hex(fmt),repr(pattern))
   value=u(arguments);value=string(value)if '%s'in pattern else struct.unpack('<i',struct.pack('<I',value))[0]
   raw=(pattern%value).encode('ascii');assert len(raw)<capacity
   g.mem_write(destination,raw+b'\0');ret(0,len(raw));return
  if p==0x2d8b90:
   name,access,share,security,creation,flags,template=args(7)
   assert (access,share,security,creation,flags,template)==(0xc0000000,0,0,4,0,0)
   name=string(name);assert name in ('n:\\savegame0.bin','n:\\savegame1.bin')
   path=Path(resource_directory.name)/name[3:]
   try:fd=os.open(path,os.O_RDWR|os.O_CREAT,0o600);token=next_file[0];next_file[0]+=1;files[token]=(fd,path);error=0
   except OSError as e:token=0xffffffff;error=e.errno
   services.append(dict(kind='file_open',source_name=name,result=token,error=error));ret(28,token);return
  if p==0x2d88c8:
   token,offset,high,method=args(4);assert method==0 and u(high)==0
   result=os.lseek(files[token][0],offset,os.SEEK_SET);assert result<=0xffffffff
   services.append(dict(kind='file_seek',file=token,offset=offset,result=result));ret(16,result);return
  if p==0x2d8844:
   token,=args(1);fd,path=files[token];length=os.lseek(fd,0,os.SEEK_CUR);os.ftruncate(fd,length)
   services.append(dict(kind='file_truncate',file=token,bytes=os.stat(path).st_size));ret(4,1);return
  assert code.lo<=p<code.hi,hex(p)
  assert bytes(g.mem_read(p,n))==image.read(p,n),(hex(p),n)
  executed.add(p);site[0]=p
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
 def run(entry):
  role[0]='main'
  for q in regs:g.reg_write(q,0)
  sp=main_stack+0xf000;w(sp,stop);g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
  g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  pc=entry
  while True:
   yielded[0]=False;g.emu_start(pc,stop,count=12000000)
   if not yielded[0]:break
   main=g.context_save();role[0]='worker';g.context_restore(worker[0]);paused[0]=False
   g.emu_start(g.reg_read(R.UC_X86_REG_EIP),stop,count=12000000);assert paused[0]
   worker[0]=g.context_save();g.context_restore(main);role[0]='main';pc=g.reg_read(R.UC_X86_REG_EIP)
  assert g.reg_read(R.UC_X86_REG_EIP)==stop and g.reg_read(R.UC_X86_REG_ESP)==sp+4
  return dict(entry=f'{entry:08x}',eax=g.reg_read(R.UC_X86_REG_EAX),stores=store_count[0])
 steps=[]
 try:
  for entry in (0x12cb70,0x1221b0,0x125280,0xb76b0):
   steps.append(run(entry));print(json.dumps(steps[-1]),flush=True)
  output.close()
  finals=[]
  for s in image.sections:
   if not s.flags&1:continue
   raw=bytes(g.mem_read(s.va,s.vsize));finals.append(dict(address=s.va,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  raw=bytes(g.mem_read(0x80061000,0x30e4000));finals.append(dict(address=0x80061000,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  listing=a.output/'executed-original.asm';listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
  pool=u(0x4e78d0);arena=u(0x4ed650)
  report=dict(source_sha256=PIN,oracle_sha256=sha(__file__),boundaries=__doc__,steps=steps,stores=store_count[0],store_file='stores.bin',store_sha256=sha(a.output/'stores.bin'),final_banks=finals,services=services,arena=arena,arena_offset=u(0x4ed654),object_pool=pool,object_pool_valid=u(pool+0x29,1),object_heap=u(0x4e58bc),async_free_head=u(0x4e9eb8),async_work_head=u(0x4e9ebc),arena_checksum=u(0x4ed65c),cache_files=[dict(token=p,bytes=os.stat(path).st_size)for p,(fd,path)in files.items()],executed_original_instructions=len(executed),instruction_listing_sha256=sha(listing),instruction_patches=[],supplied_gameplay_results=[],live_match_proof=False)
  (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('stores','arena','arena_offset','object_pool','object_pool_valid','cache_files')}),flush=True)
 finally:
  output.close()
  for fd,path in files.values():os.close(fd)
  for pages in physical:pages.close()
  resource_directory.cleanup();priority_stop.set();native_priority_thread.join()
if __name__=='__main__':main()
