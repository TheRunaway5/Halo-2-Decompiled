#!/usr/bin/env python3
"""Read-only projectile inventory and Xbox/MCC comparison; run via run_test.py."""
import argparse
import hashlib
import json
import os
from pathlib import Path

from inspect_multiplayer import Cache
from mcc_content import MccCache, directory
from audit_rocket_ghost_reference_tu15 import inspect
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def extract(cache, tag, mcc=False):
    u = cache.u32 if mcc else cache.u
    p = tag['offset']
    ref = lambda value: ([cache.tags[value]['group'], cache.tags[value]['name']]
                         if value in cache.tags else None)
    def symbol(value):
        if not value:
            return ''
        index = value & 65535
        count, table, strings = (48, 60, 52) if mcc else (356, 364, 368)
        if index >= u(count):
            raise ValueError('Attachment symbol exceeds its owning StringID bank')
        location = u(strings) + u(u(table) + index * 4)
        end = cache.data.find(b'\0', location)
        if end < location:
            raise ValueError('Unterminated attachment symbol')
        return cache.data[location:end].decode('utf-8')
    result = inspect(cache, tag, mcc)
    result['model'] = ref(u(p + 0x38))
    # Include the complete attachment contract omitted by the earlier audit.
    result['attachment_words'] = [
        {'reference': ref(u(a + 4)), 'marker': u(a + 8),
         'color_flags': u(a + 0xc), 'primary_function': u(a + 0x10),
         'secondary_function': u(a + 0x14),
         'marker_name': symbol(u(a + 8)),
         'primary_function_name': symbol(u(a + 0x10)),
         'secondary_function_name': symbol(u(a + 0x14))}
        for a in cache.block(p + 0x94, 0x18)]
    result['object_flags'] = u(p) >> 16
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    assert sha(NEW_DEFAULT) == NEW_PIN
    root = Path(__file__).resolve().parents[1]
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    dlc = Path.home() / '.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c'
    paths = sorted((root / 'assets/disc/maps').glob('*.map'))
    paths += sorted(p for p in dlc.rglob('*.map') if not p.parent.name.endswith('old'))
    definitions, comparisons, files = {}, [], []
    shared = MccCache(maps / 'shared.map')
    try:
        for path in paths:
            if path.stem in {'shared', 'single_player_shared', 'mainmenu'}:
                continue
            original = Cache(path)
            imported = MccCache(maps / path.name) if (maps / path.name).is_file() else None
            try:
                files.append({'map': path.stem, 'xbox_sha256': sha(path),
                              'mcc_sha256': sha(maps / path.name) if imported else None})
                for tag in original.tags.values():
                    if tag['group'] != 'proj' or tag['offset'] is None:
                        continue
                    value = extract(original, tag)
                    entry = definitions.setdefault(tag['name'], {'maps': [], 'contracts': []})
                    entry['maps'].append(path.stem)
                    if value not in entry['contracts']:
                        entry['contracts'].append(value)
                    if not imported:
                        continue
                    other = None
                    for donor in (imported, shared):
                        other = next((t for t in donor.tags.values()
                            if t['group'] == 'proj' and t['name'] == tag['name'] and t['offset'] is not None), None)
                        if other:
                            break
                    if not other:
                        comparisons.append({'map': path.stem, 'name': tag['name'], 'missing': True})
                        continue
                    actual = extract(donor, other, True)
                    # Raw string IDs differ by platform. Compare normalized
                    # function names from inspect(); retain raw words separately.
                    keys = ('projectile_words', 'effects_and_damage_references', 'attachments', 'model', 'object_flags')
                    differences = {key: {'xbox': value[key], 'mcc': actual[key]}
                                   for key in keys if value[key] != actual[key]}
                    original_rows, mcc_rows = value['attachment_words'], actual['attachment_words']
                    canonical = lambda rows: [{k:v for k,v in row.items()
                                               if k in ('reference', 'color_flags', 'marker_name',
                                                        'primary_function_name', 'secondary_function_name')} for row in rows]
                    if canonical(original_rows) != canonical(mcc_rows):
                        differences['attachment_contract'] = {
                            'xbox': canonical(original_rows), 'mcc': canonical(mcc_rows)}
                    if differences:
                        comparisons.append({'map': path.stem, 'name': tag['name'], 'differences': differences})
            finally:
                original.data.close();original.stream.close()
                if imported:
                    imported.close()
    finally:
        shared.close()
    groups = {}
    for name, value in definitions.items():
        for contract in value['contracts']:
            for row in contract['attachment_words']:
                if row['reference']:
                    groups.setdefault(row['reference'][0], set()).add(name)
    report = {'scope': 'Authored metadata only; no gameplay/render parity certification',
              'reference': 'OG Xbox disc/DLC maps with pinned English TU1.5 executable',
              'tu15_sha256': NEW_PIN, 'tool_sha256': sha(__file__), 'source_files': files,
              'definitions': definitions, 'mcc_comparison_differences': comparisons,
              'summary': {'map_inputs': len(files), 'distinct_maps': len({f['map'] for f in files}),
                          'projectile_definitions': len(definitions),
                          'projectiles_by_attachment_group': {g: sorted(v) for g,v in groups.items()},
                          'mcc_difference_rows': len(comparisons)}}
    data = json.dumps(report, indent=2) + '\n'
    if len(data.encode()) > 1024 * 1024:
        raise ValueError('Report exceeds small-report budget')
    args.report.parent.mkdir(parents=True, exist_ok=True);args.report.write_text(data)
    print(json.dumps({'maps': len(files), 'projectiles': len(definitions),
                      'mcc_difference_rows': len(comparisons), 'bytes': len(data.encode())}))


if __name__ == '__main__':
    main()
