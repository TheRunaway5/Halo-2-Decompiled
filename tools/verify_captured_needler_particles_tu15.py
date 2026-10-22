#!/usr/bin/env python3
"""Verify the full occupied Needler system/location/emitter/particle hierarchy.

Offsets and full-identity chain contracts follow source_particle_render_state_tu15.
This is actual native source-state coverage, not geometry or framebuffer parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from inspect_multiplayer import Cache
from validate_captured_weapon_feedback_tu15 import validate


def sha(path):
    with path.open('rb') as file:
        return hashlib.file_digest(file,'sha256').hexdigest()


def regions(path):
    rows=[]
    with path.open('rb') as file:
        while header:=file.read(8):
            assert len(header)==8
            address,size=struct.unpack('<II',header)
            raw=file.read(size)
            assert len(raw)==size
            rows.append((address,raw))
    return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--actual',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();assert not args.output.exists()
    metadata=json.loads((args.actual/'native.json').read_text())
    context=validate(args.actual,metadata,regions(args.actual/'before.bin'))
    assert metadata['entry']==0x179df0 and metadata['sequence_tick']==80 and metadata['completed']
    rows=regions(args.actual/'after.bin')
    def word(address,size=4):
        for base,raw in rows:
            if base<=address and address+size<=base+len(raw):
                return int.from_bytes(raw[address-base:address-base+size],'little')
        raise ValueError(hex(address))
    def occupied(cell,stride):
        pool=word(cell);assert word(pool+0x24)==stride
        capacity,high=word(pool+0x20),word(pool+0x38)
        assert high<=capacity<=65536
        first,bits=word(pool+0x44),word(pool+0x48)
        found={}
        for index in range(high):
            if not(word(bits+(index//32)*4)&(1<<(index%32))):continue
            pointer=first+index*stride;salt=word(pointer,2)
            assert salt
            found[(salt<<16)|index]=pointer
        return found
    cache=Cache(Path(context['map']))
    try:
        firing=next(datum for datum,tag in cache.tags.items() if tag['group']=='effe' and
            tag['name']=='effects\\objects\\weapons\\pistol\\needler\\firing')
    finally:cache.data.close();cache.stream.close()
    systems=occupied(0x51df74,84);locations=occupied(0x52d71c,52)
    emitters=occupied(0x52d718,76);particles=occupied(0x52d714,64)
    used_locations,used_emitters,used_particles=set(),set(),set();chains=[]
    for system_id,system in systems.items():
        assert word(system+0x10)==firing
        location_id=word(system+0x30)
        while location_id!=0xffffffff:
            assert location_id in locations and location_id not in used_locations
            used_locations.add(location_id);location=locations[location_id]
            emitter_id=word(location+4)
            while emitter_id!=0xffffffff:
                assert emitter_id in emitters and emitter_id not in used_emitters
                used_emitters.add(emitter_id);emitter=emitters[emitter_id]
                particle_id=word(emitter+4);owned=[]
                while particle_id!=0xffffffff:
                    assert particle_id in particles and particle_id not in used_particles
                    used_particles.add(particle_id);owned.append(particle_id)
                    particle_id=word(particles[particle_id]+4)
                assert len(owned)==word(emitter+2,2)
                chains.append(dict(system=system_id,location=location_id,emitter=emitter_id,particles=owned))
                emitter_id=word(emitter+8)
            location_id=word(location+0xc)
    assert used_locations==set(locations) and used_emitters==set(emitters)
    assert used_particles==set(particles) and particles
    report=dict(passed=True,scope=__doc__,tick=80,needler_firing_effect=firing,
        systems=len(systems),locations=len(locations),emitters=len(emitters),particles=len(particles),
        all_full_identities_occupied_and_unique=True,all_emitter_counts_exact=True,
        every_live_particle_owned_by_actual_needler_firing=True,chains=chains,
        actual_before_sha256=sha(args.actual/'before.bin'),actual_after_sha256=sha(args.actual/'after.bin'),
        actual_build_sha256=sha(args.actual/'build.json'),tool_sha256=sha(Path(__file__)),
        source_layout_sha256=sha(Path(__file__).resolve().parents[1]/'src/rendering/effects/source_particle_render_state_tu15.cpp'),
        rendered_sequence_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','systems','locations','emitters','particles')}))


if __name__=='__main__':main()
