#!/usr/bin/env python3
"""Measure steady rendered frame intervals, including presentation waits.

Uses the ordinary SDL/Vulkan runtime, with 30 warmup frames by default. Keep
windows visible for playback measurements: hidden Wayland surfaces and Xvfb
can introduce unrelated presentation throttling. No preferences are written.
"""
import argparse
import csv
import hashlib
import json
import os
import re
from pathlib import Path
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[1]



def registered_vista_assets(root):
    """Resolve the same explicit Vista selection as launcher.cpp::read_sources."""
    registry = root / 'assets/asset-sources.tsv'
    # Preserve CR and NUL so malformed registry rows cannot be silently accepted.
    with registry.open(newline='') as stream:
        lines = stream.read().split('\n')
    if lines and lines[-1] == '':
        lines.pop()
    if not lines or lines[0] != 'H2_ASSET_SOURCES_V1':
        raise ValueError(f'Invalid launcher asset registry: {registry}')
    sources = {}
    for line in lines[1:]:
        if line.count('\t') != 1 or any(char in line for char in ('\r', '\0')):
            raise ValueError(f'Invalid launcher asset registry row: {registry}')
        name, value = line.split('\t')
        if name not in ('xbox', 'vista') or name in sources or not value:
            raise ValueError(f'Invalid launcher asset registry selection: {registry}')
        path = Path(value)
        sources[name] = path if path.is_absolute() else root / path
    if 'vista' not in sources:
        raise ValueError(f'No Vista UI pack registered in {registry}; use --ui-assets')
    return sources['vista']


