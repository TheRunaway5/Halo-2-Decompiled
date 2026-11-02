#!/usr/bin/env python3
"""Retain actual04 positive graph and actual10 reference receipts, retire owners."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);proofs={};owners=[]
 folders=['native-island-regroup-actual04-positive-20261101','native-constraint-reference-actual1470-10-20261102','native-dynamic-pair-partition-actual04-census-20261101']
 for folder in folders:
  base=ROOT/'analysis/physics'/folder;r=json.loads((base/'report.json').read_text());assert r['passed'];assert not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str((base/'report.json').relative_to(ROOT))))
  if folder.endswith('census-20261101'):continue
  rows=[]
  for e in base.glob('*/evidence/native.json'):
   ev=json.loads(e.read_text());assert ev['passed']and all(c['passed']for c in ev['cases']);cases=ev['cases'];keys=['persistent_compared_bytes','all_before_bytes','source_parent_events','source_parent_steps','source_steps','fully_connected','native_graph_builds','flag_skips','created_islands','native_temporary_constructors','native_temporary_destructors','native_reference_decrements','runtime_removals','remaining_owner_calls'];sums={k:sum(c.get(k,0)for c in cases)for k in keys if k in cases[0]};rows.append(dict(compiler=e.parent.parent.name,profiles=len(cases),sums=sums,report=str(e.relative_to(ROOT)),sha256=sha(e)))
  assert {p['compiler']for p in rows}=={'g++','clang++'}
  proofs[folder]=dict(qualified=True,source_hashes=r['source_hashes'],compiler_proofs=rows,report=str((base/'report.json').relative_to(ROOT)),report_sha256=sha(base/'report.json'),live_qualified=False)
 result=dict(qualified=True,proofs=proofs,actual04_scope='Unchanged genuine active-positive World/realcontactpair/partitionflags; declared standalone3187C0 originalformal. Nativegraph48/compiler fullyconnected, no newislands; no actualcaller/live claim.',actual10_scope='Actual selected22E1B0 instance1004886C/datumED76000D, ref5→4/runtimeflag0. CapturedFP +declaredroundings, allbanks/strictorderedoperands/FP/RET; no runtimeeligibleprofile expansion.',remaining=['Visible actual World regroup/wake/repartition','Newisland and positive action/constraint/contact redistribution','Runtime-selected22E1B0 actualprofile','Fault/unwind/mutableobserver domains'])
 save(a.output/'source-pins.json',result);save(a.output/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,proof_groups=len(proofs),completed_owners=len(owners),measured_parent_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
