#!/usr/bin/env python3
"""Bounded public acceptance probe; never reads player identities or credentials."""
import argparse
import datetime
import json
import time
import urllib.request
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--origin', default='https://master.halo2forever.net')
    parser.add_argument('--phase', choices=('idle', 'queued', 'joining', 'playing', 'postgame'), required=True)
    parser.add_argument('--users', type=int, required=True)
    parser.add_argument('--timeout', type=float, default=75)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not 1 <= args.users <= 16 or not 1 <= args.timeout <= 600:
        parser.error('Use one to sixteen users and a one to 600 second bound')
    started = time.monotonic()
    observations = []
    passed = False
    while time.monotonic() - started < args.timeout:
        try:
            with urllib.request.urlopen(args.origin.rstrip('/') + '/v1/online/status', timeout=10) as response:
                status = json.load(response)
            counts = {phase: int(status[phase]['users']) for phase in
                      ('online', 'idle', 'queued', 'joining', 'playing', 'postgame')}
            observation = dict(seconds=round(time.monotonic() - started, 2), **counts)
            if not observations or any(observations[-1].get(k) != v for k, v in counts.items()):
                observations.append(observation)
                print(json.dumps(observation), flush=True)
            passed = counts[args.phase] >= args.users
            if passed:
                break
        except (OSError, ValueError, KeyError, TypeError):
            observations.append(dict(seconds=round(time.monotonic() - started, 2), error='Population unavailable'))
        time.sleep(min(2, max(0, args.timeout - (time.monotonic() - started))))
    report = dict(passed=passed, expected_phase=args.phase, expected_users=args.users,
                  verified_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  observations=observations,
                  limits='Aggregate activity only; does not prove rendering, EOS routing, audio or result publication.')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
