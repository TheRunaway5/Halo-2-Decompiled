#!/usr/bin/env python3
"""Run real tag-backed multiplayer simulation without rendering/audio devices.

One authoritative world and optional lightweight loopback UDP peers share the
64 MiB aggregate budget. This does not establish rendered/live-match parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
from tag_match_memory_budget import enter_memory_budget


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--shared', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--mode', choices=('ffa', 'team', 'ffa-udp', 'team-udp'), default='ffa')
    parser.add_argument('--players', choices=(4, 16), type=int, default=16)
    parser.add_argument('--peer-seats', choices=(1, 4), type=int,
                        help='Players per UDP peer: defaults to four for 16 players, otherwise one')
    parser.add_argument('--network-profile', choices=('clean', 'loss-reorder'), default='clean',
                        help='Optional bounded downstream UDP loss, two-tick delay and duplication')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=120)
    args = parser.parse_args()
    if args.peer_seats is None:
        args.peer_seats = 4 if args.mode.endswith('-udp') and args.players == 16 else 1
    if args.network_profile != 'clean' and not args.mode.endswith('-udp'):
        parser.error('--network-profile requires a UDP simulation mode')
    if not 10 <= args.timeout <= 600:
        parser.error('--timeout must be between 10 and 600 seconds')
    for path in (args.native, args.map, args.xbe):
        if not path.is_file(): parser.error('Missing input file: ' + str(path))
    contained = enter_memory_budget(args.output, args.timeout)
    if contained is not None: return contained
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    command = [str(args.native.resolve()), str(args.map.resolve()), str(args.shared.resolve()),
               str(args.xbe.resolve()), args.mode, str(output), str(args.players)]
    if args.peer_seats != 1 or args.network_profile != 'clean':
        command.append(str(args.peer_seats))
    if args.network_profile != 'clean':
        command.append(args.network_profile)
    with args.native.open('rb') as source:
        digest = hashlib.file_digest(source, 'sha256').hexdigest()
    started = time.monotonic()
    errors = []
    status = None
    with (output / 'simulation.log').open('w') as log:
        try:
            status = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                    timeout=args.timeout).returncode
        except (OSError, subprocess.TimeoutExpired) as error:
            errors.append(str(error))
    text = (output / 'simulation.log').read_text()
    match = re.search(r'PASS complete headless .* match: (.*)', text)
    metrics = {key: int(value) for key, value in re.findall(r'(\w+)=(\d+)', match.group(1))} if match else {}
    with args.native.open('rb') as source:
        unchanged = hashlib.file_digest(source, 'sha256').hexdigest() == digest
    if status != 0: errors.append('Simulation exited with status ' + str(status))
    if not metrics: errors.append('Strict complete-match simulation checks did not pass')
    if not unchanged: errors.append('Simulation executable changed during execution')
    report = {'scope': __doc__, 'passed': not errors, 'command': command, 'measurements': metrics,
              'elapsed_seconds': round(time.monotonic() - started, 3), 'binary_sha256': digest,
              'errors': errors, 'network_profile': args.network_profile,
              'rendering_tested': False, 'audio_tested': False,
              'independent_client_worlds_tested': False, 'xbox_system_link_tested': False}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2), flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
