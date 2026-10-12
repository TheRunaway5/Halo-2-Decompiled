#!/usr/bin/env python3
"""Continue unchanged source audio constructor to its next actual device gate.

Uses the frozen raw storage/clock/context/critical-section producers. No sound
device, pool allocation, capability or hardware response is supplied anew.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from oracle_smg_audio_rng_startup_1_1 import RngTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,CONTEXT_OUT,CONTEXT_EXE_PIN,digest
from oracle_mp_spawn_fixed_memory_world import FixedTrace
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_constructor_equipped_1_1 import _Code,_trace
from oracle_smg_firing_1_1 import STOP

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-device-prefix'
FROZEN={**OLDER,
 'tools/oracle_smg_audio_rng_startup_1_1.py':'c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6',
 'tools/audit_smg_audio_rng_startup_1_1.py':'6eb81b808fd48a039ac5a7beb88757fe12701270ba5d602fd4f3cafbb133f6cf',
 'docs/SMG_AUDIO_STARTUP_1_1_RESEARCH.md':'4033273b067a875d7a74e14c3029e2777ecbf79216a1fe443a60e71e4d240c17',
 'docs/SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md':'789deca189492580b2f351024a30eb7a898c5add411bb843c71a0d4e2193497a',
 'analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen/suite.json':'830913b3064076209284c4772f1531254e7a25a1f4ac852bec39f4f13d35b146',
 'analysis/multiplayer/map-pack/smg-audio-rng-startup/source-fixture.json':'d2f3ccd5468e56d522b24ddc54329e79318870df5ad1c70c3054c4018b2297cc',
 'analysis/multiplayer/map-pack/smg-audio-rng-startup/audit.json':'3b3f1e47c77490a8dd88a6c0e296dd3d40a546caa99f986a6cb35beb408eff02'}
ENTRIES=(0x2205C0,0x383DD7,0x3804DE,0x383D6A,0x3802BF,0x383F92,0x383ECC,
 0x380772,0x383C31,0x3832AA,0x384C7C,0x3858D8,0x3803E1)

class DeviceTrace(RngTrace):
 def __init__(self,data,replay=None):
  self.device_running=False;self.device_executed=set();self.device_calls=[];self.device_returns=[];self.device_stores=[]
  super().__init__(data,replay);self.ds_code=_Code(self.image,'DSOUND')
 def device_state(self):
  p=self.u(0x526474);sys=self.u(0x4ED950);ds=self.u(0x38D7D8)
  return {'driver_pointer':f'{p:08x}','driver_2ad8_hex':bytes(self.g.mem_read(p,0x2AD8)).hex()if p else None,
   'source_sound_system_pointer':f'{sys:08x}','source_sound_initialized_78':self.u(sys+0x78,1),
   'source_shared_DirectSound_pointer':f'{ds:08x}','source_DirectSound_2c_hex':bytes(self.g.mem_read(ds,0x2C)).hex()if ds else None,
   'source_DirectSound_allocation_statistics_pointer':f'{self.u(0x38D804):08x}',
   'source_DirectSound_private_pool':f'{self.u(0x38D74C):08x}',
   'source_DirectSound_critical_section_pointer':'0038d158',
   'source_DirectSound_critical_section_1c_hex':bytes(self.g.mem_read(0x38D158,0x1C)).hex(),
   'source_directsound_imports':{f'{a:08x}':f'{self.u(a):08x}'for a in(0x417BE0,0x417CFC,0x417D4C)},
   'compatibility_current_thread_token':f'{self.context["ethread_address"]:08x}',
   'original_registers':self.registers()}
 def hook(self,g,a,size,user):
  if self.device_running:
   if self.code.at(a)or self.ds_code.at(a):self.device_executed.add(a)
   for ret,row in self.device_returns[:]:
    if a==ret:row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.device_returns.remove((ret,row))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}','return_address':f'{self.u(sp):08x}',
     'registers':self.registers(),'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)]}
    self.device_calls.append(row);self.device_returns.append((self.u(sp),row))
   if a==0x2205C0:
    # Continue the exact original frame retained at its source call entry.
    # Only the prior stop observer is omitted; every producer byte executes.
    FixedTrace.hook(self,g,a,size,user);return
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.device_running:
   p=self.u(0x526474)
   if 0x38D000<=address<0x38E000 or address==0x526474 or (p and p<=address<p+0x2AD8):
    self.device_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
     'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
     'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def continue_device(self):
  assert self.g.reg_read(UC_X86_REG_EIP)==0x2205C0
  self.active='resume_original_full_sound_constructor_into_real_device_prefix'
  self.device_running=True;before=self.device_state();start=len(self.lock_calls);helper_start=len(self.helper_calls)
  try:self.g.emu_start(0x2205C0,STOP,count=1000000)
  except (KernelBoundary,unicorn.UcError):assert self.new_invalid,self.g.reg_read(UC_X86_REG_EIP)
  else:raise AssertionError('Whole audio initialization returned unexpectedly; boundary must be reviewed')
  for _,row in self.device_returns:row['halted_before_return_at_dependency']=self.new_invalid['boundary']
  return {'before':before,'after':self.device_state(),'first_unprovided_dependency':self.new_invalid,
   'original_source_frame_preserved_from_126b20_call_126c90':True,
   'critical_section_calls':self.lock_calls[start:],'actual_host_atomic_helpers':self.helper_calls[helper_start:],
   'original_device_calls':self.device_calls,'original_device_stores':self.device_stores,
   'source_driver_constructor_or_DirectSound_return_supplied':False}

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'working/report.json')
 ap.add_argument('--replay',type=Path);ap.add_argument('--context-replay',type=Path);args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 replay=None if args.replay is None else json.loads(args.replay.read_text())['clock_calls'];proc=None
 if args.context_replay:data=json.loads(args.context_replay.read_text())
 else:
  assert digest(CONTEXT_OUT/'generator.exe')==CONTEXT_EXE_PIN
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
  proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
  data=json.loads(proc.stdout.readline())
 args.output.parent.mkdir(parents=True,exist_ok=True)
 try:t=DeviceTrace(data,replay);preparation=t.execute();result=t.continue_device()
 finally:
  if proc is not None:
   _,err=proc.communicate('x',timeout=60);(args.output.parent/'runtime-stderr.txt').write_text(err);assert proc.returncode==0,err
 context_path=args.output.parent/'raw-host-context.json';context_path.write_text(json.dumps(data,indent=2)+'\n')
 d=args.output.parent/'instructions';d.mkdir(exist_ok=True);listings=[]
 for entry in ENTRIES:
  code=t.code if t.code.at(entry)else t.ds_code
  f=_trace(code,entry,t.starts|set(ENTRIES));ins=[code.at(a)for a in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=d/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append({'entry':f'{entry:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins),
   'executed':sum(i.address in t.device_executed for i in ins),'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'}for a,b,k in f.sites if k=='call']})
 ins=[t.code.at(a)or t.ds_code.at(a)for a in sorted(t.device_executed)]
 for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
 p=d/'executed-original-device-instructions.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 for file,h in FROZEN.items():assert digest(ROOT/file)==h,file
 report={'source_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_sha256':digest(__file__),'frozen_pins':FROZEN,
  'context_snapshot_sha256':digest(context_path),'context_source_executable_sha256':CONTEXT_EXE_PIN,
  'context_mode':'whole_recorded_context_replay'if args.context_replay else'real_Wine_worker_alive_during_source_execution',
  'clock_mode':'verbatim_measured_input_replay'if args.replay else'real_host_clock_measurement',
  'clock_calls':t.clock_calls,'original_rng_preparation':preparation,'original_rng_writes':t.rng_writes,
  'source_device_prefix':result,'instruction_listings':listings,
  'executed_original_device_listing':{'file':p.name,'sha256':digest(p),'instructions':len(ins)},
  'new_raw_pool_device_capability_hardware_or_consumption_service_supplied':False,
  'whole_original_DirectSound_or_audio_initialization_completed':False,'native_gameplay_update_applied':False,
  'boundaries':[
   'Frozen real raw backing/clock/whole primary compatibility context and exact CRT/game/audio producer prerequisites execute anew. Full title/XAPI allocation order, normal Xbox TLS/hardware context and prior rand consumption remain unproved.',
   'Execution resumes the exact source-produced126B20 frame at original2205C0 entry after its original126C90 call. The old gate observer is omitted; no producer byte is changed and no return is supplied.',
   'Original DirectSound critical section38D158 comes unchanged from the executable. Previously pinned primary compatibility RtlEnterCriticalSection body executes with actual host-serialized atomic and produced ETHREAD ownership token, not a supplied success.',
   'Unprovided device/kernel requests are stopped before execution. No raw pool allocation, allocation-size result, DirectSound pointer/success, driver capability, voice/stream, hardware register or completion is supplied anew.',
   'Partial source objects, ownership/lock state and frame remain retained at the actual boundary; no unwind/cleanup success is substituted. Independent repeat begins with a fresh original source instance and the recorded whole context/clocks.',
   'All earlier freezes and production remain unchanged.']}
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'output':str(args.output),'dependency':result['first_unprovided_dependency'],
  'executed':len(ins),'calls':[x['entry']for x in t.device_calls],'initialized':t.device_state()['source_sound_initialized_78']}))
if __name__=='__main__':main()
