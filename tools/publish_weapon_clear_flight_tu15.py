#!/usr/bin/env python3
"""Publish bounded live flight/glow evidence; run through tools/run_test.py."""
import copy
import hashlib
import json
import os
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load(name):return json.loads((BASE/name).read_text())
def save(name,data):(BASE/name).write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')


def main():
    progress_name='weapon-clear-flight-analysis-v1.json';progress=load(progress_name)
    assert progress['passed'] and progress['source_created_objects']==progress['live_flight_owners']==4
    assert not progress['full_weapon_parity'] and not progress['full_firing_sound_sequence_passed']
    folder='weapon-clear-flight-validation-v1'
    parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
    assert parent['status']=='failed' and parent['source_scratch_removed_after_runner_exit']
    assert child['status']=='failed' and child['steps'][-1]['exit_code']==8
    assert all(step['exit_code']==0 for step in child['steps'][:-1])
    log=(BASE/folder/'release/tests.log').read_text()
    assert 'Clear-lane observations:'in log and 'Ordinary weapon did not submit one owned firing sound per shot'in log
    manifest=load(folder+'/source-manifest.json')['source_files_sha256']
    for name in ['tests/native_covenant_weapon_runtime_tests.cpp','tools/validate_weapon_material_snapshot_tu15.py']:
        assert manifest[name]==sha(ROOT/name),name
    source_analysis_name='weapon-clear-flight-source-analysis-v1.json';source=load(source_analysis_name)
    paths={parent['runner_scratch'],child['build'],progress['runner_scratch'],source['runner_scratch']}
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(paths)]
    assert all(row['removed']for row in checks)
    next_work=('Compare actual clear-lane glow inputs with unchanged original TU1.5 sprite/projector owners. '
        'The sustained flight trial now exposes all four original source bolt handles and their actual gameplay-view glow submissions. '
        'Record required GPU inputs only inside owned scratch and retain small comparison receipts. '
        'Complete audio device/IRQ/DSP/output activation and the complete feature/rendered-reference sequences for all63 weapons; '
        'delete unused generated tests after validation. No weapon is fully certified.')
    matrix=copy.deepcopy(load('weapon-completion-matrix-v57.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\plasma_rifle\plasma_rifle')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual source bolt flight publication and authored glow submission in a clear authored scene.',
        map='assets/disc/maps/lockout.map',certifies_entire_gate=False,certifies_all_map_variants=False,
        component_covers_all_scoped_maps_for_weapon=False))
    weapon['gates']['projectiles'].update(status='partial',remaining=
        'Four source bolts observed through sustained flight, contact effects and retirement with matching full owners. Full original-reference lifecycle acceptance, other variants and original-render acceptance remain open.')
    weapon['gates']['materials_effects'].update(status='partial',remaining=
        'All four live source bolts submit authored glow into the actual gameplay view. Original draw-input comparison for this sequence, framebuffer parity and complete materials/effects remain open.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v57.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));save('weapon-completion-matrix-v58.json',matrix)
    refs=[progress_name,source_analysis_name,folder+'/report.json',folder+'/release/report.json',
        folder+'/release/tests.log',folder+'/source-manifest.json',folder+'/projectile-birth-observation.json',
        folder+'/release/evidence/weapon-source-firing-pinned.json']
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-clear-flight-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,large_artifacts_retained=[],live_owned_builds=[],
        cleanup_required_for_completion=True,publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-clear-flight-20261018.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-projectile-evidence-20261018.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v58.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v58.json'),storage_audit=audit_name,
        references=[dict(report=n,sha256=sha(BASE/n))for n in refs],
        fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v57.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix='weapon-completion-matrix-v58.json',
        progress=progress['component'],next_work=next_work,full_firing_failure=folder+'/release/tests.log',
        source_flight_checked_samples=progress['checked_live_samples'],
        source_flight_maximum_observed_travel=progress['observed_source_travel_maximum'],
        authored_glow_draws=progress['authored_glow_draws'],
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v58.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v57.json` expands','`weapon-completion-matrix-v58.json` expands',1)
    text+='''
## Source plasma-bolt flight in a clear authored scene, October18

The actual SDL pickup/fire fixture now supports an authored spawn selected by
host clearance rays. Those rays only choose scene inputs; original B95D0 owns
placement and original projectile routines still own all movement and contacts.
All four constructed bolt handles are observed in sustained flight, with
monotonic travel read directly from their actual source banks. Each also submits
its authored glow with valid vertices into the real gameplay viewport. All four
complete13 source ticks of flight, contact effects and retirement in this scene. Exact
counts, selected spawn, source hashes and failure diagnostics are preserved in
`checkpoint-weapon-clear-flight-20261018.json`.

The four focused material/heat/charge checks pass. The complete firing check
still fails on zero firing sounds. This sequence does not yet compare its glow
inputs with independent original execution or certify an Xbox framebuffer,
complete projectile lifecycle, whole weapon or other map variants. Temporary
build/source trees are removed; no new large reference inputs are retained.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,flight_samples=progress['checked_live_samples'],
        glow_draws=progress['authored_glow_draws'],full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
