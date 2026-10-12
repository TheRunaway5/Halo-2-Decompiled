#!/usr/bin/env python3
"""Independent unchanged original item-current-state producer investigation.

Extends the frozen scratch-backed source-empty world. It does not supply
placement, contact, node pose, allocation success or pickup admission.
"""
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP
from oracle_mp_spawn_fixed_memory_world import FixedTrace, FROZEN as OLDER, CONTEXT_OUT, ENTRIES as PRIOR_ENTRIES
from oracle_mp_inventory_1_1 import ROOT, PIN, CACHE_PIN, InventoryTrace
from oracle_smg_firing_1_1 import STACK, STOP, GAME, TIME, SEED
from functions import _trace

OUT = ROOT/'analysis/multiplayer/integration/inventory-source/item-current-producers'
FROZEN = {**OLDER,
 'tools/oracle_mp_spawn_fixed_memory_world.py':'d04c6f66b32cbbdf039a1c0f4bfa91748b6fe184c77b035d0e5ae64b7ae40d61',
 'docs/MP_FIXED_MEMORY_WORLD_RESEARCH.md':'68a1e2a428d9ec4cb591d0958f47214126b9a91a94f329147d7a62247a726f71',
 'analysis/multiplayer/integration/spawn-source/fixed-memory-world/report.json':'93c2f875b646ee1e298059237d1ba9f5e064153834d8188376bb101b9a7df75d'}
TRANSFER=ROOT/'analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/report.json'
assert hashlib.sha256(TRANSFER.read_bytes()).hexdigest()=='22d0a03d4256e1a3ed7f8099cfa0b7f9aa8ac655e5e57e39ddf41395fa0d7f23'
FROZEN.update(json.loads(TRANSFER.read_text())['source_pins'])
FROZEN.update({
 'tools/oracle_mp_pickup_transfer_measured_1_1.py':'e22428cddbb685b8496b52774ad87ae9430febdf14164bc665fdf9f44c061fed',
 'tools/oracle_mp_pickup_transfer_measured_audit_1_1.py':'8e00e7077b0879d0856549379c101b9bd9a0c9f20ab1df5e300881284e7a4ebf',
 'docs/MP_PICKUP_TRANSFER_MEASURED_RESEARCH.md':'6b715bb92a1757d1f2ff7752a952a429073aeaaf6b92ffae9c6e2e3a0ba3746b',
 'analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/report.json':'22d0a03d4256e1a3ed7f8099cfa0b7f9aa8ac655e5e57e39ddf41395fa0d7f23',
 'analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/manifest.json':'69f76789503370fb268add2d046b39e7556834f185d3eff8274755a6384cfac7',
 'analysis/multiplayer/integration/inventory-source/pickup-transfer-measured/audit.json':'0a91940be2dc6fda98c4143f7966eb2795e96452cdf3451fa2d535e2bbb0f5db'})
EXTRA=(0xb76b0,0x125520,0x18fc80,0x18fce0,0x18fd70,0x18ff90,0x190030,0x1900c0,
 0x1ce5b0,0x29c520,0xb78c0,0xb7aa0,0xc0bb0,0xc0f50,0xc0ef0,0xd5e90,
 0x13a380,0x13a2a0,0x151ec0,0x1cd540,0x13a220,0xb94f0,0xb96a0,0xb7c50,
 0xbdf10,0xbdf80,0xbede0,0x15c110,0x15c320,0x15c370,0xb8040,0xbd360,0xbd710,
 0xf9b10,0xf9ea0,0x1c7050,0x1c6f50,0x1c7660,0x16b240,0x246a50,0xb7e00,0xb7f50,
 0x1ce650,0x1ce7e0,0x16d2c0,0x1d0a60,0x1cb180,0x1cb990,0x14cc00,0x1eef40,
 0xb8a30,0xbd170,0xbd270,0x11d4a0,0x10cdf0,0x13fb50,0xd5e30,0x10ca70,0xc0e50,
 0x139390,0x139410,0x1394b0,0x1c5f00,0x1c5fd0)
