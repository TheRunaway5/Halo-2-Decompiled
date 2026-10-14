#!/usr/bin/env python3
"""Verify MCC-only routing in both launcher variants using isolated recorders.

Synthetic readiness metadata exercises argument routing only, not gameplay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHERS = ('Halo2-Launcher', 'Halo2-Title-Launcher')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/asset-setup/launcher-mcc.json')
    args = parser.parse_args()
    cases = []
    for name in LAUNCHERS:
        binary = args.build.resolve() / name
        result = subprocess.run([sys.executable, str(ROOT / 'tests/launcher_first_run_test.py'), str(binary), '-v'],
                                capture_output=True, text=True, timeout=120)
        cases.append(dict(launcher=name, sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                          passed=result.returncode == 0, output=result.stdout + result.stderr))
    report = dict(schema='halo2-launcher-mcc-only-v1', scope=__doc__, passed=all(c['passed'] for c in cases), cases=cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(('PASS' if report['passed'] else 'FAIL') + ': ' + str(args.output))
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
