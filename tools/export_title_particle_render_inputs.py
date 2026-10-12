#!/usr/bin/env python3
"""Freeze original live 175610 handoffs and compare the native producer.

The records are read-only live guest observations, with exact linked-chain
order and original callbacks. They supply completed simulation state, not a
substitute emission implementation.
"""
import argparse, hashlib, json, struct, subprocess
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def raw(group, key, length):
    value = group[key]
    result = bytes.fromhex(value) if value is not None else bytes(length)
    assert len(result) == length, (key, len(result))
    return result

def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--records', type=Path, default=ROOT / 'analysis/frontend/title-effects-runtime/live-render-inputs.json')
    args = parser.parse_args()
    groups = json.loads(args.records.read_text())['groups']
    assert 0 < len(groups) <= 65536
    map_path = ROOT / 'assets/disc/maps/mainmenu.map'
    assert hashlib.sha256(map_path.read_bytes()).hexdigest() == MAP_SHA
    assert hashlib.sha256((ROOT / 'assets/disc/default.xbe').read_bytes()).hexdigest() == XBE_SHA
    assets = Cache(map_path)
    inputs = bytearray(struct.pack('<I', len(groups)))
    expected = bytearray()
    call_count = 0
    references = set()
    for group in groups:
        assert group['callback'] == 0x224550
        system = raw(group, 'system_bytes', 84)
        definition = raw(group, 'definition_bytes', 56)
        owner, event, index = u32(system, 16), u32(system, 20), struct.unpack_from('<H', system, 14)[0]
        references.add((owner, event, index, u32(definition, 4)))
        tag = assets.tags[owner]
        if tag['group'] == 'effe':
            _, root = assets.tag(owner, 'effe')
            selected = assets.block(assets.block(root + 20, 56)[event] + 48, 56)[index]
        elif tag['group'] == 'prt3':
            assert event == 0xffffffff
            _, root = assets.tag(owner, 'prt3')
            selected = assets.block(root + 132, 56)[index]
        else:
            raise AssertionError(tag)
        # Runtime relocation changes pointers, not authored scalar fields.
        assert bytes(assets.data[selected:selected + 52]) == definition[:52]
        emitter_definitions = assets.block(selected + 48, 184)
        assert len(group['emitters']) <= len(emitter_definitions)
        for at, emitter in zip(emitter_definitions, group['emitters']):
            captured = raw(emitter, 'definition_bytes', 184)
            assert bytes(assets.data[at + 176:at + 184]) == captured[176:184]
            for offset in (72, 88, 104):
                assert bytes(assets.data[at + offset:at + offset + 12]) == captured[offset:offset + 12]
                size = assets.u32(at + offset + 8)
                start = assets.pointer(assets.u32(at + offset + 12), assets.magic, size) if size else 0
                assert bytes(assets.data[start:start + size]).hex() == emitter['curves'][str(offset)]
        assert [p['address'] for e in group['emitters'] for p in e['particles']] == [c['particle_address'] for c in group['calls']]
        assert not (struct.unpack_from('<H', system, 12)[0] & 0x400), 'Camera-scale source must be observed before exporting this branch.'
        context = raw(group, 'context_before', 16)
        bitmap = raw(group, 'bitmap_header', 68)
        sequence = -1
        if u32(context, 8):
            relative = u32(context, 8) - u32(bitmap, 64)
            assert relative >= 0 and relative % 60 == 0 and relative // 60 < u32(bitmap, 60)
            sequence = relative // 60
        inputs += struct.pack('<5I', owner, event, index, group['system_address'], group['location_address'])
        inputs += raw(group, 'cache_before', 84) + system + raw(group, 'location_bytes', 52)
        inputs += struct.pack('<I', group['effect_bytes'] is not None) + raw(group, 'effect_bytes', 400)
        inputs += raw(group, 'clock', 12) + raw(group, 'camera', 36)
        inputs += struct.pack('<i', sequence) + context[12:16] + struct.pack('<I', len(group['emitters']))
        for emitter in group['emitters']:
            inputs += raw(emitter, 'bytes', 76) + struct.pack('<I', len(emitter['particles']))
            for particle in emitter['particles']:
                inputs += struct.pack('<I', particle['address']) + raw(particle, 'bytes', 64)
        expected += struct.pack('<I', len(group['calls']))
        for call in group['calls']:
            color = raw(call, 'color', 16)
            expected += struct.pack('<I', call['particle_address']) + raw(call, 'cache', 84)[:72]
            expected += raw(call, 'position', 12) + raw(call, 'direction', 12) + struct.pack('<I', call['scale_bits'])
            expected += color[4:16] + color[:4] + raw(call, 'submission', 88)
        expected += raw(group, 'cache_after', 84)
        call_count += len(group['calls'])
    fixtures = ROOT / 'tests/fixtures'
    input_path = fixtures / 'title_particle_render_inputs.inputs.bin'
    expected_path = fixtures / 'title_particle_render_inputs.original.bin'
    input_path.write_bytes(inputs); expected_path.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), str(map_path), str(input_path), str(expected_path)], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': XBE_SHA, 'map_sha256': MAP_SHA,
        'records_file': str(args.records.resolve().relative_to(ROOT)),
        'records_sha256': hashlib.sha256(args.records.read_bytes()).hexdigest(),
        'groups': len(groups), 'sprite_calls': call_count, 'original_bytes': len(expected),
        'different_bytes': 0, 'all_handoff_and_submission_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(),
        'authored_property_bytes_match_live_definitions': True,
        'system_references': [{'owner': f'{owner:08x}', 'group': assets.tags[owner]['group'],
            'event': event, 'system': system, 'particle': f'{particle:08x}'} for owner, event, system, particle in sorted(references)],
        'native_test_output': result.stdout.strip(),
        'scope': 'Original live world-coordinate rendering-input producer, cache owner invalidation, emitter properties, linked-chain order and CPU sprite submission, supplied completed simulation state.',
        'local_or_first_person_transform_verified': False, 'camera_scale_branch_verified': False,
        'emission_simulation_history_verified': False, 'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-render-inputs.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
