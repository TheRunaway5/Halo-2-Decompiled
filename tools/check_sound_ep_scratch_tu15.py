#!/usr/bin/env python3
"""Compare unchanged original scratch/EP callers in disposable owned storage."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--reference-python', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    base = root / 'tests/fixtures/sound_gp_upload_tu15'
    previous = root / 'tests/fixtures/sound_gp_ep_append_bridge_tu15'
    if not args.reference_python.is_file() or not (base / 'report.json').is_file():
        return 77
    result = dict(passed=False, stages=[], retained_large_artifacts=[], whole_engine_verified=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    try:
        # The outer CTest wrapper provides disk scratch and cancellation
        # cleanup; this context also owns cleanup for direct helper invocation.
        with tempfile.TemporaryDirectory(prefix='ep-original-', dir=os.environ.get('HALO2_TEST_SCRATCH')) as name:
            scratch = Path(name)
            result['scratch'] = str(scratch)

            def guarded(stage, command):
                output = scratch / ('guard-' + stage)
                status = subprocess.run([sys.executable, str(root / 'tools/tag_match_memory_budget.py'),
                    '--output-root', str(output), '--timeout', '120', '--', *map(str, command)]).returncode
                receipts = list(output.glob('*/memory-budget.json'))
                result['stages'].append(dict(name=stage, status=status,
                    memory=json.loads(receipts[0].read_text()) if len(receipts) == 1 else None))
                if status:
                    raise RuntimeError(f'Original EP comparison stage {stage} failed: {status}')

            guarded('original', [args.reference_python, root / 'tools/oracle_sound_gp_scratch_tu15.py',
                '--upload-fixtures', base, '--output', scratch / 'original'])
            report = json.loads((scratch / 'original/report.json').read_text())
            if (previous / 'report.json').is_file():
                prior = json.loads((previous / 'report.json').read_text())
                for case in prior['cases']:
                    assert (previous / case['file']).read_bytes() == (scratch / 'original' / case['file']).read_bytes()
                result['previous_cases_unchanged'] = len(prior['cases'])
            cases = [json.loads((scratch / 'original' / row['file']).read_text()) for row in report['cases']]
            selector = [row for row in cases if row['entry'] == 0x38f544]
            assert len(cases) == 79 and len(selector) == 9
            assert not report['instruction_patches'] and not report['supplied_external_results']
            guarded('native', [args.native, scratch / 'original'])
            result.update(passed=True, original_cases=len(cases), native_comparisons=2 * len(cases),
                selector_cases=len(selector), ordered_stores=sum(row['store_count'] if 'store_count' in row else len(row['stores']) for row in cases),
                oracle_sha256=hashlib.sha256((root / 'tools/oracle_sound_gp_scratch_tu15.py').read_bytes()).hexdigest(),
                original_report_sha256=hashlib.sha256((scratch / 'original/report.json').read_bytes()).hexdigest())
        result['scratch_removed'] = not Path(result['scratch']).exists()
        print(json.dumps({key: result[key] for key in ('passed', 'original_cases', 'native_comparisons',
            'ordered_stores', 'scratch_removed', 'whole_engine_verified')}))
        return 0
    finally:
        args.report.write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
