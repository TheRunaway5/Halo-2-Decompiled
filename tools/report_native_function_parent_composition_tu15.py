#!/usr/bin/env python3
"""Freeze wholePART all11function and separate native error-policy proofs."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True,type=Path);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);base=ROOT/'analysis/physics';part=base/'native-part-contact-all-functions-tu15-20261102-v8/report.json';error=base/'native-function-power-exception-tu15-20261102-first/report.json';compat=base/'native-part-contact-extended-compatibility-tu15-20261102-first/report.json';pins={};proofs={}
 for name,path in(('part',part),('error',error),('compatibility',compat)):
  r=json.loads(path.read_text());assert r['passed'];hashes=r.get('source_sha256',r.get('source_hashes',{}));assert hashes
  for relative,value in hashes.items():assert sha(ROOT/relative)==value,(relative,value);pins[relative]=value
  proofs[name]=dict(report=str(path.relative_to(ROOT)),sha256=sha(path))
 parent=json.loads(part.read_text());assert parent['original_cases']==261 and parent['body_coverage']['0x1e0a60']==dict(total=418,executed=418)
 for row in parent['positive_declared_function_kind_activity'].values():assert row['profiles']==[4]*11 and row['calls']==[16]*11
 errors=[]
 for compiler in('g++','clang++'):
  path=error.parent/compiler/'evidence/native.json';r=json.loads(path.read_text());assert r['passed']and len(r['infinite_cases'])==76 and len(r['error_cases'])==96
  errors.append(dict(compiler=compiler,infinite_cases=76,whole_policy_cases=96,declared_compared_bytes=r['total_declared_compared_bytes'],report=str(path.relative_to(ROOT)),sha256=sha(path)))
 owners=[]
 folders=['native-part-contact-extended-compatibility-tu15-20261102-first']+['native-part-contact-all-functions-tu15-20261102-'+v for v in('first','v2','v3','v4','v5','v6','v7','v8')]+['native-function-power-exception-tu15-20261102-first','native-function-power-error-policy-inventory-20261102','native-function-power-integer-classification-20261102']
 for folder in folders:
  path=base/folder/'report.json';r=json.loads(path.read_text());owner=r.get('owned_scratch',r.get('scratch_cleanup_owner'));assert owner and not Path(owner).exists(),owner
  owners.append(dict(owned_scratch=owner,allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
 result=dict(qualified=True,source_hashes=pins,qualified_reports=proofs,typed_error_proofs=errors,
  complete_part_profiles_per_compiler=261,all_11_positive_function_profiles_per_compiler=[4]*11,
  all_11_positive_native_function_calls_per_compiler=[16]*11,source_part_sites=418,source_material_sites=62,
  source_property_sites=26,source_ordered_reads=parent['original_ordered_reads'],source_ordered_stores=parent['original_ordered_stores'],
  source_original_instructions=parent['physical_instructions'],mutants_rejected=parent['negative_controls_rejected']+parent['part_parent_mutants_rejected'],
  extended_evaluator_frozen_receipt='analysis/physics/native-function-evaluation-extended-qualified-20261102/source-pins.json',
  source_early_failures='Prior unqualified fixture cohorts lacked originalcurveCFG/readonlymathbank coverage; v7 only activity-output retention failed aftertests. All scratchremoved; no kernel/output substitution.',
  scope='Whole typed extendedPART helper, unchangedPART/material/shader/currentdepth0 caller, declaredpositive11kind authoredDATA. Realunchanged sourceCRT math executes independently; curve CRT internalledger is opaque here and independently640profilequalified. No runtime/live/MCC authored-conveyor claim. Errorpolicy components are separately qualified, not yet composed into frozen1476 evaluator.',
  remaining=['Exact extended point/gameplay callback composition and World stage','Actual authored MCC conveyor/Part selection and visiblephysics',
   'Exception policies not yet composed into frozen extended evaluator','Unmasked source traps, numerical range/domain/error/fault profiles remain explicit'])
 save(a.output/'source-pins.json',result);save(a.output/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,owners=len(owners),measured_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
