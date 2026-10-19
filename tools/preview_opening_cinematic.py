#!/usr/bin/env python3
"""Test the opening campaign with the launcher's selected MCC cache only.

Run through tools/run_test.py. Input verification establishes source provenance,
not campaign readiness or Xbox presentation parity. No cache/XBE/UI override or
asset-viewer fallback is accepted here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import struct
import subprocess
import sys

from audit_rendering import FormatError
from mcc_onboarding import selected

ROOT = Path(__file__).resolve().parents[1]
MAP_DIRECTORY = 'halo2/h2_maps_win64_dx11'
MAP_NAME = '00a_introduction.map'
INPUTS = ('mainmenu.map', MAP_NAME, 'shared.map', 'single_player_shared.map',
          'textures.dat', 'sounds_en.dat', 'sounds_neutral.dat', 'sounds_remastered.dat')


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def opening_inputs(root=ROOT):
    selection = selected(root)
    if selection is None:
        raise FormatError('Select a prepared MCC cache with Halo2-Launcher --setup first')
    cache, provenance = selection
    conversion = json.loads((cache / 'tag-conversion.json').read_text())
    expected = {name: row['sha256'] for name, row in provenance['source_files'].items()}
    for row in conversion['maps']:
        name = (PurePosixPath('h2_maps_win64_dx11') / row['map']).as_posix()
        if name not in expected:
            raise FormatError('Converted map has no MCC donor: ' + name)
        expected[name] = row['output_sha256']
    changes = provenance.get('output_changes', []) + provenance.get('graphics_corrections', {}).get('changes', [])
    for change in changes:
        name = PurePosixPath(change['path']).relative_to('halo2').as_posix()
        if change['donor_sha256'] != expected.get(name):
            raise FormatError('MCC conversion provenance chain differs: ' + name)
        expected[name] = change['converted_sha256']
    rows = []
    for name in INPUTS:
        relative = PurePosixPath('h2_maps_win64_dx11') / name
        path = cache / 'halo2' / relative
        if path.is_symlink() or not path.is_file() or not path.resolve().is_relative_to(cache):
            raise FormatError('Opening input escaped the selected MCC cache: ' + name)
        actual = digest(path)
        if actual != expected.get(relative.as_posix()):
            raise FormatError('Opening input differs from prepared MCC provenance: ' + name)
        row = dict(path=str(path.resolve()), bytes=path.stat().st_size, sha256=actual)
        if path.suffix == '.map':
            with path.open('rb') as stream:
                header = stream.read(896)
            if len(header) != 896 or struct.unpack_from('<II', header) != (0x68656164, 13):
                raise FormatError('Opening requires MCC version-13 caches: ' + name)
            row['cache_version'] = 13
        rows.append(row)
    return dict(cache=str(cache), map=str(cache / MAP_DIRECTORY / MAP_NAME),
                shared_maps=str(cache / MAP_DIRECTORY), inputs=rows,
                source='Halo MCC only', scope='MCC input provenance; no playback/parity claim')


def opening_command(runtime, inputs, *, hidden=False, frames=0, viewport=(1280, 720)):
    command = [str(Path(runtime).resolve(strict=True)), '--campaign', '--map', inputs['map'],
               '--shared-maps', inputs['shared_maps'], '--graphics-mode', 'Original',
               '--viewport', *map(str, viewport), '--frames', str(frames)]
    if hidden:
        command.append('--hidden')
    return command


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path)
    parser.add_argument('--check-inputs', action='store_true')
    parser.add_argument('--hidden', action='store_true')
    parser.add_argument('--frames', type=int, default=0)
    parser.add_argument('--viewport', nargs=2, type=int, default=(1280, 720))
    parser.add_argument('--timeout', type=float, default=240)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Run this test through tools/run_test.py')
    if args.frames < 0 or any(v <= 0 for v in args.viewport) or not 0 < args.timeout <= 600:
        parser.error('Invalid frame, viewport or timeout bound')
    if not args.check_inputs and args.runtime is None:
        parser.error('--runtime is required for campaign playback')
    report = dict(passed=False, retained_large_artifacts=[], scratch=os.environ['HALO2_TEST_SCRATCH'])
    status = 1
    try:
        report['mcc'] = opening_inputs()
        if args.check_inputs:
            status = 0
        else:
            report['command'] = opening_command(args.runtime, report['mcc'], hidden=args.hidden,
                                               frames=args.frames, viewport=args.viewport)
            report['runtime_sha256'] = digest(Path(report['command'][0]))
            environment = {k: v for k, v in os.environ.items()
                           if not k.startswith('HALO2_') or k == 'HALO2_TEST_SCRATCH'}
            log = Path(os.environ['HALO2_TEST_SCRATCH']) / 'opening-native.log'
            with log.open('w') as stream:
                result = subprocess.run(report['command'], cwd=ROOT, env=environment,
                                        stdout=stream, stderr=subprocess.STDOUT, timeout=args.timeout)
            status = result.returncode
            report['runtime_returncode'] = status
            # Keep bounded diagnostics, never the runtime or captures.
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size - 16384))
                report['log_tail'] = stream.read().decode('utf-8', errors='replace')
        report['passed'] = status == 0
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        status = 1
        report['error'] = str(error)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], source=report.get('mcc', {}).get('source'),
                         map=report.get('mcc', {}).get('map'), error=report.get('error'),
                         report=str(args.report) if args.report else None)))
    return status


if __name__ == '__main__':
    raise SystemExit(main())
