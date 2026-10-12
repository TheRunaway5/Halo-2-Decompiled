#!/usr/bin/env python3
"""Verify user-owned original collision projection tables in both images."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code

IMAGES = (
    ('base', 'assets/disc/default.xbe',
     '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',
     0x440B94, (0x1DEB1B, 0x1DEC3D)),
    ('disc1.1', 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
     'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
     0x4471E4, (0x1E250B, 0x1E262D)),
)
TABLE_PIN = 'afaf7ee3fe87b6989541d81f79a6a737b1e7cadede1ab3ed6737c6a29b43c5db'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/map-pack/paired-spartan-projection-table.json')
    args = p.parse_args()
    rows = []
    for name,path,pin,at,sites in IMAGES:
        image = Xbe(ROOT/path)
        assert hashlib.sha256(image.data).hexdigest() == pin
        raw = image.read(at,36)
        assert hashlib.sha256(raw).hexdigest() == TABLE_PIN
        code = _Code(image,'.text')
        refs = []
        for site in sites:
            i = code.at(site)
            assert i.mnemonic == 'movsx' and f'0x{at:x}' in i.op_str
            refs.append({'instruction':f'{i.address:08x}', 'bytes':i.bytes.hex(),
                         'mnemonic':i.mnemonic, 'operands':i.op_str})
        rows.append({'image':name,'path':path,'sha256':pin,
                     'table_va':f'{at:08x}','table_hex':raw.hex(),
                     'table_sha256':TABLE_PIN,'instruction_references':refs})
    assert rows[0]['table_hex'] == rows[1]['table_hex']
    report = {'identical':True,'table_size':36,
              'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'images':rows}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('Verified identical 36-byte original collision projection tables.')


if __name__ == '__main__':main()
