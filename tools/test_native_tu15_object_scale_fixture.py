#!/usr/bin/env python3
"""Portable compact original-evidence regression; run through run_test.py."""
import argparse
import gzip
import json
from pathlib import Path
from validate_native_tu15_object_scale import PIN,cases,compare,digest


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    fixture=json.loads(gzip.decompress(a.fixture.read_bytes()));rows=list(cases())
    if fixture['schema']!='TU15_NATIVE_OBJECT_SCALE_V1'or fixture['original_xbe_sha256']!=PIN:raise ValueError('Fixture authority differs')
    if fixture['inputs_sha256']!=[digest(r)for r in rows]:raise ValueError('Fixture declared inputs differ')
    result=compare(rows,fixture['expected'],a.native);print('PASS',result['comparisons'],'compact original scale comparisons')


if __name__=='__main__':main()