ENTRIES=tuple(dict.fromkeys(PRIOR_ENTRIES+EXTRA))
STATE_OWNERS={0xb8a30,0xbd170,0xbd270,0xbdf10,0xbdf80,0xbede0,0x11d4a0,0xb94f0,0xb96a0,0x1ce650,0xbd360,0xbd710,0xf9b10,0xf9ea0,0x16b240,0x246a50,0xb7c50}

def digest(path):
 return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def encoded(value):
 return (json.dumps(value,sort_keys=True,indent=2)+'\n').encode()

def frozen_write(path,raw):
 if path.exists():assert path.read_bytes()==raw,('Refusing changed evidence',str(path))
 else:path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(raw)

def instruction_evidence(traces,out):
 t=traces[0];executed=set().union(*(x.executed for x in traces));result=[]
 for a in executed:
  i=t.code.at(a);assert t.image.read(a,len(i.bytes))==i.bytes
  assert all(bytes(x.g.mem_read(a,len(i.bytes)))==i.bytes for x in traces)
 def listing(name,ins):
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode()
  frozen_write(out/'instructions'/name,raw)
  return dict(file='instructions/'+name,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),executed_rows=sum(i.address in executed for i in ins))
 for entry in ENTRIES:
  routine=_trace(t.code,entry,t.starts|set(ENTRIES));ins=[t.code.at(a)for a in sorted(routine.code)]
  for i in ins:
   assert t.image.read(i.address,len(i.bytes))==i.bytes
   assert all(bytes(x.g.mem_read(i.address,len(i.bytes)))==i.bytes for x in traces)
  result.append(dict(entry=f'{entry:08x}',**listing(f'{entry:08x}.asm',ins),
   direct_calls=[dict(site=f'{a:08x}',target=f'{b:08x}')for a,b,k in routine.sites if k=='call']))
 result.append(dict(kind='executed_original_instructions',**listing('executed-original-instructions.asm',[t.code.at(a)for a in sorted(executed)])))
 sites=sorted({int(r['instruction'],16)for x in traces for r in x.current_stores})
 result.append(dict(kind='observed_original_store_sites',**listing('observed-store-sites.asm',[t.code.at(a)for a in sites])))
 return result

def metadata(t):
 c=t.cache;rows=[];models=[]
 for row in (0,1,12):
  p=c.block(c.scenario+0x120,0x90)[row]
  rows.append(dict(index=row,file_offset=f'{p:08x}',address=f'{p+c.magic:08x}',record_hex=bytes(c.data[p:p+0x90]).hex()))
 for name in ('objects\\weapons\\rifle\\smg\\smg','objects\\weapons\\rifle\\battle_rifle\\battle_rifle'):
  w=next(v for v in c.tags.values()if v['group']=='weap'and v['name']==name)
  m=c.tags[c.u(w['offset']+0x38)];r=c.tags[c.u(m['offset']+4)]
  models.append(dict(weapon=w,model=m,render_model=r,animation_graph=f'{c.u(m["offset"]+0x14):08x}',
   cache_magic=f'{c.magic:08x}',render_nodes=[dict(index=n,file_offset=f'{p:08x}',address=f'{p+c.magic:08x}',
    raw_hex=bytes(c.data[p:p+0x60]).hex())for n,p in enumerate(c.block(r['offset']+0x48,0x60))]))
 callbacks=[]
 for index in (19,25,27,58,59):
  p=0x447428+index*0x24;raw=t.image.read(p,0x24)
  callbacks.append(dict(index=index,address=f'{p:08x}',raw_original_hex=raw.hex(),
   words=[f'{int.from_bytes(raw[n:n+4],"little"):08x}'for n in range(0,0x24,4)]))
 return dict(rows=rows,models=models,selected_source_callback_records=callbacks)

