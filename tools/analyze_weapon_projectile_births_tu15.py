#!/usr/bin/env python3
"""Analyze bounded live constructor observations; run through run_test.py."""
import argparse
import hashlib
import json
import math
import os
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from capstone.x86 import X86_OP_IMM
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN,full_tokens


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--validation',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--placement',choices=('both','pinned','native_motion'),default='both')
    args=parser.parse_args();validation=args.validation.absolute()
    authority=ROOT/'analysis/multiplayer/sandbox-source/later-update-functions'
    mapping=json.loads((authority/'report.json').read_text())
    routine=next(row for row in mapping['routines']if row['disc_1_1_entry']=='000b8a30')
    assert routine['later_entry']=='000b9b70'
    assert routine['normalized_instruction_and_internal_branch_topology_equal']
    old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path)==PIN and sha(NEW_DEFAULT)==NEW_PIN
    old,new=Xbe(old_path),Xbe(NEW_DEFAULT)
    disassembler=Cs(CS_ARCH_X86,CS_MODE_32);disassembler.detail=True
    bodies=[];references=[]
    for label,image in [('disc',old),('later',new)]:
        record=routine[label+'_listing'];path=authority/'instructions'/record['file']
        assert sha(path)==record['sha256']
        instructions=[]
        for line in path.read_text().splitlines():
            address,encoded,*_=line.split();address=int(address,16);raw=bytes.fromhex(encoded)
            assert image.read(address,len(raw))==raw
            decoded=list(disassembler.disasm(raw,address));assert len(decoded)==1
            instructions.extend(decoded)
        assert len(instructions)==614
        bodies.append(instructions);references.append(dict(path=str(path),sha256=sha(path)))
    assert full_tokens(bodies[0],old)==full_tokens(bodies[1],new)
    cases=[]
    for label in ('pinned','native_motion'):
        if args.placement!='both' and args.placement!=label:continue
        path=validation/'release/evidence'/f'weapon-source-firing-{label}.json'
        trace=json.loads(path.read_text());assert trace['render_loop_completed']
        assert trace['legacy_fired_rounds']>0 and trace['source_shot_tick_equality_checked']
        births=trace['projectile_births'];created=[]
        for event in births:
            assert event['completed'] and len(event['request_words'])==49
            if event['returned_object']==0xffffffff:continue
            assert event['definition']==event['request_words'][0] and event['type']==5
            created.append(event)
        actual_ticks=sorted({row['weapon_16c_words'][55]for row in trace['samples']if row['weapon_16c_words'][55]})
        case=dict(placement=label,prepared_source_ticks=actual_ticks,
            published_shots=trace['legacy_fired_rounds'],constructor_requests=len(births),
            completed_constructor_observations=len(created),
            completed_projectile_objects=len({row['returned_object']for row in created}),
            returned_objects=sorted({row['returned_object']for row in created}),
            source_birth_ticks=[row['source_tick']for row in births],
            parent_at_constructor_return=[row['parent']for row in created],
            sampled_maximum_published_projectiles=max(row['projectiles']for row in trace['samples']),
            firing_sound_submissions=trace['firing_sound_submissions'],
            source_report=str(path),source_report_sha256=sha(path))
        if 'projectile_retirements' in trace:
            birth_by_object={event['returned_object']:event for event in created}
            retirements=[]
            for event in trace['projectile_retirements']:
                birth=birth_by_object[event['object']]
                assert event['definition']==birth['definition']
                elapsed=event['source_tick']-birth['source_tick'];assert elapsed>=0
                call_site=event['caller_return']-5;raw=new.read(call_site,5)
                decoded=list(disassembler.disasm(raw,call_site))
                direct_target=None
                if len(decoded)==1 and decoded[0].mnemonic=='call' and decoded[0].operands[0].type==X86_OP_IMM:
                    direct_target=f'{decoded[0].operands[0].imm:X}'
                retirements.append(dict(event,elapsed_source_ticks=elapsed,
                    original_call_site=f'{call_site:X}',original_call_bytes=raw.hex(),
                    original_direct_target=direct_target))
            case['retirements']=retirements
            case['published_source_contacts']=trace.get('published_source_contacts',[])
            contact_distances=[]
            scalar=lambda value:struct.unpack('<f',struct.pack('<I',value))[0]
            for event in retirements:
                contacts=[contact for contact in case['published_source_contacts']if contact['projectile']==event['object']]
                event['completed_contact_effects']=len(contacts)
                event['traveled']=scalar(event['traveled_bits'])
                if contacts:
                    position=[scalar(value)for value in birth_by_object[event['object']]['position_words']]
                    distance=math.dist(position,contacts[0]['position'])
                    event['birth_to_contact_distance']=distance;contact_distances.append(distance)
            case['all_created_bolts_contact_and_retire_during_birth_tick']=(
                len(retirements)==case['completed_projectile_objects'] and all(
                    event['elapsed_source_ticks']==0 and event['completed_contact_effects']>0 for event in retirements))
            if contact_distances:case['observed_birth_to_contact_distance_range']=[min(contact_distances),max(contact_distances)]
        cases.append(case)
    all_created=all(case['completed_projectile_objects']>0 for case in cases)
    decision=('Completed actual constructor returns prove that source bolts are created. '
        'Trace their subsequent contact/attachment/retirement before treating empty tick snapshots as failed creation.'
        if all_created else 'At least one trial has no completed matching constructor return. '
        'Check original admission and observation coverage before claiming failed projectile creation.')
    if cases and all(case.get('all_created_bolts_contact_and_retire_during_birth_tick')for case in cases):
        decision=('Every created bolt has completed source contact effects and a retirement request in its birth tick. '
            'The captured nearby surface contacts explain the empty tick snapshots in this scene. '
            'Select an authored scene with a clear firing lane before testing sustained projectile flight; '
            'do not replace source projectile creation or lifecycle.')
    report=dict(passed=True,scope='Read-only live projectile constructor evidence',
        original_xbe_sha256=NEW_PIN,old_image_sha256=PIN,constructor_entry='B9B70',
        original_normalized_body_instructions=614,full_body_correspondence_reverified=True,
        constructor_returns_observed_in_every_trial=all_created,
        cases=cases,decision=decision,references=references,
        observation_sha256=sha(validation/'projectile-birth-observation.json'),
        runner_scratch=os.environ['HALO2_TEST_SCRATCH'],tool_sha256=sha(Path(__file__)),
        complete_projectile_lifecycle_verified=False,full_weapon_parity=False,
        physical_xbox_verified=False,large_artifacts_retained=[])
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,constructor_returns_observed_in_every_trial=all_created,
        cases=cases,decision=decision)),flush=True)


if __name__=='__main__':main()
