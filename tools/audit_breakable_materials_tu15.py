#!/usr/bin/env python3
"""Inventory original/MCC BSP breakable records without supplying live state.

Run through tools/run_test.py. Scenario-owned Xbox BSP partitions and resident
MCC sbsp metadata are separate address spaces; neither uses a guessed tag magic.
Schema field names are navigation. This report does not establish the original
damage producer, dynamic breakable bitmap, fragment physics or render delivery.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from inspect_multiplayer import Cache
from mcc_content import MccCache, directory


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def extract(data, at, magic, lower, upper):
    def require(offset, size):
        if offset < lower or offset > upper or size < 0 or size > upper - offset:
            raise ValueError('Breakable metadata escapes its owning BSP partition')
    def word(offset):
        require(offset, 4)
        return struct.unpack_from('<I', data, offset)[0]
    def block(offset, stride):
        count = word(offset)
        if count > 65536:
            raise ValueError('Excessive breakable metadata count')
        if not count:
            return []
        start = word(offset + 4) - magic
        require(start, count * stride)
        return [start + index * stride for index in range(count)]
    records = []
    for offset in block(at + 0xbc, 0x18):
        instance, surface = struct.unpack_from('<hh', data, offset)
        records.append({'instance': instance, 'breakable': surface,
                        'centroid_radius_words': [f'{word(offset + field):08x}' for field in (4, 8, 12, 16)],
                        'collision_surface': struct.unpack_from('<i', data, offset + 20)[0]})
    collision_breakables = []
    for index, collision in enumerate(block(at + 0x14, 0x40)):
        for surface_index, surface in enumerate(block(collision + 0x28, 8)):
            if data[surface + 4] & 8:
                collision_breakables.append({'record': index, 'surface': surface_index,
                    'raw_hex': data[surface:surface + 8].hex()})
    return {'records': records, 'flagged_terrain_surfaces': collision_breakables}


def original_bsps(cache):
    result = {}
    for row in cache.block(cache.scenario + 0x210, 0x44):
        start, size, address = (cache.u(row + field) for field in (0, 4, 8))
        if start > len(cache.data) or size > len(cache.data) - start or size < 0xd4:
            raise ValueError('Original scenario BSP partition is truncated')
        tag = cache.tags[cache.u(row + 0x14)]
        if tag['group'] != 'sbsp' or tag['name'] in result:
            raise ValueError('Original scenario BSP identity is invalid/duplicated')
        result[tag['name']] = extract(cache.data, start + 16, address - start, start, start + size)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census['schema'] != 'H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require the completed destruction-contract source census')
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    rows = []
    for file in census['source_files']:
        path = Path(file['path'])
        if sha(path) != file['sha256']:
            raise ValueError('Original map changed since its contract census')
        original = Cache(path)
        # Same logical filename mapping as compare_pc_tag_banks.py; actual
        # sbsp identities and records must still compare independently.
        mcc_path = maps / ('highplains.map' if path.stem == 'highplains2' else path.name)
        imported = MccCache(mcc_path) if mcc_path.is_file() else None
        try:
            expected = original_bsps(original)
            actual = {tag['name']: extract(imported.data, tag['offset'], imported.magic, 0, len(imported.data))
                      for tag in imported.tags.values() if tag['group'] == 'sbsp' and tag['offset'] is not None} if imported else {}
            for name, records in expected.items():
                other = actual.get(name)
                rows.append({'map': path.stem, 'mcc_path': str(mcc_path) if imported else None,
                             'mcc_sha256': sha(mcc_path) if imported else None, 'bsp': name, 'xbox': records,
                             'mcc': other, 'matching': records == other})
        finally:
            original.data.close();original.stream.close()
            if imported:
                imported.close()
    summary = {'bsp_pairs': len(rows), 'xbox_breakable_records': sum(len(row['xbox']['records']) for row in rows),
               'mcc_breakable_records': sum(len((row['mcc'] or {}).get('records', [])) for row in rows),
               'xbox_flagged_terrain_surfaces': sum(len(row['xbox']['flagged_terrain_surfaces']) for row in rows),
               'mcc_flagged_terrain_surfaces': sum(len((row['mcc'] or {}).get('flagged_terrain_surfaces', [])) for row in rows),
               'different_bsp_pairs': sum(not row['matching'] for row in rows)}
    report = {'scope': __doc__, 'inventory_completed': True, 'runtime_verified': False,
              'contract_census': str(args.contracts), 'contract_census_sha256': sha(args.contracts),
              'tool_sha256': sha(__file__), 'summary': summary, 'rows': rows}
    data = json.dumps(report, separators=(',', ':')) + '\n'
    if len(data.encode()) > 1024 * 1024:
        raise ValueError('Breakable metadata exceeds the small-report budget')
    args.report.parent.mkdir(parents=True, exist_ok=True);args.report.write_text(data)
    print(json.dumps(summary | {'report': str(args.report), 'bytes': len(data.encode())}))


if __name__ == '__main__':
    main()
