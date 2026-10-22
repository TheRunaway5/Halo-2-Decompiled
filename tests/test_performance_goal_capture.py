#!/usr/bin/env python3
"""Prevent incomplete quality evidence from qualifying a performance cohort."""
import csv
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    'performance_goal', ROOT / 'tools/validate_performance_goal.py')
goal = importlib.util.module_from_spec(spec)
spec.loader.exec_module(goal)


class CaptureAdmission(unittest.TestCase):
    def setUp(self):
        folder = tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(folder.cleanup)
        self.directory = Path(folder.name)
        self.image = self.directory / 'capture.ppm'
        self.timing = self.directory / 'timing.tsv'
        self.rgb = bytes(range(12))
        self.image.write_bytes(b'P6\n2 2\n255\n' + self.rgb)
        self.write_timing((2, 2), (2, 2))

    def write_timing(self, render, present):
        with self.timing.open('w') as stream:
            writer = csv.writer(stream, delimiter='\t')
            writer.writerow(('render_width', 'render_height',
                             'present_width', 'present_height'))
            writer.writerow((*render, *present))

    def test_complete_full_size_capture(self):
        rgb, sha256 = goal.read_capture(self.image, self.timing, [2, 2])
        self.assertEqual(rgb, self.rgb)
        self.assertEqual(sha256, goal.digest(self.image))

    def test_clamped_presentation_rejected_even_with_full_size_scene(self):
        self.write_timing((2, 2), (1, 1))
        with self.assertRaisesRegex(ValueError, 'full-size scene and presentation'):
            goal.read_capture(self.image, self.timing, [2, 2])

    def test_truncated_rgb_rejected(self):
        self.image.write_bytes(self.image.read_bytes()[:-1])
        with self.assertRaisesRegex(ValueError, 'complete requested internal resolution'):
            goal.read_capture(self.image, self.timing, [2, 2])

    def test_capture_without_timing_samples_rejected(self):
        self.timing.write_text('render_width\trender_height\tpresent_width\tpresent_height\n')
        with self.assertRaisesRegex(ValueError, 'full-size scene and presentation'):
            goal.read_capture(self.image, self.timing, [2, 2])

    def test_validation_error_before_retained_log_tail_rejected(self):
        log = self.directory / 'runtime.log'
        log.write_text('Validation Error: VUID-command-123\n' + 'normal log line\n' * 2000)
        self.assertNotIn('Validation Error', goal.tail(log))
        self.assertEqual(goal.validation_errors(log), 1)


if __name__ == '__main__':
    unittest.main()
