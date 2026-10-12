#!/usr/bin/env python3
"""Freeze read-only original 224550 entry/23ED30 submission observations.

The owner state and initial value-cache bytes come from the running original.
No emission or simulation history is reconstructed by this exporter.
"""
import argparse
import collections
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
MAP_SHA = '1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'


def raw(record, name, size):
    value = record[name]
    result = bytes.fromhex(value) if value is not None else bytes(size)
    assert len(result) == size, (name, len(result), size)
    return result


def u32(data, offset):
    return struct.unpack_from('<I', data, offset)[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--records', type=Path, default=ROOT / 'analysis/frontend/title-effects-runtime/live-sprites.json')
    args = parser.parse_args()
    records = json.loads(args.records.read_text())['records']
    assert 1 <= len(records) <= 65536
    map_path = ROOT / 'assets/disc/maps/mainmenu.map'
    assert hashlib.sha256(map_path.read_bytes()).hexdigest() == MAP_SHA
    assert hashlib.sha256((ROOT / 'assets/disc/default.xbe').read_bytes()).hexdigest() == XBE_SHA
    assets = Cache(map_path)
    inputs = bytearray(struct.pack('<I', len(records)))
    expected = bytearray()
    tags = collections.Counter()
    for record in records:
        cache = raw(record, 'cache_before_hex', 84)
        system = raw(record, 'system_hex', 84)
        location = raw(record, 'location_hex', 56)
        particle = raw(record, 'particle_hex', 64)
        effect = raw(record, 'effect_hex', 400)
        clock = raw(record, 'game_clock_hex', 12)
        seed = raw(record, 'seed_hex', 64)
        context = raw(record, 'context_before_hex', 16)
        bitmap = raw(record, 'bitmap_header_hex', 68)
        _, definition = assets.tag(int(record['tag']['datum'], 16), 'prt3')
        parameters = [p for p in assets.block(definition + 20, 40) if assets.u16(p + 4) == 0]
        assert len(parameters) == 1
        _, bitmap_definition = assets.tag(assets.u32(parameters[0] + 12), 'bitm')
        assert bytes(assets.data[bitmap_definition:bitmap_definition + 64]) == bitmap[:64]
        assert assets.u32(definition + 152) == record['definition_mask']
        flags = sum(bit for key, bit in (('system_hex', 1), ('location_hex', 2),
            ('particle_hex', 4), ('effect_hex', 8)) if record[key] is not None)
        assert bool(flags & 8) == (bool(flags & 1) and u32(system, 24) != 0xffffffff)
        sequence = -1
        if u32(context, 8):
            offset = u32(context, 8) - u32(bitmap, 64)
            assert offset >= 0 and offset % 60 == 0 and offset // 60 < u32(bitmap, 60)
            sequence = offset // 60
        # Preserve raw float words, including untouched uninitialized slots.
        value = struct.pack('<5I', flags, record['definition_mask'] & 0xf80f,
            u32(cache, 68), u32(clock, 8), location[3])
        value += cache[:68] + system[4:8] + system[40:48] + location[28:32]
        value += particle[8:12] + particle[16:20] + particle[56:60] + particle[28:40]
        value += effect[108:116] + clock[4:8] + raw(record, 'camera_frame_hex', 36) + particle[20:28]
        assert len(value) == 184
        color = raw(record, 'color_hex', 16)  # Original alpha, red, green, blue.
        sprite = struct.pack('<I', int(record['tag']['datum'], 16)) + value
        sprite += raw(record, 'position_hex', 12) + raw(record, 'direction_hex', 12)
        sprite += struct.pack('<I', record['scale_bits']) + context[12:16]
        sprite += color[4:16] + color[:4] + seed[20:24] + seed[60:64] + struct.pack('<i', sequence)
        assert len(sprite) == 248
        inputs += sprite
        expected += raw(record, 'cache_after_hex', 84)[:72] + raw(record, 'submission_hex', 88)
        tags[(record['tag']['datum'], record['tag']['name'], record['definition_mask'])] += 1
    fixtures = ROOT / 'tests/fixtures'
    input_path = fixtures / 'title_particle_live_sprite.inputs.bin'
    expected_path = fixtures / 'title_particle_live_sprite.original.bin'
    input_path.write_bytes(inputs)
    expected_path.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), '--live-sprites', str(map_path),
        str(input_path), str(expected_path)], text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = {
        'map_sha256': MAP_SHA,
        'xbe_sha256': XBE_SHA,
        'records_file': str(args.records.resolve().relative_to(ROOT)),
        'records_sha256': hashlib.sha256(args.records.read_bytes()).hexdigest(),
        'records': len(records), 'original_bytes': len(expected), 'different_bytes': 0,
        'all_cache_and_submission_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(),
        'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(),
        'live_bitmap_header_matches_original_asset': True,
        'observed_tags': [{'datum': datum, 'name': name, 'required_values': mask, 'records': count}
            for (datum, name, mask), count in tags.items()],
        'native_test_output': result.stdout.strip(),
        'scope': 'Original live owner state through masked value cache and CPU sprite submission; no proof of emission, simulation history, ordering, GPU billboard styles or whole-frame parity.',
        'complete_frontend_parity_verified': False,
    }
    (ROOT / 'analysis/frontend/title-particle-live-values-sprite.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
