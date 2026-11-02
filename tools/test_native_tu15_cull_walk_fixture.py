#!/usr/bin/env python3
"""Replay compact required original44710 reference input; no original ELF needed."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import validate_native_tu15_cull_walk as original

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--fixture',type=Path,required=True);p.add_argument('--native',type=Path,required=True);a=p.parse_args()
    fixture=json.loads(gzip.decompress(a.fixture.read_bytes()))
    assert fixture['schema']=='TU15_NATIVE_CULL_WALK_V1'and fixture['original_xbe_sha256']==original.base.PIN
    class Image:
        def read(self,at,size):
            assert at==0x46a000 and size==4096
            return bytes.fromhex(fixture['constants'])
    rows=list(original.cases(Image()))
    assert [hashlib.sha256(json.dumps(row,sort_keys=True).encode()).hexdigest()for row in rows]==fixture['input_sha256']
    original.compare(rows,fixture['expected'],a.native)
    print('PASS',len(rows),'authenticated original cull-walk fixture cases')
    return 0
if __name__=='__main__':raise SystemExit(main())
