#!/usr/bin/env python3
"""Export observed C57A0/E0C20 results with unmeasured inputs left absent."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
from export_mp_pickup_outer_fixtures_1_1 import ROOT, BASE, sha, require, source_pins, json_bytes

OUT = BASE / 'pickup-consumer-fixtures'
PINS = {
    'tools/export_mp_pickup_outer_fixtures_1_1.py': '666b8b0cc79aa2dc1ede057e015a373ff6608e7db931ba46b5fca9e6579ab124',
    'analysis/multiplayer/integration/inventory-source/pickup-native-contract/00030bd0.asm': '4ac33ed77002105df833f5446e5974aec847ed939eaae77a4679e980ed601d3a',
    'analysis/multiplayer/integration/inventory-source/pickup-native-contract/source-inputs.json': 'd1134cb7d9add9aca7b666a52694550586eba185ecc4af19560712963a336bd0',
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=OUT)
    out = parser.parse_args().output_dir.resolve()
    require(ROOT / 'analysis' in out.parents, 'Export only under ignored analysis')
    reports = {suite: json.loads((BASE / suite / 'report.json').read_text()) for suite in ('pickup-visibility', 'pickup-retreat')}
    pins = source_pins(reports) | PINS
    for relative, pin in pins.items():
        require(sha(ROOT / relative) == pin, f'Frozen source changed: {relative}')
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    listing = ROOT / 'analysis/multiplayer/integration/inventory-source/pickup-native-contract/00030bd0.asm'
    for line in listing.read_text().splitlines():
        at, code, _ = line.split(' ', 2)
        require(image.read(int(at, 16), len(bytes.fromhex(code))) == bytes.fromhex(code), 'Normalizer bytes differ')
    constants = json.loads((ROOT / 'analysis/multiplayer/integration/inventory-source/pickup-native-contract/source-inputs.json').read_text())['constants']
    for constant in constants:
        raw = bytes.fromhex(constant['raw_hex'])
        require(image.read(int(constant['address'], 16), len(raw)) == raw, 'Source constant differs')
    constants += [dict(address='0046507c', raw_hex=image.read(0x46507c, 4).hex())]
    cache = (ROOT / 'assets/disc/maps/lockout.map').read_bytes()
    cases = []; casts = []; outputs = {}; true = clips = side_branches = 0
    for suite, report in reports.items():
        source_calls = report['calls']
        query_calls = [(i, c) for i, c in enumerate(source_calls) if c['entry'] == '0016b240']
        require(len(query_calls) == len(report['outer_queries']), 'Outer call binding differs')
        startup = bytes.fromhex(report['initial']['startup']['unit_initialized']['object_state_hex'])
        # Metadata magic is decoded from the untouched cache header, not native.
        u = lambda at: struct.unpack_from('<I', cache, at)[0]
        index = u(16); table = u(index + 8) - (u(index) - (index + 32))
        metadata_magic = u(table + 8) - (index + u(20))
        for call_index, call in enumerate(source_calls):
            if call['entry'] != '000c57a0':
                continue
            case_id = len(cases); begin, end = call['query_range']
            nested = source_calls[call_index + 1:query_calls[begin][0]]
            capsule = next(c for c in nested if c['entry'] == '000e0c20' and c['caller'] == '000c5810')
            root = next(c for c in nested if c['entry'] == '000bacc0' and c['caller'] == '000e0c5c')
            point = next(c for c in nested if c['entry'] == '000fbfa0' and c['caller'] == '000c5845')
            require(capsule['registers']['eax'] == call['unit'], 'Unit/capsule identity differs')
            require(point['object_before']['datum'] == call['target'], 'Target point identity differs')
            definition_address = int(root['registers']['edi'], 16)
            file_offset = definition_address - metadata_magic
            fields = cache[file_offset + 0x264:file_offset + 0x274]
            require(len(fields) == 16, 'Biped metadata is unavailable')
            metadata_id = f'{definition_address:08x}'
            outputs[metadata_id] = dict(address=f'{definition_address + 0x264:08x}',
                file_offset=f'{file_offset + 0x264:08x}', raw_hex=fields.hex(),
                flags=struct.unpack_from('<I', fields)[0], standing_bits=f'{struct.unpack_from("<I", fields, 4)[0]:08x}',
                crouching_bits=f'{struct.unpack_from("<I", fields, 8)[0]:08x}', radius_bits=f'{struct.unpack_from("<I", fields, 12)[0]:08x}')
            marker = [dict(source_call_index=call_index + 1 + i, call=c) for i, c in enumerate(nested)
                if c['entry'] in ('000b9c20', '001dc8f0', '001dc940', '0014ba50')]
            target = point['object_before']
            annotated = []
            poses = [(f'authored[{i}].pose', row['pose']) for i, row in enumerate(report.get('authored', []))]
            poses += [(f'near[{i}].pose', row['pose']) for i, row in enumerate(report.get('near', []))]
            if 'conditional_source_contact' in report:
                poses.append(('conditional_source_contact.pose', report['conditional_source_contact']['pose']))
            if 'pose' in report:
                poses.append(('pose', report['pose']))
            for reference, pose in poses:
                if pose['source_root']['bits'] == root['point']['bits']:
                    annotated.append(dict(reference=reference, authored_start=pose['authored_start'],
                        authored_record=pose['authored_record'], supplied_proximity_root=pose['supplied_proximity_root'],
                        source_root=pose['source_root']))
            case = dict(id=case_id, suite=suite, source_call_index=call_index, phase=call['phase'],
                caller=call['caller'], unit=call['unit'], item=call['target'], query_range=call['query_range'],
                final_al=int(call['return_eax'], 16) & 255, visible=call['visible'],
                capsule=dict(source_call_index=source_calls.index(capsule), root_source_call_index=source_calls.index(root),
                    metadata=metadata_id, source_root_bits=root['point']['bits'], bottom_bits=capsule['bottom']['bits'],
                    extent_bits=capsule['extent_bits'], radius_bits=capsule['radius_bits'],
                    current_crouch_bits=None, current_unit_parent=None, current_unit_header_type=None),
                initial_unit_provenance=dict(definition=f'{struct.unpack_from("<I", startup)[0]:08x}',
                    parent=f'{struct.unpack_from("<I", startup, 0x14)[0]:08x}', object_type_byte=startup[0xaa],
                    crouch_bits=f'{struct.unpack_from("<I", startup, 0x2c4)[0]:08x}',
                    scope='Initial source constructor snapshot only; these values are not substituted for later per-call inputs.'),
                item_point=dict(source_call_index=source_calls.index(point), point_bits=point['point']['bits'],
                    source_object_before=target, marker_calls=marker),
                pose_root_matches=annotated,
                side_normalizer=dict(executed=end - begin > 2, input_bits=None, length_spill_bits=None,
                    normalized_output_bits=None, return_bits=None,
                    scope='Not hooked in frozen reports; source instruction bytes and downstream emitted casts are retained.'),
                scope='Actual observed capsule/item point and C57A0 cast consumer. Supplied roots/heap/residency, full motion/current-node and reciprocal boundaries remain explicit.')
            require(case['final_al'] == int(case['visible']), 'Final AL/visible mismatch')
            true += case['visible']; side_branches += case['side_normalizer']['executed']
            for ordinal, query_index in enumerate(range(begin, end)):
                q = report['outer_queries'][query_index]
                wrapper = None
                if ordinal >= 2:
                    outer_call_index = query_calls[query_index][0]
                    wrapper_index = outer_call_index - 1
                    wrapper = source_calls[wrapper_index]
                    require(wrapper['entry'] == '0016bac0', 'Endpoint wrapper not immediately before outer query')
                cast = dict(case_id=case_id, ordinal=ordinal, suite=suite, source_query_index=query_index,
                    caller=q['caller'], mask=q['mask'], ignored=q['ignored'], start_bits=q['start']['bits'],
                    delta_bits=q['delta']['bits'], return_al=int(q['return_eax'], 16) & 255,
                    result_point_bits=q['point']['bits'], fraction_bits=q['fraction_bits'], collision_type=q['collision_type'],
                    initial_location8=q['initial_location_hex'], final_location8=q['final_location_hex'],
                    wrapper=None if wrapper is None else dict(source_call_index=wrapper_index, caller=wrapper['caller'],
                        actual_start_bits=wrapper['start']['bits'], actual_endpoint_bits=wrapper['end']['bits']))
                require(q['ignored'] == [call['unit'], call['target']], 'Excluded identity/order differs')
                if ordinal in (2, 3):
                    clips += 1
                    require(cast['mask'] == '24800001' and cast['return_al'] == 0, 'Unexpected source side-clip hit')
                casts.append(cast)
            cases.append(case)
    require((len(cases), len(casts), true, side_branches, clips) == (207, 1513, 21, 187, 374), 'Frozen consumer coverage differs')
    payloads = {'cases.jsonl': b''.join((json.dumps(c, separators=(',', ':')) + '\n').encode() for c in cases),
        'casts.jsonl': b''.join((json.dumps(c, separators=(',', ':')) + '\n').encode() for c in casts),
        'metadata.json': json_bytes(dict(biped_capsule_fields=outputs, constants=constants,
            normalizer_listing=str(listing.relative_to(ROOT)), normalizer_instruction_count=len(listing.read_text().splitlines()),
            missing_inputs=['Per-call unit+2C4 crouch, unit parent/header type', '30BD0 input, x87 length/spill, normalized output and return',
                'Internal C57A0 pre/post clip endpoint stores; all actual side clips are clear'],
            policy='All absent current inputs remain null. Source-produced roots/capsule/marker/item points and emitted cast expectations are literal original observations. No expected geometry is reconstructed.'))}
    manifest = dict(version=1, tool_sha256=sha(Path(__file__)), source_pins=pins,
        coverage=dict(cases=len(cases), casts=len(casts), visible=true, blocked=len(cases) - true,
            side_normalizer_branches=side_branches, source_side_clips=clips, source_side_clip_hits=0),
        payloads={name: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest()) for name, data in payloads.items()},
        authority='Pinned original reports and original cache/XBE bytes only; no native or new runtime evaluation.')
    payloads['manifest.json'] = json_bytes(manifest)
    out.mkdir(parents=True, exist_ok=True)
    for name, data in payloads.items():
        path = out / name
        if path.exists():
            require(path.read_bytes() == data, f'Refusing to overwrite changed evidence: {path}')
        else:
            path.write_bytes(data)
    print(json.dumps(dict(output=str(out.relative_to(ROOT)), manifest_sha256=sha(out / 'manifest.json'), **manifest['coverage'])))


if __name__ == '__main__':
    main()
