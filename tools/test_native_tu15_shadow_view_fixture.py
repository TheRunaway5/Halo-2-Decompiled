#!/usr/bin/env python3
"""Compact whole-original shadow view evidence; use run_test.py."""
import argparse
import gzip
import json
from pathlib import Path
from validate_native_tu15_shadow_view import PIN,cases,compare,digest


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    fixture=json.loads(gzip.decompress(a.fixture.read_bytes()));rows=list(cases(fixture['constants'],fixture['pointers']))
    if fixture['schema']!='TU15_NATIVE_SHADOW_VIEW_V1'or fixture['original_xbe_sha256']!=PIN:raise ValueError('Shadow view fixture authority differs')
    if fixture['inputs_sha256']!=[digest(r)for r in rows]:raise ValueError('Shadow view fixture declared inputs differ')
    print('PASS',compare(rows,fixture['expected'],a.native)['comparisons'],'whole shadow view comparisons')


if __name__=='__main__':main()
