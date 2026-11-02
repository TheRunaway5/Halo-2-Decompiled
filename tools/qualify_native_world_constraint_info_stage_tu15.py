#!/usr/bin/env python3
"""Freeze two complete nativeinfo leaves and narrow service/counter transport."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
PROOFS={'constant':'native-constant-constraint-info-tu15-20261102-first','extended':'native-extended-constraint-info-tu15-20261102-first','world':'native-world-constraint-info-model-20261102','counters':'native-world-constraint-info-counters-20261102'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();stage=a.stage/'staged';manifest=json.loads((a.stage/'source-pins.json').read_text());assert all(sha(stage/f)==h for f,h in manifest['staged_hashes'].items());proofs={};owners=[]
 for name,folder in PROOFS.items():
  path=ROOT/'analysis/physics'/folder/'report.json';r=json.loads(path.read_text());assert r['passed'];assert not Path(r['owned_scratch']).exists();entry=dict(report=str(path.relative_to(ROOT)),sha256=sha(path),passed=True)
  owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
  if name in('constant','extended'):
   assert {v['label']for v in r['results']if v['exit_code']==0}>={'g++','clang++'};assert r['body_coverage']['total']==r['body_coverage']['executed'];entry.update(profiles=r['original_cases'],sites=r['body_coverage']['total'],ordered_reads=r['ordered_reads'],ordered_stores=r['ordered_stores'])
   unit=name+'_constraint_info'
   for f in('include/halo2/native_tu15_'+unit+'.hpp','src/physics/native_tu15_'+unit+'.cpp'):assert r['source_hashes'][f]==manifest['staged_hashes'][f]and sha(ROOT/f)==manifest['staged_hashes'][f]
  if name=='world':assert r['compiled_cpp_count']==123 and r['stage_hashes']==manifest['staged_hashes']and r['deduplicated']
  if name=='counters':assert r['fields']==2 and r['serializers']==2 and not r['physics_state_supplied']
  proofs[name]=entry
 callback=(stage/'src/physics/native_tu15_island_regroup_binding.cpp').read_text();assert 'if(owner.identity==0x31d630u)'in callback and 'native_tu15_get_constant_constraint_info(m,info,observe)'in callback;assert 'if(owner.identity==0x31f840u)'in callback and 'native_tu15_get_extended_constraint_info(m,constraint,info,observe)'in callback;assert '++activity->unknown_constraint_info;unknown("constraint info",owner)'in callback
 base=Path(manifest['base_stage'])/'staged'
 for f,h in json.loads((base.parent/'source-pins.json').read_text())['staged_hashes'].items():
  if f not in manifest['delta_files']:assert sha(stage/f)==h,f
 manifest.update(qualified=True,qualified_reports=proofs,qualification_scope='Complete6-site31D630/7-site31F840+sole325E90 leaves bothcompilers74/111profiles, signed/alias/fault/mutationDATA+FP. Typedknownowner route/sourceaudit, coherent123CPPWorld default10/factory/merge-prefix build/model,2Getter/twoJSON/positivegates bothcompilers. Worldmodel doesnotexercise selectedinfo callback; actualtyped association/fullPLAYER/frame remainsseparate.',remaining=['Actualtypedinfoinput/livePLAYER/fulloriginalframe association','Otherremainingnativephysicsowners; unknowninfos stillcountthrow','Root-owneddeferredgrowth/MPreadonlyobserver composition'])
 save(a.stage/'source-pins.json',manifest);save(a.stage/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,proof_groups=len(proofs),completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
