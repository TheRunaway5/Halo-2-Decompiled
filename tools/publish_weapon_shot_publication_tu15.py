#!/usr/bin/env python3
"""Publish bounded live source shot-time evidence; run through run_test.py."""
import copy
import hashlib
import json
import os
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(name):
    return json.loads((BASE/name).read_text())


def save(name,value):
    (BASE/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    comparison_name='source-weapon-shot-publication-comparison-v1.json'
    comparison=load(comparison_name)
    assert comparison['passed'] and not comparison['full_weapon_parity']
    assert not comparison['full_firing_sound_sequence_passed']
    assert comparison['production_source_sha256']==sha(ROOT/'src/gameplay/multiplayer/multiplayer.cpp')
    before='weapon-source-firing-diagnostic-v1'
    after='weapon-source-firing-publication-v2'
    references=[comparison_name,'weapon-preparation-counter-static-v1.json',
        'weapon-preparation-counter-static-v2.json','weapon-preparation-clock-store-v1.json',
        'weapon-last-shot-constructor-static-v1.json']
    scratch_paths=set()
    for folder in (before,after):
        parent=load(folder+'/report.json')
        assert parent['status']=='failed' and not parent['passed']
        assert parent['source_scratch_removed_after_runner_exit']
        child=load(folder+'/release/report.json')
        assert not child['passed'] and child['status']=='failed'
        assert child['steps'][-1]['name']=='tests' and child['steps'][-1]['exit_code']==8
        assert all(step['exit_code']==0 for step in child['steps'][:-1])
        scratch_paths.add(parent['runner_scratch']);scratch_paths.add(child['build'])
        references += [folder+'/report.json',folder+'/release/report.json',
            folder+'/source-manifest.json',folder+'/release/tests.log']
        references += [folder+'/release/evidence/weapon-source-firing-'+label+'.json'
            for label in ('pinned','native_motion')]
    manifest=load(after+'/source-manifest.json')['source_files_sha256']
    source_files=['src/gameplay/multiplayer/multiplayer.cpp','tests/native_covenant_weapon_runtime_tests.cpp',
        'tools/validate_weapon_material_snapshot_tu15.py']
    for name in source_files:
        assert manifest[name]==sha(ROOT/name),name
    def collect_scratch(value):
        if isinstance(value,dict):
            for item in value.values():collect_scratch(item)
        elif isinstance(value,list):
            for item in value:collect_scratch(item)
        elif isinstance(value,str) and value.startswith(str(ROOT/'.local/test-scratch')+'/'):
            relative=Path(value).relative_to(ROOT/'.local/test-scratch')
            if relative.parts[0].startswith('halo2-test-'):
                scratch_paths.add(str(ROOT/'.local/test-scratch'/relative.parts[0]))
    for name in references:
        if name.endswith('.json') and not name.endswith('source-manifest.json'):
            collect_scratch(load(name))
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch_paths)]
    assert all(item['removed']for item in checks)
    matched=sum(case['versions'][1]['samples']for case in comparison['cases'])
    old_mismatches=sum(case['versions'][0]['different_publications']for case in comparison['cases'])
    next_work=('Continue actual TU1.5 firing effects, projectile admission and audio activation. '
        'Live source timestamps now publish correctly; both placement modes still submit zero firing sounds. '
        'The legacy projectile presentation list is not proof that the original world created no projectiles. '
        'Resolve the actual224EC0 device/IRQ/DSP/output owners without supplying success, and complete '
        'rendered reference sequences for all63 weapon paths. Remove unused generated tests after validation.')
    progress=dict(passed=True,component='Source-authoritative shot timestamp publication',
        original_xbe_sha256=comparison['original_xbe_sha256'],source_preparation_entry='108080',
        original_clock_load='1085F9',original_tick_store='108607',weapon_tick_offset='248',
        native_tick_adjustment=0,matched_live_samples=matched,
        previous_different_live_publications=old_mismatches,
        independently_checked_placement_modes=2,
        full_firing_sound_sequence_passed=False,full_weapon_parity=False,
        fully_certified_weapons=0,physical_xbox_verified=False,
        complete_rendered_reference_sequence_verified=False,
        references=[dict(report=n,sha256=sha(BASE/n))for n in references],
        source_files_sha256={n:sha(ROOT/n)for n in source_files},
        evidence_update_tool_sha256=sha(Path(__file__)),
        next_work=next_work,large_artifacts_retained=[])
    progress_name='weapon-shot-publication-original-native-progress-v1.json'
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v55.json'))
    assert matrix['weapon_paths']==len(matrix['weapons'])==63
    assert sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\plasma_rifle\plasma_rifle')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual source preparation tick readback; both live placement modes match original weapon banks.',
        map='assets/disc/maps/lockout.map',matched_live_samples=matched,
        certifies_entire_gate=False,certifies_all_map_variants=False,
        component_covers_all_scoped_maps_for_weapon=False))
    weapon['gates']['hud_audio_publication'].update(status='partial',remaining=
        'Source shot timestamp publication matches64 live samples in one plasma-rifle variant. '
        'Firing sounds, complete HUD, split-screen and LAN independent histories remain open.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v55.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__))
    save('weapon-completion-matrix-v56.json',matrix)
    publisher=str(Path(os.environ['HALO2_TEST_SCRATCH']))
    audit=dict(passed=True,cleanup_required_for_completion=True,scratch_checks=checks,
        completed_validation_private_directories_removed=len(checks),live_owned_builds=[],
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        large_artifacts_retained=[],
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True)
    audit_name='post-weapon-shot-publication-storage-audit-v1.json';save(audit_name,audit)
    checkpoint_name='checkpoint-weapon-shot-publication-20261018.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-material-input-20261018.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v56.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v56.json'),storage_audit=audit_name,
        full_weapon_parity=False,fully_certified_weapons=0,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v55.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix='weapon-completion-matrix-v56.json',
        next_work=next_work,progress=progress['component'],matched_source_shot_tick_samples=matched,
        full_firing_failure=after+'/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        publisher_scratch_removed_after_runner_exit=False,
        codex_usage_instruction=dict(threshold_remaining_percent=1,
            action_requested='Redeem one available full reset',
            redemption_control_available=False,
            fallback='Report threshold when observed; available usage tool is read-only.'))
    save('continuation-working-state-v56.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v55.json` expands','`weapon-completion-matrix-v56.json` expands',1)
    text+='''
## Source-authoritative shot publication, October18

The source equipment readback now copies the literal preparation tick at weapon
offset0x248 into the existing primary/offhand shot publication. OriginalTU1.5
0x108607 stores the game clock tick loaded through0x51DF54; no tick adjustment
is applied. The previous SDL plasma-rifle traces contained four actual source
preparations in each placement mode, with52 sampled publication mismatches.
After the correction, all64 sampled publications match the actual source banks.
Both continued source placement and natural movement after pickup were checked.

The four focused material/heat/charge checks pass. The two complete firing
checks remain failed: four published shots each produce zero observed firing
sounds. This validates timestamp readback only. Device activation, DSP/output,
projectiles, complete rendered sequences and the full weapon scope remain open.
The evidence is in `checkpoint-weapon-shot-publication-20261018.json`; private
build/source trees are removed and only small reports and diagnostics remain.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,matched_live_samples=matched,
        previous_different_publications=old_mismatches,full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
