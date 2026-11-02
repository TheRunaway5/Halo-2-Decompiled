#!/usr/bin/env python3
"""Portable replay of compact original renderer script-flag effects."""
import argparse
import gzip
import json
from pathlib import Path

from validate_native_tu15_render_script_flags import PIN,compare,digest,inputs


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    fixture=json.loads(gzip.decompress(a.fixture.read_bytes()))
    if fixture['schema']!='TU15_NATIVE_RENDER_SCRIPT_FLAGS_V1'or fixture['original_xbe_sha256']!=PIN:raise ValueError('Wrong physical reference provenance')
    rows=inputs()
    if [digest(r)for r in rows]!=fixture['inputs_sha256']:raise ValueError('Declared input construction changed after physical qualification')
    result=compare(rows,fixture['expected'],a.native);print('PASS',result['cases'],'portable physical render-script flag comparisons')
    return 0


if __name__=='__main__':raise SystemExit(main())
