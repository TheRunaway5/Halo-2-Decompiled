#!/usr/bin/env python3
"""Authenticate final narrow regroup stage/proofs and measured private cleanup."""
import argparse,hashlib,json
from pathlib import Path
from prepare_native_world_island_regroup_binding_tu15 import FIELDS,UNITS,sha,save
ROOT=Path(__file__).resolve().parents[1]
REPORTS={
 'physical_builder':'native-island-regroup-binding-v2-actual09-declared-20261101/report.json',
 'g++_world':'native-world-island-regroup-model-20261101/report.json',
 'clang++_world':'native-world-island-regroup-model-clang-20261101/report.json',
 'counters':'native-world-island-regroup-counters-v2-20261101/report.json'}
EXTRAS=['native-island-regroup-actual1470-census-20261101/report.json','native-island-regroup-binding-actual09-declared-20261101/report.json','native-world-island-regroup-counters-20261101/report.json',*REPORTS.values()]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();stage=a.stage/'staged';pin=a.stage/'source-pins.json';manifest=json.loads(pin.read_text());proofs={}
 assert all(sha(stage/path)==h for path,h in manifest['staged_hashes'].items())
 for relative,h in manifest['base_hashes'].items():
  bases=[Path(root)/relative for root in manifest['base_source_roots']if(Path(root)/relative).is_file()];assert sha(bases[-1])==h,relative
 for name,relative in REPORTS.items():
  path=ROOT/'analysis/physics'/relative;r=json.loads(path.read_text());assert r['passed'],name;assert not Path(r['owned_scratch']).exists(),name;proofs[name]=dict(report=str(path.relative_to(ROOT)),sha256=sha(path),passed=True)
  if name=='physical_builder':
   assert {c['compiler']for c in r['compiler_results']}=={'g++','clang++'}
   for source,h in r['source_hashes'].items():assert sha(stage/source if(stage/source).is_file()else ROOT/source)==h,source
   for e in path.parent.glob('*/evidence/native.json'):
    ev=json.loads(e.read_text());assert ev['passed']and len(ev['cases'])==72 and all(c['passed']for c in ev['cases']);assert {c['depth']for c in ev['cases']}==set(range(6))
  if name.endswith('_world'):
   assert r['compiled_cpp_count']==120 and r['deduplicated'];assert r['stage_hashes']==manifest['staged_hashes']
   assert all(f'src/physics/native_tu15_{n}.cpp'in r['compiled_cpp_closure']for n in UNITS)
  if name=='counters':assert r['fields']==24 and r['phase_and_positive_split_gates_distinct']
 for unit in UNITS:
  for source in('include/halo2/native_tu15_'+unit+'.hpp','src/physics/native_tu15_'+unit+'.cpp'):assert sha(ROOT/source)==manifest['staged_hashes'][source]
 acceptance=(stage/'tests/physics_multiplayer_acceptance.cpp').read_text();assert 'HALO2_REQUIRE_NATIVE_CONTACT_POINT_PIPELINE'in acceptance
 for n in FIELDS:assert acceptance.count('{"native_world_regroup_'+n+'",native_frames.native_world_regroup_'+n+'},')==2
 assert 'world_call(0x3187c0u'not in(stage/'src/physics/native_tu15_world_integration_residuals.cpp').read_text()
 manifest.update(qualified=True,qualified_reports=proofs,qualification_scope='Exact production native service builder physical72profiles/compiler at depth0..5; coherent120CPP World ten-profile/default and actualfactory/merge-prefix models on both compilers; actual24-field Getter/both serializers/distinct phase/split gates on both compilers. No full original World/live positive repartition claim.',remaining=['Visible multiplayer normal native regroup phase and genuine positive repartition','Positive action/constraint/contact redistribution callbacks','Empty-island and callback alias/fault/unwind domains','Fixed/secondary/lazy-default/large/exhausted allocator domains'])
 save(pin,manifest);owners=[];seen=set()
 for relative in EXTRAS:
  path=ROOT/'analysis/physics'/relative;r=json.loads(path.read_text());owner=Path(r['owned_scratch']);assert not owner.exists();amount=r['temporary_allocated_bytes_before_cleanup'];assert amount>0
  if str(owner)in seen:continue
  seen.add(str(owner));owners.append(dict(owned_scratch=str(owner),allocated_bytes_before_cleanup=amount,removed_verified=True,report=str(path.relative_to(ROOT)),passed=r.get('passed'),scope='Measured completed private owner; child CMake builds already retired and excluded from final parent allocation'))
 # The production-builder owner already appears in its family cleanup receipt.
 owners=[r for r in owners if r['report']!='analysis/physics/'+REPORTS['physical_builder']]
 save(a.stage/'extra-cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True))
 print(dict(qualified=True,proofs=len(proofs),extra_completed_owners=len(owners),allocated_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners)))
 return 0
if __name__=='__main__':raise SystemExit(main())
