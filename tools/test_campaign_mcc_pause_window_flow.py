#!/usr/bin/env python3
"""Check MCC campaign pause, Graphics Mode and Restart in the actual game main.

Run under tools/run_test.py. The existing SDL/Vulkan preload probe supplies
only keyboard input and observes presented frames; it does not replace game
state or renderer callbacks. All binaries, preferences and raw logs are owned
by HALO2_TEST_SCRATCH. Only the explicit bounded JSON report is retained.

The positive cases omit optional sampled HUD assets. The invalid_hud negative
control supplies an explicit malformed override, which must remain an error:
load_ui_asset_library accepts an empty path as no override, not an empty pack.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--map', required=True, type=Path)
    parser.add_argument('--case', action='append', choices=['pause', 'graphics', 'restart', 'invalid_hud'])
    parser.add_argument('--report-output', required=True, type=Path)
    parser.add_argument('--timeout-seconds', type=int, default=180)
    parser.add_argument('--validation', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run this helper through tools/run_test.py')
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    output = scratch / 'campaign-mcc-pause'
    output.mkdir()
    runtime, level = args.runtime.resolve(), args.map.resolve()
    with level.open('rb') as stream:
        header = stream.read(8)
    if len(header) != 8 or int.from_bytes(header[4:8], 'little') != 13:
        parser.error('--map must be an admitted MCC version13 campaign cache')
    if not (level.parent / 'mainmenu.map').is_file():
        parser.error('The campaign cache must contain its own mainmenu.map')
    probe = output / 'game_pause_window_probe.so'
    source = root / 'tests/game_pause_window_probe.cpp'
    flags = shlex.split(subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
    compile_command = ['c++', '-std=c++20', '-shared', '-fPIC', str(source),
                       '-o', str(probe), *flags, '-lvulkan', '-ldl']
    subprocess.run(compile_command, check=True)
    schedules = {
        'pause': '30:Escape',
        # Campaign Settings is row3; Video is settings row3. The mode list
        # starts on pStencil, so Up selects Original through its live adapter.
        'graphics': '30:Escape,40:Down,41:Down,42:Down,43:Return,'
                    '50:Down,51:Down,52:Down,53:Return,60:Return,65:Up,66:Return',
        'restart': '60:Escape,70:Down,71:Down,72:Return,80:Up,81:Return',
        # An explicitly supplied empty pack is invalid. Native MCC needs no
        # override (the positive cases above), but must not ignore a bad one.
        'invalid_hud': '30:Escape',
    }
    report = {
        'scope': 'Actual main.cpp MCC campaign SDL/Vulkan input; no reference-capture bypass, screenshots or AI controls.',
        'runtime': str(runtime), 'runtime_sha256': digest(runtime),
        'map': str(level), 'map_sha256': digest(level),
        'helper_sha256': digest(Path(__file__)), 'validation_enabled': args.validation,
        'probe_source_sha256': digest(source), 'compile_command': compile_command,
        'scratch': str(scratch), 'cases': [],
        'hud_override_contract': {
            'source': 'src/ui/frontend_ui_assets.cpp:load_ui_asset_library',
            'source_sha256': digest(root / 'src/ui/frontend_ui_assets.cpp'),
            'behavior': 'Empty path means no sampled override; a supplied directory must own a valid hud.tsv.',
            'negative_case': 'invalid_hud',
        },
    }
    for case in args.case or ['pause', 'graphics', 'restart', 'invalid_hud']:
        owned = output / case
        owned.mkdir()
        events, log_path = owned / 'events.log', owned / 'runtime.log'
        frames = 120 if case == 'restart' else 90
        command = [str(runtime), '--campaign', '--map', str(level),
                   '--shared-maps', str(level.parent), '--graphics-mode', 'pStencil',
                   '--frames', str(frames), '--viewport', '640', '360', '--mute']
        if args.validation:
            command.append('--validation')
        if case == 'invalid_hud':
            hud = owned / 'empty-hud'
            hud.mkdir()
            command += ['--hud-ui-assets', str(hud)]
        env = os.environ.copy()
        env.update(LD_PRELOAD=str(probe), XDG_DATA_HOME=str(owned / 'prefs'),
                   SDL_VIDEODRIVER='x11', HALO2_PAUSE_PROBE_EVENTS=schedules[case],
                   HALO2_PAUSE_PROBE_LOG=str(events))
        timed_out = False
        with log_path.open('w') as log:
            try:
                result = subprocess.run(command, cwd=root, env=env, stdout=log,
                                        stderr=subprocess.STDOUT, timeout=args.timeout_seconds)
                returncode = result.returncode
            except subprocess.TimeoutExpired:
                timed_out, returncode = True, None
        log = log_path.read_text(errors='replace')
        lines = events.read_text().splitlines() if events.exists() else []
        observed = []
        for line in lines:
            match = re.match(r'(\d+) relative=(\d+) .*?\btick=(\d+)\b', line)
            if match:
                observed.append(dict(zip(['frame', 'relative', 'tick'], map(int, match.groups()))))
        checks = {
            'exited_successfully': returncode == 0 and not timed_out,
            'all_frames_presented': len(lines) == frames,
            'simulation_ticks_observed': len(observed) == frames,
            'no_vulkan_validation_errors': 'VUID-' not in log and 'Validation Error' not in log,
        }
        if case == 'invalid_hud':
            checks = {
                'rejected_invalid_hud_override': returncode == 1 and not timed_out,
                'no_frames_presented': not lines,
                'no_simulation_observations': not observed,
                'exact_missing_hud_manifest_diagnostic':
                    'error: missing regular UI asset file: hud.tsv' in log.splitlines(),
                'no_vulkan_validation_errors': checks['no_vulkan_validation_errors'],
            }
        elif case != 'restart':
            paused = [row for row in observed if row['frame'] > 45]
            checks['mouse_owned_by_pause'] = len(paused) == frames - 45 and all(row['relative'] == 0 for row in paused)
            checks['simulation_ticks_frozen'] = len(paused) == frames - 45 and len({row['tick'] for row in paused}) == 1
        else:
            before = [row['tick'] for row in observed if 45 < row['frame'] <= 60]
            after = [row['tick'] for row in observed if 90 < row['frame'] <= 105]
            checks['simulation_restarted'] = bool(before and after) and min(after) < max(before)
            checks['mouse_returned_to_game'] = bool(observed) and observed[-1]['relative'] == 1
            checks['ticks_advance_after_restart'] = len({row['tick'] for row in observed if row['frame'] > 90}) > 1
        preference = owned / 'prefs/Halo2Native/Halo2/menu.cfg'
        saved_mode = None
        if preference.is_file():
            match = re.search(r'^pc_graphics_mode (\d+)$', preference.read_text(), re.MULTILINE)
            if match:
                saved_mode = int(match.group(1))
        if case == 'graphics':
            checks['live_original_mode_saved'] = saved_mode == 0
        entry = {'case': case, 'command': command, 'schedule': schedules[case],
                 'returncode': returncode, 'timed_out': timed_out,
                 'presented_frames': len(lines), 'saved_graphics_mode': saved_mode,
                 'observations': observed, 'runtime_log_tail': log[-4096:],
                 'checks': checks, 'passed': all(checks.values())}
        report['cases'].append(entry)
        print(json.dumps({'case': case, 'checks': checks, 'passed': entry['passed']}), flush=True)
    report['passed'] = all(case['passed'] for case in report['cases'])
    encoded = json.dumps(report, indent=2) + '\n'
    if len(encoded.encode()) > 1024 * 1024:
        raise RuntimeError('Report exceeds retained diagnostic limit')
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(encoded)
    print(f'Report: {args.report_output.resolve()}', flush=True)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
