#!/usr/bin/env python3
"""Compare ordinary native Slayer scoring/ending with frozen original execution.

Attribution history, medals, round transitions and announcement delivery remain
outside this slice. Original fixture exports stay in ignored local analysis.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / 'analysis/multiplayer/integration/spawn-source/slayer-accounting-report.json'
CLOCK = ROOT / 'analysis/multiplayer/integration/spawn-source/clock-text-report.json'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/slayer_accounting_tests')
    p.add_argument('--fixtures', type=Path, default=ROOT / 'analysis/multiplayer/integration/slayer-accounting-fixtures.tsv')
    p.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/slayer-accounting-differential.json')
    p.add_argument('--export-only', action='store_true')
    args = p.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Keep original fixtures in ignored local analysis')
    for path, pin in {
        REPORT: '284cbcd6bb293bb8e9edc74e62dc9ffe3b3a2929877983884abc93d05b66f128',
        ROOT / 'tools/oracle_mp_spawn_slayer_accounting.py': '462a23a83a8a3525ae958d615c35ea19064c56509119c858a2d9b5cdd1ae7563',
        CLOCK: 'cb557916448e79a079b5bc80813f53238c98c171aee4ff59e433f4c5a7d82e55',
        ROOT / 'tools/oracle_mp_clock_text_1_1.py': 'be0062d7c85f71888fd0796368070cadf40513d2e314e61235404cc4f5cd91bc',
    }.items():
        if digest(path) != pin: raise ValueError(f'Original authority changed: {path}')
    report = json.loads(REPORT.read_text())
    for relative, pin in report['dependencies'].items():
        if digest(ROOT / relative) != pin: raise ValueError(f'Original dependency changed: {relative}')
    for path, pin in {
        ROOT / report['source_listing']: report['source_listing_sha256'],
        ROOT / 'assets/disc/maps/lockout.map': report['cache_sha256'],
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': report['executable_sha256'],
    }.items():
        if digest(path) != pin: raise ValueError(f'Original source changed: {path}')
    lines = []
    counts = dict(callbacks=0, native_callbacks=0, writers=0, ranking=0, ending=0, clocks=0)
    clock = json.loads(CLOCK.read_text())
    for relative, pin in clock['dependencies'].items():
        if digest(ROOT / relative) != pin: raise ValueError(f'Original clock dependency changed: {relative}')
    for field in ('source_listing', 'executable', 'cache'):
        if digest(ROOT / clock[field]) != clock[field + '_sha256']:
            raise ValueError(f'Original clock authority changed: {field}')
    for row in clock['cases']:
        lines.append(f'F {row["clock_seconds"]} {json.dumps(row["text"])}')
    counts['clock_text'] = len(clock['cases'])
    for row in report['score_callbacks']:
        if row['flags22c'] & 1: continue  # Bonus scoring is explicitly unsupported.
        victim_flags = 3 if row['case'] == 'victim_flag1' else 1
        requests = [e for e in row['events'] if e['kind'] == 'original_score_request']
        values = [row['flags22c'], row['attacker'], row['victim'], row['relationship_flag'], victim_flags, len(requests)]
        for request in requests: values += [request['player'], request['delta']]
        lines.append('D ' + ' '.join(map(str, values))); counts['callbacks'] += 1
        if row['case'] == 'relationship_flag': continue  # Native FFA has no such producer yet.
        expected = [row['after']['stats'][str(slot)]['fields'][0] for slot in (0, 3, 15)]
        lines.append('N ' + ' '.join(map(str, values[:5] + expected))); counts['native_callbacks'] += 1
    for row in report['score_writer_boundaries']:
        flags = [1, 1, 1, 1]
        if 'gate' in row:
            key = row['gate']
            flags[{'no_engine': 0, 'lifecycle': 1, 'status': 2, 'player_none': 3}[key]] = 0
            start, delta = 0, 1
        else: start, delta = row['start'], row['input_delta']
        score = row['after']['stats']['0']['fields'][0]
        lines.append('W ' + ' '.join(map(str, [start, delta, 0, *flags, row['result'], 0, score])))
        counts['writers'] += 1
    for row in report['ranking']:
        values = [3]
        for slot in (0, 3, 15):
            stats = row['before']['stats'][str(slot)]
            flags = 3 if row['case'] == 'suppressed_high' and slot == 0 else 1
            member = 255 if row['case'] == 'no_membership_high' and slot == 0 else slot
            values += [stats['full_player'], stats['fields'][0], flags, member]
        lines.append('R ' + ' '.join(map(str, values + [row['winner']]))); counts['ranking'] += 1
    for row in report['ending']:
        if 'delta' in row:
            end = row['after']['status_c04'] == 2
            lines.append('W ' + ' '.join(map(str, [row['before']['stats']['0']['fields'][0],
                row['delta'], row.get('score_limit', 25), 1, 1, 1, 1, 1, int(end),
                row['after']['stats']['0']['fields'][0]])))
            counts['ending'] += 1
        elif row['case'] == 'two_ordered_score_limit_callbacks':
            values = list(row['order'])
            for slot in (0, 3): values += [row['after']['stats'][str(slot)]['fields'][0]]
            winner = next(e['winner'] for sample in row['samples'] for e in sample['events']
                if e['kind'] == 'original_end_request')
            lines.append('E ' + ' '.join(map(str, values + [int(winner, 16) & 0xffff])))
            counts['ending'] += 1
        elif row['case'].startswith('time_'):
            for sample in row['samples']:
                active = sample['before']['status_c04'] == 1
                end = active and sample['after']['status_c04'] == 2
                lines.append('T ' + ' '.join(map(str, [sample['game_tick'] & 0xffffffff,
                    0, row['rate'] * row['time_limit_seconds'], int(active), int(end),
                    sample['after']['clock_seconds']])))
                counts['clocks'] += 1
        elif row['case'] == 'unlimited_clock':
            lines.append(f'T {row["game_tick"] & 0xffffffff} 0 0 1 0 {row["after"]["clock_seconds"]}'); counts['clocks'] += 1
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(json.dumps(counts)); return
    run = subprocess.run([str(args.binary.resolve()), str(ROOT), str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0, counts=counts,
        native_binary_sha256=digest(args.binary), original_report_sha256=digest(REPORT),
        original_clock_text_report_sha256=digest(CLOCK),
        fixtures_sha256=digest(args.fixtures), tool_sha256=digest(Path(__file__)),
        scope=__doc__, stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)

if __name__ == '__main__': main()
