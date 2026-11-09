#!/usr/bin/env python3
"""Final retained World contexts, counters and linked models on exact1480."""
from pathlib import Path
import hashlib
import inspect
import json
import os
import subprocess
import sys

import validate_native_moving_retained_full_context_tu15 as contexts
import validate_native_moving_retained_world_counters_tu15 as counters

ROOT = Path(__file__).resolve().parents[1]
STAGE = ROOT / 'analysis/physics/native-moving-shape-retained-world-stage-v2-20261102'
OUT = ROOT / 'analysis/physics/native-moving-retained-final-world-20261102-first'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    OUT.mkdir(exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    pins = json.loads((STAGE/'source-pins.json').read_text())
    report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[],
                  stage_source_sha256=pins['source_sha256'])
    try:
        for name, pin in pins['source_sha256'].items():
            assert sha(STAGE/'staged'/name) == pin, name
        source = inspect.getsource(contexts.main).replace(
            'native-moving-retained-full-context-20261102-first',
            'native-moving-retained-full-context-20261102-second')
        environment = dict(contexts.__dict__, STAGE=STAGE/'staged')
        exec(source, environment)
        assert environment['main']() == 0
        source = inspect.getsource(counters.main).replace(
            'native-moving-shape-retained-world-stage-20261102',
            'native-moving-shape-retained-world-stage-v2-20261102')
        environment = dict(counters.__dict__)
        exec(source, environment)
        prior = sys.argv
        sys.argv = [str(ROOT/'tools/validate_native_moving_retained_world_counters_tu15.py'),
                    '--stage', str(STAGE), '--output', str(OUT/'counters')]
        try:
            assert environment['main']() == 0
        finally:
            sys.argv = prior
        command = [sys.executable, str(ROOT/'tools/qualify_native_moving_shape_world_stage_tu15.py'),
                   '--stage', str(STAGE), '--output', str(OUT/'models')]
        run = subprocess.run(command)
        report.update(command=command, command_exit_code=run.returncode)
        assert run.returncode == 0
        for name, pin in pins['source_sha256'].items():
            assert sha(STAGE/'staged'/name) == pin, name
        report.update(passed=True, full_context_report='analysis/physics/native-moving-retained-full-context-20261102-second/report.json',
                      scope='Exact final1480 plus retained World v2: four complete syntax contexts, exact35 counters/twoJSON/gates and linked default/factory/merge World models. No live assertion.')
    except BaseException as error:
        report['error'] = str(error) or repr(error)
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(
            p.stat().st_blocks*512 for p in scratch.rglob('*') if p.is_file())
        (OUT/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
