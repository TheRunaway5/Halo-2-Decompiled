#!/usr/bin/env python3
"""Audit authored creation variants and child owners for the vehicle matrix.

Read-only original/MCC constructor inputs. Named references are normalized;
no creation, damage, immunity, physics or presentation outcome is inferred.
"""
import argparse
import json
import os
from pathlib import Path
from audit_destruction_contracts_tu15 import Reader,GROUPS,sha
from audit_destruction_campaign_inputs_tu15 import placements
from inspect_multiplayer import Cache
from mcc_content import MccCache


def scenario_inputs(reader,obj_name,group,default,variants):
    cache=reader.cache;word=reader.word
    scenario=cache.tags[word(cache.index+12)]['offset']if reader.mcc else cache.scenario
    placed=[]
    if group in ('vehi','scen'):
        block,stride=(0x70,0x54)if group=='vehi'else(0x50,0x5c)
        source_rows=cache.block(scenario+block,stride)
        for row in placements(cache,obj_name,mcc=reader.mcc,group=group):
            row.pop('literal_row_words',None)
            explicit=reader.symbol(word(source_rows[row['placement']]+0x34));row['explicit_variant']=explicit
            effective=explicit or default
            row['selected_variant_index']=next((i for i,v in enumerate(variants)if effective and v['name']==effective),0)if variants else None
            placed.append(row)
    return placed


def inventory(reader,targets,with_scenario=True):
    cache=reader.cache;word=reader.word;result={}
    for obj in cache.tags.values():
        if obj['group']not in GROUPS or obj['offset']is None:continue
        model=cache.tags.get(word(obj['offset']+0x38))
        if not model or model['group']!='hlmt'or model['offset']is None:continue
        variants=[];related=obj['group']+':'+obj['name']in targets
        for index,at in enumerate(cache.block(model['offset']+0x50,0x38)):
            children=[]
            for child in cache.block(at+0x1c,0x10):
                reference=word(child+12)
                if reference==0xffffffff:continue
                tag=cache.tags[reference];related|=tag['group']+':'+tag['name']in targets
                row=dict(parent_marker=reader.symbol(word(child)),child_marker=reader.symbol(word(child+4)),
                    reference_group_word=f'{word(child+8):08x}',definition=[tag['group'],tag['name']])
                if tag['group']=='vehi':
                    child_model=cache.tags.get(word(tag['offset']+0x38))if tag['offset']is not None else None
                    if child_model and child_model['group']=='hlmt'and child_model['offset']is not None:
                        row.update(model=child_model['name'],damage_information_count=word(child_model['offset']+0x60))
                    else:row['child_model_metadata_not_local']=True
                children.append(row)
            variants.append(dict(index=index,name=reader.symbol(word(at)),children=children))
        if not related:continue
        default=reader.symbol(word(obj['offset']+0x30))
        selected=next((i for i,v in enumerate(variants)if default and v['name']==default),0)if variants else None
        placed=scenario_inputs(reader,obj['name'],obj['group'],default,variants)if with_scenario else[]
        result[obj['group']+':'+obj['name']]=dict(model=model['name'],object_default_variant=default,
            default_variant_index=selected,variants=variants,scenario_placements=placed)
    return result


