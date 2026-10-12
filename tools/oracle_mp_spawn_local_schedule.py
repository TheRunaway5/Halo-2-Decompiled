#!/usr/bin/env python3
"""Bounded original local-storage producers and full map-start dependency stop."""
import argparse,hashlib,json,os,subprocess,time,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_ESI
from oracle_mp_spawn_command_input import InputTrace,ClockProvider,CLOCK,FROZEN as OLDER
from oracle_mp_spawn_player_visibility import PlayerTrace,prepare,ROOT,CONTEXT_OUT,digest
from oracle_smg_firing_1_1 import PIN,CACHE_PIN
from functions import _trace
OUT=ROOT/'analysis/multiplayer/integration/spawn-source/local-command-scheduling'
CLOCK_FIXTURE=OUT/'measured-extra-clock-inputs.json'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_command_input.py':'de382cb5f14ccbc64fbc5609419382f2a574fc1f46ba00e371efbfa3a641f4fc',
 'docs/MP_COMMAND_INPUT_RESEARCH.md':'e29c2cad74a0c10bd6ff4b8612787fd3c89a347c3c9d14c7c133d1aa80a2b967',
 'analysis/multiplayer/integration/spawn-source/command-input/report.json':'1fc1b58c65fd804477ac9bbdcd3bcad1ad10bd954f98b0f5ea17348891ba0a76',
 'analysis/multiplayer/integration/spawn-source/local-command-scheduling/measured-extra-clock-inputs.json':'e672f902e7cb280c152eae2039b1ac56c7fd977e01442e8fdd8f803941ec034f'}
ENTRIES=(0x1874d0,0x187520,0x1ea3d0,0x1ea410,0x156a30,0x156ad0,0x188d00,
 0x1479b0,0x139390,0x190a30,0x139410,0x139540,0x1395b0,0x11d780,0x11e330,
 0x12cbc0,0x12ce00,0x68300,0x69170,0x69e90,0x6b750,0x69fe0,0x6a740,
 0x14dda0,0x14dff0,0x1508b0,0x150950,0x188ad0,0x1884e0,0x187610,0x69280,0x6a860,0x852a0,
 0x19f160,0x19f720,0x16d070,0x16d220,0x19f6b0,0x19fc60,0x19fd70,
 0x32803e,0x328505,0x32c609,0x32e813,0x337a40,0x336983,0x327866,0x68460)
