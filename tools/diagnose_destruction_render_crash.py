#!/usr/bin/env python3
"""Replay an authenticated crash ELF under GDB in an owned isolated display.

Use tools/run_test.py. The required reference executable, native outputs,
captures and debugger state stay in HALO2_TEST_SCRATCH and are removed by it.
Only a bounded log and execution receipt leave the owner.
"""
import argparse
import ctypes
import ctypes.util
from collections import deque
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--execution', type=Path)
    parser.add_argument('--mcc-root', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--worker', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if args.worker:
        maps = args.mcc_root.resolve() / 'halo2/h2_maps_win64_dx11'
        native_output = scratch / 'native'
        command = ['gdb', '--batch', '-ex', 'set pagination off', '-ex', 'set print thread-events off',
                   '-ex', 'run', '-ex', 'thread apply all bt 24', '-ex', 'info registers', '--args',
                   str(args.runtime), str(maps / 'coagulation.map'), str(maps), 'native-mcc',
                   str(args.mcc_root.resolve()), str(native_output), 'vehicle']
        environment = dict(os.environ, HALO2_TU15_VEHICLE_NAME='warthog',
                           HALO2_TU15_VEHICLE_BLAST_INPUT='1', HALO2_TU15_VEHICLE_WEAPON_INPUT='1',
                           SDL_VIDEODRIVER='x11')
        # Deliver real backend focus to the actual reported X11 window. This
        # supplies local input only; SDL still verifies the OS focus itself.
        x11 = ctypes.CDLL(ctypes.util.find_library('X11'))
        x11.XOpenDisplay.argtypes = [ctypes.c_char_p];x11.XOpenDisplay.restype = ctypes.c_void_p
        x11.XMapRaised.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        x11.XSetInputFocus.argtypes = [ctypes.c_void_p, ctypes.c_ulong, ctypes.c_int, ctypes.c_ulong]
        x11.XFlush.argtypes = [ctypes.c_void_p];x11.XCloseDisplay.argtypes = [ctypes.c_void_p]
        display = x11.XOpenDisplay(os.environ['DISPLAY'].encode())
        if not display:
            raise RuntimeError('Cannot inspect owned X11 display')
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                   text=True, bufsize=1, env=environment)
        prefix=[];prefix_bytes=0;tail=deque();tail_bytes=0
        try:
            for line in process.stdout:
                size=len(line.encode())
                if prefix_bytes+size<=64*1024:
                    prefix.append(line);prefix_bytes+=size
                else:
                    tail.append(line);tail_bytes+=size
                    while tail_bytes>512*1024:
                        tail_bytes-=len(tail.popleft().encode())
                match = re.search(r'acceptance_x11_window=(\d+)', line)
                if match:
                    window = int(match[1]);x11.XMapRaised(display, window)
                    x11.XSetInputFocus(display, window, 2, 0);x11.XFlush(display)
            (scratch/'gdb.log').write_text(''.join(prefix)+'\n[bounded debugger tail]\n'+''.join(tail))
            return process.wait()
        finally:
            x11.XCloseDisplay(display)
            if process.poll() is None:
                process.terminate();process.wait(timeout=10)
    if not args.execution:
        parser.error('--execution is required to authenticate the observed crash executable')
    expected = json.loads(args.execution.read_text())['runtime_sha256']
    clone = scratch / 'physics_multiplayer_acceptance'
    shutil.copyfile(args.runtime, clone);clone.chmod(0o700)
    if sha(clone) != expected:
        raise ValueError('Runtime changed before the required crash reference was captured')
    args.output = args.output.resolve();args.output.mkdir(parents=True, exist_ok=False)
    report = {'scratch': str(scratch), 'crash_runtime_sha256': expected,
              'required_reference_bytes': clone.stat().st_size, 'large_artifacts_retained': [],
              'source_runtime_modified': False, 'shader_input_owner': str(args.runtime.parent / 'shaders')}
    command = ['xvfb-run', '-a', '-s', '-screen 0 960x540x24 -nolisten tcp', sys.executable,
               '-B', str(Path(__file__).resolve()), '--worker', '--runtime', str(clone),
               '--mcc-root', str(args.mcc_root.resolve()), '--output', str(scratch / 'unused')]
    report['command'] = command
    code = subprocess.run(command, timeout=300, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    report['returncode'] = code
    log = scratch / 'gdb.log'
    if log.exists():
        text = log.read_text();report['segfault_reproduced'] = 'received signal SIGSEGV' in text
        report['log_sha256'] = sha(log);shutil.copyfile(log, args.output / 'gdb.log')
    report['reference_clone_unchanged'] = sha(clone) == expected
    (args.output / 'execution.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return code


if __name__ == '__main__':
    raise SystemExit(main())
