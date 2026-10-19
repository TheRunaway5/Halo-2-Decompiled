#!/usr/bin/env python3
"""Audit original authored weapon-function inputs before expanding render wiring.

The object function block uses the same literal32-byte records as
Cache::load_object_functions. Metadata candidates are not rendered parity proof.
Run through tools/run_test.py; retain only bounded records and source hashes.
"""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
from inspect_multiplayer import Cache
from original_xbox_pose_case import BASE, ROOT, sha


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    output=args.output.absolute()
    output.mkdir(exist_ok=False)
    (output/'attempt.json').write_text(json.dumps(dict(runner_scratch=str(scratch)))+'\n')
    scope_path=BASE/'authored-scope.json'
    scope=json.loads(scope_path.read_text())
    contracts={}
    inputs=collections.Counter()
    rows=0
    consumers=[]
    raw={0x09000568,0x04000567,0x03000577,0x1200056a}
    identity=bytes.fromhex('00000000000000000000803f0000000000000000')
    for item in scope['maps']:
        path=Path(item['path'])
        assert sha(path)==item['sha256']
        cache=Cache(path)
        try:
            for weapon in item['weapons']:
                if not weapon['resident']:
                    continue
                tag=cache.tags[int(weapon['tag']['datum'],16)]
                assert tag['group']=='weap' and tag['offset']==weapon['tag']['offset']
                rows+=1
                for index,at in enumerate(cache.block(tag['offset']+0x64,32)):
                    length,address=cache.u(at+20),cache.u(at+24)
                    offset=address-cache.magic
                    assert not length or (0<=offset and offset+length<=len(cache.data))
                    curve=bytes(cache.data[offset:offset+length]) if length else b''
                    assert len(curve)<=8192
                    record=dict(flags=cache.u(at),input=cache.u(at+4),output=cache.u(at+8),
                        enable=cache.u(at+12),threshold_bits=cache.u(at+16),scale=cache.u(at+28),
                        curve_hex=curve.hex(),curve_sha256=hashlib.sha256(curve).hexdigest())
                    key=hashlib.sha256(json.dumps(record,sort_keys=True).encode()).hexdigest()
                    if key not in contracts:
                        ammo=(record['flags']==0 and record['input']==0x1200056a and not record['enable'] and
                            record['threshold_bits']==0 and not record['scale'] and curve==identity)
                        contracts[key]=dict(record,current_direct_weapon_selector=record['output'] in raw,
                            current_literal_ammunition_alias=ammo,
                            non_ammunition_alias_candidate=record['input'] in raw and record['output'] not in raw and not ammo,
                            occurrences=[])
                    contracts[key]['occurrences'].append(dict(map=str(path),weapon=tag['name'],datum=tag['datum'],function=index))
                    inputs[record['input']]+=1
                models=[]
                world=cache.u(tag['offset']+0x38)
                if world!=0xffffffff:
                    owner=cache.tags[world]
                    assert owner['group']=='hlmt'
                    model=cache.u(owner['offset']+4)
                    if model!=0xffffffff:models.append(('world',model))
                models.extend((f'first-person-{n}',cache.u(at+4)) for n,at in
                    enumerate(cache.block(tag['offset']+0x2a8,16)) if cache.u(at+4)!=0xffffffff)
                outputs={cache.u(at+8) for at in cache.block(tag['offset']+0x64,32)}
                for kind,model in models:
                    render=cache.tags[model]
                    assert render['group']=='mode'
                    for material in cache.block(render['offset']+0x60,32):
                        shader=cache.tags.get(cache.u(material+12))
                        if not shader:continue
                        assert shader['group']=='shad'
                        for definition in cache.block(shader['offset']+32,124):
                            for overlay in cache.block(definition+60,20):
                                for role,offset in [('input',0),('range',4)]:
                                    name=cache.u(overlay+offset)
                                    if name in outputs or name in (0x030005a6,0x040005a7,0x0f000574):
                                        consumers.append(dict(map=str(path),weapon=tag['name'],weapon_datum=tag['datum'],
                                            model=render['name'],model_role=kind,shader=shader['name'],
                                            shader_datum=shader['datum'],overlay_offset=overlay,role=role,name=name,
                                            authored_weapon_function_output=name in outputs))
        finally:
            cache.data.close()
            cache.stream.close()
    assert len(scope['maps'])==38 and rows==1110
    candidates=[row for row in contracts.values() if row['non_ammunition_alias_candidate']]
    report=dict(metadata_audit_completed=True,maps=38,authored_weapon_rows=rows,
        distinct_function_contracts=len(contracts),total_function_occurrences=sum(inputs.values()),
        inputs={f'{key:08x}':value for key,value in sorted(inputs.items())},
        non_ammunition_alias_candidate_contracts=len(candidates),
        non_ammunition_alias_candidate_occurrences=sum(len(row['occurrences'])for row in candidates),
        contracts=contracts,model_material_consumers=consumers,
        builtin_one_model_material_references=sum(row['name']==0x030005a6 for row in consumers),
        builtin_one_weapon_paths=sorted({row['weapon'] for row in consumers if row['name']==0x030005a6}),
        primary_trigger_model_material_references=sum(row['name']==0x0f000574 for row in consumers),
        source_scope_sha256=sha(scope_path),
        source_files_sha256={str(path):sha(path) for path in [Path(__file__),ROOT/'src/shader_animation.cpp',
            ROOT/'src/source_weapon_magazine.cpp',ROOT/'src/source_weapon_shader_input.cpp',ROOT/'src/multiplayer_runtime.cpp',
            ROOT/'src/model.cpp',ROOT/'src/material.cpp',ROOT/'src/content.cpp']},
        material_consumption_or_runtime_behavior_verified=False,full_weapon_parity=False,
        runner_scratch=str(scratch),large_artifacts_retained=[])
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ['metadata_audit_completed','maps','authored_weapon_rows',
        'distinct_function_contracts','non_ammunition_alias_candidate_contracts','non_ammunition_alias_candidate_occurrences']}),flush=True)


if __name__=='__main__':
    main()
