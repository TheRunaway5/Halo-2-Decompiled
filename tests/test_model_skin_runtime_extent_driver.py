#!/usr/bin/env python3
"""Reject false internal/presented extent claims without launching a game."""
import copy
import os
from pathlib import Path
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from run_model_skin_runtime import GPU_COUNTERS, parse_extent_tsv, presented_ppm_evidence, strict_case


class ModelSkinExtentDriver(unittest.TestCase):
    name = 'spartan-1p-neutral'
    internal = (3840, 2160)
    presented = (64, 64)  # Bounded fixture; no physical framebuffer claim.

    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='halo2-skin-extents-', dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.header = 'frame\tinterval_ms\trender_width\trender_height\tpresent_width\tpresent_height\n'
        self.rows = [f'{frame}\t0.0\t3840\t2160\t64\t64\n' for frame in range(60)]
        result = dict(input_callbacks=60, submitted_frames=60, validation_messages=0,
                      render_width=3840, render_height=2160, present_width=64, present_height=64,
                      dual_extent_mode=True, framebuffer_extent_kind='presented')
        case = dict(name=self.name, passed=True, full_frame_identical=True, ordered_trace_identical=True)
        for role in ('cpu', 'gpu'):
            directory = self.root / (self.name + '-' + role)
            directory.mkdir()
            (directory / 'render-timings.tsv').write_text(self.header + ''.join(self.rows))
            (directory / 'final.ppm').write_bytes(b'P6\n64 64\n255\n' + b'\x13\x27\x3b' * (64 * 64))
            value = dict(result)
            value.update({key: 0 for key in GPU_COUNTERS})
            if role == 'gpu':
                value.update(gpu_frames=60, gpu_jobs=60, gpu_vertices=600, gpu_weighted_vertices=600, gpu_verified_vertices=600)
            value['frame_extent_evidence'] = parse_extent_tsv(directory / 'render-timings.tsv', 60, *self.internal, *self.presented)
            value.update(presented_ppm_evidence(directory / 'final.ppm', *self.presented))
            case[role] = value
        self.report = dict(passed=True, cases=[case], dual_extent_mode=True, framebuffer_extent_kind='presented',
                           requested_width=3840, requested_height=2160, requested_present_width=64, requested_present_height=64)

    def check_report(self, report=None, raw=True):
        return strict_case(report or self.report, self.name, *self.internal, self.presented, self.root if raw else None)

    def test_separate_extents_accept_complete_actual_evidence(self):
        self.assertIsNone(self.check_report())

    def test_every_missing_receipt_field_rejected(self):
        original = self.report['cases'][0]['gpu']['frame_extent_evidence']
        for key in original:
            with self.subTest(key=key):
                report = copy.deepcopy(self.report)
                del report['cases'][0]['gpu']['frame_extent_evidence'][key]
                self.assertIsNotNone(self.check_report(report, raw=False))

    def test_missing_or_wrong_role_timing_cannot_use_another_receipt(self):
        (self.root / (self.name + '-gpu') / 'render-timings.tsv').unlink()
        self.assertIsNotNone(self.check_report())

    def test_first_middle_or_last_presented_mismatch_rejected(self):
        path = self.root / (self.name + '-gpu') / 'render-timings.tsv'
        for frame in (0, 1, 30, 59):
            with self.subTest(frame=frame):
                rows = list(self.rows)
                rows[frame] = rows[frame].replace('\t64\t64\n', '\t63\t64\n')
                path.write_text(self.header + ''.join(rows))
                with self.assertRaisesRegex(RuntimeError, 'mismatched internal/presented'):
                    parse_extent_tsv(path, 60, *self.internal, *self.presented)
                self.assertIsNotNone(self.check_report())

    def test_actual_internal_mismatch_rejected(self):
        path = self.root / (self.name + '-gpu') / 'render-timings.tsv'
        path.write_text((self.header + ''.join(self.rows)).replace('\t3840\t2160\t', '\t1920\t1080\t'))
        self.assertIsNotNone(self.check_report())

    def test_missing_extra_duplicate_and_gapped_frames_rejected(self):
        path = self.root / (self.name + '-gpu') / 'render-timings.tsv'
        variants = [self.rows[:-1], self.rows + ['60\t0.0\t3840\t2160\t64\t64\n'],
                    self.rows[:1] + self.rows[:1] + self.rows[2:], self.rows[:3] + self.rows[4:]]
        for rows in variants:
            with self.subTest(rows=len(rows)):
                path.write_text(self.header + ''.join(rows))
                self.assertIsNotNone(self.check_report())

    def test_noninteger_incomplete_or_missing_columns_rejected(self):
        path = self.root / (self.name + '-gpu') / 'render-timings.tsv'
        data = self.header + ''.join(self.rows)
        variants = [data.replace('\t3840\t', '\t3840.0\t', 1), data.replace('\t3840\t', '\t-3840\t', 1),
                    data.replace('present_width', 'missing_width', 1), data.replace('present_height', 'present_width', 1),
                    data.replace(self.rows[-1], '59\t0.0\t3840\t2160\t64\n')]
        for value in variants:
            with self.subTest(value=value[:80]):
                path.write_text(value)
                self.assertIsNotNone(self.check_report())

    def test_truncated_extra_or_wrong_header_ppm_rejected(self):
        path = self.root / (self.name + '-gpu') / 'final.ppm'
        original = path.read_bytes()
        for value in (original[:-1], original + b'\0', original.replace(b'P6\n64 64\n', b'P6\n63 64\n', 1)):
            with self.subTest(length=len(value)):
                path.write_bytes(value)
                self.assertIsNotNone(self.check_report())

    def test_same_size_changed_ppm_cannot_use_old_frame_hash(self):
        path = self.root / (self.name + '-gpu') / 'final.ppm'
        data = bytearray(path.read_bytes());data[-1] ^= 1;path.write_bytes(data)
        self.assertIsNotNone(self.check_report())

    def test_forged_identical_flag_cannot_hide_different_actual_frame_hashes(self):
        path = self.root / (self.name + '-gpu') / 'final.ppm'
        data = bytearray(path.read_bytes());data[-1] ^= 1;path.write_bytes(data)
        self.report['cases'][0]['gpu'].update(presented_ppm_evidence(path, *self.presented))
        self.assertIsNotNone(self.check_report())

    def test_false_full_4k_capture_claim_rejected(self):
        for field, value in (('framebuffer_extent_kind', 'internal'), ('present_width', 3840),
                             ('rgb_payload_bytes', 3840 * 2160 * 3), ('ppm_header', 'P6\n3840 2160\n255\n')):
            with self.subTest(field=field):
                report = copy.deepcopy(self.report);report['cases'][0]['gpu'][field] = value
                self.assertIsNotNone(self.check_report(report))

    def test_reported_submitted_count_must_match_each_frame(self):
        report = copy.deepcopy(self.report);report['cases'][0]['gpu']['submitted_frames'] = 61
        self.assertIsNotNone(self.check_report(report))

    def test_legacy_default_remains_compatible_but_cannot_qualify_dual_mode(self):
        legacy = copy.deepcopy(self.report)
        for key in ('dual_extent_mode', 'framebuffer_extent_kind', 'requested_present_width', 'requested_present_height'):
            legacy.pop(key)
        for role in ('cpu', 'gpu'):
            for key in ('dual_extent_mode', 'framebuffer_extent_kind', 'present_width', 'present_height',
                        'frame_extent_evidence', 'ppm_header'):
                legacy['cases'][0][role].pop(key)
        self.assertIsNone(strict_case(legacy, self.name, *self.internal))
        self.assertIsNotNone(self.check_report(legacy))


if __name__ == '__main__':
    unittest.main()
