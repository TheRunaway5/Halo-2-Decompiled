#!/usr/bin/env python3
"""Publish actual campaign weapon-unit/section/input evidence; use run_test.py."""
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
    previous_audit=BASE/'post-weapon-campaign-source-storage-audit-v1.json'
    if previous_audit.exists():
        prior=json.loads(previous_audit.read_text());scratch.add(prior['publisher_scratch'])
        scratch.update(row['path']for row in prior['scratch_checks'])
    for version in (1,2):
        folder=f'weapon-campaign-source-validation-v{version}'
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['source_scratch_removed_after_runner_exit'] and not parent['source_cleanup_pending_runner_exit']
        assert parent['steps'][0]['report_sha256']==sha(BASE/folder/'release/report.json')
        scratch.update((parent['runner_scratch'],child['build']))
        references += [folder+'/'+n for n in ('report.json','release/report.json','source-manifest.json','release/build.log')]
        if version==1:
            assert parent['status']==child['status']=='failed' and child['steps'][-1]['exit_code']==2
            assert "unknown type name 'ObjectLightingState'"in (BASE/folder/'release/build.log').read_text()
        else:
            assert parent['passed'] and child['passed'] and all(s['exit_code']==0 for s in child['steps'])
            log=(BASE/folder/'release/tests.log').read_text()
            assert '100% tests passed' in log and log.count('   Passed')==7 and '***Failed' not in log
            assert 'weapon_campaign_source_submission'in log
            references.append(folder+'/release/tests.log')
    final='weapon-campaign-source-validation-v2'
    case_name=final+'/release/evidence/weapon-campaign-source-submission.json';case=load(case_name);references.append(case_name)
    assert case['passed'] and case['source_rng_unchanged'] and case['source_weapon_submission_verified']
    assert case['source_weapon_no_stale_draw_without_admission'] and case['source_weapon_vertices']>0
    assert case['source_weapon_full_owners']==case['attached_weapons']>0
    assert all(row['source_lighting_words_equal'] and row['vertices']>0 for row in case['weapon_submissions'])
    assert case['source_weapon_shader_inputs']==sum(row['shader_inputs_verified']for row in case['weapon_submissions'])
    scope=load('authored-scope.json');cairo=next(m for m in scope['maps']if Path(m['path']).stem=='01b_spacestation')
    definitions={int(row['tag']['datum'],16):row for row in cairo['weapons']}
    tested=sorted({definitions[row['definition']]['tag']['name']for row in case['weapon_submissions']})
    world_only=sorted({definitions[row['definition']]['tag']['name']for row in case['weapon_submissions']
        if not definitions[row['definition']]['first_person']})
    assert world_only==[r'objects\weapons\fixed\h_turret_mp\item\h_turret_mp_item']
    references += ['weapon-world-model-count-original-v1.json','weapon-world-model-scope-v1.json']
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)]
    assert all(r['removed']for r in checks)
    source_files=['CMakeLists.txt','include/halo2/source_campaign_weapons.hpp','src/campaign/source_campaign_weapons.cpp',
        'src/app/main.cpp','tests/campaign_model_geometry_runtime_tu15_tests.cpp','tools/validate_weapon_material_snapshot_tu15.py']
    pins=load(final+'/source-manifest.json')['source_files_sha256'];current={n:sha(ROOT/n)for n in source_files}
    # The manifest hashes CMake before private test registrations are appended,
    # so its original root bytes can be compared alongside the source files.
    drift=[n for n in source_files if pins[n]!=current[n]]
    next_work=('Verify actual campaign GPU submission and full weapon material inputs, followed by other '
        'AI/player/loose/objective/seat owners and every applicable map. The94 multiplayer-filter '
        'metadata exclusions are not94 proved missing campaign pixel cases: campaign selects its own '
        'first-person record, and source weapon residency is now independent of that choice. '
        'Complete overlays, ownership/lifetime transitions, BSP restores, interpolation, split/LAN '
        'views and all63 paths/630 gates, including original audio device/IRQ/DSP/output owners. '
        'Remove unused generated tests/builds/scratch after validation and retain small evidence.')
    progress_name='weapon-campaign-source-submission-progress-v1.json'
    progress=dict(passed=True,component='Campaign original admitted AI-held/loose weapon submission',
        actual_source_case=case_name,tested_weapon_paths=tested,source_weapon_submissions=case['weapon_submissions'],
        tested_world_only_paths_without_authored_first_person_records=world_only,
        source_weapon_vertices=case['source_weapon_vertices'],source_weapon_full_owners=case['source_weapon_full_owners'],
        source_weapon_shader_inputs=case['source_weapon_shader_inputs'],source_rng_unchanged=True,
        authored_world_resources_independent_of_first_person_eligibility=True,
        original_admitted_sections_and_material_lods_compared_to_independent_cache_decode=True,
        completed_world_geometry_and_cached_lighting_words_equal=True,stale_draw_without_admission_rejected=True,
        source_shader_inputs='Actual object+12C/+16C/+180/+184, both fixed magazines as authored, primary trigger/charge; unresolved producers reject',
        pending_overlays_kept_separate=True,bsp_material_rebase_and_pose_reset_implemented=True,
        focused_checks_passed=4,actual_campaign_cpu_scenario_passed=True,application_players_verified=[1,4],
        actual_campaign_application_gpu_draw_verified=False,framebuffer_parity_verified=False,
        all_world_model_resource_variants_verified=False,full_native_firing_audio_verified=False,
        full_weapon_parity=False,fully_certified_weapons=0,large_artifacts_retained=[],
        original_instruction_patches=[],supplied_callee_results=[],next_work=next_work,
        validated_snapshot_source_files_sha256={n:pins[n]for n in source_files},current_source_files_sha256=current,
        source_changed_since_validation=drift,references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v63.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    for weapon in matrix['weapons']:
        if weapon['name']not in tested:continue
        weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
            component='Actual original Cairo ai_place/child/main units now reach the production campaign weapon scene; authored sections/WORLD/material LODs and cached lighting match. CPU declared camera; GPU/framebuffer and all variants remain unverified.',
            certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
            map='assets/disc/maps/01b_spacestation.map'))
        weapon['gates']['poses_models'].update(status='partial',remaining=
            'Actual Cairo actor-held weapon units match selected authored sections skinned by completed original WORLD poses. Actual campaign GPU, intermediate poses, ownership transitions, every species/view/map variant and full reference sequences remain required.')
        weapon['gates']['materials_effects'].update(status='partial',remaining=
            'Source cached lighting words and selected authored material identities/LODs reach the campaign weapon scene. Complete shader producers/effects/overlays, current pixels and all variants remain required.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v63.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));matrix_name='weapon-completion-matrix-v64.json';save(matrix_name,matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-campaign-source-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],
        cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint='checkpoint-weapon-campaign-source-submission-20261019.json'
    save(checkpoint,dict(passed=True,previous_checkpoint='checkpoint-weapon-loose-source-pose-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix=matrix_name,matrix_sha256=sha(BASE/matrix_name),
        storage_audit=audit_name,fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v63.json');state.update(latest_checkpoint=checkpoint,latest_matrix=matrix_name,
        progress=progress['component'],campaign_source_weapon_case=case_name,next_work=next_work,
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v64.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v63.json` expands','`weapon-completion-matrix-v64.json` expands',1)
    section='''
## Campaign source weapon units, October19

The original campaign collector separately builds actor-held weapon units, but
the old drawing loop submitted actor bodies and native pickup previews. The new
campaign weapon scene prepares authored world resources independently of FP or
pickup eligibility, and submits only completed original main units. It retains
each unit's section order/LOD, completed WORLD skin, packed colors and cached
lighting; shader state comes from that actual weapon's source fields and
magazines. Unknown producers and resource owners remain explicit boundaries.
Overrides stay separate. BSP material rebases and restored/discontinuous pose
histories reset without advancing simulation.

The actual Cairo ai_place scenario now submits its original attached weapon.
It is the authored h_turret_mp_item, whose FP record bank is empty; this is an
actual source-owned world-only positive, not a synthetic pickup eligibility case.
Geometry matches independently decoded authored sections skinned by the source
WORLD bank, material identities/LODs match, cached lighting words match, and
three actual source magazine/builtin shader inputs pass per submitted unit.
Source RNG is unchanged; absent original admission emits no stale draw. Exact
counts and source hashes are in
`checkpoint-weapon-campaign-source-submission-20261019.json`. Four focused and
two finite application checks pass, together with the armed CPU scenario.
The application checks here are multiplayer headless checks; composed campaign
GPU submission, final pixels, other map/owner variants and full audio remain open.

The earlier94-row result describes the multiplayer body_guns filter, not94
proved missing campaign pixel cases. Campaign selects its own FP record; the
new world scene has no FP residency prerequisite. Keep original owner eligibility
and full visual acceptance separate. Temporary source/build/probe outputs are
removed after terminal runners; small diagnostics and evidence remain. Full
certification is still zero of63 weapons.
'''
    marker='\n## Campaign source weapon units, October19\n'
    if marker in text:text=text[:text.index(marker)]
    doc.write_text(text+section)
    print(json.dumps(dict(passed=True,tested_weapon_paths=tested,weapon_vertices=case['source_weapon_vertices'],
        source_changed_since_validation=drift,fully_certified_weapons=0)),flush=True)


if __name__=='__main__':main()
