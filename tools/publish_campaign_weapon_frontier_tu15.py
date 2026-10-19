#!/usr/bin/env python3
"""Publish the actual failed campaign weapon application frontier; run_test.py."""
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
    folders=['weapon-campaign-application-validation-v1','weapon-campaign-scene-frontier-validation-v1',
        'weapon-campaign-application-validation-v2']
    for folder in folders:
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['source_scratch_removed_after_runner_exit'] and not parent['source_cleanup_pending_runner_exit']
        assert parent['status']==child['status']=='failed'
        assert all(s['exit_code']==0 for s in child['steps'][:-1]) and child['steps'][-1]['exit_code']==8
        assert parent['steps'][0]['report_sha256']==sha(BASE/folder/'release/report.json')
        scratch.update((parent['runner_scratch'],child['build']))
        references += [folder+'/'+n for n in ('report.json','release/report.json','source-manifest.json','release/tests.log')]
        log=(BASE/folder/'release/tests.log').read_text();assert log.count('   Passed')==(5 if 'scene-frontier'in folder else 7)
    app_names=[f'weapon-campaign-application-validation-v{n}/release/evidence/weapon-campaign-application.json'for n in (1,2)]
    for name in app_names:
        report=load(name);assert not report['passed'] and report['application_exit_code']==1 and not report['timed_out']
        assert not report['submitted_frames'] and not report['source_weapon_inputs'] and not report['weapon_gpu_commands']
        assert 'Campaign scenery requires its separate source lighting or override owner'in report['application_log_tail']
        scratch.add(report['runner_scratch']);references.append(name)
    cpu_name='weapon-campaign-scene-frontier-validation-v1/release/evidence/weapon-campaign-scene-frontier.json'
    cpu=load(cpu_name);assert cpu['diagnostic_probe_completed'] and not cpu['main_scenery_guard_reproduced']
    assert cpu['guard_rejected_units']==0;references.append(cpu_name)
    app=load(app_names[-1]);events=app['diagnostic_events'];frame=next(r for r in events if r['event']=='source_frame')
    rejected=next(r for r in events if r['event']=='scenery_rejected')
    assert frame['tick']==0 and frame['main_commands_completed'] and frame['source_particle_commands']==21
    assert rejected['definition']==0xf9d6179e and rejected['placement']==583 and rejected['lighting_class']==1
    assert rejected['override_slot']==255 and rejected['source_unit_words'][0]&1
    alias_name='cairo-dress-cap-shared-lighting-cache-v1.json';alias=load(alias_name)
    assert alias['evidence_verified'] and not alias['application_rendering_fixed']
    first=alias['first_cached_placement'];actual=alias['rejected_placement']
    assert first['placement']==152 and not first['bsp0_baked_records']
    assert actual['placement']==583 and actual['bsp0_baked_records']==[146]
    assert first['cache_key']==actual['cache_key'] and alias['original_baked_record']['index']==146
    for name in ('cairo-dress-cap-baked-identity-v1.json',alias_name):
        references.append(name);scratch.add(load(name)['runner_scratch'])
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)]
    assert all(row['removed']for row in checks)
    selected=['src/main.cpp','src/renderer.cpp','include/halo2/renderer.hpp',
        'tools/check_campaign_weapon_application_tu15.py','tools/validate_weapon_material_snapshot_tu15.py',
        'tests/campaign_model_geometry_runtime_tu15_tests.cpp']
    pins=load('weapon-campaign-application-validation-v2/source-manifest.json')['source_files_sha256']
    current={n:sha(ROOT/n)for n in selected};drift=[n for n in selected if current[n]!=pins[n]]
    next_work=('Correct shared scene resource capability versus per-instance lighting selection. Actual source '
        'class1 must retain reference146/its checksum and per-section incident/radiance vertex streams with '
        'implementation1; class2 keeps its dynamic bank. Do not rewrite class1 to2, skip scenery or call '
        'the non-reproducing model-only probe an application proof. Prefer shared geometry/material capability '
        'banks per definition/variant, then select from the actual original unit lighting_class and packed '
        'source instance. Consume the current BSP lighting on each draw so transitions cannot retain another '
        'BSP baked bank. Verify a matching CPU consumer case before repeating the full application and '
        'require positive actual weapon commands. All63 paths/630 gates, framebuffer/audio and cleanup remain required.')
    progress_name='weapon-campaign-application-frontier-progress-v1.json'
    progress=dict(evidence_verified=True,application_passed=False,application_weapon_gpu_commands=0,
        original_source_unit_identified=True,rejected_scenery_name=alias['source_scenery_name'],
        rejected_original_unit=rejected,actual_source_frame=frame,metadata_alias=alias_name,
        diagnosis=alias['diagnosis'],first_cached_placement=152,rejected_placement=583,
        original_baked_instance=146,original_baked_bucket=4,original_checksum=alias['original_baked_record']['checksum'],
        smaller_model_only_probe_reproduced=False,smaller_probe_camera_matches_application=False,
        actual_application_attempts=2,companion_checks_passed_per_application_attempt=7,
        application_gpu_observer_implemented=True,application_gpu_observer_positive_case_verified=False,
        original_instruction_patches=[],supplied_callee_results=[],original_source_guard_relaxed=False,
        actual_scene_rendering_fixed=False,full_weapon_parity=False,fully_certified_weapons=0,
        framebuffer_parity_verified=False,full_firing_audio_verified=False,large_artifacts_retained=[],next_work=next_work,
        validated_snapshot_source_files_sha256={n:pins[n]for n in selected},current_source_files_sha256=current,
        source_changed_since_validation=drift,references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v64.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\fixed\h_turret_mp\item\h_turret_mp_item')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual campaign application attempts stop before weapon submission on shared dress-cap class1/baked lighting ownership. Native CPU weapon proof remains bounded; GPU observer has no positive application case.',
        certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
        map='assets/disc/maps/01b_spacestation.map'))
    weapon['gates']['full_reference_sequence']['remaining']='Actual Cairo application stops before its first submitted frame on a class1 dress-cap scenery draw incorrectly sharing a dynamic bank. Source packet/record146/first-placement cache alias are identified; per-instance lighting correction and positive composed weapon GPU/framebuffer/audio sequences remain required.'
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v64.json');matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    matrix_name='weapon-completion-matrix-v65.json';save(matrix_name,matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-campaign-application-frontier-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint='checkpoint-weapon-campaign-application-frontier-20261019.json'
    save(checkpoint,dict(evidence_verified=True,application_passed=False,scene_rendering_fixed=False,
        previous_checkpoint='checkpoint-weapon-campaign-source-submission-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix=matrix_name,matrix_sha256=sha(BASE/matrix_name),
        storage_audit=audit_name,fully_certified_weapons=0,full_weapon_parity=False,next_work=next_work))
    state=load('continuation-working-state-v65-pending.json')
    state.update(status='diagnosed_frontier_implementation_required',latest_checkpoint=checkpoint,latest_matrix=matrix_name,
        active_validation=None,active_unified_exec_session_id=None,all_owned_builds_and_probes_terminal=True,
        next_work=next_work,progress=alias['diagnosis'],application_gpu_observer_positive_case_verified=False,
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v65.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text();text=text.replace('`weapon-completion-matrix-v64.json` expands','`weapon-completion-matrix-v65.json` expands',1)
    text+='''
## Actual campaign application frontier, October19

Two freshly built actual Cairo application runs place the original Johnson
squad and initialize the RTX5080, then fail before their first frame on a
scenery lighting-owner guard. Seven companion checks pass in each attempt;
zero weapon inputs or Vulkan weapon commands submit. A read-only per-object
vkCmdDraw observer is implemented, but has no positive application case yet.
The smaller model-only probe does not reproduce this failure and stays red.

The full application identifies dress_cap F9D6179E, placement583, class1,
no override and a full range. Its original instance146 selects baked bucket4
with matching checksum336987148. The scene caches its model bank using only
definition/variant; the first matching placement152 has no BSP0 baked record,
so a dynamic bank is reused for the later baked placement. The actual source
packet and untouched cache records establish this ownership mismatch. See
`checkpoint-weapon-campaign-application-frontier-20261019.json`.

The correction still requires class1 material capability and original per-section
incident/radiance streams, selected per original unit/current BSP instead of
the first placement's lighting mode. The guard remains strict; scenery is not
omitted. GPU/framebuffer and full audio acceptance remain open, with zero of63
weapons fully certified. All owned builds/source/scratch are removed after
terminal runs; only small commands, hashes, reports and diagnostics remain.
''';doc.write_text(text)
    print(json.dumps(dict(evidence_verified=True,application_passed=False,source_class=1,source_instance=146,
        source_changed_since_validation=drift,fully_certified_weapons=0)),flush=True)


if __name__=='__main__':main()
