#!/usr/bin/env python3
"""Separate original live biped/world constructor investigation.

Earlier freezes are read-only. Actual owning storage initializers execute;
no object/body/pose/contact/readiness field or allocation success is supplied.
"""
import argparse,hashlib,json,os,subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ESP,UC_X86_REG_EIP
from oracle_mp_spawn_fixed_memory_world import FixedTrace,FROZEN as OLDER,CONTEXT_OUT,ROOT,ENTRIES as PRIOR_ENTRIES
from oracle_mp_inventory_1_1 import InventoryTrace
from oracle_smg_firing_1_1 import STACK,STOP,PIN,CACHE_PIN
from oracle_mp_spawn_biped_physics import PLACEMENT,RAW_HAVOK,RAW_COMPONENTS
from oracle_mp_spawn_process_heap_world import KernelBoundary
from functions import _trace

OUT=ROOT/'analysis/multiplayer/integration/spawn-source/live-body-world'
FROZEN={**OLDER,
 'tools/oracle_mp_spawn_fixed_memory_world.py':'d04c6f66b32cbbdf039a1c0f4bfa91748b6fe184c77b035d0e5ae64b7ae40d61',
 'docs/MP_FIXED_MEMORY_WORLD_RESEARCH.md':'68a1e2a428d9ec4cb591d0958f47214126b9a91a94f329147d7a62247a726f71',
 'analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json':'93c2f875b646ee1e298059237d1ba9f5e064153834d8188376bb101b9a7df75d'}
EXTRA=(0xb76b0,0xb76e0,0xb78c0,0xb7aa0,0x18fce0,0x18fd70,0x18fc80,0x125520,0x1ce5b0,0x1c5f00,0x1eef40,
 0x1d4f30,0x1d5210,0x315820,0x1c7660,0x1e8f60,0x14cc00,0x14ccd0,0x1cb180,
 0x1cb990,0x1c5fd0,0x1eefe0,0x139390,0x139410,0x1394b0,0x13a380)
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+EXTRA))
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