class LocalTrace(InputTrace):
 def __init__(self,data,extra_replay=None):
  self.local_enabled=False;self.local_calls=[];self.local_stores=[];self.local_stages=[];self.map_callbacks=[];self.extra_clock_calls=[];self.extra_replay=extra_replay;self.extra_replay_cursor=0
  super().__init__(data);self.local_enabled=True
 def hook(self,g,a,size,user):
  if self.instrument and self.local_enabled:
   if a==0x13944a:
    offset=g.reg_read(UC_X86_REG_ESI);index=offset//36;assert offset%36==0 and index<68
    self.map_callbacks.append(dict(phase=self.active,index=index,record_address=f'{0x447428+offset:08x}',raw24_hex=bytes(g.mem_read(0x447428+offset,36)).hex(),target=f'{g.reg_read(UC_X86_REG_EAX):08x}',registers=self.registers()))
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP);self.local_calls.append(dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(8)]))
   if a in(0x337a40,0x336983)and self.active!='source_simulation_new_map':
    if a==0x337a40:
     if self.extra_replay is None:ns=time.clock_gettime_ns(time.CLOCK_BOOTTIME);v=(ns//1000000)&0xffffffff
     else:
      r=self.extra_replay[self.extra_replay_cursor];self.extra_replay_cursor+=1;assert r['entry']==f'{a:08x}'and r['phase']==self.active
      ns=r['observed_boot_ns'];v=r['tick_ms_u32'];assert v==(ns//1000000)&0xffffffff
     self.w(CLOCK,v);self.extra_clock_calls.append(dict(phase=self.active,entry=f'{a:08x}',observed_boot_ns=ns,tick_ms_u32=v,original_GetTickCount_instructions_execute=True,provider_mode='verbatim_replay_of_measured_host_input'if self.extra_replay is not None else'measured_host_input'))
     PlayerTrace.hook(self,g,a,size,user);return
    old_replay,old_cursor=self.replay,self.replay_cursor
    self.replay=self.extra_replay;self.replay_cursor=self.extra_replay_cursor;start=len(self.clock_calls)
    ClockProvider.clock_provider(self,a)
    self.extra_replay_cursor=self.replay_cursor;self.replay,self.replay_cursor=old_replay,old_cursor
    row=self.clock_calls.pop();assert len(self.clock_calls)==start;row['provider_mode']='verbatim_replay_of_measured_host_input'if self.extra_replay is not None else'measured_host_input';self.extra_clock_calls.append(row);return
  super().hook(g,a,size,user)
 def local_state(self):
  fields={}
  for name,global_ptr,n in [('controls',0x4f4854,0x264),('views',0x526250,0x6c8),('camera_time',0x51845c,0x78),('camera_flag',0x4f0204,4),('random',0x4ee9d8,8),('game',0x4edf18,0x1200)]:
   p=self.u(global_ptr);fields[name]=dict(global_pointer=f'{global_ptr:08x}',pointer=f'{p:08x}',raw_hex=bytes(self.g.mem_read(p,n)).hex()if p else None)
  p=self.u(0x4d6d44);fields['simulation']=dict(pointer=f'{p:08x}',raw1224_hex=bytes(self.g.mem_read(p,0x1224)).hex()if p else None)
  fields['camera_static50c_hex']=bytes(self.g.mem_read(0x4f0208,0x50c)).hex()
  p=self.u(0x4f01f0);fields['player_globals']=dict(pointer=f'{p:08x}',raw130_hex=bytes(self.g.mem_read(p,0x130)).hex()if p else None)
  fields['missing_callback13_pool_slots']={f'{a:08x}':bytes(self.g.mem_read(a,4)).hex()for a in(0x4f5cbc,0x4f5cb8)}
  fields['arena_owner_globals10_hex']=bytes(self.g.mem_read(0x4ed650,16)).hex()
  return fields
 def local_action(self,name,entry,args=(),regs=()):
  start=len(self.local_calls);before=self.local_state();ok=self.stage(name,entry,args,regs)
  row=self.player_stages[-1];row.update(local_state_before=before,local_state_after=self.local_state(),local_calls=self.local_calls[start:]);self.local_stages.append(row)
  return ok
 def mem_write(self,g,access,address,size,value,user):
  if self.local_enabled and self.track_players:
   kind=None
   if 0x4f0208<=address<0x4f0208+0x50c:kind='camera_static+'+f'{address-0x4f0208:04x}'
   for label,a in [('controls_slot',0x4f4854),('views_slot',0x526250),('camera_time_slot',0x51845c),('camera_flag_slot',0x4f0204),('random_slot',0x4ee9d8)]:
    if a<=address<a+4:kind=label;break
   if kind is None:
    for label,a,n in [('controls',0x4f4854,0x264),('views',0x526250,0x6c8),('camera_time',0x51845c,0x78),('camera_flag',0x4f0204,4),('random',0x4ee9d8,8),('game',0x4edf18,0x1200)]:
     p=self.u(a)
     if p and p<=address<p+n:kind=label+'+'+f'{address-p:04x}';break
   if kind:self.local_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',kind=kind,address=f'{address:08x}',size=size,before_hex=bytes(g.mem_read(address,size)).hex(),after_hex=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
  super().mem_write(g,access,address,size,value,user)
def execute(data,rng,extra_replay=None):
 t=prepare(data,trace_type=lambda d:LocalTrace(d,extra_replay))
 for name,entry in [('source_control_storage_record20',0x1874d0),('source_view_storage_record21',0x1ea3d0),('source_camera_storage_record35',0x156a30),('source_control_newmap_record20',0x187520),('source_view_newmap_record21',0x1ea410),('source_camera_newmap_record35',0x156ad0)]:
  if not t.local_action(name,entry):return t
 for name,entry in [('source_network_configuration_default_handler',0x663b0),('source_simulation_storage_constructor',0x68040),('source_empty_simulation_dispose',0x68170),('source_simulation_new_map',0x680f0),('source_readiness_gated_simulation_scheduler',0x68300)]:
  if not t.local_action(name,entry):return t
 if rng and not t.local_action('source_random_storage_record3',0x1479b0):return t
 # Same untouched inherited options delivery already present in the prior fixture.
 # No local identity/profile/session options or source-owned fields are assigned.
 t.local_action('source_complete_map_ready_owner',0x190a30,regs=((UC_X86_REG_EAX,t.u(0x4edf18)+8),))
 assert t.local_stages[-1]['completion']=='blocked_original_instruction' and t.local_stages[-1]['stopped_eip']=='0019f727'
 assert t.local_stages[-1]['invalid']['address']=='00000029' and t.u(0x4f5cbc)==0
 assert t.u(t.u(0x4edf18)+0x1120,1)==0 and t.u(t.u(0x4d6d44)+0x18)==2
 assert not any(c['entry']in('001395b0','0011d780','00069170','00188ad0','001884e0','00187610','00069280','000852a0')for c in t.local_calls)
 assert t.extra_replay is None or t.extra_replay_cursor==len(t.extra_replay)
 assert t.replay_cursor==len(t.replay)==3
 return t
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--clock-replay',type=Path,default=CLOCK_FIXTURE);args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 replay=json.loads(args.clock_replay.read_text())
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 cases=[];traces=[]
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  for rng in (False,True):
   name='local_storage_without_rng_map_gate'if not rng else'local_storage_with_source_rng_map_gate'
   t=execute(data,rng,replay[name]if replay else None);traces.append(t)
   cases.append(dict(case=name,source_random_constructor_included=rng,stages=t.local_stages,calls=t.local_calls,stores=t.local_stores,map_newmap_callbacks=t.map_callbacks,input_calls=t.input_calls,input_stores=t.input_stores,player_stores=t.player_stores,all_player_stages=t.player_stages,extra_clock_calls=t.extra_clock_calls,inherited_clock_calls=t.clock_calls,preparation=t.producers,services=t.events,physical_services=t.physical_services,platform=t.platform_input,compatibility_lock_calls=t.lock_calls,source_raw_impact_pool_allocations=t.os_pool_allocs,final_local_state=t.local_state()))
   print(json.dumps(dict(case=name,stages=[dict(name=s['name'],completion=s['completion'],eip=s['stopped_eip'],invalid=s['invalid'])for s in t.local_stages])),flush=True)
 finally:
  # Wait for this owned generator, rather than pipe EOF from unrelated workers
  # sharing the compatibility Wine prefix. Its storage remains alive until here.
  proc.stdin.write('x');proc.stdin.flush();proc.stdin.close();proc.wait(timeout=60)
  proc.stdout.close();proc.stderr.close()
 assert proc.returncode==0
 listings=[];directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True);executed=set().union(*(t.executed for t in traces))
 def emit(name,ins):
  for i in ins:assert traces[0].image.read(i.address,len(i.bytes))==i.bytes and all(bytes(t.g.mem_read(i.address,len(i.bytes)))==i.bytes for t in traces)
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in executed for i in ins))
 whole=emit('executed-original-instructions.asm',[traces[0].code.at(a)for a in sorted(executed)])
 for e in ENTRIES:
  r=_trace(traces[0].code,e,traces[0].starts|set(ENTRIES));ins=[traces[0].code.at(a)for a in sorted(r.code)]
  listings.append(dict(entry=f'{e:08x}',**emit(f'{e:08x}.asm',ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in r.sites if k=='call']))
 stores=emit('observed-local-source-store-sites.asm',[traces[0].code.at(a)for a in sorted({int(r['instruction'],16)for t in traces for r in t.local_stores+t.input_stores+t.player_stores})])
 manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,cases=cases,instruction_patches=[],new_supplied_gameplay_or_physics_results=[],instruction_listings=listings,instruction_manifest_sha256=manifest,executed_original_listing=whole,observed_local_store_listing=stores,executed_original_addresses_verified=len(executed),callback_records=[dict(index=i,address=f'{0x447428+i*36:08x}',raw24_hex=traces[0].image.read(0x447428+i*36,36).hex())for i in(3,13,19,20,21,22,23,35)],summary=dict(cases=2,complete_control_view_camera_constructors_per_case=3,complete_control_view_camera_newmap_resets_per_case=3,complete_original_map_start_calls=0,exact_map_start_stop='0019f727',local_membership_or_device_dispatches=0,readiness_or_state4_supplied=0),boundaries=[
 'Every prior frozen tool/report remains read-only. The inherited complete compatibility KPCR/ETHREAD/stack/process generator and original-requested raw heap/world storage providers remain explicit; compatibility-produced unknown zeros are not claimed as Xbox hardware defaults.',
 'Preparation is the frozen selected player/world initializer/reset composition, followed by actual callback20/21/35 storage then their new-map resets and selected callback23 roots. This is a bounded source-root composition, not complete139390 storage startup or authored whole-title/session scheduling. Whole139390 calls125280 including217090 savegame I/O, which remains unavailable in the frozen inherited preparation.',
 'Original1874D0,1EA3D0,156A30 allocate and bind complete control/view/camera records using the actual source game arena. Original187520,1EA410,156AD0 then populate those records. No owner, user/view/controller mapping, profile, readiness, simulation state, command mask or packet is supplied.',
 'Inherited game/options input backing and exact source default Slayer variant delivery remain declared.190A30 receives the existing options backing pointer from its original EAX contract. No local sign-in, identity/member/profile/session creation records are supplied. The source player/object pools remain empty in these cases.',
 'The omitted-random-constructor case retains the older declared global RNG adapter at01018000, initially7856341200000000. It is not a missing-RNG case. The other case executes actual1479B0, which owns new8-byte arena storage and initializes it via originaltime/rand/GetTickCount. Full upstream title RNG history remains open.',
 'The extra recorded clock fixture is replayed verbatim:336983 uses the pinned real-host FILETIME wrapper provider,337A40 executes original thunk against declared ordinal156 real-host boot-millisecond backing. Original32803E is time(NULL), not srand. No RNG/game/readiness field is written by the clock provider.',
 'Full68300 returns with actualGame+1120=0 and simulationkind1/state2, before69170. Neither state4 nor readiness is assigned to admit the local scheduler.',
 'Full190A30 enters full139410/139540 and ascending original map-reset callback traversal, then stops at callback13:19F727 attempts a byte write toNULL+29 because4F5CBC isNULL. The original19F160 pool constructor is statically listed but not executed.1395B0 identity creation,11D780 BSP startup and190A6E readiness writer are not reached.',
 'Static1395B0/14DDA0/14DFF0 identify ownership producers from game-options membership records. Actual local control scheduling188AD0/1884E0/187610/69280/852A0, processed device/profile delivery, original simulation2-to4 transition and complete12CBC0 remain unexecuted. Static listings do not prove those paths. No explicit neutral command roots from the earlier suite are entered here.'])
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),oracle_sha256=out['oracle_sha256'],manifest=manifest,executed=len(executed))),flush=True)
if __name__=='__main__':main()
