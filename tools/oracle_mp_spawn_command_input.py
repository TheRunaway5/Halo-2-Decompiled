#!/usr/bin/env python3
"""Bounded original neutral command producer/consumer; prior freezes are read-only."""
import argparse,json,os,subprocess,struct,time,hashlib
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI
from oracle_mp_spawn_player_visibility import PlayerTrace,prepare,spawn_and_activate,FROZEN as OLDER,ROOT,CONTEXT_OUT,digest,ENTRIES as PRIOR_ENTRIES
from oracle_smg_firing_1_1 import STACK,STOP
from oracle_mp_spawn_process_heap_world import KernelBoundary
from functions import _trace
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from oracle_smg_audio_startup_1_1 import StartupTrace as ClockProvider
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/command-input'
CLOCK=0xb000000
PACKET=0x10f0000
COMMAND=0x10ec000
CLOCK_FIXTURE=OUT/'measured-host-clock-inputs.json'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_player_visibility.py':'aeafe79cff80e9d607e843d5bdb9f7c86b2814df15ec1b1179ebf4fe505e8572',
 'docs/MP_PLAYER_VISIBILITY_RESEARCH.md':'0383d7b128c173322e2902452c03143522193211309b283853afe6c96033347b',
 'analysis/multiplayer/integration/spawn-source/player-visibility/report.json':'f58232e963b26bcb4a6471db8c40488f2bb195c4be5cf8e0617331ae4d5f78dd',
 'tools/oracle_smg_audio_startup_1_1.py':'cbca5876b76195d85071aebfa7f404544f95152a832b9f038deb499d4f4ca6a3',
 'analysis/multiplayer/integration/spawn-source/command-input/measured-host-clock-inputs.json':'3789bb92e1fb3e8f18ed3b2a9471c5156f8b9af86360055952f85e625adf3326'}
