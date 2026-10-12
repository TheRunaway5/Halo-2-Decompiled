#!/usr/bin/env python3
"""Measure process start to the first captured native title/movie/match frame.

Runs serially on the real SDL/Vulkan renderer. These are process-cold runs;
OS file caches and driver caches are deliberately left alone. Intro playback
is sampled at its first frame, not timed through its authored duration.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import selectors
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def run(runtime, output, scenario, repetition, timeout, validation, mute):
    name = f'{scenario}-{repetition:02d}'
    capture = output / f'{name}.ppm'
    capture.unlink(missing_ok=True)
    if scenario in ('title', 'boot'):
        options = ['--menu-screen', '9', '--menu-ui-assets',
                   str(ROOT / 'assets/menu-ui/vista'), '--menu-tick', '0', '--freeze-menu']
        options += ['--skip-intro'] if scenario == 'title' else ['--movie-frame', '0']
    else:
        options = ['--multiplayer', '--map', str(ROOT / 'assets/disc/maps/lockout.map'),
                   '--mp-starting-weapon', '5', '--local-players',
                   '4' if scenario == 'split' else '1']
    command = ['stdbuf', '-oL', '-eL', str(runtime), *options, '--hidden',
               '--frames', '1', '--screenshot', str(capture)]
    if mute:
        command.append('--mute')
    if validation:
        command.append('--validation')
    started = time.monotonic()
    ready = None
    stages = []
    environment = os.environ.copy()
    environment.setdefault('HALO2_MENU_FOG_SEED', '1')
    process = subprocess.Popen(command, cwd=ROOT, env=environment, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, bufsize=0)
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    pending = b''
    try:
        with (output / f'{name}.log').open('w') as log:
            while selector.get_map():
                remaining = timeout - (time.monotonic() - started)
                if remaining <= 0:
                    raise RuntimeError(f'{name}: timed out after {timeout}s')
                for key, _ in selector.select(min(remaining, .25)):
                    chunk = os.read(key.fd, 65536)
                    if not chunk:
                        selector.unregister(key.fileobj)
                        continue
                    pending += chunk
                    while b'\n' in pending:
                        line, pending = pending.split(b'\n', 1)
                        text = line.decode('utf-8', errors='replace')
                        elapsed = time.monotonic() - started
                        log.write(f'{elapsed:.6f} {text}\n')
                        log.flush()
                        if text.startswith('Captured '):
                            ready = elapsed
                        if any(marker in text for marker in
                               ('Retail frontend:', 'Frontend GPU:', 'Native Slayer:',
                                'Multiplayer GPU:', 'Retail movie:')):
                            stages.append({'seconds': elapsed, 'message': text})
                        if 'Vulkan validation:' in text:
                            raise RuntimeError(f'{name}: {text}')
            if pending:
                log.write(pending.decode('utf-8', errors='replace'))
        returncode = process.wait(timeout=max(.1, timeout - (time.monotonic() - started)))
        if returncode or ready is None or not capture.is_file():
            raise RuntimeError(f'{name}: no successful first frame (exit {returncode}); see {name}.log')
        data = capture.read_bytes()
        magic, dimensions, maximum, pixels = data.split(b'\n', 3)
        width, height = map(int, dimensions.split())
        if magic != b'P6' or maximum != b'255' or len(pixels) != width * height * 3:
            raise RuntimeError(f'{name}: incomplete screenshot')
        print(f'{name}: first_frame={ready:.3f}s', flush=True)
        return {'first_frame_seconds': ready, 'total_seconds': time.monotonic() - started,
                'capture': str(capture), 'sha256': hashlib.sha256(data).hexdigest(),
                'stages': stages}
    finally:
        selector.close()
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        process.stdout.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scenario', choices=('title', 'boot', 'match', 'split'), action='append')
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--validation', action='store_true')
    parser.add_argument('--audio', action='store_true', help='Include real audio startup/decoding')
    parser.add_argument('--compare', type=Path, help='Earlier report.json to compare')
    parser.add_argument('--max-ratio', type=float, help='Fail if a median exceeds this baseline ratio')
    args = parser.parse_args()
    if args.repetitions < 1 or args.timeout <= 0 or (args.max_ratio and not args.compare):
        parser.error('Use positive repetitions/timeout; --max-ratio needs --compare')
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    runtime = args.runtime.resolve()
    report = {'runtime': str(runtime), 'runtime_sha256': hashlib.sha256(runtime.read_bytes()).hexdigest(),
              'audio': args.audio, 'validation': args.validation,
              'menu_fog_seed': os.environ.get('HALO2_MENU_FOG_SEED', '1'),
              'cache_policy': 'Fresh processes; OS/driver caches retained', 'scenarios': {}}
    failed = False
    try:
        for scenario in args.scenario or ('title', 'match'):
            runs = [run(runtime, args.output, scenario, n, args.timeout,
                        args.validation, not args.audio) for n in range(1, args.repetitions + 1)]
            report['scenarios'][scenario] = {
                'median_first_frame_seconds': statistics.median(r['first_frame_seconds'] for r in runs),
                'runs': runs}
        if args.compare:
            baseline = json.loads(args.compare.read_text())
            if (baseline['audio'], baseline['validation']) != (args.audio, args.validation):
                raise RuntimeError('Comparison must use the same audio/validation settings')
            for scenario, result in report['scenarios'].items():
                ratio = result['median_first_frame_seconds'] / baseline['scenarios'][scenario]['median_first_frame_seconds']
                result['baseline_ratio'] = ratio
                print(f'{scenario}: {100 * (1 - ratio):.1f}% faster than baseline', flush=True)
                failed |= args.max_ratio is not None and ratio > args.max_ratio
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        report['error'] = str(error)
        failed = True
        print(error, flush=True)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
