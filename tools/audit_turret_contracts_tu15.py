#!/usr/bin/env python3
"""Compare turret weapon/projectile inputs with offline Xbox TU1.5 references.

Run through run_test.py. This records declared fields, not runtime acceptance.
References/StringIDs are compared by identity; binary32 scalars stay exact.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from audit_destruction_contracts_tu15 import Reader, sha
from audit_destruction_effect_graphs_tu15 import graph
from audit_weapon_vehicle_sound_presence_tu15 import schema, SCHEMAS, PIN
from inspect_multiplayer import Cache, ROOT
from mcc_content import MccCache, directory

PREFIXES = ('objects\\vehicles\\h_turret_ap\\', 'objects\\vehicles\\c_turret_ap\\',
            'objects\\weapons\\fixed\\', 'objects\\vehicles\\warthog\\',
            'objects\\vehicles\\scorpion\\')
SIZES = {'int8': 1, 'uint8': 1, 'enum8': 1, 'flags8': 1,
         'int16': 2, 'uint16': 2, 'enum16': 2, 'flags16': 2,
         'int32': 4, 'uint32': 4, 'enum32': 4, 'flags32': 4, 'float32': 4,
         'degree': 4, 'undefined': 4, 'rangef': 8, 'ranged': 8,
         'range': 4, 'range16': 4, 'degree2': 8, 'degree3': 12, 'point2': 8, 'point3': 12,
         'vector2': 8, 'vector3': 12, 'color32': 4}


def fields(reader, node, base, minimum=0, prefix='', omitted=None):
    result = {}
    for field in node:
        if 'offset' not in field.attrib:
            continue
        offset = int(field.attrib['offset'], 0)
        if offset < minimum:
            continue
        at, kind = base + offset, field.tag.lower()
        key = prefix + f'{offset:03x}:' + field.get('name', '')
        if kind == 'tagref':
            result[key] = reader.ref(reader.word(at + (0 if field.get('withGroup', 'true').lower() == 'false' else 4)))
        elif kind == 'stringid':
            result[key] = reader.symbol(reader.word(at))
        elif kind == 'tagblock':
            if field.get('name') == 'Predicted Resources':
                omitted.add(key)
                continue
            result[key] = [fields(reader, field, row, omitted=omitted)
                           for row in reader.cache.block(at, int(field.get('elementSize'), 0))]
        elif kind in SIZES:
            result[key] = bytes(reader.cache.data[at:at + SIZES[kind]]).hex()
        elif kind == 'colorf':
            result[key] = bytes(reader.cache.data[at:at + (16 if field.get('alpha') == 'true' else 12)]).hex()
        elif kind in ('ascii', 'utf16'):
            result[key] = bytes(reader.cache.data[at:at + int(field.get('length'), 0)]).hex()
        else:
            omitted.add(key + ':' + kind)
    return result


def differences(expected, actual, trail=''):
    if isinstance(expected, dict) and isinstance(actual, dict):
        return [row for key in sorted(expected.keys() | actual.keys())
                for row in differences(expected.get(key), actual.get(key), trail + '/' + key)]
    if isinstance(expected, list) and isinstance(actual, list) and len(expected) == len(actual):
        return [row for i, (a, b) in enumerate(zip(expected, actual))
                for row in differences(a, b, trail + f'/{i}')]
    return [] if expected == actual else [{'field': trail, 'xbox': expected, 'mcc': actual}]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--mcc-root', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--reference-scope', type=Path, default=ROOT / 'analysis/weapons-tu15-goal-20261013/authored-scope.json')
    args = ap.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        ap.error('Use tools/run_test.py')
    source = json.loads(args.reference_scope.read_text())
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    shared = MccCache(maps / 'shared.map')
    sr = Reader(shared, True)
    shared_names = {(t['group'], t['name']): t for t in shared.tags.values()}
    records, contexts, diffs, missing, omitted = {}, [], [], [], set()
    inputs = []
    try:
        for item in source['maps']:
            path = Path(item['path'])
            if sha(path) != item['sha256']:
                raise ValueError('Reference changed: ' + str(path))
            paired = maps / ('highplains.map' if path.stem == 'highplains2' else path.name)
            inputs.append({'xbox': str(path), 'xbox_sha256': item['sha256'],
                           'mcc': str(paired), 'mcc_sha256': sha(paired)})
            original, imported = Cache(path), MccCache(paired)
            try:
                xr, mr = Reader(original), Reader(imported, True)
                names = {(t['group'], t['name']): t for t in imported.tags.values()}
                roots = [t for t in original.tags.values() if t['group'] == 'weap' and t['name'].startswith(PREFIXES)]
                selected = {(t['group'], t['name']): t for t in roots}
                for weapon in roots:
                    for barrel in original.block(weapon['offset'] + 0x2d0, 0xec):
                        target = original.tags.get(original.u(barrel + 0x90))
                        if target:
                            selected[(target['group'], target['name'])] = target
                effect_roots = set()
                for identity, tag in sorted(selected.items()):
                    target = names.get(identity)
                    owner = mr
                    if target is None or target['offset'] is None:
                        target, owner = shared_names.get(identity), sr
                    if target is None or target['offset'] is None:
                        missing.append({'map': path.name, 'identity': identity})
                        continue
                    layout = schema(identity[0])
                    a = fields(xr, layout, tag['offset'], 0x12c if identity[0] == 'weap' else 0, omitted=omitted)
                    b = fields(owner, layout, target['offset'], 0x12c if identity[0] == 'weap' else 0, omitted=omitted)
                    for platform, contract in (('xbox', a), ('mcc', b)):
                        digest = hashlib.sha256(json.dumps(contract, sort_keys=True).encode()).hexdigest()
                        records.setdefault(digest, contract)
                        contexts.append({'map': path.name, 'identity': identity, 'platform': platform, 'contract': digest})
                    changes = differences(a, b)
                    if changes:
                        diffs.append({'map': path.name, 'identity': identity, 'differences': changes})
                    def effects(value):
                        if isinstance(value, dict):
                            for child in value.values():
                                effects(child)
                        elif isinstance(value, list):
                            if len(value) == 2 and value[0] in ('effe', 'jpt!'):
                                effect_roots.add(tuple(value))
                            else:
                                for child in value:
                                    effects(child)
                    effects(a)
                a, b = graph(xr, effect_roots), graph(mr, effect_roots, sr)
                for identity in sorted(a.keys() | b.keys()):
                    changes = differences(a.get(identity), b.get(identity))
                    if changes:
                        diffs.append({'map': path.name, 'effect_node': identity, 'differences': changes})
                print(json.dumps({'map': path.name, 'weapons': len(roots), 'projectiles': len(selected) - len(roots)}), flush=True)
            finally:
                original.data.close();original.stream.close();imported.close()
    finally:
        shared.close()
    report = {'schema': 'H2_TURRET_CONTRACT_AUDIT_V1', 'inventory_completed': True,
              'reference_scope_sha256': sha(args.reference_scope), 'schema_commit': PIN,
              'schema_sha256': SCHEMAS, 'contracts': records, 'contexts': contexts,
              'source_files': inputs, 'mcc_shared_sha256': sha(maps / 'shared.map'),
              'differences': diffs, 'missing': missing, 'omitted_fields': sorted(omitted),
              'gameplay_verified': False, 'full_tu15_parity': False,
              'large_artifacts_retained': [], 'tool_sha256': sha(Path(__file__))}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'completed': True, 'contexts': len(contexts), 'differences': len(diffs), 'missing': len(missing)}))
    return bool(missing)


if __name__ == '__main__':
    raise SystemExit(main())
