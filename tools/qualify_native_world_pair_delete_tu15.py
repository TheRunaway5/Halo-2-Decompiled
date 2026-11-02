#!/usr/bin/env python3
"""Seal native pair-deletion routing from physical, scope and coherent proofs."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();pins=a.stage/'source-pins.json';data=json.loads(pins.read_text());stage=a.stage/'staged'
 for relative,value in data['staged_hashes'].items():assert sha(stage/relative)==value,relative
 reports=['native-pair-delete-scoped-tu15-20261102-v4/report.json','native-pair-delete-scope-control-tu15-20261102-v2/report.json','native-world-pair-delete-counters-tu15-20261102-first/report.json','native-world-pair-delete-model-gcc-20261102-first/report.json','native-world-pair-delete-model-clang-20261102-first/report.json'];receipts={};models=[]
 for relative in reports:
  path=ROOT/'analysis/physics'/relative;r=json.loads(path.read_text());assert r['passed'],relative;assert not Path(r['owned_scratch']).exists(),r['owned_scratch'];receipts[str(path.relative_to(ROOT))]=sha(path)
  if 'stage_hashes'in r:assert r['stage_hashes']==data['staged_hashes'],relative;models.append(r)
 physical=json.loads((ROOT/'analysis/physics'/reports[0]).read_text());assert len(physical['compiler_results'])==2;assert all(r['exit_code']==0 for r in physical['compiler_results']);assert physical['body_sites']==247
 totals={};all_sites=set()
 for compiler in('g++','clang++'):
  evidence=ROOT/'analysis/physics'/Path(reports[0]).parent/compiler/'evidence';rows=[json.loads(p.read_text())for p in evidence.glob('child*.json')];assert len(rows)==36
  for row in rows:
   assert row['passed']and row['different_bytes']==0 and row['native_old_executor_calls']==0 and row['native_binding_attempts']==1 and row['native_binding_completions']==1 and row['native_binding_refusals']==0 and row['native_scope_refusals']==0 and row['native_binding_unqualified_owners']==0 and not row['unqualified_native_children'];all_sites.update(row['executed_sites'])
  totals[compiler]=sum(r['persistent_compared_bytes']for r in rows)
 assert len(all_sites)==188;assert len(models)==2 and all(r['compiled_cpp_count']==136 for r in models)
 source=(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text();at=source.index('  auto call_at=');start=source.index('{',at);assert source[start+1:].lstrip().startswith('pair_deletion_scope.reject_unconverted(target);');assert source.count('const auto formal=std::uint32_t(end)-0xa4u;put(m,formal,pair);')==1
 scope=json.loads((ROOT/'analysis/physics'/reports[1]).read_text());header=ROOT/'include/halo2/native_tu15_pair_deletion_native_scope.hpp';assert scope['source_hashes'][str(header)]==sha(stage/'include/halo2/native_tu15_pair_deletion_native_scope.hpp')
 data.update(qualified=True,root_reviewed=False,production_applied=False,live_qualified=False,qualification_reports=receipts,physical_cases_per_compiler=36,source_sites_qualified=188,source_sites_remaining=59,persistent_compared_bytes_per_compiler=totals,coherent_world_cpp_count=136,coherent_world_models=['default_ten_manager_profiles_plus_pair_deletion_and_nested_refusal','factory_prefix','genuine09_merge_prefix','declared_native_pair_wake_queue_prefix'],scope_guard_controls=dict(refuse_before_transport=6,outside_restoration=4,nested_and_exception_restore=True),native_refused_owner_census_separate_from_executed_residual_census=True,remaining=['59 unexercised source sites, including original3190D0-generated multi-record2F82D0 composition','Whole exact World320260 physical comparison','Live positive320260 selection/full strict original-frame and actual vehicle camera','Actual future destructor/listener/lifetime owners beyond qualified native closure remain named refusals'],qualification_source_hashes={str(p.relative_to(ROOT)):sha(p)for p in[ROOT/'tools/validate_native_pair_delete_scoped_tu15.py',ROOT/'tools/validate_native_pair_delete_scope_tu15.py',ROOT/'tools/prepare_native_world_pair_delete_tu15.py',ROOT/'tools/validate_native_world_pair_delete_counters_tu15.py',Path(__file__).resolve()]})
 save(pins,data);print(dict(qualified=True,source_pins_sha256=sha(pins),sites=188,remaining=59,physical_cases_per_compiler=36,cpp_count=136));return 0
if __name__=='__main__':raise SystemExit(main())
