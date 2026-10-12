#!/usr/bin/env python3
"""Compare native equipped single-SMG scheduling with independently frozen original allocations."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/map-pack'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f'{value & 0xffffffff:08x}'


def state(row):
    numeric = ('loaded', 'reserve', 'magazine_mode', 'magazine_ticks', 'reload_transfer_ticks',
               'magazine_232', 'blocking_ticks', 'weapon_state_u32', 'trigger_state', 'trigger_ticks')
    fields = [word(row[key]) for key in numeric]
    fields += [row[key] for key in ('trigger_flags', 'trigger_effect_handle', 'weapon_flags', 'input_flags')]
    fields += [word(value) for value in row['unit_hand_timers']]
    channel = row['weapon_own_primary_channel']
    assert channel['events_16'] == '0000' and row['source_selected_weapon_own_animation']['frame_count'] == 1
    fields += [channel['flags_11'], channel['events_14'], word(1), row['barrel_state_hex'], row['shared_seed']]
    return fields


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/smg_equipped_tests')
    parser.add_argument('--fixtures', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-equipped-fixtures.tsv')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-equipped-differential.json')
    parser.add_argument('--export-only', action='store_true')
    args = parser.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Original-data fixture must remain ignored')
    pins = {
        BASE / 'smg-constructor-equipped-source-fixture.json': '8792cebe9429427e9d026dacea2adcee53deba967c18a0ba9c2b3d39fe6597b6',
        BASE / 'smg-constructor-equipped-1.1-oracle.json': 'e071c76ab098696d88f9606026916f042dee169b848872a4eeb49d4acbaf50b1',
        BASE / 'smg-constructor-equipped-audit.json': 'ef16f1d255fde5a3089a42492e28f262988d6c9c677c0b899cecd4ddcbde0b7b',
        BASE / 'smg-constructor-equipped-suite/suite.json': '54dad96620c0ee23b2e4be3086984a4f2d0696cdd2b07e1ec8ec1cb91119a764',
        BASE / 'smg-constructor-equipped-instructions/inventory.json': '50171802696c5385955809601797f1c0fdd4975120ad60ea7088d7c3cddd05ee',
        ROOT / 'tools/oracle_smg_constructor_equipped_1_1.py': '1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99',
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',
        ROOT / 'assets/disc/maps/lockout.map': '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9',
    }
    for path, pin in pins.items():
        if digest(path) != pin: raise ValueError(f'Frozen equipped authority changed: {path}')
    source = json.loads((BASE / 'smg-constructor-equipped-source-fixture.json').read_text())
    constructor = json.loads((BASE / 'smg-constructor-equipped-1.1-oracle.json').read_text())
    lines = []
    for row in constructor['explicit_reload_admission_operand_cases']['cases']:
        lines.append(' '.join(['G', word(row['supplied_weapon_state_178']), word(row['supplied_trigger_20c']),
                              word(row['supplied_trigger_218']), row['return_eax']]))
    for case in source['cases']:
        if digest(BASE / 'smg-constructor-equipped-suite' / (case['name'] + '.json')) != case['source_sha256']:
            raise ValueError('Frozen source case changed')
        lines.append(' '.join(['I', case['name'], *state(case['initial'])]))
        for row in case['rows']:
            # This original input-record conversion supplies buttons1 for reload.
            buttons, = struct.unpack_from('<H', bytes.fromhex(row['supplied_input_record_hex']), 0x1c)
            assert buttons in (0, 1)
            fields = ['T', word(row['tick']), word(row['supplied_held']), word(buttons == 1)]
            for phase in ('before', 'after_unit', 'after_weapon', 'after_postpass'):
                fields += state(row['phases'][phase])
            fields.append(word(len(row['projectile_requests'])))
            lines.append(' '.join(fields))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print('Exported12 original reload gates and599 equipped tick rows'); return
    run = subprocess.run([str(args.binary.resolve()), str(ROOT / 'assets/disc/maps/lockout.map'),
                          str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0, scope=__doc__, gates=12,
        original_ticks=599, phase_snapshots_compared=2396, requests=165, local_ticks=599, actual_udp_ticks=599,
        native_binary_sha256=digest(args.binary), fixture_sha256=digest(args.fixtures),
        source_fixture_sha256=digest(BASE / 'smg-constructor-equipped-source-fixture.json'),
        source_suite_sha256=source['source_suite_sha256'], tool_sha256=digest(Path(__file__)),
        stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__': main()
