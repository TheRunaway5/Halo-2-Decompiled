#!/usr/bin/env python3
"""Compare actual linked Primary and CombatAction format-four source exports."""
import argparse
import filecmp
import hashlib
import json
import os
import subprocess
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_PIN
from validate_native_codec_format_four_decode_tu15 import inputs


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--format', choices=('raw', 'prepared'), required=True)
    parser.add_argument('--full', action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(passed=False, private_overlay=True, owned_scratch=str(scratch), source_format=args.format,
                  retained_large_artifacts=[], runs={})
    try:
        if digest(args.xbe) != NEW_PIN:
            raise ValueError('Wrong original format-four TU1.5 image')
        rows = inputs(Xbe(args.xbe), not args.full)
        fixture = scratch / 'linked-inputs.json'
        fixture.write_text(json.dumps(rows, separators=(',', ':')))
        report.update(input_count=len(rows), input_sha256=digest(fixture), linked_binary_sha256=digest(args.native))
        expected_cases = len(rows) * (12 if args.format == 'prepared' else 2)
        environment = {key: value for key, value in os.environ.items()
                       if not key.startswith(('HALO2_REFERENCE_', 'HALO2_VERIFY_'))}
        controls = (('default', {}), ('own-reference', {'HALO2_REFERENCE_CODEC_FORMAT_FOUR_DECODE': '1'}),
                    ('unrelated-reference', {'HALO2_REFERENCE_PACKED_ROTATION': '1'}))
        baseline = None
        for name, override in controls:
            output = scratch / (name + '.stream')
            command = [str(args.native), str(fixture), str(output), args.format]
            result = subprocess.run(command, env={**environment, **override}, capture_output=True, text=True, timeout=1200)
            if result.returncode:
                raise ValueError('Actual linked format-four ' + name + ' failed: ' + (result.stdout + result.stderr)[-6000:])
            counts = json.loads(result.stdout)
            expected_native = 0 if name == 'own-reference' else expected_cases
            if (counts['cases'] != expected_cases or counts['native_entries'] != expected_native or
                counts['external_child_returns'] <= 0 or
                (args.format == 'prepared' and (counts['exceptions'] != len(rows) * 4 or counts['translation_groups'] <= 0))):
                raise ValueError('Actual linked format-four route accounting differs: ' + str(counts))
            report['runs'][name] = dict(command=command, controls=override, result=counts,
                                        stream_sha256=digest(output), stream_bytes=output.stat().st_size)
            if baseline is None:
                baseline = output
            elif not filecmp.cmp(baseline, output, shallow=False):
                raise ValueError('Actual linked format-four ABI/provider/observer/bank stream differs in ' + name)
        report.update(passed=True, complete_streams_equal=True)
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        (args.output / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
