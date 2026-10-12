#!/usr/bin/env python3
"""Separate unchanged player binding, loadout and cluster visibility research.

Earlier frozen fixtures are read-only. No current-unit, cluster mask, contact,
pose, allocation success or physical readiness input is supplied here.
"""
import argparse,hashlib,json,os,subprocess,struct
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_live_body_world import LiveTrace,FROZEN as OLDER,ROOT,CONTEXT_OUT,digest,ENTRIES as PRIOR_ENTRIES
from oracle_mp_spawn_biped_physics import PLACEMENT
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STACK,STOP,PIN,CACHE_PIN
from functions import _trace
from oracle_mp_item_current_producers_1_1 import CurrentTrace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/player-visibility'
CREATION=0x10e8000
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_live_body_world.py':'5b2550b5f09ff214372c51e832407353e3f5da8e18713aca46d89855ee0c2586',
 'docs/MP_LIVE_BIPED_WORLD_RESEARCH.md':'cec754147a737775df3071de858913a512ca244e7e638784d930682ca3ebdd9a',
 'analysis/multiplayer/integration/spawn-source/live-body-world/report.json':'2bf6fd373182b3db29d7409012f9ae566dcf2d9d496bbf15700184c4c8858951',
 'tools/oracle_mp_spawn_smg_producers.py':'a86cff27319ce75a838ea42bca7fd9f1907b01fbb8b5329a1ede22f16627dd82',
 'analysis/multiplayer/integration/spawn-source/smg-producers-report.json':'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa',
 'tools/oracle_mp_item_current_producers_1_1.py':'cc04b51cd4ce882c25323ce534c264373356930725cf015a93b2908e8058bb81',
 'docs/MP_ITEM_CURRENT_PRODUCERS_RESEARCH.md':'16f291f82614d6cc58bab95793feda8a5ba77cf19de089202cee9a2118aa13e5',
 'analysis/multiplayer/integration/inventory-source/item-current-producers/report.json':'8e0351b19fbae952a6dd895f77489fed31cb9f04723ce2b5814fa0f656181a76'}
EXTRA=(0x37eb40,0x158ba0,0x158c10,0x14d350,0x14d800,0x14d600,0x14d3d0,
 0x14e240,0x1509c0,0x150640,0x15ffd0,0x1a28a0,0x14eae0,0x151ec0,
 0x13a2a0,0x13a380,0x16f8a0,0x16fbd0,0xb7ce0,0xcd3b0,0x159ef0,0x153a90,
 0x10d490,0x10d4f0,0x1efe10,0x1efe70,0x1efef0)
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+EXTRA))

