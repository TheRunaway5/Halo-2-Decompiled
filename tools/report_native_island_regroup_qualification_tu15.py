#!/usr/bin/env python3
"""Retain bounded proof receipts and verify completed partition scratch removal."""
import argparse,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FAMILIES={
 'allocator-binding':['native-island-partition-allocator-binding-actual09-20261101','native-island-partition-allocator-binding-actual09-clang-20261101'],
 'temporary':['native-island-partition-temporary-actual09-20261101','native-island-partition-temporary-actual09-clang-20261101'],
 'construction':['native-island-construction-20261101'],
 'edges':['native-island-partition-edges-actual09-20261101','native-island-partition-edges-actual09-clang-20261101'],
 'parent':['native-island-regroup-parent-actual09-declared-20261101','native-island-regroup-parent-actual09-declared-clang-20261101'],
 'service-binding':['native-island-regroup-binding-v2-actual09-declared-20261101']}
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p,j):p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(j,indent=2,sort_keys=True)+'\n')
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();proofs={};allpins={};owners=[]
 for name,folders in FAMILIES.items():
  rows=[];pins={};familyowners=[]
  for folder in folders:
   base=ROOT/'analysis/physics'/folder;r=json.loads((base/'report.json').read_text());assert r['passed']
   for path,h in r['source_hashes'].items():
    assert digest(ROOT/path)==h,path
    assert path not in pins or pins[path]==h,path
    pins[path]=h;allpins[path]=h
   owner=Path(r['owned_scratch']);assert not owner.exists(),owner
   item=dict(owned_scratch=str(owner),allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str((base/'report.json').relative_to(ROOT)))
   assert item['allocated_bytes_before_cleanup']>0;owners.append(item);familyowners.append(item)
   for report in sorted(base.glob('*/evidence/native.json')):
    evidence=json.loads(report.read_text());assert evidence['passed'];cases=evidence['cases'];assert cases and all(c['passed']for c in cases)
    sums={k:sum(c.get(k,0)for c in cases)for k,v in cases[0].items()if isinstance(v,int)and not isinstance(v,bool)and k not in ['depth','mode','selection','variation','declared_profile','declared_count','kind','declared_graph','declared_output_capacity','island','declared_private_bodies']}
    rows.append(dict(compiler=report.parent.parent.name,profiles=len(cases),sums=sums,report=str(report.relative_to(ROOT))))
  assert {r['compiler']for r in rows}=={'g++','clang++'}
  result=dict(qualified=True,source_hashes=pins,compiler_proofs=rows,native_original_CPU_children=False,live_qualified=False)
  result['scope']={'allocator-binding':'Caller-owned native array/raw/pool/arena service composition on actual09 allocator DATA and declared graph topology','temporary':'Full202-site temporary4/8-array construction/lifetime with native arena/pool and declared count/growth controls','construction':'Full49-site3212C0 original stores, declared initialized allocation/world DATA','edges':'Full209-site321EB0 collector on actual09 unchanged formal island DATA; actions and dynamic/dynamic contact positive remain open','parent':'Whole482-site normal-completion3187C0: actual09 World flag skips plus explicitly declared coherent1/6-body no-row split controls','service-binding':'Same sole production native service builder,72profiles/compiler with depth0..5; unknown owners counted and refused without original CPU fallback'}[name]
  result['remaining']=['Genuine active-positive and visible multiplayer repartition','Positive action/constraint/contact redistribution callbacks','Empty-island and callback fault/unwind/alias domains','Fixed/secondary/lazy-default and large/exhausted allocator admission']
  destination=ROOT/'analysis/physics'/('native-island-regroup-'+name+'-qualified-20261101');save(destination/'source-pins.json',result);save(destination/'cleanup-verification.json',dict(verified=True,completed_owners=familyowners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));proofs[name]=str((destination/'source-pins.json').relative_to(ROOT))
 result=dict(qualified=True,component_proofs=proofs,source_hashes=allpins,native_original_CPU_children=False,production_binding_qualified=False,genuine_active_positive_partition_claimed=False,live_qualified=False,completed_owners=owners,retained_large_artifacts=[])
 save(a.output,result);print(json.dumps(dict(qualified=True,proofs=len(proofs),completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(r['allocated_bytes_before_cleanup']for r in owners))))
 return 0
if __name__=='__main__':raise SystemExit(main())
