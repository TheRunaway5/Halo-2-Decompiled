#!/usr/bin/env python3
"""Publish bounded campaign scenery/weapon command evidence; use run_test.py."""
import copy
import hashlib
import json
import os
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load(name):return json.loads((BASE/name).read_text())
def save(name,value):(BASE/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')

def main():
    references=[];scratch=set()
    for version in (1,2):
        folder=f'weapon-campaign-scenery-validation-v{version}'
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['source_scratch_removed_after_runner_exit'] and not parent['source_cleanup_pending_runner_exit']
        assert parent['steps'][0]['report_sha256']==sha(BASE/folder/'release/report.json')
        assert not parent['passed'] and not child['passed']
        scratch.update((parent['runner_scratch'],child['build']))
        references.extend(folder+'/'+n for n in ('report.json','release/report.json','source-manifest.json','release/tests.log'))
    folder='weapon-campaign-scenery-validation-v2'
    cpu_name=folder+'/release/evidence/weapon-campaign-scenery-lighting.json';cpu=load(cpu_name)
    app_name=folder+'/release/evidence/weapon-campaign-application.json';app=load(app_name)
    source_name=folder+'/release/evidence/weapon-campaign-source-submission.json';source=load(source_name)
    assert cpu['passed'] and cpu['rejected_application_cap_reached'] and cpu['negative_controls']==12
    assert cpu['baked_vertices_verified']>0 and cpu['dynamic_vertices_verified']>0
    cap=next(row for row in cpu['scenery_units']if row['definition']==0xf9d6179e and row['placement']==583)
    assert cap['lighting_class']==1 and cap['instance']==146 and cap['verified_vertices']>0
    alias=load('cairo-dress-cap-shared-lighting-cache-v1.json')
    assert cap['source_unit_words']==alias['original_selected_unit']['source_unit_words']
    assert not app['passed'] and app['application_exit_code']==1 and not app['timed_out']
    assert app['submitted_frames']==[0] and app['weapon_gpu_commands'] and app['source_weapon_inputs']
    assert '32ab71 -> 32abd8' in app['application_log_tail']
    assert app['actual_owner_constant_and_vertex_bindings_verified'] and not app['visible_pixels_verified']
    assert source['passed'] and source['source_weapon_submission_verified']
    log=(BASE/folder/'release/tests.log').read_text();assert log.count('   Passed')==8
    references.extend((cpu_name,app_name,source_name,'cairo-dress-cap-shared-lighting-cache-v1.json'))
    scratch.add(app['runner_scratch'])
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)]
    assert all(row['removed']for row in checks)
    selected=['src/main.cpp','src/renderer.cpp','include/halo2/renderer.hpp',
        'src/source_campaign_model.cpp','include/halo2/source_campaign_model.hpp',
        'src/source_dynamic_model_shaders.cpp','src/scenery_lighting.cpp','include/halo2/scenery.hpp',
        'tests/campaign_model_geometry_runtime_tu15_tests.cpp',
        'tools/validate_weapon_material_snapshot_tu15.py','tools/check_campaign_weapon_application_tu15.py']
    pins=load(folder+'/source-manifest.json')['source_files_sha256']
    current={n:sha(ROOT/n)for n in selected};drift=[n for n in selected if current[n]!=pins[n]]
    next_work=('Resolve the exact original TU1.5 runtime owner32AB71 ->32ABD8 reached by root1CC8D0 '
        'after the first submitted Cairo frame. The four-frame application remains failed; do not supply '
        'a return or stop at one frame to pass it. Continue all63 paths/630 gates: actual campaign weapon '
        'commands are bounded to the retained Cairo case. Matched OG Xbox TU1.5 framebuffer sequences, other owners/maps/variants, '
        'weapon-specific firing/impact/lifecycle behavior and full authored firing audio remain open. '
        'Preserve unrelated work and remove all disposable tests/builds after each terminal run.')
    progress_name='weapon-campaign-scenery-lighting-progress-v1.json'
    progress=dict(evidence_verified=True,application_passed=False,prior_scenery_guard_resolved_in_bounded_case=True,
        checks_passed=8,submitted_frames=app['submitted_frames'],
        application_failure='TU1.5 live actual root1CC8D0 at32AB71: owner32ABD8 unavailable',
        actual_scene_source_weapon_units=app['frame_outcomes'][0]['source_weapon_units'],
        actual_scene_weapon_gpu_draws=app['frame_outcomes'][0]['weapon_gpu_draws'],
        source_weapon_input_count=len(app['source_weapon_inputs']),weapon_gpu_command_count=len(app['weapon_gpu_commands']),
        original_cap_reference=146,original_cap_placement=583,
        original_cap_packet_equal_to_failed_application=True,
        baked_vertices_verified=cpu['baked_vertices_verified'],dynamic_vertices_verified=cpu['dynamic_vertices_verified'],
        negative_controls=cpu['negative_controls'],cpu_main_units=cpu['main_units'],
        cpu_source_particle_commands=cpu['source_particle_commands'],
        cpu_replays_retained_application_camera=True,cpu_is_not_entire_application=True,
        cpu_particle_horizontal_angle_is_declared_test_input=1.2217304706573486,
        original_instruction_patches=[],supplied_callee_results=[],original_source_guard_relaxed=False,
        selected_shader_banks_share_geometry=True,lighting_selected_per_original_draw_and_current_bsp=True,
        actual_owner_constant_and_vertex_bindings_verified=True,visible_pixels_verified=False,
        matched_xbox_frame_verified=False,full_firing_audio_verified=False,
        full_weapon_parity=False,fully_certified_weapons=0,large_artifacts_retained=[],next_work=next_work,
        validated_snapshot_source_files_sha256={n:pins[n]for n in selected},current_source_files_sha256=current,
        source_changed_since_validation=drift,references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v65.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\fixed\h_turret_mp\item\h_turret_mp_item')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual Cairo campaign submits its first frame and original held world-only turret-item weapon Vulkan draw commands after per-instance scenery lighting correction; the four-frame application then fails at original owner32ABD8.',
        certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
        map='assets/disc/maps/01b_spacestation.map'))
    weapon['gates']['full_reference_sequence']['remaining']=next_work
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v65.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    matrix_name='weapon-completion-matrix-v66.json';save(matrix_name,matrix)
    audit_name='post-weapon-campaign-scenery-lighting-storage-audit-v1.json';publisher=os.environ['HALO2_TEST_SCRATCH']
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint='checkpoint-weapon-campaign-scenery-lighting-20261019.json'
    save(checkpoint,dict(evidence_verified=True,application_passed=False,bounded_scenery_guard_resolved=True,
        previous_checkpoint='checkpoint-weapon-campaign-application-frontier-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix=matrix_name,matrix_sha256=sha(BASE/matrix_name),
        storage_audit=audit_name,fully_certified_weapons=0,full_weapon_parity=False,next_work=next_work))
    state=load('continuation-working-state-v66-pending.json')
    state.update(status='bounded_campaign_application_weapon_commands_verified',latest_checkpoint=checkpoint,latest_matrix=matrix_name,
        active_validation=None,active_unified_exec_session_id=None,all_owned_builds_and_probes_terminal=True,
        actual_campaign_application_report=app_name,actual_campaign_application_gpu_draw_verified=True,
        actual_application_failure=app_name,
        actual_application_verdict='Eight companion checks pass; first actual campaign frame submits12 weapon units/17 weapon Vulkan commands, including2 commands for the held world-only turret item. Application then fails at root1CC8D0 ->32AB71 ->32ABD8. No four-frame, visible-pixel or Xbox framebuffer acceptance.',
        application_gpu_observer_positive_case_verified=True,next_work=next_work,
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v66.json',state)
    print(json.dumps(dict(evidence_verified=True,application_passed=False,checks_passed=8,frames=1,
        weapon_gpu_commands=len(app['weapon_gpu_commands']),fully_certified_weapons=0)),flush=True)

if __name__=='__main__':main()
