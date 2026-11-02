#!/usr/bin/env python3
"""Freeze native authored function/curve qualification and measured cleanup."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def read(p):return json.loads(p.read_text())
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 base=ROOT/'analysis/physics';full=base/'native-function-evaluation-extended-tu15-20261102-v6';curves=base/'native-function-curves-tu15-20261102-v3'
 r=read(full/'report.json');curve=read(curves/'report.json');assert r['passed']and curve['passed'];pins={}
 for report in(r,curve):
  for name,value in report['source_hashes'].items():assert sha(ROOT/name)==value;pins[name]=value
 # This unchanged borrowed driver is independently frozen by the prior bounded
 # qualification. It is transformed only to call the extended typed API.
 bounded_base=read(base/'native-function-evaluation-bounded-qualified-20261102/source-pins.json')
 borrowed='tools/observe_native_function_evaluation_tu15.cpp';assert sha(ROOT/borrowed)==bounded_base['source_hashes'][borrowed];pins[borrowed]=sha(ROOT/borrowed)
 proofs=[]
 for compiler in('g++','clang++'):
  n=read(full/compiler/'evidence/native.json');b=read(full/compiler/'evidence/bounded.json');c=read(curves/compiler/'evidence/native.json')
  assert n['passed']and n['profiles']==1476 and len(n['parent_executed_sites'])==492 and not n['parent_unexercised_sites']
  assert b['passed']and len(b['cases'])==504 and c['passed']and len(c['cases'])==640
  assert all(row['passed']for j in(n,b,c)for row in j['cases'])
  kinds=sorted(set(row['kind']for row in n['cases']));assert kinds==list(range(11))
  proofs.append(dict(compiler=compiler,whole_parent_profiles=n['profiles'],parent_sites=492,bounded_edge_profiles=504,curve_profiles=640,
   whole_parent_declared_compared_bytes=n['total_declared_compared_bytes'],whole_parent_original_instructions=n['total_original_instructions'],
   native_power_calls=sum(v['native_powers']for v in n['cases']),native_control_point_queries=sum(v['native_control_points']for v in n['cases']),
   whole_parent_report=str((full/compiler/'evidence/native.json').relative_to(ROOT)),whole_parent_report_sha256=sha(full/compiler/'evidence/native.json'),
   bounded_report=str((full/compiler/'evidence/bounded.json').relative_to(ROOT)),bounded_report_sha256=sha(full/compiler/'evidence/bounded.json'),
   curve_report=str((curves/compiler/'evidence/native.json').relative_to(ROOT)),curve_report_sha256=sha(curves/compiler/'evidence/native.json')))
 owners=[]
 folders=['native-function-curves-tu15-20261102-'+v for v in('first','v2','v3')]+['native-function-evaluation-extended-tu15-20261102-'+v for v in('first','v2','v3','v4','v5','v6')]
 folders+=['native-function-fmod-zero-owner-20261102','native-function-power-finalization-20261102','native-function-power-exception-inventory-20261102']
 for folder in folders:
  path=base/folder/'report.json';q=read(path);assert not Path(q['owned_scratch']).exists()
  owners.append(dict(owned_scratch=q['owned_scratch'],allocated_bytes_before_cleanup=q['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
 q=read(base/'native-function-negative-control-point-input-20261102.json');assert not Path(q['owned_scratch']).exists()
 owners.append(dict(owned_scratch=q['owned_scratch'],allocated_bytes_before_cleanup=q['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report='analysis/physics/native-function-negative-control-point-input-20261102.json'))
 result=dict(qualified=True,source_hashes=pins,compiler_proofs=proofs,original_sha256=r['reference']['original_sha256'],
  all_eleven_kinds_normal_domain_qualified=True,full_function_parity_proven=False,full_power_parity_proven=False,
  declared_authored_inputs=True,captured_runtime_inputs=False,source_outcomes_supplied=False,native_only_within_admitted_domain=True,
  remaining_domains=['Unmasked numerical traps','Nonfinite/CRT domain and range publication','Alternate-CW pow precision publication',
   'Numerical/source x87 stack-fault domains','Memory-fault prefixes and mutable read observers','Genuine authored non0/1 live function use',
   'PentiumIII versus host transcendental approximation identity'],
  binding_seam='NativeTu15SurfaceConveyorServices::evaluate can call native_tu15_evaluate_authored_function_extended directly with actual8Breference,2floatwords,FP and caller x87depth; child failure must propagate/count, never replay original13DE70.',
  native_units=['src/core/native_tu15_function_evaluation_extended.cpp','src/core/native_tu15_function_curves.cpp','src/core/native_tu15_function_control_point.cpp'],
  existing_bounded_owner_preserved=True,admission=dict(kinds=list(range(11)),transition_types=list(range(8)),periodic_types=list(range(12)),
   power_control_word='0x027F for actual pow call; source guard shortcuts retain other masked CW profiles',power_depth='positive<=5, nonzero negative<=4; source requires4 temporaries on negative integer path'),
  scope='Complete492-site13DE70 semantic parent in tested admitted normal finite math,504boundededge and640curve controls per compiler. Native arithmetic/typedDATA only; originalpublicparents/children are independent physical oracle. Declared negative control points select real source clamps and are not MCC/live rows. Exceptional owners remain explicit refusal, not fallback.')
 save(a.output/'source-pins.json',result);save(a.output/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True))
 print(dict(qualified=True,whole_parent_profiles=1476,source_parent_sites=492,bounded_profiles=504,curve_profiles=640,completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(o['allocated_bytes_before_cleanup']for o in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
