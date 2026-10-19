#!/usr/bin/env python3
"""Publish bounded species/view glow evidence; run through tools/run_test.py."""
import base64
import copy
import hashlib
import json
import os
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def load(name):return json.loads((BASE/name).read_text())
def save(name,value):(BASE/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')


def main():
    folders=['weapon-glow-species-view-validation-v1','weapon-glow-species-view-validation-v2']
    cases=[];references=[];scratch=set();retained=[]
    specifications=[(folders[0],'elite1',3,1),(folders[1],'spartan2',2,2),
        (folders[1],'spartan4',2,4),(folders[1],'elite2',3,2),(folders[1],'elite4',3,4)]
    for folder,label,representation,players in specifications:
        prefix=folder+'/release/evidence/'
        stem=prefix+'weapon-live-glow-'+label
        names=[stem+'-'+kind+'.json'for kind in ('comparison','inputs','original')]
        comparison,inputs,original=[load(name)for name in names]
        native_name=prefix+'weapon-source-firing-'+label+'.json';native=load(native_name)
        assert comparison['passed'] and comparison['original_exit_code']==0
        assert comparison['native_exit_code']!=0 and comparison['native_failure_preserved']
        assert original['passed'] and original['actual_bitmap_ranges_compared']
        assert native['local_players']==players and native['selected_representation']==representation
        assert len(native['actual_unit_definitions'])==players
        assert all(value==native['expected_unit_definition']for value in native['actual_unit_definitions'])
        assert native['legacy_fired_rounds']==4 and native['firing_sound_submissions']==0
        assert native['glow_screen_views']==(1<<players)-1
        raw=base64.b64decode(inputs['data_base64'],validate=True)
        assert raw[:4]==b'LVG2' and len(raw)==inputs['bytes']<=700*1024
        assert hashlib.sha256(raw).hexdigest()==inputs['sha256']==original['capture_sha256']==comparison['capture_sha256']
        at=4;records=0;vertices=0;owners={view:set()for view in range(players)}
        while at<len(raw):
            tick,view,owner,definition,count=struct.unpack_from('<5I',raw,at)
            assert count>0 and count%6==0
            owners[view].add(owner);records+=1;vertices+=count
            at+=20+68+192+16+4+count*40
            materials=struct.unpack_from('<I',raw,at)[0];at+=4+materials*8
            ranges=struct.unpack_from('<I',raw,at)[0];at+=4+ranges*12
        assert at==len(raw) and records==original['actual_gpu_records']==native['glow_draws']
        assert all(values==set(native['flight_owners']) and len(values)==4 for values in owners.values())
        assert all(version['bit_identical'] and not version['failures'] and version['compared_vertices']==vertices
            for version in original['versions'])
        tu15=next(version for version in original['versions']if version['label']=='TU1.5')
        assert tu15['xbe_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
        cases.append(dict(label=label,representation=representation,local_players=players,
            actual_unit_definitions=native['actual_unit_definitions'],actual_gpu_records=records,
            vertices_per_original_version=vertices,views=sorted(owners),source_bolts_per_view=4,
            bit_identical_to_TU15=True,native_firing_exit_code=comparison['native_exit_code'],
            firing_sound_submissions=0,framebuffer_parity_verified=False))
        references+=names+[native_name];scratch.add(comparison['runner_scratch'])
        retained.append(dict(path=names[1],bytes=(BASE/names[1]).stat().st_size,decoded_bytes=len(raw),
            sha256=sha(BASE/names[1]),reason='Exact live per-view input fixture for independently reproducible original-consumer comparison'))
    for folder,expected in [(folders[0],1),(folders[1],4)]:
        parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
        assert parent['status']=='failed' and parent['source_scratch_removed_after_runner_exit']
        assert child['status']=='failed' and child['steps'][-1]['exit_code']==8
        assert all(row['exit_code']==0 for row in child['steps'][:-1])
        log=(BASE/folder/'release/tests.log').read_text()
        assert log.count('Clear-lane observations:')==expected
        assert log.count('Ordinary weapon did not submit one owned firing sound per shot')==expected
        if folder==folders[0]:assert log.count('Local input bindings duplicate the keyboard')==2
        scratch.update([parent['runner_scratch'],child['build']])
        references += [folder+'/report.json',folder+'/release/report.json',folder+'/release/tests.log',folder+'/source-manifest.json']
    replay_name='weapon-glow-views-retained-replay-v1.json';receipt_name='weapon-glow-views-retained-replay-receipt-v1.json'
    replay=load(receipt_name);assert replay['passed']
    assert sha(BASE/replay_name)==sha(BASE/(folders[1]+'/release/evidence/weapon-live-glow-elite4-original.json'))
    scratch.add(replay['runner_scratch'])
    checks=[dict(path=path,removed=not Path(path).exists())for path in sorted(scratch)]
    assert all(row['removed']for row in checks)
    references += [replay_name,receipt_name]
    next_work=('Continue applicable dual-hand and remaining weapon-variant histories, '
        'then full original-render and gameplay reference sequences. Single/two/four-view Spartan/Elite glow '
        'consumer evidence remains bounded; complete producer/lifecycle and framebuffer acceptance remain open. '
        'Complete real audio device/IRQ/DSP/output activation without supplying success, preserve '
        'all63 weapon paths/630 gates, and remove unused generated tests after validation.')
    progress_name='weapon-glow-species-view-progress-v1.json'
    progress=dict(passed=True,component='Actual source species and per-view glow inputs versus original TU1.5 consumers',
        cases=cases,actual_gpu_records=sum(row['actual_gpu_records']for row in cases),
        vertices_per_original_version=sum(row['vertices_per_original_version']for row in cases),
        required_reference_inputs=retained,large_artifacts_retained=[],retained_four_view_input_replay_byte_identical=True,
        preserved_fixture_failures=[folders[0]+'/release/tests.log'],
        corrected_fixture_input_assignment='One keyboard owner; runtime standalone assignments leave absent controllers neutral.',
        corrected_parent_reporting_failure='Cohort labels no longer shadow the release directory label.',
        full_firing_sound_sequence_passed=False,full_weapon_parity=False,fully_certified_weapons=0,
        framebuffer_parity_verified=False,next_work=next_work,
        references=[dict(report=name,sha256=sha(BASE/name))for name in references],evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v59.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\plasma_rifle\plasma_rifle')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual source Spartan/Elite tags and live single/two/four-view glow vertices/colors/UVs/bitmap ranges versus original consumers.',
        certifies_entire_gate=False,certifies_all_map_variants=False,component_covers_all_scoped_maps_for_weapon=False,
        map='assets/disc/maps/lockout.map'))
    weapon['gates']['materials_effects'].update(status='partial',remaining=
        'Single/two/four-view Spartan/Elite plasma-glow inputs match original consumers. Full framebuffer/producer acceptance, dual-hand histories and other variants/effects remain open.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v59.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));save('weapon-completion-matrix-v60.json',matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-glow-species-view-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],
        required_reference_inputs=retained,cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-glow-species-view-20261019.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-live-glow-original-20261019.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v60.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v60.json'),storage_audit=audit_name,
        fully_certified_weapons=0,full_weapon_parity=False,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v59.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix='weapon-completion-matrix-v60.json',
        progress=progress['component'],next_work=next_work,glow_species_view_cases=cases,
        required_glow_input_fixtures=retained,full_firing_failure=folder+'/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v60.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v59.json` expands','`weapon-completion-matrix-v60.json` expands',1)
    text+=f'''
## Source species and split-screen glow comparison, October19

The five new cohorts cover two/four Spartan views and one/two/four Elite views.
Together with the preceding single Spartan view, both species cover all three
local-view layouts. Each verifies actual source biped tags against the selected authored
representation and observes all four source bolts in every gameplay view.
All{progress['actual_gpu_records']} captured glow records and
{progress['vertices_per_original_version']} vertices per original image, including
colors/UVs/bitmap ranges, match unchanged TU1.5 consumers exactly. The retained
four-view fixture independently reproduces the original report after build
cleanup. See `checkpoint-weapon-glow-species-view-20261019.json`.

The first batch preserved two duplicate-keyboard fixture failures and a parent
report-directory label error. The corrected fixture uses the existing standalone
input assignment with one keyboard owner and neutral absent controllers.
The four focused checks pass; all complete firing tests still fail on
zero firing sounds. Framebuffer, complete producer/lifecycle
and remaining weapon/variant acceptance remain open. Only bounded required
input fixtures and small reports remain; private binaries/raw captures/source
copies/build trees are removed.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,cases=len(cases),actual_gpu_records=progress['actual_gpu_records'],
        vertices_per_version=progress['vertices_per_original_version'],full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
