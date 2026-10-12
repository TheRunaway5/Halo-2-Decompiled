#!/usr/bin/env python3
"""Run existing rendered local/LAN sound-delivery checks against a retained binary.

This verifies actual SDL submissions and event delivery. It does not compare
spatial mixing, all game events, physical speakers or Xbox waveforms.
Run inside an isolated Xvfb with SDL_AUDIODRIVER=dummy.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--shared', type=Path, required=True)
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--timeout', type=float, default=120)
    parser.add_argument('--cases', nargs='+', choices=('local1','local4','lan1','lan4','lan-slow'))
    args = parser.parse_args()
    assert os.environ.get('DISPLAY') and os.environ.get('SDL_AUDIODRIVER') == 'dummy'
    args.output.mkdir(parents=True, exist_ok=False)
    binary = (args.output / 'multiplayer_audio_runtime_tests.bin').resolve()
    shutil.copy2(args.binary, binary)
    cases = []
    for name, extra in (('local1', []), ('local4', ['--split4']),
                        ('lan1', ['--lan']), ('lan4', ['--lan', '--split4']),
                        ('lan-slow', ['--lan', '--slow-client'])):
        if args.cases and name not in args.cases:
            continue
        command = list(map(str, [binary, args.map.resolve(), args.shared.resolve(),
                                args.image.resolve(), *extra]))
        started = time.monotonic()
        timed_out = False
        with (args.output / f'{name}.stdout').open('w') as stdout, \
                (args.output / f'{name}.stderr').open('w') as stderr:
            try:
                result = subprocess.run(command, stdout=stdout, stderr=stderr, timeout=args.timeout)
                returncode = result.returncode
            except subprocess.TimeoutExpired:
                timed_out, returncode = True, None
        cases.append(dict(name=name, command=command, returncode=returncode, timed_out=timed_out,
                          seconds=time.monotonic() - started))
        print(name, 'timeout' if timed_out else returncode, flush=True)
        report = dict(boundary=__doc__, binary_sha256=digest(binary),
                      map_sha256=digest(args.map), image_sha256=digest(args.image),
                      tool_sha256=digest(Path(__file__)), cases=cases)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    assert all(c['returncode'] == 0 for c in cases), cases


if __name__ == '__main__':
    main()
