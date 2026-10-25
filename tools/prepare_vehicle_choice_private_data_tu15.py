#!/usr/bin/env python3
"""Replace only this active snapshot's data alias with an identical owned copy."""
import argparse
import json
import os
from pathlib import Path
import shutil
from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepared',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    prepared=json.loads(args.prepared.read_text());source=owner/'source'
    if Path(prepared['owner'])!=owner:raise ValueError('Source owner differs')
    if any(sha(source/name)!=pin for name,pin in prepared['source_hashes'].items()):raise ValueError('Source drift before data copy')
    data=source/'data';original=ROOT/'data';copy=owner/'vehicle-choice-private-data'
    if not data.is_symlink() or data.resolve()!=original.resolve():raise ValueError('Expected the authenticated original data alias')
    if any(path.is_symlink()for path in original.rglob('*')):raise ValueError('Nested data aliases are unsupported')
    pins={str(path.relative_to(original)):sha(path)for path in original.rglob('*')if path.is_file()}
    shutil.copytree(original,copy)
    if any(sha(original/name)!=pin or sha(copy/name)!=pin for name,pin in pins.items()):raise ValueError('Data changed during copy')
    data.unlink();copy.rename(data)
    if any(sha(source/name)!=pin for name,pin in prepared['source_hashes'].items()):raise ValueError('Source changed during data copy')
    report=dict(status='prepared',passed=False,validation_not_run=True,owner=str(owner),
        original_preserved=True,source_bytes_unchanged=True,data_sha256=pins,
        temporary_data_bytes=sum((data/name).stat().st_size for name in pins),
        prepared_sha256=sha(args.prepared),retained_large_artifacts=[],full_vehicle_parity=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(status='prepared',files=len(pins))),flush=True)


if __name__=='__main__':main()