class LiveTrace(FixedTrace):
 def __init__(self,data):
  self.live_calls=[];self.live_stages=[];self.live=True;self.actual_heap=False
  super().__init__(data)
 def hook(self,g,a,size,user):
  if self.instrument and a in EXTRA:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.live_calls.append(dict(phase=self.active,entry=f'{a:08x}',registers=self.registers(),
    return_address=f'{self.u(sp):08x}',stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)]))
  if self.actual_heap and a in (0x18fce0,0x18fd70):
   # The inherited WorldTrace observer writes supplied body pointers here.
   # Bypass only that observer; execute actual unchanged heap instructions.
   InventoryTrace.hook(self,g,a,size,user);return
  if self.instrument and a==0x33785c:
   sp=g.reg_read(UC_X86_REG_ESP);size=self.u(sp+8)
   if size in (0x1450,0x850):
    args=[self.u(sp+4+i*4)for i in range(4)]
    assert args[0]==0 and args[2:]==[0x101000,4]
    cap=(size+4095)&~4095;p=self.pool_next;self.pool_next+=cap;g.mem_map(p,cap)
    self.os_pool_allocs.append(dict(original_return_site=f'{self.u(sp):08x}',args=[f'{v:08x}'for v in args],
     supplied_raw_backing=f'{p:08x}',mapped_capacity=cap,scope='retained raw source-requested impact pool storage ABI'))
    self.executed.add(a);g.reg_write(UC_X86_REG_EAX,p);g.reg_write(UC_X86_REG_ESP,sp+20);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
   assert size in (0x1409b,0x106470),hex(size)
   backing=RAW_COMPONENTS if size==0x1409b else RAW_HAVOK
   self.services[a]=('retained raw OS backing for exact original initializer request',backing,16)
  if a==0xb8a30:
   # FixedTrace's allocation evidence uses the same Python attribute name as
   # the older supplied-object queue. Hide only that observer queue at entry;
   # no guest state, returned datum or instruction is altered.
   records=self.allocations;self.allocations=[]
   try:super().hook(g,a,size,user)
   finally:self.allocations=records
   return
  super().hook(g,a,size,user)
 def arena_root(self):
  self.call('original_physical_arena_root',0x12cb70)
  self.active='original_game_arena_except_unprovided_savegame_io'
  self.g.reg_write(UC_X86_REG_ESP,STACK)
  self.g.emu_start(0x125280,0x1252ba,count=2000000)
  assert self.g.reg_read(UC_X86_REG_EIP)==0x1252ba
  self.g.emu_start(0x1252bf,STOP,count=2000000)
  assert self.g.reg_read(UC_X86_REG_EIP)==STOP
  self.producers.append(dict(name=self.active,original_fragments=[['00125280','001252ba'],['001252bf','00125339']],
   excluded_original_call=['001252ba','001252bf'],excluded_callee='00217090',frame_preserved=True))
 def state(self):
  pool=self.u(0x4e78d0);data=self.u(pool+0x44);count=self.u(pool+0x3c);n=self.u(pool+0x38)
  objects=[]
  for i in range(n):
   h=data+i*12;p=self.u(h+8)
   if not p:continue
   component=self.u(p+0xb4);body_records=[];component_hex=None
   if component!=0xffffffff:
    cp=self.u(self.u(0x526248)+0x44)+(component&0xffff)*0xa0
    component_hex=bytes(self.g.mem_read(cp,0xa0)).hex()
    slots,number=self.u(cp+0x70),self.u(cp+0x74)
    assert number<100,number
    for j in range(number):
     slot=slots+j*0x60;body=self.u(slot+0x40)
     body_records.append(dict(slot=f'{slot:08x}',slot_hex=bytes(self.g.mem_read(slot,0x60)).hex(),
      body=f'{body:08x}',body_hex=bytes(self.g.mem_read(body,0xa0)).hex()if body else None,
      world=f'{self.u(body+8):08x}'if body else None,
      motion=f'{self.u(body+0x3c):08x}'if body else None,
      motion_prefix90_hex=bytes(self.g.mem_read(self.u(body+0x3c),0x90)).hex()if body else None,
      transform30_hex=bytes(self.g.mem_read(self.u(body+0x14),0x30)).hex()if body else None))
   objects.append(dict(index=i,datum=f'{self.u(h,2)<<16|i:08x}',header_hex=bytes(self.g.mem_read(h,12)).hex(),
    pointer=f'{p:08x}',definition=f'{self.u(p):08x}',size=self.u(h+6,2),root=self.vec(p+0x64),
    component=f'{component:08x}',component_hex=component_hex,
    raw_hex=bytes(self.g.mem_read(p,self.u(h+6,2))).hex(),bodies=body_records))
  game=self.u(0x4edf18);world=self.u(0x526234);pp=self.u(0x4f01f4)
  return dict(pool=f'{pool:08x}',pool_count=count,pool_high_water=n,objects=objects,
   arena_cursor=self.u(0x4ed654),body_heap=f'{self.u(0x4e58bc):08x}',object_world=f'{self.u(0x4e58c4):08x}',
   object_world_hex=bytes(self.g.mem_read(self.u(0x4e58c4),0x84)).hex(),
   physics_counter=f'{self.u(0x526230):08x}',physics_counter_value=self.u(self.u(0x526230)),
   physics_constants=f'{self.u(0x526254):08x}',physics_constants_hex=bytes(self.g.mem_read(self.u(0x526254),0x1c)).hex(),
   player_pool=f'{pp:08x}',player_pool_header_hex=bytes(self.g.mem_read(pp,0x4c)).hex(),player_count=self.u(pp+0x3c),
   source_visible_mask_hex=bytes(self.g.mem_read(game+0x1138,0x40)).hex(),
   source_active_mask_hex=bytes(self.g.mem_read(game+0x11b8,0x40)).hex(),
   world=f'{world:08x}',world260_hex=bytes(self.g.mem_read(world,0x260)).hex()if world else None)
 def attempt(self,name,entry,args=()):
  self.active=name;self.new_invalid=None;start=len(self.events)
  try:self.run(entry,args);completion='complete_original_return'
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;completion='blocked_original_instruction'
  self.live_stages.append(dict(name=name,entry=f'{entry:08x}',completion=completion,
   stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
   invalid=self.new_invalid,state=self.state(),services=self.events[start:]))
  return completion=='complete_original_return'

