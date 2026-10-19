#!/usr/bin/env python3
"""Publish exact live glow/original comparisons; run through tools/run_test.py."""
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
    folder='weapon-live-glow-original-validation-v1';evidence=folder+'/release/evidence/'
    comparison=load(evidence+'weapon-live-glow-comparison.json')
    original_name=evidence+'weapon-live-glow-original.json';original=load(original_name)
    inputs_name=evidence+'weapon-live-glow-inputs.json';inputs=load(inputs_name)
    native=load(evidence+'weapon-source-firing-pinned.json')
    assert comparison['passed'] and comparison['original_exit_code']==0
    assert comparison['native_exit_code']!=0 and comparison['native_failure_preserved']
    assert original['passed'] and original['actual_bitmap_ranges_compared']
    assert not original['framebuffer_parity_verified']
    assert comparison['original_report_sha256']==sha(BASE/original_name)
    assert comparison['original_tool_sha256']==original['tool_sha256']==sha(ROOT/'tools/oracle_live_projectile_glow_tu15.py')
    raw=base64.b64decode(inputs['data_base64'],validate=True)
    assert raw[:4]==b'LVG2' and len(raw)==inputs['bytes']<=256*1024
    assert hashlib.sha256(raw).hexdigest()==inputs['sha256']==original['capture_sha256']==comparison['capture_sha256']
    at=4;records=0;vertices=0;owners=set();views=set()
    while at<len(raw):
        tick,view,owner,definition,count=struct.unpack_from('<5I',raw,at)
        assert count>0 and count%6==0
        records+=1;vertices+=count;owners.add(owner);views.add(view)
        at+=20+68+192+16+4+count*40
        material_count=struct.unpack_from('<I',raw,at)[0];at+=4+material_count*8
        range_count=struct.unpack_from('<I',raw,at)[0];at+=4+range_count*12
    assert at==len(raw) and records==original['actual_gpu_records']==native['glow_draws']
    assert owners==set(native['flight_owners'])==set(native['glow_owners']) and len(owners)==4 and views=={0}
    assert all(row['bit_identical'] and not row['failures'] and row['compared_vertices']==vertices for row in original['versions'])
    tu15=next(row for row in original['versions']if row['label']=='TU1.5')
    assert tu15['xbe_sha256']=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    replay_name='weapon-live-glow-retained-replay-v1.json';replay_receipt_name='weapon-live-glow-retained-replay-receipt-v1.json'
    replay=load(replay_receipt_name);assert replay['passed']
    assert replay['decoded_sha256']==inputs['sha256'] and replay['inputs_sha256']==sha(BASE/inputs_name)
    assert replay['original_report_sha256']==sha(BASE/replay_name)==sha(BASE/original_name)
    parent=load(folder+'/report.json');child=load(folder+'/release/report.json')
    assert parent['source_scratch_removed_after_runner_exit'] and parent['status']=='failed'
    assert child['status']=='failed' and child['steps'][-1]['exit_code']==8
    assert all(row['exit_code']==0 for row in child['steps'][:-1])
    log=(BASE/folder/'release/tests.log').read_text()
    assert 'Clear-lane observations:'in log and 'Ordinary weapon did not submit one owned firing sound per shot'in log
    scratch={parent['runner_scratch'],child['build'],comparison['runner_scratch'],replay['runner_scratch']}
    checks=[dict(path=p,removed=not Path(p).exists())for p in sorted(scratch)];assert all(row['removed']for row in checks)
    refs=[original_name,inputs_name,evidence+'weapon-live-glow-comparison.json',evidence+'weapon-source-firing-pinned.json',
        replay_name,replay_receipt_name,folder+'/report.json',folder+'/release/report.json',folder+'/release/tests.log',folder+'/source-manifest.json']
    reference=dict(path=inputs_name,bytes=(BASE/inputs_name).stat().st_size,decoded_bytes=len(raw),
        sha256=sha(BASE/inputs_name),reason='Required exact live marker/camera/viewport/vertex inputs for reproducible original-consumer comparison')
    next_work=('Extend the same actual-source/original-glow comparison to applicable split-screen and species views, '
        'then continue the remaining weapon variants, full original-render and gameplay reference sequences. '
        'The single-view plasma-rifle glow consumer inputs match TU1.5 exactly; original framebuffer and '
        'complete producer/lifecycle acceptance remain open. Complete real audio device/IRQ/DSP/output activation '
        'without supplying success, preserve all63 weapons/630 gates, and delete unused generated tests after validation.')
    progress_name='weapon-live-glow-original-progress-v1.json'
    progress=dict(passed=True,component='Actual live plasma-rifle glow inputs versus unchanged original TU1.5 consumers',
        actual_gpu_records=records,vertices_per_original_version=vertices,live_object_owners=len(owners),views=sorted(views),
        original_versions=[dict(label=row['label'],xbe_sha256=row['xbe_sha256'],compared_vertices=row['compared_vertices'],
            bit_identical=row['bit_identical'],complete_owners=row['owners'],unavailable_hardware_boundaries=row['unavailable_hardware_boundaries'])for row in original['versions']],
        retained_input_replay_byte_identical=True,native_firing_exit_code=comparison['native_exit_code'],ctest_exit_code=8,
        firing_sound_submissions=native['firing_sound_submissions'],full_firing_sound_sequence_passed=False,
        original_framebuffer_verified=False,original_flight_verified=False,full_weapon_parity=False,fully_certified_weapons=0,
        required_reference_inputs=[reference],large_artifacts_retained=[],next_work=next_work,
        references=[dict(report=n,sha256=sha(BASE/n))for n in refs],evidence_update_tool_sha256=sha(Path(__file__)))
    save(progress_name,progress)
    matrix=copy.deepcopy(load('weapon-completion-matrix-v58.json'))
    assert len(matrix['weapons'])==63 and sum(len(w['gates'])for w in matrix['weapons'])==630
    weapon=next(w for w in matrix['weapons']if w['name']==r'objects\weapons\rifle\plasma_rifle\plasma_rifle')
    weapon['bounded_evidence'].append(dict(report=progress_name,sha256=sha(BASE/progress_name),
        component='Actual live glow XYZW/RGBA/UV vertices and bitmap ranges match unchanged TU1.5 sprite/projector consumers.',
        map='assets/disc/maps/lockout.map',certifies_entire_gate=False,certifies_all_map_variants=False,
        component_covers_all_scoped_maps_for_weapon=False))
    weapon['gates']['materials_effects'].update(status='partial',remaining=
        'Single-view live glow vertices/colors/UVs/bitmap ranges match original TU1.5 consumers exactly. Complete producer/lifecycle, framebuffer, other views/variants and materials/effects remain open.')
    matrix['previous_matrix_sha256']=sha(BASE/'weapon-completion-matrix-v58.json')
    matrix['evidence_update_tool_sha256']=sha(Path(__file__));save('weapon-completion-matrix-v59.json',matrix)
    publisher=os.environ['HALO2_TEST_SCRATCH'];audit_name='post-weapon-live-glow-original-storage-audit-v1.json'
    save(audit_name,dict(passed=True,scratch_checks=checks,publisher_scratch=publisher,
        publisher_cleanup_pending_runner_exit=True,live_owned_builds=[],large_artifacts_retained=[],
        required_reference_inputs=[reference],cleanup_required_for_completion=True,
        original_assets_reference_evidence_frozen_sources_selected_runtimes_and_unrelated_work_preserved=True))
    checkpoint_name='checkpoint-weapon-live-glow-original-20261019.json'
    save(checkpoint_name,dict(passed=True,previous_checkpoint='checkpoint-weapon-clear-flight-20261018.json',
        evidence=progress_name,evidence_sha256=sha(BASE/progress_name),matrix='weapon-completion-matrix-v59.json',
        matrix_sha256=sha(BASE/'weapon-completion-matrix-v59.json'),storage_audit=audit_name,
        full_weapon_parity=False,fully_certified_weapons=0,cleanup_required_for_completion=True,next_work=next_work))
    state=load('continuation-working-state-v58.json')
    state.update(latest_checkpoint=checkpoint_name,latest_matrix='weapon-completion-matrix-v59.json',
        progress=progress['component'],next_work=next_work,original_glow_records=records,
        original_glow_vertices_per_version=vertices,original_glow_input_comparison_for_single_view_passed=True,
        required_glow_input_fixture=reference,full_firing_failure=folder+'/release/tests.log',
        publisher_scratch=publisher,publisher_cleanup_pending_runner_exit=True,
        publisher_scratch_removed_after_runner_exit=False)
    save('continuation-working-state-v59.json',state)
    doc=ROOT/'docs/WEAPONS_TU15_PARITY_GOAL.md';text=doc.read_text()
    text=text.replace('`weapon-completion-matrix-v58.json` expands','`weapon-completion-matrix-v59.json` expands',1)
    text+=f'''
## Original comparison of actual plasma-bolt glow inputs, October19

All{records} actual single-view gameplay glow records, over four complete source
object generations, match the unchanged TU1.5 sprite/projector consumers:
{vertices} XYZW/RGBA/UV vertices and every bitmap range are bit-identical. Retail
and TU1.1 consumers independently agree under the declared same-input ABI.
The bounded retained input JSON is{reference['bytes']} bytes and reproduces the
original comparison report byte-for-byte after the private build is removed.
See `checkpoint-weapon-live-glow-original-20261019.json`.

The full firing test still fails on zero firing sounds and retains its original
nonzero verdict. Hardware entry/submission boundaries are explicit in the
original report. Framebuffer, complete producer/lifecycle, other views/species/
variants and whole-weapon acceptance remain open. Raw captures, binaries and
private source/build trees are removed; the small required input fixture and
comparison/failure reports are retained.
'''
    doc.write_text(text)
    print(json.dumps(dict(passed=True,actual_gpu_records=records,vertices_per_version=vertices,
        required_fixture_bytes=reference['bytes'],full_weapon_parity=False)),flush=True)


if __name__=='__main__':main()
