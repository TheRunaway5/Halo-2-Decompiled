#!/usr/bin/env python3
"""Independent real-clock startup prefixes; never fabricates Xbox audio success.

Clock values come from actual host UTC/boot clocks, or are replayed verbatim from
that measured provider log. Kernel TLS/IRQL, DirectSound devices, capabilities and
stream consumption remain unavailable. All prior frozen tools are unchanged.
"""
import argparse,hashlib,json,time,struct,subprocess,os
from pathlib import Path
from oracle_smg_audio_cache_1_1 import CacheTrace,ROOT,PIN,CACHE_PIN,AUDIO_ARENA,AUDIO_CAPACITY
from oracle_smg_fp_sound_1_1 import BoundaryReached
from oracle_smg_firing_1_1 import *
from oracle_smg_constructor_equipped_1_1 import _Code,_trace
from oracle_mp_spawn_biped_platform import segment
from unicorn.x86_const import UC_X86_REG_GDTR,UC_X86_REG_CS,UC_X86_REG_DS,UC_X86_REG_ES,UC_X86_REG_SS,UC_X86_REG_FS,UC_X86_REG_FS_BASE

CACHE_TOOL_PIN='9c546c136b0056a741b1d5c1c519cc476f4cbdaacd6c346bc3661d63fdf262a1'
OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-startup'
GDT=0x7500000  # Explicit emulator routing page; prior pose arena occupies6300000.
FILETIME_EPOCH_100NS=116444736000000000
CLOCK_SERVICES={0x336983:('GetSystemTimeAsFileTime',1),0x337A40:('GetTickCount',0)}
CONTEXT=ROOT/'analysis/multiplayer/integration/spawn-source/complete-compat-kpcr'
CONTEXT_EXE_PIN='4d8de99dd61203a801e256770edeba0af50ca05068203abdb8040faa0ed3967e'
ENTRIES=(0x126B20,0x126DE0,0x127540,0x128140,0x21BC50,0x32803E,0x328505,0x32C609,
 0x2205C0,0x383DD7,0x3804DE,0x192CA0,0x2215F0,0x2B4970,0x383B22,0x222B80,0x383AFE)
INSPECT=(0x139390,0x139410,0x139470,0x139540,0x1902E0,0x1904D0,0x190650,
 0x190690,0x190730,0x190A30,0x190B30,0x124040,0x19E070,0x192D30,
 0x32E813,0x2D8674,0x327290,0x381BEE,0x381C0A,0x383B46,0x383C0D,
 0x381C77,0x383B6A,0x38059B,0x383D6A,0x3802BF,0x380772,0x383C31,
 0x2215F0,0x220520,0x2B4970,0x383B22,0x381E58,0x222B80,0x223920,
 0x2204C0,0x2217E0,0x222840,0x2B4B30,0x383835,0x383799,0x220D10,
 0x2D7F8E,0x2DA83A,0x2D7969,0x2DAC5E)

