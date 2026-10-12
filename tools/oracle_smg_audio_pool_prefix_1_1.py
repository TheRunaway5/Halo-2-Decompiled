#!/usr/bin/env python3
"""Execute original DirectSound static owners and raw pool consumers.

Anonymous host mappings supply only raw block storage and measured capacity.
Original constructors own statistics and device records. Kernel/device requests
outside that narrow provider stop before execution, without an audio success.
"""
import argparse,ctypes,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_ESI,UC_X86_REG_EDI
from oracle_smg_audio_device_prefix_1_1 import DeviceTrace,ROOT,PIN,CACHE_PIN,FROZEN as OLDER,ENTRIES as DEVICE_ENTRIES,CONTEXT_OUT,CONTEXT_EXE_PIN,digest
from oracle_smg_audio_rng_startup_1_1 import AUDIO_ARENA,AUDIO_CAPACITY
from oracle_mp_spawn_fixed_memory_world import FixedTrace
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_constructor_equipped_1_1 import _trace
from oracle_smg_firing_1_1 import STACK,STOP

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-pool-prefix'
FROZEN={**OLDER,
 'tools/oracle_smg_audio_device_prefix_1_1.py':'5313c91bd67a63aaf4aa3807dfd51fad01460e7dafa5503713cf107f17a81513',
 'tools/audit_smg_audio_device_prefix_1_1.py':'26853266f72db9e169b8d9f020fb1ee7d27661809ce5a8f4543e8f6983221f08',
 'docs/SMG_AUDIO_DEVICE_PREFIX_1_1_RESEARCH.md':'05a1bb4f32fdae06c5d38ea2948adfbb80b68318be3077aa4a04f1251ca74ec7',
 'analysis/multiplayer/map-pack/smg-audio-device-prefix/frozen/suite.json':'d81a75b09f83a72b41a1caad2fac4a4855525c1ea63c98fd0a31c344cb1ef348',
 'analysis/multiplayer/map-pack/smg-audio-device-prefix/source-fixture.json':'571503ddd26d2d0fa0afae23b0f5328964e3d460f7c8bef7eda36e29b8e6fa40',
 'analysis/multiplayer/map-pack/smg-audio-device-prefix/audit.json':'acf1b4a72afd6b37e30a080c27383bcdc816072fb56e9d7015c784d42acbbcf1'}
# Actual contiguous original C++ table entries, dispatched in their source order.
OWNERS=((0x468724,0x38410A,0x38D804,0x38D120),
 (0x468728,0x384115,0x38D800,0x38D124),
 (0x46872C,0x386EE8,0x38D8B0,0x38D128),
 (0x468730,0x386EF3,0x38D8AC,0x38D12C))
ENTRIES=tuple(dict.fromkeys((0x2D80FD,*(x[1]for x in OWNERS),*DEVICE_ENTRIES,
 0x383F13,0x383F2B,0x383F45,0x383EC7,0x380750,0x379FEA,0x2DD56C)))

