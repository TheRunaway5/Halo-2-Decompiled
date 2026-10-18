#!/usr/bin/env python3
"""Publish bounded full-link/runtime evidence and verify disposable storage."""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


def main():
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    names = ['current-root-weapon-runtime-build-v1/report.json',
             'current-root-weapon-application-v1/report.json',
             'sound-cache-sector-scope-v1/report.json',
             'test-report-retention-regression-v1/report.json']
    build, application, sectors, retention = [json.loads((BASE / name).read_text()) for name in names]
    assert build['passed'] and build['status'] == 'passed'
    assert all(step['exit_code'] == 0 for step in build['steps'])
    tests = (BASE / 'current-root-weapon-runtime-build-v1/tests.log').read_text()
    expected = ('original_tu15_sound_cache_io', 'native_tu15_primary_weapon_cycle',
                'native_tu15_sound_submit_events')
    assert '100% tests passed out of 3' in tests or '100% tests passed, 0 tests failed out of 3' in tests
    assert all(name in tests for name in expected) and 'Skipped' not in tests
    assert application['passed'] and len(application['commands']) == 3
    assert all(row['exit_code'] == 0 and row['expected_found'] for row in application['commands'])
    assert sectors['audit_completed'] and sectors['map_count'] == 38
    assert sectors['counts']['nonempty_chunks'] == 347922
    assert sectors['all_nonempty_rounded_ranges_available'] and sectors['all_nonempty_raw_ranges_available']
    assert retention['passed'] and retention['scratch_removed']
    checks = []
    for name, record in zip(names, (build, application, sectors, retention)):
        path = Path(record['build']).parent if record.get('build') and name == names[0] else Path(record['runner_scratch'] if 'runner_scratch' in record else record['scratch'])
        assert path.is_relative_to(ROOT / '.local/test-scratch'), path
        assert not path.exists(), path
        checks.append(dict(path=str(path), report=name, removed=True))
    diagnostics = ['smg-current-native-firing-frontier-v1/report.json',
                   'smg-current-native-firing-frontier-v2/failure.json',
                   'smg-baseline-native-firing-frontier-v3/report.json']
    for name in diagnostics:
        record = json.loads((BASE / name).read_text())
        assert not record['passed']
        path = Path(record['runner_scratch'])
        assert not path.exists(), path
        checks.append(dict(path=str(path), report=name, removed=True))
    for row in build['retained_small_reports']:
        if row['status'] == 'retained':
            assert sha(row['retained_path']) == row['sha256']
    submissions_path = BASE / 'current-root-weapon-runtime-build-v1/evidence/sandbox-sound-submit-acceptance/source-sound-submissions.json'
    submissions = json.loads(submissions_path.read_text())
    assert len(submissions) == 18
    assert all(row['driver'] == 0 and row['result'] == 0xffffffff for row in submissions)
    provenance = json.loads((BASE / 'current-root-weapon-runtime-build-v1/concurrent-source-observations.json').read_text())
    assert not provenance['production_sound_cache_handler_changed']
    summary = dict(passed=True, timestamp_utc=now,
        full_application_link_passed=True, focused_CTest_checks_passed=list(expected),
        finite_actual_application_checks=['help', '120 ticks with 1 local player', '120 ticks with 4 local players'],
        headless_integration_not_weapon_input_or_renderer_or_PCM_proof=True,
        selected_sound_submission_report_retained=True,
        observed_sound_submissions=18, observed_sound_driver_values=[0],
        observed_sound_results={'0xffffffff': 18},
        sound_submissions_accepted_by_driver=False,
        full_world_sound_cache_read_handler_exercised_by_these_submission_tests=False,
        sound_resource_sector_audit_maps=38, sound_resource_nonempty_rows=347922,
        sound_resource_unique_ranges=sectors['unique_nonempty_resource_ranges'],
        all_nonempty_sound_rows_fit_bound_files_with_512_byte_rounding=True,
        sound_dependency_selection_and_playback_verified=False,
        complete_original_or_native_firing_verified=False,
        installed_runtime_verified=False, matched_final_weapon_frames_verified=False,
        immutable_whole_checkout_during_build=False,
        build_source_changes_report='current-root-weapon-runtime-build-v1/concurrent-source-observations.json',
        production_sound_cache_handler_unchanged_during_build=True,
        disposable_report_retention_regression_tests=2,
        evidence={name: sha(BASE / name) for name in names},
        incomplete_firing_diagnostics={name: sha(BASE / name) for name in diagnostics},
        original_entry_native_firing_frontier='129833 -> 1296E0 remains unavailable in the baseline-library composition; first fresh-library diagnostic also needed an existing Damage dispatcher route',
        new_large_reference_inputs=False, physical_xbox_verified=False, full_weapon_parity=False)
    summary_path = BASE / 'weapon-current-runtime-and-sound-sector-progress-v1.json'
    write(summary_path, summary)
    previous = BASE / 'weapon-completion-matrix-v44.json'
    matrix = json.loads(previous.read_text())
    for weapon in matrix['weapons']:
        weapon['bounded_evidence'].append(dict(
            report='sound-cache-sector-scope-v1/report.json', sha256=sha(BASE / names[2]),
            component='Literal sound gestalt chunk ranges in all 38 scoped maps fit the bound native file namespace and 512-byte reads. Weapon sound dependencies, selection and playback are not established.',
            component_covers_all_scoped_maps_for_weapon=True,
            certifies_entire_gate=False, certifies_all_map_variants=False))
    smg = next(w for w in matrix['weapons'] if w['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    smg['bounded_evidence'].append(dict(
        report=summary_path.name, sha256=sha(summary_path),
        component='Full application link, primary weapon cycle and submission-boundary CTests; finite headless app integration. All 18 sound requests return NONE with driver 0, so these tests do not establish sound admission or exercise the new cache-read handler. No final frames, PCM or installed-runtime certification.',
        certifies_entire_gate=False, certifies_all_map_variants=False,
        immutable_whole_checkout_during_build=False, physical_xbox_verified=False))
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps']) == (63, 1110, 38)
    assert len(matrix['weapons']) == 63 and sum(len(w['gates']) for w in matrix['weapons']) == 630
    assert matrix['fully_certified_weapons'] == 0 and not matrix['full_weapon_parity']
    assert matrix['cleanup_required_for_completion']
    write(BASE / 'weapon-completion-matrix-v45.json', matrix)
    audit = dict(passed=True, timestamp_utc=now, scratch_checks=checks,
        mandatory_cleanup_contract='goal-completion-contract-20261018.json',
        no_new_persistent_builds_objects_libraries_executables_or_raw_banks=True,
        existing_reference_evidence_original_assets_frozen_sources_and_runtimes_preserved=True,
        explicitly_retained_large_artifacts=[],
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        publisher_scratch=str(scratch), publisher_cleanup_pending_runner_exit=True,
        full_weapon_parity=False)
    write(BASE / 'post-weapon-runtime-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v45.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v45.json'),
        evidence=summary_path.name, evidence_sha256=sha(summary_path),
        storage_audit='post-weapon-runtime-storage-audit-v1.json',
        previous_checkpoint='checkpoint-original-sound-worker-20261018.json',
        cleanup_required_for_completion=True, fully_certified_weapons=0, full_weapon_parity=False)
    write(BASE / 'checkpoint-weapon-runtime-20261018.json', checkpoint)
    shutil.copyfile(Path(__file__), BASE / 'publish-weapon-runtime-v45-source.py')
    current_path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(current_path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
                   latest_storage_audit=checkpoint['storage_audit'],
                   latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
                   latest_current_required_input_check='checkpoint-weapon-runtime-20261018.json',
                   current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(current_path, current)
    print(json.dumps(checkpoint | dict(publisher_scratch=str(scratch),
          current_goal_allocated_bytes=current['current_allocated_bytes'])), flush=True)


if __name__ == '__main__':
    main()
