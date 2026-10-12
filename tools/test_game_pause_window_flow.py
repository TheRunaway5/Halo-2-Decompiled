#!/usr/bin/env python3
"""Exercise the pause overlay in actual native Vulkan game windows on Linux."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--case', choices=['pause', 'settings', 'confirm', 'resume', 'campaign', 'audio', 'controller', 'keyboard', 'revert', 'restart'], required=True)
    parser.add_argument('--validation', action='store_true')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    sources = dict(line.split('\t', 1) for line in (root / 'assets/asset-sources.tsv').read_text().splitlines()[1:])
    xbe = (root / 'assets/gameplay-source.tsv').read_text().splitlines()[1].split('\t')[1]
    output = args.output.resolve() / args.case
    output.mkdir(parents=True, exist_ok=True)
    events = output / 'events.log'
    events.write_text('')
    schedules = {
        'pause': '5:Escape',
        'settings': '5:Escape,15:Down,16:Return',
        'confirm': '5:Escape,15:Down,16:Down,17:Return',
        'resume': '5:Escape,25:Escape',
        'campaign': '5:Escape',
        'audio': '5:Escape,15:Down,16:Return,25:Down,26:Down,27:Return,35:Return,40:Up,41:Up,42:Up,43:Return',
        'revert': '5:Escape,15:Down,16:Return',
        'restart': '5:Escape,15:Down,16:Down,17:Return,25:Up,26:Return',
        'controller': '5:Escape,15:Down,16:Return,25:Return,35:Down,36:Down,37:Down,38:Return,45:Up,46:Return,55:Up,56:Up,57:Up,58:Return,65:Down,66:Down,67:Down,68:Return',
        'keyboard': '5:Escape,15:Down,16:Return,25:Down,26:Return,35:Return,40:Up,41:Return',
    }
    command = [str(args.runtime.resolve()), '--map', str(Path(sources['xbox']) / 'maps' /
               ('01a_tutorial.map' if args.case in ['campaign', 'revert', 'restart'] else 'lockout.map')),
               '--hud-ui-assets', sources['vista'], '--gameplay-xbe', xbe,
               '--frames', '90', '--viewport', '1280', '720', '--mute',
               '--screenshot', str(output / 'frame.ppm')]
    command += ['--campaign'] if args.case == 'campaign' else ['--play'] if args.case in ['revert', 'restart'] else ['--multiplayer']
    if args.validation:
        command.append('--validation')
    env = os.environ.copy()
    env.update(LD_PRELOAD=str(args.probe.resolve()), XDG_DATA_HOME=str(output / 'prefs'),
               HALO2_PAUSE_PROBE_EVENTS=schedules[args.case], HALO2_PAUSE_PROBE_LOG=str(events))
    with (output / 'runtime.log').open('w') as log:
        result = subprocess.run(command, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=240)
    lines = events.read_text().splitlines()
    log = (output / 'runtime.log').read_text()
    checks = {
        'exited_successfully': result.returncode == 0,
        'presented_90_frames': len(lines) == 90,
        'capture_exists': (output / 'frame.ppm').is_file(),
        'mouse_returns_to_game' if args.case in ['resume', 'restart'] else 'mouse_owned_by_pause':
            bool(lines) and ('relative=1' if args.case in ['resume', 'restart'] else 'relative=0') in lines[-1],
        'no_vulkan_validation_errors': 'VUID-' not in log,
    }
    if args.case == 'campaign' and len(lines) == 90:
        # Main updates its actual simulation tick in the title every15 frames.
        checks['simulation_ticks_frozen'] = len({line.split('relative=0 ', 1)[-1] for line in lines[31:]}) == 1
    if args.case == 'audio':
        config = output / 'prefs/Halo2Native/Halo2/menu.cfg'
        checks['live_volume_saved'] = config.is_file() and any(
            line.startswith('pc_display ') and line.split()[-2] == '7'
            for line in config.read_text().splitlines())
    if args.case in ['controller', 'keyboard']:
        import shlex
        config = output / 'prefs/Halo2Native/Halo2/menu.cfg'
        options = {}
        if config.is_file():
            for line in config.read_text().splitlines():
                row = shlex.split(line)
                if len(row) == 4 and row[0] == 'option':
                    options[int(row[2])] = int(row[3])
        if args.case == 'controller':
            checks['controller_inversion_saved'] = options.get(0x393) == 1
            checks['controller_stick_layout_saved'] = options.get(0x390) == 3
        else:
            checks['southpaw_preset_saved'] = options.get(0x384) == 2
            checks['southpaw_forward_binding_saved'] = options.get(0x402) == 12  # SDL_SCANCODE_I
    report = {'scope': 'Native Linux SDL/Vulkan pause interaction; screenshots require visual inspection.',
              'runtime_sha256': hashlib.sha256(args.runtime.read_bytes()).hexdigest(),
              'command': command, 'validation_enabled': args.validation,
              'schedule': schedules[args.case], 'checks': checks,
              'passed': all(checks.values())}
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
