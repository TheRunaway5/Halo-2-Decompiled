#!/usr/bin/env python3
"""Freeze literal original partial-range inputs/outputs; never solve expectations.

Input is generate-original.py's unchanged retail execution at four canonical
ticks. It records the all-resident BSP-only schedule, not dynamic133720 or
asynchronous loading. Binary v1 is LE u32 banks (length then words), with raw
geometry byte banks likewise length-prefixed. The native test reads the real
map independently and compares its owned resources to these literal banks.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
MANIFEST_SHA = '48d9dba463f9f276393ac56bb6d6e763a0d4e7fd3afe44680a6eaacc8f601992'
MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
TICKS = (0, 181, 293, 840)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


class Output:
    def __init__(self):
        self.data = bytearray(b'H2BRG001')

    def words(self, *values):
        self.data += struct.pack('<' + 'I' * len(values), *(v & 0xffffffff for v in values))

    def bank(self, values):
        self.words(len(values), *values)

    def raw(self, raw):
        self.words(len(raw))
        self.data += raw


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--retained', type=Path, default=ROOT / 'analysis/frontend/title-partial-range-live/retained')
    p.add_argument('--output', type=Path, default=ROOT / 'tests/fixtures/source_bsp_ranges_original.bin')
    args = p.parse_args()
    manifest_raw = (args.retained / 'manifest.json').read_bytes()
    assert sha(manifest_raw) == MANIFEST_SHA, 'retained original manifest changed'
    manifest = json.loads(manifest_raw)
    assert manifest['source_unchanged'] and manifest['map_sha256'] == MAP_SHA
    assert tuple(c['tick'] for c in manifest['cases']) == TICKS
    resources = {}
    for row in manifest['resource_files']:
        raw = (args.retained / row['file']).read_bytes()
        assert len(raw) == row['bytes'] and sha(raw) == row['sha256']
        key = (row['kind'], row['section'], row['type'], row['channel'])
        assert key not in resources
        resources[key] = raw
    out = Output()
    # Hashes are literal provenance bytes, not hashes inferred from native output.
    out.data += bytes.fromhex(MANIFEST_SHA) + bytes.fromhex(MAP_SHA)
    out.words(len(manifest['section_banks']), len(TICKS))
    for section in manifest['section_banks']:
        kind, ordinal = section['kind'], section['section']
        header = (args.retained / section['header_file']).read_bytes()
        assert sha(header) == section['header_sha256'] and header.hex() == section['original_header_hex']
        out.words(0 if kind == 'cluster' else 1, ordinal)
        out.raw(header)
        for resource_type, channel in ((0, 72), (8, 8), (40, 65535), (48, 2), (32, 2)):
            out.raw(resources.get((kind, ordinal, resource_type, channel), b''))
    totals = dict(units=0, queries=0, draws=0, indices=0)
    for case in manifest['cases']:
        raw = (args.retained / case['file']).read_bytes()
        assert sha(raw) == case['sha256']
        data = json.loads(raw)
        units = data['units']
        # The imported original constructor asserts actual mode/query0, and
        # the schedule's final original2CBF0 count is the actual main prefix.
        final_count = data['collector_calls'][-1]['count_after']
        assert final_count == len(units) == case['units']
        assert all(call['accepted'] for call in data['collector_calls'])
        arena_address = data['allocations'][0]['context'] + 0x25c
        assert all(a['context'] + 0x25c == arena_address for a in data['allocations'])
        out.words(case['tick'], final_count, final_count, 0, 0, arena_address)
        out.bank(data['input_words'])
        out.words(len(data['visibility_group_words']))
        for bank in data['visibility_group_words']:
            assert len(bank) == 111
            out.bank(bank)
        out.bank(data['visibility_words'])
        out.words(len(data['association_words']))
        for bank in data['association_words']:
            assert len(bank) == 4
            out.bank(bank)
        for phase in ('initial', 'before_cull', 'after_cull', 'final'):
            assert len(data['arena'][phase + '_words']) == 512
            out.words(data['arena'][phase + '_used'])
            out.bank(data['arena'][phase + '_words'])
        out.words(len(data['allocations']))
        for a in data['allocations']:
            out.words(a['subparts'], a['used_before'], a['used_after'], a['returned'],
                      0xffffffff if a['arena_word'] is None else a['arena_word'])
        out.words(len(units))
        for ordinal, unit in enumerate(units):
            assert unit['unit'] == ordinal
            assert len(unit['before']) == len(unit['after']) == 8
            assert unit['transform_words'] is None or len(unit['transform_words']) == 13
            out.words(unit['geometry'], 0 if unit['kind'] == 'cluster' else 1, unit['section'],
                      0xffffffff if unit['mask_word_offset'] is None else unit['mask_word_offset'])
            out.bank(unit['before'])
            out.bank(unit['after'])
            out.bank(unit['transform_words'] or [])
            out.bank(unit['mask'])
            out.words(len(unit['parts']))
            for part in unit['parts']:
                out.words(part['part'], part['returned_index_count'], len(part['draws']))
                for draw in part['draws']:
                    indices = draw['indices']
                    assert len(indices) == draw['count']
                    assert sha(struct.pack('<' + 'H' * len(indices), *indices)) == draw['indices_sha256']
                    assert draw['primitive'] in (5, 6)
                    out.words(draw['primitive'], draw['first'], draw['count'])
                    out.bank(indices)
                    totals['indices'] += len(indices)
        totals['units'] += len(units)
        totals['queries'] += len(data['queries'])
        totals['draws'] += case['draws']
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(out.data)
    print(json.dumps(dict(file=str(args.output.relative_to(ROOT)), bytes=len(out.data),
                         sha256=sha(out.data), banks=len(manifest['section_banks']), cases=4, **totals)))


if __name__ == '__main__':
    main()
