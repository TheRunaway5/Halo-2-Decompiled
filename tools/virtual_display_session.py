#!/usr/bin/env python3
"""Finite isolated KWin virtual-output session, owned by tools/run_test.py.

Run through run_test.py. Send JSON argv commands to OUTPUT/commands.pipe, or
{"quit": true}. Config, cache, runtime sockets and D-Bus belong to scratch;
the real desktop is never replaced or reconfigured. This establishes a virtual
presentation provider, not physical-display timing or game acceptance.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--width', type=int, default=3840)
    parser.add_argument('--height', type=int, default=2160)
    parser.add_argument('--refresh', type=int, default=240, help='Virtual output Hz (60..1000)')
    parser.add_argument('--xwayland', action='store_true',
                        help='Also expose Xwayland; native Wayland is the default presentation path')
    parser.add_argument('--idle-seconds', type=int, default=1200)
    parser.add_argument('--child', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py so the display has an owned lifecycle')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    pipe = args.output / 'commands.pipe'
    if args.child:
        # KWin supplies its newly created Wayland/Xwayland/bus environment.
        names = ['DISPLAY', 'WAYLAND_DISPLAY', 'DBUS_SESSION_BUS_ADDRESS',
                 'XDG_RUNTIME_DIR', 'XDG_CONFIG_HOME', 'XDG_CACHE_HOME', 'XDG_DATA_HOME',
                 'XAUTHORITY', 'KWIN_RENDER_NODES', 'SDL_VIDEODRIVER', 'GDK_BACKEND']
        receipt = {'at': time.time(), 'scratch_owner': str(scratch), 'pid': os.getpid(),
                   'environment': {k: os.environ[k] for k in names if k in os.environ},
                   'virtual': True, 'physical_display_latency_proven': False,
                   'presentation_provider_admitted': False}
        def save():
            (args.output / 'display.json').write_text(json.dumps(receipt, indent=2) + '\n')
        save()
        configuration = []
        def outputs():
            result = subprocess.run(['kscreen-doctor', '-j'], capture_output=True, text=True, timeout=15)
            result.check_returncode()
            return json.loads(result.stdout)
        initial = outputs()
        if len(initial['outputs']) != 1:
            raise RuntimeError('Virtual session must own exactly one output')
        name = initial['outputs'][0]['name']
        change = ['kscreen-doctor', f'output.{name}.addCustomMode.{args.width}.{args.height}.{args.refresh * 1000}.reduced']
        result = subprocess.run(change, capture_output=True, text=True, timeout=15)
        configuration.append({'argv': change, 'exit_code': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr})
        result.check_returncode()
        modes = outputs()['outputs'][0]['modes']
        mode = next(m for m in modes if m['size'] == {'width': args.width, 'height': args.height}
                    and abs(m['refreshRate'] - args.refresh) < .5)
        change = ['kscreen-doctor', f'output.{name}.mode.{mode["id"]}']
        result = subprocess.run(change, capture_output=True, text=True, timeout=15)
        configuration.append({'argv': change, 'exit_code': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr})
        result.check_returncode()
        final = outputs()['outputs'][0]
        if final['currentModeId'] != mode['id'] or not final['enabled']:
            raise RuntimeError('Requested virtual mode was not enabled')
        receipt['mode_configuration'] = configuration
        save()
        probes = [('outputs', ['kscreen-doctor', '-j']),
                  ('compositor', ['qdbus6', 'org.kde.KWin', '/KWin', 'org.kde.KWin.supportInformation'])]
        if os.environ.get('DISPLAY'):
            probes.append(('xrandr', ['xrandr', '--current']))
        for name, argv in probes:
            try:
                result = subprocess.run(argv, capture_output=True, text=True, timeout=15)
                receipt[name] = {'argv': argv, 'exit_code': result.returncode,
                                 'stdout': result.stdout[-48000:], 'stderr': result.stderr[-4000:]}
            except subprocess.TimeoutExpired:
                receipt[name] = {'argv': argv, 'status': 'timed out', 'timeout_seconds': 15}
            save()
        receipt['ready'] = True
        save()
        with os.fdopen(os.open(pipe, os.O_RDWR), 'rb', buffering=0) as commands:
            return subprocess.run([sys.executable, '-B', str(ROOT / 'tools/macos/test_session.py'),
                                   '--output', str(args.output / 'session'),
                                   '--idle-seconds', str(args.idle_seconds)], stdin=commands).returncode
    if not 64 <= args.width <= 8192 or not 64 <= args.height <= 8192:
        parser.error('Virtual output dimensions must be 64..8192')
    if not 60 <= args.refresh <= 1000:
        parser.error('Virtual refresh must be 60..1000 Hz')
    if pipe.exists():
        parser.error('Choose a fresh output directory; a command pipe already exists')
    os.mkfifo(pipe, 0o600)
    env = dict(os.environ)
    for key, name in [('XDG_RUNTIME_DIR', 'runtime'), ('XDG_CONFIG_HOME', 'config'),
                      ('XDG_CACHE_HOME', 'cache'), ('XDG_DATA_HOME', 'data')]:
        # UNIX-domain socket paths are limited to108 bytes on Linux; owned
        # workspace roots are long, so keep the runtime/socket names short.
        directory = scratch / ('r' if key == 'XDG_RUNTIME_DIR' else 'virtual-' + name)
        directory.mkdir(mode=0o700)
        env[key] = str(directory)
    for key in ['DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'DBUS_SESSION_BUS_ADDRESS',
                'DBUS_SESSION_BUS_PID', 'DBUS_ACTIVATION_ADDRESS', 'DBUS_ACTIVATION_BUS_TYPE']:
        env.pop(key, None)
    env['KWIN_RENDER_NODES'] = '/dev/dri/by-path/pci-0000:01:00.0-render'
    env['XDG_SESSION_TYPE'] = 'wayland'
    env['SDL_VIDEODRIVER'] = 'wayland'
    env['GDK_BACKEND'] = 'wayland'
    # CLI output management needs no desktop settings/file chooser portals.
    env['QT_QPA_PLATFORMTHEME'] = 'generic'
    entry = [sys.executable, '-B', str(Path(__file__).resolve()), '--child',
             '--output', str(args.output), '--idle-seconds', str(args.idle_seconds),
             '--width', str(args.width), '--height', str(args.height), '--refresh', str(args.refresh)]
    if args.xwayland:
        entry.append('--xwayland')
    argv = ['dbus-run-session', '--', 'kwin_wayland', '--virtual', '--width', str(args.width),
            '--height', str(args.height), '--output-count', '1',
            '--socket', 'w', '--no-lockscreen', '--no-global-shortcuts',
            '--no-kactivities', '--exit-with-session', shlex.join(entry)]
    if args.xwayland:
        argv.insert(argv.index('--socket'), '--xwayland')
    (args.output / 'launch.json').write_text(json.dumps({'argv': argv, 'scratch_owner': str(scratch),
        'pid': os.getpid(), 'scope': 'Isolated virtual compositor and bus, owned config/runtime/cache; existing desktop untouched.'}, indent=2) + '\n')
    # Reuse the bounded log pump; the enclosing supervisor owns every process.
    from run_tests import run_step
    original = dict(os.environ)
    try:
        os.environ.clear()
        os.environ.update(env)
        return run_step(argv, args.output / 'kwin.log')
    finally:
        os.environ.clear()
        os.environ.update(original)


if __name__ == '__main__':
    raise SystemExit(main())
