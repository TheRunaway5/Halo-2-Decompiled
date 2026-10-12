#!/usr/bin/env python3
"""Separate original boot/local/controller RNG producers with measured clocks.

Unchanged original heap/CRT constructors run on the pinned compatibility host
context. Actual host mmap/mprotect backs requested raw pages, without writing
heap, RNG, ready or audio fields. Complete Xbox/title startup is not asserted.
"""
import argparse,ctypes,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_fixed_memory_world import FixedTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,digest,CONTEXT_OUT
from oracle_mp_spawn_process_heap_world import RAW_HEAP,KernelBoundary
from oracle_smg_audio_startup_1_1 import StartupTrace as ClockProvider,CLOCK_SERVICES,FILETIME_EPOCH_100NS,CONTEXT_EXE_PIN
from oracle_smg_audio_cache_1_1 import CacheTrace as CacheStorage
from audit_smg_audio_startup_1_1 import PRIOR_PINS as AUDIO_PINS
from oracle_smg_firing_1_1 import STACK,STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-rng-startup'
AUDIO_ARENA,AUDIO_CAPACITY=0xA000000,0x23000
FROZEN={**OLDER,**AUDIO_PINS,
 'tools/oracle_mp_spawn_fixed_memory_world.py':'d04c6f66b32cbbdf039a1c0f4bfa91748b6fe184c77b035d0e5ae64b7ae40d61',
 'docs/MP_FIXED_MEMORY_WORLD_RESEARCH.md':'68a1e2a428d9ec4cb591d0958f47214126b9a91a94f329147d7a62247a726f71',
 'analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json':'93c2f875b646ee1e298059237d1ba9f5e064153834d8188376bb101b9a7df75d',
 'tools/oracle_smg_audio_startup_1_1.py':'cbca5876b76195d85071aebfa7f404544f95152a832b9f038deb499d4f4ca6a3',
 'analysis/multiplayer/map-pack/smg-audio-startup/frozen/suite.json':'a368b7735502f02ec0db5d164cdd1f049367dec81fc76acc5942a1453dfa2a4c',
 'analysis/multiplayer/map-pack/smg-audio-startup/audit.json':'ccdee10aab8b1f0cfdea3a2511b4ac7b5ca9fd54b0dc88ec0ffb4e30a62f7f78'}
ENTRIES=(0x1479B0,0x147A20,0x139390,0x32803E,0x328505,0x32C609,0x32E813,0x2D8674,
 0x32CFFC,0x32C7A6,0x3274E4,0x2D7F8E,0x2DA83A,0x2D7969,0x2DAC5E,0x125280,
 0x126B20,0x21A6E0,0x21BC50,0x2205C0,0x11E340,0x16CFF0)

