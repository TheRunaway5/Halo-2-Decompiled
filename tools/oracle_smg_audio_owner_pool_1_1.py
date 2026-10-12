#!/usr/bin/env python3
"""Original game allocator/owner pool/owning removal with real raw services.

Complete game-arena preparation preserves the original frame across the sole
unavailable savegame-I/O call. Device/clock/map tails remain explicit boundaries.
Prior request bytes are supplied to the unchanged source-creation kernel, not
admitted as successful ordinary playback.
"""
import argparse, hashlib, json, os, subprocess
from pathlib import Path
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_smg_audio_rng_startup_1_1 import RngTrace, CONTEXT_OUT, CONTEXT_EXE_PIN, ROOT, PIN, CACHE_PIN, digest
from oracle_smg_audio_cache_1_1 import CacheTrace as CacheStorage, SERVICES as CACHE_SERVICES
from oracle_smg_audio_adapter_1_1 import SliceEnd
from oracle_smg_audio_ownership_1_1 import FROZEN as OLDER
from oracle_smg_audio_startup_1_1 import CLOCK_SERVICES
from oracle_smg_firing_1_1 import STACK, STOP, SEED
from oracle_smg_constructor_equipped_1_1 import _trace

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-owner-pool'
PREVIOUS=ROOT/'analysis/multiplayer/map-pack/smg-audio-host-contract'
REQUEST_ARENA=0xB100000
FROZEN={**OLDER,
 'tools/oracle_smg_audio_ownership_1_1.py':'dc006540ccf04c6a66b6793ba3faf36de107ac3f3e2544a38be16a23a2afc36f',
 'tools/inspect_smg_audio_host_contract_1_1.py':'edf8d5a044f5ee0ae2410e8873cc493b26e9ef31e71b50c938e5b4a7649d9a38',
 'tools/audit_smg_audio_host_contract_1_1.py':'68fbe7a732490a42f6db65e33a0bd2494c1e2976c415b5469f7fb18299ef770e',
 'tools/probe_sdl_audio_capabilities.py':'2079364cf9548e37eed2937e01a22a8c190ffd3bf283c50f669310a122654ae9',
 'docs/SMG_AUDIO_HOST_CONTRACT_1_1_RESEARCH.md':'de42203a9c9f7ae49b64b69e6bd819f37f0b88eaa3ec36bea6161779e93e25cf',
 'analysis/multiplayer/map-pack/smg-audio-host-contract/frozen/suite.json':'a0c186747c1d9cf5e6dddbb2b8e75c08de4a08c7274e307276599fb0ab172f78',
 'analysis/multiplayer/map-pack/smg-audio-host-contract/source-fixture.json':'2c929a8f8396e1d9009d5d4973a0537ad5358773043a249b09ab32bc6c082c28',
 'analysis/multiplayer/map-pack/smg-audio-host-contract/audit.json':'78ee514b2683804b61923dfefa9c89deb7f6a3c4bdb53213ca6deb1aad68f945',
}
ENTRIES=(0x125280,0x125E50,0x1656A0,0x18A200,0x18A2A0,0x18A790,0x16D070,
 0x16D220,0x128A70,0x128AE0,0x1280B0,0x18E2F0,0xBBCB0,0xD2270,0x16D4C0,
 0x2230E0,0x223960,0x2239B0,0x126DE0,0x1221B0,0x127750,0x129450,0x21A960)
HOST_SERVICES=set(CACHE_SERVICES)-{0x33785C,0x2D79AA,0x2D7A1A}

class OwnerBoundary(Exception):pass

