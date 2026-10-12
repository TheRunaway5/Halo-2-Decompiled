#!/usr/bin/env python3
"""Audit an original19BE0 fixture against its observed scalar-mode0 contract.

This is a derived interpretation check, not another original/native oracle.
It does not generate the fixture's expected outputs or execute game code.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, default=ROOT / 'tests/fixtures/camouflage_vertex_extern_original.json')
    ap.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/game-variants/camouflage-binder-20261011/vertex-contract-audit.json')
    args = ap.parse_args()
    raw = args.fixture.read_bytes()
    fixture = json.loads(raw)
    counts = dict(cases=0, scalar_writes=0, rgb_writes=0)
    for row in fixture['cases']:
        expected = bytearray.fromhex(row['initial_vertex_hex'])
        dirty = row['initial_vertex_dirty_masks'][:]
        assert len(row['callee_inputs']) == len(row['mappings'])
        for mapping, actual in zip(row['mappings'], row['callee_inputs']):
            dest, ignored_byte, source = struct.unpack('<HBB', bytes.fromhex(mapping))
            bank, register, component = (dest >> 4) & 1, dest & 15, (dest >> 5) & 31
            assert actual == dict(texture_selector=dest >> 13, scalar_mode=0, source=source, component=component)
            at = bank * 256 + register * 16
            if component < 4:
                expected[at + component * 4:at + component * 4 + 4] = struct.pack('<I', row['scalars_bits'][str(source)])
                counts['scalar_writes'] += 1
            elif component == 4:
                expected[at:at + 12] = struct.pack('<3I', *row['colors_bits'][str(source)])
                counts['rgb_writes'] += 1
            else:
                raise AssertionError('Dimension component is outside this audit')
            dirty[bank] |= 1 << register
        assert expected.hex() == row['vertex_hex']
        assert dirty == row['vertex_dirty_masks']
        counts['cases'] += 1
    control = fixture['cases'][4320]
    assert control['mappings'] == ['00200000']
    assert control['scalars_bits']['0'] == 0xc0000000
    assert bytes.fromhex(control['vertex_hex'])[:4] == struct.pack('<I', 0xc0000000)
    report = dict(schema='camouflage-vertex-contract-audit-v1', boundary=__doc__,
                  fixture_sha256=hashlib.sha256(raw).hexdigest(),
                  auditor_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), counts=counts,
                  all_banks_masks_and_callee_inputs_match=True,
                  correction_control=dict(case=4320, mapping=control['mappings'][0],
                                          callee_input=control['callee_inputs'][0], scalar_bits=0xc0000000, output_bits=0xc0000000))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(counts, indent=2))


if __name__ == '__main__':
    main()