def prepare(data):
 t=LiveTrace(data);t.heap_prefix(None)
 t.call('original_CRT_lock_initializer',0x32cffc);t.call('original_CRT_onexit_initializer',0x3274e4)
 t.register();t.call('original_scratch_CRT_constructor',0x37d8c0)
 t.fixup();t.arena_root()
 t.actual_heap=True
 for a in (0x18fce0,0x18fd70,0x246a50):t.services.pop(a,None)
 # Ascending selected init callbacks from original447428, stride24:
 # player19, constants25, objects27, AI58prefix, physical59.
 t.call('original_source_empty_player_pool',0x14cc00)
 t.call('original_physics_constant_storage_initializer',0x1eef40)
 t.call('original_full_object_storage_initializer',0xb76b0)
 t.fragment('original_AI_storage_and_reset_prefix',0x1cb180,0x1cb1f4)
 t.producers.append(dict(name=t.active,original_fragment=['001cb180','001cb1f4'],stop_is_exclusive=True,
  completion='complete_original_fragment',full_AI_initializer_excluded_tail=True))
 t.call('original_physics_storage_initializer',0x1c5f00)
 # Ascending selected map-reset callbacks from original447430.
 t.call('original_empty_player_map_reset',0x14ccd0)
 t.call('original_physics_constants_map_reset',0x1eefe0)
 t.call('original_object_map_reset',0xb78c0)
 t.call('original_BSP_datum_table_map_reset',0x1c5fd0)
 # The actual activation writer clears all three mask banks. Preserve this
 # original prefix; other activation callbacks are explicitly excluded.
 t.fragment('original_activation_mask_zero_prefix',0x1394b0,0x1394f0)
 t.producers.append(dict(name=t.active,original_fragment=['001394b0','001394f0'],stop_is_exclusive=True,
  completion='complete_original_fragment',full_activation_loop_excluded=True))
 t.call('original_object_map_activation',0xb7aa0)
 t.call('original_full_new_map_order',0x1c5ff0)
 t.call('original_current_cluster_masks_with_empty_player_pool',0x13a380)
 assert t.u(t.u(0x4f01f4)+0x3c)==0
 assert bytes(t.g.mem_read(t.u(0x4edf18)+0x1138,0x40))==bytes(0x40)
 assert bytes(t.g.mem_read(t.u(0x4edf18)+0x11b8,0x40))==bytes(0x40)
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 args.output.parent.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 try:
  data=json.loads(proc.stdout.readline());(args.output.parent/'raw-host-context.json').write_text(json.dumps(data,indent=2)+'\n')
  setup=json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/placement-report.json').read_text())
  traces=[];cases=[]
  for index in (None,0,32):
   t=prepare(data);r=None;before=t.state()
   if index is not None:
    r=next(x for x in setup['normal_setup']if x['version']=='tu11'and x['authored_index']==index)
    t.g.mem_write(PLACEMENT,bytes.fromhex(r['placement_hex']))
    assert t.attempt('original_authored_biped_creation',0xb8a30,(PLACEMENT,))
    created=t.live_stages[-1]['state']['objects'][0]
    assert created['datum']=='e26f0000'and created['size']==5750 and created['component']=='e1680000'
    assert created['root']['bits']==r['position']['bits']
    assert not int(created['header_hex'][4:6],16)&1
    assert created['bodies'][0]['world']==before['world']=='06005470'
   assert t.attempt('original_source_inactive_world_step',0x1c7660)
   assert not t.live_stages[-1]['services']
   assert not [c for c in t.new_calls if c['entry'] in ('000e3b20','001e9540')]
   cases.append(dict(case='empty_source_world_step'if index is None else f'authored_biped_{index}_inactive_world_step',
    authored_index=index,source_placement=r,preparation=t.producers,before=before,stages=t.live_stages,
    calls=t.live_calls,original_calls=t.new_calls,resources=t.resource_requests,raw_services=t.events,
    compatibility_lock_calls=t.lock_calls,compatibility_initializer_calls=t.compat_calls,
    compatibility_platform_scope=t.platform_input,physical_raw_services=t.physical_services,
    source_raw_impact_pool_allocations=t.os_pool_allocs,source_scratch_record_stores=t.scratch_stores,
    retained_fixed_registry=t.registry(),physical_counter_value=t.u(t.u(0x526230))))
   traces.append(t)
 finally:
  _,err=proc.communicate('x',timeout=60)
 assert proc.returncode==0,err
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 directory=args.output.parent/'instructions';directory.mkdir(exist_ok=True)
 executed=set().union(*(x.executed for x in traces));listings=[]
 for a in executed:
  i=t.code.at(a);assert t.image.read(a,len(i.bytes))==i.bytes
  assert all(bytes(x.g.mem_read(a,len(i.bytes)))==i.bytes for x in traces)
 p=directory/'executed-original-instructions.asm'
 p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in(t.code.at(a)for a in sorted(executed))))
 executed_listing=dict(file=p.name,sha256=digest(p),rows=len(executed))
 for entry in ENTRIES:
  routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert all(bytes(x.g.mem_read(i.address,len(i.bytes)))==i.bytes for x in traces)
  p=directory/f'{entry:08x}.asm'
  p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append(dict(entry=f'{entry:08x}',file=p.name,sha256=digest(p),instructions=len(ins),
   executed=sum(i.address in executed for i in ins),direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 callbacks=[dict(index=i,address=f'{0x447428+i*36:08x}',original24_hex=t.image.read(0x447428+i*36,36).hex())for i in (19,25,27,58,59)]
 d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
  cases=cases,original_callback_records=callbacks,instruction_patches=[],new_supplied_game_or_Havok_fields=[],
  instruction_listings=listings,executed_original_listing=executed_listing,executed_original_addresses_verified=len(executed),
  instruction_manifest_sha256=hashlib.sha256(json.dumps(listings,sort_keys=True,separators=(',',':')).encode()).hexdigest(),
  summary=dict(cases=3,complete_actual_biped_constructors=2,complete_inactive_world_steps=3,ordinary_player_motion_contact_cases=0),
  boundaries=[
   'Frozen whole native compatibility KPCR/ETHREAD/host stack/process generator is read unchanged, kept alive and transferred whole. All prior raw process-heap, physical backing, cache/tag/resource and optional attachment/sound boundaries remain; no Xbox hardware parity is claimed.',
   'Selected actual init callbacks retain recovered ascending order19 empty player pool,25 physics constants,27 complete object storage,58 AI storage/reset prefix,59 complete physical storage. Selected reset callbacks19/25/27/59 and activation27/59 retain their own original table order. Other callbacks and whole title/session map initialization remain excluded, not replaced with success.',
   'Actual125280 frame is retained around exact excluded217090 I/O call. Original1394B0..1394F0 clears all three mask banks; original13A380 recomputes zero visible/active masks from source-created empty player membership and source-produced AI storage/reset prefix. No inherited FF or manually selected active mask owns admission.',
   'CompleteB76B0 owns original2048x12 object header pool,1MB body heap,84-byte object world,A00 scratch and original2048-entry32/12 collidable/noncollidable registries. Original1EEF40 owns1C physics-constant storage; original1C5F00 owns4-byte physical counter, BSP table, impact/component pools,raw Havok allocation and fallback record.',
   'The retained raw VirtualAlloc service is generalized only to the exact same original1450/850 impact-pool requests inside the full owning1C5F00 caller. Original1409B component and106470 Havok requests use the same declared raw backing services. Only original constructors populate storage; no body or allocation success is supplied.',
   'Only Python supplied-object/heap observers are excluded atB8A30/18FCE0/18FD70. The source allocator and growth instructions execute unchanged; guest registers/memory/returns are never changed by these observer exclusions.',
   'ActualB8A30 createsE26F0000 from frozen source-generated authored-player descriptors0/32; all5750 body bytes, runtime animation nodes, physical componentE1680000 and Havok body/motion/world insertion come from original instructions and actual cache/resource bytes. Player current-unit binding, loadout and ownership are not entered.',
   'Allocated biped headers lack activebit1 in the source-empty-player context. Complete1C7660 steps the real created world with no supplied game/Havok return service; ordinary character E3B20/1E9540 contact producer is not entered. This is inactive-body world-step completion, not ordinary live FFA player motion/contact or a complete139750 tick.',
   'The inherited supplied session/options/tick inputs remain explicit; complete game-global allocation/session producers and all intermediate startup callbacks are not established. Source-created capacities and original fixed15000 bytes are retained. Fallback backing518434 remains NULL; no fallback allocation is requested.',
   'All29 earlier immutable tool/document/report/native producer pins and production remain unchanged.'])
 args.output.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),cases=3,manifest=d['instruction_manifest_sha256'],executed=len(executed))))
if __name__=='__main__':main()