def measure(args, scenario, repetition):
    stem = args.output / f'{scenario}-{repetition:02d}'
    timing = stem.with_suffix('.tsv')
    timing.unlink(missing_ok=True)
    if scenario in ('title', 'profile'):
        options = ['--skip-intro', '--menu-screen', '9' if scenario == 'title' else '47',
                   '--menu-ui-assets', str(args.ui_assets),
                   '--menu-tick', '181', '--freeze-menu']
    else:
        options = ['--multiplayer', '--map', str(args.map),
                   '--mp-starting-weapon', str(args.starting_weapon), '--local-players',
                   str(args.local_players) if scenario == 'split' else '1']
        if args.shared_maps:
            options += ['--shared-maps', str(args.shared_maps)]
        if args.ui_assets:
            options += ['--hud-ui-assets', str(args.ui_assets)]
    if args.gameplay_xbe:
        options += ['--gameplay-xbe', str(args.gameplay_xbe)]
    command = [str(args.runtime), *options, '--frames', str(args.frames),
               '--viewport', str(args.width), str(args.height)]
    if args.render_size:
        command += ['--render-size', *map(str,args.render_size)]
    if args.profile_multiplayer:
        command = [str(args.runtime), str(args.map),
                   str(args.shared_maps or args.map.parent), str(args.gameplay_xbe),
                   '4' if scenario == 'split' else '1', str(args.frames), str(args.width),
                   str(stem.with_suffix('.cpu.tsv')), 'resident', str(args.ui_assets)]
    elif not args.audio:
        command.append('--mute')
    if args.hidden:
        command.append('--hidden')
    environment = os.environ.copy()
    environment['HALO2_RENDER_TIMINGS'] = str(timing)
    environment['HALO2_MENU_FOG_SEED'] = '1'
    environment['SDL_WINDOW_NO_ACTIVATION_WHEN_SHOWN'] = '1'
    environment['SDL_WINDOW_ACTIVATE_WHEN_SHOWN'] = '0'
    environment['SDL_WINDOW_ACTIVATE_WHEN_RAISED'] = '0'
    environment['SDL_GAMECONTROLLER_IGNORE_DEVICES_EXCEPT'] = '0x0000/0x0000'
    if os.environ.get('HALO2_TEST_SCRATCH'):
        scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
        environment.update(HALO2_PIPELINE_CACHE_DIRECTORY=str(scratch / 'vulkan-cache'),
                           MESA_SHADER_CACHE_DIR=str(scratch / 'mesa-cache'),
                           __GL_SHADER_DISK_CACHE_PATH=str(scratch / 'nvidia-cache'))
    with stem.with_suffix('.log').open('w') as log:
        result = subprocess.run(command, cwd=ROOT, env=environment,
                                stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout)
    if result.returncode:
        raise RuntimeError(f'{scenario}: exit {result.returncode}; see {stem}.log')
    if scenario in ('title', 'profile') and re.search(r'^Menu screen ', stem.with_suffix('.log').read_text(), re.MULTILINE):
        raise RuntimeError(f'{scenario}: external input changed the sampled screen; repeat this run')
    with timing.open() as stream:
        rows = list(csv.DictReader(stream, delimiter='\t'))
    if len(rows) != args.frames:
        raise RuntimeError(f'{scenario}: expected {args.frames} submitted timing rows, got {len(rows)}')
    rows = rows[max(1, args.warmup):]
    phases = {}
    for name in rows[0]:
        if not name.endswith('_ms'):
            continue
        values = sorted(float(row[name]) for row in rows)
        phases[name] = {'mean': statistics.mean(values), 'median': statistics.median(values),
                        'p95': values[int(.95 * (len(values) - 1))]}
    dimensions = {}
    for kind in ('render', 'present'):
        if f'{kind}_width' in rows[0]:
            sizes = sorted({(int(row[f'{kind}_width']), int(row[f'{kind}_height'])) for row in rows})
            if len(sizes) != 1:
                raise RuntimeError(f'{scenario}: {kind} dimensions changed during sampling: {sizes}')
            dimensions[kind] = sizes[0]
    expected_render=tuple(args.render_size or (args.width,args.height))
    if scenario in ('match','split') and dimensions.get('render') != expected_render:
        raise RuntimeError(f'{scenario}: internal render size differs from requested workload: {dimensions}')
    fps = 1000 / phases['interval_ms']['mean']
    intervals = sorted(float(row['interval_ms']) for row in rows)
    frame_budget = None
    if args.max_frame_ms:
        misses = [float(row['interval_ms']) > args.max_frame_ms for row in rows]
        longest = consecutive = 0
        for missed in misses:
            consecutive = consecutive + 1 if missed else 0
            longest = max(longest, consecutive)
        frame_budget = {'limit_ms': args.max_frame_ms, 'missed_frames': sum(misses),
                        'sampled_frames': len(rows), 'longest_miss_streak': longest,
                        'p99_ms': intervals[int(.99 * (len(intervals) - 1))],
                        'max_ms': intervals[-1], 'passed': not any(misses)}
    print(f'{scenario}-{repetition:02d}: {fps:.2f} FPS, '
          f'p95 {phases["interval_ms"]["p95"]:.2f} ms', flush=True)
    if frame_budget:
        print(f'  {frame_budget["missed_frames"]}/{len(rows)} frames over '
              f'{args.max_frame_ms:.6f} ms; p99 {frame_budget["p99_ms"]:.2f} ms, '
              f'max {frame_budget["max_ms"]:.2f} ms', flush=True)
    return {'scenario': scenario, 'repetition': repetition, 'frames': len(rows),
            'fps': fps, 'phases': phases, 'dimensions': dimensions, 'command': command,
            'frame_budget': frame_budget}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scenario', action='append', choices=['title', 'profile', 'match', 'split'])
    parser.add_argument('--gameplay-xbe', type=Path)
    parser.add_argument('--map', type=Path, default=ROOT / 'assets/disc/maps/lockout.map',
                        help='Actual gameplay cache to measure, including prepared MCC maps')
    parser.add_argument('--shared-maps', type=Path,
                        help='Shared resources belonging to the selected gameplay cache')
    parser.add_argument('--local-players', type=int, choices=(1, 2, 3, 4), default=4,
                        help='Local player count for the split scenario (default: 4)')
    parser.add_argument('--starting-weapon', type=int, choices=(0, 5), default=5,
                        help='Map-authored starting weapon (0) or the existing SMG benchmark (5)')
    parser.add_argument('--ui-assets', type=Path,
                        help='HUD/UI sampling pack; defaults to the launcher asset registry selection')
    parser.add_argument('--native-hud', action='store_true',
                        help='Use the ordinary gameplay HUD without an imported UI override')
    parser.add_argument('--report-output', type=Path,
                        help='Also retain the small JSON summary outside disposable raw output storage')
    parser.add_argument('--frames', type=int, default=210)
    parser.add_argument('--warmup', type=int, default=30)
    parser.add_argument('--repetitions', type=int, default=3)
    parser.add_argument('--width', type=int, default=1280)
    parser.add_argument('--height', type=int, default=720)
    parser.add_argument('--render-size', type=int, nargs=2, metavar=('WIDTH','HEIGHT'),
                        help='Pin ordinary multiplayer internal rendering independently of the presentation window')
    parser.add_argument('--timeout', type=float, default=300)
    parser.add_argument('--min-fps', type=float, default=0)
    parser.add_argument('--max-frame-ms', type=float, default=0,
                        help='Require every sampled frame interval to fit this budget; zero disables the gate')
    parser.add_argument('--profile-multiplayer', action='store_true',
                        help='--runtime is profile_mp_render_cpu; pins internal resolution despite window clamping')
    parser.add_argument('--audio', action='store_true')
    parser.add_argument('--hidden', action='store_true')
    args = parser.parse_args()
    if args.warmup < 1 or args.frames <= args.warmup or args.repetitions < 1:
        parser.error('Require positive warmup/repetitions and frames greater than warmup')
    if args.max_frame_ms < 0:
        parser.error('Frame budget must be nonnegative')
    if args.render_size and any(size<64 or size>4096 for size in args.render_size):
        parser.error('Internal render dimensions must be 64..4096')
    args.runtime = args.runtime.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    if args.shared_maps:
        args.shared_maps = args.shared_maps.resolve(strict=True)
    try:
        if args.native_hud:
            if args.ui_assets:
                parser.error('--native-hud cannot be combined with --ui-assets')
        else:
            args.ui_assets = (args.ui_assets or registered_vista_assets(ROOT)).resolve(strict=True)
            if not all((args.ui_assets / name).is_file() for name in ('pack.tsv', 'hud.tsv')):
                parser.error('UI assets must contain pack.tsv and hud.tsv')
    except (OSError, ValueError) as error:
        parser.error(str(error))
    if args.gameplay_xbe:
        args.gameplay_xbe = args.gameplay_xbe.resolve(strict=True)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    scenarios = args.scenario or (['match', 'split'] if args.profile_multiplayer else ['title', 'profile', 'match', 'split'])
    if args.render_size and (args.profile_multiplayer or any(scene not in ('match','split') for scene in scenarios)):
        parser.error('--render-size requires ordinary multiplayer-only scenarios')
    if args.native_hud and any(scene in ('title', 'profile') for scene in scenarios):
        parser.error('--native-hud requires gameplay-only scenarios')
    if args.profile_multiplayer:
        if args.local_players != 4 or args.native_hud:
            parser.error('The detailed profiler currently requires four split views and imported UI assets')
        if not args.gameplay_xbe or args.audio or args.hidden or args.height != args.width * 9 // 16:
            parser.error('Profiler requires --gameplay-xbe, visible muted playback and a 16:9 render size')
        if any(scene not in ('match', 'split') for scene in scenarios):
            parser.error('Profiler supports only match/split scenarios')
    runs = [measure(args, scenario, repetition) for repetition in range(args.repetitions)
            for scenario in scenarios]
    medians = {scenario: statistics.median(run['fps'] for run in runs if run['scenario'] == scenario)
               for scenario in scenarios}
    report = {'runtime': str(args.runtime),
              'runtime_sha256': hashlib.sha256(args.runtime.read_bytes()).hexdigest(),
              'ui_assets': str(args.ui_assets) if args.ui_assets else None,
              'map': str(args.map),
              'map_sha256': hashlib.sha256(args.map.read_bytes()).hexdigest(),
              'shared_maps': str(args.shared_maps) if args.shared_maps else None,
              'split_local_players': args.local_players,
              'starting_weapon': args.starting_weapon,
              'backend_requested': os.environ.get('SDL_VIDEODRIVER', 'automatic'),
              'reference_controls': {name: os.environ.get(name) for name in
                  ('HALO2_REFERENCE_REPEAT_VEHICLE_SKINS', 'HALO2_REFERENCE_INSTRUCTION_CALLBACKS',
                   'HALO2_REFERENCE_WRITE_DISPATCH', 'HALO2_REFERENCE_LITERAL_INSTRUCTIONS',
                   'HALO2_REFERENCE_TEXTURE_FINGERPRINTS', 'HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS',
                   'HALO2_REFERENCE_REPEAT_UNIT_SKINS', 'HALO2_REFERENCE_SCALAR_VECTOR_READS')},
              'visible': not args.hidden, 'audio': args.audio, 'profile_multiplayer': args.profile_multiplayer,
              'requested_width': args.width, 'requested_height': args.height, 'warmup': args.warmup,
              'requested_render_size': args.render_size,
              'min_fps': args.min_fps, 'max_frame_ms': args.max_frame_ms,
              'median_fps': medians, 'runs': runs,
              'limits': 'Host frame intervals include driver/presentation waits; fixed title/profile view '
                        'and idle players on the selected map, not physical refresh timestamps or a full playthrough. '
                        'Menu preferences can override requested viewport size; per-run dimensions record actual targets.'}
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    if args.report_output:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'median_fps': medians}), flush=True)
    fps_passed = all(value >= args.min_fps for value in medians.values())
    budget_passed = all(not run['frame_budget'] or run['frame_budget']['passed'] for run in runs)
    return 0 if fps_passed and budget_passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
