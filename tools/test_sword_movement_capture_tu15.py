#!/usr/bin/env python3
"""Capture a canonical native Sword case and replay its actual movement inputs.

Run inside tools/run_test.py scratch with a binary linked to the unchanged
capture_sword_movement_tu15.cpp observer. Cancellation clocks come only from
the native log. The existing replay owns per-call original/native comparison;
the native case owns gameplay acceptance. Retain only the bounded JSON report.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
REPLAY = Path(__file__).with_name('replay_sword_movement_tu15.py')
DIRECTED = {
    'reload_host': ('sword-cancel-neutral-host', {}),
    'jump_remote': ('sword-cancel-jump-client', {}),
    'switch_remote': ('sword-cancel-switch-client', {}),
    'swap_host': ('sword-flight-swap-host', {}),
    'jump_elite': ('sword-cancel-jump-client', {'HALO2_SWORD_REPRESENTATION': '3'}),
    'air_lunge_remote': ('lunge-client', {'HALO2_SWORD_JUMP_BEFORE_ATTACK': '1'}),
    'air_cancel_host': ('sword-cancel-neutral-host', {'HALO2_SWORD_JUMP_BEFORE_ATTACK': '1'}),
}
NATIVE_MODES = {mode for mode, _ in DIRECTED.values()}
REPORT_LIMIT = 1024 * 1024


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def bounded_log(path):
    size = path.stat().st_size
    with path.open('rb') as stream:
        stream.seek(max(0, size - 48 * 1024))
        tail = stream.read().decode(errors='replace')
    return dict(bytes=size, sha256=sha(path), tail=tail, truncated=size > 48 * 1024)


def observed_cancellation(log, mode):
    matches = []
    with log.open(errors='replace') as stream:
        for line in stream:
            if line.startswith('Source sword cancellation input:'):
                fields = dict(re.findall(r'([A-Za-z_]+)=([^\s]+)', line))
                if fields.get('mode') != mode or fields.get('physical_mode') != '6':
                    raise RuntimeError('Cancellation record lacks the requested native mode6 dash')
                matches.append(dict(loop_tick=int(fields['tick']), source_tick=int(fields['source_tick'])))
    if len(matches) != 1:
        raise RuntimeError('Expected exactly one observed native cancellation input record')
    if matches[0]['source_tick'] <= 0:
        raise RuntimeError('Observed cancellation source clock must be positive')
    return matches[0]


def run(command, environment, log, timeout):
    start = time.monotonic()
    with log.open('wb') as output:
        try:
            result = subprocess.run(command, env=environment, stdout=output,
                                    stderr=subprocess.STDOUT, timeout=timeout)
            return dict(command=command, exit_code=result.returncode,
                        seconds=time.monotonic() - start, timed_out=False)
        except subprocess.TimeoutExpired:
            return dict(command=command, exit_code=124,
                        seconds=time.monotonic() - start, timed_out=True)


def save(path, report):
    raw = (json.dumps(report, indent=2) + '\n').encode()
    if len(raw) > REPORT_LIMIT:
        raise RuntimeError('Sword movement wrapper report exceeds1MiB')
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(raw)


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--shared-maps', type=Path)
    parser.add_argument('--mode', choices=sorted(DIRECTED.keys() | NATIVE_MODES), required=True)
    # Keep this venv path explicit: resolving its symlink would select the
    # system interpreter and lose the installed Unicorn/Capstone modules.
    parser.add_argument('--oracle-python', type=Path, default=ROOT / '.tools/python/bin/python')
    parser.add_argument('--timeout-seconds', type=float, default=180)
    parser.add_argument('--max-capture-calls', type=int, default=256)
    parser.add_argument('--report-output', type=Path, required=True)
    args = parser.parse_args()
    if not 0 < args.timeout_seconds <= 600:
        parser.error('--timeout-seconds must be finite and at most600')
    if not 1 <= args.max_capture_calls <= 1024:
        parser.error('--max-capture-calls must be1..1024')
    if not args.report_output.is_absolute():
        parser.error('--report-output must name an explicit absolute bounded-report path')
    return args


def main():
    args = arguments()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    if not scratch.is_dir():
        raise RuntimeError('Use the existing tools/run_test.py scratch owner')
    args.binary = args.binary.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    args.shared_maps = (args.shared_maps or args.map.parent).resolve(strict=True)
    args.oracle_python = args.oracle_python.absolute()
    if not args.oracle_python.is_file() or not os.access(args.oracle_python, os.X_OK):
        raise RuntimeError('Supply an executable --oracle-python with Unicorn and Capstone')
    mode, additions = DIRECTED.get(args.mode, (args.mode, {}))
    environment = dict(os.environ)
    if args.mode in DIRECTED:
        # These seven aliases specify their complete representation/air setup.
        environment['HALO2_SWORD_REPRESENTATION'] = '2'
        environment.pop('HALO2_SWORD_JUMP_BEFORE_ATTACK', None)
    environment.update(additions)
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    environment.setdefault('H2_NET_PROFILE', 'complete')
    environment.pop('HALO2_SWORD_MOVEMENT_CAPTURE_MIN_TICK', None)
    environment['HALO2_SWORD_MOVEMENT_CAPTURE_MAX'] = str(args.max_capture_calls)
    cancel = mode.startswith('sword-cancel-') or mode == 'sword-flight-swap-host'
    report = dict(schema='H2_TU15_LIVE_SWORD_MOVEMENT_V1', passed=False,
                  directed_case=args.mode, mode=mode, binary=str(args.binary),
                  binary_sha256=sha(args.binary), wrapper_sha256=sha(__file__),
                  replay_sha256=sha(REPLAY), oracle_python=str(args.oracle_python),
                  qualification='Per-call original/native movement comparison plus native canonical-case acceptance',
                  floating_point_scope='Replay compares x87 control/exception/TOP/live values and SSE state; x87 condition bits remain unqualified',
                  cancellation_required=cancel, retained_large_artifacts=[])
    code = 1
    with tempfile.TemporaryDirectory(prefix='sword-movement-', dir=scratch) as temporary:
        owner = Path(temporary)
        capture, native_log = owner / 'movement.jsonl', owner / 'native.log'
        environment['HALO2_SWORD_MOVEMENT_CAPTURE'] = str(capture)
        try:
            command = [str(args.binary), str(args.map), str(args.shared_maps),
                       'native-mcc', str(owner / 'world'), mode]
            report['native'] = run(command, environment, native_log, args.timeout_seconds)
            report['native']['log'] = bounded_log(native_log)
            code = report['native']['exit_code']
            if code:
                raise RuntimeError('Canonical native movement case failed')
            if not capture.is_file() or not capture.stat().st_size:
                code = 1
                raise RuntimeError('Native binary omitted its actual movement capture')
            report['capture'] = dict(bytes=capture.stat().st_size, sha256=sha(capture))
            original_report, original_log = owner / 'original.json', owner / 'original.log'
            command = [str(args.oracle_python), '-B', str(REPLAY), '--capture', str(capture),
                       '--report-output', str(original_report)]
            if cancel:
                observed = observed_cancellation(native_log, mode)
                report['observed_cancellation'] = observed
                command += ['--require-span-tick', str(observed['source_tick']),
                            '--require-termination-transition']
            report['original'] = run(command, environment, original_log, args.timeout_seconds)
            report['original']['log'] = bounded_log(original_log)
            code = report['original']['exit_code']
            if original_report.is_file():
                if original_report.stat().st_size > 4 * REPORT_LIMIT:
                    raise RuntimeError('Original movement result exceeds bounded summary input')
                result = json.loads(original_report.read_text())
                report['original']['report_sha256'] = sha(original_report)
                report['original']['comparison'] = {key: value for key, value in result.items()
                                                     if key not in ('cases', 'scratch')}
                report['original']['compared_calls'] = len(result.get('cases', []))
            if code or not report['original'].get('comparison', {}).get('passed'):
                raise RuntimeError('Actual original/native movement replay failed')
            if sha(REPLAY) != report['replay_sha256'] or sha(args.binary) != report['binary_sha256']:
                raise RuntimeError('Declared movement replay input changed during validation')
            report['passed'] = True
            code = 0
        except Exception as error:
            report['failure'] = f'{type(error).__name__}: {error}'
            code = code or 1
    report['private_raw_capture_and_world_removed'] = not owner.exists()
    save(args.report_output, report)
    print(('PASS ' if report['passed'] else 'FAIL ') + args.mode + ': actual native movement/original replay')
    if not report['passed']:
        print(report.get('failure', 'Unqualified native movement case'))
        for stage in ('native', 'original'):
            print(report.get(stage, {}).get('log', {}).get('tail', '')[-12000:])
    return code


if __name__ == '__main__':
    raise SystemExit(main())
