#!/usr/bin/env python3
"""Seal exact extended point PART callback, source pins and owned cleanup."""
import argparse,hashlib,json
from pathlib import Path
from build_native_tu15_constraint_before_reference import sha,save
ROOT=Path(__file__).resolve().parents[1]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--stage',required=True,type=Path);a=p.parse_args();a.stage=a.stage.resolve();stage=a.stage/'staged';manifest=json.loads((a.stage/'source-pins.json').read_text());assert all(sha(stage/r)==v for r,v in manifest['staged_hashes'].items())
 path=ROOT/'analysis/physics/native-point-part-extended-callback-tu15-20261102-first/report.json';r=json.loads(path.read_text());assert r['passed']and r['original_cases']==261
 header=stage/'include/halo2/native_tu15_contact_point_binding.hpp';text=header.read_text();start=text.index('        gameplay.release_part_contact=');end=text.index('        NativeTu15IslandConstraintDetachServices detach;',start);callback=text[start:end]
 assert r['private_extended_point_header_sha256']==sha(header)and r['point_callback_sha256']==hashlib.sha256(callback.encode()).hexdigest()
 assert r['body_coverage']['0x1e0a60']==dict(total=418,executed=418)
 for row in r['positive_declared_function_kind_activity'].values():assert row['profiles']==[4]*11 and row['calls']==[16]*11
 hashes=r.get('source_sha256',r.get('source_hashes',{}));assert all(sha(ROOT/f)==v for f,v in hashes.items())
 owner=r['scratch_cleanup_owner'];assert not Path(owner).exists()
 manifest.update(qualified=True,qualified_report=str(path.relative_to(ROOT)),qualified_report_sha256=sha(path),kernel_source_hashes=hashes,
  point_callback_sha256=r['point_callback_sha256'],profiles_per_compiler=261,positive_kind_profiles_per_compiler=[4]*11,positive_kind_calls_per_compiler=[16]*11,
  point_scratch_layout=r['point_scratch_layout'],remaining=['Whole221 gameplay caller extended composition','RootWorld/getter/twoJSON/gates stage','Genuine captured positivePART/conveyor/extendedfunction runtime selection'],
  scope='Exact callback and retained scratch aliases on261/all11positive declaredPART profiles BOTH,6mutants,fullpersistentDATA/stores/reads/FP. Wholepoint header inherits boundedstage; fullheadercaller/runtime not newlyclaimed. No boundedRoot edits, no childresults or CPU state supplied.')
 save(a.stage/'source-pins.json',manifest);save(a.stage/'cleanup-verification.json',dict(verified=True,completed_owners=[dict(owned_scratch=owner,allocated_bytes_before_cleanup=r['temporary_allocated_bytes_before_cleanup'],removed_verified=True,report=str(path.relative_to(ROOT)))],retained_large_artifacts=[],protected_reference_inputs_preserved=True));print(dict(qualified=True,profiles=261,measured_parent_allocation_bytes_removed=r['temporary_allocated_bytes_before_cleanup']));return 0
if __name__=='__main__':raise SystemExit(main())
