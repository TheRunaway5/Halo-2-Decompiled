#!/usr/bin/env python3
"""Publish bounded original loose-weapon draw/transport evidence; use run_test.py."""
import argparse
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
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--final-version',type=int,default=4)
    parser.add_argument('--comparison',default='weapon-loose-source-pose-comparison-v2.json')
    args=parser.parse_args();assert args.final_version>=3
    comparison=load(args.comparison);assert comparison['passed']
    assert comparison['original_node_and_palette_mismatches']==0
    references=[args.comparison];scratch={comparison['runner_scratch']};failures=[]
    for version in range(1,args.final_version+1):
        folder=f'weapon-loose-source-pose-validation-v{version}'
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['source_scratch_removed_after_runner_exit']
        assert not parent['source_cleanup_pending_runner_exit']
        assert parent['status']==child['status']=='failed'
        assert parent['steps'][0]['report_sha256']==sha(BASE/folder/'release/report.json')
        assert all(s['exit_code']==0 for s in child['steps'][:-1])
        assert child['steps'][-1]['name']=='tests' and child['steps'][-1]['exit_code']==8
        scratch.update((parent['runner_scratch'],child['build']))
        log=(BASE/folder/'release/tests.log').read_text()
        assert '2 tests failed out of 9'in log and log.count('   Passed')==7
        assert 'native_source_scene_physics'in log
        references += [folder+'/'+n for n in ('report.json','release/report.json','source-manifest.json','release/tests.log')]
        cases=[]
        for species in ('spartan','elite'):
            path=folder+f'/release/evidence/weapon-source-dual-{species}.json';references.append(path)
            trace=load(path);metrics=trace['world_pose_metrics']
            assert trace['render_loop_completed'] and metrics['mismatches']==0
            cases.append(dict(species=species,draws=metrics['draws'],palette_matrices=metrics['original_palette_matrices'],
                node_publications=metrics['publication_checks'],lan_admission=metrics.get('lan_admission'),
                native_lan_poses=metrics.get('native_lan_poses',0)))
        if version<3:
            assert 'Loose weapon native LAN admission failed'in log
            if version==2:
                admission=cases[1]['lan_admission']
                assert not admission['ready'] and admission['host_peers']==1
                assert admission['rejected_packets']>0 and admission['last_packet_rejection']=='Invalid LAN source explosion location'
            failures.append(dict(validation=folder,cases=cases,observed_failure='Elite snapshot rejected at original explosion transform validation'))
        else:
            assert 'Loose weapon native LAN admission failed'not in log
            assert log.count('Live dual SMGs did not submit exactly one authored sound per fired round')==2
            assert all(c['lan_admission']['ready'] and c['lan_admission']['rejected_packets']==0 for c in cases)
    original=load('loose-pose-explosion-scale-original-v1.json')
    assert original['passed'] and original['entry']=='17BA50' and len(original['cases'])==6
    assert all(c['all_52_transform_bytes_preserved']for c in original['cases'])
    assert not original['original_instruction_patches'] and not original['supplied_callee_results']
    assert original['original_xbe_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    model_scope=load('weapon-world-model-scope-v1.json')
    assert model_scope['authored_metadata_audit_completed'] and model_scope['authored_rows']==1110
    assert model_scope['weapon_paths']==63 and len(model_scope['maps'])==38
    assert sum(model_scope['counts'].values())==1110
    world_count=load('weapon-world-model-count-original-v1.json')
    assert world_count['passed'] and world_count['entry']=='3D3D0'
    assert world_count['executed_authored_model_chains']==667 and world_count['caller_model_NONE_rows_not_executed']==443
    assert len(world_count['excluded_authored_rows_with_original_world_count_one'])==94
    assert world_count['first_person_count_and_block_reads']==0
    assert not world_count['original_instruction_patches'] and not world_count['supplied_callee_results']
    for name in ('loose-pose-explosion-scale-original-v1.json','loose-pose-grenade-location-authored-v1.json',
        'weapon-world-model-scope-v1.json','weapon-world-model-count-original-v1.json'):
        scratch.add(load(name)['runner_scratch']);references.append(name)
    # Keep the first corrected snapshot comparison as well as the refreshed build.
    if args.comparison!='weapon-loose-source-pose-comparison-v1.json':
        references.append('weapon-loose-source-pose-comparison-v1.json')
        scratch.add(load('weapon-loose-source-pose-comparison-v1.json')['runner_scratch'])
    final_folder=f'weapon-loose-source-pose-validation-v{args.final_version}'
    cases=[]
    for row in comparison['cases']:
        metrics=row['metrics'];scales=row['original_explosion_scales']
        assert row['render_inputs_and_native_transport_passed'] and metrics['render_fraction']==1
        assert metrics['native_lan_explosion_locations']==len(scales)>0
        assert metrics['native_lan_protocol']==90 and metrics['native_lan_poses']>0
        assert row['fired_rounds']==7 and row['firing_sound_submissions']==0
        cases.append(dict(species=row['species'],representation=row['representation'],
            source_unit_definition=row['source_unit_definition'],draws=metrics['draws'],
            node_publications=metrics['publication_checks'],palette_matrices=metrics['original_palette_matrices'],
            node_or_palette_mismatches=0,native_lan_poses=metrics['native_lan_poses'],
            native_lan_explosion_locations=len(scales),native_lan_rejected_packets=0,
            original_explosion_scale_words=sorted({r['word']for r in scales}),firing_sound_submissions=0))
    assert 0x3fc00000 in next(c for c in cases if c['species']=='elite')['original_explosion_scale_words']
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)]
    assert all(r['removed']for r in checks)
    source_files=['src/multiplayer.cpp','include/halo2/multiplayer.hpp','src/multiplayer_network.cpp',
        'include/halo2/multiplayer_network.hpp','include/halo2/multiplayer_datagrams.hpp',
        'src/multiplayer_presentation.cpp','src/multiplayer_runtime.cpp','tests/native_weapon_runtime_tests.cpp',
        'tests/source_scene_physics_tests.cpp','tools/validate_weapon_material_snapshot_tu15.py']
    pins=load(final_folder+'/source-manifest.json')['source_files_sha256']
    current={n:sha(ROOT/n)for n in source_files}
    drift=[n for n in source_files if current[n]!=pins[n]]
    next_work=('Remove the first-person-record restriction from world-visual preparation only after checking '
        'actual model/render/owner contracts for the94 excluded authored rows across27 paths. '
        'Keep player pickup/AI/seat/objective eligibility with its original owner. Verify intermediate '
        'render fractions, split views and composed LAN geometry/LOD/lighting; complete other pairs and '
        'all63 paths/630 gates, including the real device/IRQ/DSP/output audio owner. Remove unused '
        'test/build/scratch outputs after validation, retaining small evidence and required references.')
    progress_name='weapon-loose-source-pose-progress-v1.json'
    progress=dict(passed=True,component='Original loose-weapon WORLD and accepted palette submission with native LAN ownership',
        cases=cases,original_node_publications=comparison['original_node_publications'],
        original_completed_draws=comparison['original_completed_draws'],
        original_palette_matrices=comparison['original_palette_matrices'],node_and_palette_mismatches=0,
        native_lan_protocol=90,native_lan_pose_banks=sum(c['native_lan_poses']for c in cases),
        native_lan_explosion_transforms=sum(c['native_lan_explosion_locations']for c in cases),
        earlier_failures=failures,explosion_scale_contract='Original17BA50 preserves scale and full52-byte WORLD transform; actual Elite source locations include1.5',
        local_submission='Actual original accepted geometry/LOD/regions, rendered WORLD nodes and cached lighting; single submission per owner/view',
        attachment_interpolation_cuts_verified=True,application_players_verified=[1,4],focused_checks_passed=5,
        full_native_tests_passed=False,full_native_failures='Both species: seven rounds, zero firing sound submissions',
        metadata_world_model_filter_audit=model_scope['counts'],
        original_world_model_count_chains=667,original_world_model_count_one_excluded_rows=94,
        original_owner_eligibility_for_world_only_variants_verified=False,
        render_fraction=1,intermediate_render_fraction_verified=False,composed_lan_visuals_verified=False,
        framebuffer_parity_verified=False,full_weapon_parity=False,fully_certified_weapons=0,
        original_instruction_patches=[],supplied_callee_results=[],large_artifacts_retained=[],next_work=next_work,
        references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        validated_snapshot_source_files_sha256={n:pins[n]for n in source_files},
        current_source_files_sha256=current,source_changed_since_validation=drift,
        evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v62.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\smg\smg')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Dual-SMG scenarios: completed loose-weapon WORLD/palettes match original across both species; native LAN carries loose poses and unchanged non-unit explosion transforms. Endpoint inputs, not framebuffer parity.',
        certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
        map='assets/disc/maps/lockout.map'))
    weapon['gates']['poses_models'].update(status='partial',remaining=
        'Completed-tick loose WORLD/palette inputs match both species in Lockout dual-SMG scenarios. Intermediate interpolation, full first/third-person/IK/camera sequences, split/LAN views and all variants remain required.')
    weapon['gates']['materials_effects'].update(status='partial',remaining=
        'Loose original accepted geometry/LOD/region/palette and cached lighting reach the local draw path. Full framebuffer/material/effect acceptance and LAN view-dependent selection/lighting remain required.')
    weapon['gates']['inventory_lifetime']['remaining']='Both species match2130 detached world publications through1024 neutral ticks; loose source WORLD nodes and attach/drop interpolation cuts now have bounded checks. Retirement policy, mixed pairs, interruptions and all variants remain required.'
    weapon['gates']['hud_audio_publication']['remaining']+=' Native LAN protocol90 carries loose WORLD ownership and all observed original explosion transforms without rejection; complete live firing audio remains zero.'
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v62.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));matrix_name='weapon-completion-matrix-v63.json';save(matrix_name,matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-loose-source-pose-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],
        cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-loose-source-pose-20261019.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-dual-drop-lifetime-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix=matrix_name,matrix_sha256=sha(BASE/matrix_name),
        storage_audit=audit_name,fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v62.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix=matrix_name,progress=progress['component'],
        next_work=next_work,loose_weapon_pose_cases=cases,full_firing_failure=final_folder+'/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    state['codex_usage_instruction'].update(redemption_control_available=True,
        action_requested='At1 percent remaining, redeem one available full reset as explicitly requested by the user',
        fallback='Use fresh get_usage_limits; consume_usage_reset is now exposed. Reuse one idempotency key if an attempt is uncertain.')
    save('continuation-working-state-v63.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v62.json` expands','`weapon-completion-matrix-v63.json` expands',1)
    text+='''
## Loose weapon original poses and native transport, October19

Loose weapons now publish original WORLD nodes, participate in native LAN pose
ownership, and use original accepted geometry, LOD/regions, rendered nodes and
cached lighting in local source-owned views. Attach/drop transitions cut the
pose interpolation across inventory ownership changes. Native LAN protocol90
rejects older decoders that cannot admit loose owners. Both species match the
completed-tick nodes and original selected-region palette matrices in actual
dual-SMG scenarios. See `checkpoint-weapon-loose-source-pose-20261019.json` for
exact counts, source hashes and retained failures.

The first Elite LAN attempts exposed an existing explosion-location decoder
restriction to scale1. Original17BA50 preserves all52 WORLD transform bytes;
the live Elite scene contains scale1.5. The decoder now retains finite source
scale using the existing firing-location bound. Both species admit with zero
packet rejections, and every observed source explosion transform survives
byte-for-byte. All five focused checks and both finite application checks pass.
Both full live tests still fail on seven rounds and zero firing sounds each.
Endpoint matrices do not certify intermediate interpolation, split views,
composed LAN LOD/lighting or an Xbox framebuffer.

The all-map metadata audit also finds94 of1110 authored rows with world models
excluded by the current first-person-record filter, spanning27 weapon paths.
Unchanged original3D3D0 returns one world model for all667 authored model
chains, including those94 rows, without reading a first-person bank. The443
model-NONE rows are excluded before that original call. These campaign variants
still require actual owner/render checks; a model count does not authorize
native pickup or equip behavior. Keep the complete63-path/630-gate scope open.
All validation source/build/scratch trees are removed after terminal runners;
only small reports remain. Full weapon certification stays zero.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,draws=progress['original_completed_draws'],
        matrices=progress['original_palette_matrices'],source_changed_since_validation=drift,
        full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
