#!/usr/bin/env python3
"""Emit finite HUD shader metadata guards, without pixel or shader-program bytes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
ROOT=Path(__file__).resolve().parents[1]
DATA_SHA256='b7e56c5c02a27277858dbac54c7ff60481ba7741b63ba39a57fa714cc93bc5ff'
def generate(raw):
    if hashlib.sha256(raw).hexdigest()!=DATA_SHA256:
        raise ValueError('unrecognized HUD shader metadata authority')
    data=json.loads(raw)
    if data['schema']!='H2_MCC_GREY_WIDGET_FLASH_METADATA_V1':
        raise ValueError('unsupported HUD shader metadata schema')
    for closure in data['native_closures']:
        for member in ('template','pass'):
            if closure[member]['strings']:
                raise ValueError('unexpected native HUD program StringID owner')
    policy={'source':data['canonical_source'],'preimages':list(dict.fromkeys(p['normalized_allocation_sha256']for p in data['source_profiles'])),'native_closures':data['native_closures']}
    return '// Finite source and retained-native metadata identities. No program or pixel bytes.\nconstexpr std::string_view hud_shader_policy=R"H2HSG('+json.dumps(policy,sort_keys=True,separators=(',',':'))+')H2HSG";\n'
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data',type=Path,default=ROOT/'tools/data/mcc-hud-shader-reference.json')
    parser.add_argument('--output',type=Path,default=ROOT/'src/mcc_hud_shader_reference.inc')
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    try:
        output=generate(args.data.read_bytes())
        if args.check:
            if args.output.read_text()!=output:raise ValueError('HUD shader reference output is stale')
        else:
            with tempfile.NamedTemporaryFile(mode='w',dir=args.output.parent,delete=False)as file:
                pending=Path(file.name);file.write(output)
            try:os.replace(pending,args.output)
            finally:pending.unlink(missing_ok=True)
    except(OSError,ValueError,KeyError,TypeError)as error:parser.exit(1,f'{error}\n')
if __name__=='__main__':main()
