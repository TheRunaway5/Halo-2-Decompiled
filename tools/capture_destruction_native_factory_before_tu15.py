#!/usr/bin/env python3
"""Capture a genuine destruction factory input and retain its required BEFORE.

Run inside tools/run_test.py. The real fixture verdict is preserved, including
an unsupported native owner refusal. No AFTER banks or binaries are retained.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from retain_native_contact_agent_factory_before_tu15 import sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--case', required=True)
    parser.add_argument('--owner', type=lambda value: int(value, 0), required=True)
    parser.add_argument('--exclude-class-query', type=lambda value: int(value, 0))
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--retain-reference', type=Path, required=True)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use a tools/run_test.py-owned finite command session')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    args.engine = args.engine.resolve(strict=True)
    args.map = args.map.resolve(strict=True)
    args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    retainer = Path(__file__).with_name('retain_native_contact_agent_factory_before_tu15.py').resolve()
    inputs = {str(path): sha(path) for path in (args.engine, args.map, retainer, Path(__file__).resolve())}
    capture = scratch / 'destruction-agent-factory'
    if capture.exists():
        raise ValueError('This owner already has an actual factory input capture')
    case_report = scratch / 'destruction-factory-capture-case.json'
    log = scratch / 'destruction-factory-capture.log'
    command = [str(args.engine), str(args.map), str(args.map.parent), args.case, str(case_report)]
    report = dict(capture_complete=False, fixture_passed=False, inputs=inputs, command=command,
        raw_capture=str(capture), original_guest_abi_captured=False, expected_after_consumed=False,
        required_reference=str(args.retain_reference.resolve()), runtime_binary_retained=False)
    receipt = args.output / 'capture.json'

    def save():
        receipt.write_text(json.dumps(report, indent=2) + '\n')

    save()
    env = dict(os.environ, HALO2_DESTRUCTION_AGENT_FACTORY_CAPTURE='1',
        HALO2_DESTRUCTION_AGENT_FACTORY_CAPTURE_OWNER=hex(args.owner),
        HALO2_DESTRUCTION_FIXED_CLOCK='1', HALO2_DESTRUCTION_INITIAL_POSE_READ_ONLY='1',
        HALO2_DESTRUCTION_SOUND_REQUESTS='1', SDL_AUDIODRIVER='dummy')
    if args.exclude_class_query is not None:
        env['HALO2_DESTRUCTION_AGENT_FACTORY_CAPTURE_EXCLUDE_CLASS_QUERY'] = hex(args.exclude_class_query)
    with log.open('w') as stream:
        code = subprocess.run(command, env=env, stdout=stream, stderr=subprocess.STDOUT, timeout=600).returncode
    report['fixture_exit_code'] = code
    with log.open('rb') as stream:
        stream.seek(max(0, log.stat().st_size - 65536))
        (args.output / 'fixture.log').write_bytes(stream.read())
    if case_report.is_file():
        if case_report.stat().st_size > 1024 * 1024:
            raise ValueError('Actual destruction fixture report exceeds its bounded budget')
        shutil.copyfile(case_report, args.output / case_report.name)
        case = json.loads(case_report.read_text())
        report['fixture_passed'] = case.get('passed', False)
        report['fixture_error'] = case.get('error')
        report['fixture_report_sha256'] = sha(case_report)
    manifest = capture / 'capture-native-before.json'
    if manifest.is_file():
        data = json.loads(manifest.read_text())
        if (data['input']['owner'] != args.owner or data['physics_result_supplied'] or
                data['expected_after_consumed']):
            raise ValueError('Actual factory input has a different owner or supplied result')
        if args.exclude_class_query is not None and data['actual_second_shape']['class_query_owner'] == args.exclude_class_query:
            raise ValueError('The capture did not reach the requested new class-query domain')
        retain = [sys.executable, '-B', str(retainer), '--capture', str(capture),
            '--output', str(args.retain_reference.resolve())]
        subprocess.run(retain, check=True)
        report['retention_command'] = retain
        report['capture_complete'] = True
        report['actual_input'] = data['input']
        report['actual_second_shape'] = data.get('actual_second_shape')
        report['retention_receipt_sha256'] = sha(args.retain_reference / 'retention.json')
    report['inputs_unchanged'] = all(sha(path) == digest for path, digest in inputs.items())
    save()
    if not report['inputs_unchanged']:
        raise ValueError('Actual factory capture input changed during execution')
    print(json.dumps(dict(capture_complete=report['capture_complete'], fixture_exit_code=code,
        actual_second_shape=report.get('actual_second_shape'), report=str(receipt))))
    return code or (0 if report['capture_complete'] else 1)


if __name__ == '__main__':
    raise SystemExit(main())
