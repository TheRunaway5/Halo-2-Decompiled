#!/usr/bin/env python3
"""Inventory every original vehicle/model variant against prepared MCC data.

Run through run_test.py. Original files are hash-pinned offline references.
This compares declared authoring, not mode postprocessing or live acceptance.
Large banks are never copied; normalized metadata is deduplicated and sharded.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from audit_destruction_contracts_tu15 import Reader, sha
from audit_turret_contracts_tu15 import differences, fields
from audit_weapon_vehicle_sound_presence_tu15 import schema, SCHEMAS, PIN
from inspect_multiplayer import Cache, ROOT
from mcc_content import MccCache, directory


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--references', type=Path,
                        default=ROOT / 'analysis/destruction-goal-20261022/contracts-v5.json')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    reference = json.loads(args.references.read_text())
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    args.output.mkdir(parents=True, exist_ok=False)
    contracts, contexts, diff_rows, missing, omitted = {}, [], {}, [], set()
    definitions, variants, files = {}, {}, []

    def retain(contract):
        digest = hashlib.sha256(encoded(contract)).hexdigest()
        contracts.setdefault(digest, contract)
        return digest

    def compare(kind, name, map_name, original, donor):
        a, b = retain(original), retain(donor)
        contexts.append(dict(kind=kind, name=name, map=map_name, xbox=a, mcc=b))
        if a != b:
            key = kind, name, a, b
            row = diff_rows.setdefault(key, dict(kind=kind, name=name, xbox=a, mcc=b,
                                                maps=[], differences=differences(original, donor)))
            row['maps'].append(map_name)
        return a, b

    shared = MccCache(maps / 'shared.map')
    shared_reader = Reader(shared, True)
    shared_names = {(tag['group'], tag['name']): tag for tag in shared.tags.values()}
    try:
        for item in reference['source_files']:
            path = Path(item['path'])
            if sha(path) != item['sha256']:
                raise ValueError('Original reference changed: ' + str(path))
            donor_path = maps / ('highplains.map' if path.stem == 'highplains2' else path.name)
            original, donor = Cache(path), MccCache(donor_path)
            xr, mr = Reader(original), Reader(donor, True)
            names = {(tag['group'], tag['name']): tag for tag in donor.tags.values()}

            def target(identity):
                tag = names.get(identity)
                if tag and tag['offset'] is not None:
                    return mr, tag
                tag = shared_names.get(identity)
                return (shared_reader, tag) if tag and tag['offset'] is not None else (None, None)

            try:
                files.append(dict(xbox=str(path), xbox_sha256=item['sha256'],
                                  mcc=str(donor_path), mcc_sha256=sha(donor_path)))
                count = 0
                for vehicle in original.tags.values():
                    if vehicle['group'] != 'vehi' or vehicle['offset'] is None:
                        continue
                    identity = 'vehi', vehicle['name']
                    reader, imported = target(identity)
                    if not imported:
                        missing.append(dict(map=path.name, identity=identity))
                        continue
                    a = fields(xr, schema('vehi'), vehicle['offset'], omitted=omitted)
                    b = fields(reader, schema('vehi'), imported['offset'], omitted=omitted)
                    left, right = compare('vehicle', identity[1], path.name, a, b)
                    entry = definitions.setdefault(identity[1], dict(name=identity[1], contexts=[], variants=set()))
                    entry['contexts'].append(dict(map=path.name, xbox_contract=left, mcc_contract=right))
                    model = original.tags.get(xr.word(vehicle['offset'] + 0x38))
                    if not model or model['group'] != 'hlmt' or model['offset'] is None:
                        missing.append(dict(map=path.name, vehicle=identity[1], reason='Original model owner absent'))
                        continue
                    model_reader, imported_model = target(('hlmt', model['name']))
                    if not imported_model:
                        missing.append(dict(map=path.name, identity=('hlmt', model['name'])))
                        continue
                    layout = next(field for field in schema('hlmt')
                                  if field.tag.lower() == 'tagblock' and field.get('name') == 'Variants')
                    bank = original.block(model['offset'] + int(layout.get('offset'), 0),
                                          int(layout.get('elementSize'), 0))
                    imported_bank = model_reader.cache.block(imported_model['offset'] + int(layout.get('offset'), 0),
                                                            int(layout.get('elementSize'), 0))
                    imported_variants = {model_reader.symbol(model_reader.word(row)): row for row in imported_bank}
                    default = xr.symbol(xr.word(vehicle['offset'] + 0x30))
                    bank_contract = retain([fields(xr, layout, row, omitted=omitted) for row in bank])
                    default_key = identity[1], None, left, bank_contract
                    default_case = variants.setdefault(default_key, dict(vehicle=identity[1], model=model['name'],
                        variant=None, selection='object_default', object_default_variant=default,
                        xbox_vehicle_contract=left, xbox_variant_bank=bank_contract, maps=[],
                        acceptance={gate: 'open' for gate in (
                            'native_creation_and_original_eligibility', 'controls_seats_motion_and_contacts',
                            'weapons_and_damage', 'model_material_animation_and_effects',
                            'audio_lifecycle_and_output', 'damage_destruction_wreck_and_occupants',
                            'replication_continuation_and_remote_views', 'in_game_original_comparison',
                            'launcher_delivery')}, complete=False))
                    default_case['maps'].append(path.name)
                    for row in bank:
                        name = xr.symbol(xr.word(row))
                        entry['variants'].add(name)
                        other = imported_variants.get(name)
                        if other is None:
                            missing.append(dict(map=path.name, vehicle=identity[1], model=model['name'], variant=name))
                            continue
                        original_variant = fields(xr, layout, row, omitted=omitted)
                        mcc_variant = fields(model_reader, layout, other, omitted=omitted)
                        va, vb = compare('model_variant', model['name'] + ':' + name, path.name,
                                         original_variant, mcc_variant)
                        key = identity[1], name, left, va
                        variant = variants.setdefault(key, dict(vehicle=identity[1], model=model['name'],
                            variant=name, selection='named', object_default_variant=default, xbox_vehicle_contract=left,
                            xbox_variant_contract=va, maps=[], mcc_contracts=[],
                            acceptance={gate: 'open' for gate in (
                                'native_creation_and_original_eligibility', 'controls_seats_motion_and_contacts',
                                'weapons_and_damage', 'model_material_animation_and_effects',
                                'audio_lifecycle_and_output', 'damage_destruction_wreck_and_occupants',
                                'replication_continuation_and_remote_views', 'in_game_original_comparison',
                                'launcher_delivery')}, complete=False))
                        variant['maps'].append(path.name)
                        pair = dict(vehicle=right, variant=vb)
                        if pair not in variant['mcc_contracts']:
                            variant['mcc_contracts'].append(pair)
                    count += 1
                print(json.dumps(dict(map=path.name, vehicles=count)), flush=True)
            finally:
                original.data.close();original.stream.close();donor.close()
    finally:
        shared.close()
    shards, shard, size = [], {}, 0

    def flush():
        nonlocal shard, size
        if not shard:
            return
        path = args.output / f'contracts-{len(shards):02d}.json'
        path.write_bytes(encoded(shard) + b'\n')
        shards.append(dict(path=path.name, bytes=path.stat().st_size, sha256=sha(path)))
        shard, size = {}, 0

    for digest, contract in sorted(contracts.items()):
        amount = len(encoded({digest: contract}))
        if amount > 512 * 1024:
            raise ValueError('One declared contract exceeds the bounded report budget')
        if size + amount > 512 * 1024:
            flush()
        shard[digest] = contract;size += amount
    flush()
    for entry in definitions.values():
        entry['variants'] = sorted(entry['variants'])
    variant_shards, rows = [], []
    def flush_variants():
        nonlocal rows
        if not rows:
            return
        path = args.output / f'variants-{len(variant_shards):02d}.json'
        path.write_bytes(encoded(rows) + b'\n')
        variant_shards.append(dict(path=path.name, bytes=path.stat().st_size, sha256=sha(path), rows=len(rows)))
        rows = []
    for variant in variants.values():
        if rows and len(encoded(rows + [variant])) > 512 * 1024:
            flush_variants()
        rows.append(variant)
    flush_variants()
    report = dict(schema='H2_ALL_VEHICLE_VARIANTS_TU15_V2', inventory_completed=not missing,
                  full_tu15_parity=False, runtime_verified=False, source_files=files,
                  original_scope_sha256=sha(args.references), schema_commit=PIN, schema_sha256=SCHEMAS,
                  shared_sha256=sha(maps / 'shared.map'), definitions=list(definitions.values()),
                  variant_shards=variant_shards, variant_authoring_contexts=len(variants),
                  named_variant_contexts=sum(row['selection'] == 'named' for row in variants.values()),
                  default_constructor_contexts=sum(row['selection'] == 'object_default' for row in variants.values()),
                  contexts=contexts, differences=list(diff_rows.values()),
                  contract_shards=shards, missing=missing, omitted_fields=sorted(omitted),
                  limits=['Declared metadata only; native mode postprocessing remains a separate owner',
                          'Model variants are inventoried without inventing driver/destructibility eligibility',
                          'Original empty/default constructor selection requires live qualification',
                          'Particle/sound/material payload and runtime gates remain separate'],
                  tool_sha256=sha(Path(__file__)), large_artifacts_retained=[])
    path = args.output / 'inventory.json'
    path.write_bytes(encoded(report) + b'\n')
    if path.stat().st_size > 1024 * 1024:
        raise ValueError('Inventory index exceeds its bounded report budget')
    print(json.dumps(dict(inventory_completed=not missing, definitions=len(definitions),
                          variant_authoring_contexts=len(variants), missing=len(missing),
                          report=str(path))))
    return int(bool(missing))


if __name__ == '__main__':
    raise SystemExit(main())
