#!/usr/bin/env python3
"""Independent original-byte/input/output audit and literal measured fixture export."""
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source'
OUT = BASE / 'pickup-consumer-measured'
TOOL = ROOT / 'tools/oracle_mp_pickup_consumer_measured_1_1.py'
DOC = ROOT / 'docs/MP_PICKUP_CONSUMER_MEASURED_RESEARCH.md'
TOOL_PIN = '0fd18d4f19876552180d2cf6149c9961c5ff1d7a557bfb09b9e9fafa15918da7'
REPORT_PIN = '4161d66409fa9779302f6cbc75fa41b458f46f0a461866336b95be833cc2fcfe'
FIELDS = {
    'E': 'id unit_type item_type unit_parent item_parent metadata_flags standing crouching radius current_crouch root_x root_y root_z bottom_x bottom_y bottom_z extent output_radius',
    'N': 'id case_id input_x input_y input_z length_spill output_x output_y output_z return_xmm0 chosen_x chosen_y chosen_z fpcw mxcsr reciprocal length80',
    'G': 'case_id instruction vector_name x y z',
    'V': 'id suite source_call_index caller unit item unit_type item_type unit_parent item_parent item_flags contact_bsp contact_instance contact_surface final_al query_begin query_end',
    'Q': 'case_id ordinal suite source_query_index caller mask ignored_unit ignored_item start_x start_y start_z delta_x delta_y delta_z hit result_x result_y result_z endpoint_x endpoint_y endpoint_z',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def value(word):
    return struct.unpack('<f', struct.pack('<I', int(word, 16)))[0]


def f32(number):
    return struct.unpack('<f', struct.pack('<f', number))[0]


def bits(number):
    return f'{struct.unpack("<I", struct.pack("<f", number))[0]:08x}'


def raw_u(raw, at=0):
    return struct.unpack_from('<I', raw, at)[0]


def top_fp(state):
    top = (int(state['status'], 16) >> 11) & 7
    assert state['top'] == top
    assert (int(state['tag'], 16) >> (top * 2)) & 3 != 3
    reg = state['registers'][top]
    raw = struct.pack('<QH', int(reg['mantissa_hex'], 16), int(reg['exponent_hex'], 16))
    assert raw.hex() == reg['raw80_hex']
    return reg


def fp_value(reg):
    mantissa = int(reg['mantissa_hex'], 16); exponent = int(reg['exponent_hex'], 16)
    assert exponent & 0x7fff not in (0, 0x7fff)
    # Measured PC=53 length significands are exactly representable as binary64.
    assert mantissa & 0x7ff == 0
    return math.ldexp((-1 if exponent & 0x8000 else 1) * (mantissa / (1 << 63)), (exponent & 0x7fff) - 16383)


def main():
    assert sha(TOOL) == TOOL_PIN and sha(OUT / 'report.json') == REPORT_PIN
    measured = json.loads((OUT / 'report.json').read_text())
    assert (OUT / 'report.json').read_bytes() == (OUT / 'reexecution/report.json').read_bytes()
    assert measured['source_tool_sha256'] == TOOL_PIN
    for path, pin in measured['source_pins'].items():
        assert sha(ROOT / path) == pin, path
    observed = BASE / 'pickup-consumer-fixtures'
    manifest = json.loads((observed / 'manifest.json').read_text())
    for name, pin in manifest['payloads'].items():
        assert sha(observed / name) == pin['sha256']
        assert (observed / name).read_bytes() == (observed / 'reexecution' / name).read_bytes()
    original = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in ('pickup-visibility', 'pickup-retreat')}
    for entry in measured['preflight'] + measured['measured_baseline']:
        assert entry['byte_identical'] and entry['frozen_listing_writes_compared_and_suppressed']
        assert (ROOT / entry['fresh_report']).read_bytes() == (BASE / entry['suite'] / 'report.json').read_bytes()
    cache = (ROOT / 'assets/disc/maps/lockout.map').read_bytes()
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    rows = 0
    listings = [(BASE / 'pickup-native-contract/00030bd0.asm', 38)]
    for r in original.values():
        listings += [(ROOT / line['path'], line['instructions']) for line in r['instructions']]
    for path, count in listings:
        lines = path.read_text().splitlines(); assert len(lines) == count
        for line in lines:
            address, data, _ = line.split(' ', 2); raw = bytes.fromhex(data)
            assert image.read(int(address, 16), len(raw)) == raw
        rows += count
    records = []; counts = collections.Counter(); controls = collections.Counter(); crouches = collections.Counter()
    below = marker_cases = normal_count = 0

    def emit(kind, *fields):
        records.append('\t'.join(map(str, [kind, *fields]))); counts[kind] += 1

    observed_cases = [json.loads(line) for line in (observed / 'cases.jsonl').read_text().splitlines()]
    observed_casts = collections.defaultdict(list)
    for line in (observed / 'casts.jsonl').read_text().splitlines():
        c = json.loads(line); observed_casts[c['case_id']].append(c)
    for case in measured['cases']:
        case_id = case['global_id']; prior = observed_cases[case_id]
        assert case['suite'] == prior['suite'] and case['source_call_index'] == prior['source_call_index']
        assert case['final_al'] == prior['final_al'] and case['query_range'] == prior['query_range']
        cap = case['capsule']; unit = case['unit_input']; item = case['item_input']
        for state in (unit, item, cap['unit_input'], case['item_point_input']):
            header = bytes.fromhex(state['header_hex']); raw = bytes.fromhex(state['body_prefix_hex'])
            assert header[3] == state['header_type']
            assert raw_u(header, 8) == int(state['body_address'], 16)
            assert struct.unpack_from('<H', header)[0] == int(state['datum'], 16) >> 16
            assert raw_u(raw, 0x14) == int(state['parent'], 16)
            assert raw[0x18] == state['parent_node']
            assert [f'{raw_u(raw, 0x64 + i * 4):08x}' for i in range(3)] == state['root_bits']
            assert raw_u(raw) == int(state['definition'], 16)
        assert (unit['header_type'], item['header_type'], unit['parent'], item['parent']) == (0, 2, 'ffffffff', 'ffffffff')
        unit_raw = bytes.fromhex(cap['unit_input']['body_prefix_hex'])
        assert cap['current_crouch_bits'] == f'{raw_u(unit_raw, 0x2c4):08x}'
        crouches[cap['current_crouch_bits']] += 1
        fields = bytes.fromhex(cap['metadata_fields_hex']); at = int(cap['metadata_file_offset'], 16)
        assert cache[at:at + 16] == fields
        flags, standing, crouching, radius = struct.unpack('<Ifff', fields)
        assert flags == 4
        root = cap['source_root_bits']; bottom = cap['bottom_bits']
        assert root == unit['root_bits'] == prior['capsule']['source_root_bits']
        assert bottom == prior['capsule']['bottom_bits'] and cap['extent_bits'] == prior['capsule']['extent_bits']
        assert cap['radius_bits'] == prior['capsule']['radius_bits'] == bits(radius)
        assert bottom[:2] == root[:2] and bottom[2] == bits(f32(value(root[2]) + radius))
        extent = f32(f32(f32(f32(crouching - standing) * value(cap['current_crouch_bits'])) + standing) - f32(radius * 2.0))
        assert cap['extent_bits'] == bits(extent)
        assert case['item_point_bits'] == prior['item_point']['point_bits']
        item_raw = bytes.fromhex(case['item_point_input']['body_prefix_hex'])
        item_flags = struct.unpack_from('<H', item_raw, 0x12c)[0]
        if item_flags & 0x20:
            marker_cases += 1; assert prior['item_point']['marker_calls']
        else:
            assert case['item_point_bits'] == case['item_point_input']['root_bits']
        emit('E', case_id, unit['header_type'], item['header_type'], unit['parent'], item['parent'], f'{flags:08x}',
            *[f'{raw_u(fields, i * 4):08x}' for i in (1, 2, 3)], cap['current_crouch_bits'],
            *root, *bottom, cap['extent_bits'], cap['radius_bits'])
        emit('V', case_id, case['suite'], case['source_call_index'], case['caller'], unit['datum'], item['datum'],
            unit['header_type'], item['header_type'], unit['parent'], item['parent'], f'{item_flags:04x}',
            *[f'{struct.unpack_from("<H", item_raw, at)[0]:04x}' for at in (0x130, 0x132, 0x134)],
            case['final_al'], *case['query_range'])
        geom = {g['instruction']: g['vectors'] for g in case['geometry']}
        assert geom['000c5845']['bottom'] == bottom and geom['000c5845']['item'] == case['item_point_bits']
        assert geom['000c5845']['top'][:2] == bottom[:2]
        assert geom['000c5845']['top'][2] == bits(f32(value(bottom[2]) + value(cap['extent_bits'])))
        for g in case['geometry']:
            for name, vector in g['vectors'].items():
                emit('G', case_id, g['instruction'], name, *vector)
        for cast in observed_casts[case_id]:
            endpoint = cast['wrapper']['actual_endpoint_bits'] if cast['wrapper'] else ['-'] * 3
            emit('Q', case_id, cast['ordinal'], cast['suite'], cast['source_query_index'], cast['caller'],
                cast['mask'], *cast['ignored'], *cast['start_bits'], *cast['delta_bits'], cast['return_al'],
                *cast['result_point_bits'], *endpoint)
        norm = case['normalizer']
        if norm is None:
            assert len(observed_casts[case_id]) == 1
            continue
        stages = {s['instruction']: s for s in norm['stages']}
        assert norm['caller'] == '000c5985' and norm['input_bits'] == stages['00030be9']['input_or_output_bits']
        assert norm['world_up_bits'] == ['00000000', '00000000', '3f800000']
        assert norm['fallback_bits'] == ['00000000', 'bf800000', '00000000']
        assert image.read(0x46504c, 4).hex() == struct.pack('<I', int(norm['threshold_bits'], 16)).hex()
        controls[norm['entry_fpu_state']['control'] + '/' + norm['entry_fpu_state']['mxcsr']] += 1
        assert norm['entry_fpu_state']['control'] == '027f' and norm['entry_fpu_state']['mxcsr'] == '00001f80'
        assert not stages['00030bf1']['spill_defined'] and stages['00030bf4']['spill_defined']
        length_reg = top_fp(stages['00030bf1']['fpu_state'])
        assert top_fp(stages['00030beb']['fpu_state'])['raw80_hex'] == length_reg['raw80_hex']
        assert bits(fp_value(length_reg)) == norm['length_spill_bits'] == stages['00030bf4']['spill_bits']
        below += '00030c38' in stages
        assert '00030c02' in stages and '00030c38' not in stages
        assert norm['return_xmm0_bits'] == norm['length_spill_bits']
        reciprocal = stages['00030c17']['xmm1_bits']
        assert reciprocal == bits(f32(1.0 / value(norm['length_spill_bits'])))
        assert norm['output_bits'] == [bits(f32(value(v) * value(reciprocal))) for v in norm['input_bits']]
        assert geom['000c59ab']['chosen_side'] == norm['output_bits']
        b = [value(v) for v in bottom]; p = [value(v) for v in case['item_point_bits']]
        d = [f32(a - z) for a, z in zip(p, b)]; up = [value(v) for v in norm['world_up_bits']]
        cross = [f32(f32(d[1] * up[2]) - f32(d[2] * up[1])),
            f32(f32(d[2] * up[0]) - f32(d[0] * up[2])), f32(f32(d[0] * up[1]) - f32(d[1] * up[0]))]
        assert norm['input_bits'] == [bits(v) for v in cross]
        cs = observed_casts[case_id]
        assert geom['000c5b4b']['positive_target'] == cs[2]['wrapper']['actual_endpoint_bits']
        assert geom['000c5b8d']['negative_target'] == cs[3]['wrapper']['actual_endpoint_bits']
        assert geom['000c5b4b'] == geom['000c5b70'] == geom['000c5b8d'] == geom['000c5bb2']
        emit('N', normal_count, case_id, *norm['input_bits'], norm['length_spill_bits'], *norm['output_bits'],
            norm['return_xmm0_bits'], *geom['000c59ab']['chosen_side'], norm['entry_fpu_state']['control'],
            norm['entry_fpu_state']['mxcsr'], reciprocal, length_reg['raw80_hex'])
        normal_count += 1
    assert (len(measured['cases']), normal_count, marker_cases, below, counts['Q']) == (207, 187, 2, 0, 1513)
    fixture = ('\n'.join(records) + '\n').encode()
    schema = dict(version=1, delimiter='TAB', fields={k: v.split() for k, v in FIELDS.items()},
        encoding='All float32 words, datums, parents, masks, callers, metadata flags are hex; IDs/indices/types/counts are decimal. Item/contact/FPCW fields are hex16. length80 is little-endian x87 bytes from the valid source FP register. Missing direct-cast endpoint is -.',
        expected_authority='All exported runtime expectations are literal measured report or frozen observed-export fields. Independent arithmetic only audits correspondence; its calculated values are never fixture output.',
        scope='Stock unparented unit/type0 and BR/SMG item/type2; metadata flags4; all measured crouch values0; all normalizers execute non-small-length path under inherited configured FPCW027F/MXCSR1F80. Two marker/contact origins retain unavailable Havok producer boundary.',
        undefined='Inactive x87 banks and pre-FST spill are captured raw provenance, not expected length. Consumer side-hit replacement, normalizer small-length/fallback, different metadata flags/crouch/type and parented unit cases remain unexecuted.')
    for name, data in {'fixtures.tsv': fixture, 'schema.json': (json.dumps(schema, indent=2) + '\n').encode()}.items():
        path = OUT / name
        if path.exists():
            assert path.read_bytes() == data, name
        else:
            path.write_bytes(data)
    links = []
    for target in re.findall(r'\]\(([^)]+)\)', DOC.read_text()):
        if '://' in target:
            continue
        path = (DOC.parent / target.partition('#')[0]).resolve()
        assert path.exists() or path in (OUT / 'audit.json', OUT / 'manifest.json'), target
        links.append(target)
    audit = dict(source_tool_sha256=TOOL_PIN, report_sha256=REPORT_PIN, audit_tool_sha256=sha(Path(__file__)),
        document_sha256=sha(DOC), byte_identical_independent_measured_reexecution=True,
        byte_identical_original_preflight_and_measured_baseline=True,
        original_byte_instruction_rows=rows, instruction_listings=len(listings), source_pins_preserved=len(measured['source_pins']),
        record_counts=dict(counts), per_call_crouch=dict(crouches), actual_controls=dict(controls),
        independently_audited_capsules=counts['E'], independently_audited_normalizers=normal_count,
        current_marker_contact_cases=marker_cases, below_threshold_cases=below, resolved_doc_links=len(links),
        fixtures_sha256=sha(OUT / 'fixtures.tsv'), schema_sha256=sha(OUT / 'schema.json'),
        scope='Original-byte/cache/header/current-crouch/root/point, capsule SSE arithmetic, actual x87 length→spill, reciprocal/multiply and source buffer/cast correspondence. Expected fixture values copied from original observations only.')
    def preserve_write(name, data):
        path = OUT / name
        if path.exists():
            assert path.read_bytes() == data, f'Refusing changed measured evidence: {name}'
        else:
            path.write_bytes(data)
    preserve_write('audit.json', (json.dumps(audit, indent=2) + '\n').encode())
    manifest = dict(version=1, source_tool_sha256=TOOL_PIN, audit_tool_sha256=sha(Path(__file__)),
        document_sha256=sha(DOC), observed_manifest_sha256=sha(observed / 'manifest.json'),
        payloads={name: dict(sha256=sha(OUT / name), bytes=(OUT / name).stat().st_size)
            for name in ('report.json', 'fixtures.tsv', 'schema.json', 'audit.json')},
        record_counts=dict(counts), global_id_correlation='Measured case global_id equals frozen observed cases.jsonl id; query indices/ranges remain original suite indices.',
        source_pins=measured['source_pins'],
        expected_authority='Untouched original execution and actual cache bytes; no native implementation-derived expected values.',
        limitations=schema['scope'] + ' ' + schema['undefined'])
    preserve_write('manifest.json', (json.dumps(manifest, indent=2) + '\n').encode())
    print(json.dumps(audit))


if __name__ == '__main__':
    main()