class StartupTrace(CacheTrace):
 def __init__(self,replay=None,context=None):
  self.startup_enabled=False;self.startup_calls=[];self.startup_returns=[]
  self.startup_writes=[];self.startup_executed=set();self.clock_calls=[]
  self.replay=replay;self.replay_cursor=0;self.context=context;self.context_regions=[]
  super().__init__();self.startup_enabled=True
  if context is not None:
   for kind,ak,hk in(('complete_native_produced_KPCR','kpcr_address','kpcr_hex'),
    ('complete_native_produced_ETHREAD','ethread_address','ethread_hex'),
    ('real_native_host_stack_snapshot','host_backed_stack_address','host_backed_stack_hex'),
    ('source_BSS_KiUniqueProcess','source_static_process_address','source_static_process_hex')):
    address=context[ak];raw=bytes.fromhex(context[hk]);lo=address&~4095;hi=(address+len(raw)+4095)&~4095
    assert all(hi<=a or lo>b for a,b,_ in self.g.mem_regions()),(kind,hex(lo),hex(hi))
    self.g.mem_map(lo,hi-lo);self.g.mem_write(address,raw)
    self.context_regions.append({'kind':kind,'address':f'{address:08x}','bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()})
   assert all(GDT+0x1000<=a or GDT>b for a,b,_ in self.g.mem_regions())
   self.g.mem_map(GDT,0x1000);self.g.mem_write(GDT+8,segment(0,0x9B)+segment(0,0x93)+segment(context['kpcr_address'],0x93))
   self.g.reg_write(UC_X86_REG_GDTR,(0,GDT,0xFFF,0))
   for reg,v in((UC_X86_REG_CS,8),(UC_X86_REG_DS,16),(UC_X86_REG_ES,16),(UC_X86_REG_SS,16),(UC_X86_REG_FS,24)):self.g.reg_write(reg,v)
   assert self.g.reg_read(UC_X86_REG_FS_BASE)==context['kpcr_address']
   assert self.u(context['kpcr_address']+0x28)==context['ethread_address'] and self.u(context['ethread_address']+0x28)==0
 def clock_provider(self,a):
  name,pop=CLOCK_SERVICES[a];sp=self.g.reg_read(UC_X86_REG_ESP)
  args=[self.u(sp+4+4*i)for i in range(pop)]
  if self.replay is None:
   if a==0x336983:
    ns=time.time_ns();v=ns//100+FILETIME_EPOCH_100NS
    inp={'clock':'time.time_ns / CLOCK_REALTIME','observed_ns':ns,'filetime_100ns':v}
   else:
    ns=time.clock_gettime_ns(time.CLOCK_BOOTTIME);v=(ns//1000000)&0xFFFFFFFF
    inp={'clock':'clock_gettime_ns(CLOCK_BOOTTIME)','observed_ns':ns,'tick_ms_u32':v}
  else:
   r=self.replay[self.replay_cursor];self.replay_cursor+=1
   assert r['entry']==f'{a:08x}' and r['phase']==self.active and r['original_args']==[f'{x:08x}'for x in args]
   inp=r['measured_provider_input'];v=inp.get('filetime_100ns',inp.get('tick_ms_u32'))
  old=self.g.reg_read(UC_X86_REG_EAX)
  row={'phase':self.active,'entry':f'{a:08x}','provider':name,'original_args':[f'{x:08x}'for x in args],
   'original_return_address':f'{self.u(sp):08x}','measured_provider_input':inp,
   'provider_mode':'live_host_measurement'if self.replay is None else'replay_of_recorded_live_provider_input',
   'no_source_rng_or_ready_field_written_by_provider':True,'xbox_clock_epoch_granularity_not_claimed':True}
  if a==0x336983:
   row['output_before_hex']=bytes(self.g.mem_read(args[0],8)).hex()
   b=struct.pack('<Q',v);self.g.mem_write(args[0],b);row['output_after_hex']=b.hex()
   row['void_api_eax_preserved']=f'{old:08x}';self.ret(pop,old)
  else:row['return_eax']=f'{v:08x}';self.ret(pop,v)
  self.clock_calls.append(row)
 def startup_state(self):
  r={'audio':self.audio_state(),'controller_root_1c_hex':None,'driver_2ad8_hex':None,
   'arena_used':self.u(0x517FF4),'controller_root':f'{self.u(0x52646C):08x}',
   'driver_pointer':f'{self.u(0x526474):08x}','local_seed':f'{self.u(SEED+4):08x}',
   'rand_fallback_pointer':f'{self.u(0x56B920):08x}','tls_index':self.u(0x487620),
   'original_process_heap_pointer':f'{self.u(0x566F08):08x}',
   'original_options_mode':self.u(self.u(0x4EDF18)+8)}
  p=self.u(0x52646C)
  if p:r['controller_root_1c_hex']=bytes(self.g.mem_read(p,0x1C)).hex()
  p=self.u(0x526474)
  if p:r['driver_2ad8_hex']=bytes(self.g.mem_read(p,0x2AD8)).hex()
  if self.context is not None:
   r['retained_compatibility_context']={'KPCR_280_hex':bytes(self.g.mem_read(self.context['kpcr_address'],0x280)).hex(),
    'ETHREAD_140_hex':bytes(self.g.mem_read(self.context['ethread_address'],0x140)).hex()}
  return r
 def stop(self,a,dependency,details=None):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  row={'phase':self.active,'instruction':f'{a:08x}','dependency':dependency,
   'registers':self.registers(),'stack_64_hex':bytes(self.g.mem_read(sp,64)).hex(),
   'original_result_supplied':False,'startup_state':self.startup_state(),**(details or {})}
  self.boundaries.append(row);raise BoundaryReached(dependency)
 def hook(self,g,a,size,user):
  if self.startup_enabled:
   self.startup_executed.add(a)
   for ret,row in self.startup_returns[:]:
    if a==ret:row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.startup_returns.remove((ret,row))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}',
     'registers':self.registers(),'return_address':f'{self.u(sp):08x}',
     'stack':[f'{self.u(sp+4+4*i):08x}'for i in range(8)]}
    if a==0x383B22:
     p=self.u(sp+8);fmt=self.u(p+8)
     row.update(original_stream_descriptor_pointer=f'{p:08x}',original_stream_descriptor_18_hex=bytes(g.mem_read(p,0x18)).hex(),
      original_waveformat_pointer=f'{fmt:08x}',original_waveformat_14_hex=bytes(g.mem_read(fmt,0x14)).hex(),
      original_device_pointer=f'{self.u(sp+4):08x}',original_output_pointer=f'{self.u(sp+12):08x}')
    if a==0x383AFE:
     p=self.u(sp+8)
     row.update(original_buffer_descriptor_pointer=f'{p:08x}',original_buffer_descriptor_18_hex=bytes(g.mem_read(p,0x18)).hex(),
      original_device_pointer=f'{self.u(sp+4):08x}',original_output_pointer=f'{self.u(sp+12):08x}')
    self.startup_calls.append(row);self.startup_returns.append((self.u(sp),row))
   if a in CLOCK_SERVICES:self.clock_provider(a);return
   if self.context is None and a in (0x32C609,0x3804DE):
    self.stop(a,'unavailable original Xbox FS thread/IRQL context',
     {'first_unexecuted_instruction_hex':bytes(g.mem_read(a,8)).hex(),'first_context_operand':'byte ptr fs:[0x24]',
      'context_not_fabricated':True})
   if self.context is not None and a==0x3804F2:
    sp=g.reg_read(UC_X86_REG_ESP)
    self.stop(a,'unavailable original RtlEnterCriticalSection provider for DirectSound',
     {'import_slot':'00417be0','original_import_value':f'{self.u(0x417BE0):08x}',
      'original_critical_section_pointer':f'{self.u(sp):08x}',
      'original_critical_section_1c_hex':bytes(g.mem_read(self.u(sp),0x1C)).hex(),
      'no_critical_section_or_audio_service_return_supplied':True})
  super().hook(g,a,size,user)
 def invalid(self,g,access,address,size,value,user):
  if self.startup_enabled:
   self.stop(g.reg_read(UC_X86_REG_EIP),'unmapped original startup dependency',
    {'access':access,'address':f'{address:08x}','size':size,'context_not_fabricated':True})
  return super().invalid(g,access,address,size,value,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.startup_enabled and (AUDIO_ARENA<=address<AUDIO_ARENA+AUDIO_CAPACITY or
   0x526464<=address<0x526478 or 0x4ED944<=address<0x4ED954 or address==SEED+4):
   self.startup_writes.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def startup_action(self,name,entry,args=(),regs=()):
  before=self.startup_state();b=len(self.boundaries);cl=len(self.clock_calls)
  try:self.fp_action(name,entry,args,regs);completed=True
  except BoundaryReached:completed=False
  except UcError:
   # Unicorn reports the underlying memory fault after an invalid-memory
   # observer raises. Only the independently recorded dependency is accepted.
   assert len(self.boundaries)>b
   completed=False
  for pending in(self.startup_returns,self.audio_returns,self.adapter_returns,self.cache_returns):
   for ret,row in pending[:]:
    if completed and ret==STOP:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';pending.remove((ret,row))
    elif not completed:row['halted_at_dependency']=self.boundaries[-1]['dependency'];pending.remove((ret,row))
  return {'name':name,'entry':f'{entry:08x}','original_call_site_supplied':True,
   'stack_args':[f'{v:08x}'for v in args],'register_inputs':[{'register':r,'value':f'{v:08x}'}for r,v in regs],
   'before':before,'after':self.startup_state(),'completed':completed,
   'return_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}'if completed else None,
   'boundaries':self.boundaries[b:],'clock_calls':self.clock_calls[cl:]}
 def execute(self):
  self.fp_action('original_physical_arena_constructor_real_host_backing',0x12CB70)
  self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
  rows=[self.startup_action('full_original_sound_constructor_until_real_thread_context',0x126B20)]
  assert not rows[-1]['completed']
  if self.context is None:assert self.boundaries[-1]['instruction']=='0032c609'
  assert self.u(self.u(0x4ED950)+0x78,1)==0 and self.u(self.u(0x4ED950)+0x79,1)==1
  # An independent direct constructor probe, not a successful return for the
  # blocked controller in the full normal constructor above.
  rows.append(self.startup_action('direct_original_driver_constructor_until_thread_context',0x2205C0,(0x451220,)))
  assert not rows[-1]['completed']
  assert self.boundaries[-1]['instruction']==('003804de'if self.context is None else'003804f2')
  assert self.u(self.u(0x526474)+0x2AB0)==0
  rows.append(self.startup_action('direct_original_map_reset_with_real_clock_until_device_dependency',0x126DE0))
  assert not rows[-1]['completed']
  # The update kernel is isolated explicitly; unavailable admission is not
  # bypassed in an ordinary scheduler call.
  for n in range(6):rows.append(self.startup_action(f'direct_original_clock_kernel_{n}',0x128140))
  assert all(r['completed']for r in rows[3:9])
  for typ,channel in((0,0),(1,40),(2,81)):
   rows.append(self.startup_action(f'direct_unadmitted_original_stream_constructor_type_{typ}',0x2215F0,(channel,),((UC_X86_REG_EBX,typ),)))
   assert not rows[-1]['completed']
  rows.append(self.startup_action('direct_unadmitted_original_voice_constructor_index_0',0x222B80,regs=((UC_X86_REG_EAX,0),)))
  assert not rows[-1]['completed']
  assert self.u(self.u(0x4ED950)+0x78,1)==0
  if self.replay is not None:assert self.replay_cursor==len(self.replay)
  return rows

def dump(t,path):
 path.mkdir(parents=True,exist_ok=True);rows=[]
 codes={'.text':t.code,'DSOUND':_Code(t.image,'DSOUND')}
 for a in sorted(set(ENTRIES)|set(INSPECT)|set(CLOCK_SERVICES)):
  code=next((c for c in codes.values()if c.at(a)),None);assert code
  f=_trace(code,a,t.starts|set(ENTRIES)|set(INSPECT)|set(CLOCK_SERVICES));ins=[code.at(p)for p in sorted(f.code)]
  p=path/f'{a:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  rows.append({'entry':f'{a:08x}','path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
   'section':next(k for k,v in codes.items()if v is code),'instructions':len(ins),
   'executed':sum(i.address in t.startup_executed for i in ins),
   'host_provider_entry_instructions_substituted':a in CLOCK_SERVICES,
   'direct_calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
 p=path/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
 return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'working.json');ap.add_argument('--replay',type=Path)
 ap.add_argument('--instructions',type=Path);ap.add_argument('--compat-context',action='store_true');ap.add_argument('--context-replay',type=Path);args=ap.parse_args()
 args.output=args.output.resolve()
 assert not(args.compat_context and args.context_replay)
 assert hashlib.sha256((ROOT/'tools/oracle_smg_audio_cache_1_1.py').read_bytes()).hexdigest()==CACHE_TOOL_PIN
 replay=None if args.replay is None else json.loads(args.replay.read_text())['clock_calls']
 context=None;process=None
 if args.compat_context:
  assert hashlib.sha256((CONTEXT/'generator.exe').read_bytes()).hexdigest()==CONTEXT_EXE_PIN
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT/'wine-prefix'))
  process=subprocess.Popen(['wine',str(CONTEXT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
  line=process.stdout.readline();assert line;context=json.loads(line)
 elif args.context_replay:context=json.loads(args.context_replay.read_text())
 try:t=StartupTrace(replay,context);rows=t.execute()
 finally:
  if process is not None:
   stdout,stderr=process.communicate('x',timeout=60)
   args.output.parent.mkdir(parents=True,exist_ok=True);(args.output.parent/'compat-runtime-stderr.txt').write_text(stderr)
 if context is not None:
  p=args.output.parent/'raw-host-context.json';p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(context,indent=2)+'\n')
 r={'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cache_dependency_sha256':CACHE_TOOL_PIN,
  'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'steps':rows,'calls':t.startup_calls,'writes':t.startup_writes,
  'clock_calls':t.clock_calls,'storage_provider_calls':t.provider_calls,'boundaries':t.boundaries,'final':t.startup_state(),
  'provider_mode':'live_host_measurement'if replay is None else'replay_of_recorded_live_provider_input',
  'compatibility_context':None if context is None else{'producer_executable_sha256':CONTEXT_EXE_PIN,
   'producer_mode':'real_live_Wine32_worker_held_alive_during_consumers'if args.compat_context else'replay_of_recorded_whole_real_host_context',
   'snapshot_path':str(p.relative_to(ROOT)),'snapshot_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
   'entire_regions':t.context_regions,'no_pointer_relocation_or_selected_field_patch':True,
   'explicit_emulator_routing_GDT':{'address':f'{GDT:08x}','descriptors_18_hex':bytes(t.g.mem_read(GDT+8,24)).hex(),
    'selectors':{'CS':8,'DS_ES_SS':16,'FS':24},'base_matches_produced_KPCR':True,'Xbox_selectors_not_claimed':True},
   'host_branch_TlsData_NULL_is_compatibility_source_output':True,'ordinary_title_thread_or_Xbox_context_not_claimed':True},
  'source_controller_seed_initialized':False,'audio_available_forced':False,'source_audio_initialization_completed':False,
  'original_sound_device_capabilities_voices_streams_or_consumption_supplied':False,'native_gameplay_update_applied':False,
  'explicit_static_arena_backing_input':{'pointer':f'{AUDIO_ARENA:08x}','size':AUDIO_CAPACITY,'initial_used':0},
  'normal_map_request_and_dispatch_inspected_not_executed':True,
  'original_callback_table':{'base':'00447428','stride':36,'count':68,
   'raw_hex':t.image.read(0x447428,0x990).hex(),
   'sound_row_index':44,'sound_row_original_va':'00447a58','sound_row_24_hex':t.image.read(0x447A58,36).hex(),
   'class_row_index':43,'class_row_original_va':'00447a34','class_row_24_hex':t.image.read(0x447A34,36).hex()},
  'supplied_inherited_services':{f'{a:08x}':{'name':n,'return':v,'cleanup':p}for a,(n,v,p)in t.services.items()},
  'scope':'Original startup/map-reset/clock prefixes with real host storage and recorded real clocks. Direct driver/map-reset/clock-kernel call sites are explicit fixture inputs. Genuine FS and device/effect context dependencies stop; no ready flag, RNG initialization, device result or availability is fabricated.'}
 if args.instructions:r['instruction_inventory']=dump(t,args.instructions.resolve())
 args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(r,indent=2)+'\n')
 print(json.dumps({'output':str(args.output),'clock_calls':len(t.clock_calls),'boundaries':[b['instruction']for b in t.boundaries]}))
if __name__=='__main__':main()
