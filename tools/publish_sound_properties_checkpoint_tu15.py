#!/usr/bin/env python3
"""Publish the bounded original sound-property port and verified cleanup."""
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
    names = ['smg-sound-properties-cmake-v1/report.json',
        'smg-sound-properties-sanitizers-v1/report.json',
        'smg-sound-properties-world-compile-v2/report.json',
        'smg-sound-properties-host-v1/report.json',
        'smg-sound-properties-candidate-v1/report.json',
        'smg-baseline-native-firing-frontier-v11/report.json',
        'smg-original-sound-distance-reentry-v1/report.json',
        'smg-baseline-native-firing-frontier-v12/report.json',
        'smg-original-sound-127e00-entry-v1/report.json',
        'sound-properties-production-generation-v1/report.json']
    reports = {name: json.loads((BASE / name).read_text()) for name in names}
    for name in names[:5] + [names[-1]]:
        assert reports[name]['passed']
    assert reports[names[3]]['actual_host_original_cases'] == 61
    assert reports[names[4]]['missing_original_instructions'] == [0x229110]
    for name in names[:2]:
        assert all(row['status'] == 'retained' for row in reports[name]['retained_small_reports'])
    for native_name, original_name, target in [
        ('smg-baseline-native-firing-frontier-v11/native-observations.json',names[6],0x12a930),
        ('smg-baseline-native-firing-frontier-v12/native-observations.json',names[8],0x127e00)]:
        native = json.loads((BASE / native_name).read_text())
        original = reports[original_name]
        assert not native['returned'] and not original['passed']
        assert original['declared_prefix_stop_reached'] and original['declared_prefix_stop'] == target
        for key in ['gp','xmm','eflags','mxcsr']:
            assert native['CPU'][key] == original['actual_CPU'][key]
    checks = []
    for name in names:
        report = reports[name]
        path = Path(report['build']).parent if name in names[:2] else Path(report['runner_scratch'])
        assert not path.exists(), path
        checks.append(dict(report=name,scratch=str(path),removed=True))
    accepted_path = BASE / 'smg-sound-properties-cmake-v1/evidence/sound-properties-acceptance/report.json'
    accepted = json.loads(accepted_path.read_text())
    assert accepted['passed'] and accepted['production_source_regenerated_and_equal']
    assert accepted['native_generated_source_sha256'] == sha(ROOT / 'src/audio/source_sound_properties_tu15_generated.cpp')
    world = reports[names[2]]
    for path, pin in world['source_files_sha256'].items():
        assert sha(Path(path)) == pin, path
    summary = dict(passed=True, timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        root_entry=0x2290c0, complete_original_bodies=5, complete_original_cfg_instructions=189,
        original_instructions_exercised=188, successful_original_host_cases=61,
        unexecuted_snapshot_count_recheck_guard=0x229110,
        unexecuted_instruction_preserved_in_native_source=True,
        no_asynchronous_memory_transition_supplied_to_force_that_guard=True,
        original_tail_call_preserved=dict(site=0x18ff57,target=0x21feb0),
        original_host_GP_XMM_defined_flags_raw_FP_state_traces_and_ordered_writes_equal=True,
        reference_input_derived_by_uninterrupted_original_execution_from_retained_live_call=True,
        declared_record_and_pointer_controls_are_not_captured_gameplay_or_production_banks=True,
        root_world_dispatch_wired_and_current_source_syntax_checked=True,
        additional_original_listener_distance_entry_admitted=0x12a930,
        additional_listener_caller_authenticated=0x128bd3,
        release_component_CTests_passed=7, ASAN_UBSAN_component_CTests_passed=7,
        next_boundary=dict(caller=0x128c21,target=0x127e00,
            original_prefix_instructions=35860,native_instruction_observations=35846,
            GP_XMM_flags_MXCSR_equal=True,raw_FP_status_and_canonical_observer_PC_differences_retained=True,
            baseline_library_composition=True,full_native_firing_returned=False),
        current_full_application_relink_and_runtime_still_required=True,
        full_original_firing_independently_verified=False,sound_admission_or_PCM_verified=False,
        physical_xbox_verified=False,full_weapon_parity=False,large_artifacts_retained=[],
        evidence={name:sha(BASE/name) for name in names},
        production_acceptance_sha256=sha(accepted_path))
    summary_path = BASE/'sound-properties-original-native-progress-v1.json'
    write(summary_path,summary)
    previous = BASE/'weapon-completion-matrix-v49.json'
    matrix = json.loads(previous.read_text())
    weapon = next(row for row in matrix['weapons'] if row['name']=='objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary_path.name,sha256=sha(summary_path),
        component='Original2290C0 sound properties and four closed callees: 61 original host CPU cases match across188 executed instructions of189; the stable-snapshot count guard remains unexecuted and preserved. Seven Release and seven sanitizer checks pass. Direct12A930 distance admission advances firing to127E00. Full relink, firing, PCM and visual parity remain open.',
        certifies_entire_gate=False,certifies_all_map_variants=False,physical_xbox_verified=False))
    matrix['previous_matrix_sha256']=sha(previous)
    matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert (matrix['weapon_paths'],matrix['authored_rows'],matrix['maps'])==(63,1110,38)
    assert len(matrix['weapons'])==63 and sum(len(row['gates'])for row in matrix['weapons'])==630
    assert matrix['fully_certified_weapons']==0 and not matrix['full_weapon_parity']
    write(BASE/'weapon-completion-matrix-v50.json',matrix)
    audit_path=BASE/'post-sound-properties-storage-audit-v1.json'
    write(audit_path,dict(passed=True,scratch_checks=checks,completed_validation_scratch_paths_removed=len(checks),
        live_owned_builds=[],new_persistent_test_binaries_or_raw_RAM=0,large_artifacts_retained=[],
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True,
        publisher_scratch=str(scratch),publisher_cleanup_pending_runner_exit=True,cleanup_required_for_completion=True))
    checkpoint=dict(passed=True,matrix='weapon-completion-matrix-v50.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v50.json'),evidence=summary_path.name,
        evidence_sha256=sha(summary_path),storage_audit=audit_path.name,
        previous_checkpoint='checkpoint-sound-parameters-20261018.json',
        full_weapon_parity=False,fully_certified_weapons=0,cleanup_required_for_completion=True,
        current_full_application_relink_and_runtime_still_required=True)
    write(BASE/'checkpoint-sound-properties-20261018.json',checkpoint)
    shutil.copyfile(Path(__file__),BASE/'publish-sound-properties-v50-source.py')
    path=BASE/'storage-cleanup-current-20261017.json'
    current=json.loads(path.read_text())
    current.update(latest_weapon_completion_matrix=checkpoint['matrix'],latest_storage_audit=audit_path.name,
        latest_storage_audit_sha256=sha(audit_path),latest_current_required_input_check='checkpoint-sound-properties-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','-B1',str(BASE)],text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(path,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],scratch_checks=len(checks),publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':
    main()
