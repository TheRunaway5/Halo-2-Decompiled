#!/usr/bin/env python3
"""Run a full visible local match through ordinary SDL controller input.

Requires original assets and an active display. The native driver checks actual
combat, salted respawn, rendering/audio and the complete results countdown.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
from tag_match_memory_budget import enter_memory_budget

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--shared', type=Path, default=ROOT / 'assets/disc/maps')
    parser.add_argument('--xbe', type=Path,
                        default=ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    parser.add_argument('--mode', choices=('ffa', 'team'), default='ffa')
    parser.add_argument('--players', type=int, choices=(2, 4), default=2)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=240)
    args = parser.parse_args()
    if not 30 <= args.timeout <= 3600:
        parser.error('--timeout must be between 30 and 3600 seconds')
    binary = args.native.resolve()
    if not binary.is_file():
        parser.error('build tag_gameplay_match_tests first')
    contained = enter_memory_budget(args.output, args.timeout)
    if contained is not None:
        return contained
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with binary.open('rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    command = [str(binary), str(args.map.resolve()), str(args.shared.resolve()),
               str(args.xbe.resolve()), args.mode, str(output)]
    if args.players != 2:
        command.append(str(args.players))
    started = time.monotonic()
    errors = []
    log = output / 'runtime.log'
    with log.open('w') as stream:
        try:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                    timeout=args.timeout)
            status = result.returncode
        except subprocess.TimeoutExpired:
            status = None
            errors.append('Full local match exceeded its deadline')
    text = log.read_text()
    metrics = {}
    match = re.search(r'PASS complete live .* match: (.*)', text)
    if match:
        for key, value in re.findall(r'(\w+)=(-?\d+(?:,-?\d+)*)', match.group(1)):
            metrics[key] = [int(x) for x in value.split(',')] if ',' in value else int(value)
    if status != 0:
        errors.append(f'Native driver failed with exit status {status}; see {log}')
    for line in text.splitlines():
        if line.startswith('Native gait/footstep owner:'):
            errors.append(line)
    with binary.open('rb') as source:
        final_digest = hashlib.file_digest(source, 'sha256').hexdigest()
    if final_digest != digest:
        errors.append('Test executable changed during execution')
    if not metrics:
        errors.append('Strict full-match combat/results checks did not pass')
    if args.players == 4 and metrics.get('players') != 4:
        errors.append('Four-player participant checks did not pass')
    report = dict(scope=__doc__, command=command, binary_sha256=digest,
                  exit_status=status, elapsed_seconds=round(time.monotonic() - started, 3),
                  measurements=metrics, errors=errors, passed=not errors)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
