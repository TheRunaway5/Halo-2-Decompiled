#!/usr/bin/env python3
"""Publish original-bank world-weapon readback evidence; use run_test.py."""
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
    before=load('weapon-dual-drop-lifetime-comparison-v1.json')
    after=load('weapon-dual-drop-lifetime-comparison-v2.json')
    assert not before['passed'] and after['passed']
    assert sum(c['lifetime']['publication_mismatches']for c in before['cases'])==2130
    assert sum(c['lifetime']['publication_checks']for c in after['cases'])==2130
    assert all(c['lifetime']['publication_mismatches']==0 for c in after['cases'])
    scratch=set();references=[]
    for version in (1,2,3):
        folder=f'weapon-dual-drop-lifetime-validation-v{version}'
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['source_scratch_removed_after_runner_exit']
        scratch.update((parent['runner_scratch'],child['build']))
        references += [folder+'/'+n for n in ('report.json','release/report.json','source-manifest.json')]
        if version==1:
            assert parent['status']==child['status']=='cancelled' and parent['cancellation_exit_code']==143
            continue
        assert parent['status']==child['status']=='failed'
        assert all(s['exit_code']==0 for s in child['steps'][:-1])
        assert child['steps'][-1]['name']=='tests' and child['steps'][-1]['exit_code']==8
        log=(BASE/folder/'release/tests.log').read_text()
        if version==2:assert log.count('Detached source identity/ammunition/root differs from world publication')==2
        else:
            assert 'Detached source identity/ammunition/root differs'not in log
            assert log.count('Live dual SMGs did not submit exactly one authored sound per fired round')==2
            assert 'weapon_material_application_1_players' in log and 'weapon_material_application_4_players' in log
            assert log.count('   Passed')==6
        references += [folder+'/release/tests.log',folder+'/dual-drop-retirement-observation.json']
        references += [folder+'/release/evidence/weapon-source-dual-'+s+'.json'for s in ('spartan','elite')]
    cases=[]
    fields=('object','definition','parent','inventory_owned','world_publications','loaded','reserve',
            'position','object_words_120','weapon_words_16c','source_nodes','preview_nodes','source_world_node_words')
    for species in ('spartan','elite'):
        paths=[f'weapon-dual-drop-lifetime-validation-v{v}/release/evidence/weapon-source-dual-{species}.json'for v in (2,3)]
        old,new=[load(p)for p in paths]
        assert old['render_loop_completed'] and new['render_loop_completed']
        old_rows,new_rows=old['drop_samples'],new['drop_samples']
        assert len(old_rows)==len(new_rows)==75
        differences=[]
        for a,b in zip(old_rows,new_rows):
            for field in fields:
                if a.get(field)!=b.get(field):differences.append(dict(tick=b['tick'],field=field))
        first=new_rows[0];owner=first['object']
        last_held=next(h for r in reversed(old['samples'])for h in r['hands']if h['datum']==owner)
        assert first['published_reserve']==first['reserve']==53
        assert last_held['published_reserve']==116
        assert all(r['publication_matches'] and r['published_reserve']==r['reserve'] and
            r['published_loaded']==r['loaded']for r in new_rows)
        assert not new['drop_lifetime']['source_retirement_observed']
        assert new['drop_lifetime']['headless_neutral_ticks']==1024
        assert new['observations']['firing_sounds']==0 and new['observations']['fired_rounds']==7
        cases.append(dict(species=species,representation=new['representation'],
            source_unit_definition=new['expected_unit_definition'],matched_world_publications=1065,
            former_held_reserve=last_held['published_reserve'],first_source_drop_reserve=first['reserve'],
            first_corrected_world_reserve=first['published_reserve'],samples=len(new_rows),
            unchanged_source_sample_fields=not differences,source_sample_field_differences=differences,
            headless_neutral_ticks=1024,source_retirement_observed=False,
            completed_source_preview_node_differences=new['drop_lifetime']['completed_source_preview_differences'],
            actual_source_nodes=first['source_nodes'],preview_nodes=first['preview_nodes'],
            gpu_or_framebuffer_comparison=False,firing_sound_submissions=0))
    comparison_names=['weapon-dual-drop-lifetime-comparison-v1.json','weapon-dual-drop-lifetime-comparison-v2.json']
    for name in comparison_names:
        r=load(name);scratch.add(r['runner_scratch']);references.append(name)
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)]
    assert all(r['removed']for r in checks)
    source_files=['src/gameplay/multiplayer/multiplayer.cpp','include/halo2/multiplayer.hpp','tests/native_weapon_runtime_tests.cpp',
        'tools/validate_weapon_material_snapshot_tu15.py']
    pins=load('weapon-dual-drop-lifetime-validation-v3/source-manifest.json')['source_files_sha256']
    assert all(pins[n]==sha(ROOT/n)for n in source_files)
    next_work=('Publish and render original WORLD node banks for loose weapons, including LAN owner/pose '
        'transport, instead of locally sampling a preview idle. Quantify the current pose difference against '
        'the original render palette and verify composed views. Continue other dual-compatible pairs, '
        'interruptions, all63 weapon paths/630 gates and real device/IRQ/DSP/output audio. '
        'No retirement timer is certified from the1024-tick unretired window. Remove all unused generated '
        'test/build/scratch files after validation while retaining small evidence and required references.')
    progress_name='weapon-dual-drop-lifetime-progress-v1.json'
    progress=dict(passed=True,component='Source-authoritative world-weapon resource readback after detach',
        cases=cases,matched_world_publications=2130,previous_different_world_publications=2130,
        shared_readback_for_held_and_world_weapons=True,application_players_verified=[1,4],
        actual_original_outcomes_compared_without_masks=True,
        original_instruction_patches=[],supplied_callee_results=[],
        retirement_within_window_observed=False,full_retirement_policy_verified=False,
        full_weapon_parity=False,framebuffer_parity_verified=False,fully_certified_weapons=0,
        complete_firing_sound_sequence_passed=False,large_artifacts_retained=[],next_work=next_work,
        references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        source_files_sha256={n:sha(ROOT/n)for n in source_files},evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v61.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\smg\smg')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual detached full-datum inventory/root/ammunition readback matches2130 world publications across both source species. Original reserves redistribute116-to53 on drop; both1024-tick windows remain live. Current idle preview differs from completed source nodes; GPU acceptance remains open.',
        certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
        map='assets/disc/maps/lockout.map'))
    weapon['gates']['inventory_lifetime'].update(status='partial',remaining=
        'Both species match2130 detached world publications through1024 neutral headless ticks. Full retirement policy, original rendered poses, mixed pairs, interruptions and all map variants remain required.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v61.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));matrix_name='weapon-completion-matrix-v62.json';save(matrix_name,matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-dual-drop-lifetime-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],
        cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-dual-drop-lifetime-20261019.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-dual-source-history-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix=matrix_name,matrix_sha256=sha(BASE/matrix_name),
        storage_audit=audit_name,fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v61.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix=matrix_name,progress=progress['component'],
        next_work=next_work,dual_drop_lifetime_cases=cases,
        full_firing_failure='weapon-dual-drop-lifetime-validation-v3/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v62.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v61.json` expands','`weapon-completion-matrix-v62.json` expands',1)
    text+='''
## Detached weapon resource readback, October19

The real drop sequence redistributes reserves from116 while held to53 in the
detached original object. The native world item retained its former held state.
Held and loose weapons now share the same source-bank readback for both barrels,
triggers, magazines, preparation clocks and shader fields. Both species match
all2130 detached identity/inventory/root/ammunition publications, including1024
neutral headless ticks each. Both objects remain live in that window; no cleanup
timer or complete retirement policy is inferred. See
`checkpoint-weapon-dual-drop-lifetime-20261019.json`.

Completed source node banks differ from the local pickup idle sampler in these
scenes. This is a completed-tick matrix comparison, not an interpolated GPU or
framebuffer proof. Loose-weapon source pose publication/rendering and LAN pose
ownership are the next required visual work. The fresh application passes one-
and four-player finite checks; four focused checks pass. Both complete live
tests still fail on seven rounds and zero firing sounds. Private source/build/
scratch trees are removed, including the cancelled fixture setup; only small
reports and source-node evidence remain. Full certification stays zero of63.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,matched_world_publications=2130,full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
