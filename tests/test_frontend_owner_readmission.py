"""Real file-identity races; synthetic capabilities do not prove rendered menus."""
import hashlib
import json
import os
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import native_gameplay_test as admission
import qualify_frontend_runtime as qualification
import test_frontend_capability as fixtures


class FrontendOwnerReadmissionTests(unittest.TestCase):
    def setUp(self):
        fixture = fixtures.FrontendCapabilityTests()
        fixture.setUp()
        self.addCleanup(fixture.doCleanups)
        self.fixture = fixture
        self.root, self.runtime, self.cache = fixture.root, fixture.runtime, fixture.cache
        self.receipt = self.root / 'frame-report.json'
        self.receipt.write_text('{"synthetic":true}\n')
        fixture.report['validation']['report_sha256'] = fixture.digest(self.receipt)
        fixture.sidecar.write_text(json.dumps(fixture.report, indent=2) + '\n')
        self.target = self.cache / 'halo2/h2_fonts/fixture.font'
        self.alias = self.root / 'temporary-hardlink'
        self.baseline_sidecar = fixture.sidecar.read_bytes()
        self.baseline_report = self.receipt.read_bytes()

    def readmit(self):
        qualification._readmit_frontend_owner(
            self.root, self.runtime, self.cache, self.fixture.sidecar,
            self.receipt, self.fixture.report)

    def diagnostic(self):
        return json.loads((self.root / 'frontend-owner-readmission.json').read_text())

    def drift(self, target, mutation, limit=None):
        """Mutate an actual file after hashing, before the original identity guard."""
        original = admission._file_digest
        original_hash = hashlib.file_digest
        calls = []

        def wrapped(path, **kwargs):
            if Path(path) != target or (limit is not None and len(calls) >= limit):
                return original(path, **kwargs)

            def hashed(stream, *args, **options):
                result = original_hash(stream, *args, **options)
                mutation()
                calls.append(str(path))
                return result

            with patch.object(admission.hashlib, 'file_digest', side_effect=hashed):
                return original(path, **kwargs)

        return patch.object(admission, '_file_digest', side_effect=wrapped), calls

    def hardlink_churn(self):
        os.link(self.target, self.alias)
        self.alias.unlink()

    def test_unchanged_owner_passes_without_rewriting_evidence(self):
        self.readmit()
        self.assertEqual([True], [a['passed'] for a in self.diagnostic()['attempts']])
        self.assertEqual(self.fixture.sidecar.read_bytes(), self.baseline_sidecar)
        self.assertEqual(self.receipt.read_bytes(), self.baseline_report)

    def test_real_hardlink_churn_retries_same_bytes_and_original_capability(self):
        context, calls = self.drift(self.target, self.hardlink_churn, limit=1)
        with context:
            self.readmit()
        result = self.diagnostic()
        self.assertTrue(result['passed'])
        self.assertEqual(len(calls), 1)
        self.assertEqual(len(result['attempts']), 2)
        failure = result['attempts'][0]
        self.assertEqual(failure['path'], str(self.target))
        self.assertNotEqual(failure['before'], failure['after_fd'])
        self.assertEqual(failure['before'][:4], failure['after_fd'][:4])
        self.assertEqual(self.fixture.sidecar.read_bytes(), self.baseline_sidecar)
        self.assertEqual(self.receipt.read_bytes(), self.baseline_report)

    def test_continuous_hardlink_churn_refuses_after_three_checks(self):
        context, calls = self.drift(self.target, self.hardlink_churn)
        with context, self.assertRaises(admission.NativeTestOwnerChanged):
            self.readmit()
        result = self.diagnostic()
        self.assertFalse(result['passed'])
        self.assertEqual(len(calls), 3)
        self.assertEqual([True, True, False], [a['retry_allowed'] for a in result['attempts']])

    def test_changed_bytes_are_never_repinned_by_retry(self):
        context, _ = self.drift(self.target, lambda: self.target.write_bytes(b'changed content'), limit=1)
        with context, self.assertRaisesRegex(RuntimeError, 'Frontend owner changed'):
            self.readmit()
        self.assertFalse(self.diagnostic()['passed'])
        self.assertEqual(len(self.diagnostic()['attempts']), 2)
        self.assertEqual(self.fixture.sidecar.read_bytes(), self.baseline_sidecar)

    def test_runtime_identity_drift_is_not_a_cache_resource_retry(self):
        def runtime_churn():
            os.link(self.runtime, self.alias)
            self.alias.unlink()

        context, calls = self.drift(self.runtime, runtime_churn, limit=1)
        with context, self.assertRaises(admission.NativeTestOwnerChanged):
            self.readmit()
        result = self.diagnostic()
        self.assertEqual(len(calls), 1)
        self.assertEqual(len(result['attempts']), 1)
        self.assertFalse(result['attempts'][0]['retry_allowed'])

    def test_replaced_capability_during_drift_refuses_without_repinning(self):
        def change_capability():
            self.hardlink_churn()
            self.fixture.sidecar.write_bytes(self.baseline_sidecar + b' ')

        context, _ = self.drift(self.target, change_capability, limit=1)
        with context, self.assertRaisesRegex(RuntimeError, 'receipt or capability changed'):
            self.readmit()
        self.assertFalse(self.diagnostic()['passed'])
        self.assertEqual(len(self.diagnostic()['attempts']), 2)


if __name__ == '__main__':
    unittest.main()
