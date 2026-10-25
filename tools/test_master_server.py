#!/usr/bin/env python3
"""Run through run_test.py: build the Rust service ABI and test the backend."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT/'build/test-results/master-server')
    parser.add_argument('tests', nargs='*')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    env = dict(os.environ, CARGO_TARGET_DIR=str(scratch/'cargo'), CARGO_INCREMENTAL='0',
               RUSTUP_TOOLCHAIN='1.96.0', RUSTUP_AUTO_INSTALL='0')
    args.output.mkdir(parents=True, exist_ok=True)
    commands = []
    if not env.get('H2_SECURITY_LIBRARY'):
        commands.append(['cargo','build','--locked','--release','--manifest-path',str(ROOT/'rust/online_security_ffi/Cargo.toml')])
        filename = 'halo2_online_security_ffi.dll' if sys.platform=='win32' else 'libhalo2_online_security_ffi.'+('dylib' if sys.platform=='darwin' else 'so')
        env['H2_SECURITY_LIBRARY'] = str(scratch/'cargo/release'/filename)
    commands.append([sys.executable,'-B','-m','unittest',*args.tests] if args.tests else
                    [sys.executable,'-B','-m','unittest','discover','-s','multiplayer/master_server/tests','-t','.'])
    report = {'commands': [], 'passed': False, 'retained_large_artifacts': []}
    for index, command in enumerate(commands):
        print('+ '+' '.join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output = result.stdout[-1024*1024:]
        (args.output/f'step-{index}.log').write_bytes(output)
        print(output.decode(errors='replace'), end='', flush=True)
        report['commands'].append({'argv': command, 'returncode': result.returncode})
        if result.returncode:
            (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
            return result.returncode
    report['passed']=True
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0


if __name__=='__main__':
    raise SystemExit(main())
