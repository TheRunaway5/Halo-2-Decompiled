#!/usr/bin/env python3
"""Validate live weapon shot publication against the original108607 store.

Run through tools/run_test.py. Compares retained small live-bank traces, not
synthetic shot results. Complete audio/projectile/weapon acceptance stays open.
"""
import argparse
import json
import os
from pathlib import Path
from original_xbox_pose_case import ROOT,sha
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before',type=Path,required=True)
    parser.add_argument('--after',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT)
    original=image.read(0x1085f9,20)
    assert original.hex()=='a154df51008b48088a44241284c0898d48020000'
    cases=[];references=[]
    for label in ('pinned','native_motion'):
        versions=[]
        for name,directory in [('before',args.before),('after',args.after)]:
            path=directory/f'weapon-source-firing-{label}.json'
            trace=json.loads(path.read_text())
            assert trace['render_loop_completed'] and trace['picked_tick']>0
            samples=trace['samples'];assert len(samples)>=30
            differences=[dict(tick=row['tick'],published=row['legacy_shot_tick'],source=row['weapon_16c_words'][55])
                for row in samples if row['legacy_shot_tick']!=row['weapon_16c_words'][55]]
            source_ticks=sorted({row['weapon_16c_words'][55]for row in samples if row['weapon_16c_words'][55]})
            assert source_ticks
            result=dict(version=name,samples=len(samples),different_publications=len(differences),
                examples=differences[:3],source_preparation_ticks=source_ticks,
                published_fired_rounds=trace['legacy_fired_rounds'],
                firing_sound_submissions=trace['firing_sound_submissions'],
                source_shot_tick_equality_checked=trace.get('source_shot_tick_equality_checked',False))
            if name=='before':assert differences and trace['legacy_fired_rounds']==0
            else:
                assert not differences and trace['source_shot_tick_equality_checked']
                assert trace['legacy_fired_rounds']==len(source_ticks)
            versions.append(result);references.append(dict(path=str(path),sha256=sha(path)))
        cases.append(dict(placement=label,versions=versions))
    report=dict(passed=True,component='actual source shot-time readback into existing player/hand publication',
        original_xbe_sha256=NEW_PIN,source_entry='108080',source_store='108607',
        source_game_clock_pointer='51DF54',source_clock_tick_offset=8,weapon_tick_offset='248',
        original_clock_and_store_bytes=original.hex(),native_tick_adjustment=0,
        cases=cases,references=references,
        production_source_sha256=sha(ROOT/'src/multiplayer.cpp'),
        tool_sha256=sha(Path(__file__)),runner_scratch=os.environ['HALO2_TEST_SCRATCH'],
        original_instruction_patches=[],supplied_callee_results=[],
        full_firing_sound_sequence_passed=all(case['versions'][1]['firing_sound_submissions']==
            case['versions'][1]['published_fired_rounds']for case in cases),
        full_weapon_parity=False,large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,placements=len(cases),matched_live_samples=sum(c['versions'][1]['samples']for c in cases),
        full_firing_sound_sequence_passed=report['full_firing_sound_sequence_passed'])),flush=True)


if __name__=='__main__':main()
