#!/usr/bin/env python3
"""Authenticate the exact Hunter model difference from untouched original maps.

Use tools/run_test.py. This only inventories original/MCC authored data. It
does not infer flag meanings, change runtime inputs or prove damage behavior.
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


def literal(cache,model_name,mcc):
    word = cache.u32 if mcc else cache.u
    matches = [tag for tag in cache.tags.values() if tag['group']=='hlmt' and tag['name']==model_name and tag['offset'] is not None]
    if len(matches)!=1:raise ValueError('Hunter model identity is absent or ambiguous')
    model = matches[0]
    infos = cache.block(model['offset']+0x60,0xf8)
    if len(infos)!=1:raise ValueError('Hunter damage-info shape differs')
    sections = cache.block(infos[0]+0xbc,0x38)
    if len(sections)!=2:raise ValueError('Hunter section shape differs')
    result = dict(damage_count=len(infos),section_count=len(sections),sections=[])
    for row in sections:
        responses = cache.block(row+0xc,0x50)
        result['sections'].append(dict(section_words=[word(row+n*4) for n in range(14)],
                                       flags=word(row+4),response_count=word(row+0xc),
                                       response_pointer_is_null=word(row+0x10)==0,
                                       response_words=[[word(at+n*4) for n in range(20)] for at in responses]))
    return result,Reader(cache,mcc).model(model)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    census = json.loads(args.contracts.read_text())
    if census['schema']!='H2_DESTRUCTION_CONTRACTS_V1' or not census['inventory_completed']:
        raise ValueError('Require the completed original/MCC model census')
    model = r'objects\characters\hunter\hunter'
    entry = census['definitions']['bipd:'+model]
    if len(entry['xbox'])!=1 or len(entry['mcc'])!=1:raise ValueError('Hunter has context-dependent contracts')
    original_hash,donor_hash = next(iter(entry['xbox'])),next(iter(entry['mcc']))
    names = set(entry['xbox'][original_hash])
    if names!=set(entry['mcc'][donor_hash]):raise ValueError('Hunter original/MCC contexts differ')
    contracts = {}
    for shard in census['contract_shards']:
        path = args.contracts.parent/shard['path']
        if sha(path)!=shard['sha256']:raise ValueError('Authored contract shard changed')
        contracts.update(json.loads(path.read_text()))
    original,donor = contracts[original_hash],contracts[donor_hash]
    normalized = copy.deepcopy(donor)
    body = normalized['damage'][0]['sections'][0]
    if body['name']!='body' or body['words']['04']!='00000002' or len(body['responses'])!=1:
        raise ValueError('Reviewed Hunter donor body shape differs')
    body['words']['04']='00000000';body['responses']=[]
    if normalized!=original:raise ValueError('Hunter contracts differ beyond the reviewed body flag/added response')
    owners = [row for row in census['source_files'] if Path(row['path']).stem in names]
    if {Path(row['path']).stem for row in owners}!=names:raise ValueError('Hunter lacks a paired map owner')
    rows = []
    for owner in owners:
        if sha(owner['path'])!=owner['sha256'] or sha(owner['mcc_path'])!=owner['mcc_sha256']:
            raise ValueError('A Hunter reference changed since its census')
        before,after = Cache(Path(owner['path'])),MccCache(Path(owner['mcc_path']))
        try:
            raw_before,canonical_before = literal(before,model,False)
            raw_after,canonical_after = literal(after,model,True)
            if canonical_before!=original or canonical_after!=donor:
                raise ValueError('Live reference parsing differs from its authenticated contract')
            if raw_before['sections'][0]['flags']!=0 or raw_before['sections'][0]['response_count']!=0 or not raw_before['sections'][0]['response_pointer_is_null']:
                raise ValueError('Original Hunter body has another empty-response representation')
            if raw_after['sections'][0]['flags']!=2 or raw_after['sections'][0]['response_count']!=1:
                raise ValueError('MCC Hunter has another body response input')
            rows.append(dict(map=Path(owner['path']).stem,original=raw_before,donor=raw_after,source_owner=owner))
        finally:
            after.close()
            if hasattr(before,'close'):before.close()
    result = dict(passed=True,schema='H2_HUNTER_AUTHORED_MODEL_CONTRACT_V1',model=model,
                  scope='Exact raw authored model inputs only; flag/response meaning and native behavior remain unqualified',
                  full_goal_complete=False,original_contract=original_hash,donor_contract=donor_hash,
                  canonical_original=original,canonical_donor=donor,paired_maps=rows,
                  contracts_sha256=sha(args.contracts),tool_sha256=sha(__file__),
                  owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(passed=True,paired_maps=len(rows),report=str(args.output))))


if __name__=='__main__':main()