EXTRA=(0x663b0,0x663a0,0x663f0,0x67a70,0x66f50,0x67230,0x673a0,0x68170,0x69030,0x6aab0,0x6aba0,0x197450,0x32803e,0x32c609,0x32e813,0x328505,0x337a40,0x336983,0x1961b0,0x87b40,0x88070,0x14e7e0,0x68040,0x680f0,0x68e10,0x69700,0x85220,0x14e780,0x684f0,0x694a0,0x69350,0x85460,0x68460,0x685b0,0x151c90,0xc8140,0xc8250,0x188ad0,0x1884e0,0x187610,0x69280,0x852a0,0x1ea490,0x150f00,0x150dd0,0x190a30,0x139410,0x11d780)
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+EXTRA))
class InputTrace(PlayerTrace):
 def __init__(self,data):
  self.input_calls=[];self.input_returns=[];self.input_stores=[];self.clock_calls=[];self.track_input=False;self.replay=json.loads(CLOCK_FIXTURE.read_text())['clock_calls'];self.replay_cursor=0
  super().__init__(data)
  self.g.mem_map(CLOCK,0x1000)
  assert self.u(0x417cd4)==0x8000009c
  self.w(0x417cd4,CLOCK)
  self.track_input=True
 def hook(self,g,a,size,user):
  if self.instrument and self.track_input:
   if a==0x336983:
    ClockProvider.clock_provider(self,a);self.clock_calls[-1]['provider_mode']='verbatim_replay_of_measured_host_input';return
   for ret,sp,row in self.input_returns[:]:
    if a==ret and g.reg_read(UC_X86_REG_ESP)==sp:
     row['returned_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
     if row['entry']=='00085460':
      row['defined_return_al']=g.reg_read(UC_X86_REG_EAX)&0xff
      row['record88_after_hex']=bytes(g.mem_read(int(row['record_pointer'],16),0x88)).hex()
      row['delivery5c_after_hex']=bytes(g.mem_read(int(row['delivery_pointer'],16),0x5c)).hex()
     self.input_returns.remove((ret,sp,row))
   if a in EXTRA:
    sp=g.reg_read(UC_X86_REG_ESP);row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(8)])
    if a==0xc8140:
     ptr=g.reg_read(UC_X86_REG_ECX);row['source_control_pointer']=f'{ptr:08x}';row['source_control7c_hex']=bytes(g.mem_read(ptr,0x7c)).hex()
    if a==0x85460:
     record=self.u(sp+4);delivery=self.u(sp+8);clock=self.u(0x518444)
     row.update(record_pointer=f'{record:08x}',delivery_pointer=f'{delivery:08x}',record88_before_hex=bytes(g.mem_read(record,0x88)).hex(),delivery5c_before_hex=bytes(g.mem_read(delivery,0x5c)).hex(),clock_pointer=f'{clock:08x}',clock12_entry_hex=bytes(g.mem_read(clock,12)).hex(),expiration4_entry_hex=bytes(g.mem_read(0x4d62cc,4)).hex())
    self.input_calls.append(row)
    if a==0xc8140:self.input_returns.append((self.u(sp),sp+4,row))
    if a==0x85460:self.input_returns.append((self.u(sp),sp+12,row))
   if a==0x337a40:
    original=self.replay[self.replay_cursor];self.replay_cursor+=1
    assert original['entry']==f'{a:08x}'and original['phase']==self.active
    ns=original['observed_boot_ns'];v=original['tick_ms_u32']
    assert v==(ns//1000000)&0xffffffff
    self.w(CLOCK,v)
    self.clock_calls.append(dict(phase=self.active,entry=f'{a:08x}',ordinal156=True,original_thunk_word='8000009c',guest_raw_backing=f'{CLOCK:08x}',observed_boot_ns=ns,tick_ms_u32=v,original_GetTickCount_instructions_execute=True))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.track_input:
   label=None
   if 0x4d62cc<=address<0x4d62d0:label='command_expiration_configuration'
   elif 0x47e54c<=address<0x47e550:label='configuration_handler_flags'
   elif 0x4d6d38<=address<0x4d6d50:label='simulation_global'
   elif 0x54c670<=address<0x54c670+0x1224:label='simulation_context'
   elif 0x54d894<=address<0x54d894+0xc34:label='simulation_watcher'
   elif address in (0x518494,0x518498):label='source_recording_time_initialization'
   if self.track_players:
    pool=self.u(0x4e78d0);heads=self.u(pool+0x44)
    for i in range(self.u(pool+0x38)):
     ptr=self.u(heads+i*12+8)
     if not ptr:continue
     off=address-ptr;datum=f'{self.u(heads+i*12,2)<<16|i:08x}'
     if 0x148<=off<0x1c8 or 0x1c8<=off<0x1ec or off in(0x134,0x214,0x23d,0x241):
      label='source_unit_control_'+datum+'_'+f'{off:03x}';break
   if label:self.input_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',kind=label,address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
 def ret(self,pop,result):
  sp=self.g.reg_read(UC_X86_REG_ESP);self.g.reg_write(UC_X86_REG_EAX,result);self.g.reg_write(UC_X86_REG_ESP,sp+4+pop*4);self.g.reg_write(UC_X86_REG_EIP,self.u(sp))
 def input_state(self):
  return dict(simulation_global=f'{self.u(0x4d6d44):08x}',simulation_context1224_hex=bytes(self.g.mem_read(self.u(0x4d6d44),0x1224)).hex(),watcher=f'{self.u(0x4d6d48):08x}',watcher_c34_hex=bytes(self.g.mem_read(self.u(0x4d6d48),0xc34)).hex()if self.u(0x4d6d48)else None,global_flags_hex=bytes(self.g.mem_read(0x4d6d38,16)).hex(),game_1120=self.u(self.u(0x4edf18)+0x1120,1),current_bsp=self.u(self.u(0x4edf18)+2,2),packet4048_hex=bytes(self.g.mem_read(PACKET,0x4048)).hex(),command5c_hex=bytes(self.g.mem_read(COMMAND,0x5c)).hex())
 def command_action(self,name,a,args=(),regs=()):
  start=len(self.input_calls);before=self.input_state();ok=self.stage(name,a,args,regs)
  self.player_stages[-1]['command_state_before']=before;self.player_stages[-1]['command_state_after']=self.input_state();self.player_stages[-1]['command_calls']=self.input_calls[start:]
  return ok

def execute(data,config=True):
 t=prepare(data,trace_type=InputTrace)
 if config:assert t.command_action('source_network_configuration_default_handler',0x663b0)
 for n,e in [('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0)]:
  assert t.command_action(n,e)
 assert spawn_and_activate(t)
 assert t.command_action('source_tick_packet_producer',0x684f0,regs=((UC_X86_REG_EAX,PACKET),))
 assert t.u(PACKET+8)==0 and t.u(t.u(0x4edf18)+0x1120,1)==0
 assert t.command_action('source_tick_packet_preplayer_dispatch',0x685b0,(PACKET,))
 assert t.command_action('source_complete_early_player_phase',0x14eae0,(PACKET,))
 assert not any(c['entry']=='000c8140'for c in t.input_calls)
 assert t.command_action('source_neutral_command_initializer',0x14e780,regs=((UC_X86_REG_EDX,COMMAND),))
 record=t.u(0x4d6d44)+0x80+(t.spawned_player&0xffff)*0x88
 assert t.command_action('source_neutral_command_submission_explicit_root',0x852a0,(record,COMMAND))
 assert t.command_action('source_packet_command_assembly_explicit_root',0x69350,(t.u(0x4d6d44),PACKET))
 assert t.u(PACKET+8)==(1 if config else 0)
 assert t.command_action('source_early_player_phase_with_assembled_command',0x14eae0,(PACKET,))
 assert sum(c['entry']=='000c8140'for c in t.input_calls)==int(config)
 validation=[c for c in t.input_calls if c['entry']=='00085460']
 assert len(validation)==1 and validation[0]['defined_return_al']==int(config)
 for n,e in [('prephysics',0xb7e00),('world',0x1c7660),('postphysics',0xb7f50),('newflags',0xb8040)]:
  assert t.command_action('source_command_motion_'+n,e)
  assert not any(q.get('service')for q in t.player_stages[-1]['services'])
 assert t.replay_cursor==len(t.replay)==3
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 traces=[];cases=[]
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  for config in (False,True):
   t=execute(data,config);traces.append(t)
   name='configuration_omitted_age_zero_command_rejected'if not config else'source_configuration_neutral_command_and_motion'
   cases.append(dict(case=name,configuration_producer_included=config,stages=t.player_stages,input_calls=t.input_calls,clock_calls=t.clock_calls,original_calls=t.new_calls,preparation=t.producers,services=t.events,input_state=t.input_state(),source_input_stores=t.input_stores,player_and_object_stores=t.player_stores,motion_request_producers_consumers=t.motion_requests,resource_delivery=t.resource_delivery,resource_requests=t.resource_requests,raw_services=t.events,physical_raw_services=t.physical_services,compatibility_platform_scope=t.platform_input,compatibility_lock_calls=t.lock_calls,source_raw_impact_pool_allocations=t.os_pool_allocs,creation_input_hex=t.creation_input_hex,default_variant_hex=t.default_variant))
   print(json.dumps(dict(case=name,executed=len(t.executed),control_writes=sum(c['entry']=='000c8140'for c in t.input_calls))),flush=True)
 finally:
  _,err=proc.communicate('x',timeout=60)
 assert proc.returncode==0,err
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);t=traces[0];executed=set().union(*(q.executed for q in traces));listings=[]
 def emit(name,ins):
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert all(bytes(q.g.mem_read(i.address,len(i.bytes)))==i.bytes for q in traces)
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in executed for i in ins))
 whole=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(executed)])
 for e in ENTRIES:
  r=_trace(t.code,e,t.starts|set(ENTRIES));listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',[t.code.at(a)for a in sorted(r.code)]),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-source-input-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for q in traces for r in q.input_stores})])
 manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,cases=cases,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],instruction_listings=listings,instruction_manifest_sha256=manifest,executed_original_listing=whole,observed_input_store_listing=stores,executed_original_addresses_verified=len(executed),source_configuration_descriptor=dict(address='0046e5e4',raw20_hex=t.image.read(0x46e5e4,32).hex(),handle='0047e548',raw10_hex=t.image.read(0x47e548,16).hex()),simulation_callback_record=dict(index=23,address='00447764',raw24_hex=t.image.read(0x447428+23*36,36).hex()),summary=dict(cases=2,original_outer_packet_player_admissions=0,explicit_source_neutral_command_admissions=1,original_C8140_calls=1,complete_four_motion_phases=2,complete_device_or_map_ready_scheduling=0),boundaries=[
 'All previous immutable suites are read-only. Exact complete compatibility context, source cache/resources, raw storage and loadout attachment/audio service boundaries remain declared; Xbox hardware platform/input/contact parity is not claimed.',
 'Preparation reuses the frozen selected player/world producer order. This extension executes the actual network-configuration default handler and selected simulation callback23 init/dispose/reset after that preparation, before join. Complete title and ordinary callback23 lifecycle placement remain open. The empty-dispose callback owns source NONE table sentinels; no table fields are supplied.',
 'Original configuration descriptor46E5F4 dispatches663A0→663F0→67A70; it assigns command expiration4D62CC from exact XBE literal46507C. The negative case omits this producer and retains actual zero BSS, so even source age-zero commands fail strict85460 expiration admission.',
 'Original680F0→197450 executes actual time(NULL) at32803E and source recording-time stores. This path does not execute srand, rand,32C609 or cold CRT fallback allocation; their static listings are not executed-path claims. Original337A40 executes unchanged and reads declared kernel ordinal156 data backing containing recorded real host CLOCK_BOOTTIME milliseconds.336983 uses the separately pinned real-host FILETIME provider, replayed verbatim. No game clock, readiness or RNG field is supplied directly.',
 'Source nonlocal zero-key creation/profile/default variant inputs remain the frozen delivery boundary. Current player/unit binding, source loadout, source cluster4 and actualFFDF PVS are produced unchanged. Local user/view/controller and whole session/device state are not supplied or claimed.',
 'Complete684F0→694A0 retains original Game+1120=0 and emits zero player command mask. Full685B0/14EAE0 return withoutC8140. Actual outer admission additionally needs map readiness and simulation state4;190A30 owns readiness after full11D780 and map startup, which is not executed here.',
 'A separate explicit-root source command composition executes14E780 neutral initialization→852A0 submission/codec→69350 assembly→complete14EAE0. These roots do not prove188AD0 device dispatch,69280 local membership or full684F0 admission scheduling. No command bytes, target mask, head pose, control sequence, velocity or contact are supplied.',
 'The positive case reaches original150DD0→1EA490→150F00→151C90→C8250→C8140 on the true allocated owner. Exact5C command,7C control and source unit writes are retained. Neutral codec quantization output is original and is not replaced with ideal zeros.',
 'B7E00→1C7660→B7F50→B8040 then return in original object-phase order with exact source control/body/PVS inputs and no game/Havok service used. Source tick increment, other139750 subsystems, originalcontrol87 entry and full ordinary motion/contact/hardware parity remain excluded.'])
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),oracle_sha256=out['oracle_sha256'],manifest=manifest,executed=len(executed))),flush=True)
if __name__=='__main__':main()
