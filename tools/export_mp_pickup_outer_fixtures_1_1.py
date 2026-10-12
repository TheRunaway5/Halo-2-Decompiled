#!/usr/bin/env python3
"""Export pinned original outer/locator observations; never execute native code.

All expected runtime values are copied or byte-decoded from frozen original
instruction reports. Cache records are source inputs, not synthesized contacts.
The two active-physics casts remain conditional and separately identified.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source'
DEFAULT_OUT = BASE / 'pickup-outer-fixtures'
FROZEN = {
    'grenades.json': 'a1b3d10d9fe3b446007beae2ed97f324ce84622fac5a318fd934e1a2740b0122',
    'netgame/report.json': '3183939bad6d9abe3f13a5c64ececde234c5c73db12cdeb7199ebbde2318356f',
    'world/report.json': 'f7b7d075ce0c2988283b013bc1e148c102afd2b67d50de9996c5dc016e644437',
    'target/report.json': '7bd7eacc38f6e64295540c5616e4bd8638426c0a05483db3837fd9934cab13b6',
    'controls/report.json': 'a6d9af4e99823eb9d014e083e35bbc340a575bea91d0f3411665e59c6b2cf0e9',
    'spatial/report.json': '6b03f62ec351afeb0760a9052305a08a1cbc3d15a3175f80edfd672c76320b59',
    'equipped-pickup/report.json': 'd6cec76de0a155f2c5817b61d8073d4555ddb70efa48e731b349fd8ff742c00b',
    'weapon-attachment/report.json': '657104018ea629e0f58c26f87ef08e9778d75d4c0657419515b695bdfeffc67f',
    'pickup-visibility/report.json': 'bcdf9055fcaafaf68f71cc5732c47da9364a8526793b6238e0c3d4593d37397b',
    'pickup-retreat/report.json': '3b19c3140ea0681a9f1be85580c41d6dd0e28c49583977eb2f58aa6001300713',
}
REPRO_PINS = {
    'pickup-visibility': 'cb94edb9b8794a41128edd980069c9b2ace33c6c5bfff05ed6762670b326d41e',
    'pickup-retreat': 'cefba40545e336890cbfa5536a140d28a65f616d644bcfc1e48118374c42201c',
}
FIELDS = {
    'I': 'bsp_index bsp_magic metadata_magic root_address root_file_offset collision_address collision_file_offset collision_count material_count leaf_cluster_count',
    'B': 'name descriptor_offset stride count address file_offset raw_sha256',
    'M': 'material_index global_material_i16',
    'J': 'leaf_index cluster_i16',
    'Q': 'id suite source_query_index ordinary_mask_scope caller mask ignored0 ignored1 start_x start_y start_z delta_x delta_y delta_z return_al type fraction point_x point_y point_z initial_location8 final_location8 defined_byte_mask outer_result5c',
    'K': 'id flags bsp root range_min range_max initial_best breakable_count breakable_pointer hit prior_before_leaf prior_before_class prior_before_plane prior_after_leaf prior_after_class prior_after_plane visited_count lower_hit18 descriptor_before28 descriptor_after28 visited_leaves...',
    'S': 'id sequence source_stream source_snapshot_index instruction iteration point_defined fraction point_x point_y point_z initial_location8 final_location8 normal_x normal_y normal_z plane_distance dot_or_step_stack xmm0 xmm3 xmm4',
    'R': 'id iteration pre_snapshot fraction_written_snapshot point_formed_snapshot located_snapshot step fraction_before fraction_after point_before_x point_before_y point_before_z point_after_x point_after_y point_after_z location_before8 location_after8',
    'C': 'id suite source_call_index caller root bsp point_x point_y point_z return_leaf',
    'L': 'id suite source_call_index caller active_bsp point_x point_y point_z return_eax location8',
    'P': 'id suite source_call_index caller collision_bsp signed_plane output_plane4f',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def json_bytes(value):
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def raw_u(raw, offset=0):
    return struct.unpack_from('<I', raw, offset)[0]


def raw_i(raw, offset=0):
    return struct.unpack_from('<i', raw, offset)[0]


def record(raw, address, magic):
    offset = address - magic
    require(0 <= offset <= len(raw), 'Cache address is out of range')
    return offset


def source_pins(reports):
    pins = {str((BASE / p).relative_to(ROOT)): pin for p, pin in FROZEN.items()}
    for suite, report in reports.items():
        for dep in report['dependencies']:
            if dep['path'] in pins:
                require(pins[dep['path']] == dep['sha256'], 'Conflicting dependency pin')
            pins[dep['path']] = dep['sha256']
        for path, key in [
            ('assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03', 'xbe_sha256'),
            ('assets/disc/maps/lockout.map', 'cache_sha256'),
            ('assets/disc/maps/shared.map', 'shared_cache_sha256'),
        ]:
            if path in pins:
                require(pins[path] == report[key], 'Conflicting original asset pin')
            pins[path] = report[key]
        for row in report['instructions']:
            pins[row['path']] = row['sha256']
        repro_path = BASE / suite / 'reproducibility.json'
        pins[str(repro_path.relative_to(ROOT))] = REPRO_PINS[suite]
        reproduction = json.loads(repro_path.read_text())
        report_path = BASE / suite / 'report.json'
        require(reproduction['byte_identical_report'] and reproduction['instruction_listing_hashes_match'], 'Unfrozen reproduction')
        require(report_path.read_bytes() == (ROOT / reproduction['reexecution_report']).read_bytes(), 'Original reexecution differs')
        pins[reproduction['reexecution_report']] = sha(report_path)
        audit = json.loads((BASE / suite / 'audit.json').read_text())
        require(audit['report_sha256'] == sha(report_path), 'Frozen audit report pin differs')
        pins[f'tools/oracle_mp_{"pickup_visibility" if suite == "pickup-visibility" else "pickup_retreat"}_audit_1_1.py'] = audit['audit_tool_sha256']
        pins[f'docs/MP_PICKUP_{"VISIBILITY" if suite == "pickup-visibility" else "RETREAT"}_RESEARCH.md'] = audit['document_sha256']
    for relative, pin in pins.items():
        require(sha(ROOT / relative) == pin, f'Frozen source changed: {relative}')
    return pins


def audit_source_bytes(reports, cache):
    # The original asset parser is pinned above; no emulation/native import.
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    instructions = 0
    for report in reports.values():
        for listing in report['instructions']:
            lines = (ROOT / listing['path']).read_text().splitlines()
            require(len(lines) == listing['instructions'], 'Listing length differs')
            for line in lines:
                at, data, _ = line.split(' ', 2)
                code = bytes.fromhex(data)
                require(image.read(int(at, 16), len(code)) == code, f'Instruction bytes differ: {at}')
            instructions += len(lines)
    records = 0

    def visit(value):
        nonlocal records
        if isinstance(value, dict):
            if 'raw_hex' in value and 'file_offset' in value:
                offset = value['file_offset']
                offset = int(offset, 16) if isinstance(offset, str) else offset
                raw = bytes.fromhex(value['raw_hex'])
                require(cache[offset:offset + len(raw)] == raw, f'Captured cache bytes differ: {offset:x}')
                records += 1
            for child in value.values():
                visit(child)
        elif isinstance(value, list):
            for child in value:
                visit(child)

    for report in reports.values():
        visit(report)
    return dict(original_instruction_rows=instructions, captured_cache_records=records)


def import_metadata(cache, spatial):
    source = spatial['terrain_metadata']
    chunk = source['chunk']
    magic = int(chunk['base'], 16) - int(chunk['file_offset'], 16)
    root_address = int(source['root']['address'], 16)
    root_offset = record(cache, root_address, magic)
    require(root_offset == int(source['root']['file_offset'], 16), 'Root magic mismatch')
    collision_address = raw_u(cache, root_offset + 0x18)
    collision_offset = record(cache, collision_address, magic)
    require(collision_address == int(source['collision_bsp']['address'], 16), 'Collision descriptor mismatch')
    tag_index = raw_u(cache, 16)
    tag_table = raw_u(cache, tag_index + 8) - (raw_u(cache, tag_index) - (tag_index + 32))
    metadata_magic = raw_u(cache, tag_table + 8) - (tag_index + raw_u(cache, 20))
    bsp_row = int(source['scenario_bsp_row_file_offset'], 16)
    require(cache[bsp_row:bsp_row + 12] == struct.pack('<III', int(chunk['file_offset'], 16), int(chunk['length'], 16), int(chunk['base'], 16)), 'Scenario BSP row differs')
    require(raw_u(cache, int(chunk['file_offset'], 16) + 4) == root_address, 'Chunk root differs')
    blocks = {}
    for name, b in source['blocks'].items():
        offset = int(b['descriptor_offset'], 16)
        count, address = struct.unpack_from('<II', cache, collision_offset + offset)
        require(count == b['count'] and address == int(b['data']['address'], 16), 'Raw collision block differs')
        file_offset = record(cache, address, magic)
        raw = cache[file_offset:file_offset + count * b['stride']]
        require(raw == bytes.fromhex(b['data']['raw_hex']), 'Frozen collision records differ')
        blocks[name] = dict(descriptor_owner='collision', descriptor_offset=f'{offset:02x}', count=count,
            stride=b['stride'], address=f'{address:08x}', file_offset=f'{file_offset:08x}',
            raw_sha256=hashlib.sha256(raw).hexdigest(), raw_hex=raw.hex())
    for name, offset, stride in [('collision_materials', 0x0c, 0x14), ('leaf_clusters', 0x2c, 8)]:
        count, address = struct.unpack_from('<II', cache, root_offset + offset)
        file_offset = record(cache, address, magic)
        raw = cache[file_offset:file_offset + count * stride]
        require(len(raw) == count * stride, 'Root block is truncated')
        blocks[name] = dict(descriptor_owner='root', descriptor_offset=f'{offset:02x}', count=count,
            stride=stride, address=f'{address:08x}', file_offset=f'{file_offset:08x}',
            raw_sha256=hashlib.sha256(raw).hexdigest(), raw_hex=raw.hex())
    require(blocks['leaf_clusters']['count'] == blocks['leaves']['count'] == 2313, 'Leaf bank changed')
    require(blocks['collision_materials']['raw_hex'] == source['collision_materials']['raw_hex'], 'Materials differ')
    return dict(bsp_index=0, bsp_magic=f'{magic:08x}', metadata_magic=f'{metadata_magic:08x}',
        scenario_bsp_row_file_offset=f'{bsp_row:08x}', scenario_bsp_row_stride=0x44,
        scenario_bsp_row_raw_hex=cache[bsp_row:bsp_row + 0x44].hex(), chunk=chunk,
        root=dict(address=f'{root_address:08x}', file_offset=f'{root_offset:08x}', raw_hex=cache[root_offset:root_offset + 0x34].hex()),
        collision=dict(address=f'{collision_address:08x}', file_offset=f'{collision_offset:08x}',
            count=raw_u(cache, root_offset + 0x14), raw_hex=cache[collision_offset:collision_offset + 0x40].hex()),
        blocks=blocks, packed_node_layout=dict(plane_i16_offset=0, negative_child24_offset=2,
            positive_child24_offset=5, leaf_bit='00800000', none_child24='00ffffff'),
        leaf_cluster_layout=dict(cluster_i16_offset=0, stride=8, count_offset='2c', pointer_offset='30', owner='root'),
        intact_breakable_input=dict(count=256, pointer='04030001', words=['ffffffff'] * 8,
            source='Pinned SpatialTrace original full-reset fragment 15B670..15B680 and verified FF fill assertion',
            supplied_storage=source['supplied_intact_breakable_storage']),
        classifier_scope='Original 14B9D0 returns and 11D4A0 locations only; no synthesized native expected classifier or contact values.')


def unified_snapshots(query):
    if 'retreat_trace' not in query:
        return [('C', i, s) for i, s in enumerate(query['correction_steps'])]
    # Both hooks observe the same pre-instruction state. Retain rich retreat
    # observations and the stale pre-formation site absent from the richer hook.
    rich = query['retreat_trace']
    require(query['correction_steps'][0]['instruction'] == '0016b89e', 'Missing pre-formation snapshot')
    remaining = iter(rich)
    for small in query['correction_steps'][1:]:
        found = next((s for s in remaining if s['instruction'] == small['instruction']), None)
        require(found is not None, 'Correction snapshot absent from retreat stream')
        for key in ['fraction_bits', 'point', 'initial_location_hex', 'final_location_hex', 'registers']:
            require(found[key] == small[key], 'Overlapping snapshots differ')
    return [('C', 0, query['correction_steps'][0])] + [('R', i, s) for i, s in enumerate(rich)]


def build(reports, metadata):
    lines = []
    counts = Counter()

    def emit(kind, *fields):
        counts[kind] += 1
        lines.append('\t'.join(map(str, [kind, *fields])))

    blocks = metadata['blocks']
    emit('I', metadata['bsp_index'], metadata['bsp_magic'], metadata['metadata_magic'],
        metadata['root']['address'], metadata['root']['file_offset'], metadata['collision']['address'],
        metadata['collision']['file_offset'], metadata['collision']['count'],
        blocks['collision_materials']['count'], blocks['leaf_clusters']['count'])
    for name, block in blocks.items():
        emit('B', name, block['descriptor_offset'], block['stride'], block['count'], block['address'], block['file_offset'], block['raw_sha256'])
    raw = bytes.fromhex(blocks['collision_materials']['raw_hex'])
    for index in range(blocks['collision_materials']['count']):
        emit('M', index, struct.unpack_from('<h', raw, index * 0x14 + 8)[0])
    raw = bytes.fromhex(blocks['leaf_clusters']['raw_hex'])
    for index in range(blocks['leaf_clusters']['count']):
        emit('J', index, struct.unpack_from('<h', raw, index * 8)[0])
    provenance = dict(queries=[], calls=[])
    ordinary = normal_offsets = iterations = hits = 0
    conditional = []
    query_id = 0
    for suite, report in reports.items():
        cases = {c['query_index']: c for c in report.get('cases', [])}
        selected = range(len(report['outer_queries'])) if suite == 'pickup-visibility' else list(cases)
        require(len(selected) == (1763 if suite == 'pickup-visibility' else 24), 'Frozen query count differs')
        for source_index in selected:
            q = report['outer_queries'][source_index]
            scope = int(int(q['mask'], 16) & 0xe == 0)
            ordinary += scope
            if not scope:
                conditional.append(query_id)
            result = bytes.fromhex(q['result_hex'])
            require(len(result) == 0x5c and q['collision_type'] in (0, 1), 'Unexpected outer result')
            defined = bytearray(0x5c)
            defined[0:0x24] = b'\xff' * 0x24
            defined[0x38:0x44] = b'\xff' * 12
            if q['collision_type'] == 1:
                defined[0x24:0x26] = b'\xff' * 2
                defined[0x28:0x38] = b'\xff' * 16
                defined[0x4c:0x5c] = b'\xff' * 16
                hits += 1
            require(q['fraction_bits'] == f'{raw_u(result, 4):08x}', 'Fraction field mismatch')
            require(q['point']['bits'] == [f'{raw_u(result, 8 + i * 4):08x}' for i in range(3)], 'Point mismatch')
            require(q['initial_location_hex'] == result[0x14:0x1c].hex() and q['final_location_hex'] == result[0x1c:0x24].hex(), 'Location mismatch')
            emit('Q', query_id, suite, source_index, scope, q['caller'], q['mask'], *q['ignored'],
                *q['start']['bits'], *q['delta']['bits'], int(q['return_eax'], 16) & 255, q['collision_type'],
                q['fraction_bits'], *q['point']['bits'], q['initial_location_hex'], q['final_location_hex'],
                defined.hex(), result.hex())
            lower = q['lower']; wrapper = q['lower_wrapper']
            before = bytes.fromhex(lower['descriptor_before_hex']); after = bytes.fromhex(lower['descriptor_after_hex'])
            raw = bytes.fromhex(lower['result_hex']); hit = int(lower['return_eax'], 16) & 255
            visited = lower['visited_leaves']
            require(len(raw) == 0x420 and len(before) == len(after) == 0x28, 'Lower size differs')
            require(raw_u(raw, 0x18) == len(visited) <= 256, 'Visited count differs')
            require(visited == [raw_u(raw, 0x1c + n * 4) for n in range(len(visited))], 'Visited order differs')
            require(lower['start']['bits'] == q['start']['bits'] and lower['delta']['bits'] == q['delta']['bits'], 'Outer/lower inputs differ')
            require(wrapper['breakable_count'] == 256 and wrapper['breakable_pointer'] == '04030001', 'Unexpected breakable bank')
            emit('K', query_id, wrapper['flags'], wrapper['bsp'], lower['root'], *lower['range_bits'],
                wrapper['initial_best_fraction'], wrapper['breakable_count'], wrapper['breakable_pointer'], hit,
                raw_i(before, 0x1c), before[0x20], raw_i(before, 0x24), raw_i(after, 0x1c), after[0x20], raw_i(after, 0x24),
                len(visited), raw[:0x18].hex() if hit else '-', before.hex(), after.hex(), *visited)
            snapshots = unified_snapshots(q)
            groups = []; iteration = -1
            for sequence, (stream, source_snapshot_index, s) in enumerate(snapshots):
                if s['instruction'] == '0016ba30':
                    iteration += 1; groups.append([])
                if groups:
                    groups[-1].append((sequence, s))
                if s['instruction'] == '0016b96b':
                    normal_offsets += 1
                emit('S', query_id, sequence, stream, source_snapshot_index, s['instruction'], iteration,
                    int(s['instruction'] != '0016b89e'), s['fraction_bits'], *s['point']['bits'],
                    s['initial_location_hex'], s['final_location_hex'], *s.get('normal_bits', ['-'] * 4),
                    s.get('dot_or_step_stack_bits', '-'), s.get('xmm0_bits', '-'), s.get('xmm3_bits', '-'), s.get('xmm4_bits', '-'))
            for number, group in enumerate(groups):
                stages = {s['instruction']: (sequence, s) for sequence, s in group}
                require(all(k in stages for k in ['0016ba30', '0016ba4d', '0016ba80', '0016ba85']), 'Incomplete original iteration')
                pre, written, formed, located = [stages[k] for k in ['0016ba30', '0016ba4d', '0016ba80', '0016ba85']]
                emit('R', query_id, number, pre[0], written[0], formed[0], located[0], pre[1]['xmm4_bits'],
                    pre[1]['fraction_bits'], written[1]['fraction_bits'], *pre[1]['point']['bits'],
                    *formed[1]['point']['bits'], pre[1]['final_location_hex'], located[1]['final_location_hex'])
                iterations += 1
            if suite == 'pickup-retreat':
                require(len(groups) == cases[source_index]['retreats'], 'Retreat count differs')
            provenance['queries'].append(dict(id=query_id, suite=suite, source_query_index=source_index,
                phase=q['phase'], output_pointer=q['output_pointer'], ordinary_mask_scope=bool(scope),
                case=cases.get(source_index), source_snapshot_count=len(snapshots)))
            query_id += 1
    # Preserve report call order, including constructor/location duplicates.
    # Return values below are original observations, not a new classifier run.
    call_id = 0
    for suite, report in reports.items():
        for source_index, call in enumerate(report['calls']):
            kind = {'0014b9d0': 'C', '0011d4a0': 'L', '0014ba50': 'P'}.get(call['entry'])
            if not kind:
                continue
            if kind == 'C':
                emit(kind, call_id, suite, source_index, call['caller'], call['root'], call['bsp'],
                    *call['point']['bits'], call['return_eax'])
            elif kind == 'L':
                active_bsp = struct.unpack_from('<h', bytes.fromhex(call['output_hex']), 6)[0]
                require(active_bsp == metadata['bsp_index'], 'Locator active BSP differs')
                emit(kind, call_id, suite, source_index, call['caller'], active_bsp,
                    *call['point']['bits'], call['return_eax'], call['output_hex'])
            else:
                emit(kind, call_id, suite, source_index, call['caller'], call['collision_bsp'], call['signed_plane'], call['output_hex'])
            provenance['calls'].append(dict(id=call_id, suite=suite, source_call_index=source_index,
                phase=call['phase'], entry=call['entry']))
            call_id += 1
    require((query_id, ordinary, iterations, normal_offsets, hits) == (1787, 1785, 2122, 107, 1393), 'Exported query coverage differs')
    require(counts['C'] == 2645 and counts['L'] == 2364 and counts['P'] == 3, 'Original classifier/locator count differs')
    return ('\n'.join(lines) + '\n').encode(), provenance, dict(record_counts=dict(counts), queries=query_id,
        ordinary_mask_queries=ordinary, conditional_active_physics_query_ids=conditional,
        hits=hits, normal_offsets=normal_offsets, retreat_iterations=iterations,
        classifier_calls=counts['C'], locator_calls=counts['L'], oriented_plane_calls=counts['P'])


def schema():
    return dict(version=1, delimiter='TAB', header=False, record_fields={k: v.split() for k, v in FIELDS.items()},
        encoding='ASCII; float32 words, masks, addresses, raw root/leaf returns are eight lowercase hexadecimal digits without 0x; counts/ordinary indices are decimal. B descriptor_offset and P signed_plane are hexadecimal. - is unavailable.',
        field_types={
            'I': ['i16dec', 'hex32', 'hex32', 'hex32', 'hex32', 'hex32', 'hex32', 'u32dec', 'u32dec', 'u32dec'],
            'B': ['text', 'hex8', 'u32dec', 'u32dec', 'hex32', 'hex32', 'sha256'],
            'M': ['u32dec', 'i16dec'], 'J': ['u32dec', 'i16dec'],
            'Q': ['u32dec', 'text', 'u32dec', 'booldec', 'hex32', 'hex32', 'hex32', 'hex32'] + ['float32bits'] * 6
                + ['u8dec', 'u32dec', 'float32bits'] + ['float32bits'] * 3 + ['bytes8', 'bytes8', 'bytes92', 'bytes92'],
            'K': ['u32dec', 'hex32', 'hex32', 'hex32', 'float32bits', 'float32bits', 'float32bits', 'u16dec', 'hex32', 'booldec',
                'i32dec', 'u8dec', 'i32dec', 'i32dec', 'u8dec', 'i32dec', 'u32dec', 'bytes24_or_dash', 'bytes40', 'bytes40', 'u32dec_variadic'],
            'S': ['u32dec', 'u32dec', 'C_or_R', 'u32dec', 'hex32', 'i32dec', 'booldec'] + ['float32bits'] * 4
                + ['bytes8', 'bytes8'] + ['float32bits_or_dash'] * 8,
            'R': ['u32dec'] * 6 + ['float32bits'] * 9 + ['bytes8', 'bytes8'],
            'C': ['u32dec', 'text', 'u32dec', 'hex32', 'hex32', 'hex32'] + ['float32bits'] * 3 + ['hex32'],
            'L': ['u32dec', 'text', 'u32dec', 'hex32', 'i16dec'] + ['float32bits'] * 3 + ['hex32', 'bytes8'],
            'P': ['u32dec', 'text', 'u32dec', 'hex32', 'hex32', 'hex16', 'bytes16'],
        },
        byte_encoding='Raw record/location hex is original little-endian byte order, not a hexadecimal integer.',
        identities='Q/K/S/R id indexes exported queries; C/L/P id indexes the combined original call stream. Source indices and suite names refer to pinned reports.',
        outer_defined_fields=dict(always=['00..23', '38..43'], terrain_hit_only=['24..25', '28..37', '4c..5b'], never=['26..27', '44..4b']),
        undefined_policy='Only bytes whose Q defined_byte_mask is ff are expected values. On K miss lower_hit18 is unavailable. Descriptor padding and inactive snapshot XMM/plane values are captured provenance, not initialized output contracts.',
        snapshot_policy='S observations precede the named original instruction. S at16B89E has a stale, undefined point. R joins exact S pre-subtraction/fraction-written/point-formed/located snapshots, without recomputing expected values.',
        lower_hit18_layout=dict(fraction='00:float32', selected_plane_pointer='04:u32', leaf='08:i32', surface='0c:i32', signed_plane='10:i32', flags='14:u8', breakable='15:u8', material='16:i16'),
        location8_layout=dict(leaf='00:i32', cluster='04:i16', active_bsp='06:i16'),
        classification_scope='Root0 and active ordinary BSP0; all original returns and location bytes preserved, including NONE. L active_bsp is recovered from source-written output+6, not a separately captured global input. Ordered node/plane distance and packed24 children are specified in the source docs; no native-generated outputs.',
        scopes='ordinary_mask_scope is (mask & 0xE)==0. It certifies branch scope of these query inputs, not original world/Havok/current-node producers. Two mask24909C0D records are conditional first-item casts.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    out = args.output_dir.resolve()
    require(ROOT / 'analysis' in out.parents, 'Export only beneath ignored analysis')
    reports = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in REPRO_PINS}
    pins = source_pins(reports)
    cache = (ROOT / 'assets/disc/maps/lockout.map').read_bytes()
    audited = audit_source_bytes(reports, cache)
    spatial = json.loads((BASE / 'spatial/report.json').read_text())
    metadata = import_metadata(cache, spatial)
    fixtures, provenance, coverage = build(reports, metadata)
    payloads = {'fixtures.tsv': fixtures, 'schema.json': json_bytes(schema()),
        'cache-import.json': json_bytes(metadata), 'provenance.json': json_bytes(provenance)}
    manifest = dict(version=1, exporter=str(Path(__file__).relative_to(ROOT)), exporter_sha256=sha(Path(__file__)),
        source_pins=pins, byte_audit=audited, coverage=coverage,
        payloads={name: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest()) for name, data in payloads.items()},
        expected_data_authority='Pinned original reports only: integer/byte decoding and field copying; cache-import records are actual cache inputs. No native code, classifier implementation, collision implementation or new emulator execution is used.',
        retained_boundaries=['Supplied heap/resource residency/arena setup', 'Active Havok and complete item settling/dropped trajectories',
            'Native current-node/equipped pose and world producers', 'Xbox RCPSS/RSQRTPS behavior',
            'Active object/instance broadphase winners', 'Unexecuted zero-dot/unordered and extra collision-BSP retreat paths'])
    payloads['manifest.json'] = json_bytes(manifest)
    out.mkdir(parents=True, exist_ok=True)
    for name, data in payloads.items():
        path = out / name
        if path.exists():
            require(path.read_bytes() == data, f'Refusing to overwrite a different frozen export: {path}')
        else:
            path.write_bytes(data)
    print(json.dumps(dict(output_dir=str(out.relative_to(ROOT)), manifest_sha256=sha(out / 'manifest.json'), **coverage), sort_keys=True))


if __name__ == '__main__':
    main()
