"""The streamed oracle must preserve coverage, comparison order and failures."""
from contextlib import ExitStack, redirect_stdout
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
SPEC = importlib.util.spec_from_file_location(
    'streaming_validator_under_test', ROOT / 'tools/validate_native_campaign_script_builtins_tu15.py')
validator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(validator)


class StreamingTests(unittest.TestCase):
    def run_pipeline(self, shard='0/1', failure=None, reorder=False):
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
            scratch = Path(directory)
            original = scratch / 'original.xbe'
            original.write_bytes(b'fixture')
            report = scratch / 'report.json'
            first_released = threading.Event()
            lock = threading.Lock()
            observed, compared, completion_order = [], [], []

            def prepare(*unused):
                owned = scratch / 'physical'
                owned.mkdir()
                for name in ('observer', 'reference', 'metadata.json'):
                    (owned / name).write_bytes(b'fixture')
                return owned

            def generate(*unused):
                for case in range(9):
                    yield dict(id=case, kernel='fixture')

            base_cases = validator.cases

            def wrapped(*args):
                rows = base_cases(*args)
                for row in rows:
                    row['wrapped'] = True
                if reorder:
                    rows.reverse()  # An in-place change must also be rejected.
                return rows

            def physical(lane, rows):
                self.assertTrue((lane / 'observer').is_file())
                self.assertTrue(all(row.get('wrapped') for row in rows))
                start = rows[0]['id']
                if shard == '0/1':
                    if start == 0:
                        self.assertTrue(first_released.wait(5), 'second observer did not run')
                if failure == 'physical' and start == 2:
                    first_released.set()
                    raise RuntimeError('original observer failed')
                with lock:
                    observed.extend(row['id'] for row in rows)
                    completion_order.append(start)
                if start == 2:
                    first_released.set()
                return dict(cases=[dict(value=row['id'], banks=[], executed_sites=[row['id']])
                                   for row in rows], original_instructions=len(rows))

            def expected(row, original_row):
                self.assertEqual(row['id'], original_row['value'])
                return dict(value=row['id'], writes=[])

            def compare(rows, golds, native, total):
                if failure == 'native' and rows[0]['id'] == 2:
                    raise ValueError('native comparison failed')
                for row, gold in zip(rows, golds, strict=True):
                    self.assertEqual(row['id'], gold['value'])
                    compared.append(row['id'])
                    for _ in range(2):
                        total.update(json.dumps(gold, sort_keys=True).encode())
                return dict(comparisons=len(rows) * 2, comparison_sha256=total.hexdigest())

            replacements = dict(
                load_tracks=lambda: None, KERNELS={'fixture': (1, 'void')},
                ROUTINES={'fixture': {}}, NEW_DEFAULT=original,
                Xbe=lambda path: None, sha=lambda path: validator.NEW_PIN,
                registrations=lambda image: {'fixture': dict(evaluator=1, routine=True)},
                metadata=lambda *args: (dict(owners=[]), b''), prepare=prepare,
                generate_cases=generate, cases=wrapped, physical=physical,
                expected=expected, compare=compare, BATCH_CASES=2, OBSERVERS=2)
            argv = ['validator', '--native', str(scratch / 'native'),
                    '--report', str(report), '--shard', shard]
            with ExitStack() as patches, redirect_stdout(io.StringIO()):
                patches.enter_context(patch.dict(os.environ, HALO2_TEST_SCRATCH=str(scratch)))
                patches.enter_context(patch.object(sys, 'argv', argv))
                for name, value in replacements.items():
                    patches.enter_context(patch.object(validator, name, value))
                if failure or reorder:
                    with self.assertRaisesRegex((ValueError, RuntimeError),
                                                'failed|added, dropped or reordered'):
                        validator.main()
                    self.assertFalse(report.exists(), 'failure published a passing report')
                    return
                self.assertEqual(validator.main(), 0)
            return json.loads(report.read_text()), observed, compared, completion_order

    def test_out_of_order_observers_keep_case_and_digest_order(self):
        report, observed, compared, completed = self.run_pipeline()
        self.assertEqual(sorted(observed), list(range(9)))
        self.assertEqual(compared, list(range(9)))
        self.assertLess(completed.index(2), completed.index(0))
        digest = hashlib.sha256()
        for case in range(9):
            for _ in range(2):
                digest.update(json.dumps(dict(value=case, writes=[]), sort_keys=True).encode())
        self.assertEqual(report['comparison_sha256'], digest.hexdigest())
        self.assertEqual(report['physical_cases'], 9)
        self.assertEqual(report['comparisons'], 18)

    def test_shards_partition_the_identical_generated_case_sequence(self):
        observations = []
        for shard in ('0/3', '1/3', '2/3'):
            report, observed, compared, _ = self.run_pipeline(shard)
            self.assertEqual(report['generated_cases'], 9)
            self.assertEqual(compared, sorted(observed))
            observations.extend(observed)
        self.assertEqual(sorted(observations), list(range(9)))

    def test_physical_and_native_failures_cannot_publish_acceptance(self):
        for failure in ('physical', 'native'):
            with self.subTest(failure=failure):
                self.run_pipeline(failure=failure)

    def test_in_place_case_reordering_is_rejected(self):
        self.run_pipeline(reorder=True)


if __name__ == '__main__':
    unittest.main()
