#!/usr/bin/env python3
"""Narrow owned1.1/1.5 camera and required command-gate correspondence."""
import research_mp_live_combat_updates as comparison
from oracle_smg_firing_1_1 import ROOT
from oracle_mp_combat_postprocess_updates import sha
from oracle_smg_firing_1_1 import Xbe
import json,struct

def main():
    prior=ROOT/'analysis/multiplayer/integration/inventory-source/live-locomotion-owners-u5/complete/report.json'
    assert sha(prior)=='104075c71753a770ba82863a63aa793248b5bad418e397a76eb363c619b82fa8'
    old,new=Xbe(comparison.OLD),Xbe(comparison.NEW_DEFAULT)
    old_writers={0x37dc60:(0x5563a4,0x55d6e4),0x37dc80:(0x5563a8,0x55d6e8),
        0x37dca0:(0x5563ac,0x55d6ec),0x37dcc0:(0x5563b0,0x55d6f0),
        0x37e890:(0x565fe4,0x56d324),0x37e8b0:(0x565fe8,0x56d328)}
    text=next(s for s in new.sections if s.name=='.text');raw=new.section_bytes(text)
    seeds={0xc7e90:(0xc9010,{'kind':'prior_complete_literal_instruction_topology_and_calls',
        'report':str(prior.relative_to(ROOT)),'sha256':sha(prior)})}
    for entry,(p,q) in old_writers.items():
        original=old.read(entry,17)
        assert original[:4]==bytes.fromhex('f30f1005') and original[8:12]==bytes.fromhex('f30f1105') and original[-1:]==b'\xc3'
        assert struct.unpack_from('<I',original,12)[0]==p
        needle=bytes.fromhex('f30f1105')+struct.pack('<I',q)+b'\xc3'
        candidates=[];start=0
        while (at:=raw.find(needle,start))>=0:
            start=at+1
            if at>=8 and raw[at-8:at-4]==bytes.fromhex('f30f1005'):
                source=struct.unpack_from('<I',raw,at-4)[0]
                if old.read(struct.unpack_from('<I',original,4)[0],4)==new.read(source,4):candidates.append(text.va+at-8)
        assert len(candidates)==1,(hex(entry),candidates)
        seeds[entry]=(candidates[0],{'kind':'unique_literal_same_MOVSS_constructor_and_actual_runtime_destination',
            'disc_destination':f'{p:08x}','later_destination':f'{q:08x}',
            'scope':'Destination correspondence is recovered from the paired complete owning camera operands; the original writer and unchanged float input are checked independently.'})
    comparison.main(groups={
        'normal_camera_fov_packet':(0x188b60,0x23e210,0x23de70,0xccc60,0xcc320),
        'observer_fov_lifecycle':(0x170f30,0x170e40,0x170c10,0x170ff0,0x171910,
            0x172630,0x172680,0x171990,0x171c70,0x171e70,0x1720b0,
            0x1727f0,0x172a50,0x173810,0x173fa0,0x173f20,0x173e60,
            0x173ea0,0x173ee0,0x170d00),
        'camera_input_owner':(0x156c00,0x156ad0,0x156eb0,0x187610,0x1884e0,0xc9a40,0x188ad0),
        'normal_unit_control_gate':(0xc7e90,0xc5d10,0xc6c50,0xc7210,0xc7aa0),
        'ordinary_jump_crouch_owners':(0xe5cb0,0xe5da0,0xe6e10,0xdee00,0xe3b20),
        'fov_runtime_static_initializers':tuple(old_writers)},seed_pairs=seeds,
        source_files=(),configuration_tool={'path':str(Path(__file__).relative_to(ROOT)),
            'sha256':sha(__file__)})

from pathlib import Path
if __name__=='__main__':main()
