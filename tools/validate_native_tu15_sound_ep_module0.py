#!/usr/bin/env python3
"""Compare the public module0 implementation with retained original values."""
import argparse
import gzip
import json
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--vectors', type=Path, required=True)
parser.add_argument('--report', type=Path, required=True)
args = parser.parse_args()
with gzip.open(args.vectors, 'rt') as stream:
    corpus = json.load(stream)
if corpus['schema'] != 'H2_EP_MODULE0_ORIGINAL_VALUES_V1':
    raise ValueError('Unexpected module0 corpus schema')
expected_calls = len(corpus['fixtures']['cases']) * corpus['fixtures']['frame_count']
if expected_calls != len(corpus['original_observations']):
    raise ValueError('Missing original observations')
with tempfile.TemporaryDirectory(prefix='halo2-ep-module0-values-') as temporary:
    work = Path(temporary)
    fixtures = work / 'fixtures.json'
    observations = work / 'observations.jsonl'
    report_path = work / 'report.json'
    fixtures.write_text(json.dumps(corpus['fixtures']) + '\n')
    observations.write_text(''.join(json.dumps(row) + '\n'
                                    for row in corpus['original_observations']))
    result = subprocess.run([str(args.probe.resolve()), str(fixtures),
                             str(observations), str(report_path)],
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    report = json.loads(report_path.read_text())
    if (not report['passed'] or report['retained_native_calls'] != expected_calls
            or report['exact_output_diagnostic_history_words'] != expected_calls * 594
            or report['actual_packed_bytes'] != expected_calls * 1024
            or report['history_refilled_between_calls'] or report['Ready']):
        raise ValueError('Incomplete module0 comparison')
    report['source_execution_abi_bits'] = 32
    report['native_resource_owner_bits'] = 64
    report['complete_native_EP_status_install_clock_or_initializer45_ready'] = False
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
