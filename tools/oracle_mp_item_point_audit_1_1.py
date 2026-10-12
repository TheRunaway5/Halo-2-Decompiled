#!/usr/bin/env python3
"""Independent source-byte/current-input audit and literal item-point fixtures."""
import collections
import json
from pathlib import Path
import re
import struct

from export_mp_pickup_outer_fixtures_1_1 import ROOT, BASE, sha, require, json_bytes
from oracle_smg_firing_1_1 import Xbe

OUT = BASE / 'pickup-item-point'
TOOL = ROOT / 'tools/oracle_mp_item_point_1_1.py'
DOC = ROOT / 'docs/MP_ITEM_POINT_RESEARCH.md'
TOOL_PIN = 'f9b51db79291538e07dba3d44517f80f0701d623702bd627b965ed9d2e5aab28'
REPORT_PIN = '5d817bc84be74ba29ab505cd5aee06b9fbfb2715c3f2d70c45ba76367e96eda2'
FIELDS = {
    'P': 'id consumer_global_id suite source_call_index datum definition header_type object_flags parent parent_node item_flags_byte contact_bsp contact_instance contact_surface root_x root_y root_z point_x point_y point_z return_eax',
    'N': 'case_id node_index matrix13_hex',
    'Q': 'case_id quaternion4_hex rotation9_hex',
    'C': 'case_id node_matrix13_hex local_marker_matrix13_hex composed_world_matrix13_hex',
    'M': 'case_id selected_node local_matrix13_hex world_matrix13_hex scalar_bits',
    'L': 'case_id signed_plane raw_plane4_hex emitted_plane4_hex',
    'S': 'case_id instruction owned_value words_hex',
    'R': 'case_id sequence instruction address size origin body_offset file_offset raw_hex',
}


