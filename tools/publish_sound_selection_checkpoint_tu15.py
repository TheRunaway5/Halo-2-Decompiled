#!/usr/bin/env python3
"""Publish original sound selection/binding CPU evidence and completed cleanup."""
import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
from original_xbox_pose_case import BASE, ROOT, sha


def write(path,value):
    path.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    names=['smg-sound-selection-cmake-v1/report.json','smg-sound-selection-sanitizers-v1/report.json',
        'smg-sound-selection-world-compile-v1/report.json',
        'current-root-sound-parameters-runtime-build-v1/report.json',
        'current-root-sound-parameters-runtime-build-v1/terminal-source-and-cleanup-observation.json',
        'current-root-sound-parameters-application-v1/report.json','smg-fresh-native-firing-frontier-v3/report.json',
        'smg-baseline-native-firing-frontier-v16/report.json','smg-original-sound-128970-entry-v1/report.json',
        'sound-permutation-authored-domains-v1/report.json','sound-selection-production-generation-v1/report.json']
    rows={name:json.loads((BASE/name).read_text())for name in names}
    for name in names[:6]+[names[-1]]:assert rows[name]['passed']
    for name in names[:2]:assert all(row['status']=='retained'for row in rows[name]['retained_small_reports'])
    for path,pin in rows[names[2]]['source_files_sha256'].items():assert sha(Path(path))==pin,path
    accepted={}
    for owner in ['range-selection','permutation','binding']:
        p=BASE/'smg-sound-selection-cmake-v1/evidence'/('sound-'+owner+'-acceptance/report.json')
        data=json.loads(p.read_text());assert data['passed'] and data['production_source_regenerated_and_equal']
        stem=owner.replace('-','_');assert data['native_generated_source_sha256']==sha(ROOT/('src/source_sound_'+stem+'_tu15_generated.cpp'))
        accepted[owner]=data;names.append(str(p.relative_to(BASE)))
    assert sum(v['original_host_successful_cases']for v in accepted.values())==151
    assert sum(v['original_host_actual_SIGSEGV_cases']for v in accepted.values())==6
    native=json.loads((BASE/'smg-baseline-native-firing-frontier-v16/native-observations.json').read_text())
    original=rows['smg-original-sound-128970-entry-v1/report.json']
    assert not native['returned'] and '128e5d -> 128970'in native['error']
    assert not original['passed'] and original['declared_prefix_stop_reached']
    for key in ['gp','xmm','eflags','mxcsr']:assert native['CPU'][key]==original['actual_CPU'][key]
    checks=[]
    for name in names[:11]:
        data=rows[name]
        owned=Path(data['build']).parent if name in names[:2]+[names[3]]else Path(data['runner_scratch'])if 'runner_scratch'in data else None
        if owned is not None:
            assert not owned.exists(),owned;checks.append(dict(report=name,scratch=str(owned),removed=True))
    audit=rows['sound-permutation-authored-domains-v1/report.json']
    assert audit['audit_completed'] and audit['map_count']==38 and audit['maximum_referenced_count']==32
    assert not audit['invalid_sound_range_or_class_references']
    assert audit['referenced_range_permutation_counts']['32']==12
    summary=dict(passed=True,timestamp_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        root_entries=[0x21f960,0x21fa80,0x12b1c0],complete_original_bodies=11,
        complete_original_cfg_instructions=527,original_instructions_exercised=526,
        proven_unreachable_instruction=0x21fc5a,unreachable_proof=accepted['permutation']['unreachable_proof'],
        original_host_successful_cases=151,original_host_actual_SIGSEGV_cases=6,
        original_host_GP_XMM_defined_flags_raw_FP_state_traces_and_ordered_writes_equal=True,
        source_count32_shift_mask_behavior_exercised=True,source_reference_byte_wrap_preserved=True,
        controls_are_declared_inputs_not_additional_captured_gameplay=True,
        release_component_CTests_passed=11,ASAN_UBSAN_component_CTests_passed=11,
        world_dispatch_wired_and_current_source_syntax_checked=True,
        prior_full_application_and_ten_selected_CTests_passed=True,
        prior_application_help_and_120_ticks_with_one_and_four_players_passed=True,
        prior_build_source_inventory_changes=len(rows['current-root-sound-parameters-runtime-build-v1/terminal-source-and-cleanup-observation.json']['source_changes']),
        prior_full_build_predates_these_three_additional_ports=True,
        current_whole_application_relink_and_runtime_still_required=True,
        sound_submission_requests=18,sound_submission_requests_admitted=0,
        authored_domain_audit=dict(maps=38,maximum_permutation_count=32,referenced_count32=12,
            above32=0,weapon_dependency_closure_verified=False),
        next_boundary=dict(caller=0x128e5d,target=0x128970,native_instruction_observations=36388,
            original_prefix_instructions=36402,GP_XMM_flags_MXCSR_equal=True,
            raw_FP_and_canonical_observer_PC_differences_retained=True,baseline_library_composition=True),
        full_native_or_original_firing_verified=False,sound_admission_or_PCM_verified=False,
        physical_xbox_verified=False,full_weapon_parity=False,large_artifacts_retained=[],
        evidence={name:sha(BASE/name)for name in names})
    p=BASE/'sound-selection-original-native-progress-v1.json';write(p,summary)
    previous=BASE/'weapon-completion-matrix-v51.json';matrix=json.loads(previous.read_text())
    weapon=next(row for row in matrix['weapons']if row['name']=='objects\\weapons\\rifle\\smg\\smg')
    weapon['bounded_evidence'].append(dict(report=p.name,sha256=sha(p),
        component='Original21F960 range selection,21FA80 permutation selection and12B1C0 binding: 11 complete bodies,151 original host successes,six actual host faults,526 executed instructions of527. Source32-permutation behavior and chunk-reference byte updates match. Eleven Release and eleven sanitizer checks pass. Prior full app passes; these additional ports need relink. Firing stops at128970.',
        certifies_entire_gate=False,certifies_all_map_variants=False,physical_xbox_verified=False))
    matrix['previous_matrix_sha256']=sha(previous);matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    assert (matrix['weapon_paths'],matrix['authored_rows'],matrix['maps'])==(63,1110,38)
    assert len(matrix['weapons'])==63 and sum(len(row['gates'])for row in matrix['weapons'])==630
    assert matrix['fully_certified_weapons']==0 and not matrix['full_weapon_parity']
    write(BASE/'weapon-completion-matrix-v52.json',matrix)
    audit_path=BASE/'post-sound-selection-storage-audit-v1.json'
    write(audit_path,dict(passed=True,scratch_checks=checks,completed_validation_scratch_paths_removed=len(checks),
        new_persistent_test_binaries_or_raw_RAM=0,large_artifacts_retained=[],live_owned_builds=[],
        required_existing_reference_retention='original-weapon-reference-retention-v1.json',
        original_assets_references_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True,
        publisher_scratch=str(scratch),publisher_cleanup_pending_runner_exit=True,cleanup_required_for_completion=True))
    checkpoint=dict(passed=True,matrix='weapon-completion-matrix-v52.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v52.json'),evidence=p.name,evidence_sha256=sha(p),
        storage_audit=audit_path.name,previous_checkpoint='checkpoint-sound-random-short-20261018.json',
        fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,
        current_whole_application_relink_and_runtime_still_required=True)
    write(BASE/'checkpoint-sound-selection-20261018.json',checkpoint)
    shutil.copyfile(Path(__file__),BASE/'publish-sound-selection-v52-source.py')
    current_path=BASE/'storage-cleanup-current-20261017.json';current=json.loads(current_path.read_text())
    current.update(latest_weapon_completion_matrix=checkpoint['matrix'],latest_storage_audit=audit_path.name,
        latest_storage_audit_sha256=sha(audit_path),latest_current_required_input_check='checkpoint-sound-selection-20261018.json',
        current_allocated_bytes=int(subprocess.check_output(['du','-s','-B1',str(BASE)],text=True).split()[0]))
    current['folder_allocated_size_reduction_bytes']=current['original_baseline_allocated_bytes']-current['current_allocated_bytes']
    current['folder_allocated_size_reduction_percent']=round(100*current['folder_allocated_size_reduction_bytes']/current['original_baseline_allocated_bytes'],3)
    write(current_path,current)
    print(json.dumps(dict(passed=True,matrix=checkpoint['matrix'],scratch_checks=len(checks),publisher_scratch=str(scratch))),flush=True)


if __name__=='__main__':main()
