#!/usr/bin/env python3
"""Exercise each authored campaign startup and retain its real runtime failures.

This bounded script test does not establish combat, mission completion, visual
parity, controller behavior, or a full campaign playthrough.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
MAPS = (
    '00a_introduction', '01a_tutorial', '01b_spacestation',
    '03a_oldmombasa', '03b_newmombasa', '04a_gasgiant', '04b_floodlab',
    '05a_deltaapproach', '05b_deltatowers', '06a_sentinelwalls',
    '06b_floodzone', '07a_highcharity', '07b_forerunnership',
    '08a_deltacliffs', '08b_deltacontrol',
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, default=ROOT / 'build-campaign/halo2_native')
    parser.add_argument('--maps', type=Path, default=ROOT / 'assets/disc/maps')
    parser.add_argument('--ticks', type=int, default=60)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/campaign/startup-latest.json')
    args = parser.parse_args()
    if args.ticks < 1 or args.ticks > 100000:
        parser.error('--ticks must be between 1 and 100000')
    native = args.native.resolve()
    if not native.is_file():
        parser.error('build the native executable first')
    rows = []
    for name in MAPS:
        path = args.maps.resolve() / (name + '.map')
        result = subprocess.run(
            [str(native), '--map', str(path), '--campaign', '--simulate', str(args.ticks)],
            cwd=ROOT, capture_output=True, text=True, timeout=120)
        faults = [line.removeprefix('Script fault: ')
                  for line in result.stderr.splitlines() if line.startswith('Script fault: ')]
        rows.append(dict(map=path.name, exit_status=result.returncode,
                         map_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                         script_faults=faults, stdout=result.stdout, stderr=result.stderr))
        print(f'{name}: ' + ('; '.join(faults) or result.stderr.strip() or 'startup window passed'))
    failed = sum(row['exit_status'] != 0 for row in rows)
    report = dict(scope='authored startup only; not mission or campaign completion',
                  ticks=args.ticks, native=str(native),
                  native_sha256=hashlib.sha256(native.read_bytes()).hexdigest(),
                  maps=len(rows), failed=failed, results=rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{failed}/{len(rows)} startup paths blocked; report: {args.output.resolve()}')
    return int(bool(failed))


if __name__ == '__main__':
    raise SystemExit(main())