class OwnerTrace(RngTrace):
 def __init__(self,data,replay=None):
  self.owner_enabled=False;self.owner_calls=[];self.owner_returns=[];self.owner_stores=[];self.owner_executed=set()
  self.provider_calls=[];self.live_objects={};self.provider_next_handle=0x77000001
  self.cache_calls=[];self.cache_returns=[];self.cache_writes=[];self.source_pointer=None;self.chunk_pointer=None
  self.created_sources=[];self.worker_wait_count=None;self.owner_boundaries=[];self.steps=[]
  super().__init__(data,replay);self.owner_enabled=True
 provider_object=CacheStorage.provider_object
 provide=CacheStorage.provide
 cache_state=CacheStorage.cache_state
 worker_once=CacheStorage.worker_once
 def fp_action(self,name,entry,args=(),regs=()):
  self.active=name;self.run(entry,args,regs)
 def owner_state(self):
  allocator=self.u(0x51841C);pool=self.u(0x4F485C);state=self.u(0x4F4858)
  header_pool=self.u(0x4E78D0);owner=None
  if self.source_pointer:
   datum=self.u(self.source_pointer+0x10);index=datum&0xFFFF
   owner={'datum':f'{datum:08x}','object_header_pool':f'{header_pool:08x}',
    'object_header_pool_4c_hex':bytes(self.g.mem_read(header_pool,0x4C)).hex(),
    'high_water':self.u(header_pool+0x38),'stride':self.u(header_pool+0x24)}
   if index<self.u(header_pool+0x38):
    p=self.u(header_pool+0x44)+index*self.u(header_pool+0x24)
    owner.update(selected_header_pointer=f'{p:08x}',selected_header_0c_hex=bytes(self.g.mem_read(p,12)).hex())
  return {'game_arena':f'{self.u(0x4ED650):08x}','game_arena_used':self.u(0x4ED654),
   'game_arena_checksum':f'{self.u(0x4ED65C):08x}','game_arena_initialized':self.u(0x4EB130,1),
   'allocator_pointer':f'{allocator:08x}','allocator_4_hex':bytes(self.g.mem_read(allocator,4)).hex()if allocator else None,
   'owner_pool_pointer':f'{pool:08x}','owner_pool_60cc_hex':bytes(self.g.mem_read(pool,0x60CC)).hex()if pool else None,
   'owner_state_pointer':f'{state:08x}','owner_state_244_hex':bytes(self.g.mem_read(state,0x244)).hex()if state else None,
   'source_cache':self.cache_state()if self.u(0x5098DC)else None,
   'source_owned_rng_pointer':f'{self.u(0x4EE9D8):08x}',
   'source_owned_rng_8_hex':bytes(self.g.mem_read(self.u(0x4EE9D8),8)).hex()if self.u(0x4EE9D8)else None,
   'driver_pointer':f'{self.u(0x526474):08x}',
   'audio_initialized':self.u(self.u(0x4ED950)+0x78,1)if self.u(0x4ED950)else None,
   'owner_object_lookup_input':owner,
   'owner_lifecycle_callback_row_36_hex':self.image.read(0x447AA0,36).hex(),
   'original_source_callback_table_hex':self.image.read(0x44B14C,32).hex(),
   'original_source_owner_table_hex':self.image.read(0x447A9C,24).hex()}
 def hook(self,g,a,size,user):
  if self.owner_enabled:
   for ret,row in self.owner_returns[:]:
    if a==ret:row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';self.owner_returns.remove((ret,row))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}','return_site':f'{self.u(sp):08x}',
     'registers':self.registers(),'stack':[f'{self.u(sp+4+4*i):08x}'for i in range(6)]}
    self.owner_calls.append(row);self.owner_returns.append((self.u(sp),row))
    if a==0x21A960:self.cache_calls.append(row)
   if a in (0x22310C,0x2217E8,0x2D8606):
    self.owner_boundaries.append({'phase':self.active,'instruction':f'{a:08x}',
     'instruction_bytes':self.image.read(a,6 if a==0x22310C else 8).hex(),
     'dependency':('absent source driver526474 at gain store+2ABC'if a==0x22310C else
       'absent source driver526474 at update store+2A0C'if a==0x2217E8 else'original async CreateThread unavailable'),
     'registers':self.registers(),'stack':[f'{self.u(g.reg_read(UC_X86_REG_ESP)+4+4*i):08x}'for i in range(6)],
     'source_return_device_owner_validity_or_ready_result_supplied':False})
    assert a==0x2D8606 or self.u(0x526474)==0
    raise OwnerBoundary()
   if a in HOST_SERVICES:
    self.provide(a);return
   # Entrypoint observation is not execution of a replaced service instruction.
   if a!=STOP and a!=0x2205C0 and a not in CLOCK_SERVICES and a!=0x33785C and a not in self.services and self.code.at(a):
    self.owner_executed.add(a)
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.owner_enabled:
   watch=address in(0x51841C,0x4F4858,0x4F485C,0x4ED650,0x4ED654,0x4ED65C,0x4EB130)
   pool=self.u(0x4F485C);state=self.u(0x4F4858);allocator=self.u(0x51841C)
   watch|=bool(pool and pool<=address<pool+0x60CC)or bool(state and state<=address<state+0x244)
   watch|=bool(allocator and address==allocator)
   # Capture the original pool construction before its pointer is published.
   watch|=self.active.startswith('original_owner')and not(0x02100000<=address<0x02200000)
   watch|=self.active.startswith('original_full_source_remove')
   watch|=self.active=='direct_original_async_worker_one_iteration'and(0x4E7900<=address<=0x4E9F00 or 0x5098DC<=address<0x509900)
   watch|=g.reg_read(UC_X86_REG_EIP)==0x1223AC
   if watch:self.owner_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def owner_action(self,name,entry,args=(),regs=(),blocked=False):
  before=self.owner_state();first=len(self.owner_boundaries);start=len(self.owner_stores)
  try:self.fp_action(name,entry,args,regs);completed=True
  except OwnerBoundary:completed=False
  assert completed!=blocked,(name,completed)
  for ret,row in self.owner_returns[:]:
   if completed and ret==STOP:row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
   else:row['call_not_returned_at_slice_boundary']=True
   self.owner_returns.remove((ret,row))
  result={'name':name,'entry':f'{entry:08x}','args':[f'{v:08x}'for v in args],
   'supplied_registers':[{'register':r,'value':f'{v:08x}'}for r,v in regs],
   'before':before,'after':self.owner_state(),'completed':completed,
   'boundary':self.owner_boundaries[first:],'stores':self.owner_stores[start:]}
  self.steps.append(result);return result
 def prepare(self):
  # The frozen startup consumes four clock inputs. The fifth belongs to this
  # independent extension; do not change the frozen assertion or source values.
  replay=self.replay
  if replay is not None:self.replay=replay[:4]
  preparation=self.execute()
  self.replay=replay
  assert self.u(0x51841C) and self.u(self.u(0x51841C))==0x45A610
  # A genuine host clock services the original map reset; its first unavailable
  # driver write is retained rather than supplied as a success/return.
  self.owner_action('original_sound_map_reset_real_clock_until_absent_driver',0x126DE0,blocked=True)
  assert self.u(0x526464)
  self.owner_action('original_class_fade_constructor_actual_game_arena',0x223960)
  self.owner_action('original_class_fade_map_reset',0x2239B0)
  self.owner_action('original_async_constructor_until_unavailable_thread',0x1221B0,blocked=True)
  shared=ROOT/'assets/disc/maps/shared.map';assert digest(shared)=='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
  handle=self.provider_object('file',shared.open('rb'),{'path':str(shared.relative_to(ROOT))});self.w(0x55F798,handle)
  self.owner_action('original_owner_pool_constructor',0x18A200)
  self.owner_action('original_owner_map_reset_until_absent_driver',0x18A2A0,blocked=True)
  return preparation,{'slot':'0055f798','real_provider_handle':f'{handle:08x}',
   'source_file_registry_owner_unexecuted':True,'file':str(shared.relative_to(ROOT)),'sha256':digest(shared)}
 def case(self,scenario,request):
  preparation,file_binding=self.prepare()
  self.g.mem_map(REQUEST_ARENA,0x1000)
  raw=bytes.fromhex(request['play_state_before_hex']);self.g.mem_write(REQUEST_ARENA,raw)
  self.fp_action('direct_original_listener_from_frozen_actual_request',0x129450,
    (REQUEST_ARENA+0x0C,0x3F800000,REQUEST_ARENA+0xA0))
  listener=self.g.reg_read(UC_X86_REG_EAX)
  self.fp_action('direct_original_source_creation_from_frozen_actual_rejected_FP_request',0x127750,
    (int(request['stack'][0],16),listener,REQUEST_ARENA,0xFFFFFFFF))
  datum=self.g.reg_read(UC_X86_REG_EAX);assert datum!=0xFFFFFFFF
  self.source_pointer=self.u(self.u(0x4ED94C)+0x44)+(datum&0xFFFF)*0xBC
  self.chunk_pointer=int(self.cache_calls[-1]['stack'][1],16)
  self.created_sources.append({'datum':f'{datum:08x}','pointer':f'{self.source_pointer:08x}'})
  creation={'actual_rejected_fp_request':request,'copied_actual_request_hex':raw.hex(),
   'request_raw_storage_alias':f'{REQUEST_ARENA:08x}',
   'created_original_source_datum':f'{datum:08x}','created_source_pointer':f'{self.source_pointer:08x}',
   'created_source_bc_hex':bytes(self.g.mem_read(self.source_pointer,0xBC)).hex(),
   'original_listener':f'{listener:08x}','creation_kernel_call_site_supplied':True,
   'ordinary_available_admission_or_playback_not_claimed':True}
  if scenario=='reload':self.worker_once()
  before=self.owner_state();source=int(creation['created_original_source_datum'],16)
  row=self.owner_action('original_full_source_remove_reason3',0x128A70,(3,),((UC_X86_REG_EAX,source),))
  after=self.owner_state();completion=None
  if scenario=='ready':
   start=len(self.owner_stores);self.worker_once()
   completion={'entry':'00122360','one_worker_iteration_call_site_supplied':True,
    'before':after,'after':self.owner_state(),'stores':self.owner_stores[start:]}
  return {'scenario':scenario,'preparation':preparation,'file_binding':file_binding,'creation':creation,
   'steps':self.steps,'cleanup':row,'before_cleanup':before,'after_cleanup':after,
   'pending_read_after_full_cleanup':completion,'retained':self.owner_state(),
   'owner_calls':self.owner_calls,'owner_stores':self.owner_stores,'boundaries':self.owner_boundaries,
   'clock_calls':self.clock_calls,'raw_providers':self.raw_providers,'cache_provider_calls':self.provider_calls,
   'inherited_supplied_services':{f'{a:08x}':str(v)for a,v in self.services.items()},
   'source_allocator_pool_validity_driver_voice_or_callback_fields_supplied':False,
   'complete_owner_map_or_audio_playback_proven':False,'native_gameplay_update_applied':False}

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--replay',type=Path);ap.add_argument('--context-replay',type=Path);args=ap.parse_args()
 for path,pin in FROZEN.items():assert digest(ROOT/path)==pin,path
 previous=json.loads((PREVIOUS/'source-fixture.json').read_text())
 requests={row['scenario']:row['creation']['actual_rejected_fp_request']for row in previous['independently_reexecuted_original_ownership_cases']}
 replay=None if args.replay is None else json.loads(args.replay.read_text())
 proc=None
 if args.context_replay:data=json.loads(args.context_replay.read_text())
 else:
  assert digest(CONTEXT_OUT/'generator.exe')==CONTEXT_EXE_PIN
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
  proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
  data=json.loads(proc.stdout.readline())
 args.output.parent.mkdir(parents=True,exist_ok=True);rows=[];traces=[]
 try:
  for i,scenario in enumerate(('ready','reload')):
   t=OwnerTrace(data,None if replay is None else replay['cases'][i]['clock_calls'])
   try:row=t.case(scenario,requests[scenario])
   except Exception:
    print(json.dumps({'source_dependency':t.new_invalid,'recent':[f'{a:08x}'for a in t.recent],'phase':t.active}),flush=True)
    raise
   rows.append(row);traces.append(t)
   if replay is not None:assert t.replay_cursor==len(replay['cases'][i]['clock_calls'])
 finally:
  if proc is not None:
   _,err=proc.communicate('x',timeout=60);(args.output.parent/'runtime-stderr.txt').write_text(err);assert proc.returncode==0,err
 context_path=args.output.parent/'raw-host-context.json';context_path.write_text(json.dumps(data,indent=2)+'\n')
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[];t=traces[0]
 executed=set().union(*(t.owner_executed for t in traces));executed_ins=[t.code.at(a)for a in sorted(executed)]
 for z in executed_ins:
  assert t.image.read(z.address,len(z.bytes))==z.bytes
  assert all(bytes(v.g.mem_read(z.address,len(z.bytes)))==z.bytes for v in traces)
 executed_path=directory/'executed-original-instructions.asm'
 executed_path.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in executed_ins))
 for entry in ENTRIES:
  f=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(f.code)]
  for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes
  p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
  listings.append({'entry':f'{entry:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins),
   'calls':[{'site':f'{a:08x}','target':f'{b:08x}','kind':k}for a,b,k in f.sites]})
 for prior,pin in FROZEN.items():assert digest(ROOT/prior)==pin,prior
 report={'oracle_sha256':digest(__file__),'source_sha256':PIN,'cache_sha256':CACHE_PIN,'frozen_prior_pins':FROZEN,
  'context_snapshot_sha256':digest(context_path),'context_mode':'whole_recorded_context_replay'if args.context_replay else'real_Wine_worker_alive_during_source_execution',
  'clock_mode':'verbatim_measured_input_replay'if replay is not None else'real_host_clock_measurement',
  'cases':rows,'instruction_listings':listings,
  'executed_original_listing':{'file':executed_path.name,'sha256':digest(executed_path),'instructions':len(executed_ins),
   'replaced_service_entries_and_unexecuted_dependency_stop_instructions_excluded':True},
  'native_gameplay_update_applied':False}
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'output':str(args.output),'cases':len(rows),
  'cleanup_returns':sum(row['cleanup']['completed']for row in rows),'prior_pins':len(FROZEN)}))
if __name__=='__main__':main()
