#!/usr/bin/env python3
"""Compare authored original/MCC D8110 tables and global armor bindings.

This is an offline metadata census, not native delivery or gameplay proof.
Authored row/duplicate order and exact multiplier bits are retained.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from audit_destruction_contracts_tu15 import Reader
from inspect_multiplayer import Cache
from mcc_content import MccCache
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def sha(path):
    with Path(path).open('rb')as source:return hashlib.file_digest(source,'sha256').hexdigest()


def inventory(reader):
    cache=reader.cache;word=reader.word;result=[]
    for tag in cache.tags.values():
        if tag['group']!='matg'or tag['offset']is None:continue
        p=tag['offset'];tables=cache.block(p+0xd0,8)
        groups=[]
        for table in tables:
            rows=[]
            for group in cache.block(table,12):
                rows.append(dict(name=reader.symbol(word(group)),modifiers=[
                    dict(name=reader.symbol(word(modifier)),bits=f'{word(modifier+4):08x}')
                    for modifier in cache.block(group+4,8)]))
            groups.append(rows)
        materials=[dict(name=reader.symbol(word(at)),parent=reader.half(at+8),
            armor=[reader.symbol(word(at+0x10)),reader.symbol(word(at+0x14))])
            for at in cache.block(p+0x150,0xb4)]
        result.append(dict(tag=tag['name'],damage_tables=groups,materials=materials))
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 executable changed')
    census=json.loads(args.contracts.read_text())
    if census['schema']!='H2_DESTRUCTION_CONTRACTS_V1'or not census['inventory_completed']:
        raise ValueError('Require authenticated original/MCC map inventory')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    contracts={};rows=[];differences=[];empty=[]
    shared_path=Path(census['source_files'][0]['mcc_path']).parent/'shared.map'
    if sha(shared_path)!=census['mcc_shared_sha256']:raise ValueError('MCC shared globals owner changed')
    shared=MccCache(shared_path)
    try:shared_globals=inventory(Reader(shared,True))
    finally:shared.close()
    shared_uses=[]
    for number,owner in enumerate(census['source_files']):
        for path_field,hash_field in (('path','sha256'),('mcc_path','mcc_sha256')):
            if not owner[path_field]or sha(owner[path_field])!=owner[hash_field]:raise ValueError('Original/MCC map changed')
        original=Cache(Path(owner['path']));imported=MccCache(Path(owner['mcc_path']))
        try:values=[inventory(Reader(original)),inventory(Reader(imported,True))]
        finally:original.data.close();original.stream.close();imported.close()
        if not values[1]:values[1]=shared_globals;shared_uses.append(Path(owner['path']).stem)
        digests=[]
        for value in values:
            digest=hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
            contracts[digest]=value;digests.append(digest)
        name=Path(owner['path']).stem
        rows.append(dict(map=name,original=digests[0],mcc=digests[1],source=owner))
        if not values[0]or not values[1]:empty.append(name)
        if values[0]!=values[1]:
            details=[]
            for a,b in zip(values[0],values[1]):
                details.append(dict(tag=a['tag'],damage_tables_equal=a['damage_tables']==b['damage_tables'],
                    material_prefix_equal=a['materials']==b['materials'][:len(a['materials'])],
                    original_materials=len(a['materials']),mcc_materials=len(b['materials']),
                    additional_mcc_materials=b['materials'][len(a['materials']):]))
            differences.append(dict(map=name,original=digests[0],mcc=digests[1],details=details))
        if number%10==0:print(json.dumps(dict(maps_completed=len(rows))),flush=True)
    report=dict(schema='H2_DESTRUCTION_MATERIAL_INPUTS_V1',inventory_completed=True,
        recorded_inputs_equal=not differences and not empty,original_tu15_sha256=NEW_PIN,
        source_inventory_sha256=sha(args.contracts),maps=rows,contracts=contracts,differences=differences,
        absent_globals=empty,mcc_shared_globals=dict(path=str(shared_path),sha256=census['mcc_shared_sha256'],used_by=shared_uses),
        full_goal_complete=False,native_delivery_or_gameplay_qualified=False,
        scope='Authored matg D0 table order, group/armor StringID names, exact multiplier bits and global150 material parent/armor bindings only; numeric StringID relocation, other material fields, jpt inputs and native delivery remain separate',
        tool_sha256=sha(__file__),owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[])
    encoded=json.dumps(report,indent=2)+'\n'
    if len(encoded.encode())>1024*1024:raise ValueError('Material census exceeds bounded report budget')
    (args.output/'report.json').write_text(encoded)
    print(json.dumps(dict(maps=len(rows),unique_contracts=len(contracts),differences=len(differences),empty=empty,recorded_inputs_equal=report['recorded_inputs_equal'])),flush=True)
    return 0


if __name__=='__main__':raise SystemExit(main())
