#!/usr/bin/env python3
"""Recover 147A90's immutable direction table from the pinned 1.1 image.

The table must also match the untouched base game's table exactly. This tool
exports data only; no original executable instructions enter the native build.
"""
import argparse
import hashlib
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

IMAGES = (
    ('assets/disc/default.xbe',
     '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d', 0x4417f0),
    ('assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
     'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377', 0x447e40),
)
TABLE_PIN = '9581ebb544c806b2c88cdc2a094df6a0ac07978c9bb07dcb9f68fe2962fd2feb'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, help='Optional local binary export; never a source header')
    args = parser.parse_args()
    tables = []
    for path, expected, address in IMAGES:
        image = Xbe(ROOT / path)
        if hashlib.sha256(image.data).hexdigest() != expected:
            raise ValueError(f'unexpected image: {path}')
        data = image.read(address, 1026 * 12)
        if hashlib.sha256(data).hexdigest() != TABLE_PIN:
            raise ValueError(f'unexpected direction table: {path}')
        tables.append(data)
    if tables[0] != tables[1]:
        raise ValueError('base and update direction tables differ')
    if args.output:
        target = args.output.resolve()
        if target.is_relative_to(ROOT) and not any(
                target.is_relative_to(ROOT / name) for name in ('assets','analysis','.tools')):
            raise ValueError('original table exports must remain outside repository source')
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(tables[1])
    print('Verified 1026 identical original directions; runtime reads your local executable.')


if __name__ == '__main__':
    main()
