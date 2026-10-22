#!/usr/bin/env python3
"""Separate direct scenario/netgame placements from resident damageable tags.

Run with tools/run_test.py. This is authored metadata, not creation/admission
or gameplay evidence. AI, scripts, model children and debris are other producers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from inspect_multiplayer import Cache
from mcc_content import MccCache

BLOCKS = (('scen', 0x50, 0x58, 0x5c), ('bipd', 0x60, 0x68, 0x54),
          ('vehi', 0x70, 0x78, 0x54), ('eqip', 0x80, 0x88, 0x38),
          ('weap', 0x90, 0x98, 0x54), ('mach', 0xa8, 0xb0, 0x48),
          ('ctrl', 0xb8, 0xc0, 0x44), ('bloc', 0x328, 0x330, 0x4c),
          ('crea', 0x368, 0x370, 0x34))


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def inventory(cache, mcc=False):
    word = cache.u32 if mcc else cache.u
    scenario = cache.tags[word(cache.index + 12)]['offset'] if mcc else cache.scenario
    rows = {}
    def tag(value):
        if value == 0xffffffff:
            return None
        definition = cache.tags[value]
        return definition['group'] + ':' + definition['name']
    for group, placed, palette, stride in BLOCKS:
        choices = [word(row + 4) for row in cache.block(scenario + palette, 0x28)]
        for index, address in enumerate(cache.block(scenario + placed, stride)):
            choice = struct.unpack_from('<h', cache.data, address)[0]
            if choice == -1:
                continue
            if choice < 0 or choice >= len(choices):
                raise ValueError('Scenario placement escapes its owning palette')
            identity = tag(choices[choice])
            if identity is None:
                raise ValueError('Scenario placement selects a NONE definition')
            entry = rows.setdefault(identity, {'direct_group': group, 'scenario_rows': [], 'flags': {}, 'netgame_rows': []})
            entry['scenario_rows'].append(index)
            flags = f'{word(address + 4):08x}'
            entry['flags'][flags] = entry['flags'].get(flags, 0) + 1
    unresolved = []
    for index, address in enumerate(cache.block(scenario + 0x120, 0x90)):
        datum = word(address + 0x5c)
        collection = cache.tags.get(datum)
        if not collection or collection['offset'] is None:
            unresolved.append({'row': index, 'collection': tag(datum) if collection else None})
            continue
        if collection['group'] not in ('itmc', 'vehc'):
            raise ValueError('Netgame row has a non-collection definition')
        for member in cache.block(collection['offset'], 0x10):
            identity = tag(word(member + 8))
            if identity is None:
                continue
            entry = rows.setdefault(identity, {'direct_group': None, 'scenario_rows': [], 'flags': {}, 'netgame_rows': []})
            entry['netgame_rows'].append({'row': index, 'classification': cache.data[address + 0x14],
                'modes': list(struct.unpack_from('<4h', cache.data, address + 4)),
                'weight_word': f'{word(member):08x}', 'collection': collection['name']})
    return {'objects': rows, 'unresolved_collection_backing': unresolved}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census['schema'] != 'H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require a completed object-contract census')
    maps = []
    for file in census['source_files']:
        original_path = Path(file['path'])
        if sha(original_path) != file['sha256']:
            raise ValueError('Original map changed since its contract census')
        imported_path = Path(file['mcc_path']) if file['mcc_path'] else None
        if imported_path and sha(imported_path) != file['mcc_sha256']:
            raise ValueError('MCC map changed since its contract census')
        original = Cache(original_path);imported = MccCache(imported_path) if imported_path else None
        try:
            maps.append({'path': str(original_path), 'map': original_path.stem,
                         'mcc_path': str(imported_path) if imported_path else None,
                         'xbox': inventory(original), 'mcc': inventory(imported, True) if imported else None})
        finally:
            original.data.close();original.stream.close()
            if imported:
                imported.close()
    definition_contexts = {}
    for definition in census['definitions']:
        contexts = [row['map'] for row in maps if definition in row['xbox']['objects']]
        definition_contexts[definition] = sorted(set(contexts))
    report = {'scope': __doc__, 'runtime_verified': False, 'other_producers_remain': True,
              'contract_census_sha256': sha(args.contracts), 'tool_sha256': sha(__file__),
              'maps': maps, 'damage_contract_direct_or_netgame_contexts': definition_contexts,
              'summary': {'physical_map_inputs': len(maps), 'damageable_contract_definitions': len(definition_contexts),
                  'definitions_with_direct_or_netgame_context': sum(bool(value) for value in definition_contexts.values())}}
    data = json.dumps(report, separators=(',', ':')) + '\n'
    if len(data.encode()) > 1024 * 1024:
        raise ValueError('Placement inventory exceeds the bounded report budget')
    args.report.parent.mkdir(parents=True, exist_ok=True);args.report.write_text(data)
    print(json.dumps(report['summary'] | {'report': str(args.report), 'bytes': len(data.encode())}))


if __name__ == '__main__':
    main()
