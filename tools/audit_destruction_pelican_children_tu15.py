#!/usr/bin/env python3
"""Authenticate Pelican attachment inputs for the remaining no-info gates.

Read-only authoring evidence. An absent damage-information block does not by
itself prove that a projectile reaches this object or that it is invulnerable.
"""
import argparse
import json
import os
from pathlib import Path
from audit_destruction_contracts_tu15 import Reader,sha
from audit_destruction_campaign_inputs_tu15 import placements
from inspect_multiplayer import Cache
from mcc_content import MccCache

PARENT=r'objects\vehicles\pelican\pelican'
WEAPON=r'objects\weapons\support_high\rocket_launcher\rocket_launcher'
CHILDREN={r'objects\vehicles\pelican\pelican_rear_gun\pelican_rear_gun',
          r'objects\vehicles\pelican\pelican_rocket_pod\pelican_rocket_pod'}


def inventory(reader,declared_variant=None):
    cache=reader.cache;word=reader.word
    parents=[t for t in cache.tags.values()if t['group']=='vehi'and t['name']==PARENT and t['offset']is not None]
    if len(parents)!=1:raise ValueError('Pelican parent identity absent or ambiguous')
    parent=parents[0];model=cache.tags[word(parent['offset']+0x38)]
    if model['group']!='hlmt'or model['offset']is None:raise ValueError('Pelican model owner absent')
    variants=cache.block(model['offset']+0x50,0x38)
    default=word(parent['offset']+0x30)
    scenario=cache.tags[word(cache.index+12)]['offset']if reader.mcc else cache.scenario
    rows=cache.block(scenario+0x70,0x54)
    result=[]
    for placement in placements(cache,PARENT,mcc=reader.mcc):
        explicit=word(rows[placement['placement']]+0x34);selected=explicit or default
        if declared_variant is not None:
            selected=next((word(at)for at in variants if reader.symbol(word(at))==declared_variant),None)
            if selected is None:raise ValueError('Declared variant is not authored in this model')
        index=next((i for i,at in enumerate(variants)if selected and word(at)==selected),0)
        if not variants:raise ValueError('Authored Pelican has no model variant')
        at=variants[index];children=[]
        for child in cache.block(at+0x1c,0x10):
            definition=word(child+12)
            if definition==0xffffffff:continue
            tag=cache.tags[definition]
            item=dict(parent_marker=reader.symbol(word(child)),child_marker=reader.symbol(word(child+4)),
                definition=[tag['group'],tag['name']])
            if tag['group']=='vehi'and tag['name']in CHILDREN:
                child_model=cache.tags[word(tag['offset']+0x38)]
                if child_model['group']!='hlmt'or child_model['offset']is None:raise ValueError('Pelican child model owner absent')
                item.update(model=child_model['name'],damage_information_count=word(child_model['offset']+0x60))
            children.append(item)
        result.append(dict(placement=placement,explicit_variant=reader.symbol(explicit),declared_constructor_variant=declared_variant,
            object_default_variant=reader.symbol(default),selected_variant_index=index,
            selected_variant=reader.symbol(word(at)),children=children))
    resident=[t['name']for t in cache.tags.values()if t['group']=='weap'and t['name']==WEAPON and t['offset']is not None]
    return dict(parent=PARENT,model=model['name'],placements=result,resident_rocket_launcher=resident)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',type=Path,required=True)
    parser.add_argument('--map',default='03a_oldmombasa')
    parser.add_argument('--variant',help='Exact authored named variant supplied as a declared constructor input')
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    census=json.loads(args.contracts.read_text())
    if census['schema']!='H2_DESTRUCTION_CONTRACTS_V1'or not census['inventory_completed']:raise ValueError('Unqualified contract inventory')
    owners=[r for r in census['source_files']if Path(r['path']).stem==args.map]
    if len(owners)!=1:raise ValueError('Ambiguous original/MCC map owner')
    owner=owners[0]
    if sha(owner['path'])!=owner['sha256']or sha(owner['mcc_path'])!=owner['mcc_sha256']:raise ValueError('Authored input map changed')
    original=Cache(Path(owner['path']));imported=MccCache(Path(owner['mcc_path']))
    try:a,b=inventory(Reader(original),args.variant),inventory(Reader(imported,True),args.variant)
    finally:original.data.close();original.stream.close();imported.close()
    observed={c['definition'][1]for p in a['placements']for c in p['children']if c['definition'][1]in CHILDREN}
    no_info=all(c.get('damage_information_count')==0 for p in a['placements']for c in p['children']if c['definition'][1]in CHILDREN)
    passed=a==b and bool(a['placements'])and len(a['resident_rocket_launcher'])==1 and observed==CHILDREN and no_info
    report=dict(passed=passed,metadata_only=True,actual_creation_or_damage_qualified=False,full_goal_complete=False,
        original=a,mcc=b,source_owner=owner,contracts_sha256=sha(args.contracts),tool_sha256=sha(__file__),
        owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],
        next_gate='Create the actual selected parent/children, fire ordinary rockets, retain original recipient/full-generation admission and its actual no-info gate; preserve coupled parent/child state and continuation')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=passed,placements=len(a['placements']),children=sorted(observed),metadata_only=True)),flush=True)
    return 0 if passed else 1


if __name__=='__main__':raise SystemExit(main())
