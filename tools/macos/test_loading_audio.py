#!/usr/bin/env python3
"""Disposable real loading-audio validation, optionally fed over SSH.

Run through tools/run_test.py. On Linux, --source-halo2 authenticates the
installed MCC archive and streams only the 3.4 MB selected WEM to Mac scratch.
The Mac mode reads that bounded input and invokes tools/run_tests.py.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from mcc_loading_presentation import MEDIA_SHA, read_media


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--source-halo2', type=Path)
    mode.add_argument('--stdin-media', action='store_true')
    parser.add_argument('--ssh-host', default='codex-mac')
    parser.add_argument('--remote-root', default='/Users/eric/Developer/Halo2-macos-port')
    parser.add_argument('--output', default='build/test-results/macos-vorbis-loading')
    args = parser.parse_args()
    if args.source_halo2:
        payload, owner = read_media(args.source_halo2)
        print('Authenticated loading media:', owner['sha256'], 'bytes:', len(payload), flush=True)
        remote = [
            'python3', '-B', 'tools/run_test.py', '--', 'python3', '-B',
            'tools/macos/test_loading_audio.py', '--stdin-media', '--output', args.output]
        command = 'export PATH=/opt/homebrew/bin:$PATH; cd ' + shlex.quote(args.remote_root) + '; ' + shlex.join(remote)
        return subprocess.run(['ssh', '-o', 'BatchMode=yes', args.ssh_host, command], input=payload).returncode
    if sys.platform != 'darwin':
        parser.error('--stdin-media requires macOS')
    payload = sys.stdin.buffer.read(16 * 1024**2 + 1)
    if len(payload) > 16 * 1024**2 or hashlib.sha256(payload).hexdigest() != MEDIA_SHA:
        raise RuntimeError('SSH input is not the authenticated supported loading media')
    media = Path(os.environ['HALO2_TEST_SCRATCH']) / 'loading.wem'
    media.write_bytes(payload)
    return subprocess.run([sys.executable, '-B', str(ROOT / 'tools/run_tests.py'),
        '--source', str(ROOT / 'tests/macos/vorbis'), '--target', 'macos_vorbis_tests',
        '--filter', '^macos_loading_audio_', '--jobs', '2', '--test-jobs', '1', '--output', args.output,
        '--cmake-arg=-GNinja', '--cmake-arg=-DHALO2_MCC_LOADING_TEST_MEDIA=' + str(media),
        '--retain-report', 'loading-audio.json', '--retain-report', 'macos-dependencies/vorbis/manifest.json',
        '--retain-report', 'halo2_mcc_loading_audio.bundle.json',
        '--retain-report', 'macos_vorbis_dependency_tests.bundle.json']).returncode


if __name__ == '__main__':
    raise SystemExit(main())
