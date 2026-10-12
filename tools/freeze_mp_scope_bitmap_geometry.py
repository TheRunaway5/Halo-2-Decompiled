#!/usr/bin/env python3
"""Freeze the separate original HUD bitmap geometry/native fixture checkpoint."""
import argparse
import json
import subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_scope_bitmap_geometry import sha
from audit_mp_scope_bitmap_geometry import BASE

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--native-test',type=Path,required=True)
    parser.add_argument('--sanitized-test',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    audit_path=BASE/'audit-frozen/audit.json';audit=json.loads(audit_path.read_text())
    for name,digest in audit['source_files'].items():assert sha(ROOT/name)==digest
    for name,digest in audit['verified_prior_pins'].items():assert sha(ROOT/name)==digest
    assert audit['actual_authored_cases']==120 and audit['explicit_flag_controls']==32
    assert audit['ordered_draw_requests']==312
    files=dict(audit['source_files'])
    files['tools/freeze_mp_scope_bitmap_geometry.py']=sha(__file__)
    for folder in ['measured-complete','repeat-complete','update-measured','update-repeat','audit-frozen']:
        for path in (BASE/folder).rglob('*'):
            if path.is_file():files[str(path.relative_to(ROOT))]=sha(path)
    tests=[];fixture=BASE/'measured-complete/fixtures.tsv'
    for binary in [args.native_test,args.sanitized_test]:
        result=subprocess.run([str(binary.resolve()),str(fixture)],cwd=ROOT,capture_output=True,text=True,check=True)
        tests.append(dict(binary=str(binary.resolve()),binary_sha256=sha(binary),fixture=str(fixture.relative_to(ROOT)),fixture_sha256=sha(fixture),returncode=result.returncode,stdout=result.stdout,stderr=result.stderr))
    report=dict(schema=1,source_files=files,verified_prior_pins=audit['verified_prior_pins'],
                previous_scope_manifest_sha256=audit['previous_scope_manifest_sha256'],
                independently_identical_pairs=[['measured-complete','repeat-complete'],['update-measured','update-repeat']],
                native_tests=tests,actual_authored_cases=120,explicit_flag_controls=32,ordered_draw_requests=312,
                original_xbe_pins=audit['original_xbe_pins'],
                scope='Bounded regular original bitmap CPU construction, rounding, flips, stretch and ordered mirrors. Source output fixtures plus whole original static1.1/u5 correspondence. Live HUD/GPU/render validation belongs to root and is not claimed here.',
                module_runtime_integration_by_this_agent=False)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(manifest_sha256=sha(args.output),source_files=len(files),prior_pins=len(audit['verified_prior_pins']),native_tests=len(tests),doc_sha256=files['docs/MP_SCOPE_BITMAP_GEOMETRY_SOURCE_RESEARCH.md'])))

if __name__=='__main__':main()
