#!/usr/bin/env python3
"""Compare authored destruction response damage/effect edges, read-only.

Uses the completed model census for exact map identities. Effect events/parts,
markers and the first58 bytes of damage definitions are normalized. Particle,
sound, light, spawned-object and other terminal owners remain separate; this
is not execution or complete presentation/material-response certification.
Run through tools/run_test.py; retain bounded deduplicated JSON receipts only.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from audit_destruction_contracts_tu15 import Reader, inventory, sha
from inspect_multiplayer import Cache
from mcc_content import MccCache


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':')).encode() + b'\n'


def response_roots(contract):
    result = set()
    for info in contract['damage']:
        for value in info['effects_and_shaders'].values():
            if value and value[0] in ('effe', 'jpt!'):
                result.add(tuple(value))
        for section in info['sections']:
            for response in section['responses']:
                for field in ('effect', 'delay_effect', 'damage'):
                    value = response[field]
                    if value:
                        result.add(tuple(value))
    return sorted(result)


def graph(reader, roots, shared=None):
    readers = [reader] + ([shared] if shared else [])
    by_name = [{(tag['group'], tag['name']): tag for tag in r.cache.tags.values()}
               for r in readers]
    pending = list(roots)
    nodes = {}
    while pending:
        identity = tuple(pending.pop())
        key = ':'.join(identity)
        if key in nodes:
            continue
        match = next(((r, names[identity]) for r, names in zip(readers, by_name)
                      if identity in names and names[identity]['offset'] is not None), None)
        if match is None:
            nodes[key] = {'resident': False}
            continue
        r, tag = match
        root = tag['offset']
        node = {'resident': True}
        nodes[key] = node
        if identity[0] == 'jpt!':
            node['words_00_4c'] = r.words(root, range(0, 0x50, 4))
            node['damage_group'] = r.symbol(r.word(root + 0x50))
            node['damage_group_b'] = r.symbol(r.word(root + 0x54))
        elif identity[0] == 'effe':
            node['flags'] = r.words(root, (0,))
            node['locations'] = [r.symbol(r.word(row)) for row in r.cache.block(root + 0xc, 4)]
            node['events'] = []
            for event in r.cache.block(root + 0x14, 0x38):
                value = {'words': r.words(event, range(0, 0x18, 4)), 'parts': [],
                         'beam_count': r.word(event + 0x20),
                         'acceleration_count': r.word(event + 0x28),
                         'particle_system_count': r.word(event + 0x30)}
                for part in r.cache.block(event + 0x18, 0x38):
                    target = r.ref(r.word(part + 0x10))
                    value['parts'].append({'words': r.words(part, (0, 4, 8, *range(0x14, 0x38, 4))),
                                           'target': target})
                    if target and target[0] in ('effe', 'jpt!'):
                        pending.append(target)
                node['events'].append(value)
        else:
            raise ValueError(f'Unexpected destruction response root: {identity}')
    return nodes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census.get('schema') != 'H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require the completed model-contract census')
    maps = Path(census['source_files'][0]['mcc_path']).parent
    if sha(maps / 'shared.map') != census['mcc_shared_sha256']:
        raise ValueError('MCC shared map changed since the census')
    shared = MccCache(maps / 'shared.map')
    shared_reader = Reader(shared, True)
    shared_models = {(g, n): contract for g, n, contract in inventory(shared_reader)}
    records, contexts, differences = {}, [], []
    try:
        for file in census['source_files']:
            if sha(file['path']) != file['sha256'] or sha(file['mcc_path']) != file['mcc_sha256']:
                raise ValueError('Map changed since the model census')
            original, imported = Cache(Path(file['path'])), MccCache(Path(file['mcc_path']))
            try:
                expected, actual = Reader(original), Reader(imported, True)
                expected_models = inventory(expected)
                actual_models = {(g, n): contract for g, n, contract in inventory(actual)}
                roots = {
                    'xbox': sorted({root for _, _, contract in expected_models for root in response_roots(contract)}),
                    'mcc': sorted({root for g, n, _ in expected_models
                                   for root in response_roots(actual_models.get((g, n),
                                       shared_models.get((g, n), {'damage': []})))})}
                row = {'map': Path(file['path']).stem, 'source_path': file['path'], 'roots': roots}
                graphs = {'xbox': graph(expected, roots['xbox']),
                          'mcc': graph(actual, roots['mcc'], shared_reader)}
                for platform, nodes in graphs.items():
                    row[platform] = {}
                    for key, node in sorted(nodes.items()):
                        digest = hashlib.sha256(encoded(node)).hexdigest()
                        records.setdefault(digest, node)
                        row[platform][key] = digest
                for key in sorted(row['xbox'].keys() | row['mcc'].keys()):
                    if row['xbox'].get(key) != row['mcc'].get(key):
                        differences.append({'map': row['map'], 'node': key,
                            'xbox': row['xbox'].get(key), 'mcc': row['mcc'].get(key)})
                contexts.append(row)
            finally:
                original.data.close(); original.stream.close(); imported.close()
    finally:
        shared.close()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    shards, shard = [], {}
    def flush():
        nonlocal shard
        if not shard:
            return
        path = args.report.with_name(args.report.stem + f'-nodes-{len(shards):02d}.json')
        data = encoded(shard)
        path.write_bytes(data)
        shards.append({'path': path.name, 'bytes': len(data), 'sha256': sha(path)})
        shard = {}
    for digest, value in sorted(records.items()):
        if len(encoded({digest: value})) > 512 * 1024:
            raise ValueError('One graph node exceeds the bounded receipt budget')
        if len(encoded(shard | {digest: value})) > 512 * 1024:
            flush()
        shard[digest] = value
    flush()
    context_shards, batch = [], []
    def flush_contexts():
        nonlocal batch
        if not batch:
            return
        path = args.report.with_name(args.report.stem + f'-contexts-{len(context_shards):02d}.json')
        data = encoded(batch)
        path.write_bytes(data)
        context_shards.append({'path': path.name, 'bytes': len(data), 'sha256': sha(path)})
        batch = []
    for row in contexts:
        if len(encoded([row])) > 512 * 1024:
            raise ValueError('One graph context exceeds the bounded receipt budget')
        if len(encoded([*batch, row])) > 512 * 1024:
            flush_contexts()
        batch.append(row)
    flush_contexts()
    report = {'scope': __doc__, 'runtime_verified': False, 'complete_effect_graph': False,
        'contract_census_sha256': sha(args.contracts), 'tool_sha256': sha(__file__),
        'context_shards': context_shards, 'node_shards': shards, 'differences': differences,
        'summary': {'map_inputs': len(contexts), 'distinct_nodes': len(records),
            'difference_rows': len(differences), 'unresolved_contexts': sum(
                not records[digest]['resident'] for row in contexts for platform in ('xbox', 'mcc')
                for digest in row[platform].values())}}
    data = encoded(report)
    if len(data) > 1024 * 1024 or len(data) + sum(row['bytes'] for row in [*shards, *context_shards]) > 8 * 1024 * 1024:
        raise ValueError('Graph receipts exceed the small-report retention budget')
    args.report.write_bytes(data)
    print(json.dumps(report['summary'] | {'report': str(args.report)}))


if __name__ == '__main__':
    main()
