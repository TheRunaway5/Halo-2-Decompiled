#!/usr/bin/env python3
"""Compare native incoming-owner mutation with independently frozen original CBA60 execution."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'analysis/multiplayer/integration/spawn-source'
REPORT = BASE / 'owner-counter-report.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def word(value):
    return f'{value & 0xffffffff:08x}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=ROOT / 'build-multiplayer/owner_counter_tests')
    parser.add_argument('--fixtures', type=Path, default=BASE.parent / 'owner-counter-fixtures.tsv')
    parser.add_argument('--output', type=Path, default=BASE.parent / 'owner-counter-differential.json')
    parser.add_argument('--export-only', action='store_true')
    args = parser.parse_args()
    if ROOT / 'analysis' not in args.fixtures.resolve().parents:
        raise ValueError('Original-data fixtures must remain ignored')
    report = json.loads(REPORT.read_text())
    pins = {
        REPORT: 'e7492035911eb502c77b5154b3a35a9ad3c8decac254f3c5f18758d9a0d56dac',
        ROOT / 'tools/oracle_mp_spawn_owner_counter.py': '349577d11c85e23895946c9591e38aebad26af794240a5ede97127767d430dba',
        BASE / 'tu11-owner-counter-instructions.txt': '6d31cacba708122f3a4abe43284acffb5e96b5f2c34810fecb9302bcc657c31f',
        ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03': report['source_sha256'],
        ROOT / 'assets/disc/maps/lockout.map': report['cache_sha256'],
    }
    pins.update({ROOT / path: pin for path, pin in report['dependencies'].items()})
    pins.update({BASE / path: pin for path, pin in report['frozen_reports'].items()})
    for path, pin in pins.items():
        if digest(path) != pin:
            raise ValueError(f'Frozen incoming-owner authority changed: {path}')
    lines = []
    for group in ('admission_cases', 'ownership_cases', 'timing_cases', 'cap_cases'):
        for case in report[group]:
            request = case['request']; before = request['before']; after = request['after']; inputs = request['input']
            victim = next(unit for unit in before['units'].values() if unit['datum'] == inputs['victim'])
            fields = [case['case'], before['tick'], inputs['attacker'], inputs['owner'], before['seconds_per_tick'],
                      word(inputs['damage_class_short']), word(inputs['designator_short']), word(victim['membership']),
                      word(inputs['lethal_byte']), word(before['ai_enabled']), word(len(before['players']))]
            for _, player in sorted(before['players'].items(), key=lambda item: int(item[0])):
                fields.extend([player['full'], player['current']])
            # Real headers select their actual body pointer. A released old body
            # may remain in the oracle arena but is not that index's current body.
            headers = {}
            for name, unit in before['units'].items():
                index = int(unit['datum'], 16) & 0xffff
                current = headers.get(index)
                if current is None or unit['header_body'] == unit['body']:
                    headers[index] = (name, unit)
            ordered = sorted(headers.items())
            fields.append(word(len(ordered)))
            names = []
            for index, (name, unit) in ordered:
                names.append(name); expected = after['units'][name]
                fields.extend([word((int(unit['header_salt'], 16) << 16) | index), unit['owner'],
                    unit['forward_9'], unit['forward_other'], word(unit['header_type']),
                    word(int(unit['header_body'], 16) != 0), word(unit['dead']), word(unit['counter']), unit['last_tick'],
                    word(expected['counter']), expected['last_tick']])
            recipient = case.get('expected_recipient', 'attacker0')
            fields.append(word(names.index(recipient) if recipient is not None else 0xffffffff))
            lines.append(' '.join(fields))
    args.fixtures.write_text('\n'.join(lines) + '\n')
    if args.export_only:
        print(f'Exported {len(lines)} original owner-counter cases'); return
    run = subprocess.run([str(args.binary.resolve()), str(args.fixtures.resolve())], text=True, capture_output=True)
    print(run.stdout, end=''); print(run.stderr, end='')
    args.output.write_text(json.dumps(dict(passed=run.returncode == 0, scope=__doc__, cases=len(lines),
        original_report_cases=report['case_count'], source_report_sha256=digest(REPORT),
        native_binary_sha256=digest(args.binary), fixture_sha256=digest(args.fixtures),
        tool_sha256=digest(Path(__file__)), stdout=run.stdout, stderr=run.stderr), indent=2) + '\n')
    raise SystemExit(run.returncode)


if __name__ == '__main__': main()
