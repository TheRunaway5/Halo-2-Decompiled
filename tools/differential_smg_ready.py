#!/usr/bin/env python3
"""Compare native FP event timing and firing admission with frozen TU1.1 output.

Local/UDP checks cover 44 original-wrapper ticks with the explicit timed-equip
boundary supplied. Initial loadout instead requests instant equip; full weapon
constructor fields and first-person presentation remain separate gates.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'analysis/multiplayer/map-pack/smg-ready-admission-1.1-oracle.json'
def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def state_fields(s):
    return [str(s['blocking_ticks']), str(s['loaded']), str(s['reserve']),
        str(s['hand_timers'][0]), str(s['hand_timers'][1]), s['input_flags'],
        s['weapon_flags'], s['trigger_flags'], s['barrel_state_hex']]
def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/smg_ready_tests')
    p.add_argument('--fixtures', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-ready-fixtures.tsv')
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/smg-ready-differential.json')
    p.add_argument('--export-only', action='store_true')
    args = p.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents: raise ValueError('Keep original exports in ignored analysis')
    for path, pin in {
        REPORT: 'c5a128b7fb32dac666e573595f660c4b1569845bf524cbe0e20e5f919596a2e3',
        ROOT / 'tools/oracle_smg_ready_admission_1_1.py': 'cc5d445c1d048d17be6189d2e340c18f794c4e7dd17ed9d4baaff429ec49961b',
        ROOT / 'tools/oracle_spartan_equipped_unit_phase_1_1.py': 'cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38',
        ROOT / 'analysis/multiplayer/map-pack/smg-ready-admission-instructions/inventory.json': 'd132eab864487fb57a87304cdc2635c959bdfd825b836db22f0deeb62d5369a1',
    }.items():
        if digest(path) != pin: raise ValueError(f'Original ready authority changed: {path}')
    source = json.loads(REPORT.read_text())
    for path, pin in {
        ROOT / 'assets/disc/maps/lockout.map': source['cache_sha256'],
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': source['xbe_sha256'],
    }.items():
        if digest(path) != pin: raise ValueError(f'Original source changed: {path}')
    inventory = json.loads((ROOT / 'analysis/multiplayer/map-pack/smg-ready-admission-instructions/inventory.json').read_text())
    for row in inventory['routines']:
        if digest(ROOT / row['listing']) != row['listing_sha256']: raise ValueError('Original ready listing changed')
    clip = source['authored']['first_person_block'][0]['selected_animations'][0]
    lines = [' '.join(['C', str(clip['index']), str(clip['frame_count']), str(len(clip['events'])),
        *[str(v) for e in clip['events'] for v in (e['type'], e['frame'])]])]
    for row in source['explicit_varied_clock_queries']:
        lines.append(f"R {row['supplied_clock_rate']} {row['result']}")
    for i, row in enumerate(source['explicit_modified_admission_states']):
        s = row['after_postpass']
        lines.append(' '.join(['G', str(i), str(row['supplied_blocking_ticks']),
            str(row['supplied_input_flags']), str(row['supplied_weapon_flags']),
            *state_fields(s), s['seed'], str(len(row['requests']))]))
    ticks = 0
    for name, case in source['stock_30_tick_cases'].items():
        for row in case['rows']:
            lines.append(' '.join(['T', name, str(row['tick']), str(int(row['supplied_held'])),
                *state_fields(row['after_postpass']), str(len(row['projectile_requests']))]))
            ticks += 1
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(f'Exported 7 clock queries, 7 admission states and {ticks} live ticks'); return
    run = subprocess.run([str(args.binary.resolve()), str(ROOT / 'assets/disc/maps/lockout.map'),
        str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0,
        clock_queries=7, admission_states=7, original_ticks=ticks, native_local_udp_ticks=2*ticks,
        scope=__doc__, native_binary_sha256=digest(args.binary), source_report_sha256=digest(REPORT),
        fixtures_sha256=digest(args.fixtures), tool_sha256=digest(Path(__file__)),
        stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)
if __name__ == '__main__': main()
