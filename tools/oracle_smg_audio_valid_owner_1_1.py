#!/usr/bin/env python3
"""Independent original join/loadout and unvoiced sound-owner lifecycle.

Previous audio/world fixtures are immutable. Original storage, join, loadout,
request producer and removal run with real raw services. No object header,
identity, owner, driver, voice or successful admission field is supplied.
"""
import argparse,json,os,subprocess
from pathlib import Path
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_smg_audio_owner_pool_1_1 import OwnerTrace,OwnerBoundary,ROOT,PIN,CACHE_PIN,digest,FROZEN as OLDER,REQUEST_ARENA,ENTRIES as OWNER_ENTRIES
from oracle_smg_audio_rng_startup_1_1 import RngTrace,AUDIO_ARENA,AUDIO_CAPACITY,CONTEXT_OUT,CONTEXT_EXE_PIN
from oracle_mp_spawn_player_visibility import PlayerTrace,prepare,spawn_and_activate,FROZEN as WORLD_PINS
from oracle_mp_spawn_live_body_world import LiveTrace
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STOP
from functions import _trace

OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-valid-owner'
FROZEN={**OLDER,**WORLD_PINS,
 'tools/oracle_smg_audio_owner_pool_1_1.py':'ac4aadc15659f4dbddb756f7973f9cd0f3e83b47a4b30ee0d2479bb382b9d380',
 'tools/audit_smg_audio_owner_pool_1_1.py':'fc44b4613023c2a9a71f5703cee1931ca125fe30824f957c01853842bb0e034a',
 'docs/SMG_AUDIO_OWNER_POOL_1_1_RESEARCH.md':'7f5a061b148612e7e9c8a2abcb36761ef40a0d3d212734d02a06f07d8b8448bd',
 'analysis/multiplayer/map-pack/smg-audio-owner-pool/frozen/suite.json':'3e2985acef8fce1958d64dbd7b8ea42d4cb1f82fae011af6818bd39e66523f39',
 'analysis/multiplayer/map-pack/smg-audio-owner-pool/source-fixture.json':'fb96df417f7c5c50e2e0d91d5c99984fba8f7570d5525a590684ee21b575d1f0',
 'analysis/multiplayer/map-pack/smg-audio-owner-pool/audit.json':'ded68ee99655c92b286db0f4b90cd215eb09a3f646d4e9e0be02dd48b1c6d73d',
 'tools/oracle_mp_spawn_player_visibility.py':'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json':'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd'}
REQUEST_ENTRIES=(0x18AA90,0x18AA40,0x18ACD0,0x18AD70,0x18EF20,0x18DDE0,0x18BA10,0x21F220)
ENTRIES=tuple(dict.fromkeys(OWNER_ENTRIES+REQUEST_ENTRIES+(0x1687C2,0x14D350,0x1509C0,0x1A28A0,0xB8A30,0x16D2C0,0x1874D0,0x156A30,0x156AD0)))

