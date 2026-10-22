#!/usr/bin/env python3
"""Observe the owned native startup stack; no original guest state is changed."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import time

from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log


def descendants(pid):
    path = Path(f'/proc/{pid}/task/{pid}/children')
    try:
        children = [int(p) for p in path.read_text().split()]
    except FileNotFoundError:
        return []
    return [p for child in children for p in descendants(child)] + children


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--executable', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--interrupt-after', type=int, default=60)
    parser.add_argument('--timeout', type=int, default=360)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not 1 <= args.interrupt_after < args.timeout <= 600:
        parser.error('Require 1 <= interrupt-after < timeout <= 600')
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    scratch = owner / 'combat-startup-profile';scratch.mkdir(exist_ok=True)
    args.report = args.report.resolve();args.report.parent.mkdir(parents=True, exist_ok=True)
    runtime_report = args.report.with_name(args.report.stem + '-runtime.json')
    log = scratch / 'runtime.log';capture = scratch / 'actual.ppm'
    command = ['gdb', '--batch', '--quiet', '-ex', 'set pagination off',
               '-ex', 'set startup-with-shell off', '-ex', 'set disable-randomization off',
               '-ex', 'run', '-ex', 'echo [COMBAT-STARTUP-STACK]\\n',
               '-ex', 'bt 16', '-ex', 'thread apply all bt 4',
               '-ex', 'echo [COMBAT-STARTUP-RESUME]\\n', '-ex', 'continue',
               '--args', str(args.executable.resolve()), 'zoom', str(args.map.resolve()),
               str(runtime_report), 'sniper_rifle', '1']
    started = time.monotonic();interrupted = False;timed_out = False
    with private_display(scratch) as display, log.open('w') as stream:
        environment = dict(os.environ, DISPLAY=display, SDL_VIDEODRIVER='x11',
                           SDL_AUDIODRIVER='dummy', XDG_CACHE_HOME=str(owner / 'graphics-cache'),
                           HALO2_COMBAT_AUDIT='1', HALO2_COMBAT_AUDIT_CAPTURE=str(capture))
        process = subprocess.Popen(command, env=environment, stdout=stream, stderr=subprocess.STDOUT)
        try:
            process.wait(timeout=args.interrupt_after)
        except subprocess.TimeoutExpired:
            process.send_signal(signal.SIGINT);interrupted = True
            try:
                process.wait(timeout=args.timeout - args.interrupt_after)
            except subprocess.TimeoutExpired:
                timed_out = True
        finally:
            if process.poll() is None:
                owned = descendants(process.pid)
                for pid in owned:
                    try: os.kill(pid, signal.SIGTERM)
                    except ProcessLookupError: pass
                process.terminate()
                try: process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    for pid in owned:
                        try: os.kill(pid, signal.SIGKILL)
                        except ProcessLookupError: pass
                    process.kill();process.wait()
    keep_log(log, args.report.with_suffix('.log'))
    text = log.read_text(errors='replace')
    begin = text.find('[COMBAT-STARTUP-STACK]');end = text.find('[COMBAT-STARTUP-RESUME]', begin)
    stack = text[begin:end] if begin >= 0 and end >= 0 else 'No interrupted startup stack captured\n'
    args.report.with_name(args.report.stem + '-stack.log').write_text(stack[:65536])
    result = {'scope': 'Native startup observed under GDB; no Xbox framebuffer comparison',
              'command': command, 'gdb_exit': process.returncode, 'interrupted_for_stack': interrupted,
              'timed_out': timed_out, 'timeout_seconds': args.timeout,
              'elapsed_seconds': time.monotonic()-started, 'runtime_report_produced': runtime_report.is_file(),
              'capture_produced': capture.is_file(), 'capture_scratch': str(capture)}
    args.report.write_text(json.dumps(result, indent=2)+'\n');print(json.dumps(result))
    return 124 if timed_out else 0 if runtime_report.is_file() else 2


if __name__ == '__main__':
    raise SystemExit(main())