class PlayerTrace(LiveTrace):
 def __init__(self,data):
  self.player_calls=[];self.player_stores=[];self.player_stages=[];self.player_returns=[];self.track_players=False;self.resource_delivery=[];self.motion_requests=[]
  super().__init__(data)
 def player_state(self):
  pp=self.u(0x4f01f4);n=self.u(pp+0x38);p=self.u(pp+0x44);stride=self.u(pp+0x24)
  return dict(pool=f'{pp:08x}',header_hex=bytes(self.g.mem_read(pp,0x4c)).hex(),
   globals=f'{self.u(0x4f01f0):08x}',globals130_hex=bytes(self.g.mem_read(self.u(0x4f01f0),0x130)).hex(),
   engine=f'{self.u(0x4f10b8):08x}',engine_c9c_hex=bytes(self.g.mem_read(self.u(0x4f10b8),0xc9c)).hex(),
   unit_queue=f'{self.u(0x517fe0):08x}',unit_queue1d88_hex=bytes(self.g.mem_read(self.u(0x517fe0),0x1d88)).hex(),
   trigger_bits=f'{self.u(0x526258):08x}',trigger24_hex=bytes(self.g.mem_read(self.u(0x526258),0x24)).hex()if self.u(0x526258)else None,
   records=[dict(index=i,datum=f'{self.u(p+i*stride,2)<<16|i:08x}',pointer=f'{p+i*stride:08x}',
    flags=f'{self.u(p+i*stride+2,2):04x}',cluster=self.u(p+i*stride+0x2a,2),
    current=f'{self.u(p+i*stride+0x2c):08x}',previous=f'{self.u(p+i*stride+0x30):08x}',
    raw_hex=bytes(self.g.mem_read(p+i*stride,stride)).hex())for i in range(n)if self.u(p+i*stride,2)])
 def hook(self,g,a,size,user):
  if self.instrument:
   for ret,sp,row in reversed(self.player_returns[:]):
    if a==ret and (sp is None or g.reg_read(UC_X86_REG_ESP)==sp):
     row['returned_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
     if 'request_pointer' in row:row['request154_after_hex']=bytes(g.mem_read(int(row['request_pointer'],16),0x154)).hex()
     self.player_returns.remove((ret,sp,row));break
  if self.instrument and a in EXTRA:
   sp=g.reg_read(UC_X86_REG_ESP)
   row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),
    return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
   self.player_calls.append(row);self.player_returns.append((self.u(sp),None,row))
  if self.instrument and a in (0x1e9540,0xe3b20):
   sp=g.reg_read(UC_X86_REG_ESP);ptr=g.reg_read(UC_X86_REG_EDI)if a==0x1e9540 else self.u(sp+12)
   row=dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),return_address=f'{self.u(sp):08x}',
    request_pointer=f'{ptr:08x}',request154_before_hex=bytes(g.mem_read(ptr,0x154)).hex(),
    stack=[f'{self.u(sp+4+i*4):08x}'for i in range(24)])
   self.motion_requests.append(row)
   self.player_returns.append((self.u(sp),sp+(88 if a==0x1e9540 else 16),row))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.track_players:
   pp=self.u(0x4f01f4);start=self.u(pp+0x44);end=start+self.u(pp+0x20)*self.u(pp+0x24);game=self.u(0x4edf18)
   label=None
   if start<=address<end:label=dict(kind='player',index=(address-start)//self.u(pp+0x24),offset=f'{(address-start)%self.u(pp+0x24):03x}')
   elif game+0x1138<=address<game+0x11f8:label=dict(kind='cluster_mask_bank',offset=f'{address-game:04x}')
   elif self.u(0x4f01f0)<=address<self.u(0x4f01f0)+0x130:label=dict(kind='player_globals',offset=f'{address-self.u(0x4f01f0):03x}')
   op=self.u(0x4e78d0);heads=self.u(op+0x44)
   if heads<=address<heads+self.u(op+0x20)*12:label=dict(kind='object_header',index=(address-heads)//12,offset=f'{(address-heads)%12:02x}')
   for i in range(self.u(op+0x38)):
    p=self.u(heads+i*12+8)
    if not p:continue
    off=address-p
    if off in (4,0x14,0x28,0x2c,0x13c,0x140,0x134,0x154)or 0x210<=off<0x240 or 0x64<=off<0xa0:
     label=dict(kind='object',datum=f'{self.u(heads+i*12,2)<<16|i:08x}',offset=f'{off:03x}');break
   if label:
    self.player_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
     address=f'{address:08x}',size=size,before=bytes(g.mem_read(address,size)).hex(),
     value=f'{value&((1<<(size*8))-1):0{size*2}x}',**label))
  super().mem_write(g,access,address,size,value,user)
 def stage(self,name,entry,args=(),regs=()):
  self.active=name;self.new_invalid=None;start=len(self.events)
  try:self.run(entry,args,regs);completion='complete_original_return'
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;completion='blocked_original_instruction'
  self.player_stages.append(dict(name=name,entry=f'{entry:08x}',arguments=[f'{v:08x}'for v in args],
   input_registers={str(k):f'{v:08x}'for k,v in regs},completion=completion,
   stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
   invalid=self.new_invalid,player=self.player_state(),world=self.state(),services=self.events[start:]))
  return completion=='complete_original_return'
 def prefix_stage(self,name,entry,end,args=()):
  self.active=name;self.new_invalid=None;start=len(self.events);self.w(STACK,STOP)
  for i,v in enumerate(args):self.w(STACK+4+i*4,v)
  self.g.reg_write(UC_X86_REG_ESP,STACK)
  try:
   self.g.emu_start(entry,end,count=1000000)
   assert self.g.reg_read(UC_X86_REG_EIP)==end
   completion='complete_original_fragment'
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;completion='blocked_original_instruction'
  self.player_stages.append(dict(name=name,entry=f'{entry:08x}',stop_exclusive=f'{end:08x}',
   arguments=[f'{v:08x}'for v in args],completion=completion,stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
   invalid=self.new_invalid,player=self.player_state(),world=self.state(),services=self.events[start:]))
  return completion=='complete_original_fragment'

