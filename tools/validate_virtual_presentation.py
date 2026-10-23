#!/usr/bin/env python3
"""Admit a virtual FIFO presentation provider using the actual SDL/Vulkan loop."""
import argparse
import csv
import json
import os
from pathlib import Path
import statistics
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = scratch / 'fifo-loop'
    argv = [str(args.runtime), 'fifo', '2', str(output)]
    try:
        result = subprocess.run(argv, capture_output=True, text=True, timeout=20)
    except subprocess.TimeoutExpired as error:
        def tail(value):
            return (value.decode(errors='replace') if isinstance(value, bytes) else value or '')[-16000:]
        report = {'argv': argv, 'passed': False, 'status': 'timeout', 'timeout_seconds': 20,
                  'virtual': True, 'physical_latency_proven': False,
                  'stdout': tail(error.stdout), 'stderr': tail(error.stderr)}
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report))
        return 1
    report = {'argv': argv, 'exit_code': result.returncode, 'passed': False,
              'virtual': True, 'physical_latency_proven': False,
              'stdout': result.stdout[-16000:], 'stderr': result.stderr[-16000:]}
    if result.returncode == 0:
        sample = json.loads((output / 'result.json').read_text())
        rows = list(csv.DictReader((output / 'timings.tsv').open(), delimiter='\t'))[5:]
        intervals = [float(r['interval_ms']) for r in rows]
        acquire = [float(r['acquire_ms']) for r in rows]
        errors = sum('Validation Error' in s or 'VUID-' in s for s in (result.stdout + result.stderr).splitlines())
        report.update(sample=sample, measured_frames=len(rows), validation_errors=errors)
        if rows:
            report.update(mean_interval_ms=statistics.mean(intervals),
                          median_acquire_ms=statistics.median(acquire),
                          fps=1000 / statistics.mean(intervals))
            report['passed'] = (len(rows) >= 240 and sample['mode'] == 'fifo'
                and sample['device'] == 'NVIDIA GeForce RTX 5080'
                and report['fps'] >= 120 and report['median_acquire_ms'] < 20 and errors == 0)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
