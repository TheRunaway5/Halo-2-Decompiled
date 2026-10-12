#!/usr/bin/env python3
"""Validate retained-object scheduling against frozen original TU1.1 execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/spawn-source'
REPORT = BASE / 'corpse-schedule-report.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f'{value & 0xffffffff:08x}'


def body(state):
    fields = state['body_fields']
    return [fields['004'], fields['01c'], fields['020']]


def queue(state):
    return [state['garbage_head'], word(state['garbage_count'])]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/object_garbage_tests')
    parser.add_argument('--fixtures', type=Path, default=BASE.parent / 'object-garbage-fixtures.tsv')
    parser.add_argument('--output', type=Path, default=BASE.parent / 'corpse-schedule-differential.json')
    parser.add_argument('--export-only', action='store_true')
    args = parser.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Original-data fixtures must remain ignored')
    report = json.loads(REPORT.read_text())
    pins = {
        REPORT: '649a0761e442ff5698476a18d4fcef36ebe89d4431ecb8619de497a46e357f6d',
        ROOT / 'tools/oracle_mp_spawn_corpse_schedule.py': '4ab8290691690a7bf92d3e8752521a249bb241e34515bbbc985f0a7628e7a68e',
        BASE / 'tu11-corpse-schedule-instructions.txt': 'ea28b160b7f5bb3bb8d7508684fe279fa9042792852f9df0740d08060aee3626',
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': report['source_sha256'],
        ROOT / 'assets/disc/maps/lockout.map': report['cache_sha256'],
    }
    pins.update({ROOT / path: pin for path, pin in report['dependencies'].items()})
    pins.update({BASE / path: pin for path, pin in report['frozen_reports'].items()})
    for path, pin in pins.items():
        if digest(path) != pin:
            raise ValueError(f'Frozen corpse authority changed: {path}')
    lines = []
    for case in report['garbage_schedule_cases']:
        before = case['before']
        for phase, enable, interval in [('first_enable', 1, 300), ('shorter_enable', 1, 10), ('after_disable', 0, -1)]:
            after = case[phase]
            lines.append(' '.join(['S', case['case'] + '-' + phase, before['datum'],
                word(before['header_flags'] & 1), word(before['tick']), word(enable), word(interval),
                *body(before), *queue(before), *body(after), *queue(after)]))
            before = after
    for case in report['collector_cases']:
        inputs = case['explicit_inputs']; before = case['before']
        levels = [entry['level'] for entry in case['source_trace'] if entry['stage'] == 'candidate_consumer']
        actual_level = levels[0] if levels else -1
        lines.append(' '.join(['L', case['case'], word(inputs['request_byte']), word(inputs['force_byte']),
            word(inputs['heap_remaining']), word(before['object_count']), word(before['garbage_count']), word(actual_level)]))
        # A supplied nearby service is original fixture input, not a computed
        # pose. Preserve that boundary rather than exporting a native result.
        if inputs['nearby_service'] is not None:
            lines.append(' '.join(['C', case['case'], word(actual_level), word(inputs['tick']),
                word(before['header_flags'] & 1), word(inputs['nearby_service']),
                word(case['delete_preparation_requested']), *body(before)]))
    for case in report['pressure_cases']:
        inputs = case['explicit_inputs']
        lines.append(' '.join(['P', case['case'], word(inputs['level']), word(inputs['remaining_heap_bytes']),
            word(inputs['free_highwater_headers']), word(inputs['signed_active_garbage_count']),
            word(case['pressure']), word(case['critical'])]))
    for case in report['scheduling_cases']:
        if 'varied_options' not in case:
            continue
        inputs = case['varied_options']; after = case['after']
        lines.append(' '.join(['D', case['case'], word(after['tick']), word(inputs['respawn_seconds'] * 30),
            word(inputs['suicide_penalty_seconds'] * 30 if inputs['suicide'] else 0),
            word(after['player_countdown']), word(after['tick'])]))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(f'Exported {len(lines)} original schedule/pressure/writer rows'); return
    run = subprocess.run([str(args.binary.resolve()), str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0, scope=__doc__,
        rows=len(lines), source_report_sha256=digest(REPORT), native_binary_sha256=digest(args.binary),
        fixture_sha256=digest(args.fixtures), tool_sha256=digest(Path(__file__)),
        stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__': main()