def prepare(data,trace_type=PlayerTrace,trigger=True):
 # Same recovered selected order as the live-body-world freeze, with original
 # record22 inserted between player19 and physics25. Full title order stays open.
 t=trace_type(data);t.heap_prefix(None)
 t.call('original_CRT_lock_initializer',0x32cffc);t.call('original_CRT_onexit_initializer',0x3274e4)
 t.register();t.call('original_scratch_CRT_constructor',0x37d8c0)
 t.call('original_engine_table_CRT_callback',0x37eb40)
 t.fixup();t.arena_root();t.actual_heap=True
 for a in (0x18fce0,0x18fd70,0x246a50):t.services.pop(a,None)
 for a in (0x111490,0x111500,0x1dc8f0,0x1dc940,0xfb560,0xefee0,0xc9a40):t.services.pop(a,None)
 t.call('original_source_empty_player_pool',0x14cc00)
 t.call('original_engine_global_storage',0x158ba0)
 t.call('original_physics_constant_storage_initializer',0x1eef40)
 t.call('original_full_object_storage_initializer',0xb76b0)
 t.call('original_unit_queue_storage_initializer',0x10d490)
 if trigger:t.call('original_trigger_volume_bitset_storage_initializer',0x1efe10)
 t.fragment('original_AI_storage_and_reset_prefix',0x1cb180,0x1cb1f4)
 t.call('original_physics_storage_initializer',0x1c5f00)
 t.call('original_empty_player_map_reset',0x14ccd0)
 # Exact original default variant output from the frozen source producer.
 # This is game-options delivery, not a source session/lobby producer.
 report=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/smg-producers-report.json').read_text())
 t.default_variant=report['original_default_slayer_multiplier_cases'][0]['default_variant_hex']
 t.g.mem_write(t.u(0x4edf18)+0x13c,bytes.fromhex(t.default_variant))
 t.stage('original_engine_map_reset',0x158c10)
 if t.player_stages[-1]['completion']!='complete_original_return':return t
 t.call('original_physics_constants_map_reset',0x1eefe0)
 t.call('original_object_map_reset',0xb78c0)
 t.call('original_unit_queue_map_reset',0x10d4f0)
 if trigger:t.call('original_trigger_volume_bitset_map_reset',0x1efe70)
 t.call('original_BSP_datum_table_map_reset',0x1c5fd0)
 t.fragment('original_activation_mask_zero_prefix',0x1394b0,0x1394f0)
 t.call('original_object_map_activation',0xb7aa0);t.call('original_full_new_map_order',0x1c5ff0)
 t.call('original_current_cluster_masks_with_empty_player_pool',0x13a380)
 t.track_players=True
 return t

def join(t,index=0):
 r=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/smg-producers-report.json').read_text())['source_strength_cases'][0]
 # Exact original-produced property packet is delivered at the join input.
 # Remote identity/sign-in/transport are a declared zero-key nonlocal boundary.
 t.g.mem_write(CREATION,bytes(0xac));t.sh(CREATION+2,-1);t.w(CREATION+4,0xffffffff)
 t.g.mem_write(CREATION+0x1c,bytes.fromhex(r['host_property_hex']))
 t.creation_input_hex=bytes(t.g.mem_read(CREATION,0xac)).hex()
 assert t.stage('original_player_join',0x14d350,regs=((UC_X86_REG_EDI,index),(UC_X86_REG_EBX,CREATION)))
 t.spawned_player=t.g.reg_read(UC_X86_REG_EAX)
 assert t.spawned_player!=0xffffffff
 return t.spawned_player

