#!/usr/bin/env python3
"""Authenticate the remaining Flood-clump response-reference input difference.

Run with tools/run_test.py and the repository Python containing Capstone.
Untouched original maps are offline authority; no response is executed here.
"""
import argparse
import copy
import json
import os
from pathlib import Path

from audit_destruction_contracts_tu15 import Reader
from inspect_multiplayer import Cache
from mcc_content import MccCache
from refresh_destruction_current_cohort import sha


def inspect(cache,name,mcc):
    word = cache.u32 if mcc else cache.u
    tags = [t for t in cache.tags.values() if t['group']=='hlmt' and t['name']==name and t['offset'] is not None]
    if len(tags)!=1:raise ValueError('Flood-clump model is absent or ambiguous')
    model = tags[0];info = cache.block(model['offset']+0x60,0xf8)
    if len(info)!=1:raise ValueError('Flood-clump damage shape differs')
    sections = cache.block(info[0]+0xbc,0x38)
    if len(sections)!=1:raise ValueError('Flood-clump section shape differs')
    responses = cache.block(sections[0]+0xc,0x50)
    if len(responses)!=1:raise ValueError('Flood-clump response shape differs')
    return dict(section_words=[word(sections[0]+n*4) for n in range(14)],
                response_words=[word(responses[0]+n*4) for n in range(20)]),Reader(cache,mcc).model(model)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text());contracts = {}
    if not census['inventory_completed']:raise ValueError('Require the completed model census')
    for shard in census['contract_shards']:
        path = args.contracts.parent/shard['path']
        if sha(path)!=shard['sha256']:raise ValueError('Flood-clump contract shard changed')
        contracts.update(json.loads(path.read_text()))
    model = r'scenarios\objects\flood\flood_floor_clump_large\flood_floor_clump_large'
    entry = census['definitions']['scen:'+model]
    if len(entry['xbox'])!=1 or len(entry['mcc'])!=1:raise ValueError('Flood-clump contracts are context-dependent')
    original_hash,donor_hash = next(iter(entry['xbox'])),next(iter(entry['mcc']))
    original,donor = contracts[original_hash],contracts[donor_hash]
    normalized = copy.deepcopy(donor)
    response = normalized['damage'][0]['sections'][0]['responses'][0]
    if response['effect']!=['effe',r'effects\objects\characters\flood_infection\body_destroyed']:
        raise ValueError('Flood-clump donor effect identity differs')
    response['effect'] = None
    if normalized!=original:raise ValueError('Flood-clump contract differs beyond the added effect reference')
    names = set(entry['xbox'][original_hash])
    if names!=set(entry['mcc'][donor_hash]):raise ValueError('Flood-clump contexts differ')
    owners = [row for row in census['source_files'] if Path(row['path']).stem in names]
    if {Path(row['path']).stem for row in owners}!=names:raise ValueError('Missing Flood-clump paired map owner')
    rows = []
    for owner in owners:
        if sha(owner['path'])!=owner['sha256'] or sha(owner['mcc_path'])!=owner['mcc_sha256']:
            raise ValueError('Flood-clump original/MCC map changed')
        before,after = Cache(Path(owner['path'])),MccCache(Path(owner['mcc_path']))
        try:
            a,ac = inspect(before,model,False);b,bc = inspect(after,model,True)
            if ac!=original or bc!=donor:raise ValueError('Raw Flood-clump parse differs from its census')
            if a['response_words'][4]!=0xffffffff:raise ValueError('Original Flood-clump effect is not a literal null handle')
            rows.append(dict(map=Path(owner['path']).stem,original=a,donor=b,source_owner=owner))
        finally:
            after.close()
            if hasattr(before,'close'):before.close()
    result = dict(passed=True,schema='H2_FLOOD_CLUMP_AUTHORED_CONTRACT_V1',model=model,
                  scope='Exact authored model/reference inputs only; no native damage or effect acceptance',
                  original_contract=original_hash,donor_contract=donor_hash,canonical_original=original,
                  canonical_donor=donor,paired_maps=rows,contracts_sha256=sha(args.contracts),tool_sha256=sha(__file__),
                  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],full_goal_complete=False,retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=True,paired_maps=len(rows),report=str(args.output))))


if __name__=='__main__':main()
