#!/usr/bin/env python3
"""Join qualified newer delivery bytes after a recorded native regression.

Only independent changes relative to the recorded common source are admitted.
The incoming production source and current selection must remain unchanged.
"""
import argparse
import json
import os
import re
from pathlib import Path
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('prepared', 'regression', 'staged', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = owner / 'source'
    prepared = json.loads(args.prepared.read_text())
    regression = json.loads(args.regression.read_text())
    stage = json.loads(args.staged.read_text())
    binding = regression.get('candidate_vehicle_regression', {})
    test_log=(args.regression.parent/'tests.log').read_text()
    completed=re.search(r'(?:100% tests passed(?:, 0 tests failed)?|99% tests passed, 2 tests failed) out of 374',test_log)
    if (Path(prepared['owner']) != owner or not completed or
            binding.get('prepared_sha256') != sha(args.prepared) or
            binding.get('expected_cases') != 374 or
            not binding.get('source_unchanged_after_run') or not binding.get('incoming_CMake_restored')):
        raise ValueError('Record the same complete unchanged native vehicle cohort before source integration')
    if (stage['prepared_sha256'] != sha(args.prepared) or
            sha(ROOT / 'gameplay-runtime-selection.json') != stage['selection_sha256'] or
            sha(Path(stage['selected_runtime'])) != stage['runtime_sha256']):
        raise ValueError('Selected delivery changed since independent source staging')
    staged = Path(stage['source']).resolve(strict=True)
    if staged.parent.parent != ROOT / '.local/test-scratch' or staged.is_relative_to(owner):
        raise ValueError('Use the independent managed staging owner')
    pins = dict(prepared['source_hashes'])
    if any(sha(source / name) != pin for name, pin in pins.items()):
        raise ValueError('Frozen vehicle source changed after its full regression')
    changes = stage['changes']
    for row in changes:
        if pins.get(row['path']) != row['before'] or sha(staged / row['path']) != row['after']:
            raise ValueError('A staged independent source preimage changed')
    args.output.mkdir(parents=True, exist_ok=False)
    for row in changes:
        path = source / row['path']
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes((staged / row['path']).read_bytes())
        pins[row['path']] = row['after']
    if any(sha(source / name) != pin for name, pin in pins.items()):
        raise ValueError('Another source changed while joining independent delivery bytes')
    report = dict(prepared, status='prepared', passed=False, validation_not_run=True,
                  source_hashes=pins, changes=changes, previous_prepared_sha256=sha(args.prepared),
                  complete_native_regression_sha256=sha(args.regression), staged_inputs_sha256=sha(args.staged),
                  previous_regression_passed=regression['passed'],qualification_still_required=True,
                  selected_delivery_sha256=stage['selection_sha256'], selected_delivery=stage['selected_runtime'],
                  full_vehicle_parity=False, tool_sha256=sha(Path(__file__)))
    (args.output / 'prepared.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(status='joined', files=len(changes), output=str(args.output))), flush=True)


if __name__ == '__main__':
    main()
