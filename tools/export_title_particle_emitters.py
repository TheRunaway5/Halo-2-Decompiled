#!/usr/bin/env python3
"""Freeze original live 247470 placement, rate accumulation and birth requests.

247950's observed particle head/count changes are explicit callback inputs.
This does not replace live particle allocation/creation with captured records.
"""
import argparse, collections, hashlib, json, struct, subprocess
from pathlib import Path
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--records', type=Path, default=ROOT / 'analysis/frontend/title-effects-runtime/live-emitter-updates.json')
    args = parser.parse_args()
    assert hashlib.sha256((ROOT / 'assets/disc/default.xbe').read_bytes()).hexdigest() == PIN
    assets = Cache(ROOT / 'assets/disc/maps/mainmenu.map');records = json.loads(args.records.read_text())['records']
    assert records
    inputs = bytearray(struct.pack('<I', len(records)));expected = bytearray();references = collections.Counter();total = 0
    for n, v in enumerate(records):
        system = bytes.fromhex(v['system_before']);definition = bytes.fromhex(v['system_definition_bytes'])
        owner, event = struct.unpack_from('<2I', system, 16);index = struct.unpack_from('<H', system, 14)[0]
        _, root = assets.tag(owner, 'effe')
        at = assets.block(assets.block(root + 20, 56)[event] + 48, 56)[index]
        assert assets.data[at:at + 48] == definition[:48], (n, 'system definition')
        raw = bytes.fromhex(v['definition_bytes']);candidates = []
        for ordinal, emitter in enumerate(assets.block(at + 48, 184)):
            if assets.data[emitter + 156:emitter + 176] != raw[156:176]: continue
            valid = True
            for offset in (8, 24, 40, 56, 72, 88, 104, 124, 140):
                curve = bytes.fromhex(v['definition_curves'][str(offset)]);size = assets.u32(emitter + offset + 8);ptr = assets.u32(emitter + offset + 12)
                if assets.data[emitter + offset:emitter + offset + 12] != raw[offset:offset + 12] or size != len(curve):valid = False;break
                if size and assets.data[ptr - assets.magic:ptr - assets.magic + size] != curve:valid = False;break
            if valid: candidates.append(ordinal)
        assert len(candidates) == 1, (n, candidates)
        references[f'{owner:08x}/{event}/{index}/{candidates[0]}'] += 1
        frame = bytes.fromhex(v['transform_bytes']) if v['transform_bytes'] else bytes(52)
        assert len(frame) == 52
        inputs += struct.pack('<8I', owner, event, index, candidates[0], struct.unpack_from('<H', system, 12)[0], v['dt_bits'], v['color'], bool(v['transform_bytes']))
        inputs += frame + bytes.fromhex(v['emitter_before']) + bytes.fromhex(v['cache_before'])[:68]
        calls = v['birth_calls'];inputs += struct.pack('<I', len(calls));expected += struct.pack('<I', len(calls));total += len(calls)
        for i, call in enumerate(calls):
            before = bytes.fromhex(call['emitter_before'])
            after = bytes.fromhex(calls[i + 1]['emitter_before'] if i + 1 < len(calls) else v['emitter_after'])
            # Only count and linked-list head belong to 247950's emitter output.
            # Remainder differences here are 247470's own between-call work.
            inputs += struct.pack('<2I', struct.unpack_from('<I', after, 4)[0], struct.unpack_from('<H', after, 2)[0])
            expected += struct.pack('<4I', call['fraction_bits'], call['dt_bits'], call['scale_bits'], call['color']) + before
        expected += bytes.fromhex(v['emitter_after'])
    ip = ROOT / 'tests/fixtures/title_particle_emitters.inputs.bin';ep = ROOT / 'tests/fixtures/title_particle_emitters.original.bin'
    ip.write_bytes(inputs);ep.write_bytes(expected)
    result = subprocess.run([str(args.native.resolve()), str(assets.path.resolve()), str(ip), str(ep)], text=True, capture_output=True)
    if result.returncode:raise RuntimeError(result.stdout + result.stderr)
    report = {'xbe_sha256': PIN, 'map_sha256': hashlib.sha256(assets.data).hexdigest(),
        'records_file': str(args.records.relative_to(ROOT)), 'records_sha256': hashlib.sha256(args.records.read_bytes()).hexdigest(),
        'entry': '247470', 'intercepted_instructions': [], 'groups': len(records), 'birth_requests': total,
        'authored_definition_and_property_bytes_match': True, 'system_emitter_references': dict(references),
        'original_output_bytes': len(expected), 'different_bytes': 0, 'all_bytes_exact': True,
        'input_sha256': hashlib.sha256(inputs).hexdigest(), 'original_sha256': hashlib.sha256(expected).hexdigest(),
        'native_binary_sha256': hashlib.sha256(args.native.read_bytes()).hexdigest(), 'native_test_output': result.stdout.strip(),
        'supplied_callback_outputs': ['particle linked-list head', 'particle count'],
        'scope': 'Live original emitter placement, accumulation, threshold, timing arguments and request order, with observed creation callback head/count outputs supplied explicitly.',
        'particle_allocation_creation_history_verified': False, 'complete_frontend_parity_verified': False}
    (ROOT / 'analysis/frontend/title-particle-emitter-updates.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':main()
