#!/usr/bin/env python3
"""Compare native unsorted registration/removal with literal original full pools."""
import argparse
import json
from pathlib import Path
import subprocess
from differential_item_transform_math import ROOT, authority, digest, require
from differential_item_node_bounds import raw_fields


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/spatial_registry_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/spatial-registry-differential.json')
    args = parser.parse_args()
    report = authority()
    calls = [c for c in report['calls'] if c['entry'] in {'001ce650', '001ce830'}]
    require(len(calls) == 4, 'Incomplete original registry core roster')
    clusters = {c['id']: [leaf for leaf in report['calls'] if leaf['entry'] == '0014bd00'
                and c['order'] < leaf['order'] < leaf['return_order'] < c['return_order']] for c in calls}
    selected = sorted({c[k] for core in calls for c in [core, *clusters[core['id']]]
                       for k in ('before_state', 'after_state')})
    rows = []
    aliases = chain_views = chain_records = 0
    for sid in selected:
        state = report['states'][sid]
        for obj in state['current']['current']['objects']:
            duplicate = next(o for o in state['current']['world']['objects'] if o['datum'] == obj['datum'])
            require(obj['body_hex'] == duplicate['raw_hex'], 'Original full body/node alias differs')
            rows.append(['B', f'{sid:x}', obj['datum'], obj['body_hex']])
        # Chain rows are decoded views of complete pools, not independently
        # allocated buffers. Derive each recorded view, including empty chains
        # and unsorted order, before comparing its canonical physical storage.
        for chain in state['current']['current']['registry_chains']:
            for kind, address in [('collidable', '004e58b0'),
                                  ('noncollidable', '004e58a4')]:
                registry = next(r for r in state['registries'] if r['global_address'] == address)
                heads = bytes.fromhex(registry['heads800_hex'])
                pool = registry['forward']
                data = bytes.fromhex(pool['data_hex'])
                stride = pool['stride']
                link = int.from_bytes(heads[chain['cluster'] * 4:chain['cluster'] * 4 + 4], 'little')
                decoded = []
                seen = set()
                while link != 0xffffffff:
                    require(link not in seen, 'Original decoded chain is cyclic')
                    seen.add(link)
                    index = link & 0xffff
                    require(index < pool['capacity'], 'Original decoded chain lies outside complete pool')
                    raw = data[index * stride:(index + 1) * stride]
                    require(int.from_bytes(raw[:2], 'little') == link >> 16,
                            'Original decoded chain salt differs from full pool')
                    successor = int.from_bytes(raw[8:12], 'little')
                    decoded.append(dict(link=f'{link:08x}', next=f'{successor:08x}',
                                        object=f"{int.from_bytes(raw[4:8], 'little'):08x}",
                                        raw_hex=raw.hex()))
                    link = successor
                require(decoded == chain[kind], 'Original decoded chain differs from complete physical storage')
                chain_views += 1
                chain_records += len(decoded)
        for path, raw in raw_fields(state):
            if not path.startswith('current/current/registry_chains/'):
                rows.append(['E', f'{sid:x}', path, raw])
        # The older current-state capture repeats physical head/header buffers
        # that the complete registry capture also owns. Prove both the original
        # pointers and full bytes before exporting their shared-storage mapping.
        for index, registry in enumerate(state['registries']):
            address = registry['global_address']
            duplicate = state['current']['current']['cluster_registries'][address]
            require(duplicate['head'] == registry['heads_pointer'] and
                    duplicate['head_hex'] == registry['heads800_hex'],
                    'Original duplicate registry head does not alias full storage')
            prefix = f'current/current/cluster_registries/{address}/'
            canonical = f'registries/{index}/'
            rows.append(['A', f'{sid:x}', prefix + 'head_hex', canonical + 'heads800_hex'])
            aliases += 1
            require(len(duplicate['pools']) == 2,
                    'Original duplicate registry pool roster differs')
            for pool_index, kind in enumerate(('forward', 'reverse')):
                pool = registry[kind]
                old = duplicate['pools'][pool_index]
                require(duplicate[kind] == pool['pointer'] == old['pointer'] and
                        old['header_hex'] == pool['header4c_hex'],
                        'Original duplicate registry header does not alias full storage')
                rows.append(['A', f'{sid:x}', prefix + f'pools/{pool_index}/header_hex',
                             canonical + kind + '/header4c_hex'])
                aliases += 1
    values = {r['raw_hex'] for r in report['reads'] if r['address'] == '00465048' and r['size'] == 4}
    require(len(values) == 1, 'Missing original zero operand')
    zero = f"{int.from_bytes(bytes.fromhex(next(iter(values))), 'little'):08x}"
    writes = leaves = 0
    for call in calls:
        state = report['states'][call['before_state']]
        packets = {p['field']: p for p in call['packets']}
        after_packets = {p['field']: p for p in call['after_packets']}
        ptr = packets['s0_registry_triplet' if call['entry'] == '001ce650' else 'edi_registry_triplet']['pointer']
        index = next(i for i, registry in enumerate(state['registries']) if registry['global_address'] == ptr)
        registry = state['registries'][index]
        body = int(call['scope']['body'], 16)
        require(int(packets['s2_reverse_head' if call['entry'] == '001ce650' else 's1_reverse_head']['pointer'], 16) == body + 0x60,
                'Original reverse head does not alias full object body')
        radius = call['stack_words'][4] if call['entry'] == '001ce650' else '00000000'
        rows.append(['C', f"scope{call['scope']['id']}_call{call['id']}", call['entry'], call['scope']['datum'],
                     f'{index:x}', f"{call['before_state']:x}", f"{call['after_state']:x}", radius, zero, 'literal_source'])
        cs = clusters[call['id']]
        if call['entry'] == '001ce650':
            require(len(cs) == 1 and call['entry_cpu']['registers']['eax'] == '00000000',
                    'Unmeasured original explicit-mask/cluster branch')
            require(int(packets['s3_center12']['pointer'], 16) == body + 0x40 and
                    int(packets['s5_location8']['pointer'], 16) == body + 0x28,
                    'Original center/location alias differs')
            for key, name in [('s3_center12', 'center'), ('s5_location8', 'location'),
                              ('s7_element', 'payload'), ('s8_overflow_output', 'overflow')]:
                rows.append(['P', name, packets[key]['raw_hex'], after_packets[key]['raw_hex']])
            leaf = cs[0]
            before = {p['field']: p for p in leaf['packets']}
            after = {p['field']: p for p in leaf['after_packets']}
            rows.append(['K', f"{leaf['before_state']:x}", f"{leaf['after_state']:x}",
                         leaf['stack_words'][0][-4:], leaf['stack_words'][3][-4:],
                         leaf['stack_words'][2], leaf['return_cpu']['registers']['eax'],
                         before['s1_center12']['raw_hex'], before['s4_clusters_output']['raw_hex'],
                         after['s4_clusters_output']['raw_hex']])
            leaves += 1
        else:
            require(not cs, 'Unexpected cluster walk during removal')
        # SourceSpatialTarget's public enumeration maps to these original
        # physical buffers. Pool pointer/shape bytes are preserved verbatim.
        ranges = [(0, int(registry['heads_pointer'], 16), bytes.fromhex(registry['heads800_hex']))]
        for kind, first in [('forward', 1), ('reverse', 4)]:
            pool = registry[kind]
            for offset, address, raw in [(0, 'pointer', 'header4c_hex'), (1, 'data_pointer', 'data_hex'), (2, 'bitmap_pointer', 'bitmap_hex')]:
                ranges.append((first + offset, int(pool[address], 16), bytes.fromhex(pool[raw])))
        ranges.append((7, body + 0x60, bytes(4)))
        ranges.append((8, int(state['source_cluster_visit_globals']['address'], 16), bytes(8)))
        if call['entry'] == '001ce650': ranges.append((9, int(packets['s8_overflow_output']['pointer'], 16), bytes(1)))
        for store in report['stores']:
            if not call['order'] < store['order'] < call['return_order'] or any(
                    leaf['order'] < store['order'] < leaf['return_order'] for leaf in cs):
                continue  # Actual portal producer is an explicit complete-state leaf.
            address = int(store['address'], 16)
            owned = [(target, address - start) for target, start, raw in ranges
                     if start <= address and address + store['size'] <= start + len(raw)]
            require(len(owned) <= 1, 'Overlapping original registry store targets')
            if owned:
                target, offset = owned[0]
                rows.append(['W', f'{target:x}', store['instruction'], f'{offset:x}', store['before_hex'], store['written_hex']])
                writes += 1
    fixture = ROOT / 'analysis/multiplayer/integration/spatial-registry-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    result = dict(passed=run.returncode == 0, scope=__doc__, core_calls=4,
                  complete_states=len(selected), ordered_native_stores=writes,
                  explicit_original_cluster_walks=leaves,
                  proved_physical_buffer_aliases=aliases,
                  proved_derived_chain_views=chain_views,
                  proved_derived_chain_records=chain_records,
                  compared='Complete occupied bodies, header/registry/pool/bitmap/world/player/context/cache/fallback/lazy/visit buffers; allocator salts/freed bytes, unsorted links/order, packets and overflow.',
                  limits='Source first-item ordinary registration/removal. Portal traversal14BD00 retains original full output leaf. Explicit mask, absent location/radius, allocation exhaustion and positive debug-fill have no measured comparison. Full source world/frame/physics remains open.',
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
