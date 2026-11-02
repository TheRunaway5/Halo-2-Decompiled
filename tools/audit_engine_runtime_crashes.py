#!/usr/bin/env python3
"""Inventory bounded, ordinary campaign/MP runtime failures on prepared MCC maps.

Run through tools/run_test.py. This is a diagnostic census, not whole-game
qualification: finite startup frames/ticks cannot establish later gameplay.
No scripts, controls, game outcomes, camera positions or physics results are
supplied. Raw logs/preferences/caches remain in the enclosing owned scratch.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time

from run_test import stop_process_tree
from weapon_test_cache import require_mcc_map


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def interrupted(number, _frame):
    raise SystemExit(128 + number)


def observations(log):
    errors, scripts, boundaries, devices, frames, statuses = [], [], Counter(), set(), [], []
    with log.open(errors='replace') as stream:
        for line in stream:
            line = line.rstrip('\r\n')
            if line.startswith('error: '):
                errors.append(line[:2000])
            if line.startswith('Script fault: '):
                scripts.append(line[:1000])
            if line.startswith('World effect boundary: '):
                boundaries[line[:1000]] += 1
            if line.startswith(('Vulkan device:', 'Frontend GPU:')):
                devices.add(line)
            if match := re.fullmatch(r'Rendered (\d+) frames', line):
                frames.append(int(match[1]))
            if match := re.match(r'Multiplayer frames=(\d+) ticks=(\d+) players=(\d+)', line):
                frames.append(int(match[1]))
            if match := re.search(r'\btick=(\d+) bsp=(\d+).*?actors=(\d+).*?script_faults=(\d+)', line):
                statuses.append(dict(zip(('tick', 'bsp', 'actors', 'script_faults'), map(int, match.groups()))))
    with log.open('rb') as stream:
        stream.seek(max(0, log.stat().st_size - 6000))
        tail = stream.read().decode(errors='replace')
    return dict(errors=errors[-8:], script_faults=scripts[-12:],
                script_fault_count=len(scripts), world_effect_boundary_count=sum(boundaries.values()),
                world_effect_boundaries=[dict(message=k, count=v) for k, v in boundaries.most_common(16)],
                devices=sorted(devices), rendered_frames=frames, source_status=statuses[-8:],
                log_bytes=log.stat().st_size, log_sha256=sha(log), diagnostic_tail=tail)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--cache', type=Path, required=True)
    parser.add_argument('--kind', choices=('campaign', 'multiplayer'), required=True)
    parser.add_argument('--mode', choices=('render', 'simulate'), default='render')
    parser.add_argument('--map', action='append', help='Exact .map basename; repeat or omit for the content census')
    parser.add_argument('--frames', type=int, default=2)
    parser.add_argument('--ticks', type=int, default=30)
    parser.add_argument('--players', type=int, default=1)
    parser.add_argument('--graphics-mode', choices=('Original', 'pStencil', 'PathTracing'), default='Original')
    parser.add_argument('--resize-test', action='store_true', help='Campaign SDL resize at its existing frame2 boundary')
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run through tools/run_test.py')
    if (not 1 <= args.frames <= 1800 or not 1 <= args.ticks <= 7200 or
            not 1 <= args.timeout <= 900 or not 1 <= args.players <= 4):
        parser.error('Declare finite frames/ticks/timeout and 1..4 local players')
    if args.kind == 'multiplayer' and args.mode == 'simulate':
        parser.error('MP requires actual submitted frames')
    if args.resize_test and (args.kind != 'campaign' or args.mode != 'render' or args.frames < 4):
        parser.error('The existing resize control requires campaign rendering and at least4 frames')
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    runtime = args.runtime.resolve(strict=True)
    cache = args.cache.resolve(strict=True)
    maps = cache/'halo2/h2_maps_win64_dx11'
    names = args.map
    if names is None:
        # Selection convention is explicit; the actual engine then admits the
        # scenario and its authored data. This does not infer tag contents.
        names = [p.name for p in sorted(maps.glob('*.map'))
                 if p.name not in ('mainmenu.map', 'shared.map', 'single_player_shared.map')
                 and bool(re.match(r'^\d\d[a-z]_', p.name)) == (args.kind == 'campaign')]
    if not names or len(names) != len(set(names)):
        parser.error('Require a nonempty set of unique maps')
    for name in names:
        if Path(name).name != name or not name.endswith('.map'):
            parser.error('Use exact map basenames')
        require_mcc_map(maps/name)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    report = dict(completed=False, scope=__doc__, kind=args.kind, mode=args.mode,
                  runtime=str(runtime), runtime_sha256=sha(runtime), cache=str(cache),
                  helper_sha256=sha(__file__), graphics_mode=args.graphics_mode, resize_test=args.resize_test,
                  names=names, cases=[], scratch=str(scratch),
                  supplied_engine_outcomes=False, supplied_input=False,
                  classification='Rendered/ticked startup census; no later gameplay or whole-game crash-free claim',
                  retained_large_artifacts=[])

    def save():
        temporary = args.report.with_suffix('.json.tmp')
        temporary.write_text(json.dumps(report, indent=2)+'\n')
        temporary.replace(args.report)

    previous = {number: signal.signal(number, interrupted) for number in (signal.SIGINT, signal.SIGTERM)}
    try:
        save()
        for index, name in enumerate(names):
            owned = scratch/f'{args.kind}-{args.mode}-{index}-{Path(name).stem}'
            owned.mkdir()
            env = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_')}
            env.update(HALO2_TEST_SCRATCH=str(owned), XDG_CONFIG_HOME=str(owned/'config'),
                       XDG_DATA_HOME=str(owned/'data'), XDG_CACHE_HOME=str(owned/'cache'),
                       HALO2_PIPELINE_CACHE_DIRECTORY=str(owned/'pipelines'),
                       MESA_SHADER_CACHE_DIR=str(owned/'mesa'),
                       __GL_SHADER_DISK_CACHE_PATH=str(owned/'nvidia'), SDL_AUDIODRIVER='dummy')
            command = [str(runtime), '--'+args.kind, '--map', str(maps/name),
                       '--shared-maps', str(maps), '--mute']
            if args.kind == 'multiplayer':
                command += ['--local-players', str(args.players)]
            if args.mode == 'render':
                command += ['--frames', str(args.frames), '--hidden', '--viewport', '640', '360',
                            '--graphics-mode', args.graphics_mode]
                if args.resize_test:
                    command += ['--resize-test']
            else:
                command += ['--simulate', str(args.ticks)]
            log = owned/'native.log'
            started = time.monotonic()
            timed_out = False
            with log.open('wb') as stream:
                process = subprocess.Popen(command, cwd=root, env=env, stdout=stream,
                                           stderr=subprocess.STDOUT, start_new_session=True)
                try:
                    try:
                        process.wait(timeout=args.timeout)
                    except subprocess.TimeoutExpired:
                        timed_out = True
                finally:
                    stop_process_tree(process)
            observed = observations(log)
            if timed_out:
                outcome = 'timeout'
            elif process.returncode < 0:
                outcome = 'signal'
            elif observed['errors']:
                outcome = 'runtime_error'
            elif observed['script_fault_count']:
                outcome = 'script_fault'
            elif process.returncode != 0:
                outcome = 'nonzero_exit'
            elif args.mode == 'render' and sum(observed['rendered_frames']) != args.frames:
                outcome = 'no_frame_submission'
            elif args.mode == 'simulate' and (not observed['source_status'] or
                    observed['source_status'][-1]['tick'] != args.ticks):
                outcome = 'early_exit'
            else:
                outcome = 'completed'
            row = dict(map=name, command=command, cwd=str(root), exit_code=process.returncode,
                       timeout=timed_out, seconds=round(time.monotonic()-started, 3),
                       map_sha256=sha(maps/name), outcome=outcome, **observed)
            report['cases'].append(row)
            save()
            print(json.dumps({key: row[key] for key in ('map', 'exit_code', 'seconds', 'outcome', 'errors')}), flush=True)
        report.update(completed=True, runtime_unchanged=sha(runtime) == report['runtime_sha256'],
                      outcomes=dict(Counter(case['outcome'] for case in report['cases'])))
        save()
    finally:
        for number, handler in previous.items():
            signal.signal(number, handler)
    # A census with observed failures is complete evidence; a failure does not
    # stop later content families. Outcomes remain explicitly classified.
    return 0 if report['completed'] and report['runtime_unchanged'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