def compare_inputs(a,b):
    # Keep actual original/MCC storage ordinals explicit. Only uniquely paired
    # rows with every other constructor field equal can identify an ordinal
    # difference; this does not qualify runtime numeric-index translation.
    paired=[];left=json.loads(json.dumps(a));right=json.loads(json.dumps(b))
    if set(left)!=set(right):return False,paired
    for parent in left:
        x=left[parent]['scenario_placements'];y=right[parent]['scenario_placements']
        if len(x)!=len(y):return False,paired
        old_rows=[{k:v for k,v in row.items()if k!='placement'}for row in x]
        new_rows=[{k:v for k,v in row.items()if k!='placement'}for row in y]
        keys=[json.dumps(row,sort_keys=True)for row in old_rows]
        if old_rows!=new_rows or len(keys)!=len(set(keys)):return False,paired
        for old,new in zip(x,y):
            if old['placement']!=new['placement']:
                paired.append(dict(parent=parent,original_row=old['placement'],mcc_row=new['placement'],
                    unique_id=old['unique_id'],other_constructor_fields_equal=True,runtime_numeric_index_remap_qualified=False))
            old.pop('placement');new.pop('placement')
    return left==right,paired


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',type=Path,required=True)
    parser.add_argument('--matrix',type=Path,required=True)
    parser.add_argument('--map',dest='maps',action='append',required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    census=json.loads(args.contracts.read_text());matrix=json.loads(args.matrix.read_text())
    if census['schema']!='H2_DESTRUCTION_CONTRACTS_V1'or not census['inventory_completed']or len(matrix['rows'])!=37:
        raise ValueError('Require completed original/MCC contracts and the complete vehicle matrix')
    targets={row['definition']for row in matrix['rows']}
    results=[];differences=[];contexts={name:[]for name in sorted(targets)};semantic_differences=[]
    first=next(r for r in census['source_files']if Path(r['path']).stem==args.maps[0])
    shared_path=Path(first['mcc_path']).parent/'shared.map'
    if sha(shared_path)!=census['mcc_shared_sha256']:raise ValueError('MCC shared constructor metadata drifted')
    shared=MccCache(shared_path)
    try:shared_inputs=inventory(Reader(shared,True),targets,with_scenario=False)
    finally:shared.close()
    for map_name in args.maps:
        owners=[r for r in census['source_files']if Path(r['path']).stem==map_name]
        if len(owners)!=1:raise ValueError('Map constructor reference absent or ambiguous')
        owner=owners[0]
        if sha(owner['path'])!=owner['sha256']or sha(owner['mcc_path'])!=owner['mcc_sha256']:raise ValueError('Constructor reference map drifted')
        original=Cache(Path(owner['path']));imported=MccCache(Path(owner['mcc_path']))
        try:
            a,b=inventory(Reader(original),targets),inventory(Reader(imported,True),targets)
            shared_resolved=[];main_reader=Reader(imported,True)
            for key in sorted(set(a)-set(b)):
                if key not in shared_inputs:continue
                group,name=key.split(':',1)
                local=[t for t in imported.tags.values()if t['group']==group and t['name']==name]
                if len(local)!=1:raise ValueError('Main map has no unique identity for its shared constructor')
                row=json.loads(json.dumps(shared_inputs[key]))
                if local[0]['offset']is not None:
                    model=imported.tags.get(main_reader.word(local[0]['offset']+0x38))
                    if model and model['offset']is not None:raise ValueError('Local constructor metadata was unexpectedly bypassed')
                    row['object_default_variant']=main_reader.symbol(main_reader.word(local[0]['offset']+0x30))
                    row['default_variant_index']=next((i for i,v in enumerate(row['variants'])if row['object_default_variant']and v['name']==row['object_default_variant']),0)if row['variants']else None
                row['scenario_placements']=scenario_inputs(main_reader,name,group,row['object_default_variant'],row['variants'])
                b[key]=row;shared_resolved.append(key)
        finally:original.data.close();original.stream.close();imported.close()
        logical_equal,index_pairs=compare_inputs(a,b)
        for parent in sorted(set(a)|set(b)):
            if a.get(parent)!=b.get(parent):differences.append(dict(map=map_name,parent=parent,original=a.get(parent),mcc=b.get(parent)))
        if not logical_equal:semantic_differences.append(map_name)
        for parent,body in a.items():
            if parent in contexts:contexts[parent].append(dict(map=map_name,owner=parent,relationship='definition',
                scenario_placements=len(body['scenario_placements']),default_variant=body['object_default_variant']))
            for variant in body['variants']:
                for child in variant['children']:
                    name=':'.join(child['definition'])
                    if name in contexts:contexts[name].append(dict(map=map_name,owner=parent,relationship='model_child',
                        variant=variant['name'],parent_marker=child['parent_marker'],child_marker=child['child_marker']))
        results.append(dict(map=map_name,passed=logical_equal,literal_storage_rows_equal=a==b,source_owner=owner,original=a,mcc=b,
            paired_storage_ordinal_differences=index_pairs,shared_metadata_resolved_definitions=shared_resolved,runtime_numeric_index_remap_qualified=False))
        print(json.dumps(dict(map=map_name,logical_constructor_inputs_equal=logical_equal,storage_ordinal_pairs=len(index_pairs),related_parent_definitions=len(a))),flush=True)
    report=dict(passed=not semantic_differences,metadata_only=True,full_goal_complete=False,
        scope=__doc__,contracts_sha256=sha(args.contracts),matrix_sha256=sha(args.matrix),tool_sha256=sha(__file__),
        mcc_shared_metadata=dict(path=str(shared_path),sha256=census['mcc_shared_sha256']),
        owned_scratch=os.environ['HALO2_TEST_SCRATCH'],maps=results,literal_storage_differences=differences,
        semantic_differences=semantic_differences,vehicle_creation_contexts=contexts,runtime_numeric_index_remap_qualified=False,
        definitions_with_context=sum(bool(rows)for rows in contexts.values()),total_vehicle_definitions=len(targets),retained_large_artifacts=[])
    data=json.dumps(report,indent=2)+'\n'
    if len(data.encode())>1024*1024:raise ValueError('Constructor report exceeds bounded evidence budget')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(data)
    print(json.dumps(dict(passed=report['passed'],definitions_with_context=report['definitions_with_context'],total_vehicle_definitions=len(targets))),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