class ValidOwnerTrace(OwnerTrace,PlayerTrace):
 def __init__(self,data,replay=None):
  self.request_calls=[];self.request_returns=[];self.request_stores=[];self.lifecycle_stores=[];self.new_steps=[]
  self.request_enabled=False;self.initial_rng=None;self.owner_callback_instructions=[]
  super().__init__(data,replay);self.request_enabled=True
 def arena_root(self):
  LiveTrace.arena_root(self)
  self.initial_rng=self.rng_action('source_random_storage_record3_before_player_storage',0x1479B0)
 def state(self):
  result=RngTrace.state(self)
  if getattr(self,'track_players',False):result['source_world']=LiveTrace.state(self)
  return result
 def source_state(self):
  op=self.u(0x4E78D0);p=self.u(op+0x44);cap=self.u(op+0x20);stride=self.u(op+0x24)
  result={'object_pool_pointer':f'{op:08x}','object_pool_4c_hex':bytes(self.g.mem_read(op,0x4C)).hex(),
   'object_pool_capacity':cap,'object_pool_stride':stride,'object_pool_data_hex':bytes(self.g.mem_read(p,cap*stride)).hex(),
   'object_pool_bitmap_hex':bytes(self.g.mem_read(self.u(op+0x48),(cap+31)//32*4)).hex(),
   'world':LiveTrace.state(self),'player':self.player_state(),'audio':self.owner_state()}
  sp=self.u(0x4ED94C)
  if sp:result['complete_source_pool_data_hex']=bytes(self.g.mem_read(self.u(sp+0x44),self.u(sp+0x20)*self.u(sp+0x24))).hex()
  return result
 def finish_requests(self,a):
  for ret,row in self.request_returns[:]:
   if ret==a:
    row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
    for key,n in(('request_pointer',0x58),('play_state_pointer',0xA0),('location_pointer',0x28)):
     if key in row:row[key.replace('_pointer','_after_hex')]=bytes(self.g.mem_read(int(row[key],16),n)).hex()
    self.request_returns.remove((ret,row))
 def fp_action(self,name,entry,args=(),regs=()):
  super().fp_action(name,entry,args,regs)
  self.finish_requests(STOP)
 def hook(self,g,a,size,user):
  if self.request_enabled:
   if self.active=='original_full_source_remove_reason3'and(0x18E2F0<=a<0x18E335 or 0xBBCB0<=a<0xBBD0B or 0xD2270<=a<0xD22A4):
    self.owner_callback_instructions.append(f'{a:08x}')
   self.finish_requests(a)
   if a in REQUEST_ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);row={'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
     'return_site':f'{self.u(sp):08x}','stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)]}
    if a==0x18AD70:
     p=self.u(sp+12);row['location_pointer']=f'{p:08x}';row['location_before_hex']=bytes(g.mem_read(p,0x28)).hex()
    if a==0x18DDE0:
     p=self.u(sp+16);row['request_pointer']=f'{p:08x}';row['request_before_hex']=bytes(g.mem_read(p,0x58)).hex()
    if a==0x18BA10:
     p=g.reg_read(UC_X86_REG_EAX);row['request_pointer']=f'{p:08x}';row['request_before_hex']=bytes(g.mem_read(p,0x58)).hex()
    if a==0x21F220:
     p=g.reg_read(UC_X86_REG_EAX);row['play_state_pointer']=f'{p:08x}';row['play_state_before_hex']=bytes(g.mem_read(p,0xA0)).hex()
    self.request_calls.append(row);self.request_returns.append((self.u(sp),row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.request_enabled and self.active.startswith(('source_sound_constructor','original_sound','original_class','original_async','original_owner','direct_original_')):
   self.lifecycle_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  if self.request_enabled and ('original_sound_request'in self.active or 'original_full_source_remove'in self.active):
   self.request_stores.append({'phase':self.active,'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
    'address':f'{address:08x}','size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
  super().mem_write(g,access,address,size,value,user)
 def audio_setup(self):
  assert all(AUDIO_ARENA+AUDIO_CAPACITY<=a or AUDIO_ARENA>b for a,b,_ in self.g.mem_regions())
  self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY);self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
  self.new_steps.append(self.rng_action('source_sound_constructor_after_real_world_until_device',0x126B20,blocked=0x2205C0))
  self.owner_action('original_sound_map_reset_real_clock_until_absent_driver',0x126DE0,blocked=True)
  self.owner_action('original_class_fade_constructor_actual_game_arena',0x223960)
  self.owner_action('original_class_fade_map_reset',0x2239B0)
  self.owner_action('original_async_constructor_until_unavailable_thread',0x1221B0,blocked=True)
  shared=ROOT/'assets/disc/maps/shared.map'
  assert digest(shared)=='084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af'
  handle=self.provider_object('file',shared.open('rb'),{'path':str(shared.relative_to(ROOT))});self.w(0x55F798,handle)
  self.file_binding={'slot':'0055f798','actual_provider_handle':f'{handle:08x}','source_registry_owner_unexecuted':True,
   'verified_file':str(shared.relative_to(ROOT)),'sha256':digest(shared)}
  self.owner_action('original_owner_pool_constructor',0x18A200)
  self.owner_action('original_owner_map_reset_until_absent_driver',0x18A2A0,blocked=True)
 def valid_case(self,scenario):
  assert spawn_and_activate(self,activate=False)
  pp=self.u(0x4F01F4);player=self.u(pp+0x44)+(self.spawned_player&0xFFFF)*self.u(pp+0x24)
  unit=self.u(player+0x2C);op=self.u(0x4E78D0);heads=self.u(op+0x44)
  up=self.u(heads+(unit&0xFFFF)*12+8);weapon=self.u(up+0x218)
  wp=self.u(heads+(weapon&0xFFFF)*12+8)
  assert weapon!=0xFFFFFFFF and self.u(wp+0x154)==unit
  allocated=self.source_state();self.audio_setup()
  # Source-produced graph/event datum is a bounded callback input, not a
  # claimed live FP clock event. Original18AA90 creates the owner/marker request.
  sound={'ready':0xE5AC0437,'reload':0xE5AD0438}[scenario]
  graph=self.cache.tags[0xE5AA0435];sound_tag=self.cache.tags[sound]
  assert graph['group']=='jmad'and sound_tag['group']=='snd!'
  refs=[p for p in self.cache.block(graph['offset']+0x14,12)if self.cache.u(p+4)==sound]
  assert len(refs)==1
  callback_input={'FP_graph':graph,'reference_file_offset':refs[0],
   'reference_0c_hex':bytes(self.cache.data[refs[0]:refs[0]+12]).hex(),
   'sound_tag':sound_tag,'sound_definition_14_hex':bytes(self.cache.data[sound_tag['offset']:sound_tag['offset']+20]).hex(),
   'callback_call_site_and_gain_explicit_inputs':True,'ordinary_FP_clock_not_executed':True}
  for slot in(0x46FC28,0x46FC48):assert bytes(self.g.mem_read(slot,4))==self.image.read(slot,4)
  caller_inputs={'original_168802_EAX_slot':'0046fc48','EAX_pointer':f'{self.u(0x46FC48):08x}',
   'original_168810_ECX_slot':'0046fc28','ECX_pointer':f'{self.u(0x46FC28):08x}',
   'position_source_hex':self.image.read(self.u(0x46FC28),12).hex(),'forward_source_hex':self.image.read(self.u(0x46FC48),12).hex()}
  self.fp_action('original_sound_request_from_authored_callback_and_source_allocated_weapon',0x18AA90,
   (weapon,0xFFFFFFFF,0x3F800000),((UC_X86_REG_EBX,sound),(UC_X86_REG_EAX,self.u(0x46FC48)),(UC_X86_REG_ECX,self.u(0x46FC28))))
  assert self.g.reg_read(UC_X86_REG_EAX)==0xFFFFFFFF
  request=[z for z in self.request_calls if z['entry']=='0021f220'][-1]
  assert request['return_eax']=='ffffffff'
  self.g.mem_map(REQUEST_ARENA,0x1000);b=bytes.fromhex(request['play_state_before_hex']);self.g.mem_write(REQUEST_ARENA,b)
  self.fp_action('direct_original_listener_from_new_actual_rejected_request',0x129450,
   (REQUEST_ARENA+0xC,0x3F800000,REQUEST_ARENA+0xA0))
  listener=self.g.reg_read(UC_X86_REG_EAX)
  self.fp_action('direct_original_unvoiced_source_creation_from_actual_new_request',0x127750,
   (sound,listener,REQUEST_ARENA,0xFFFFFFFF))
  datum=self.g.reg_read(UC_X86_REG_EAX);assert datum!=0xFFFFFFFF
  self.source_pointer=self.u(self.u(0x4ED94C)+0x44)+(datum&0xFFFF)*0xBC
  self.chunk_pointer=int(self.cache_calls[-1]['stack'][1],16)
  self.created_sources.append({'datum':f'{datum:08x}','pointer':f'{self.source_pointer:08x}'})
  assert self.u(self.source_pointer+0x10)==weapon
  assert self.u(self.source_pointer+0xAC,2)==0xFFFF and self.u(self.source_pointer+0xA8)==0xFFFFFFFF
  if scenario=='reload':self.worker_once()
  before=self.source_state();removal=self.owner_action('original_full_source_remove_reason3',0x128A70,(3,),((UC_X86_REG_EAX,datum),))
  after=self.source_state();completion=None
  if scenario=='ready':
   start=len(self.owner_stores);self.worker_once();completion={'stores':self.owner_stores[start:],'after':self.source_state()}
  return {'scenario':scenario,'spawned_player':f'{self.spawned_player:08x}','spawned_unit':f'{unit:08x}',
   'source_allocated_weapon':f'{weapon:08x}','source_weapon_pointer':f'{wp:08x}','source_sound_datum':f'{sound:08x}',
   'allocated':allocated,'audio_setup':self.new_steps,'file_binding':self.file_binding,
   'request':request,'original_request_calls':self.request_calls,'original_request_stores':self.request_stores,
   'original_caller_static_pointer_inputs':caller_inputs,
   'original_authored_callback_input':callback_input,
   'source_kernel_call_site_explicit':True,'actual_listener':f'{listener:08x}','created_source_datum':f'{datum:08x}',
   'before_removal':before,'removal':removal,'after_removal':after,'pending_read_completion':completion,
   'owner_calls':self.owner_calls,'owner_stores':self.owner_stores,'audio_boundaries':self.owner_boundaries,
   'ordered_source_lifecycle_stores':self.lifecycle_stores,
   'ordered_owner_callback_instructions':self.owner_callback_instructions,
   'clock_calls':self.clock_calls,'initial_rng':self.initial_rng,'source_player_stages':self.player_stages,
   'source_player_stores':self.player_stores,'source_producers':self.producers,'source_events':self.events,
   'raw_providers':self.raw_providers,'physical_providers':self.physical_services,'cache_provider_calls':self.provider_calls,
   'source_resource_delivery':self.resource_delivery,'source_resource_requests':self.resource_requests,
   'inherited_supplied_services':{f'{a:08x}':str(v)for a,v in self.services.items()},
   'object_header_high_water_identity_owner_driver_voice_or_admission_fields_supplied':False,
   'ordinary_FP_event_local_binding_complete_map_or_playback_proven':False,'native_gameplay_update_applied':False}

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--case',choices=('ready','reload','both'),default='ready');ap.add_argument('--replay',type=Path);ap.add_argument('--context-replay',type=Path);args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 replay=json.loads(args.replay.read_text())if args.replay else None;proc=None;errfile=None
 args.output.parent.mkdir(parents=True,exist_ok=True)
 if args.context_replay:data=json.loads(args.context_replay.read_text())
 else:
  assert digest(CONTEXT_OUT/'generator.exe')==CONTEXT_EXE_PIN
  errfile=(args.output.parent/'runtime-stderr.txt').open('w')
  proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errfile,text=True,
   env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
  data=json.loads(proc.stdout.readline())
 cases=[];traces=[]
 try:
  for i,scenario in enumerate(('ready','reload')if args.case=='both'else(args.case,)):
   t=prepare(data,trace_type=lambda d:ValidOwnerTrace(d,None if replay is None else replay['cases'][i]['clock_calls']))
   try:row=t.valid_case(scenario)
   except Exception:
    print(json.dumps({'phase':t.active,'source_stop':t.new_invalid,'recent':[f'{a:08x}'for a in t.recent]}),flush=True);raise
   cases.append(row);traces.append(t)
   if replay:assert t.replay_cursor==len(replay['cases'][i]['clock_calls'])
   print(json.dumps({'scenario':scenario,'weapon':row['source_allocated_weapon'],'cleanup_completed':row['removal']['completed']}),flush=True)
 finally:
  if proc:
   proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60)
   proc.stdout.close();errfile.close();assert proc.returncode==0,(args.output.parent/'runtime-stderr.txt').read_text()
 context=args.output.parent/'raw-host-context.json';context.write_text(json.dumps(data,indent=2)+'\n')
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);listings=[];t=traces[0]
 executed=set().union(*(t.owner_executed for t in traces));ins=[t.code.at(a)for a in sorted(executed)]
 for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes and all(bytes(q.g.mem_read(z.address,len(z.bytes)))==z.bytes for q in traces)
 p=directory/'executed-original-instructions.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
 whole={'file':p.name,'sha256':digest(p),'instructions':len(ins)}
 for e in ENTRIES:
  f=_trace(t.code,e,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(f.code)]
  for z in ins:assert t.image.read(z.address,len(z.bytes))==z.bytes and all(bytes(q.g.mem_read(z.address,len(z.bytes)))==z.bytes for q in traces)
  p=directory/f'{e:08x}.asm';p.write_text(''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in ins))
  listings.append({'entry':f'{e:08x}','file':p.name,'sha256':digest(p),'instructions':len(ins),
   'calls':[{'site':f'{a:08x}','target':f'{b:08x}','kind':k}for a,b,k in f.sites]})
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 report={'oracle_sha256':digest(__file__),'source_sha256':PIN,'cache_sha256':CACHE_PIN,'frozen_prior_pins':FROZEN,
  'context_snapshot_sha256':digest(context),'context_mode':'whole_recorded_context_replay'if args.context_replay else'real_Wine_worker_alive_during_source_execution',
  'clock_mode':'verbatim_measured_input_replay'if replay else'real_host_clock_measurement','cases':cases,
  'instruction_listings':listings,'executed_original_listing':whole,'native_gameplay_update_applied':False}
 args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'output':str(args.output),'cases':len(cases),'prior_pins':len(FROZEN)}))
if __name__=='__main__':main()
