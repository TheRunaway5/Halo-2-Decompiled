#!/usr/bin/env python3
"""Freeze the same-builder pair-wake proof, World seam and cleanup receipts."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-world-pair-island-wake-stage-v2-20261102'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,x):p.write_text(json.dumps(x,indent=2,sort_keys=True)+'\n')
def main():
 parent_path=ROOT/'analysis/physics/native-pair-create-island-wake-tu15-20261102-v5/report.json';parent=json.loads(parent_path.read_text());assert parent['passed']
 profiles={};reports=[parent_path]
 for compiler in('g++','clang++'):
  rows=[json.loads(p.read_text())for p in(parent_path.parent/compiler/'evidence').glob('child*.json')];assert len(rows)==32 and all(r['passed']for r in rows)
  positives=[r for r in rows if r['declared_sleep_list_control']];assert len(positives)==4
  assert all(r['native_island_wakes']==1 and r['native_body_updates']==2 and r['FP_equal']and r['ordered_reads_stores_equal']for r in positives)
  assert sum(r['native_island_wakes']for r in rows)==4
  profiles[compiler]=dict(full_parent_cases=32,positive_declared_sleep_list_cases=4,
   native_island_wakes=4,native_body_updates=8,persistent_bytes_per_positive_case=positives[0]['persistent_compared_bytes'],
   captured_pair_baseline_exercises_wake=False)
 for label in('native-world-pair-island-wake-model-gcc-20261102-v2','native-world-pair-island-wake-model-clang-20261102-first','native-world-pair-island-wake-counters-20261102-first'):
  path=ROOT/'analysis/physics'/label/'report.json';r=json.loads(path.read_text());assert r['passed'],label;reports.append(path)
 pins_path=STAGE/'source-pins.json';pins=json.loads(pins_path.read_text())
 for name,digest in pins['staged_hashes'].items():assert sha(STAGE/'staged'/name)==digest,name
 for path in reports[1:3]:assert json.loads(path.read_text())['stage_hashes']==pins['staged_hashes']
 owners=[];unmeasured=[]
 labels=['native-pair-create-island-wake-tu15-20261102-'+x for x in('first','v2','v3','v4','v5')]
 labels+=['native-world-pair-island-wake-model-gcc-20261102-first','native-world-pair-island-wake-model-gcc-20261102-v2','native-world-pair-island-wake-model-clang-20261102-first','native-world-pair-island-wake-counters-20261102-first']
 for label in labels:
  path=ROOT/'analysis/physics'/label/'report.json'
  if not path.exists():
   child=json.loads((path.parent/'g++/report.json').read_text());owner=Path(child['build']).parents[2]
   assert not owner.exists();unmeasured.append(dict(owned_scratch=str(owner),removed_verified=True,
    allocation_bytes_not_recorded=True,reason='Interrupted setup-only link failure before top-level final receipt; retained child build diagnostic.'))
   continue
  r=json.loads(path.read_text());assert not Path(r['owned_scratch']).exists()
  owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
 save(STAGE/'cleanup-verification.json',dict(verified=True,completed_owners=owners,unmeasured_completed_owners=unmeasured,
  total_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners),retained_large_artifacts=[],protected_reference_inputs_preserved=True,not_a_global_free_space_measurement=True))
 pins.update(qualified=True,qualification_reports={str(p.relative_to(ROOT)):sha(p)for p in reports},
  physical_profiles=profiles,whole_parent_incoming_x87_depth=0,whole_parent_remaining_services='Genuine unmodified original bodies in independent oracle adapter; shared wake itself has no original fallback.',
  positive_control_scope='Same genuine actual02 child entry/body/island identities with explicitly declared coherent inactive-list DATA. Not captured/live sleep, no new constraint rows manufactured.',
  world_models=['default10','actual02-shape-factory-prefix','genuine09-merge-prefix','declared-inactive-pair-wake-prefix'],
  production_applied=False,live_qualified=False,unknown_child_policy='Shared activation binding counts actual owner before typed boundary throws; no emulated fallback.',
  retained_DATA='Pair wake uses low caller scratch.first+1000..1880, below end−2800/min5000. Callback precedes native factory storage; no retained merge packet is reused.',
  open_acceptance=['Actual multiplayer native pair wake positive','Additional body-listener/creation/lifetime profiles and native unknown owners remain counted/refused','Full runtime original-frame parity after live wiring'])
 save(pins_path,pins)
 print(dict(qualified=True,completed_owners=len(owners),removed_parent_bytes=sum(r['allocated_bytes_before_cleanup']for r in owners)))
if __name__=='__main__':main()
