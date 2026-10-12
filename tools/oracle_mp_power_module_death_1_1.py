#!/usr/bin/env python3
"""Original death/countdown roots on the unchanged source-allocated canister.

Death is an explicit diagnostic request, not a fabricated projectile/contact.
No time, vitality, admission, world/contact or effect success is written.
The inherited frozen compatibility context remains a declared platform input.
"""
import argparse,hashlib,json
from pathlib import Path
import oracle_mp_power_module_schedule_1_1 as base
from oracle_mp_spawn_biped_physics import PLACEMENT
from functions import _trace

ROOT=base.ROOT
ENTRIES=set(base.ENTRIES)|{0xdc460,0xd8070,0xdb570,0xdb710,0xd81c0,0xdb360,0xdb470}

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
 pins={**base.FROZEN,
  'tools/oracle_mp_power_module_schedule_1_1.py':hashlib.sha256((ROOT/'tools/oracle_mp_power_module_schedule_1_1.py').read_bytes()).hexdigest(),
  'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/manifest.json':hashlib.sha256((ROOT/'analysis/multiplayer/sandbox-source/power-module-schedule/frozen/manifest.json').read_bytes()).hexdigest()}
 for p,h in pins.items():assert base.sandbox.digest(ROOT/p)==h,p
 prior=json.loads((base.OWNER/'report.json').read_text())
 t=base.player.prepare(json.loads((base.OWNER/'raw-host-context.json').read_text()),trace_type=base.PowerTrace)
 t.selected_physics_fixup(0xe6f80583);factory=t.factory(0xe6f80583)
 assert factory['completed'] and factory['returned_eax']=='e26f0000'
 keys=set(factory)-{'calls','ordered_stores'}
 assert {k:factory[k]for k in keys}=={k:prior['case'][k]for k in keys}
 assert factory['calls']==prior['case']['calls'][:len(factory['calls'])]
 assert factory['ordered_stores']==prior['case']['ordered_stores'][:len(factory['ordered_stores'])]
 assert t.selected_fixup==prior['selected_original_physics_postprocess']
 t.sandbox_enabled=False;t.power_observe=True
 owner=PLACEMENT+0x800
 # Original D7CD0 fills actual retained attribution from this exact object.
 t.phase('original_current_object_damage_owner',0xd7cd0,(0xe26f0000,owner))
 owner_hex=bytes(t.g.mem_read(owner,10)).hex()
 t.phase('explicit_original_death_request',0xd7e00,(0xe26f0000,owner,1,0))
 for i in range(1,8):
  if not t.phase('explicit_successive_original_damage_update_%d'%i,0xd73e0,(0xe26f0000,)):break
 out=args.output.parent;out.mkdir(parents=True,exist_ok=True);directory=out/'instructions';directory.mkdir(exist_ok=True)
 def emit(name,addresses):
  rows=[t.code.at(a)for a in sorted(addresses)]
  for i in rows:assert i and t.image.read(i.address,len(i.bytes))==i.bytes==bytes(t.g.mem_read(i.address,len(i.bytes)))
  raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in rows).encode()
  base.frozen_write(directory/name,raw)
  return dict(file='instructions/'+name,sha256=hashlib.sha256(raw).hexdigest(),rows=len(rows),executed_rows=sum(i.address in t.power_executed for i in rows))
 listings=[]
 for a in sorted(ENTRIES):
  f=_trace(t.code,a,t.starts|ENTRIES)
  listings.append(dict(entry=f'{a:08x}',**emit(f'{a:08x}.asm',f.code)))
 listings.append(dict(kind='complete_executed_original',**emit('executed-original.asm',t.executed)))
 listings.append(dict(kind='ordered_store_sites',**emit('store-sites.asm',{int(r['instruction'],16)for r in t.power_stores})))
 report=dict(schema=1,oracle_sha256=base.sandbox.digest(__file__),source_sha256=base.PIN,cache_sha256=base.CACHE_PIN,
  prior_pins=pins,source_factory_correlation=dict(report_sha256=base.sandbox.digest(base.OWNER/'report.json'),
   constructor_state_and_ABI_byte_identical=True,original_pre_query_call_and_store_prefix_byte_identical=True,
   selected_phmo_postprocess_byte_identical=True,retained_datum='e26f0000'),
  actual_source_owner10_hex=owner_hex,stages=t.power_stages,calls=t.power_calls,
  stores=t.power_stores,states=t.power_states,instructions=listings,all_services=t.events,
  instruction_patches=[],new_supplied_gameplay_or_physics_results=[],
  boundaries=[
   'Unchanged complete original allocated Lockout power_module constructor state/ABI/call/store prefix is correlated before this extension.',
   'D7E00 is an explicit diagnostic death request, not an incoming projectile, solver contact, source depletion admission or live title frame.',
   'D7CD0 produces the actual retained owner input. Successive whole D73E0 roots use unchanged source state; no tick, delta, pending word, position, pose or effect return is assigned.',
   'Selected original preparation and recorded compatibility backing are unchanged inherited scope. No Xbox hardware FP/Havok/full-startup parity is claimed.',
   'The first genuine unavailable effect/world dependency is retained. Static complete routines and reached instruction/store coverage are distinguished.'])
 for p,h in pins.items():assert base.sandbox.digest(ROOT/p)==h,p
 base.frozen_write(args.output,base.encoded(report))
 print(json.dumps(dict(path=str(args.output),sha256=base.sandbox.digest(args.output),stores=len(t.power_stores),
  stages=[(r['name'],r['completed'],r['stopped_eip'],r['invalid'])for r in t.power_stages])),flush=True)
if __name__=='__main__':main()