def spawn_and_activate(t,index=0,activate=True):
 """Own source join/spawn/early cluster/PVS stores; source failure is retained.

 No ordinary motion is entered by this helper. A subclass may append genuine
 authored item construction, then execute its own original object passes.
 """
 CurrentTrace.weapon_resources(t)
 d=join(t,index)
 if not t.stage('original_normal_player_spawn',0x1509c0,(d,)):return False
 if t.g.reg_read(UC_X86_REG_EAX)&255!=1:return False
 p=t.u(t.u(0x4f01f4)+0x44)+(d&0xffff)*0x21c;t.spawned_unit=t.u(p+0x2c)
 if not t.prefix_stage('original_player_update_through_cluster_writer',0x14eae0,0x14ed0c,(0,)):return False
 if not activate:return True
 return t.stage('original_current_cluster_masks_and_callbacks',0x13a380)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 args.output.parent.mkdir(parents=True,exist_ok=True)
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,
  env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix')))
 try:
  data=json.loads(proc.stdout.readline());traces=[];cases=[]
  (args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  for name,spawn,trigger in (('joined_player_without_current_unit',False,True),
    ('bound_player_without_trigger_storage',True,False),('bound_player_first_ordinary_motion',True,True)):
   t=prepare(data,trigger=trigger)
   assert t.player_stages[-1]['completion']=='complete_original_return'
   if not spawn:
    join(t);assert t.stage('original_current_cluster_masks_without_unit',0x13a380)
    assert t.player_state()['records'][0]['cluster']==0xffff
    assert t.state()['source_active_mask_hex']==bytes(64).hex()
   else:
    assert spawn_and_activate(t)
    assert t.player_state()['records'][0]['cluster']==4
    assert t.state()['source_active_mask_hex']==(bytes.fromhex('ffdf0000')+bytes(60)).hex()
    assert not any(e.get('service')for e in t.player_stages[-1]['services'])
    for stage,entry in (('object_prephysics',0xb7e00),('world_physics',0x1c7660),('object_postphysics',0xb7f50),('object_newflags',0xb8040)):
     complete=t.stage('original_'+stage,entry)
     assert not any(e.get('service')for e in t.player_stages[-1]['services'])
     if not complete:
      assert not trigger and stage=='object_newflags'and t.g.reg_read(UC_X86_REG_EIP)==0x1eff31
      break
    assert complete==trigger
    assert len(t.motion_requests)==2 and all('request154_after_hex'in r for r in t.motion_requests)
    before=t.player_stages[2]['world']['objects'][0]['root']['bits']
    assert t.state()['objects'][0]['root']['bits']==before
   authored=[]
   if spawn:
    bits=t.state()['objects'][0]['root']['bits']
    for i,q in enumerate(t.cache.block(t.cache.scenario+0x100,0x34)):
     raw=bytes(t.cache.data[q:q+0x34])
     if [f'{v:08x}'for v in struct.unpack('<3I',raw[:12])]==bits:
      authored.append(dict(index=i,file_offset=f'{q:08x}',address=f'{q+t.cache.magic:08x}',raw34_hex=raw.hex()))
    assert authored
   cases.append(dict(case=name,trigger_producer_included=trigger,preparation=t.producers,
    creation_input_hex=t.creation_input_hex,default_variant_hex=t.default_variant,
    source_selected_authored_starts=authored,stages=t.player_stages,player_calls=t.player_calls,
    player_and_object_stores=t.player_stores,motion_request_producers_consumers=t.motion_requests,
    original_calls=t.new_calls,resource_delivery=t.resource_delivery,resource_requests=t.resource_requests,
    raw_services=t.events,compatibility_platform_scope=t.platform_input,
    compatibility_lock_calls=t.lock_calls,physical_raw_services=t.physical_services,
    source_raw_impact_pool_allocations=t.os_pool_allocs,
    complete_ordinary_object_phases=bool(spawn and trigger),command_input_tail_entered=False))
   traces.append(t);print(json.dumps(dict(case=name,stages=len(t.player_stages),executed=len(t.executed))),flush=True)
 finally:
  _,err=proc.communicate('x',timeout=60)
 assert proc.returncode==0,err
 listings=[];executed=set().union(*(t.executed for t in traces));t=traces[0]
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
 def emit(name,ins):
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert all(bytes(x.g.mem_read(i.address,len(i.bytes)))==i.bytes for x in traces)
  p=directory/name;p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  return dict(file=p.name,sha256=digest(p),rows=len(ins),executed_rows=sum(i.address in executed for i in ins))
 executed_listing=emit('executed-original-instructions.asm',[t.code.at(a)for a in sorted(executed)])
 for entry in ENTRIES:
  routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
  listings.append(dict(entry=f'{entry:08x}',**emit(f'{entry:08x}.asm',ins),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 store_listing=emit('observed-source-store-sites.asm',[t.code.at(a)for a in sorted({int(r['instruction'],16)for x in traces for r in x.player_stores})])
 callbacks=[dict(index=i,address=f'{0x447428+i*36:08x}',raw24_hex=t.image.read(0x447428+i*36,36).hex())for i in (19,22,25,27,28,30,58,59)]
 manifest=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest()
 out=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
  cases=cases,original_callback_records=callbacks,engine_table_CRT_word=dict(address='00468c20',raw_hex=t.image.read(0x468c20,4).hex()),
  instruction_patches=[],new_supplied_gameplay_or_physics_results=[],instruction_listings=listings,
  instruction_manifest_sha256=manifest,executed_original_listing=executed_listing,observed_store_listing=store_listing,
  executed_original_addresses_verified=len(executed),summary=dict(cases=3,complete_source_join_cases=3,
   complete_source_spawn_loadout_cases=2,complete_genuine_bound_PVS_cases=2,complete_first_motion_cases=1,
   missing_trigger_original_stop_cases=1,complete_player_command_input_cases=0),
  boundaries=[
   'All earlier immutable suites, frozen whole compatibility generator, actual cache/resource bytes and declared raw OS backing remain read-only. Native compatibility platform inputs are retained whole; no hardware Xbox context parity or new game/Havok success return is claimed.',
   'Selected original CRT callbacks and init/reset/activation order are bounded preparation, not complete title/session/world-object startup. Original37EB40(table468C20) installs engine identities; record22 owns engine storage158BA0/reset158C10. Records28/30 own queue10D490/10D4F0 and trigger bitset1EFE10/1EFE70, removing inherited queue backing and preserving original requested capacities.',
   'Exact frozen source1A0A90 default variant is delivered as game-options input. Exact source controller0 published/host-effective90-byte property is delivered at creation+1C. Creation identity bytes are a declared zero-key nonlocal input with +2/-1 and +4/NONE; sign-in, lobby transport and complete session/game-global producers remain open. No player/current-unit/cluster store is supplied.',
   'Complete14D350/14D800/14D600/14D3D0 allocate the true playerEC700000 and sanitize representation2/handicap0. Complete1509C0 chooses from every unchanged authored Lockout player start, constructs the actual biped/SMG, binds ownership with14E240 and executes source loadout/attachment/animation/marker functions. Earlier111490/111500/1DC8F0/1DC940/FB560/EFEE0/C9A40 services are removed; optional attachment and sound boundaries remain explicitly recorded.',
   'Actual source-produced model/current-node/body/root inputs and real authoredXYZ are retained. The full original chooser and body creation execute with the existing source world, not a substituted result or ground/root adjustment. Complete source-world object allocation order and all chooser branch parity remain unproved.',
   'Original14EAE0..14ED0C preserves its real caller frame through14E9E0 membership eligibility and the current/previous unit parent-chain cluster writer. The null command-packet argument is not consumed before the exclusive stop; all command-input/action tail instructions after14ED0C remain excluded. No neutral or moving control, velocity, contact or head pose is fabricated.',
   'Complete13A380 invokes actual player/PVS consumers and original visibility/active callbacks; source cache cluster4 yields FFDF0000, original activation changes biped header4A to4B. No all-active mask, local view/user binding or forced-cluster input is supplied.',
   'Ordered B7E00→1C7660→B7F50→B8040 roots follow the original139750 object-phase order but exclude preceding/later subsystems and source tick increment. All four return in the complete case, including original1E9540 request producer andE3B20 consumer; their exact154-byte records are captured. No game/Havok service executes in these phases. This is one constructed/equipped owner and source-default command state, not a full139750 tick or hardware contact parity.',
   'The negative case omits only source trigger-bitset init/reset callbacks and stops at1EFF31 reading source-global526258=NULL after the real motion prefix; it retains all prior state. The joined/no-unit case keeps current/clusterNONE and yields zero PVS. Runtime model/body fields and actual scalar outputs remain source-owned in each case.',
   'Inherited time30Hz/1:30, RNG state, selected BSP/cache address mapping and compatibility storage/resource delivery inputs remain explicitly bounded. Local camera/view updates, authored equipment populations, full device/input dispatch and live transport remain open.'])
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 raw=(json.dumps(out,indent=2,sort_keys=True)+'\n').encode()
 if args.output.exists():assert args.output.read_bytes()==raw,('Refusing changed frozen report',str(args.output))
 else:args.output.write_bytes(raw)
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),manifest=manifest,executed=len(executed))),flush=True)
if __name__=='__main__':main()
