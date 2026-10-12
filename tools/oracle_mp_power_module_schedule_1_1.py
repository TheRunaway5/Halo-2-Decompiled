#!/usr/bin/env python3
"""Independent original authored power-module active schedule; no physics outputs.

Imports every prior oracle read-only. The original factory and phmo fixup are
correlated to their frozen case before the actual player/PVS and object passes.
The inherited 30Hz clock input remains explicit: this is one bounded schedule,
not a natural full frame or continuous settling experiment.
"""
import argparse, hashlib, json
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_ECX, UC_X86_REG_EDX
import oracle_mp_sandbox_owner_1_1 as sandbox
import oracle_mp_spawn_player_visibility as player
from oracle_mp_item_current_producers_1_1 import CurrentTrace, encoded, frozen_write
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_smg_firing_1_1 import STACK, STOP, SEED, TIME, PIN, CACHE_PIN
from functions import _trace

ROOT = sandbox.ROOT
OUT = ROOT / 'analysis/multiplayer/sandbox-source/power-module-schedule'
OWNER = ROOT / 'analysis/multiplayer/sandbox-source/owner-creation/final/power-fixed-measured'
FROZEN = {**sandbox.FROZEN,
 'tools/oracle_mp_sandbox_owner_1_1.py': '579e6c9a5d2ef33fd31527a440f3b8ee77d72a20af458517cadbcfab7bd34acd',
 'analysis/multiplayer/sandbox-source/owner-creation/frozen/suite.json': 'b6853df025d301e28662e3e254887b9e6116d83858fc8835dc295cc0b9dbbfd6',
 'analysis/multiplayer/sandbox-source/owner-creation/final/power-fixed-measured/report.json': '6924d888bbe3bb2df70e0fe9d546c6b4d3732aea1cd26029e00b531de0761e00',
 'analysis/multiplayer/sandbox-source/later-update-functions/frozen/suite.json': 'c1515de35158b18fac87866486e83f9c268becf8499fcf6b9042c3ea8a0e96c6'}
ENTRIES = set(sandbox.ENTRIES) | {0x139750, 0x13a380, 0xb7ce0, 0xb7c50,
 0xb7e00, 0x1c7660, 0xb7f50, 0xb8040, 0xbd360, 0xbd710,
 0x10cdf0, 0x10ce60, 0xbdf80, 0xbede0, 0xb96a0, 0xb94f0, 0x1ce650,
 0x1d26f0, 0x1d26c0, 0x1d4f30, 0x1d5210, 0xbf7d0,
 0x1d3df0, 0x1d2df0, 0x1d2f10, 0x1d32a0, 0x1d40b0, 0x1d3fc0, 0x1d4160, 0x1d42d0,
 0x1d3bb0, 0x1d7ee0, 0x1c8350, 0x317650,
 0x208fb0, 0x2285a0, 0x209040, 0x2298b0,
 0xd9180, 0xd7c60, 0xdb570, 0xda150, 0xd7cd0, 0xd7e00,
 0x1c88f0, 0x1c8970, 0x1019f0, 0xc0270, 0xb92a0, 0xb9350}
STATE_OWNERS = {0x13a380, 0xb7ce0, 0xb7c50, 0xb7e00, 0x1c7660,
 0xb7f50, 0xb8040, 0xbd360, 0xbd710, 0x11d2b0, 0x11d330,
 0xd73e0, 0xdc0a0, 0xdb710, 0xdb470, 0xd8280, 0xd9180,
 0x1d5830, 0x1d5a60, 0x1d5e90, 0x1d6f40, 0x1d26f0,
 0x1d3df0, 0x1d2df0, 0x1d2f10, 0x1d32a0, 0x1d40b0, 0x1d3fc0, 0x1d4160, 0x1d42d0,
 0x1d7ee0,
 0x208fb0, 0x2285a0, 0x209040, 0x2298b0,
 0x1019f0, 0xc0270}
CALLBACK_SITES = {0x10ce3b, 0x10cd5a, 0x10cea1}

