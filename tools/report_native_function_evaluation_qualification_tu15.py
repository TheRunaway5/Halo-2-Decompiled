#!/usr/bin/env python3
"""Freeze the bounded native function domain and disjoint compiler cleanup."""
import argparse,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False);base=ROOT/'analysis/physics/native-function-evaluation-bounded-tu15-20261102-v2';r=json.loads((base/'report.json').read_text());assert r['passed'];assert all(sha(ROOT/f)==h for f,h in r['source_hashes'].items());proofs=[]
 for e in base.glob('*/evidence/native.json'):
  v=json.loads(e.read_text());assert v['passed']and len(v['cases'])==504 and all(c['passed']for c in v['cases']);proofs.append(dict(compiler=e.parent.parent.name,profiles=len(v['cases']),declared_compared_bytes=sum(c['declared_compared_bytes']for c in v['cases']),source_instructions=sum(c['source_steps']for c in v['cases']),report=str(e.relative_to(ROOT)),sha256=sha(e)))
 assert {p['compiler']for p in proofs}=={'g++','clang++'};owners=[]
 for folder in('native-function-evaluation-bounded-tu15-20261102-first','native-function-evaluation-bounded-tu15-20261102-v2'):
  path=ROOT/'analysis/physics'/folder/'report.json';q=json.loads(path.read_text());assert not Path(q['owned_scratch']).exists();owners.append(dict(owned_scratch=q['owned_scratch'],allocated_bytes_before_cleanup=q['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT))))
 result=dict(qualified=True,source_hashes=r['source_hashes'],compiler_proofs=proofs,original_sha256=r['reference']['original_sha256'],admitted_kinds=[0,1],admitted_source_early_returns=['nullblob','signedcount<=0','kind>10'],all_eleven_kinds_qualified=False,native_only_within_admitted_domain=True,remaining_valid_kinds=list(range(2,11)),architecture_only_dispatch_fetch_excluded=r['architecture_only_dispatch_fetch_excluded'],scope='Nativeauthoredfunction8Breference parser, kind0/commonCOMISSclamp+kind1 andexactearlyreturns. NoSourcePC/GP/privateframe interpreter, callbacks or originaloutcomes supplied. Sourcecodejump-table fetch excluded fromsemanticDATAledger; enumdispatchnative. Othervalidkinds explicitlycountthrow; no wholefunction/liveconveyor claim.')
 save(a.output/'source-pins.json',result);save(a.output/'cleanup-verification.json',dict(verified=True,completed_owners=owners,retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,compiler_proofs=len(proofs),completed_owners=len(owners),measured_parent_allocation_bytes_removed=sum(o['allocated_bytes_before_cleanup']for o in owners)));return 0
if __name__=='__main__':raise SystemExit(main())
