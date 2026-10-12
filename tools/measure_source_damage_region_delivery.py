#!/usr/bin/env python3
"""Capture unchanged region delivery through the actual native ownership scope.

Stock Lockout Spartan/Elite metadata remains unmodified. Region, attachment,
physical-component, variant and requested state/flag inputs are declared
diagnostics. No engine callback, candidate, effect or random result is supplied.
This does not establish authored damage-state effects absent from these models.
"""
import argparse
import itertools
import json
from pathlib import Path
import measure_source_damage_feedback_vitality as vitality
from damage_feedback_composition_scope import extend
from inspect_multiplayer import Cache
from oracle_smg_firing_1_1 import ROOT, PIN, CACHE_PIN
from generate_physics_world_bootstrap_runtime_20261011 import sha


def rows():
    result=[]
    for definition in (0,1):
        count=(4,5)[definition]
        for region,bit,mask,variant in itertools.product(
                [-1,*range(count)],(-1,0,2,7),(0,-1,1,0x8000),(0,-1)):
            result.append(dict(entry=0xbb6e0,definition=definition,region=region,
                region_value=bit,region_mask=mask,variant=variant))
        for region,state,allow_lower in itertools.product(
                [-1,*range(count)],(-1,0,1,2,3,4,5),(0,1)):
            result.append(dict(entry=0xbb5e0,definition=definition,region=region,
                region_value=state,region_mask=allow_lower))
        # Literal D8070 depletion/reset and already-depleted paths. A zero
        # maximum isolates region delivery from the separately tested authored
        # shield depletion effect constructor, without supplying its result.
        for flags in (0,8):
            result.append(dict(entry=0xd8070,definition=definition,flags=flags,
                maximum_shield=0.))
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    correspondence=vitality.BASE/'generated/normalized-correspondence.json'
    paired=extend(json.loads(correspondence.read_text()))
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert sha(cache.path)==CACHE_PIN
    image=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    cases=vitality.execute(image,False,paired,cache,rows(),region_delivery=True)
    assert all(case['complete'] for case in cases)
    fixture=dict(schema=1,image='1.1',cases=cases,tool_sha256=sha(Path(__file__)),
        image_sha256=PIN,cache_sha256=CACHE_PIN,correspondence_sha256=sha(correspondence),
        native_composition_scope=paired['native_composition_scope'],
        instruction_patches=[],native_expected_data_used=False)
    output=args.output/'disc.json'
    output.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(tool_sha256=sha(Path(__file__)),capture_helper_sha256=sha(Path(vitality.__file__)),
        fixture_sha256=sha(output),image_sha256=PIN,cache_sha256=CACHE_PIN,
        cases=len(cases),complete=sum(c['complete'] for c in cases),
        instructions=sum(len(c['instructions']) for c in cases),
        writes=sum(len(c['writes']) for c in cases),
        covered_instructions=len({p for c in cases for p in c['instructions']}),
        native_composition_scope=paired['native_composition_scope'],boundary=__doc__,
        instruction_patches=[],native_expected_data_used=False,
        naturally_admitted_damage=False,authored_state_records_added=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases','complete','instructions','writes','covered_instructions')}))


if __name__=='__main__':main()
