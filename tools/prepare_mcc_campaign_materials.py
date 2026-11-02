#!/usr/bin/env python3
"""Prepare and validate the missing beach bitmap in a separate owned MCC cache.

Run through tools/run_test.py. Publication retains a required runtime input,
never a build product; the source cache and runtime selection stay unchanged.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess

from mcc_prepared_cache_delta import _snapshot, _snapshot_sha, _clone_tree, _publish
from prepare_mcc_water_beach_owner import prepare


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--destination',type=Path,required=True)
    parser.add_argument('--validator',type=Path,required=True)
    parser.add_argument('--report-output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    root=Path(__file__).resolve().parents[1]
    source=args.source.resolve(strict=True);destination=args.destination.absolute()
    if destination.exists() or destination.is_symlink():parser.error('Destination must be a new cache')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);candidate=scratch/'prepared';candidate.mkdir()
    output=args.report_output.resolve();output.mkdir(parents=True,exist_ok=False)
    before=_snapshot(source)
    clone=_clone_tree(source,candidate,before)
    receipt=prepare(root,source,candidate,output/'preparation.json')
    command=[str(args.validator.resolve(strict=True)),str(root),str(candidate),
        str(root/'tools/data/mcc-native-water-beach-owner-v1.json'),str(output/'bitmap-admission.json')]
    with (output/'validator.log').open('wb')as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=180)
    if result.returncode:
        raise RuntimeError('Complete native beach admission failed: '+(output/'validator.log').read_text()[-2000:])
    if _snapshot(source)!=before:raise RuntimeError('Read-only prepared source changed')
    _publish(candidate,destination)
    receipt.update(published=str(destination),selected=False,clone=clone,
        source_tree_snapshot_sha256=_snapshot_sha(before),original_cache_unchanged=True,
        native_bitmap_admission=json.loads((output/'bitmap-admission.json').read_text()),
        retained_runtime_input=str(destination),retained_large_build_artifacts=[])
    (output/'report.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'published':str(destination),'selected':False,
        'frames':receipt['native_bitmap_admission']['frames'],'mips':receipt['native_bitmap_admission']['mips'],
        'logical_bytes':clone['logical_bytes'],'copy_method':clone['copy_method']}))


if __name__=='__main__':main()
