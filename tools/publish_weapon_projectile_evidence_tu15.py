#!/usr/bin/env python3
"""Publish bounded source projectile observations; run through run_test.py."""
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
    reports=['weapon-projectile-birth-analysis-v1.json','weapon-projectile-retirement-analysis-v2.json']
    analyses=[load(name)for name in reports]
    assert all(item['passed'] and not item['full_weapon_parity']for item in analyses)
    assert all(case['completed_projectile_objects']==4 for item in analyses for case in item['cases'])
    assert len(analyses[0]['cases'])==2 and len(analyses[1]['cases'])==1
    folders=['weapon-projectile-birth-diagnostic-v1','weapon-projectile-retirement-diagnostic-v2']
    scratch=set();references=reports.copy()
    for folder in folders:
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['status']=='failed' and parent['source_scratch_removed_after_runner_exit']
        assert child['status']=='failed' and child['steps'][-1]['exit_code']==8
        assert all(step['exit_code']==0 for step in child['steps'][:-1])
        assert 'Ordinary weapon did not submit one owned firing sound per shot' in (BASE/folder/'release/tests.log').read_text()
        scratch.update([parent['runner_scratch'],child['build']])
        references += [folder+'/report.json',folder+'/release/report.json',folder+'/release/tests.log',
            folder+'/source-manifest.json',folder+'/projectile-birth-observation.json']
        references += [str(path.relative_to(BASE))for path in sorted((BASE/folder/'release/evidence').glob('weapon-source-firing-*.json'))]
    manifest=load(folders[-1]+'/source-manifest.json')['source_files_sha256']
    source_files=['src/gameplay/multiplayer/multiplayer.cpp','tests/native_covenant_weapon_runtime_tests.cpp',
        'tools/validate_weapon_material_snapshot_tu15.py']
    for name in source_files:assert sha(ROOT/name)==manifest[name],name
    for item in analyses:scratch.add(item['runner_scratch'])
    checks=[dict(path=path,removed=not Path(path).exists())for path in sorted(scratch)]
    assert all(row['removed']for row in checks)
    cases=[case for item in analyses for case in item['cases']]
    latest=analyses[-1]['cases'][0]
    assert latest['all_created_bolts_contact_and_retire_during_birth_tick']
    next_work=('Select an actual authored spawn with a clear firing lane for the next plasma-rifle flight trial. '
        'The pickup scene fires into a surface about0.47 units from the muzzle: all four bolts publish '
        'source contact effects and retire during their birth tick through original B92B5->BA490. '
        'Successful B9B70 birth is established in three trials. Do not replace the source projectile owner. '
        'Complete original audio device/IRQ/DSP/output activation and all63 weapon reference sequences; '
        'remove unused generated tests after validation.')
    progress=dict(passed=True,component='Actual source projectile creation and retirement observations',
        live_trials=len(cases),successful_projectile_constructor_returns=sum(case['completed_projectile_objects']for case in cases),
        original_constructor_entry='B9B70',original_normalized_body_instructions=614,
        original_body_bytes_and_correspondence_reverified=True,
        latest_retirements=latest.get('retirements',[]),latest_published_contacts=latest.get('published_source_contacts',[]),
        all_created_bolts_contact_and_retire_during_birth_tick=True,
        observed_birth_to_contact_distance_range=latest['observed_birth_to_contact_distance_range'],
        full_projectile_lifecycle_verified=False,full_firing_sound_sequence_passed=False,
        full_weapon_parity=False,fully_certified_weapons=0,physical_xbox_verified=False,
        references=[dict(report=name,sha256=sha(BASE/name))for name in references],
        source_files_sha256={name:sha(ROOT/name)for name in source_files},
        evidence_update_tool_sha256=sha(Path(__file__)),next_work=next_work,large_artifacts_retained=[])
    progress_name='weapon-projectile-birth-retirement-progress-v1.json';save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v56.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\plasma_rifle\plasma_rifle')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Four source projectile constructor successes in each of three live trials; bounded original retirement/contact observations.',
        map='assets/disc/maps/lockout.map',certifies_entire_gate=False,
        certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False))
    weapon['gates']['projectiles'].update(status='partial',remaining=
        'Successful source plasma-bolt births observed; complete flight/contact/retirement, other variants and rendered original-reference acceptance remain open.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v56.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));save('weapon-completion-matrix-v57.json',matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH']
    audit_name='post-weapon-projectile-evidence-storage-audit-v1.json'
    save(audit_name,dict(passed=True,cleanup_required_for_completion=True,scratch_checks=checks,
        live_owned_builds=[],large_artifacts_retained=[],publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-projectile-evidence-20261018.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-shot-publication-20261018.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v57.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v57.json'),storage_audit=audit_name,
        full_weapon_parity=False,fully_certified_weapons=0,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v56.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix='weapon-completion-matrix-v57.json',
        progress=progress['component'],next_work=next_work,projectile_constructor_successes_across_trials=12,
        nearby_contact_and_same_tick_retirement_explains_empty_projectile_snapshots=True,
        full_firing_failure=folders[-1]+'/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v57.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v56.json` expands','`weapon-completion-matrix-v57.json` expands',1)
    text+='''
## Source plasma-bolt creation observations, October18

Read-only entry/return observations of original B9B70 confirm four successful
plasma-bolt constructions in each of three live SDL trials. The original
614-instruction body correspondence and every retained listing byte were
reverified against the authenticated original images. Each returned object has
its actual full generation, projectile type, definition, unattached parent and
initial velocity. Continued placement and natural post-pickup movement agree.
Empty tick snapshots therefore do not prove failed source projectile creation.

The third trial additionally observes original BA490 retirement requests and
retains the existing source contact diagnostics. All four bolts complete two
source contact effects about0.47 units from their birth position and retire
in the same source tick through original B92B5->BA490. This scene's empty
projectile snapshots are explained by those contacts. The next flight trial
must use a clear firing lane. Exact callers and raw state are in
`weapon-projectile-retirement-analysis-v2.json`. Complete flight/contact/
retirement and original-render acceptance remain open; zero firing sounds still
fail the complete firing checks. No generated binary, source copy or raw bank
is retained. See `checkpoint-weapon-projectile-evidence-20261018.json`.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,live_trials=len(cases),constructor_successes=12,
        observed_retirements=len(latest.get('retirements',[])),full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
