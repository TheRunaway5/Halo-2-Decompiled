#!/usr/bin/env python3
"""Compare owning object dispatchers with immutable original active-item state.

BD360/BD710/1019F0/C0270 remain explicit literal-output producer boundaries;
this comparison does not implement item motion, Havok or the complete frame.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/inventory-source/item-active-schedule'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def verify_authority():
    pins = {
        BASE / 'manifest.json': 'aeabd0ea8833e620c403ea301e805e819fb4882740e8baddcf9e507c4bc84150',
        BASE / 'report.json': 'e347e495f8c37fb3f56b69835692754cb65b77947a8459fea34352ce61ce4be2',
        BASE / 'audit.json': '8bd757188504fb2d9ca69964650fb61dca29d4c3b9f42132320fdada396bf830',
        ROOT / 'docs/MP_ITEM_ACTIVE_SCHEDULE_RESEARCH.md': '5c2f27e60b87544f8d2f79a9308d5d978197ea3c5b297f40b0eadacfd693004f',
        ROOT / 'tools/oracle_mp_item_active_schedule_1_1.py': 'fd63e85b9b799bcf5654b11873ae08bb782d7d0f4e22b64a1c5075287c54dc23',
        ROOT / 'tools/oracle_mp_item_active_schedule_audit_1_1.py': '83046c94612efee322419884acabd85382e1f900c31d23087727f77d333e0ace',
    }
    for path, sha in pins.items():
        require(digest(path) == sha, f'Changed original object-schedule authority: {path}')
    manifest = json.loads((BASE / 'manifest.json').read_text())
    require(not manifest['instruction_patches'], 'Patched original authority')
    for payload in manifest['payloads']:
        require(digest(BASE / payload['path']) == payload['sha256'],
                f"Changed original payload: {payload['path']}")
    for name, sha in {**manifest['tools'], **manifest['source_pins']}.items():
        require(digest(ROOT / name) == sha, f'Changed original prerequisite: {name}')
    for record in manifest['original_instruction_files']:
        path = BASE / record['file']
        require(digest(path) == record['sha256'], f'Changed original instructions: {path}')
        require(path.read_bytes() == (BASE / 'reexecution' / record['file']).read_bytes(),
                f'Original instruction repeat differs: {path}')
    for name in ('report.json', 'audit.json'):
        require((BASE / name).read_bytes() == (BASE / 'reexecution' / name).read_bytes(),
                f'Original independent repeat differs: {name}')
    original = json.loads((BASE / 'report.json').read_text())
    require(not original['instruction_patches'] and
            not original['new_supplied_pose_contact_activation_fields'], 'Supplied original admission/state')
    return original


def export(original, path):
    callers = {
        '000bd360': {'000b7e5e', '000b7f26', '000b8130'},
        '000bd710': {'000b7f9e', '000b8032', '000b8140'},
        '001019f0': {'000b7fe6', '000b8090'},
        '000c0270': {'03000000'},
    }
    header_stores = {'000b8104', '000b8128'}
    context_stores = {'000b7e24', '000b7ec4', '000b7f33', '000b7f43',
                      '000b7f6e', '000b7fef', '000b804f', '000b80d5'}
    phases = []
    states = set()
    callbacks_count = stores_count = 0
    for stage in original['stages']:
        if stage['entry'] not in ('000b7e00', '000b7f50', '000b8040'):
            continue
        # The inherited event stream also contains read-only original marker
        # observations; those are not supplied return services.
        require(stage['completion'] == 'complete_original_return' and
                not any('service' in event for event in stage['services']),
                'Incomplete/supplied original dispatcher')
        callbacks = [call for call in original['calls'] if call['phase'] == stage['name'] and
                     call['entry'] in callers and call['caller'] in callers[call['entry']]]
        callbacks.sort(key=lambda call: call['order'])
        states.update((stage['before_state'], stage['after_state']))
        rows = [['P', stage['name'], stage['entry'], f"{stage['before_state']:x}", f"{stage['after_state']:x}"]]
        for call in callbacks:
            states.update((call['before_state'], call['after_state']))
            arguments = ([call['stack'][0], 'ffffffff'] if call['entry'] in ('000bd360', '000bd710')
                         else ['ffffffff', 'ffffffff'])
            rows.append(['K', call['entry'], *arguments,
                         f"{call['before_state']:x}", f"{call['after_state']:x}"])
        before = original['states'][stage['before_state']]
        pool = before['full_object_header_pool']
        for store in original['stores']:
            if store['phase'] != stage['name'] or store['instruction'] not in header_stores | context_stores:
                continue
            require(store['size'] == 1, 'Unexpected original owning store size')
            if store['instruction'] in header_stores:
                area = 'H'
                offset = int(store['address'], 16) - int(pool['data_pointer'], 16)
                require(0 <= offset < len(bytes.fromhex(pool['data_hex'])) and offset % 12 == 2,
                        'Original header-store correlation differs')
            else:
                area = 'C'
                context = before['object_context']
                offset = int(store['address'], 16) - int(context['pointer'], 16)
                require(offset == 3 and store['source_target']['kind'] == 'object_context',
                        'Original context-store correlation differs')
            rows.append(['W', store['instruction'], area, f'{offset:x}',
                         store['before_hex'], store['written_hex']])
            stores_count += 1
        callbacks_count += len(callbacks)
        phases.extend(rows)
    require(sum(row[0] == 'P' for row in phases) == 3 and callbacks_count == 10 and stores_count == 15,
            'Recorded original dispatcher suite differs')
    rows = []
    for index in sorted(states):
        state = original['states'][index]
        pool = state['full_object_header_pool']
        require(pool['capacity'] == 2048 and pool['stride'] == 12 and
                len(bytes.fromhex(pool['data_hex'])) == 2048 * 12 and
                len(bytes.fromhex(pool['bitmap_hex'])) == 256, 'Original full pool extent differs')
        queue = state['player']['unit_queue1d88_hex']
        context = state['object_context']['raw84_hex']
        require(len(bytes.fromhex(queue)) == 0x1d88 and len(bytes.fromhex(context)) == 0x84,
                'Original owning queue/context extent differs')
        require(context == state['current']['object_world_hex'], 'Original context alias differs')
        rows.append(['Q', f'{index:x}', pool['header4c_hex'], pool['data_hex'],
                     pool['bitmap_hex'], queue, context])
        for body in state['world']['objects']:
            require(len(bytes.fromhex(body['raw_hex'])) == body['size'], 'Original complete body extent differs')
            rows.append(['B', f'{index:x}', body['datum'], body['raw_hex']])
    rows.extend(phases)
    rows.append(['N', '3', f'{callbacks_count:x}', f'{stores_count:x}'])
    path.write_text('\n'.join(' '.join(row) for row in rows) + '\n')
    return dict(phases=3, literal_output_leaves=callbacks_count, owning_stores=stores_count,
                literal_states=len(states))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/object_schedule_tests')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/object-schedule-differential.json')
    args = parser.parse_args()
    original = verify_authority()
    fixture = ROOT / 'analysis/multiplayer/integration/object-schedule-fixtures.tsv'
    counts = export(original, fixture)
    run = subprocess.run([str(args.binary.resolve()), str(fixture)], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    report = dict(passed=run.returncode == 0, scope=__doc__, **counts,
                  complete_headers=2048, header_stride=12, bitmap_bytes=256,
                  queue_bytes=0x1d88, context_bytes=0x84,
                  live_object_producers_integrated=False, full_frame_or_multiplayer_parity=False,
                  limits='One genuine active-owner initial schedule. BD360/BD710/1019F0/C0270 outputs remain literal original leaves. Queue priority, deletion and alternative scan exits are source-listed but unexecuted. World physics and live pickup/motion/storage remain separate.',
                  original_report_sha256=digest(BASE / 'report.json'),
                  original_manifest_sha256=digest(BASE / 'manifest.json'),
                  fixture_sha256=digest(fixture), native_binary_sha256=digest(args.binary),
                  tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__':
    main()