class PowerTrace(sandbox.SandboxTrace):
 item_objects = CurrentTrace.item_objects
 registry_chains = CurrentTrace.registry_chains
 def __init__(self, data):
  self.power_observe = False
  self.power_calls = []; self.power_pending = []; self.power_stores = []
  self.power_states = []; self.power_state_ids = {}; self.power_order = 0
  self.power_callbacks = []; self.power_stages = []; self.power_executed = set()
  self.power_broadphase = []
  super().__init__(data)
 def snapshot(self):
  pool = self.u(0x4e78d0); header_data = self.u(pool+0x44); header_bits = self.u(pool+0x48)
  header_capacity = self.u(pool+0x20); header_stride = self.u(pool+0x24)
  world=self.state()
  for o in world['objects']:
   if o['component']=='ffffffff':continue
   cp=self.u(self.u(0x526248)+0x44)+(int(o['component'],16)&0xffff)*0xa0
   ptr,n,bank_capacity=(self.u(cp+x)for x in (0x88,0x8c,0x90))
   cap=bank_capacity&0x7fffffff;assert n<=cap<100
   phantom=self.u(cp+0x94)
   o['physical_auxiliary']=dict(contact_bank_pointer=f'{ptr:08x}',count=n,capacity_word=f'{bank_capacity:08x}',
    stride=0x48,allocated_capacity_hex=bytes(self.g.mem_read(ptr,cap*0x48)).hex()if ptr else None,
    phantom_pointer=f'{phantom:08x}',phantom_header0c_hex=bytes(self.g.mem_read(phantom,12)).hex()if phantom else None)
   if phantom:
    pp,count,pcap=(self.u(phantom+x)for x in (0,4,8));assert count<100
    o['physical_auxiliary'].update(phantom_records_hex=bytes(self.g.mem_read(pp,count*8)).hex()if pp else None)
   for body in o['bodies']:
    if body['motion'] is not None:
     body['motion_consumed_prefix_c0_hex']=bytes(self.g.mem_read(int(body['motion'],16),0xc0)).hex()
  impact_pools=[]
  for glob in (0x52648c,0x526490):
   pp=self.u(glob);cap,stride,data,bitmap=(self.u(pp+x)for x in (0x20,0x24,0x44,0x48))
   assert cap==32 and stride in (0xa0,0x40)
   impact_pools.append(dict(global_address=f'{glob:08x}',pointer=f'{pp:08x}',header4c_hex=bytes(self.g.mem_read(pp,0x4c)).hex(),
    data_pointer=f'{data:08x}',capacity=cap,stride=stride,data_hex=bytes(self.g.mem_read(data,cap*stride)).hex(),
    bitmap_pointer=f'{bitmap:08x}',bitmap_hex=bytes(self.g.mem_read(bitmap,4)).hex()))
  return dict(world=world, current=CurrentTrace.current_snapshot(self),impact_pools=impact_pools,
   player=self.player_state(), clock_pointer=f'{self.u(0x518444):08x}',
   clock0c_hex=bytes(self.g.mem_read(self.u(0x518444),12)).hex(),
   source_rng=f'{self.u(SEED):08x}',
   header_pool=dict(pointer=f'{pool:08x}',header4c_hex=bytes(self.g.mem_read(pool,0x4c)).hex(),
    capacity=header_capacity,stride=header_stride,data_pointer=f'{header_data:08x}',data_hex=bytes(self.g.mem_read(header_data,header_capacity*header_stride)).hex(),
    bitmap_pointer=f'{header_bits:08x}',bitmap_hex=bytes(self.g.mem_read(header_bits,((header_capacity+31)//32)*4)).hex()))
 def state_id(self):
  r=self.snapshot();h=hashlib.sha256(json.dumps(r,sort_keys=True,separators=(',',':')).encode()).hexdigest()
  if h not in self.power_state_ids:
   self.power_state_ids[h]=len(self.power_states)
   self.power_states.append(dict(id=len(self.power_states),sha256=h,**r))
  return self.power_state_ids[h]
 def power_returns_at(self, address):
  matches=[x for x in self.power_pending if x[0]==address and x[2]['phase']==self.active]
  if not matches:return
  sp=matches[-1][1]
  for ret,entry_sp,r in reversed(matches):
   if sp!=entry_sp:continue
   r.update(returned_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
    return_esp_delta=self.g.reg_read(UC_X86_REG_ESP)-sp,return_order=self.power_order)
   self.power_order+=1
   if int(r['entry'],16) in STATE_OWNERS:r['after_state']=self.state_id()
   if 'contact_record_pointer'in r:r['contact48_after_hex']=bytes(self.g.mem_read(int(r['contact_record_pointer'],16),0x48)).hex()
   if 'broadphase_output_pointer'in r:r['broadphase_output20_after_hex']=bytes(self.g.mem_read(int(r['broadphase_output_pointer'],16),0x20)).hex()
   if 'impact_packet_pointer'in r:r['impact_packet40_after_hex']=bytes(self.g.mem_read(int(r['impact_packet_pointer'],16),0x40)).hex()
   self.power_pending.remove((ret,entry_sp,r))
 def hook(self,g,a,size,user):
  if self.power_observe:
   self.power_executed.add(a);self.power_returns_at(a)
   if a in (0x1d2f2d,0x1d2f3c):
    r=next(row for _,_,row in reversed(self.power_pending)if row['entry']=='001d2f10')
    ptr=g.reg_read(UC_X86_REG_EAX)
    r.setdefault('actual_contact_manager_reads',[]).append(dict(return_site=f'{a:08x}',pointer=f'{ptr:08x}',
     raw_hex=bytes(g.mem_read(ptr,0x20 if a==0x1d2f2d else 4)).hex(),
     kind='point_distance_normal_packet'if a==0x1d2f2d else'source_scalar_word',order=self.power_order))
    self.power_order+=1
   if a==0x1d80ef:
    sp=g.reg_read(UC_X86_REG_ESP);ptr=self.u(sp+4);target=self.u(g.reg_read(UC_X86_REG_EDX)+0x20)
    r=dict(id=len(self.power_calls),entry=f'{target:08x}',caller='001d80f2',phase=self.active,
     order=self.power_order,registers=self.registers(),stack=[f'{self.u(sp+i*4):08x}'for i in range(2)],
     owning_indirect_site='001d80ef',actual_backend_pointer=f'{g.reg_read(UC_X86_REG_ECX):08x}',
     actual_backend80_hex=bytes(g.mem_read(g.reg_read(UC_X86_REG_ECX),0x80)).hex(),
     actual_body_handle_pointer=f'{self.u(sp):08x}',actual_body_handle14_hex=bytes(g.mem_read(self.u(sp),0x14)).hex(),
     broadphase_output_pointer=f'{ptr:08x}',broadphase_output20_before_hex=bytes(g.mem_read(ptr,0x20)).hex(),
     max_authored_margin_word=f'{self.u(sp+0x18):08x}')
    self.power_order+=1;self.power_calls.append(r);self.power_broadphase.append(r)
    self.power_pending.append((0x1d80f2,sp-4,r))
   if a in CALLBACK_SITES:
    self.power_callbacks.append(dict(instruction=f'{a:08x}',phase=self.active,order=self.power_order,
     target=f'{g.reg_read(UC_X86_REG_EAX):08x}',datum_argument=f'{self.u(g.reg_read(UC_X86_REG_ESP)):08x}',registers=self.registers()))
    self.power_order+=1
   if a in ENTRIES:
    sp=g.reg_read(UC_X86_REG_ESP)
    r=dict(id=len(self.power_calls),entry=f'{a:08x}',caller=f'{self.u(sp):08x}',phase=self.active,
     order=self.power_order,registers=self.registers(),stack=[f'{self.u(sp+4+i*4):08x}'for i in range(16)])
    self.power_order+=1
    if a in STATE_OWNERS:r['before_state']=self.state_id()
    if a==0x1d2df0:
     ptr=g.reg_read(UC_X86_REG_EAX);r.update(contact_record_pointer=f'{ptr:08x}',contact48_before_hex=bytes(g.mem_read(ptr,0x48)).hex())
     r['source_entities']=[]
     for off in (8,12):
      p=self.u(sp+off);pairs,count=self.u(p+0x30),self.u(p+0x34);assert count<100
      r['source_entities'].append(dict(argument_stack_offset=off,pointer=f'{p:08x}',prefix80_hex=bytes(g.mem_read(p,0x80)).hex(),
       property_pairs_pointer=f'{pairs:08x}',property_pairs_count=count,property_pairs_hex=bytes(g.mem_read(pairs,count*8)).hex()if pairs else None))
    if a==0x1d2f10:
     ptr=g.reg_read(UC_X86_REG_ECX);manager=g.reg_read(UC_X86_REG_EAX)
     r.update(contact_record_pointer=f'{ptr:08x}',contact48_before_hex=bytes(g.mem_read(ptr,0x48)).hex(),
      source_manager_pointer=f'{manager:08x}',source_manager40_hex=bytes(g.mem_read(manager,0x40)).hex(),
      source_manager_vtable24_hex=bytes(g.mem_read(self.u(manager),0x24)).hex())
    if a==0x208fb0:
     ptr=g.reg_read(UC_X86_REG_EAX);r.update(impact_packet_pointer=f'{ptr:08x}',impact_packet40_before_hex=bytes(g.mem_read(ptr,0x40)).hex())
    if a==0x2285a0:
     ptr=self.u(sp+4);r.update(impact_packet_pointer=f'{ptr:08x}',impact_packet40_before_hex=bytes(g.mem_read(ptr,0x40)).hex())
    self.power_calls.append(r);self.power_pending.append((self.u(sp),sp,r))
  super().hook(g,a,size,user)
 def mem_write(self,g,access,address,size,value,user):
  if self.power_observe:
   # Complete non-stack ordered writes. Stack packets are retained separately
   # at observed source calls; no observer modifies guest state or registers.
   if not STACK-0x100000 <= address < STACK+0x100000:
    self.power_stores.append(dict(phase=self.active,order=self.power_order,
     instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{address:08x}',size=size,
     before_hex=bytes(g.mem_read(address,size)).hex(),written_hex=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    self.power_order+=1
  super().mem_write(g,access,address,size,value,user)
 def phase(self,name,entry,args=()):
  self.active=name;self.new_invalid=None;start=len(self.events);before=self.state_id()
  try:self.run(entry,args);complete=True;self.power_returns_at(STOP)
  except (unicorn.UcError,KernelBoundary):
   assert self.new_invalid is not None;complete=False
  self.power_stages.append(dict(name=name,entry=f'{entry:08x}',arguments=[f'{a:08x}'for a in args],before_state=before,
   after_state=self.state_id(),completed=complete,stopped_eip=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',
   invalid=self.new_invalid,used_services=self.events[start:]))
  return complete

def evidence(t,out):
 out.mkdir(parents=True,exist_ok=True);result=[]
 entries=ENTRIES | set(player.ENTRIES)
 def write(name,addresses):
  ins=[t.code.at(a)for a in sorted(addresses)]
  for i in ins:
   assert i is not None and t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode()
  frozen_write(out/name,raw)
  return dict(file='instructions/'+name,sha256=hashlib.sha256(raw).hexdigest(),rows=len(ins),
   executed_rows=sum(i.address in t.power_executed for i in ins))
 for a in sorted(entries):
  f=_trace(t.code,a,t.starts|entries)
  result.append(dict(entry=f'{a:08x}',**write(f'{a:08x}.asm',f.code),
   direct_calls=[dict(site=f'{p:08x}',target=f'{q:08x}')for p,q,k in f.sites if k=='call']))
 result.append(dict(kind='complete_executed_original_bytes',**write('executed-original.asm',t.executed)))
 result.append(dict(kind='ordered_store_instruction_sites',**write('store-sites.asm',{int(r['instruction'],16)for r in t.power_stores})))
 return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'frozen/measured/report.json')
 ap.add_argument('--without-player',action='store_true');args=ap.parse_args()
 for p,h in FROZEN.items():assert sandbox.digest(ROOT/p)==h,p
 suite=json.loads((ROOT/'analysis/multiplayer/sandbox-source/owner-creation/frozen/suite.json').read_text())
 for p,h in suite['files'].items():assert sandbox.digest(ROOT/p)==h,p
 data=json.loads((OWNER/'raw-host-context.json').read_text());prior=json.loads((OWNER/'report.json').read_text())
 t=player.prepare(data,trace_type=PowerTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583)
 assert factory['completed'] and factory['returned_eax']=='e26f0000'
 # The older factory dict deliberately retains its live call/store lists;
 # its later nine direct collision probes append to those same lists. Compare
 # the whole constructor state/ABI and the original pre-query list prefix.
 static_keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in static_keys}=={k:prior['case'][k]for k in static_keys},'Original full factory state changed under read-only observer'
 assert factory['calls']==prior['case']['calls'][:len(factory['calls'])]
 assert factory['ordered_stores']==prior['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==prior['selected_original_physics_postprocess']
 t.sandbox_enabled=False
 if not args.without_player:
  assert player.spawn_and_activate(t,0,activate=False)
 t.power_observe=True
 for name,entry in (('original_source_PVS_activation',0x13a380),
  ('original_ordinary_objects',0xb7e00),('original_world_physics',0x1c7660),
  ('original_dirty_objects',0xb7f50),('original_new_objects',0xb8040)):
  if not t.phase(name,entry):break
 if all(s['completed']for s in t.power_stages):
  t.phase('explicit_original_current_crate_bounds_consumer_after_schedule',0xbede0,(0xe26f0000,))
 listings=evidence(t,args.output.parent/'instructions')
 report=dict(schema=1,oracle_sha256=sandbox.digest(__file__),source_sha256=PIN,cache_sha256=CACHE_PIN,
  prior_pins=FROZEN,entire_prior_factory_manifest_verified=True,
  source_factory_correlation=dict(report_sha256=sandbox.digest(OWNER/'report.json'),constructor_state_and_abi_byte_identical=True,
   original_pre_query_call_and_store_prefix_byte_identical=True,
   selected_phmo_postprocess_byte_identical=True,authored_row=1,definition='e6f80583',retained_datum='e26f0000'),
  source_context_replay_file=str((OWNER/'raw-host-context.json').relative_to(ROOT)),
  source_context_sha256=sandbox.digest(OWNER/'raw-host-context.json'),
  prepared=t.producers,player_stages=t.player_stages,source_factory=factory,
  stages=t.power_stages,calls=t.power_calls,callbacks=t.power_callbacks,stores=t.power_stores,states=t.power_states,
  actual_broadphase_calls=t.power_broadphase,
  all_services=t.events,inherited_services={f'{a:08x}':str(v)for a,v in t.services.items()},
  instructions=listings,source_executed_schedule_addresses=[f'{a:08x}'for a in sorted(t.power_executed)],
  full_setup_and_schedule_executed_addresses=[f'{a:08x}'for a in sorted(t.executed)],
  new_supplied_game_or_Havok_fields=[],instruction_patches=[],native_gameplay_update_applied=False,
  boundaries=[
   'Whole recorded native compatibility context is replayed from the unchanged power-fixed creation freeze; actual Xbox thread/FP hardware parity is not claimed.',
   'Original selected initializer, actual full crate factory/phmo postprocess, source player join/spawn/cluster producer and full PVS execute. Other authored world rows, complete title/session startup and full139750 frame remain excluded.',
   'Source default options/property packet, tag/resource residency/raw OS backing and optional audio/attachment services remain explicit inherited inputs. No contact, activation, pose, damage, impulse, velocity, body identity or Havok/game return is supplied by this extension.',
   'The inherited12-byte clock/tick/rate fixture is retained unchanged. One selected ordinary→physics→dirty→new schedule is measured; no next tick or continuous settling is inferred.',
   'After the complete selected schedule, fullBEDE0 is explicitly called on the retained actual crate datum to measure its current bounds/backend consumer. This direct diagnostic root is not claimed to have been naturally dispatched by the preceding item tick.',
   'D9180/DC0A0/DB710/DB470/D8280 and phantom-contact positive branches are listed only unless their actual calls occur. No projectile hit/damage event or explosion is injected.',
   'Non-stack source stores and complete occupied object/body/motion/node/damage/current-registry/header/queue/context buffers are retained. Raw excess stack words are provenance, not new ABI arguments.'])
 for p,h in FROZEN.items():assert sandbox.digest(ROOT/p)==h,p
 frozen_write(args.output,encoded(report))
 print(json.dumps(dict(path=str(args.output),sha256=sandbox.digest(args.output),
  stages=[(r['name'],r['completed'],r['stopped_eip'],r['invalid'])for r in t.power_stages],
  calls=len(t.power_calls),stores=len(t.power_stores),states=len(t.power_states))))

if __name__=='__main__':main()
