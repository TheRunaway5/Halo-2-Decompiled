#!/usr/bin/env python3
"""Freeze successful original 247950 creation before child/effect dispatch.

Allocated records and cache owners are explicit inputs; original instructions
are unchanged. Creation/linking/placement output is measured at 247E2A.
"""
import argparse, collections, hashlib, json, struct, subprocess
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def main():
    parser = argparse.ArgumentParser(description=__doc__);parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--records', type=Path, default=ROOT / 'analysis/frontend/title-effects-runtime/live-particle-creation.json')
    args = parser.parse_args();assert hashlib.sha256((ROOT / 'assets/disc/default.xbe').read_bytes()).hexdigest() == PIN
    assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map');records = json.loads(args.records.read_text())['records'];assert records
    inputs = bytearray(struct.pack('<I', len(records)));expected = bytearray();references = collections.Counter()
    for n, v in enumerate(records):
        system = bytes.fromhex(v['system_before']);raw = bytes.fromhex(v['definition_bytes']);cache = bytes.fromhex(v['cache_before'])
        assert struct.unpack_from('<I', cache, 72)[0] == v['system_address'], (n, 'cache owner')
        owner, event = struct.unpack_from('<2I', system, 16);index = struct.unpack_from('<H', system, 14)[0]
        _, root = assets.tag(owner, 'effe');at = assets.block(assets.block(root + 20, 56)[event] + 48, 56)[index]
        candidates = []
        for ordinal, emitter in enumerate(assets.block(at + 48, 184)):
            if assets.u32(emitter + 120) != struct.unpack_from('<I', raw, 120)[0]:continue
            valid = True
            for offset in (24, 40, 56, 124, 140):
                curve = bytes.fromhex(v['definition_curves'][str(offset)]);size = assets.u32(emitter + offset + 8);ptr = assets.u32(emitter + offset + 12)
                if assets.data[emitter + offset:emitter + offset + 12] != raw[offset:offset + 12] or size != len(curve):valid = False;break
                if size and assets.data[ptr - assets.magic:ptr - assets.magic + size] != curve:valid = False;break
            if valid:candidates.append(ordinal)
        assert len(candidates) == 1, (n, candidates);references[f'{owner:08x}/{event}/{index}/{candidates[0]}'] += 1
        assert v['allocated_datum'] != 0xffffffff and 'prefix_particle' in v
        allocated = bytes.fromhex(v['particle_after_allocation']);assert len(allocated) == 64 and allocated[2:] == bytes(62)
        inputs += struct.pack('<6I', owner, event, index, candidates[0], v['allocated_datum'], v['particle_address'])
        inputs += allocated + bytes.fromhex(v['emitter_before']) + cache + system
        inputs += bytes.fromhex(v['location_bytes']) if v['location_bytes'] else bytes(52)
        inputs += struct.pack('<I', v['effect_bytes'] is not None)
        inputs += bytes.fromhex(v['effect_bytes']) if v['effect_bytes'] else bytes(400)
        inputs += bytes.fromhex(v['clock']) + bytes.fromhex(v['camera'])
        inputs += struct.pack('<5I', v['fraction_bits'], v['dt_bits'], v['scale_bits'], v['color'], v['seed_before'])
        expected += bytes.fromhex(v['prefix_particle']) + bytes.fromhex(v['prefix_cache']) + bytes.fromhex(v['prefix_emitter']) + struct.pack('<I', v['prefix_seed'])
    ip = ROOT / 'tests/fixtures/title_particle_creation.inputs.bin';ep = ROOT / 'tests/fixtures/title_particle_creation.original.bin'
    ip.write_bytes(inputs);ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), str(assets.path.resolve()), str(ip), str(ep)], capture_output=True, text=True)
    if result.returncode:raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': PIN, 'map_sha256': hashlib.sha256(assets.data).hexdigest(),
        'records_file': str(args.records.relative_to(ROOT)), 'records_sha256': hashlib.sha256(args.records.read_bytes()).hexdigest(),
        'entry': '247950 successful allocation -> 247E2A before child/effect dispatch', 'intercepted_instructions': [],
        'records': len(records), 'system_emitter_references': dict(references), 'authored_property_bytes_match': True,
        'original_output_bytes': len(expected), 'different_bytes': 0, 'all_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(), 'native_test_output': result.stdout.strip(),
        'scope': 'Original successful particle slot linking, random fields, cache completion, initialization, placement, color/count and RNG up to child/effect dispatch, with allocated slots supplied explicitly.',
        'native_allocation_verified': False, 'child_system_birth_effect_dispatch_verified': False,
        'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-creation.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
if __name__ == '__main__':main()
