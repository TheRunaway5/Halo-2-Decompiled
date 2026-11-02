#!/usr/bin/env python3
"""Retain native actual1470 composition evidence, without upgrading live claims."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);pins={};proofs=[];owners=[]
 folders=['native-point-wake-regroup-actual1470-20261101','native-point-wake-regroup-actual1470-clang-20261101']
 for folder in folders:
  base=ROOT/'analysis/physics'/folder;r=json.loads((base/'report.json').read_text());assert r['passed']and not r['source_physics_fields_overridden']and not r['live_invocation_claimed'];assert not Path(r['owned_scratch']).exists()
  for path,h in r['source_hashes'].items():assert path not in pins or pins[path]==h;pins[path]=h
  owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str((base/'report.json').relative_to(ROOT))))
  for path in base.glob('*/evidence/native.json'):
   evidence=json.loads(path.read_text());cases=evidence['cases'];assert evidence['passed']and len(cases)==12 and all(c['passed']for c in cases)
   assert all(not c['source_partition_flag_injected']and c['remaining_owner_calls']==0 and c['graph_builds']==1 and c['fully_connected']==1 and c['new_islands']==0 for c in cases)
   for c in cases:assert len(c['phases'])==3 and all(phase['passed']and phase['different_bytes']==0 and phase['fp_equal']and phase['all_actual_before_bytes']==89795481 for phase in c['phases'])
   proofs.append(dict(compiler=path.parent.parent.name,profiles=len(cases),persistent_compared_bytes=sum(p['all_actual_before_bytes']for c in cases for p in c['phases']),native_point_removals=sum(c['point_removals']for c in cases),native_wakes=sum(c['native_wakes']for c in cases),native_body_updates=sum(c['body_updates']for c in cases),native_graph_builds=sum(c['graph_builds']for c in cases),native_fully_connected=sum(c['fully_connected']for c in cases),new_islands=0,report=str(path.relative_to(ROOT)),report_sha256=sha(path)))
 assert {r['compiler']for r in proofs}=={'g++','clang++'}
 census=ROOT/'analysis/physics/native-point-wake-frontier-actual1470-20261101/report.json';r=json.loads(census.read_text());assert r['passed']and not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(census.relative_to(ROOT))))
 result=dict(qualified=True,source_hashes=pins,compiler_proofs=proofs,captured_actual_wake_caller=False,live_qualified=False,source_physics_fields_overridden=False,native_remaining_owner_calls=0,scope='Genuine1470 typed point BEFORE followed by declared standalone316D40/3187C0 formals. Whole physical original and same native production builders agree after each phase, every actual bank+FP, with independent native POD00/A5/5A and4FP controls. Point removal naturally flags the inactive receiver; native wake activates it and native graph reaches fully-connected/flag-clear. No fabricated contacts, poses or flags, no original GP/SIMD/outcomes supplied.',remaining=['Actual source wake caller and visible multiplayer wake/regroup','Genuine new-island repartition and positive action/constraint/contact redistribution','Mutable observer/fault/unwind domains'])
 save(a.output/'source-pins.json',result);save(a.output/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,compiler_proofs=len(proofs),completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