class RngTrace(FixedTrace):
 def __init__(self,data,replay=None):
  self.rng_enabled=False;self.rng_calls=[];self.rng_returns=[];self.rng_writes=[];self.clock_calls=[]
  self.replay=replay;self.replay_cursor=0;self.rng_executed=set();self.raw_providers=[];self.host_backings=[];self.next_vm=0xA100000;self.backing=[]
  self.libc=ctypes.CDLL(None,use_errno=True)
  self.libc.mmap.restype=ctypes.c_void_p;self.libc.mmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long]
  self.libc.mprotect.restype=ctypes.c_int;self.libc.mprotect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int]
  super().__init__(data);self.rng_enabled=True
 def clock_provider(self,a):ClockProvider.clock_provider(self,a)
 def ret(self,pop,result):
  sp=self.g.reg_read(UC_X86_REG_ESP);self.g.reg_write(UC_X86_REG_EAX,result)
  self.g.reg_write(UC_X86_REG_ESP,sp+4+pop*4);self.g.reg_write(UC_X86_REG_EIP,self.u(sp))
 def raw_heap_provider(self):
  g=self.g;sp=g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(5)]
  baseptr,zerobits,sizeptr,kind,protect=args;requested_base,requested_size=self.u(baseptr),self.u(sizeptr)
  assert zerobits==0 and protect==4
  if kind==0x2000:
   assert not self.heap_reserved and requested_base==0 and requested_size==0x100000
   ctypes.set_errno(0);p=self.libc.mmap(None,requested_size,0,0x22,-1,0);err=ctypes.get_errno()
   assert p not in(None,ctypes.c_void_p(-1).value),(p,err)
   self.heap_host=p;self.host_backings.append((p,requested_size));self.heap_reserved=True
   g.mem_map_ptr(RAW_HEAP,requested_size,0,p);self.w(baseptr,RAW_HEAP)
   operation={'api':'actual libc mmap anonymous private PROT_NONE','actual_errno':err,'actual_host_address':f'{p:x}'}
  else:
   assert kind==0x1000 and self.heap_reserved and requested_base==RAW_HEAP and requested_size==0x1000
   ctypes.set_errno(0);result=self.libc.mprotect(self.heap_host,requested_size,3);err=ctypes.get_errno();assert result==0,(result,err)
   g.mem_protect(RAW_HEAP,requested_size,unicorn.UC_PROT_READ|unicorn.UC_PROT_WRITE)
   operation={'api':'actual libc mprotect PROT_READ|PROT_WRITE','actual_result':result,'actual_errno':err}
  row={'phase':self.active,'original_return_site':f'{self.u(sp):08x}','args':[f'{v:08x}'for v in args],
   'requested_base':f'{requested_base:08x}','requested_size':requested_size,'resulting_base':f'{self.u(baseptr):08x}',
   'resulting_size':self.u(sizeptr),'raw_provider_operation':operation,
   'compatibility_NTSTATUS_from_actual_OS_success':'00000000','retail_Xbox_NTSTATUS_not_claimed':True,
   'guest_alias':'08000000','provider_writes_only_raw_base_and_access':True}
  self.raw_providers.append(row);self.ret(5,0)
 def cache_vm_provider(self):
  sp=self.g.reg_read(UC_X86_REG_ESP);args=[self.u(sp+4+i*4)for i in range(4)]
  assert args[0]==0 and args[2:]==[0x101000,4],args
  amount=(args[1]+4095)&~4095;ctypes.set_errno(0);p=self.libc.mmap(None,amount,3,0x22,-1,0);err=ctypes.get_errno()
  assert p not in(None,ctypes.c_void_p(-1).value),(p,err)
  alias=self.next_vm;self.next_vm+=amount;self.host_backings.append((p,amount))
  assert all(alias+amount<=a or alias>b for a,b,_ in self.g.mem_regions())
  self.g.mem_map_ptr(alias,amount,3,p)
  self.raw_providers.append({'phase':self.active,'entry':'0033785c','original_return_site':f'{self.u(sp):08x}',
   'args':[f'{v:08x}'for v in args],'requested_size':args[1],'rounded_size':amount,'resulting_base':f'{alias:08x}',
   'raw_provider_operation':{'api':'actual libc mmap anonymous private PROT_READ|PROT_WRITE','actual_errno':err,'actual_host_address':f'{p:x}'},
   'provider_writes_only_raw_base_and_access':True,'original_cache_constructor_result_supplied':False})
  self.ret(4,alias)
 def physical_provider(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP);allocate=a==self.u(0x417BA0);n=5 if allocate else 3
  args=[self.u(sp+4+i*4)for i in range(n)]
  row={'phase':self.active,'kernel_target':f'{a:08x}','original_return_site':f'{self.u(sp):08x}',
   'args':[f'{v:08x}'for v in args],'provider_writes_only_raw_base_and_access':True,
   'Xbox_physical_hardware_address_and_protection_not_claimed':True}
  if allocate:
   assert args==[0x30E4000,0x61000,0x3144FFF,0,4],args
   result=args[1]|0x80000000
   row['storage']=CacheStorage.storage(self,result,args[0],'actual_source_requested_raw_physical_arena_with_preserved_cache_overlays')
   row['resulting_guest_alias']=f'{result:08x}';self.ret(n,result)
  else:
   assert args[2]==4,args
   ob=next(x for x in self.backing if x['alias']<=args[0] and args[0]+args[1]<=x['alias']+x['size'])
   ctypes.set_errno(0);result=self.libc.mprotect(ob['host_pointer']+args[0]-ob['alias'],args[1],3);err=ctypes.get_errno()
   assert result==0,(result,err,args);self.g.mem_protect(args[0],args[1],3)
   row['raw_provider_operation']={'api':'actual libc mprotect PROT_READ|PROT_WRITE','actual_result':result,'actual_errno':err}
   self.ret(n,self.g.reg_read(UC_X86_REG_EAX))
  self.raw_providers.append(row)
 def state(self):
  p=self.u(0x4EE9D8);crt=self.u(0x56B920);controller=self.u(0x52646C)
  return {'game_arena':f'{self.u(0x4ED650):08x}','game_arena_cursor':self.u(0x4ED654),
   'game_rng_pointer':f'{p:08x}','game_rng_8_hex':bytes(self.g.mem_read(p,8)).hex()if p else None,
   'CRT_fallback_pointer':f'{crt:08x}','CRT_fallback_88_hex':bytes(self.g.mem_read(crt,0x88)).hex()if crt else None,
   'original_process_heap':f'{self.u(0x566F08):08x}','source_heap_prefix_650_hex':bytes(self.g.mem_read(RAW_HEAP,0x650)).hex()if self.heap_reserved else None,
   'controller_root':f'{controller:08x}','controller_root_1c_hex':bytes(self.g.mem_read(controller,0x1C)).hex()if controller else None,
   'audio_pointer':f'{self.u(0x4ED950):08x}','audio_initialized_78':self.u(self.u(0x4ED950)+0x78,1)if self.u(0x4ED950)else None,
   'sound_system_20c_hex':bytes(self.g.mem_read(self.u(0x4ED950),0x20C)).hex()if self.u(0x4ED950)else None,
   'compatibility_KTHREAD_thread_id':self.u(self.context['ethread_address']+0x12C),
   'recorded_real_Wine_thread_id':self.context['native_thread_id']}
 def hook(self,g,a,size,user):
  if self.rng_enabled:
   if self.code.at(a):self.rng_executed.add(a)
   for ret,sp,row in self.rng_returns[:]:
    if a==ret and g.reg_read(UC_X86_REG_ESP)==sp:
     row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.rng_returns.remove((ret,sp,row))
   if a in ENTRIES or a in CLOCK_SERVICES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}','return_address':f'{self.u(sp):08x}',
     'registers':self.registers(),'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)]}
    self.rng_calls.append(row)
    # Entrypoints here use cdecl, except the actual void/clock providers.
    self.rng_returns.append((self.u(sp),sp+4,row))
   if a in CLOCK_SERVICES:self.clock_provider(a);return
   if a==0x800000B8:self.raw_heap_provider();return
   if a==0x33785C:self.cache_vm_provider();return
   if a in(self.u(0x417BA0),self.u(0x417BA8)):self.physical_provider(a);return
   if a==0x2205C0:
    self.new_invalid={'boundary':'unprovided original audio device driver constructor; no return supplied',
     'instruction':f'{a:08x}','state':self.state()};raise KernelBoundary()
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.rng_enabled:
   pc=g.reg_read(UC_X86_REG_EIP)
   if pc in(0x1479E6,0x1479EC,0x147A13,0x32C661,0x32C663,0x32C66A,0x32C676,0x32C67A,0x328519,0x21BCF6):
    self.rng_writes.append({'phase':self.active,'instruction':f'{pc:08x}','address':f'{address:08x}','size':size,
     'before_hex':bytes(g.mem_read(address,size)).hex(),'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def rng_action(self,name,a,args=(),regs=(),blocked=None):
  before=self.state();start=len(self.rng_calls);cl=len(self.clock_calls);w=len(self.rng_writes)
  self.call(name,a,args,regs,blocked)
  for ret,sp,row in self.rng_returns[:]:
   if self.g.reg_read(UC_X86_REG_EIP)==STOP and ret==STOP:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
   else:row['incomplete_or_non_cdecl_return_observation']=True
   self.rng_returns.remove((ret,sp,row))
  return {'name':name,'entry':f'{a:08x}','before':before,'after':self.state(),'original_call_site_explicit_fixture_input':True,
   'producer':self.producers[-1],'calls':self.rng_calls[start:],'clock_calls':self.clock_calls[cl:],'rng_writes':self.rng_writes[w:]}
 def arena_fragments(self):
  self.call('original_physical_arena_root',0x12CB70)
  self.active='source_game_arena_producer_excluding_savegame_io';self.g.reg_write(UC_X86_REG_ESP,STACK)
  self.g.emu_start(0x125280,0x1252BA,count=2000000);assert self.g.reg_read(UC_X86_REG_EIP)==0x1252BA
  self.g.emu_start(0x1252BF,STOP,count=2000000);assert self.g.reg_read(UC_X86_REG_EIP)==STOP
  self.producers.append({'name':self.active,'fragments':[['00125280','001252ba'],['001252bf','00125339']],
   'excluded_call':['001252ba','001252bf'],'excluded_callee':'00217090','original_frame_preserved':True,
   'ordinary_full_startup_allocation_order_not_claimed':True})
 def execute(self):
  rows=[];before=self.state();self.heap_prefix(None)
  rows.append({'name':'original_heap_writer','before':before,'after':self.state(),'producers':self.producers[:]})
  rows.append(self.rng_action('original_CRT_lock_initializer',0x32CFFC))
  rows.append(self.rng_action('original_CRT_onexit_initializer',0x3274E4))
  before=self.state();self.arena_fragments();rows.append({'name':'original_game_arena_fragments','before':before,'after':self.state()})
  rows.append(self.rng_action('original_callback_row3_rng_constructor',0x1479B0))
  # Only a supplied raw static arena pointer/capacity, identical protocol to
  # the previous startup slice but chosen outside the inherited pose resources.
  self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
  rows.append(self.rng_action('original_sound_constructor_through_controller_until_device',0x126B20,blocked=0x2205C0))
  if self.replay is not None:assert self.replay_cursor==len(self.replay)
  return rows

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
 try:t=RngTrace(data,replay);rows=t.execute()
 finally:
  if proc is not None:
   _,err=proc.communicate('x',timeout=60);(args.output.parent/'runtime-stderr.txt').write_text(err);assert proc.returncode==0,err
 (args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[]
 executed_path=directory/'executed-original-instructions.asm'
 executed_instructions=[t.code.at(a)for a in sorted(t.rng_executed)]
 for i in executed_instructions:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
 executed_path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in executed_instructions))
 for entry in sorted(set(ENTRIES)|set(CLOCK_SERVICES)):
  f=_trace(t.code,entry,t.starts|set(ENTRIES)|set(CLOCK_SERVICES));ins=[t.code.at(a)for a in sorted(f.code)]
  for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append({'entry':f'{entry:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins),
   'executed':sum(i.address in t.rng_executed for i in ins),'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'}for a,b,k in f.sites if k=='call']})
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 result={'source_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_sha256':digest(__file__),'frozen_pins':FROZEN,
  'context_source_executable_sha256':CONTEXT_EXE_PIN,'context_snapshot_sha256':digest(args.output.parent/'raw-host-context.json'),
  'context_mode':'whole_recorded_context_replay'if args.context_replay else'real_Wine_worker_alive_during_source_execution',
  'clock_mode':'verbatim_measured_input_replay'if args.replay else'real_host_clock_measurement','clock_calls':t.clock_calls,
  'raw_heap_provider_calls':t.raw_providers,'rows':rows,'rng_calls':t.rng_calls,'rng_writes':t.rng_writes,
  'retained_inherited_raw_physical_services':t.physical_services,'inherited_declared_services_used':t.events,
  'retained':t.state(),'source_producers':t.producers,'instruction_listings':listings,
  'executed_original_instruction_listing':{'file':executed_path.name,'sha256':digest(executed_path),'rows':len(executed_instructions)},
  'original_callback_table':{'base':'00447428','stride':36,'count':68,'raw_hex':t.image.read(0x447428,0x990).hex(),
   'rng_row_index':3,'rng_constructor_word':'00447494','rng_constructor_hex':t.image.read(0x447494,4).hex(),
   'audio_row_index':44},'whole_compatibility_context_transferred_no_pointer_or_selected_field_patch':True,
  'original_audio_driver_or_playback_result_supplied':False,'native_gameplay_update_applied':False,
  'boundaries':[
   'Complete Xbox/XAPI/title startup remains excluded. The original heap writer excludes shutdown/hardware setup2D7FA5..2D7FB0. Source fields/global566F08 are written only by original instructions.',
   'Actual mmap(PROT_NONE) and mprotect(RW) back the original requested reserve100000/commit1000. Guest08000000 is an explicit compatibility alias; NTSTATUS0 translates measured host success, not retail kernel behavior.',
   'Original32CFFC and3274E4 execute using frozen primary compatibility lock code and whole real host-thread context. Other runtime/TLS/C/C++ initializers remain outside the slice.',
   'Original game arena125280 prefix and1252BF tail run across excluded217090 savegame/thread-I/O call. The preserved original frame is not a fake callee return. Normal full startup allocation order remains unproved. The separate physical provider backs source-requested pages with actual host mmap while preserving previously supplied cache overlays; it does not prove original RAM layout.',
   'Original1479B0 is invoked directly with produced arena and real/replayed clock providers. Original126B20 is separately invoked with raw static audio arena0A000000/23000 and runs until the actual driver constructor2205C0 boundary; it receives no return there.',
   'Complete compatibility host branch produces TLSNULL; original32C609 chooses its fallback. This is not ordinary Xbox/title-thread initialization. Original cold rand seed and stores are not provided by the fixture.',
   'All earlier freezes and production are unchanged.']}
 args.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({'output':str(args.output),'clock_calls':len(t.clock_calls),'retained':{k:v for k,v in t.state().items()if 'hex'not in k}}))
if __name__=='__main__':main()