class PoolTrace(DeviceTrace):
 def __init__(self,data,replay=None):
  self.pool_enabled=False;self.pool_blocks=[];self.pool_services=[];self.pool_stores=[]
  self.static_stores=[];self.owner_rows=[];self.pool_executed=set();self.last_pool_source=None
  self.next_pool=0xB000000
  super().__init__(data,replay);self.pool_enabled=True
 def statistics(self):
  return {'owners':{f'{a:08x}':f'{self.u(a):08x}'for _,_,a,_ in OWNERS},
   'counter_storage_address':'0038d120','counter_storage_10_hex':bytes(self.g.mem_read(0x38D120,16)).hex(),
   'private_pool_pointer':f'{self.u(0x38D74C):08x}',
   'effective_audio_flags_38d144':f'{self.u(0x38D144):08x}',
   'original_audio_flags_override_38d148':f'{self.u(0x38D148):08x}'}
 def initialize_statistics(self):
  for table,entry,owner,target in OWNERS:
   assert self.u(table)==entry and self.image.read(table,4)==entry.to_bytes(4,'little')
   before=self.statistics();start=len(self.static_stores)
   self.active=f'original_CXX_table_dispatch_{table:08x}'
   self.g.reg_write(UC_X86_REG_ESP,STACK)
   self.g.reg_write(UC_X86_REG_ESI,table);self.g.reg_write(UC_X86_REG_EDI,0x468CB8)
   self.g.emu_start(0x2D813E,0x2D814B,count=100000)
   assert self.g.reg_read(UC_X86_REG_EIP)==0x2D814B and self.u(owner)==target
   self.owner_rows.append({'table_address':f'{table:08x}','entry':f'{entry:08x}',
    'fragment':['002d813e','002d814b'],'stop_is_exclusive':True,
    'original_dispatcher_register_inputs':{'esi':f'{table:08x}','edi':'00468cb8','esp':f'{STACK:08x}'},
    'before':before,'after':self.statistics(),'stores':self.static_stores[start:],
    'full_CXX_table_or_cinit_executed':False})
 def execute(self):
  rows=[];before=self.state();self.heap_prefix(None)
  rows.append({'name':'original_heap_writer','before':before,'after':self.state(),'producers':self.producers[:]})
  rows.append(self.rng_action('original_CRT_lock_initializer',0x32CFFC))
  rows.append(self.rng_action('original_CRT_onexit_initializer',0x3274E4))
  self.initialize_statistics()
  before=self.state();self.arena_fragments();rows.append({'name':'original_game_arena_fragments','before':before,'after':self.state()})
  rows.append(self.rng_action('original_callback_row3_rng_constructor',0x1479B0))
  self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
  rows.append(self.rng_action('original_sound_constructor_through_controller_until_device',0x126B20,blocked=0x2205C0))
  return rows
 def pool_provider(self,a):
  sp=self.g.reg_read(UC_X86_REG_ESP);allocate=a==0x8000000F
  args=[self.u(sp+4+i*4)for i in range(2 if allocate else 1)]
  row={'phase':self.active,'kernel_target':f'{a:08x}','kernel_ordinal':a&0x7FFFFFFF,
   'original_call_site':f'{self.last_pool_source:08x}','original_return_site':f'{self.u(sp):08x}',
   'args':[f'{x:08x}'for x in args],'Xbox_pool_rounding_not_claimed':True,
   'provider_writes_no_source_object_counter_device_or_ready_field':True}
  if allocate:
   requested,tag=args;assert 0<requested<0x100000,requested
   page=os.sysconf('SC_PAGE_SIZE');assert page==4096
   amount=(requested+page-1)//page*page
   ctypes.set_errno(0);p=self.libc.mmap(None,amount,3,0x22,-1,0);err=ctypes.get_errno()
   assert p not in(None,ctypes.c_void_p(-1).value),(p,err)
   alias=self.next_pool;self.next_pool+=amount
   assert all(alias+amount<=lo or alias>hi for lo,hi,_ in self.g.mem_regions())
   self.g.mem_map_ptr(alias,amount,3,p);self.host_backings.append((p,amount))
   initial=ctypes.string_at(p,amount)
   block={'alias':alias,'capacity':amount,'requested_bytes':requested,'tag':tag,'host_pointer':p,
    'initial_raw_storage_sha256':hashlib.sha256(initial).hexdigest()}
   self.pool_blocks.append(block)
   row.update({'resulting_guest_alias':f'{alias:08x}','requested_bytes':requested,'raw_capacity_bytes':amount,
    'raw_initial_bytes_sha256':block['initial_raw_storage_sha256'],
    'raw_provider_operation':{'api':'actual libc mmap anonymous private PROT_READ|PROT_WRITE',
     'actual_host_address':f'{p:x}','actual_errno':err,'host_page_size':page},
    'allocation_tag_recorded_not_interpreted':f'{tag:08x}'})
   self.ret(2,alias)
  else:
   block=next(b for b in self.pool_blocks if b['alias']==args[0])
   row.update({'resulting_raw_capacity_bytes':block['capacity'],
    'capacity_owner':'actual live provider mapping associated with the exact returned base pointer'})
   self.ret(1,block['capacity'])
  self.pool_services.append(row)
 def kernel_request_origin(self,sp):
  ret=self.u(sp)
  callers=[self.code.at(ret-n)or self.ds_code.at(ret-n)for n in range(2,8)]
  callers=[i for i in callers if i and i.mnemonic=='call' and i.address+len(i.bytes)==ret]
  assert len(callers)==1,(ret,callers)
  i=callers[0]
  return {'original_call_site':f'{i.address:08x}','original_call_bytes':i.bytes.hex(),
   'original_return_site':f'{ret:08x}','last_original_instruction_before_kernel':f'{self.last_pool_source:08x}'}
 def hook(self,g,a,size,user):
  if self.pool_enabled:
   if self.code.at(a)or self.ds_code.at(a):
    self.pool_executed.add(a);self.last_pool_source=a
   if self.device_running and 0x80000000<=a<0x80001000:
    if a in(0x8000000F,0x80000017):self.pool_provider(a);return
    # These exact primary compatibility bodies/providers are already measured
    # and frozen; no new device kernel result is admitted through a generic hook.
    if a not in(0x80000115,0x80000126,0x80000123):
     sp=g.reg_read(UC_X86_REG_ESP)
     self.new_invalid={'boundary':'unprovided device kernel request; no success or capability supplied',
      'kernel_target':f'{a:08x}','kernel_ordinal':a&0x7FFFFFFF,
      **self.kernel_request_origin(sp),
      'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)],'registers':self.registers()}
     if a==0x80000018:
      self.new_invalid['unprovided_nonvolatile_query']={
       'ValueIndex':f'{self.u(sp+4):08x}','Type_pointer':f'{self.u(sp+8):08x}',
       'Type_initial_4_hex':bytes(g.mem_read(self.u(sp+8),4)).hex(),
       'Value_pointer':f'{self.u(sp+12):08x}',
       'Value_initial_4_hex':bytes(g.mem_read(self.u(sp+12),4)).hex(),
       'ValueLength':self.u(sp+16),'ResultLength_pointer':f'{self.u(sp+20):08x}',
       'source_nonvolatile_settings_value_or_return_supplied':False}
     raise KernelBoundary()
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.pool_enabled:
   pc=g.reg_read(UC_X86_REG_EIP)
   row={'phase':self.active,'instruction':f'{pc:08x}','address':f'{address:08x}',
    'size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()}
   if any(address==a for _,_,a,_ in OWNERS):self.static_stores.append(row)
   if self.device_running and (0x38D000<=address<0x38E000 or any(b['alias']<=address<b['alias']+b['capacity']for b in self.pool_blocks)):
    self.pool_stores.append(row)
  super().mem_write(g,access,address,size,value,user)
 def retained_blocks(self):
  return [{'guest_alias':f'{b["alias"]:08x}','requested_bytes':b['requested_bytes'],
   'raw_capacity_bytes':b['capacity'],'tag':f'{b["tag"]:08x}',
   'initial_raw_storage_sha256':b['initial_raw_storage_sha256'],
   'retained_raw_storage_hex':bytes(self.g.mem_read(b['alias'],b['capacity'])).hex()}for b in self.pool_blocks]

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
 try:
  t=PoolTrace(data,replay);preparation=t.execute();result=t.continue_device()
  if replay is not None:assert t.replay_cursor==len(replay)
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
   'executed':sum(i.address in t.pool_executed for i in ins),
   'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'}for a,b,k in f.sites if k=='call']})
 ins=[t.code.at(a)or t.ds_code.at(a)for a in sorted(t.pool_executed)]
 for i in ins:assert t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
 p=d/'executed-original-pool-slice.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
 for file,h in FROZEN.items():assert digest(ROOT/file)==h,file
 report={'source_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_sha256':digest(__file__),'frozen_pins':FROZEN,
  'context_snapshot_sha256':digest(context_path),'context_source_executable_sha256':CONTEXT_EXE_PIN,
  'context_mode':'whole_recorded_context_replay'if args.context_replay else'real_Wine_worker_alive_during_source_execution',
  'clock_mode':'verbatim_measured_input_replay'if args.replay else'real_host_clock_measurement',
  'clock_calls':t.clock_calls,'original_rng_preparation':preparation,'original_rng_writes':t.rng_writes,
  'original_static_initializer_table':{'C_begin':'00468cbc','C_end_exclusive':'00468cd4',
   'CXX_begin':'00468630','CXX_end_exclusive':'00468cb8','executed_table_first':'00468724',
   'executed_table_end_exclusive':'00468734','executed_table_raw_hex':t.image.read(0x468724,16).hex(),
   'counter_storage_initial_hex':t.image.read(0x38D120,16).hex()},
  'original_statistics_owners':t.owner_rows,'source_device_prefix':result,
  'raw_pool_provider_requests':t.pool_services,'source_pool_stores':t.pool_stores,
  'retained_original_statistics':t.statistics(),'retained_raw_pool_blocks':t.retained_blocks(),
  'instruction_listings':listings,'executed_original_pool_listing':{'file':p.name,'sha256':digest(p),'instructions':len(ins)},
  'new_raw_pool_storage_provider_used':True,'original_Xbox_pool_block_rounding_proven':False,
  'new_DirectSound_device_capability_hardware_or_consumption_result_supplied':False,
  'whole_original_DirectSound_or_audio_initialization_completed':False,'native_gameplay_update_applied':False,
  'boundaries':[
   'Original bounded C++ dispatcher bytes invoke four adjacent original statistics owners in ascending table order after original heap/CRT prerequisites and before game/audio construction. Other startup table entries and complete cinit are excluded, not asserted.',
   'Genuine host mmap owns raw pool storage. Guest pointers are declared aliases. ExQueryPoolBlockSize returns that actual provider mapping capacity; page granularity is host compatibility backing, not verified retail Xbox pool rounding.',
   'Provider returns raw pointer/capacity only. Original383ECC owns source statistic increments and whole returned-capacity clearing; original383F92 owns the block marker and client pointer; original device constructors own record fields.',
   'Full original126B20 frame resumes at2205C0. Source allocation/device constructors execute until the next unprovided kernel/device request, which is stopped before any result. No caps, ready/initialized flags, device success, packet completion or hardware register value is supplied.',
   'Frozen genuine clocks and whole primary Wine compatibility thread/KPCR/critical-section bodies remain distinct from ordinary Xbox title thread/hardware and full startup RNG history. Partial objects/lock/frame are retained without cleanup or unwind substitution.',
   'Independent repetition uses fresh source/real storage and verbatim measured clock/whole context input. Earlier freezes and production remain unchanged.']}
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'output':str(args.output),
  'dependency':result['first_unprovided_dependency'],'source_executed':len(ins),
  'provider_requests':len(t.pool_services),'blocks':len(t.pool_blocks),'statistics':t.statistics(),
  'initialized':t.device_state()['source_sound_initialized_78']}))
if __name__=='__main__':main()
