#!/usr/bin/env python3
"""Publish the live x87 sound-result proof and an observed application relink."""
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
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    names=['smg-sound-random-short-cmake-v1/report.json',
        'smg-sound-random-short-sanitizers-v1/report.json',
        'smg-sound-random-short-world-compile-v2/report.json',
        'smg-sound-random-short-host-v1/report.json',
        'smg-sound-random-short-candidate-v2/report.json',
        'smg-baseline-native-firing-frontier-v13/report.json',
        'smg-original-sound-21f960-entry-v1/report.json',
        'sound-random-short-production-generation-v1/report.json']
    data={name:json.loads((BASE/name).read_text())for name in names}
    for name in names[:4]+[names[-1]]:assert data[name]['passed']
    assert not data[names[4]]['passed']
    accepted_path=BASE/'smg-sound-random-short-cmake-v1/evidence/sound-random-short-acceptance/report.json'
    accepted=json.loads(accepted_path.read_text())
    assert accepted['passed'] and accepted['live_x87_result_bits_verified']==80
    assert accepted['original_host_successful_cases']==34 and accepted['proven_unreachable_instruction']==0x127e4b
    assert accepted['native_generated_source_sha256']==sha(ROOT/'src/source_sound_random_short_tu15_generated.cpp')
    for name in names[:2]:assert all(row['status']=='retained'for row in data[name]['retained_small_reports'])
    native=json.loads((BASE/'smg-baseline-native-firing-frontier-v13/native-observations.json').read_text())
    original=data[names[6]]
    assert not native['returned'] and '128dcf -> 21f960'in native['error']
    assert not original['passed'] and original['declared_prefix_stop_reached']
    for key in ['gp','xmm','eflags','mxcsr']:assert native['CPU'][key]==original['actual_CPU'][key]
    checks=[]
    for name in names:
        report=data[name]
        owned=Path(report['build']).parent if name in names[:2]else Path(report['runner_scratch'])
        assert not owned.exists(),owned
        checks.append(dict(report=name,scratch=str(owned),removed=True))
    world=data[names[2]]
    for path,pin in world['source_files_sha256'].items():assert sha(Path(path))==pin,path
    parent_path=BASE/'current-root-sound-parameters-runtime-build-v1/report.json'
    parent=json.loads(parent_path.read_text())
    live=[]
    for process in Path('/proc').iterdir():
        if not process.name.isdigit():continue
        try:
            args=(process/'cmdline').read_bytes().split(b'\0')
            if args and Path(os.fsdecode(args[0])).name=='cmake' and os.fsencode(parent['build'])in args:
                live.append(dict(pid=int(process.name),argv=[os.fsdecode(x)for x in args if x]))
        except OSError:pass
    assert parent['status']=='running' and Path(parent['build']).exists() and live
    job=dict(build_report=str(parent_path.relative_to(BASE)),build_session=66587,
        private_build=parent['build'],real_live_CMake_processes=live,
        native_firing_probe_session=59351,original_entry_probe_report='smg-fresh-native-firing-frontier-v3/report.json',
        finite_application_session=12631,application_report='current-root-sound-parameters-application-v1/report.json',
        scratch_cleanup_pending_terminal_result=True,keep_artifacts=False,
        observed_timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat())
    summary=dict(passed=True,root_entry=0x127e00,complete_original_cfg_instructions=34,
        original_instructions_exercised=33,proven_unreachable_instruction=0x127e4b,
        unreachable_proof=accepted['unreachable_proof'],original_host_cases=34,
        live_logical_x87_result_depth=1,exact_returned_x87_value_bits_verified=80,
        precision_controls_exercised=[24,53,64],rounding_modes_exercised=['nearest','down','up','toward zero'],
        empty_stack_TOP_and_condition_status_controls_exercised=True,
        raw_host_FP_status_control_MXCSR_GP_XMM_flags_traces_and_writes_equal=True,
        independent_emulator_raw_status_disagreements_preserved=True,
        source_rng_pointer_from_actual_caller=True,expected_values_or_original_instruction_patches_supplied=False,
        release_component_CTests_passed=8,ASAN_UBSAN_component_CTests_passed=8,
        current_world_dispatch_wired_and_current_source_syntax_checked=True,
        next_boundary=dict(caller=0x128dcf,target=0x21f960,native_instruction_observations=36011,
            independent_original_prefix_instructions=36025,GP_XMM_flags_MXCSR_equal=True,
            raw_FP_status_and_canonical_observer_PC_differences_retained=True,baseline_library_composition=True),
        current_whole_runtime_validation=job,current_whole_runtime_validation_complete=False,
        full_native_or_original_firing_verified=False,sound_admission_or_PCM_verified=False,
        physical_xbox_verified=False,full_weapon_parity=False,large_artifacts_retained=[],
        evidence={name:sha(BASE/name)for name in names},production_acceptance_sha256=sha(accepted_path))
    summary_path=BASE/'sound-random-short-original-native-progress-v1.json';write(summary_path,summary)
    previous=BASE/'weapon-completion-matrix-v50.json';matrix=json.loads(previous.read_text())
    weapon=next(row for row in matrix['weapons']if row['name']=='objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=summary_path.name,sha256=sha(summary_path),
        component='Complete127E00 signed-short interpolation: 34 original host cases preserve the live80-bit x87 result under three precision modes, four rounding modes and empty-stack status controls.33 reachable instructions execute; one SHR/TEST/JGE guard is provably bypassed. Eight Release and eight sanitizer checks pass. Full application relink is live; firing stops at21F960.',
        certifies_entire_gate=False,certifies_all_map_variants=False,physical_xbox_verified=False))
    matrix['previous_matrix_sha256']=sha(previous);matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert (matrix['weapon_paths'],matrix['authored_rows'],matrix['maps'])==(63,1110,38)
    assert len(matrix['weapons'])==63 and sum(len(row['gates'])for row in matrix['weapons'])==630
    assert not matrix['full_weapon_parity'] and matrix['fully_certified_weapons']==0
    write(BASE/'weapon-completion-matrix-v51.json',matrix)
    audit_path=BASE/'post-sound-random-short-storage-audit-v1.json'
    write(audit_path,dict(passed=True,scratch_checks=checks,completed_validation_scratch_paths_removed=len(checks),
        new_persistent_test_binaries_or_raw_RAM=0,large_artifacts_retained=[],
        existing_required_reference_retention='original-weapon-reference-retention-v1.json',
        original_assets_references_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True,
        live_test_work=job,active_private_build_intentionally_preserved=True,
        publisher_scratch=str(scratch),publisher_cleanup_pending_runner_exit=True,cleanup_required_for_completion=True))
    checkpoint=dict(passed=True,matrix='weapon-completion-matrix-v51.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v51.json'),evidence=summary_path.name,
        evidence_sha256=sha(summary_path),storage_audit=audit_path.name,
        previous_checkpoint='checkpoint-sound-properties-20261018.json',
        current_whole_runtime_validation=job,current_whole_runtime_validation_complete=False,
        full_weapon_parity=False,fully_certified_weapons=0,cleanup_required_for_completion=True)
    write(BASE/'checkpoint-sound-random-short-20261018.json',checkpoint)
    shutil.copyfile(Path(__file__),BASE/'publish-sound-random-short-v51-source.py')
    p=BASE/'storage-cleanup-current-20261017.json';current=json.loads(p.read_text())
    current.update(latest_weapon_completion_matrix=checkpoint['matrix'],latest_storage_audit=audit_path.name,
        latest_storage_audit_sha256=sha(audit_path),latest_current_required_input_check='checkpoint-sound-random-short-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','-B1',str(BASE)],text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(p,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],scratch_checks=len(checks),publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
