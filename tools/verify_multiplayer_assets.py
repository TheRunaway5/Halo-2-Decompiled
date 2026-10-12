#!/usr/bin/env python3
"""Capture base and Map Pack maps using local original shared resources.

This checks asset admission and rendered startup with the recovered SMG variant
option. It does not verify complete sandbox, spawn scoring or visual parity.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from inspect_multiplayer import MAPS

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / 'build-multiplayer/halo2_native')
    parser.add_argument('--maps', type=Path, default=ROOT / 'assets/disc/maps')
    parser.add_argument('--map-pack', type=Path, default=ROOT / 'assets/multiplayer-map-pack')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/multiplayer/integration/maps')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    runtime, maps = args.runtime.resolve(), args.maps.resolve()
    cases = [('base', maps / (name + '.map')) for name in MAPS]
    manifest_path = args.map_pack / 'disc-manifest.json'
    manifest = json.loads(manifest_path.read_text())
    cases.extend(('map-pack', args.map_pack / item['path'])
                 for item in manifest['files']
                 if item['path'].startswith('demos/dlc/') and item['path'].endswith('.map'))
    report = {
        'runtime': str(runtime), 'runtime_sha256': digest(runtime),
        'map_pack_manifest_sha256': digest(manifest_path),
        'shared_directory': str(maps),
        'original_executable_sha256': digest(maps.parent / 'default.xbe'),
        'scope': 'Actual four-view rendered startup with original assets and authored SMG option 5. '
                 'No complete-match, sandbox, spawn-weighting or visual parity claim.',
        'cases': [],
    }
    env = os.environ.copy()
    env.setdefault('DISPLAY', ':0')
    for source, path in cases:
        name = source + '-' + path.stem
        capture = (args.output / (name + '.ppm')).resolve()
        log = args.output / (name + '.log')
        command = [str(runtime), '--multiplayer', '--map', str(path.resolve()),
                   '--shared-maps', str(maps), '--local-players', '4',
                   '--mp-starting-weapon', '5', '--hidden', '--mute', '--frames', '6',
                   '--validation', '--screenshot', str(capture)]
        try:
            with log.open('w') as stream:
                result = subprocess.run(command, cwd=ROOT, env=env, stdout=stream,
                                        stderr=subprocess.STDOUT, timeout=35)
            output = log.read_text()
            passed = (result.returncode == 0 and capture.is_file()
                      and 'players=4 connected=1' in output
                      and 'Validation Error' not in output and 'VUID-' not in output)
            row = dict(source=source, map=str(path.resolve()), map_sha256=digest(path),
                       command=command, exit_code=result.returncode, passed=passed,
                       capture=str(capture) if capture.exists() else None, log=str(log.resolve()),
                       unresolved=[line for line in output.splitlines()
                                   if 'unresolved' in line.lower() or line.startswith('error:')])
        except subprocess.TimeoutExpired:
            row = dict(source=source, map=str(path.resolve()), passed=False, timeout=True,
                       log=str(log.resolve()))
        report['cases'].append(row)
        print(name + ': ' + ('rendered' if row['passed'] else 'FAILED'), flush=True)
        report['passed'] = all(row['passed'] for row in report['cases'])
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
