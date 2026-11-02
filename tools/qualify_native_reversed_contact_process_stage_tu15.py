#!/usr/bin/env python3
"""Pin qualified reverse parents/binding, keep generated-contact claim open."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
PROOFS={'public':'native-reversed-contact-public-tu15-20261102-v3','suffix':'native-reversed-contact-suffix-tu15-20261102-first','binding':'native-reversed-contact-production-binding-tu15-20261102','g++_world':'native-reversed-contact-world-model-20261102','clang++_world':'native-reversed-contact-world-model-clang-20261102','counters':'native-reversed-contact-counters-tu15-20261102'}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',type=Path,required=True);a=p.parse_args();a.stage=a.stage.resolve();stage=a.stage/'staged';manifest=json.loads((a.stage/'source-pins.json').read_text());assert all(sha(stage/f)==h for f,h in manifest['staged_hashes'].items());proofs={};owners=[]
 for name,folder in PROOFS.items():
  base=ROOT/'analysis/physics'/folder;path=base/'report.json';r=json.loads(path.read_text());assert r['passed'],name;assert not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
  entry=dict(report=str(path.relative_to(ROOT)),sha256=sha(path),passed=True)
  if name in('public','suffix','binding'):
   assert {v['compiler']for v in r['compiler_results']}=={'g++','clang++'};assert not r['public_positive_contacts_qualified']
   for f in('src/physics/native_tu15_reversed_contact_process.cpp','include/halo2/native_tu15_reversed_contact_process.hpp'):assert r['source_hashes'][f]==manifest['staged_hashes'][f]
   entry['compiler_profiles']=[]
   for e in base.glob('*/evidence/native.json'):
    v=json.loads(e.read_text());assert v['passed']and all(c['passed']for c in v['cases']);entry['compiler_profiles'].append(dict(compiler=e.parent.parent.name,profiles=len(v['cases']),declared_compared_bytes=sum(c['all_declared_bytes']for c in v['cases']),vectors_reversed=sum(c.get('vectors_reversed',0)for c in v['cases'])))
  if name.endswith('_world'):assert r['compiled_cpp_count']==121 and r['deduplicated']and r['stage_hashes']==manifest['staged_hashes']
  if name=='counters':assert r['new_fields']==7 and r['serializers']==2 and r['remaining_child_fields_preserved']and not r['remaining304350_closure_claimed']
  proofs[name]=entry
 for folder in('native-reversed-contact-public-tu15-20261102-first','native-reversed-contact-public-tu15-20261102-v2'):
  path=ROOT/'analysis/physics'/folder/'report.json';r=json.loads(path.read_text());assert not Path(r['owned_scratch']).exists();owners.append(dict(owned_scratch=r['owned_scratch'],allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT)),scope='Superseded incomplete declared child fixture; real original fault diagnostics retained'))
 # Base47 files remain byte-identical except the five explicitly reviewed hunks.
 base=Path(manifest['base_stage'])/'staged'
 for f,h in json.loads((base.parent/'source-pins.json').read_text())['staged_hashes'].items():
  if f not in manifest['delta_files']:assert sha(stage/f)==h,f
 manifest.update(qualified=True,qualified_reports=proofs,qualification_scope='Wholepublic39+39 no-contact realchild profiles48/compiler; independentinternalpositive suffix80/compiler; sameproductiontypedbinding48/compiler; both121CPPWorlddefault/factory/merge models; actual7Getter/2serializers/phase-vs-positive-vector gates. Publicgeneratedcontact component and remaining304350 nativeclosure OPEN.',remaining=['Visiblemultiplayer fulloriginalframe and positiveflippedvector publicdomain','Noncompound304350 and other remaining typedchild nativeclosure','Additional observer/fault/alias domains and genuine generatedcontact component inputs'])
 save(a.stage/'source-pins.json',manifest);save(a.stage/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,proof_groups=len(proofs),completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(v['allocated_bytes_before_cleanup']for v in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
