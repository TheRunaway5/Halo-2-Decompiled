#!/usr/bin/env python3
"""Publish the proven listener CPU owner without certifying a whole weapon."""
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
    names = [
        'smg-sound-listener-production-v1/report.json',
        'smg-sound-listener-production-v1/independent-emulator/report.json',
        'smg-sound-listener-production-v1/original-host/report.json',
        'smg-sound-listener-cmake-v1/report.json',
        'smg-sound-listener-sanitizers-v1/report.json',
        'smg-sound-listener-world-compile-v1/report.json',
        'smg-baseline-native-firing-frontier-v7/report.json',
        'smg-original-sound-range-entry-v1/report.json',
        'current-root-sound-metadata-runtime-build-v1/report.json',
        'current-root-sound-metadata-runtime-build-v1/terminal-cleanup-observation.json',
    ]
    acceptance = json.loads((BASE / names[0]).read_text())
    assert acceptance['passed'] and acceptance['original_host_cases'] == 54
    assert len(acceptance['independent_emulator_precision_discrepancies']) == 9
    assert not json.loads((BASE / names[1]).read_text())['passed']
    assert json.loads((BASE / names[2]).read_text())['passed']
    for name in names[3:6]:
        assert json.loads((BASE / name).read_text())['passed']
    native = json.loads((BASE / 'smg-baseline-native-firing-frontier-v7/native-observations.json').read_text())
    original = json.loads((BASE / 'smg-original-sound-range-entry-v1/report.json').read_text())
    assert not native['returned'] and '129888 -> 127cf0' in native['error']
    assert not original['passed'] and original['declared_prefix_stop_reached']
    for field in ('gp', 'xmm', 'eflags', 'mxcsr'):
        assert native['CPU'][field] == original['actual_CPU'][field]
    paths = []
    for name in [names[0], names[3], names[4], names[5], names[6], names[7]]:
        report = json.loads((BASE / name).read_text())
        path = Path(report['build']).parent if 'build' in report else Path(report['runner_scratch'])
        assert not path.exists(), path
        paths.append(dict(report=name, scratch=str(path), removed=True))
    prior_cleanup = json.loads((BASE / names[-1]).read_text())
    assert all(row['removed'] and not Path(row['path']).exists() for row in prior_cleanup['scratch_checks'])
    failed = json.loads((BASE / names[-2]).read_text())
    assert failed['status'] == 'failed' and failed['steps'][-1]['exit_code'] == 2
    parent_path = BASE / 'current-root-sound-listener-runtime-build-v1/report.json'
    parent = json.loads(parent_path.read_text())
    live = []
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():
            continue
        try:
            args = (process / 'cmdline').read_bytes().split(b'\0')
            if args and Path(os.fsdecode(args[0])).name == 'cmake' and os.fsencode(parent['build']) in args:
                live.append(dict(pid=int(process.name), argv=[os.fsdecode(arg) for arg in args if arg]))
        except OSError:
            pass
    assert parent['status'] == 'running' and Path(parent['build']).exists() and live
    job = dict(build_report=str(parent_path.relative_to(BASE)), build_session=67616,
        original_entry_probe_report='smg-fresh-native-firing-frontier-v2/report.json',
        native_firing_probe_session=6032, finite_application_session=71325,
        application_report='current-root-sound-listener-application-v1/report.json',
        observed_timestamp_utc=now, private_build=parent['build'], real_live_CMake_processes=live,
        scratch_cleanup_pending_terminal_result=True, keep_artifacts=False)
    summary = dict(passed=True, timestamp_utc=now, root_entry=0x12a810,
        complete_original_bodies=5, complete_original_cfg_instructions=312,
        original_host_exact_cases=54, real_absent_bank_fault_cases=1,
        actual_unchanged_original_host_all_GP_XMM_defined_flags_raw_x87_controls_status_MXCSR_traces_and_ordered_writes_equal=True,
        independent_emulator_raw_precision_discrepancies=9,
        independent_emulator_failed_result_preserved=True,
        declared_fixture_inputs_are_not_captured_gameplay=True,
        uncached_pointer_rebinding_is_only_a_declared_fixture=True,
        release_component_CTests_passed=4, ASAN_UBSAN_component_CTests_passed=4,
        current_world_root_dispatch_wired_and_syntax_checked=True,
        actual_original_prefix=dict(instructions=original['instruction_count'],
            stores=original['ordered_source_writes'], complete_call_passed=False),
        baseline_native_next_boundary=dict(caller=0x129888, target=0x127cf0,
            instruction_observations=len(native['instructions']), GP_XMM_flags_MXCSR_equal=True,
            raw_x87_status_xor=native['CPU']['x87_status'] ^ original['actual_CPU']['x87_status'],
            raw_x87_control_xor=native['CPU']['x87_control'] ^ original['actual_CPU']['x87_control'],
            canonical_damage_observer_PC_domain_requires_qualified_mapping=True,
            complete_CPU_or_trace_parity=False, current_whole_runtime=False),
        prior_whole_runtime_build_failed=True, prior_failure='Unresolved MultiplayerRuntimeAuthority definitions',
        prior_build_and_waiting_probe_scratch_removed=True,
        current_whole_runtime_validation=job, current_whole_runtime_validation_complete=False,
        full_original_or_native_firing_verified=False, sound_admission_or_PCM_verified=False,
        expected_outcomes_or_original_instruction_patches_supplied=False,
        physical_xbox_verified=False, full_weapon_parity=False,
        evidence={name: sha(BASE / name) for name in names}, new_large_reference_inputs=False)
    summary_path = BASE / 'sound-listener-original-native-progress-v1.json'
    write(summary_path, summary)
    previous = BASE / 'weapon-completion-matrix-v47.json'
    matrix = json.loads(previous.read_text())
    weapon = next(row for row in matrix['weapons'] if row['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary_path.name, sha256=sha(summary_path),
        component='Complete12A810 listener selection and four closed callees: all312 instructions match unchanged original host execution over54 successful cases, with the actual absent-bank fault retained separately. Four Release and four sanitizer tests pass. Native firing reaches the next unowned127CF0 sound-gain routine. Current full relink remains in progress.',
        certifies_entire_gate=False, certifies_all_map_variants=False, physical_xbox_verified=False))
    matrix['previous_matrix_sha256'] = sha(previous)
    matrix['evidence_update_tool_sha256'] = sha(Path(__file__))
    assert (matrix['weapon_paths'], matrix['authored_rows'], matrix['maps']) == (63, 1110, 38)
    assert len(matrix['weapons']) == 63 and sum(len(row['gates']) for row in matrix['weapons']) == 630
    assert matrix['fully_certified_weapons'] == 0 and not matrix['full_weapon_parity']
    write(BASE / 'weapon-completion-matrix-v48.json', matrix)
    audit = dict(passed=True, timestamp_utc=now, scratch_checks=paths,
        previous_terminal_cleanup_checks=prior_cleanup['scratch_checks'],
        completed_validation_scratch_paths_removed=len(paths),
        large_artifacts_retained=[], new_persistent_test_binaries_or_raw_RAM=0,
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        existing_reference_evidence_assets_frozen_sources_launcher_selected_runtimes_and_unrelated_work_preserved=True,
        live_test_work=job, active_private_build_intentionally_preserved=True,
        publisher_scratch=str(scratch), publisher_cleanup_pending_runner_exit=True,
        cleanup_required_for_completion=True, full_weapon_parity=False)
    write(BASE / 'post-sound-listener-storage-audit-v1.json', audit)
    checkpoint = dict(passed=True, matrix='weapon-completion-matrix-v48.json',
        matrix_sha256=sha(BASE / 'weapon-completion-matrix-v48.json'),
        evidence=summary_path.name, evidence_sha256=sha(summary_path),
        storage_audit='post-sound-listener-storage-audit-v1.json',
        previous_checkpoint='checkpoint-sound-scalar-20261018.json',
        current_whole_runtime_validation=job, current_whole_runtime_validation_complete=False,
        cleanup_required_for_completion=True, fully_certified_weapons=0, full_weapon_parity=False)
    write(BASE / 'checkpoint-sound-listener-20261018.json', checkpoint)
    shutil.copyfile(Path(__file__), BASE / 'publish-sound-listener-v48-source.py')
    path = BASE / 'storage-cleanup-current-20261017.json'
    current = json.loads(path.read_text())
    current.update(timestamp_utc=now, latest_weapon_completion_matrix=checkpoint['matrix'],
        latest_storage_audit=checkpoint['storage_audit'], latest_storage_audit_sha256=sha(BASE / checkpoint['storage_audit']),
        latest_current_required_input_check='checkpoint-sound-listener-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du', '-s', '-B1', str(BASE)], text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes'] = current['original_baseline_allocated_bytes'] - current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent'] = round(100 * current['folder_allocated_size_reduction_bytes'] / current['original_baseline_allocated_bytes'], 3)
    write(path, current)
    print(json.dumps(dict(passed=True, matrix=checkpoint['matrix'], live_build=job['private_build'],
        completed_scratch_checks=len(paths), publisher_scratch=str(scratch))), flush=True)


if __name__ == '__main__':
    main()
