#!/usr/bin/env python3
"""Inventory Xbox/MCC destruction contracts; no runtime result is inferred.

Run with tools/run_test.py. Xbox maps are read-only offline references; MCC v13
maps supply the runtime-side metadata. Reports contain deduplicated contracts,
source hashes and exact scalar words, never raw cache banks or captures.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from inspect_multiplayer import Cache
from mcc_content import MccCache, directory
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GROUPS = {'bipd', 'vehi', 'weap', 'eqip', 'proj', 'scen', 'mach', 'ctrl', 'bloc', 'crea', 'garb', 'ssce'}
ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


class Reader:
    def __init__(self, cache, mcc=False):
        self.cache, self.mcc = cache, mcc
        self.word = cache.u32 if mcc else cache.u

    def ref(self, value):
        if value == 0xffffffff:
            return None
        tag = self.cache.tags[value]
        return [tag['group'], tag['name']]

    def symbol(self, value):
        if value == 0:
            return ''
        index = value & 65535
        count, table, strings = (48, 60, 52) if self.mcc else (356, 364, 368)
        if index >= self.word(count):
            raise ValueError('Destruction StringID exceeds its source bank')
        start = self.word(strings) + self.word(self.word(table) + index * 4)
        end = self.cache.data.find(b'\0', start)
        if end < start:
            raise ValueError('Unterminated destruction StringID')
        return self.cache.data[start:end].decode('utf-8')

    def half(self, at):
        return struct.unpack_from('<h', self.cache.data, at)[0]

    def words(self, at, offsets):
        return {f'{offset:02x}': f'{self.word(at + offset):08x}' for offset in offsets}

    def model(self, tag):
        at = tag['offset']
        result = {'model': tag['name'], 'collision': self.ref(self.word(at + 0xc)),
                  'physics': self.ref(self.word(at + 0x24)), 'damage': [], 'materials': []}
        for material in self.cache.block(at + 0x58, 0x14):
            result['materials'].append({'name': self.symbol(self.word(material)),
                'global_material': self.symbol(self.word(material + 12)),
                'words': self.words(material, (4, 8, 16))})
        for info in self.cache.block(at + 0x60, 0xf8):
            value = {'words': self.words(info, (0, 8, 0x28, 0x2c, 0x30, 0x34,
                     0x38, 0x8c, 0x94, 0x98, 0x9c, 0xa0, 0xcc, 0xd0, 0xd4)),
                     'indirect_material': self.symbol(self.word(info + 4)),
                     'shield_material': self.symbol(self.word(info + 0x90)),
                     'effects_and_shaders': {f'{offset:02x}': self.ref(self.word(info + offset))
                         for offset in (0x80, 0x88, 0xa8, 0xb0, 0xb8, 0xec, 0xf4)},
                     'nodes': [self.words(node, (0, 4, 8, 12))
                         for node in self.cache.block(info + 0xc4, 0x10)],
                     'seats': [{'name': self.symbol(self.word(seat)),
                               'words': self.words(seat, (4, 8, 12, 16))}
                         for seat in self.cache.block(info + 0xd8, 0x14)],
                     'sections': [], 'constraints': []}
            for section in self.cache.block(info + 0xbc, 0x38):
                row = {'name': self.symbol(self.word(section)),
                       'words': self.words(section, (4, 8, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x34)),
                       'restored_region': self.symbol(self.word(section + 0x30)),
                       'responses': []}
                for response in self.cache.block(section + 12, 0x50):
                    row['responses'].append({
                        'words': self.words(response, (0, 4, 8, 0x20, 0x2c, 0x30, 0x44, 0x4c)),
                        'region': self.symbol(self.word(response + 0x1c)),
                        'delay_marker': self.symbol(self.word(response + 0x38)),
                        'constraint_group': self.symbol(self.word(response + 0x3c)),
                        'ejecting_seat': self.symbol(self.word(response + 0x40)),
                        'destroyed_child_marker': self.symbol(self.word(response + 0x48)),
                        'effect': self.ref(self.word(response + 0x10)),
                        'damage': self.ref(self.word(response + 0x18)),
                        'delay_effect': self.ref(self.word(response + 0x34)),
                        'effect_marker': self.symbol(self.word(response + 0x24)),
                        'damage_marker': self.symbol(self.word(response + 0x28))})
                value['sections'].append(row)
            for constraint in self.cache.block(info + 0xe0, 0x14):
                value['constraints'].append({'name': self.symbol(self.word(constraint)),
                    'damage_constraint': self.symbol(self.word(constraint + 4)),
                    'group': self.symbol(self.word(constraint + 8)),
                    'words': self.words(constraint, (12, 16))})
            result['damage'].append(value)
        return result


def inventory(reader):
    cache = reader.cache
    models, objects = {}, []
    for tag in cache.tags.values():
        if tag['group'] not in GROUPS or tag['offset'] is None:
            continue
        model = cache.tags.get(reader.word(tag['offset'] + 0x38))
        if not model or model['offset'] is None:
            continue
        if model['datum'] not in models:
            models[model['datum']] = reader.model(model)
        contract = models[model['datum']]
        if contract['damage'] or tag['group'] == 'vehi':
            objects.append((tag['group'], tag['name'], contract))
    return objects


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if sha(NEW_DEFAULT) != NEW_PIN:
        raise ValueError('Original executable differs from pinned TU1.5')
    maps = directory(args.mcc_root) / 'h2_maps_win64_dx11'
    paths = sorted((ROOT / 'assets/disc/maps').glob('*.map'))
    paths += sorted((ROOT / 'assets/multiplayer-map-pack/demos/dlc').rglob('*.map'))
    dlc = Path.home() / '.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/c'
    paths += sorted(p for p in dlc.rglob('*.map') if not p.parent.name.endswith('old'))
    # Duplicate files remain source inputs; their contracts are deduplicated.
    paths = sorted(set(paths))
    contracts, definitions, files, differences, missing = {}, {}, [], [], []

    def retain(platform, name, group, contract, map_name):
        encoded = json.dumps(contract, sort_keys=True, separators=(',', ':')).encode()
        digest = hashlib.sha256(encoded).hexdigest()
        contracts.setdefault(digest, contract)
        row = definitions.setdefault(group + ':' + name, {'xbox': {}, 'mcc': {}})[platform]
        row.setdefault(digest, [])
        if map_name not in row[digest]:
            row[digest].append(map_name)
        return digest

    shared = MccCache(maps / 'shared.map')
    shared_rows = {(g, n): c for g, n, c in inventory(Reader(shared, True))}
    try:
        for path in paths:
            if path.stem in ('mainmenu', 'shared', 'single_player_shared'):
                continue
            original = Cache(path)
            # The existing PC tag-bank census maps Xbox's revised Tombstone
            # filename highplains2 to MCC's highplains. Compare raw contracts;
            # the alias itself never asserts revision/gameplay equivalence.
            mcc_path = maps / ('highplains.map' if path.stem == 'highplains2' else path.name)
            imported = MccCache(mcc_path) if mcc_path.is_file() else None
            try:
                files.append({'path': str(path), 'sha256': sha(path),
                    'mcc_path': str(imported.path) if imported else None,
                    'mcc_sha256': sha(imported.path) if imported else None})
                actual = {(g, n): c for g, n, c in inventory(Reader(imported, True))} if imported else {}
                for group, name, contract in inventory(Reader(original)):
                    a = retain('xbox', name, group, contract, path.stem)
                    other = actual.get((group, name), shared_rows.get((group, name)))
                    if other is None:
                        missing.append({'map': path.stem, 'group': group, 'name': name,
                                        'mcc_map_available': imported is not None})
                        continue
                    b = retain('mcc', name, group, other, path.stem)
                    if a != b:
                        differences.append({'map': path.stem, 'group': group, 'name': name,
                                            'xbox': a, 'mcc': b})
            finally:
                original.data.close()
                original.stream.close()
                if imported:
                    imported.close()
    finally:
        shared.close()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    # Keep individual receipts below the retention limit without dropping any
    # authored response. The index authenticates each bounded contract shard.
    shards, shard, shard_bytes = [], {}, 0
    def flush():
        nonlocal shard, shard_bytes
        if not shard:
            return
        path = args.report.with_name(args.report.stem + f'-contracts-{len(shards):02d}.json')
        data = json.dumps(shard, separators=(',', ':')) + '\n'
        path.write_text(data)
        shards.append({'path': path.name, 'bytes': len(data.encode()), 'sha256': sha(path)})
        shard, shard_bytes = {}, 0
    for digest, contract in sorted(contracts.items()):
        size = len(json.dumps({digest: contract}, separators=(',', ':')).encode())
        if size > 512 * 1024:
            raise ValueError('One destruction contract exceeds its report budget')
        if shard_bytes + size > 512 * 1024:
            flush()
        shard[digest] = contract
        shard_bytes += size
    flush()
    report = {'schema': 'H2_DESTRUCTION_CONTRACTS_V1', 'inventory_completed': True,
        'runtime_verified': False, 'full_goal_complete': False,
        'scope': 'Object-model damage contracts only; BSP breakables, effect graphs and live behavior remain separate',
        'tu15_sha256': NEW_PIN, 'tool_sha256': sha(__file__),
        'mcc_shared_sha256': sha(maps / 'shared.map'), 'source_files': files,
        'definitions': definitions, 'contract_shards': shards, 'differences': differences,
        'missing': missing, 'summary': {'source_maps': len(files), 'definitions': len(definitions),
            'contracts': len(contracts), 'difference_rows': len(differences), 'missing_rows': len(missing)}}
    data = json.dumps(report, separators=(',', ':')) + '\n'
    if len(data.encode()) > 1024 * 1024 or len(data.encode()) + sum(s['bytes'] for s in shards) > 8 * 1024 * 1024:
        raise ValueError('Destruction inventory exceeds small-report retention budget')
    args.report.write_text(data)
    print(json.dumps(report['summary'] | {'report': str(args.report), 'bytes': len(data.encode())}))


if __name__ == '__main__':
    main()