class CurrentTrace(FixedTrace):
 def __init__(self,data):
  self.current_calls=[];self.current_returns=[];self.current_stages=[];self.current_stores=[];self.current_states=[];self.current_state_ids={};self.current_order=0
  self.watch_current=False
  self.resource_delivery=[]
  super().__init__(data)
 def measured_state(self):
  value=self.current_snapshot();raw=json.dumps(value,sort_keys=True,separators=(',',':')).encode();h=hashlib.sha256(raw).hexdigest()
  if h not in self.current_state_ids:
   self.current_state_ids[h]=len(self.current_states);self.current_states.append(dict(id=len(self.current_states),sha256=h,**value))
  return self.current_state_ids[h]
 def observe_returns(self,a):
  for ret,sp,row in reversed(self.current_returns[:]):
   if a==ret and row['phase']==self.active:
    row['returned_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
    row['return_esp_delta']=self.g.reg_read(UC_X86_REG_ESP)-sp
    row['return_order']=self.current_order;self.current_order+=1
    if int(row['entry'],16) in STATE_OWNERS:row['after_state']=self.measured_state()
    self.current_returns.remove((ret,sp,row));break
 def item_objects(self):
  p=self.u(0x4e78d0);data=self.u(p+0x44);bits=self.u(p+0x48)
  result=[]
  for n in range(self.u(p+0x38)):
   if self.u(bits+(n>>5)*4)&1<<(n&31)==0:continue
   h=data+n*12;b=self.u(h+8);length=self.u(h+6,2)
   row=dict(datum=f'{self.u(h,2)<<16|n:08x}',header_address=f'{h:08x}',header_hex=bytes(self.g.mem_read(h,12)).hex(),
    body_address=f'{b:08x}',body_size=length,body_hex=bytes(self.g.mem_read(b,length)).hex())
   if b and length>=0x12c:
    row.update(definition=f'{self.u(b):08x}',flags=f'{self.u(b+4):08x}',root=self.vec(b+0x64),
     bounds_hex=bytes(self.g.mem_read(b+0x30,0x30)).hex(),location_hex=bytes(self.g.mem_read(b+0x28,8)).hex(),
     physical_component=f'{self.u(b+0xb4):08x}',runtime_flags=f'{self.u(b+0xc0,2):04x}',
     parent=f'{self.u(b+0x14):08x}',node_blocks=[])
    for off in (0x110,0x114,0x118):
     size,offset=self.u(b+off,2),self.u(b+off+2,2)
     assert size==0 or (offset!=0xffff and offset+size<=length),(hex(b),off,size,offset,length)
     row['node_blocks'].append(dict(field=f'{off:03x}',size=size,offset=offset,
      raw_hex=bytes(self.g.mem_read(b+offset,size)).hex() if size else None))
    if self.u(h+3,1)==2:
     row.update(owner=f'{self.u(b+0x154):08x}',item_flags=f'{self.u(b+0x12c,2):04x}',
      contact_hex=bytes(self.g.mem_read(b+0x130,12)).hex(),row=self.u(b+0xaf,1),
      heat_bits=f'{self.u(b+0x184):08x}',loaded=self.u(b+0x22c,2),reserve=self.u(b+0x22a,2))
   result.append(row)
  return result
 def registry_chains(self):
  out=[]
  for cluster in range(self.u(self.u(0x4e7918)+0x9c)):
   row=dict(cluster=cluster)
   for name,globalp in (('collidable',0x4e58b0),('noncollidable',0x4e58a4)):
    heads,pool=self.u(globalp),self.u(globalp+4);link=self.u(heads+cluster*4);chain=[]
    while link!=0xffffffff:
     assert len(chain)<2048
     p=self.u(pool+0x44)+(link&0xffff)*self.u(pool+0x24)
     chain.append(dict(link=f'{link:08x}',object=f'{self.u(p+4):08x}',next=f'{self.u(p+8):08x}',raw_hex=bytes(self.g.mem_read(p,32)).hex()))
     link=self.u(p+8)
    row[name]=chain
   out.append(row)
  return out
 def weapon_resources(self):
  resident=max((int(r['resident_pointer'],16)+r['size']+4095)&~4095 for r in self.resident.values())
  with (ROOT/'assets/disc/maps/shared.map').open('rb') as shared:
   for tag in self.cache.tags.values():
    if tag['group']!='jmad' or tag['name'] not in ('objects\\weapons\\rifle\\smg\\smg','objects\\weapons\\rifle\\battle_rifle\\battle_rifle'):continue
    for index,off in enumerate(self.cache.block(tag['offset']+0xac,20)):
     size,pointer=self.cache.u(off+4),self.cache.u(off+8);location,at=pointer>>30,pointer&0x3fffffff
     assert location in (0,2)
     if location==2:shared.seek(at);raw=shared.read(size)
     else:raw=bytes(self.cache.data[at:at+size])
     assert len(raw)==size
     self.g.mem_write(resident,raw)
     record=dict(index=index,descriptor=f'{off+self.cache.magic:08x}',descriptor_hex=bytes(self.cache.data[off:off+20]).hex(),
      size=size,location=location,file_offset=at,file='assets/disc/maps/shared.map' if location==2 else 'assets/disc/maps/lockout.map',
      resident_pointer=f'{resident:08x}',raw_sha256=hashlib.sha256(raw).hexdigest(),graph=tag['name'])
     self.resident[off+self.cache.magic]=record;self.resource_delivery.append(record)
     resident+=(size+4095)&~4095
  assert resident<=0x8000000
 def current_snapshot(self):
  p=self.u(0x4e58c4);h=self.u(0x4e58bc)
  regs={}
  for g in (0x4e58b0,0x4e58a4):
   head,forward,reverse=(self.u(g+i*4)for i in range(3))
   regs[f'{g:08x}']=dict(head=f'{head:08x}',forward=f'{forward:08x}',reverse=f'{reverse:08x}',
    head_hex=bytes(self.g.mem_read(head,0x800)).hex() if head else None,
    pools=[dict(pointer=f'{q:08x}',header_hex=bytes(self.g.mem_read(q,0x4c)).hex())for q in (forward,reverse) if q])
  return dict(game_arena_base=f'{self.u(0x4ed650):08x}',game_arena_cursor=self.u(0x4ed654),
   body_heap=f'{h:08x}',body_heap_header_hex=bytes(self.g.mem_read(h,0x40)).hex() if h else None,
   object_world=f'{p:08x}',object_world_hex=bytes(self.g.mem_read(p,0x84)).hex() if p else None,
   object_scratch=f'{self.u(0x4e58a0):08x}',object_pool=self.pool_snapshot(),cluster_registries=regs,
   visible_mask_hex=bytes(self.g.mem_read(GAME+0x1138,0x40)).hex(),
   active_mask_hex=bytes(self.g.mem_read(GAME+0x11b8,0x40)).hex(),
   AI_global=f'{self.u(0x4fcda8):08x}',AI_global_hex=bytes(self.g.mem_read(self.u(0x4fcda8),0x374)).hex() if self.u(0x4fcda8) else None,
   item_physics_constants=f'{self.u(0x526254):08x}',item_physics_constants_hex=bytes(self.g.mem_read(self.u(0x526254),0x1c)).hex(),
   shared_rng=f'{self.u(SEED):08x}',tick=self.u(TIME+8),objects=self.item_objects(),
   registry_chains=self.registry_chains() if self.u(p) else None)
 def hook(self,g,a,size,user):
  if self.watch_current:
   self.observe_returns(a)
   if a in EXTRA:
    sp=g.reg_read(UC_X86_REG_ESP)
    row=dict(phase=self.active,entry=f'{a:08x}',return_address=f'{self.u(sp):08x}',registers=self.registers(),
     stack=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],order=self.current_order)
    self.current_order+=1
    if a in STATE_OWNERS:row['before_state']=self.measured_state()
    if a==0xb8a30:row['creation_descriptor_hex']=bytes(g.mem_read(self.u(sp+4),0xc4)).hex()
    self.current_calls.append(row)
    # Caller stack depth is separately captured; return-site observation is
    # independent of any assumed calling convention.
    self.current_returns.append((self.u(sp),sp,row))
  if self.watch_current and a in (0x18fce0,0x18fd70):
   # The frozen WorldTrace observer writes supplied heap pointers at these
   # entries even without its service. Bypass that observer only. The base
   # unchanged-instruction runner and independent read-only observer remain.
   InventoryTrace.hook(self,g,a,size,user)
   return
  if self.watch_current and a==0xb8a30:
   # FixedTrace's read-only Havok allocation records share a Python attribute
   # name with InventoryTrace's old supplied-object queue. Disable that queue
   # interpretation during this single observer dispatch, without guest writes.
   saved=self.allocations;self.allocations=[]
   try:super().hook(g,a,size,user)
   finally:self.allocations=saved
   return
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.watch_current:
   base=self.u(0x4ed650);cursor=self.u(0x4ed654)
   watched=(0x4e58a0<=address<0x4e58c8 or address==0x4ed654 or GAME+0x1138<=address<GAME+0x11f8)
   watched|=base<=address<base+cursor if base else False
   if watched:
    self.current_stores.append(dict(phase=self.active,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',
     address=f'{address:08x}',size=size,before=bytes(g.mem_read(address,size)).hex(),
     value=f'{value&((1<<(size*8))-1):0{size*2}x}',order=self.current_order))
    self.current_order+=1
  super().mem_write(g,access,address,size,value,user)
 def attempt(self,name,entry,args=(),regs=(),stop=STOP):
  self.active=name;self.new_invalid=None;begin=len(self.events);self.count=0
  before=self.current_snapshot()
  try:
   if stop==STOP:self.run(entry,args,regs)
   else:
    self.g.reg_write(UC_X86_REG_ESP,STACK)
    for r,v in regs:self.g.reg_write(r,v)
    self.g.emu_start(entry,stop,count=2000000)
    assert self.g.reg_read(UC_X86_REG_EIP)==stop
   completion='complete_original_return' if stop==STOP else 'complete_original_fragment'
   self.observe_returns(stop)
  except unicorn.UcError:
   assert self.new_invalid is not None
   completion='blocked_original_instruction'
  row=dict(stage=name,entry=f'{entry:08x}',stop_exclusive=f'{stop:08x}',arguments=[f'{x:08x}'for x in args],
   input_registers={str(r):f'{v:08x}'for r,v in regs},completion=completion,
   stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',invalid=self.new_invalid,
   before=before,after=self.current_snapshot(),services=self.events[begin:])
  self.current_stages.append(row)
  return completion.startswith('complete_')

