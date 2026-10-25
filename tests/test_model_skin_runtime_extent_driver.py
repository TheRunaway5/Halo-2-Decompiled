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
from run_model_skin_runtime import FALLBACK_COUNTERS, GPU_COUNTERS, parse_extent_tsv, presented_ppm_evidence, strict_case, strict_common_cut_case


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

    def common_cut_report(self):
        report = copy.deepcopy(self.report)
        report.update(passed=False, control_only=True, ordinary_gpu_acceptance=False, actual_gpu_acceptance_run=False,
                      post_constructor_common_cut=True, post_constructor_qualification_passed=True,
                      qualification_kind='post-constructor-common-cut')
        case = report['cases'][0]
        case.update(passed=False, control_only=True, players=1, post_constructor_common_cut=True,
                    post_constructor_qualification_passed=True, common_cut_released_after_pair=True,
                    passive_source_trace_identical=True, external_input_tape_identical=True)
        for role in ('cpu', 'gpu'):
            result = case[role]
            result.update(actual_delivered_commands_per_view=60,
                          source_rng_construction=dict(receipts=1, auxiliary=1, pre_common_cut_observation=True,
                                                       compared_in_ordered_trace=False),
                          presentation_worker=dict(frames_with_copy_jobs=60, frames_with_actual_worker_jobs=60),
                          cpu_fallback={**{key: 0 for key in FALLBACK_COUNTERS}, 'vulkan_results': {}})
            result['post_constructor_common_cut'] = dict(enabled=True, role='export' if role == 'cpu' else 'import',
                completed_constructor_and_local_views=True, before_preparation_projection_and_runtime=True,
                local_test_binding_only=True, constructor_and_teardown_outside_tape=True,
                tape_bound_after_initial_proof=True, tape_unbound_before_native_teardown=True,
                normal_coupled_publication=role == 'gpu', tick=1,
                initial_bank_proof=dict(passed=True, regions=1, complete_bank_bytes=100, committed_ranges=1,
                    committed_bytes=100, expected_committed_ranges=1, expected_committed_bytes=100,
                    complete_topology_identical=True, complete_region_bytes_identical=True,
                    committed_run_segmentation_identical=True, committed_bytes_identical=True, no_resource_exclusion=True),
                resource_limits=dict(bank_bytes=100, regions=1, arena_file_bytes=0, arena_files=2,
                    bank_byte_limit=1024 ** 3, arena_file_byte_limit=64 * 1024 ** 2, region_limit=8192))
            result['external_source_inputs'] = dict(mode='record' if role == 'cpu' else 'replay', entries=2,
                semantic_sha256='3' * 64, kind_counts=[2] + [0] * 8, constructor_through_world_teardown=False,
                complete_call_journal=False, post_constructor_common_cut=True, constructor_outside_scope=True,
                native_teardown_outside_scope=True, complete_consumption=True, actual_values_preserved=True)
            result['passive_source_trace'] = dict(schema='H2OTR02', initial_source_boundary='post-constructor-common-cut',
                complete_storage=True, bytes=200, sha256='4' * 64, source_callbacks=1, preparation_checkpoints=60,
                final_checkpoints=1, input_callbacks=60, view_callbacks=60, rectangle_callbacks=60,
                node_stage_counts=[60, 60, 0, 0])
        return report

    def check_common_cut(self, report):
        return strict_common_cut_case(report, self.name, *self.internal, self.presented, self.root)

    def test_common_cut_has_only_separate_qualification(self):
        report = self.common_cut_report()
        self.assertIsNone(self.check_common_cut(report))
        self.assertIsNotNone(self.check_report(report))
        # Even forged normal success cannot make the old checker accept it.
        report.update(passed=True, control_only=False)
        report['cases'][0].update(passed=True, control_only=False)
        self.assertIsNotNone(self.check_report(report))
        self.assertIsNotNone(self.check_common_cut(report))

    def test_common_cut_measured_region_cap_accepts8192_rejects8193(self):
        for count in (8192, 8193):
            with self.subTest(regions=count):
                report = self.common_cut_report()
                for role in ('cpu', 'gpu'):
                    cut = report['cases'][0][role]['post_constructor_common_cut']
                    cut['initial_bank_proof'].update(regions=count, complete_bank_bytes=count,
                        committed_ranges=count, committed_bytes=count,
                        expected_committed_ranges=count, expected_committed_bytes=count)
                    cut['resource_limits'].update(regions=count, bank_bytes=count)
                if count == 8192:
                    self.assertIsNone(self.check_common_cut(report))
                else:
                    self.assertIsNotNone(self.check_common_cut(report))
        report = self.common_cut_report()
        for role in ('cpu', 'gpu'):
            report['cases'][0][role]['post_constructor_common_cut']['resource_limits']['region_limit'] = 4096
        self.assertIsNotNone(self.check_common_cut(report))

    def test_common_cut_rejects_missing_or_false_scoped_evidence(self):
        original = self.common_cut_report()
        for role in ('cpu', 'gpu'):
            for key in ('enabled', 'completed_constructor_and_local_views', 'before_preparation_projection_and_runtime',
                        'local_test_binding_only', 'constructor_and_teardown_outside_tape',
                        'tape_bound_after_initial_proof', 'tape_unbound_before_native_teardown'):
                with self.subTest(role=role, key=key):
                    report = copy.deepcopy(original)
                    report['cases'][0][role]['post_constructor_common_cut'][key] = False
                    self.assertIsNotNone(self.check_common_cut(report))
            for key in ('passed', 'complete_topology_identical', 'complete_region_bytes_identical',
                        'committed_run_segmentation_identical', 'committed_bytes_identical', 'no_resource_exclusion'):
                with self.subTest(role=role, proof=key):
                    report = copy.deepcopy(original)
                    del report['cases'][0][role]['post_constructor_common_cut']['initial_bank_proof'][key]
                    self.assertIsNotNone(self.check_common_cut(report))

    def test_common_cut_rejects_different_starting_bank_or_missing_publication(self):
        for role in ('cpu', 'gpu'):
            report = self.common_cut_report()
            report['cases'][0][role]['post_constructor_common_cut']['initial_bank_proof']['committed_bytes'] = 99
            self.assertIsNotNone(self.check_common_cut(report))
        report = self.common_cut_report()
        report['cases'][0]['gpu']['post_constructor_common_cut']['normal_coupled_publication'] = False
        self.assertIsNotNone(self.check_common_cut(report))
        report = self.common_cut_report()
        report['cases'][0]['common_cut_released_after_pair'] = False
        self.assertIsNotNone(self.check_common_cut(report))

    def test_common_cut_rejects_lifetime_overclaim_and_tape_divergence(self):
        for key, value in (('constructor_through_world_teardown', True), ('constructor_outside_scope', False),
                           ('native_teardown_outside_scope', False), ('semantic_sha256', '5' * 64),
                           ('kind_counts', [0] * 9), ('mode', 'record')):
            with self.subTest(key=key):
                report = self.common_cut_report()
                report['cases'][0]['gpu']['external_source_inputs'][key] = value
                self.assertIsNotNone(self.check_common_cut(report))
        report = self.common_cut_report()
        report['cases'][0]['gpu']['source_rng_construction']['compared_in_ordered_trace'] = True
        self.assertIsNotNone(self.check_common_cut(report))

    def test_common_cut_still_requires_actual_gpu_worker_and_complete_trace(self):
        for group, key, value in ((None, 'gpu_weighted_vertices', 0), (None, 'gpu_verified_vertices', 599),
                                  ('presentation_worker', 'frames_with_actual_worker_jobs', 0),
                                  ('passive_source_trace', 'node_stage_counts', [0, 0, 0, 0]),
                                  ('passive_source_trace', 'sha256', '6' * 64),
                                  ('cpu_fallback', 'jobs', 1)):
            with self.subTest(group=group, key=key):
                report = self.common_cut_report()
                result = report['cases'][0]['gpu']
                (result[group] if group else result)[key] = value
                self.assertIsNotNone(self.check_common_cut(report))
        path = self.root / (self.name + '-gpu') / 'final.ppm'
        data = bytearray(path.read_bytes());data[-1] ^= 1;path.write_bytes(data)
        self.assertIsNotNone(self.check_common_cut(self.common_cut_report()))


if __name__ == '__main__':
    unittest.main()
