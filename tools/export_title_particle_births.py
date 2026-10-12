#!/usr/bin/env python3
"""Freeze read-only Xemu particle initialization, validating original assets.

Only the completed 249170 initializer is compared. Allocation, emission,
post-initialization motion, child systems, physics and full pixels are separate.
"""
import argparse, collections, hashlib, json, struct, subprocess
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
XBE_SHA = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--records', type=Path, default=ROOT / 'analysis/frontend/title-effects-runtime/live-particle-initialize.json')
    args = parser.parse_args()
    assert hashlib.sha256((ROOT / 'assets/disc/default.xbe').read_bytes()).hexdigest() == XBE_SHA
    assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    records = json.loads(args.records.read_text())['records']
    assert records
    inputs = bytearray(struct.pack('<I', len(records))); expected = bytearray()
    shapes = collections.Counter(); references = collections.Counter()
    for n, record in enumerate(records):
        system = bytes.fromhex(record['system_bytes']); raw = bytes.fromhex(record['definition_bytes'])
        owner, event = struct.unpack_from('<2I', system, 16)
        definition_index = struct.unpack_from('<H', system, 14)[0]
        _, effect = assets.tag(owner, 'effe')
        at = assets.block(assets.block(effect + 20, 56)[event] + 48, 56)[definition_index]
        candidates = []
        for index, emitter in enumerate(assets.block(at + 48, 184)):
            if assets.u32(emitter + 120) != struct.unpack_from('<I', raw, 120)[0]:
                continue
            valid = True
            for offset in (24, 40, 56, 124, 140):
                curve = bytes.fromhex(record['definition_curves'][str(offset)])
                size = assets.u32(emitter + offset + 8)
                ptr = assets.u32(emitter + offset + 12)
                if assets.data[emitter + offset:emitter + offset + 12] != raw[offset:offset + 12] or size != len(curve):
                    valid = False; break
                if size and assets.data[ptr - assets.magic:ptr - assets.magic + size] != curve:
                    valid = False; break
            if valid: candidates.append(index)
        assert len(candidates) == 1, (n, candidates)
        shape = struct.unpack_from('<I', raw, 120)[0]
        assert shape in (2, 3, 4), shape
        shapes[shape] += 1
        references[f'{owner:08x}/{event}/{definition_index}/{candidates[0]}'] += 1
        values = bytes.fromhex(record['values']); before = bytes.fromhex(record['particle_before'])
        after = bytes.fromhex(record['particle_after'])
        assert len(values) == 68 and len(before) == len(after) == 64
        inputs += struct.pack('<4I', owner, event, definition_index, candidates[0])
        inputs += values + struct.pack('<I', record['seed_before']) + before
        expected += after + struct.pack('<I', record['seed_after'])
    fixtures = ROOT / 'tests/fixtures'
    ip, ep = fixtures / 'title_particle_birth.inputs.bin', fixtures / 'title_particle_birth.original.bin'
    ip.write_bytes(inputs); ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), str(assets.path.resolve()), str(ip), str(ep)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': XBE_SHA, 'map_sha256': hashlib.sha256(assets.data).hexdigest(),
        'records_file': str(args.records.relative_to(ROOT)), 'records_sha256': hashlib.sha256(args.records.read_bytes()).hexdigest(),
        'entry': '249170', 'intercepted_instructions': [], 'records': len(records), 'shapes': dict(shapes),
        'system_emitter_references': dict(references), 'authored_property_bytes_match_live_definitions': True,
        'original_output_bytes': len(expected), 'different_bytes': 0, 'all_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(), 'native_test_output': result.stdout.strip(),
        'scope': 'Read-only original Xemu 249170 initializer for observed shape 2/4 emitter properties, position, velocity, lifetime, rotation and RNG; explicit initial state and values.',
        'physical_xbox_rsqrt_verified': False, 'particle_allocation_emission_history_verified': False,
        'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-birth-initialization.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
