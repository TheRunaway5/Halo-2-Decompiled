#!/usr/bin/env python3
"""Compare native first-item motion against independently repeated original buffers."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source/item-motion-boundaries'
PINS = {
    ROOT / 'tools/oracle_mp_item_motion_boundaries_1_1.py': '397bf7574733573884f839eaecee40a9aa787b31fa27152f83c6829a31a9bc92',
    ROOT / 'tools/oracle_mp_item_motion_boundaries_audit_1_1.py': '406b06eb48b0eaf3826a0d63c664c8df10d2aa53614bb021dd4ba33863043e6a',
    ROOT / 'docs/MP_ITEM_MOTION_BOUNDARIES_RESEARCH.md': 'c284cdeee1530324246f5a9212405f2c62e7021fa793851fe204989870d5d11c',
    BASE / 'report.json': 'bcd0ca394ee876841e4eba140cff5430fa55809c3adeb429395902e05f759f52',
    BASE / 'audit.json': 'db3896e707f3a5169669b2d0c14fdeed7d7ec6fefe713600d1adb5bb48508954',
    BASE / 'manifest.json': '34015ee4eee80df466c0e5187780d45e711f8ec61022aa23dff08cc2aae3b072',
}
# These unchanged source instructions own native body stores. Other stores
# remain within explicitly supplied transform/effect/world producer intervals.
OWNING = {0xf9f0f, 0xfa2f3, 0xfa372, 0xfa382, 0xfa3a7, 0xbab16,
          0xfa410, 0xfa417, 0xfa435, 0xfa443, 0xfa451, 0xfa458, 0xfa45e,
          0xfa7c9, 0xb8659, 0xb865e, 0xb8664, 0xb8679, 0xb867e, 0xb8684,
          0xfbbbd, 0xfbbc7, 0xfbbcf}
LEAVES = {'000bc910', '0016b240', '000b8490', '000faf20', '000fbbe0'}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def authority():
    for path, sha in PINS.items():
        require(digest(path) == sha, f'Changed immutable item-motion authority: {path}')
    report = json.loads((BASE / 'report.json').read_text())
    require((BASE / 'report.json').read_bytes() == (BASE / 'reexecution/report.json').read_bytes(),
            'Independent original item-motion report differs')
    require((BASE / 'audit.json').read_bytes() == (BASE / 'reexecution/audit.json').read_bytes(),
            'Independent original item-motion audit differs')
    for path, sha in report['source_pins'].items():
        require(digest(ROOT / path) == sha, f'Changed preceding item-motion dependency: {path}')
    for listing in report['instructions']:
        path = Path(listing['file'])
        require(digest(BASE / path) == listing['sha256'], f'Changed motion instruction bytes: {path}')
        require((BASE / path).read_bytes() == (BASE / 'reexecution' / path).read_bytes(),
                f'Independent instruction repeat differs: {path}')
    require(not report['instruction_patches'] and not report['new_guest_inputs'] and
            not report['new_supplied_service_results'], 'Changed original motion composition boundary')
    require(len(report['source_pins']) == 173 and len(report['instructions']) == 49 and
            len(report['states']) == 29, 'Incomplete source motion coverage')
    return report


def raw_fields(value, path=()):
    if isinstance(value, dict):
        for key, child in sorted(value.items()):
            if path == ('current',) and key == 'objects':
                continue  # Complete world bodies contain these exact node/body aliases.
            if path[:2] == ('world', 'objects') and key in {'raw_hex', 'header_hex'}:
                continue  # Full body and full header storage are exported separately.
            if key.endswith('_hex') and isinstance(child, str):
                if child:
                    yield '/'.join(path + (key,)), child
            else:
                yield from raw_fields(child, path + (key,))
    elif isinstance(value, list):
        for i, child in enumerate(value):
            yield from raw_fields(child, path + (str(i),))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/item_motion_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/item-motion-differential.json')
    args = parser.parse_args()
    report = authority()
    rows = []
    for state in report['states']:
        sid = f"{state['id']:x}"
        for obj in state['world']['objects']:
            matched = next(o for o in state['current']['objects'] if o['datum'] == obj['datum'])
            require(matched['body_hex'] == obj['raw_hex'], 'Original complete body alias differs')
            rows.append(['B', sid, obj['datum'], obj['raw_hex']])
        for path, raw in raw_fields(state):
            rows.append(['E', sid, path, raw])
    for definition in report['definitions']:
        rows.append(['D', definition['datum'], definition['record']['raw_hex']])

    def operand(address, size=4):
        values = {r['raw_hex'] for r in report['reads']
                  if int(r['address'], 16) == address and r['size'] == size}
        require(len(values) == 1, f'Missing/changed original scalar operand {address:08x}')
        return next(iter(values))

    def bits(raw):
        return f"{int.from_bytes(bytes.fromhex(raw), 'little'):08x}"

    gravity_ptr = int(report['states'][0]['current']['item_physics_constants'], 16)
    # Each value comes from an actual immutable operand read in this unchanged
    # composition. Resting branches do not consume these dormant inputs.
    values = [bits(operand(0x1016004)), bits(operand(gravity_ptr)),
              bits(operand(0x465a4c)), bits(operand(0x446548)), bits(operand(0x46508c)),
              *[bits(operand(0x447144 + i * 4)) for i in range(3)]]
    bank = f"{int.from_bytes(bytes.fromhex(operand(0x46fb64, 2)), 'little'):04x}"
    cores = [c for c in report['calls'] if c['entry'] in {'000f9b10', '000f9ea0'}]
    require(len(cores) == 6, 'Incomplete original core calls')
    total_leaves = total_stores = 0
    for core in cores:
        scope = core['scope']
        datum = scope['datum']
        before = report['states'][core['before_state']]
        header = bytes.fromhex(before['full_object_header_pool']['data_hex'])
        index = int(datum, 16) & 0xffff
        object_type = header[index * 12 + 3]
        leaves = [c for c in report['calls'] if c['entry'] in LEAVES and
                  core['order'] < c['order'] < c['return_order'] < core['return_order']]
        casts = [c for c in leaves if c['entry'] == '0016b240']
        # An unused resting scratch backing is retained from its actual frame,
        # never manufactured; moving calls use the literal cast-entry packet.
        if casts:
            require(len(casts) == 1, 'Unmeasured alternate cast count')
            scratch = next(p['raw_hex'] for p in casts[0]['packets'] if p['field'] == 'stack5_result5c')
        else:
            frame = core['after_frames'][0]
            raw = bytes.fromhex(frame['raw_hex'])
            scratch = raw[0x58:0x58 + 0x5c].hex()
            require(len(scratch) == 0x5c * 2, 'Missing literal dormant scratch frame')
        result = int(core['return_cpu']['registers']['eax'], 16)
        rows.append(['M', f"row{scope['row']}_call{core['id']}", core['entry'], datum,
                     f'{object_type:x}', f"{core['before_state']:x}", f"{core['after_state']:x}",
                     f'{result:08x}', *values, bank, scratch, 'literal_source'])
        for leaf in leaves:
            rows.append(['K', leaf['entry'], f"{leaf['before_state']:x}", f"{leaf['after_state']:x}",
                         leaf['return_cpu']['registers']['eax']])
            for p, after in zip(leaf['packets'], leaf['after_packets']):
                require(p['field'] == after['field'] and p['pointer'] == after['pointer'],
                        'Changed original packet alias/correlation')
                if p['raw_hex'] is not None:
                    rows.append(['P', p['field'], p['raw_hex'], after['raw_hex']])
        body = int(scope['body'], 16)
        stores = [s for s in report['stores'] if core['order'] < s['order'] < core['return_order'] and
                  int(s['instruction'], 16) in OWNING and
                  body <= int(s['address'], 16) < body + scope['body_size']]
        for store in stores:
            rows.append(['W', store['instruction'], f"{int(store['address'], 16) - body:x}",
                         store['before_hex'], store['written_hex']])
        total_leaves += len(leaves)
        total_stores += len(stores)
    fixture = ROOT / 'analysis/multiplayer/integration/item-motion-fixtures.tsv'
    fixture.write_text('\n'.join(' '.join(map(str, row)) for row in rows) + '\n')
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end='')
    print(run.stderr, end='')
    result = dict(passed=run.returncode == 0, scope=__doc__, core_calls=6,
                  source_states=29, literal_output_leaves=total_leaves, ordered_body_stores=total_stores,
                  compared='Full occupied bodies including embedded nodes; all captured raw non-body/header/world/queue/context/registry buffers; actual producer packet arguments, aliasing and source defined AL.',
                  limits='One original first-item resting/miss/floor-contact schedule. BC910/16B240/B8490/FAF20/FBBE0 outputs remain literal original leaves. No continuous settling, positive Havok, full live frame/world/node/physics, impulses/bounce/timer/nonzero rotation or hardware parity claim.',
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
