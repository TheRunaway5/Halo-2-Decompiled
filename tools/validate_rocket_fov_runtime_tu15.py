#!/usr/bin/env python3
"""Exercise actual SDL rocket pickup/poses/firing with disposable captures.

Run through tools/run_test.py, or inside an already owned finite test session.
Xbox pixels and full weapon parity are not certified by this diagnostic.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

from validate_weapon_runtime_tu15 import private_display
from weapon_test_cache import require_mcc_map


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--maps', type=Path, required=True)
    parser.add_argument('--map', default='burial_mounds.map')
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--phase', choices=('all', 'idle', 'attack'), default='all')
    parser.add_argument('--players', type=int, choices=(1, 2, 4), default=1)
    parser.add_argument('--width', type=int, default=800)
    parser.add_argument('--height', type=int, default=600)
    parser.add_argument('--visuals-only', action='store_true',
                        help='Require actual rocket model and authored exhaust submissions')
    args = parser.parse_args()
    if Path(args.map).name != args.map or not args.map.endswith('.map'):
        parser.error('--map must be a cache filename inside --maps')
    require_mcc_map(args.maps / args.map)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    output = scratch / 'rocket-runtime'
    output.mkdir(exist_ok=True)
    root = Path(__file__).resolve().parents[1]
    report = dict(passed=False, runtime_cache_source='MCC',
                  Xbox_framebuffer_parity_verified=False, cases=[])
    with private_display(output) as display:
        for fov, phase in ((78, 'idle'), (120, 'idle'), (120, 'attack')):
            if args.phase != 'all' and args.phase != phase:
                continue
            case = output / f'{phase}-{fov}'
            case.mkdir(exist_ok=True)
            trace, capture = case / 'source.json', case / 'capture.ppm'
            capture.unlink(missing_ok=True)
            environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11',
                               SDL_AUDIODRIVER='dummy', XDG_CACHE_HOME=str(scratch / 'cache'),
                               HALO2_ANIMATION_VALIDATION='1',
                               HALO2_TEST_TRACE_PROJECTILE_BIRTHS='1',
                               HALO2_TEST_TRACE_WEAPON_EFFECT_REQUESTS='1',
                               HALO2_TEST_FIELD_OF_VIEW=str(fov),
                               HALO2_TEST_LOCAL_PLAYERS=str(args.players),
                               HALO2_TEST_VIEWPORT_WIDTH=str(args.width),
                               HALO2_TEST_VIEWPORT_HEIGHT=str(args.height),
                               HALO2_TEST_WEAPON_STATE_REPORT=str(trace))
            if args.visuals_only:
                environment['HALO2_TEST_ROCKET_VISUALS'] = '1'
            layers = root / '.tools/validation/usr/share/vulkan/explicit_layer.d'
            if layers.is_dir():
                environment['VK_LAYER_PATH'] = str(layers)
                environment['LD_LIBRARY_PATH'] = ':'.join([
                    str(root / '.tools/validation/usr/lib'),
                    str(root / '.tools/validation/usr/lib/x86_64-linux-gnu'),
                    environment.get('LD_LIBRARY_PATH', '')])
            command = [str(args.executable), str(args.maps / args.map),
                       str(args.maps), 'native-mcc', 'rocket_launcher']
            if phase == 'idle':
                command.extend([str(capture), 'idle'])
            else:
                environment['HALO2_TEST_CLEAR_FIRING_LANE'] = '1'
                if args.visuals_only:
                    command.extend([str(capture), 'flight'])
            with (case / 'runtime.log').open('w') as log:
                try:
                    result = subprocess.run(command, env=environment, stdout=log,
                                            stderr=subprocess.STDOUT, timeout=180, check=False)
                    exit_code = result.returncode
                except subprocess.TimeoutExpired:
                    exit_code = 'timeout'
            row = dict(fov=fov, phase=phase, players=args.players,
                       viewport=[args.width, args.height],exit_code=exit_code, command=command,
                       log_tail=(case / 'runtime.log').read_text(errors='replace')[-10000:])
            if capture.is_file():
                row['capture_sha256'] = hashlib.file_digest(capture.open('rb'), 'sha256').hexdigest()
                row['disposable_capture'] = str(capture)
            if trace.is_file():
                data = json.loads(trace.read_text())
                for key in ('render_loop_completed', 'runtime_error', 'picked_tick',
                            'projectile_geometry_draws', 'projectile_geometry_owners',
                            'projectile_geometry_samples', 'projectile_births',
                            'projectile_retirements', 'source_pose_observations',
                            'published_source_contacts', 'source_sound_submissions',
                            'rocket_exhaust_draws', 'rocket_exhaust_definition'):
                    if key in data:
                        row[key] = data[key]
            report['cases'].append(row)
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + '\n')
            print(json.dumps({key: row.get(key) for key in
                              ('fov', 'phase', 'exit_code', 'runtime_error',
                               'projectile_geometry_draws', 'disposable_capture')}), flush=True)
    report['passed'] = all(row['exit_code'] == 0 for row in report['cases'])
    encoded = json.dumps(report, indent=2) + '\n'
    if len(encoded.encode()) > 1024 * 1024:
        raise ValueError('Runtime report exceeded the small-report budget')
    args.report.write_text(encoded)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
