#!/usr/bin/env python3
"""Publish verified sound parameter ports and completed disposable validation."""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path, data):
    path.write_text(json.dumps(data, indent=2, sort_keys=True) + '\n')


def main():
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    now = datetime.datetime.now(datetime.timezone.utc).isoformat()
    names = ['smg-sound-parameters-cmake-v1/report.json',
        'smg-sound-parameters-sanitizers-v1/report.json',
        'smg-sound-parameters-world-compile-v1/report.json',
        'current-root-sound-listener-runtime-build-v1/report.json',
        'current-root-sound-listener-runtime-build-v1/terminal-source-and-cleanup-observation.json',
        'current-root-sound-listener-application-v1/report.json',
        'smg-fresh-native-firing-frontier-v2/report.json',
        'smg-baseline-native-firing-frontier-v10/report.json',
        'smg-original-sound-properties-entry-v1/report.json',
        'sound-gain-candidate-proof-v2.json',
        'sound-random-scalar-candidate-proof-v1.json']
    records = {name: json.loads((BASE / name).read_text()) for name in names}
    for name in names[:6]:
        assert records[name].get('passed', records[name].get('build_passed', False))
    component = records[names[0]]
    sanitizer = records[names[1]]
    assert all(row['status'] == 'retained' for row in component['retained_small_reports'])
    assert all(row['status'] == 'retained' for row in sanitizer['retained_small_reports'])
    accepted = {}
    for owner in ['gain', 'random-scalar']:
        path = BASE / 'smg-sound-parameters-cmake-v1/evidence' / ('sound-' + owner + '-acceptance/report.json')
        value = json.loads(path.read_text())
        assert value['passed'] and value['production_source_regenerated_and_equal']
        assert all(row['removed'] and not Path(row['path']).exists() for row in value['child_scratch_checks'])
        accepted[owner] = value
        names.append(str(path.relative_to(BASE)))
    assert accepted['gain']['original_host_successful_cases'] == 94
    assert accepted['gain']['original_host_actual_SIGSEGV_cases'] == 3
    assert accepted['random-scalar']['original_host_successful_cases'] == 33
    assert accepted['random-scalar']['original_host_actual_SIGSEGV_cases'] == 2
    assert accepted['random-scalar']['proven_unreachable_instruction'] == 0x127daa
    frontier = json.loads((BASE / 'smg-baseline-native-firing-frontier-v10/native-observations.json').read_text())
    original = records['smg-original-sound-properties-entry-v1/report.json']
    assert not frontier['returned'] and '128b9a -> 2290c0' in frontier['error']
    assert original['declared_prefix_stop_reached'] and not original['passed']
    for key in ['gp', 'xmm', 'eflags', 'mxcsr']:
        assert frontier['CPU'][key] == original['actual_CPU'][key]
    submissions = json.loads((BASE / 'current-root-sound-listener-runtime-build-v1/evidence/sandbox-sound-submit-acceptance/source-sound-submissions.json').read_text())
    assert len(submissions) == 18 and all(row['driver'] == 0 and row['result'] == 0xffffffff for row in submissions)
    cleanup = []
    for name in [names[0], names[1], names[2], names[3], names[5], names[6], names[7], names[8]]:
        report = records[name]
        owned = Path(report['build']).parent if name in [names[0], names[1], names[3]] else Path(report['runner_scratch'])
        assert not owned.exists(), owned
        cleanup.append(dict(report=name, scratch=str(owned), removed=True))
    sources = [ROOT / 'src/physics_live_world_tu15.cpp', ROOT / 'cmake/PhysicsLiveWorldTu15.cmake']
    for owner in ['gain', 'random_scalar']:
        sources += [ROOT / ('include/halo2/source_sound_' + owner + '_tu15.hpp'),
            ROOT / ('src/source_sound_' + owner + '_tu15_generated.cpp'),
            ROOT / ('tests/source_sound_' + owner + '_tu15_tests.cpp'),
            ROOT / ('tools/generate_sound_' + owner + '_tu15.py'),
            ROOT / ('tools/probe_sound_' + owner + '_tu15.py')]
    summary = dict(passed=True, timestamp_utc=now, root_entries=[0x127cf0, 0x127d70],
        complete_original_bodies=8, complete_original_cfg_instructions=386,
        original_instructions_exercised=385, proven_unreachable_instruction=0x127daa,
        unreachable_proof=accepted['random-scalar']['unreachable_proof'],
        actual_original_host_successful_cases=127, actual_original_host_SIGSEGV_cases=5,
        independent_emulator_failures_preserved=True, original_instruction_patches_or_expected_results_supplied=False,
        diagnostic_readonly_constant_controls=2, controls_are_not_production_inputs=True,
        release_component_CTests_passed=6, ASAN_UBSAN_component_CTests_passed=6,
        current_world_dispatch_wired_and_syntax_checked=True,
        previous_listener_application_build_and_six_selected_CTests_passed=True,
        previous_listener_application_help_and_120_ticks_1_and_4_players_passed=True,
        previous_listener_build_source_hash_changes=len(records[names[4]]['changed_source_files']),
        previous_listener_build_source_tree_immutable=False,
        previous_listener_build_predates_these_two_additional_ports=True,
        new_ports_whole_application_relink_and_runtime_still_required=True,
        source_sound_submission_requests=18, source_sound_submission_requests_admitted=0,
        next_boundary=dict(caller=0x128b9a, target=0x2290c0, native_instruction_observations=len(frontier['instructions']),
            independent_original_prefix_instructions=original['instruction_count'],
            GP_XMM_flags_MXCSR_equal=True, raw_FP_status_and_canonical_observer_PC_differences_retained=True,
            baseline_library_composition=True, whole_current_runtime=False),
        full_native_or_original_firing_verified=False, sound_admission_or_PCM_verified=False,
        evidence={name: sha(BASE / name) for name in names},
        source_files_sha256={str(path): sha(path) for path in sources},
        physical_xbox_verified=False, full_weapon_parity=False, large_artifacts_retained=[])
    summary_path = BASE / 'sound-parameters-original-native-progress-v1.json'
    write(summary_path, summary)
    previous = BASE / 'weapon-completion-matrix-v48.json'
    matrix = json.loads(previous.read_text())
    weapon = next(row for row in matrix['weapons'] if row['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary_path.name, sha256=sha(summary_path),
        component='Original127CF0 gain and127D70 RNG-backed scalar: eight complete bodies, 386 instructions, 127 successful original host CPU cases and five actual host faults. One instruction is bypassed by a proven SHR/TEST/JGE invariant. Six Release and six sanitizer tests pass. Previous listener app build and finite runs pass; the two new ports still require a full relink. Firing stops at2290C0.',
        certifies_entire_gate=False, certifies_all_map_variants=False, physical_xbox_verified=False))
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps']) == (63, 1110, 38)
    assert len(matrix['weapons']) == 63 and sum(len(row['gates']) for row in matrix['weapons']) == 630
    assert matrix['fully_certified_weapons'] == 0 and not matrix['full_weapon_parity']
    write(BASE / 'weapon-completion-matrix-v49.json', matrix)
    audit_path = BASE / 'post-sound-parameters-storage-audit-v1.json'
    write(audit_path, dict(passed=True, timestamp_utc=now, scratch_checks=cleanup,
        completed_validation_scratch_paths_removed=len(cleanup), live_owned_builds=[],
        new_persistent_test_binaries_or_raw_RAM=0, large_artifacts_retained=[],
        original_reference_evidence_assets_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True,
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        publisher_scratch=str(scratch), publisher_cleanup_pending_runner_exit=True,
        cleanup_required_for_completion=True, full_weapon_parity=False))
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v49.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v49.json'),
        evidence=summary_path.name, evidence_sha256=sha(summary_path), storage_audit=audit_path.name,
        previous_checkpoint='checkpoint-sound-listener-20261018.json',
        new_ports_whole_application_relink_and_runtime_still_required=True,
        fully_certified_weapons=0, full_weapon_parity=False, cleanup_required_for_completion=True)
    write(BASE / 'checkpoint-sound-parameters-20261018.json', checkpoint)
    out = BASE / 'sound-parameters-production-source-snapshot-v1'
    out.mkdir(exist_ok=False)
    for path in sources:
        shutil.copyfile(path, out / path.name)
    shutil.copyfile(Path(__file__), BASE / 'publish-sound-parameters-v49-source.py')
    current_path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(current_path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=audit_path.name, latest_storage_audit_sha256=sha(audit_path),
        latest_current_required_input_check='checkpoint-sound-parameters-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(current_path, current)
    print(json.dumps(dict(passed=True, matrix=checkpoint['matrix'], scratch_checks=len(cleanup),
        publisher_scratch=str(scratch))), flush=True)


if __name__ == '__main__':
    main()
