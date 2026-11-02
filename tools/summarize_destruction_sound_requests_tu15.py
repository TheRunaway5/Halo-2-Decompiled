#!/usr/bin/env python3
"""Join actual destruction sound caller journals without inferring playback."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--refresh', type=Path, required=True)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    refresh = json.loads(args.refresh.read_text())
    inventory = json.loads(args.inventory.read_text())
    if not refresh.get('passed') or not refresh.get('sound_requests'):
        raise ValueError('Require a completed passing actual sound-request cohort')
    if (not inventory.get('passed') or inventory.get('vehicle_paths') != 37 or
            inventory.get('dependency_roots') != 'vehicle model damage and section responses'):
        raise ValueError('Require the complete original destruction dependency inventory')
    validation = args.refresh.parent / 'validation'
    worker_path = validation / 'report.json'
    worker = json.loads(worker_path.read_text())
    if not worker.get('passed') or any(step.get('exit_code') != 0 for step in worker['steps']):
        raise ValueError('The official CMake/CTest worker did not complete successfully')
    original_banks = {sound for owner in inventory['owners'] for sound in owner['sounds']}
    cases, seen, total = {}, Counter(), 0
    for case in refresh['cases']:
        path = validation / 'evidence' / f'destruction-{case}.json'
        raw = json.loads(path.read_text())
        if not raw.get('passed') or 'original_sound_requests' not in raw:
            raise ValueError(f'{case}: missing passing actual caller journal')
        rows = raw['original_sound_requests']
        if len(rows) > 512:
            raise ValueError(f'{case}: journal exceeds its declared bound')
        digest, by_sound, previous = hashlib.sha256(), {}, None
        for row in rows:
            request = bytes.fromhex(row['request_hex'])
            if len(request) != 160:
                raise ValueError(f'{case}: incomplete original caller request')
            tick, sound, name = row['tick'], row['sound'], row['name']
            if (not isinstance(tick, int) or not 0 <= tick <= 0xffffffff or
                    not isinstance(sound, int) or not 0 <= sound <= 0xffffffff or
                    (previous is not None and tick < previous) or
                    not isinstance(name, str) or (sound != 0xffffffff and not name)):
                raise ValueError(f'{case}: invalid tick or authored sound identity')
            previous = tick
            digest.update(struct.pack('<II', tick, sound))
            digest.update(request)
            record = by_sound.setdefault((sound, name), dict(count=0, first_tick=tick,
                last_tick=tick, controls=Counter(), locations=Counter(), owners=Counter()))
            controls = struct.unpack_from('<I', request)[0]
            location = request[0xf] & 15
            record['count'] += 1
            record['last_tick'] = tick
            record['controls'][str(controls)] += 1
            record['locations'][str(location)] += 1
            if controls & 8:
                record['owners'][str(struct.unpack_from('<I', request, 0x50)[0])] += 1
            if name:
                seen[name] += 1
        sounds = []
        for (sound, name), record in sorted(by_sound.items(), key=lambda item: item[0][1]):
            sounds.append(dict(sound=sound, name=name, **record,
                name_in_original_destruction_dependencies=name in original_banks))
        cases[case] = dict(passed=True, evidence=str(path), evidence_sha256=sha(path),
            requests=len(rows), complete_request_bytes=len(rows) * 160,
            ordered_tick_sound_request_sha256=digest.hexdigest(), sounds=sounds,
            original_destruction_bank_name_matches=sum(row['count'] for row in sounds
                if row['name_in_original_destruction_dependencies']))
        total += len(rows)
    matched = sorted(original_banks & seen.keys())
    report = dict(passed=True,
        scope='Complete original 18CEFC->223B20 caller requests emitted during actual native destruction fixtures; verification replays excluded',
        refresh=str(args.refresh), refresh_sha256=sha(args.refresh),
        worker_report=str(worker_path), worker_sha256=sha(worker_path),
        original_inventory=str(args.inventory), original_inventory_sha256=sha(args.inventory),
        cases=cases, total_requests=total, complete_request_bytes=total * 160,
        matching_original_destruction_bank_names=matched,
        original_destruction_banks_not_observed=sorted(original_banks - seen.keys()),
        bank_name_matches_prove_causal_target_attribution=False,
        source_voice_admission_or_permutation_verified=False,
        waveform_device_or_loop_delivery_verified=False,
        all_37_full_definition_gates_remain_open=True, full_goal_complete=False,
        retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, cases=len(cases), requests=total,
        original_bank_names_observed=len(matched), report=str(args.output))))


if __name__ == '__main__':
    main()