def word(raw, offset=0, size=4): return int.from_bytes(raw[offset:offset + size], 'little')
def f32(value): return struct.unpack('<f', struct.pack('<f', value))[0]
def bits(value): return f'{struct.unpack("<I", struct.pack("<f", value))[0]:08x}'
def floats(raw): return struct.unpack('<' + 'f' * (len(raw) // 4), raw)
def words(raw): return [f'{word(raw, i):08x}' for i in range(0, len(raw), 4)]


def preserve(name, data):
    path = OUT / name
    if path.exists(): require(path.read_bytes() == data, f'Refusing changed item-point evidence: {name}')
    else: path.write_bytes(data)


def main():
    require(sha(TOOL) == TOOL_PIN and sha(OUT / 'report.json') == REPORT_PIN, 'Item-point tool/report changed')
    report = json.loads((OUT / 'report.json').read_text())
    require((OUT / 'report.json').read_bytes() == (OUT / 'reexecution/report.json').read_bytes(), 'Independent replay differs')
    for path, pin in report['source_pins'].items(): require(sha(ROOT / path) == pin, path)
    original = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in ('pickup-visibility', 'pickup-retreat')}
    measured = json.loads((BASE / 'pickup-consumer-measured/report.json').read_text())
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    cache = (ROOT / 'assets/disc/maps/lockout.map').read_bytes()
    authored = original['pickup-visibility']['authored_fields']
    resident_offsets = {int(tag['datum'],16) & 0xffff: tag['offset']
        for record in authored for tag in (record['tag'], record['model'], record['render_model'])}
    cache_magic = int(authored[0]['model_link']['address'],16) - authored[0]['model_link']['file_offset']
    counts = collections.Counter(); origins = collections.Counter(); rows = []; instruction_rows = executed_rows = 0
    for listing in report['instructions']:
        path = OUT / 'instructions' / listing['name']; data = path.read_bytes()
        require(sha(path) == listing['sha256'] and data == (OUT / 'reexecution/instructions' / listing['name']).read_bytes(), 'Listing repeat differs')
        lines = data.decode().splitlines(); require(len(lines) == listing['rows'], 'Listing row count differs')
        for line in lines:
            address, encoded, _ = line.split(' ', 2); raw = bytes.fromhex(encoded)
            require(image.read(int(address, 16), len(raw)) == raw, 'Instruction byte differs')
        instruction_rows += len(lines); executed_rows += listing['executed']

    def emit(kind, *fields): rows.append('\t'.join(map(str, [kind, *fields]))); counts[kind] += 1

    projections = fallback = 0
    for case in report['cases']:
        case_id = case['id']; old = original[case['suite']]['calls'][case['source_call_index']]
        require(old['entry'] == '000fbfa0' and old['point']['bits'] == case['point_bits'] and old['return_eax'] == case['return_eax'], 'Original output differs')
        state = old['object_before']; raw = bytes.fromhex(case['imported_body_hex']); header = bytes.fromhex(case['imported_header_hex'])
        require(raw.hex() == state['raw_hex'] and header.hex() == state['header_hex'], 'Literal source input differs')
        require(word(raw) == int(case['definition'], 16) and word(header, 8) == int(state['address'], 16), 'Identity resolution differs')
        require(header[3] == 2 and word(raw, 0x14) == 0xffffffff and raw[4] & 1 == 0, 'Unsupported object scope')
        require(case['configured_fpcw'] == '027f' and case['configured_mxcsr'] == '00001f80', 'Control scope differs')
        matrix_length = word(raw, 0x114, 2); matrix_offset = word(raw, 0x116, 2)
        require(raw[matrix_offset:matrix_offset + matrix_length].hex() == case['imported_node_matrix_hex'] == state['node_matrix_hex'], 'Current node bank differs')
        require(matrix_length == 4 * 0x34, 'Current node count differs')
        for node in range(4): emit('N', case_id, node, raw[matrix_offset + node * 0x34:matrix_offset + (node + 1) * 0x34].hex())
        global_id = case['consumer_global_id']
        if global_id is not None:
            consumer = measured['cases'][global_id]
            require(consumer['suite'] == case['suite'] and consumer['item_point_bits'] == case['point_bits'], 'Consumer correlation differs')
            prefix = bytes.fromhex(consumer['item_point_input']['body_prefix_hex'])
            require(consumer['item_point_input']['datum'] == case['datum'] and prefix[:len(raw)] == raw[:len(prefix)], 'Current consumer item input differs')
        emit('P', case_id, global_id if global_id is not None else -1, case['suite'], case['source_call_index'], case['datum'], case['definition'], header[3],
            f'{word(raw, 4):08x}', f'{word(raw, 0x14):08x}', raw[0x18], f'{raw[0x12c]:02x}',
            *[f'{word(raw, at, 2):04x}' for at in (0x130, 0x132, 0x134)],
            *words(raw[0x64:0x70]), *case['point_bits'], case['return_eax'])
        for sequence, read in enumerate(case['external_reads']):
            address = int(read['address'], 16); encoded = bytes.fromhex(read['raw_hex']); origin = read['origin']; origins[origin] += 1
            require(len(encoded) == read['size'], 'Operand byte length differs')
            if origin == 'literal_current_object': require(raw[address - int(state['address'], 16):address - int(state['address'], 16) + len(encoded)] == encoded, 'Current body operand differs')
            elif origin == 'literal_current_header': require(encoded == header[8:12], 'Current header operand differs')
            elif origin in ('cache_metadata', 'relocated_sbsp'):
                at = int(read['file_offset'], 16); require(cache[at:at + len(encoded)] == encoded, 'Actual cache operand differs')
            elif origin == 'original_xbe': require(image.read(address, len(encoded)) == encoded, 'Actual executable operand differs')
            elif origin == 'explicit_resident_binding': require(encoded == struct.pack('<I', int(case['explicit_bindings'][read['address']], 16)), 'Configured source binding differs')
            else:
                require(origin == 'actual_cache_tag_residency', 'Unknown operand boundary')
                slot_offset = address - int(case['explicit_bindings']['004eb114'],16) - 8
                require(slot_offset % 16 == 0 and encoded == struct.pack('<I', resident_offsets[slot_offset // 16] + cache_magic), 'Actual cache tag residency differs')
            emit('R', case_id, sequence, read['instruction'], read['address'], read['size'], origin,
                read.get('body_offset', '-'), read.get('file_offset', '-'), read['raw_hex'])
        if raw[0x12c] & 0x20 == 0:
            fallback += 1; require(case['return_eax'] == '00000000' and case['point_bits'] == words(raw[0x64:0x70]), 'Root fallback differs')
            require([c['entry'] for c in case['calls']] == ['000fbfa0', '000bacc0'], 'Root fallback call order differs')
            continue
        projections += 1; require(case['return_eax'] == '00000001' and word(raw, 0x132, 2) == 0xffff, 'Ordinary projection scope differs')
        calls = {c['entry']: c for c in case['calls']}; stages = {s['instruction']: s for s in case['stages']}
        require([c['entry'] for c in case['calls']] == ['000fbfa0', '000b9c20', '000b9b90', '001dc8f0', '001dc940', '00143580', '001441d0', '0014ba50'], 'Projection call order differs')
        require(calls['000b9b90']['return_eax'] == case['datum'], 'Marker object resolver differs')
        require(calls['001dc8f0']['return_eax'] == '00000001', 'Ground marker group differs')
        require(calls['001dc940']['node_map_pointer'] == '00000000' and calls['001dc940']['mirror'] == 0 and calls['001dc940']['maximum'] == 1, 'Marker selector scope differs')
        q = calls['00143580']; qraw = bytes.fromhex(''.join(struct.pack('<I', int(w, 16)).hex() for w in q['quaternion_bits']))
        x, y, z, w = floats(qraw); norm = f32(f32(f32(f32(x*x) + f32(y*y)) + f32(z*z)) + f32(w*w))
        factor = f32(2.0 / norm); tx, ty, tz = (f32(v*factor) for v in (x, y, z))
        xx, xy, xz, yy, yz, zz, wx, wy, wz = (f32(a*b) for a,b in ((x,tx),(x,ty),(x,tz),(y,ty),(y,tz),(z,tz),(w,tx),(w,ty),(w,tz)))
        rotation = [f32(1-f32(zz+yy)), f32(xy+wz), f32(xz-wy), f32(xy-wz), f32(1-f32(zz+xx)), f32(yz+wx), f32(xz+wy), f32(yz-wx), f32(1-f32(yy+xx))]
        require(words(bytes.fromhex(q['output_hex'])) == [bits(v) for v in rotation], 'Source quaternion arithmetic differs')
        emit('Q', case_id, qraw.hex(), q['output_hex'])
        compose = calls['001441d0']; node = floats(bytes.fromhex(compose['node_matrix_hex'])); local = floats(bytes.fromhex(compose['local_marker_matrix_hex']))
        expected = [f32(node[0] * local[0])]
        for at in (1, 4, 7):
            expected.extend(f32(f32(local[at] * node[1+k]) + f32(f32(local[at+1] * node[4+k]) + f32(local[at+2] * node[7+k]))) for k in range(3))
        expected.extend(f32(f32(f32(f32(f32(local[10] * node[1+k]) + f32(local[11] * node[4+k])) + f32(local[12] * node[7+k])) * node[0]) + node[10+k]) for k in range(3))
        require(words(bytes.fromhex(compose['output_hex'])) == [bits(v) for v in expected], 'Source matrix composition arithmetic differs')
        emit('C', case_id, compose['node_matrix_hex'], compose['local_marker_matrix_hex'], compose['output_hex'])
        marker = bytes.fromhex(calls['001dc940']['output_hex']); require(word(marker, 0, 2) == 0 and marker[0x38:0x6c].hex() == compose['output_hex'], 'Selected marker matrix differs')
        require(marker[:2] == bytes.fromhex(calls['000b9c20']['output_hex'])[:2] and marker[4:] == bytes.fromhex(calls['000b9c20']['output_hex'])[4:], 'Defined marker bytes differ')
        emit('M', case_id, word(marker, 0, 2), marker[4:0x38].hex(), marker[0x38:0x6c].hex(), f'{word(marker,0x6c):08x}')
        plane = calls['0014ba50']; plane_raw = bytes.fromhex(plane['output_hex']); require(plane['signed_plane'] == '04ff' and plane_raw.hex() == plane['plane_record']['raw_hex'], 'Ordinary plane selection differs')
        emit('L', case_id, plane['signed_plane'], plane['plane_record']['raw_hex'], plane['output_hex'])
        p = floats(marker[0x60:0x6c]); nx,ny,nz,d = floats(plane_raw)
        distance = f32(f32(f32(f32(p[2]*nz) + f32(p[1]*ny)) + f32(nx*p[0])) - d)
        offset = struct.unpack('<f', image.read(0x46508c,4))[0]; shift = f32(offset-distance)
        require(stages['000fc167']['xmm_bits'][6] == bits(distance) and stages['000fc173']['xmm_bits'][3] == bits(shift), 'Source distance/offset stage differs')
        require(case['point_bits'] == [bits(f32(f32(n*shift)+v)) for n,v in zip((nx,ny,nz),p)], 'Source point projection arithmetic differs')
        for site in ('000fc022', '000fc12e'):
            emit('S', case_id, site, 'marker_point', ''.join(struct.pack('<I', int(w,16)).hex() for w in stages[site]['marker_point_bits']))
            emit('S', case_id, site, 'plane_buffer', ''.join(struct.pack('<I', int(w,16)).hex() for w in stages[site]['plane_buffer_bits']))
        emit('S', case_id, '000fc167', 'signed_distance', struct.pack('<I', int(stages['000fc167']['xmm_bits'][6],16)).hex())
        emit('S', case_id, '000fc173', 'projection_shift', struct.pack('<I', int(stages['000fc173']['xmm_bits'][3],16)).hex())
        emit('S', case_id, '000fc19c', 'output_xyz', ''.join(struct.pack('<I', int(w,16)).hex() for w in stages['000fc19c']['xmm_bits'][:3]))
    require((len(report['cases']),fallback,projections) == (208,205,3), 'Coverage differs')
    metadata = [r for r in original['pickup-visibility']['authored_fields'] if r['tag']['group'] == 'weap']
    cache_records = 0
    def audit_records(value):
        nonlocal cache_records
        if isinstance(value,dict):
            if set(('file_offset','raw_hex')).issubset(value):
                at = value['file_offset']; at = int(at,16) if isinstance(at,str) else at
                raw = bytes.fromhex(value['raw_hex']); require(cache[at:at+len(raw)] == raw, 'Authored marker/node record differs'); cache_records += 1
            for v in value.values(): audit_records(v)
        elif isinstance(value,list):
            for v in value:audit_records(v)
    audit_records(metadata)
    schema = dict(version=1, fields={k:v.split()for k,v in FIELDS.items()}, encoding='TAB. IDs/node indices/header types/sizes are decimal; word fields are numeric hex. *_hex matrices/planes/quaternions/rotations/stages are little-endian raw bytes. Missing field is -; absent consumer ID is -1.',
        authority='Every exported expected value is copied from untouched original observations. Arithmetic checks independently audit recorded values and never supply fixture outputs.',
        scope='208 unparented stock BR/SMG items,205 flag-clear root paths,3 actual SMG flag20 ordinary contact projections, node0/mirror0/no node-map/regionFF/maximum1; positive signed-plane04FF. Current object/node/contact bytes are supplied literal original state; full live producers remain outside this replay.',
        undefined='Marker +2 padding, unused raw register values and replay caller/frame addresses are provenance only. No native or source expectation for unexecuted parented/owner redirection, marker failure, mirror, region filtering, instanced contact, negative plane or BR contact paths.')
    preserve('fixtures.tsv', ('\n'.join(rows)+'\n').encode()); preserve('schema.json',json_bytes(schema)); preserve('metadata.json',json_bytes(metadata))
    links=[]
    for link in re.findall(r'\]\(([^)]+)\)',DOC.read_text()):
        if '://' in link:continue
        path=(DOC.parent/link.partition('#')[0]).resolve()
        require(path.exists()or path in (OUT/'audit.json',OUT/'manifest.json'),link);links.append(link)
    audit = dict(tool_sha256=TOOL_PIN, report_sha256=REPORT_PIN, audit_tool_sha256=sha(Path(__file__)),document_sha256=sha(DOC),
        byte_identical_independent_reexecution=True, original_instruction_rows=instruction_rows, executed_listing_rows=executed_rows,
        source_pins_preserved=len(report['source_pins']), original_points_compared=208, consumer_correlations=207,
        marker_projection_cases=3, root_fallback_cases=205, authored_cache_records=cache_records,
        external_read_origins=dict(origins), records=dict(counts), resolved_doc_links=len(links),
        payloads={name:sha(OUT/name)for name in ('fixtures.tsv','schema.json','metadata.json')})
    preserve('audit.json',json_bytes(audit))
    manifest = dict(version=1, tool_sha256=TOOL_PIN,audit_tool_sha256=sha(Path(__file__)),document_sha256=sha(DOC),
        payloads={name:dict(sha256=sha(OUT/name),bytes=(OUT/name).stat().st_size)for name in ('report.json','fixtures.tsv','schema.json','metadata.json','audit.json')},
        instructions=report['instructions'], source_pins=report['source_pins'],coverage=report['coverage'],state_boundary=report['state_boundary'],fpu_boundary=report['fpu_boundary'])
    preserve('manifest.json',json_bytes(manifest)); print(json.dumps(audit))


if __name__ == '__main__': main()