def prepare(data):
 t=CurrentTrace(data);t.heap_prefix(None)
 t.call('original_CRT_lock_initializer',0x32cffc)
 t.call('original_CRT_onexit_array_initializer',0x3274e4)
 t.register();t.call('original_game_allocator_CRT_constructor_and_atexit',0x37d8c0)
 t.components();t.game_arena();t.object_pool();t.call('original_impact_pool_initializer',0x2284c0)
 t.memory_backing();t.call('original_full_new_map_order',0x1c5ff0)
 assert t.u(0x4ed654)==0x9318 and t.u(0x526234)==0x6005470
 t.watch_current=True
 for p in (0x18fce0,0x18fd70,0x246a50):t.services.pop(p,None)
 return t

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--case',choices=('all','whole','tail'),default='all');args=ap.parse_args()
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 args.output.parent.mkdir(parents=True,exist_ok=True)
 env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(CONTEXT_OUT/'wine-prefix'))
 proc=subprocess.Popen(['wine',str(CONTEXT_OUT/'generator.exe')],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
 try:
  data=json.loads(proc.stdout.readline());traces=[]
  for full in ((False,True) if args.case=='all' else (args.case=='whole',)):
   t=prepare(data);t.current_case='whole_object_initializer' if full else 'tail_without_prior_object_setup';traces.append(t)
   stages=[('original_whole_object_initializer',0xb76b0,STOP)] if full else [('original_dynamic_heap_world_registry_tail',0xb76e0,0xb77a6)]
   stages += [('original_object_map_reset',0xb78c0,STOP),('original_object_map_activation',0xb7aa0,STOP),('original_active_cluster_masks_missing_AI_state',0x13a380,STOP)]
   for name,entry,stop in stages:
    if not t.attempt(name,entry,stop=stop):break
   if full:
    for name,entry,stop in [('original_AI_global_storage_and_reset_prefix',0x1cb180,0x1cb1f4),
     ('original_source_empty_player_pool',0x14cc00,STOP),('original_active_cluster_masks_with_source_empty_player_pool',0x13a380,STOP),
     ('original_item_physics_constants_storage',0x1eef40,STOP)]:
     if not t.attempt(name,entry,stop=stop):break
    else:
     t.weapon_resources()
     stages=[('original_netgame_row_initialization',0x15c320,(),((UC_X86_REG_ESI,t.entries),)),
      ('original_netgame_round_reset',0x15c370,(),((UC_X86_REG_EDI,t.entries),))]
     stages += [(f'original_authored_weapon_row_{row}_allocation',0x15c110,(t.entries,row),())for row in (0,1,12)]
     stages += [('ordinary_object_pass_before_first_postpass',0xb7e00,(),()),
      ('dirty_object_pass_before_first_postpass',0xb7f50,(),()),('new_object_first_normal_item_tick',0xb8040,(),())]
     for name,entry,args_,regs in stages:
      if not t.attempt(name,entry,args=args_,regs=regs):break
 finally:
  _,err=proc.communicate('x',timeout=60)
 assert proc.returncode==0,err
 listings=instruction_evidence(traces,args.output.parent)
 for p,h in FROZEN.items():assert digest(ROOT/p)==h,p
 d=dict(schema=1,source_sha256=PIN,cache_sha256=CACHE_PIN,oracle_sha256=digest(__file__),frozen_pins=FROZEN,
  instruction_patches=[],instructions=listings,source_metadata=metadata(traces[0]),
  boundaries=[
   'Read-only import of frozen FixedTrace. A new actual Wine32 compatibility producer worker remains alive. Earlier raw OS backing, compiled compatibility locks/context and startup exclusions persist; not original Xbox kernel execution.',
   'The prior original FixedTrace source-empty world is prepared first. New whole B76B0/reset/activation after it is an explicitly selected producer fixture, not authored title callback order. Literal original callback records and their ascending original dispatchers are exported separately.',
   'Tail case runs original B76E0..B77A6 exclusive and then fails atD5E98 because earlier widget constructor is omitted. Whole case executes originalB76B0 including originalD5E30/10CA70/C0E50, then originalB78C0/B7AA0. No supplied allocator, growth, heap pointer, world-active flag, object header or registry record is used after this source preparation.',
   'The new observer bypasses only inherited Python observer side effects at18FCE0/18FD70; actual original allocator bytes execute. AtB8A30 it suspends the conflicting Python preconstructed-object queue interpretation while retaining actual source execution. These are observer changes, not guest memory/register/instruction changes.',
   'First13A380 attempt fails at1CD56D because original AI-global storage is absent. Original1CB180..1CB1F4 provides AI storage/reset but later AI constructors are excluded. Original14CC00 provides an empty player datum pool. Complete13A380 then produces actual zero masks from that bounded state; it does not prove normal connected FFA player/PVS state.',
   'Original1EEF40 owns actual1C-byte item-physics constant storage. Original15C320/15C370 and15C110 rows0,1,12 execute using untouched authored scenario records. BR/SMG identities, object/current node allocations, transforms, bounds, location, unsorted links, ammo and heat are source produced. Exact SMG200-byte shared resource delivery is explicit; resource loading is not fully implemented.',
   'Actual empty-player masks leave each weapon inactive. B7E00/B7F50/B8040 return withoutBD360/BD710/F9B10/F9EA0/16B240/246A50 calls. No direct item tick, force-active bit, replacement PVS, nearby player/root, fabricated pose or contact is introduced. New flag8 remains set.',
   'Source current item motion/contact and ordinary pickup admission still require genuine active player/visibility/world scheduling. Complete Havok motion/readback, current animated player nodes/markers and Xbox reciprocal execution remain unresolved. Allocated item initial current nodes do not prove post-motion or carried animated pose.',
   'All prior frozen files, including the measured transfer freeze, and production remain unchanged.'],
  cases=[dict(case=t.current_case,
   prior_preparation=t.producers,current_stages=t.current_stages,current_calls=t.current_calls,
   current_stores=t.current_stores,current_states=t.current_states,resource_delivery=t.resource_delivery,unresolved_services=t.events,executed_original_addresses=sorted(t.executed))for i,t in enumerate(traces)])
 frozen_write(args.output,encoded(d))
 print(json.dumps(dict(report=str(args.output),sha256=digest(args.output),cases=[[dict(stage=r['stage'],completion=r['completion'],eip=r['stopped_eip'],cursor=r['after']['game_arena_cursor'])for r in t.current_stages]for t in traces])))
if __name__=='__main__':main()
