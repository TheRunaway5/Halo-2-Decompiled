#!/usr/bin/env python3
"""Replay the compact physical-original shadow reference without an XBE."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    fixture=json.loads(gzip.decompress(a.fixture.read_bytes()))
    if fixture['schema']!='TU15_NATIVE_SHADOW_ARITHMETIC_V1' or fixture['original_xbe_sha256']!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd' or fixture['instruction_patches']or fixture['supplied_original_outcomes']:
        raise ValueError('Wrong compact physical reference provenance')
    inputs=''.join(json.dumps(c['input'],separators=(',',':'))+'\n'for c in fixture['cases'])
    result=subprocess.run([str(a.native)],input=inputs,text=True,capture_output=True,timeout=30)
    if result.returncode:raise ValueError('Native fixture execution failed: '+result.stderr[-3000:])
    rows=result.stdout.splitlines()
    if len(rows)!=len(fixture['cases']):raise ValueError('Native fixture row count differs')
    for c,line in zip(fixture['cases'],rows,strict=True):
        if json.loads(line)!=c['expected']:raise ValueError(c['input']['name']+': compact physical reference differs')
    print('PASS',len(rows),'compact physical shadow reference cases; sha256',hashlib.sha256(a.fixture.read_bytes()).hexdigest())
    return 0


if __name__=='__main__':raise SystemExit(main())
