#!/usr/bin/env python3
"""Execute unchanged TU1.5 startup through network/QoS storage construction.

No game bank snapshot, constructor field, allocation cursor, pool validity or
completion byte is imported. Actual anonymous host pages and real temporary
file open/seek/truncate outcomes cross named platform boundaries. Mutex and
semaphore objects are real host resources. The reference emulator uses an
explicit cooperative thread schedule: main yields run the complete original
worker until its next unavailable semaphore wait. This is a scheduler input,
not an Xbox thread timing/TLS/priority contract or live Match/world proof.
The priority boundary attempts the native operation on an owned host thread;
its real failure is allowed because the source caller ignores that BOOL.

Run 6DE60/70660/7EC90 and the real shared allocation/pool constructor, then
stop before XNetGetEthernetLinkStatus. No online/offline link status or XNet
initialization result is supplied to avoid its later branch. This is a retained
platform frontier, not a successful whole-game initializer or live-world proof.
"""
import argparse,hashlib,json,mmap,os,struct,tempfile,threading
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN as PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 a.output.mkdir(parents=True,exist_ok=False)
 source=NEW_DEFAULT;assert sha(source)==PIN
 image=Xbe(source);code=_Code(image,'.text')
 paired_path=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-v1/complete-report.json'
 physical_path=ROOT/'analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012/correspondence-v1/complete-report.json'
 assert sha(paired_path)=='616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933'
 assert sha(physical_path)=='4e81e7a5fd05edae4d24a35152896df4191aed827da55ba1d87b559bcb8e540f'
 pair=json.loads(paired_path.read_text());physical_pair=json.loads(physical_path.read_text())
 world_path=ROOT/'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011/correspondence-v2/complete-report.json'
 assert sha(world_path)=='92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc'
 world_pair=json.loads(world_path.read_text())
 pool_path=ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
 assert sha(pool_path)=='6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527'
 pool_pair=json.loads(pool_path.read_text())
 entries={int(p,16):q for d in (pair,physical_pair)for p,q in d['entries'].items()}
 entries[0x13a700]=0x13a700
 imap={p:q for d in (pair,physical_pair)for p,q in d['instruction_map'].items()};allowed=set(imap.values())
 control_map={p:q for p,q in world_pair['instruction_map'].items() if 0x3276fc<=int(p,16)<=0x327897}
 allowed.update(control_map.values())
 allowed.update(pool_pair['instruction_map'].values())
 for start,size,pin in ((0x6de60,61,'56f605989b6fb19f929f71f9b534ac85415f824d4ff1588d0a1ef5f61734b463'),
                        (0x70660,87,'d8a807d350cdf84609c1bb3ba95f355dbf0cfea882d829b3473e411c139f67d9'),
                        (0x7ec90,76,'d658e9fed01a7fb3f002226e440421820570864d0d017d09d9618b96e1061811')):
  assert hashlib.sha256(image.read(start,size)).hexdigest()==pin
  p=start
  while p<start+size:allowed.add(p);p+=code.at(p).size
 # The actual-ID pool constructor/string-copy bodies are independently
 # lowered from this authenticated image. Admit only those complete bodies.
 import re
 support=(ROOT/'src/physics_live_support_tu15_generated.cpp').read_text()
 for name in ('16e450','16e4b0','32a4e0'):
  m=re.search(r'static void run_'+name+r'\([^\n]*\)\s*\{',support);assert m
  body=support[m.start():support.index('\n}\n',m.start())+3]
  allowed.update(int(p,16)for p in re.findall(r'services.instruction\(0x([a-f0-9]+)u\)',body))
 assert hashlib.sha256(image.read(0x13a700,123)).hexdigest()=='1370232f655b7636c1e00e623b3d171d901f36088e978d15e12f398cb56b1405'
 p=0x13a700
 while p<0x13a77b:allowed.add(p);p+=code.at(p).size
 frames=json.loads((paired_path.parent.parent/'external-owners.json').read_text())
 frame_map={row['old']:row for routine in pair['routines']for row in routine['instructions']}
 expected_frames={body['later_target']:{frame_map[call['old_site']]['later']+len(bytes.fromhex(frame_map[call['old_site']]['later_bytes']))for call in body['calls']}for body in frames['external_direct_calls'].values()}
 expected_frames.setdefault(0x32a4e0,set()).add(0x16e4c9) # Complete original QoS pool name-copy caller.
 instruction_count=[0]
 g=Uc(UC_ARCH_X86,UC_MODE_32)
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
 frontiers=[];services=[];executed=set();site=[0];store_count=[0];role=['main'];paused=[False];yielded=[False];worker=[None]
 prefix=[False];prefix_stores=[];prefix_instructions=[];game_frontier=[None]
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
  if prefix[0]:prefix_stores.append(dict(instruction=site[0],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
 def write(uc,access,p,n,value,user):record(p,n,value)
 def hook(uc,p,n,user):
  if p==0x37654d:
   game_frontier[0]=abi();g.emu_stop();return
  sp=g.reg_read(R.UC_X86_REG_ESP)
  args=lambda count:[u(sp+4+4*i)for i in range(count)]
  if p==0x800000b8:
   base_cell,zero_bits,size_cell,kind,protection=args(5)
   assert u(sp)==0x33b3c5 and (u(base_cell),zero_bits,u(size_cell),kind,protection)==(0,0,0x150,0x101000,4)
   size=(u(size_cell)+4095)&~4095;base=0x10000000
   pages=mmap.mmap(-1,size);physical.append(pages);g.mem_map(base,size);g.mem_write(base,pages[:])
   w(base_cell,base);w(size_cell,size)
   services.append(dict(kind='shared_raw_allocate',arguments=[0,zero_bits,0x150,kind,protection],result=[0,base,size]))
   ret(20,0);return
  if p in expected_frames:
   assert u(sp) in expected_frames[p],(hex(p),hex(u(sp)))
   frontiers.append(dict(target=p,role=role[0],abi=abi()))
  if p==0x800000a6:
   size,low,high,alignment,protection=args(5);assert (size,low,high,alignment,protection)==(0x30e4000,0x61000,0x3144fff,0,4)
   pages=mmap.mmap(-1,size);base=low|0x80000000;g.mem_map(base,size);g.mem_write(base,pages[:]);physical.append(pages)
   services.append(dict(kind='physical_allocate',arguments=args(5),result=base));ret(20,base);return
  if p==0x800000b6:
   base,size,protection=args(3);assert protection==4 and base>=0x80061000 and base+size<=0x83145000
   # Anonymous pages already have actual host read/write protection. The
   # native comparison additionally invokes mprotect on this retained range.
   services.append(dict(kind='physical_protect',arguments=args(3)));ret(12);return
  if p==0x2e02ea:
   assert args(3)==[0,0,0];token=next_sync[0];next_sync[0]+=1;sync[token]=threading.RLock()
   services.append(dict(kind='mutex_create',result=token));ret(12,token);return
  if p==0x2e0269:
   assert args(4)==[0,0,150,0];token=next_sync[0];next_sync[0]+=1;semaphores[token]=threading.BoundedSemaphore(150)
   for _ in range(150):assert semaphores[token].acquire(False)
   services.append(dict(kind='semaphore_create',result=token));ret(16,token);return
  if p==0x2dc0b6:
   assert args(6)==[0,0x4000,0x1235f0,0,0,0]
   context=g.context_save()
   for q in regs:g.reg_write(q,0)
   g.reg_write(R.UC_X86_REG_ESP,worker_stack+0xf000);g.reg_write(R.UC_X86_REG_EFLAGS,2)
   g.reg_write(R.UC_X86_REG_EIP,0x1235f0);worker[0]=g.context_save();g.context_restore(context)
   services.append(dict(kind='worker_create',entry=0x1235f0,result=0xf5000001));ret(24,0xf5000001);return
  if p==0x2dbea6:
   assert args(2)==[0xf5000001,1]
   try:os.setpriority(os.PRIO_PROCESS,priority_id[0],-1);result=1;error=0
   except OSError as e:result=0;error=e.errno
   services.append(dict(kind='native_priority_attempt',result=result,error=error));ret(8,result);return
  if p==0x2e0478:
   token,milliseconds=args(2);assert milliseconds==0xffffffff
   if token in sync:assert sync[token].acquire(False)
   else:
    assert token in semaphores and role[0]=='worker'
    if not semaphores[token].acquire(False):paused[0]=True;g.emu_stop();return
   ret(8,0);return
  if p==0x2e0342:
   token,=args(1);sync[token].release();ret(4,1);return
  if p==0x2e02c4:
   token,count,previous=args(3);assert count==1 and token in semaphores
   semaphores[token].release();w(previous,0);ret(12,1);return
  if p==0x2dbf74:
   assert role[0]=='main';ret(0);yielded[0]=True;g.emu_stop();return
  if p==0x32ba80:
   destination,capacity,fmt,arguments=args(4);pattern=string(fmt)
   assert pattern in ('n:\\savegame%d.bin','cluster %s','%s cluster','%s reference'), (hex(fmt),repr(pattern))
   value=u(arguments);value=string(value)if '%s'in pattern else struct.unpack('<i',struct.pack('<I',value))[0]
   raw=(pattern%value).encode('ascii');assert len(raw)<capacity
   g.mem_write(destination,raw+b'\0');ret(0,len(raw));return
  if p==0x2dc640:
   name,access,share,security,creation,flags,template=args(7)
   assert (access,share,security,creation,flags,template)==(0xc0000000,0,0,4,0,0)
   name=string(name);assert name in ('n:\\savegame0.bin','n:\\savegame1.bin')
   path=Path(resource_directory.name)/name[3:]
   try:fd=os.open(path,os.O_RDWR|os.O_CREAT,0o600);token=next_file[0];next_file[0]+=1;files[token]=(fd,path);error=0
   except OSError as e:token=0xffffffff;error=e.errno
   services.append(dict(kind='file_open',source_name=name,result=token,error=error));ret(28,token);return
  if p==0x2dc378:
   token,offset,high,method=args(4);assert method==0 and u(high)==0
   result=os.lseek(files[token][0],offset,os.SEEK_SET);assert result<=0xffffffff
   services.append(dict(kind='file_seek',file=token,offset=offset,result=result));ret(16,result);return
  if p==0x2dc2f4:
   token,=args(1);fd,path=files[token];length=os.lseek(fd,0,os.SEEK_CUR);os.ftruncate(fd,length)
   services.append(dict(kind='file_truncate',file=token,bytes=os.stat(path).st_size));ret(4,1);return
  assert p in allowed,hex(p)
  instruction_count[0]+=1
  assert bytes(g.mem_read(p,n))==image.read(p,n),(hex(p),n)
  executed.add(p);site[0]=p
  if prefix[0]:prefix_instructions.append(p)
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
 def abi():return dict(gp=[g.reg_read(r)for r in regs],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
  x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
  xmm=[f'{g.reg_read(getattr(R,f"UC_X86_REG_XMM{i}")):032x}'for i in range(8)])
 def run(entry,arguments=()):
  role[0]='main'
  for q in regs:g.reg_write(q,0)
  sp=main_stack+0xf000;w(sp,stop);g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
  g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
  for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
  for i,value in enumerate(arguments):w(sp+4+4*i,value)
  before=abi();pc=entries[entry]
  while True:
   yielded[0]=False;g.emu_start(pc,stop,count=12000000)
   if not yielded[0]:break
   main=g.context_save();role[0]='worker';g.context_restore(worker[0]);paused[0]=False
   g.emu_start(g.reg_read(R.UC_X86_REG_EIP),stop,count=12000000);assert paused[0]
   worker[0]=g.context_save();g.context_restore(main);role[0]='main';pc=g.reg_read(R.UC_X86_REG_EIP)
  assert g.reg_read(R.UC_X86_REG_EIP)==(0x37654d if entry==0x13a700 else stop)
  return dict(entry=f'{entry:08x}',arguments=arguments,entry_abi=before,exit_abi=abi(),eax=g.reg_read(R.UC_X86_REG_EAX),stores=store_count[0])
 steps=[]
 try:
  for entry in (0x12cb70,0x1221b0,0x125280):
   steps.append(run(entry));print(json.dumps(steps[-1]),flush=True)
  prefix[0]=True;steps.append(run(0x13a700));assert game_frontier[0] is not None
  output.close()
  finals=[]
  for s in image.sections:
   if not s.flags&1:continue
   raw=bytes(g.mem_read(s.va,s.vsize));finals.append(dict(address=s.va,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  raw=bytes(g.mem_read(0x80061000,0x30e4000));finals.append(dict(address=0x80061000,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  listing=a.output/'executed-original.asm';listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
  arena=u(0x4f2950)
  report=dict(source_sha256=PIN,oracle_sha256=sha(__file__),boundaries=__doc__,steps=steps,stores=store_count[0],store_file='stores.bin',store_sha256=sha(a.output/'stores.bin'),final_banks=finals,services=services,external_frontiers=frontiers,arena=arena,arena_offset=u(0x4f2954),async_free_head=u(0x4ef1b8),async_work_head=u(0x4ef1bc),arena_checksum=u(0x4f295c),cache_files=[dict(token=p,bytes=os.stat(path).st_size)for p,(fd,path)in files.items()],executed_original_instructions=instruction_count[0],executed_sites=len(executed),canonical_instruction_map=imap,correspondence_sha256=sha(paired_path),physical_correspondence_sha256=sha(physical_path),instruction_listing_sha256=sha(listing),instruction_patches=[],supplied_gameplay_results=[],live_match_proof=False)
  report.update(schema='H2_NATIVE_TU15_NETWORK_STORAGE_ORIGINAL_V1',game_frontier=game_frontier[0],frontier_target=0x37654d,
   prefix_instructions=prefix_instructions,prefix_stores=prefix_stores,control_instruction_map=control_map,
   world_correspondence_sha256=sha(world_path),game_options=u(0x4f3218),game_options_sha256=hashlib.sha256(g.mem_read(u(0x4f3218),0x1200)).hexdigest(),
   game_storage_complete=False,world_startup_complete=False,reference_executable_is_runtime_input=False)
  report.update(pool_instruction_map=pool_pair['instruction_map'],pool_correspondence_sha256=sha(pool_path),
   qos_pool=u(0x4dc538),qos_pool_sha256=hashlib.sha256(g.mem_read(u(0x4dc538),0x150)).hexdigest(),
   network_fields=[dict(address=p,bytes=n,sha256=hashlib.sha256(g.mem_read(p,n)).hexdigest())for p,n in
      ((0x4dc060,0x88),(0x4dc3f0,0x144),(0x4dc534,8),(0x56e1cc,4),(0x10000000,0x1000))])
  (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('stores','arena','arena_offset','cache_files')}),flush=True)
 finally:
  output.close()
  for fd,path in files.values():os.close(fd)
  for pages in physical:pages.close()
  resource_directory.cleanup();priority_stop.set();native_priority_thread.join()
if __name__=='__main__':main()
