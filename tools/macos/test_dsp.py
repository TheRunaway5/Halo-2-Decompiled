#!/usr/bin/env python3
"""Validate universal DSP archives with a private, disposable Rust toolchain.

Invoke through tools/run_test.py. Downloads, Cargo cache, generated bindings
and object/archive files stay under owned scratch. Only bounded reports remain.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import signal
import subprocess
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
TOOLCHAIN = '1.99.0'
RUSTUP = '1.28.2'


def main():
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/test-results/macos-dsp')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        raise RuntimeError('Run this validation on the Mac')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    host = {'arm64': 'aarch64', 'x86_64': 'x86_64'}[platform.machine()] + '-apple-darwin'
    url = f'https://static.rust-lang.org/rustup/archive/{RUSTUP}/{host}/rustup-init'
    installer = scratch / 'rustup-init'
    with urllib.request.urlopen(url + '.sha256', timeout=90) as response:
        expected = response.read(512).decode().split()[0]
    with urllib.request.urlopen(url, timeout=90) as response, installer.open('wb') as stream:
        shutil.copyfileobj(response, stream)
    with installer.open('rb') as stream:
        actual = hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual != expected:
        raise RuntimeError('Rustup installer checksum differs from its versioned upstream checksum')
    installer.chmod(0o700)
    env = dict(os.environ, CARGO_HOME=str(scratch / 'cargo'), RUSTUP_HOME=str(scratch / 'rustup'))
    env['PATH'] = str(scratch / 'cargo/bin') + os.pathsep + env['PATH']
    record = dict(toolchain=TOOLCHAIN, rustup_version=RUSTUP, installer_url=url,
                  installer_sha256=actual, upstream_checksum_url=url + '.sha256', scratch=str(scratch))
    (args.output / 'toolchain.json').write_text(json.dumps(record, indent=2) + '\n')
    subprocess.run([str(installer), '-y', '--no-modify-path', '--profile', 'minimal',
                    '--default-toolchain', TOOLCHAIN, '--target', 'aarch64-apple-darwin,x86_64-apple-darwin'],
                   env=env, check=True)
    record['rustc'] = subprocess.check_output(['rustc', '+' + TOOLCHAIN, '-Vv'], env=env, text=True)
    (args.output / 'toolchain.json').write_text(json.dumps(record, indent=2) + '\n')
    return subprocess.run([sys.executable, '-B', str(ROOT / 'tools/run_tests.py'),
        '--source', str(ROOT / 'tests/macos/dsp'), '--output', str(args.output),
        '--target', 'macos_dsp_tests', '--filter', '^macos_dsp_', '--jobs', '2', '--test-jobs', '1',
        '--cmake-arg=-GNinja', f'--cmake-arg=-DHALO2_DSP_CARGO={scratch}/cargo/bin/cargo',
        '--retain-report', 'Testing/Temporary/LastTest.log'], env=env).returncode


if __name__ == '__main__':
    raise SystemExit(main())
