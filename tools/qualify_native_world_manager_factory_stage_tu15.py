#!/usr/bin/env python3
"""Seal native World3232E0 route, exact counters and disjoint cleanup."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);a=p.parse_args();a.stage=a.stage.resolve();stage=a.stage/'staged';manifest=json.loads((a.stage/'source-pins.json').read_text());assert all(sha(stage/r)==v for r,v in manifest['staged_hashes'].items());base=Path(manifest['base_source_root']);proofs={};owners=[];models=[]
 groups={'builder':'native-activation-manager-factory-binding-tu15-20261102-first','gcc':'native-world-manager-factory-model-gcc-20261102','clang':'native-world-manager-factory-model-clang-20261102','counters':'native-world-manager-factory-counters-v2-20261102'}
 for name,folder in groups.items():
  path=ROOT/'analysis/physics'/folder/'report.json';r=json.loads(path.read_text());assert r['passed'];assert not Path(r['owned_scratch']).exists();proofs[name]=dict(report=str(path.relative_to(ROOT)),sha256=sha(path),passed=True)
  owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
  if name=='builder':
   assert r['physical_cases_per_compiler']==20 and {v['compiler']for v in r['compiler_results']}=={'g++','clang++'}
   helper='include/halo2/native_tu15_activation_manager_factory_binding.hpp';assert r['source_hashes'][helper]==manifest['staged_hashes'][helper]
   for relative in('src/physics/native_tu15_activation_contact_manager.cpp','include/halo2/native_tu15_activation_contact_manager.hpp','src/physics/native_tu15_array_growth.cpp','src/physics/native_tu15_array_allocator.cpp','src/physics/native_tu15_contact_unwind_internal.hpp'):
    assert sha(base/relative)==r['source_hashes'][relative],relative
   totals={}
   for compiler in('g++','clang++'):
    rows=[json.loads(p.read_text())for p in(path.parent/compiler/'evidence').rglob('native_*.json')];assert len(rows)==20 and all(p['passed']for p in rows)
    total={field:sum(row.get(field,0)for row in rows)for field in('native_manager_binding_attempts','native_manager_binding_completed','native_manager_registrations','native_manager_binding_refusals','native_manager_binding_body_refusals')}
    assert total['native_manager_binding_completed']==10 and total['native_manager_registrations']==20 and total['native_manager_binding_refusals']==0 and total['native_manager_binding_body_refusals']==0;totals[compiler]=total
   proofs[name].update(profiles_per_compiler=20,native_factory_totals=totals,mutants_rejected=r['negative_controls'])
  elif name in('gcc','clang'):
   assert r['stage_hashes']==manifest['staged_hashes']and r['deduplicated']and r['compiled_cpp_count']==124;models.append(r['compiled_cpp_closure']);proofs[name]['compiled_cpp_count']=r['compiled_cpp_count']
  else:
   assert r['fields']==19 and r['serializers']==2 and r['builder_unknown_and_first_read_throw_counts_checked']and not r['physics_state_supplied'];proofs[name].update(fields=19,serializers=2,normal_and_throw_counters=True)
 assert models[0]==models[1]
 # The superseded counter collector failed before compiling due to a path
 # expression. Its private header copy is also gone and remains accounted.
 path=ROOT/'analysis/physics/native-world-manager-factory-counters-20261102/report.json';r=json.loads(path.read_text());assert not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
 world=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();start=world.index('    create.agents.create_manager=[&]');end=world.index('    create.agents.classify_shape=',start);callback=world[start:end];assert 'native_tu15_create_bound_activation_contact_manager'in callback and 'pair_storage'in callback and 'call('not in callback
 manifest.update(qualified=True,qualified_reports=proofs,source_qualified_direct_factory_count_per_compiler=10,source_qualified_registrations_per_compiler=20,
  qualification_scope='Sole native3232E0 builder BOTH20genuineSleepingB9800 profiles/10actualfactory completions/20registrations +3mutants; sameWorldroute BOTH124CPP default10/factoryprefix/genuine09mergeprefix;19exactGetter/twoJSON/positivegate/normal-and-throw countertruth. Unknownbody/registeredlock owners explicitly refuse. WholeactualWorld/runtime/livebyteFP remains separate.',
  remaining=['Actual multiplayer nativefactory positivecounts and strictwholeframe/camera','Positive native body-destructor/registered-lock construction-unwind ownership','Memoryfault/mutable observer domains beyondqualifiedfactoryproof'])
 save(a.stage/'source-pins.json',manifest);save(a.stage/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True,frozen_source_snapshots_preserved=True));print(dict(qualified=True,proof_groups=len(proofs),measured_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
