#!/usr/bin/env python3
"""Compare original/MCC Pelican collision references, mappings and geometry.

Read-only constructor/collision inputs. No projectile or damage result is
inferred from a missing collision tag or a disabled authored mapping.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path

from audit_destruction_contracts_tu15 import Reader,sha
from inspect_multiplayer import Cache
from mcc_content import MccCache

NAMES=[r'objects\vehicles\pelican\pelican',
       r'objects\vehicles\pelican\pelican_rear_gun\pelican_rear_gun',
       r'objects\vehicles\pelican\pelican_rocket_pod\pelican_rocket_pod']


def inventory(reader):
    cache=reader.cache;word=reader.word;result={}
    for name in NAMES:
        tags=[tag for tag in cache.tags.values()if tag['group']=='vehi'and tag['name']==name and tag['offset']is not None]
        if len(tags)!=1:raise ValueError('Pelican definition absent or ambiguous')
        tag=tags[0];model=cache.tags[word(tag['offset']+0x38)]
        if model['group']!='hlmt'or model['offset']is None:raise ValueError('Pelican model absent')
        at=model['offset'];collision=word(at+0xc)
        item=dict(model=model['name'],collision=reader.ref(collision),physics=reader.ref(word(at+0x24)),
            damage_information_count=word(at+0x60),model_nodes=word(at+0x78),
            model_regions=[],collision_regions=[])
        for region in cache.block(at+0x70,0x10):
            item['model_regions'].append(dict(name=reader.symbol(word(region)),
                collision_region=cache.data[region+4],permutations=[dict(name=reader.symbol(word(p)),
                    mapping_bytes=list(cache.data[p+4:p+8]))for p in cache.block(region+8,8)]))
        if collision!=0xffffffff:
            coll=cache.tags[collision]
            if coll['group']!='coll'or coll['offset']is None:raise ValueError('Collision reference is not resident coll')
            for region in cache.block(coll['offset']+0x1c,0xc):
                row=dict(name=reader.symbol(word(region)),permutations=[])
                for permutation in cache.block(region+4,0x14):
                    value=dict(name=reader.symbol(word(permutation)),bsps=[])
                    for bsp in cache.block(permutation+4,0x44):
                        geometry={}
                        for offset,stride,label in ((0,8,'nodes'),(8,16,'planes'),(0x10,4,'leaves'),
                                (0x18,4,'references'),(0x20,16,'nodes2'),(0x28,8,'surfaces'),(0x30,12,'edges'),(0x38,16,'vertices')):
                            records=cache.block(bsp+4+offset,stride)
                            raw=b''.join(cache.data[p:p+stride]for p in records)
                            geometry[label]=dict(count=len(records),sha256=hashlib.sha256(raw).hexdigest())
                        value['bsps'].append(dict(node=reader.half(bsp),geometry=geometry))
                    row['permutations'].append(value)
                item['collision_regions'].append(row)
        result[name]=item
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inputs',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    inputs=json.loads(args.inputs.read_text());owner=inputs['source_owner']
    if not inputs['passed']or sha(owner['path'])!=owner['sha256']or sha(owner['mcc_path'])!=owner['mcc_sha256']:
        raise ValueError('Qualified original/MCC input context drifted')
    original=Cache(Path(owner['path']));mcc=MccCache(Path(owner['mcc_path']))
    try:a,b=inventory(Reader(original)),inventory(Reader(mcc,True))
    finally:original.data.close();original.stream.close();mcc.close()
    report=dict(passed=a==b,metadata_only=True,original=a,mcc=b,input_reference_sha256=sha(args.inputs),
        source_owner=owner,tool_sha256=sha(__file__),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],
        projectile_or_damage_routing_qualified=False,full_goal_complete=False,retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=a==b,definitions={name:dict(collision=row['collision'],model_regions=row['model_regions'],
        collision_regions=len(row['collision_regions']))for name,row in a.items()})),flush=True)
    return 0 if a==b else 1


if __name__=='__main__':raise SystemExit(main())
