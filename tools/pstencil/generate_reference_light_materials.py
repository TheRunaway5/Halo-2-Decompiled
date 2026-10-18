#!/usr/bin/env python3
"""Retain primary-checked original dependent-light V10/V11 and utility choices.

Only unanimous static resident rows are admitted. This is material binding
evidence; no captured original frame is inferred.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
BASE=ROOT/'analysis/pstencil/2003-presentation'


def field(row,name):
    return next(f for f in row['fields'] if f['name']==name)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--include',type=Path,default=ROOT/'src/pstencil/reference_light_materials.inc')
    args=parser.parse_args()
    if args.output.exists():raise RuntimeError('Evidence already exists')
    spec=importlib.util.spec_from_file_location('resident_light_materials',BASE/'extract_resident_environment.py')
    resident=importlib.util.module_from_spec(spec);spec.loader.exec_module(resident)
    if hashlib.sha256(resident.DATA).hexdigest()!='1c40d851a19dd9077b1a5cc8d84319e00150678dbfed5d6713e7e786973cc4b1':
        raise RuntimeError('Original cache differs')
    shaders=json.loads((BASE/'resident-environment.json').read_text())
    bindings=json.loads((BASE/'accumulation-bindings-v1/reference.json').read_text())
    indexed={row['shader']['datum']:row for row in bindings['shaders']}
    checked=0
    def verify(value):
        nonlocal checked
        if isinstance(value,dict):
            if 'raw_hex'in value and 'file_offset'in value:
                offset=int(value['file_offset'],16);raw=bytes.fromhex(value['raw_hex'])
                if resident.DATA[offset:offset+len(raw)]!=raw:raise RuntimeError('Original material field differs')
                checked+=1
            for child in value.values():verify(child)
        elif isinstance(value,list):
            for child in value:verify(child)
    verify(shaders)
    admitted=[];unresolved=[]
    for shader in shaders['shaders']:
        choices=set();register_rows=[]
        for lighting in shader['postprocess_lighting']:
            layers=field(lighting,'runtime layers*')['rows']
            for selected in field(layers[17],'passes*')['rows']:
                for implementation in field(selected,'implementations*')['rows']:
                    constants={}
                    for row in field(implementation,'VS constants*')['rows']:
                        if field(row,'register bank*')['value']!=0:continue
                        index=field(row,'register index*')['value']
                        if index not in (10,11):continue
                        if field(row,'overlays*')['rows']:raise RuntimeError('Original light color needs its animated owner')
                        constants[index]=tuple(field(row,axis+'*')['value'] for axis in 'xyzw')
                        register_rows.append(row['file_offset'])
                    if set(constants)=={10,11}:choices.add((constants[10],constants[11]))
        if not choices:continue
        kinds={row['builtin'] for row in indexed[shader['tag']['datum']]['selections'] if row['layer']==16}
        finals={row['builtin'] for row in indexed[shader['tag']['datum']]['selections'] if row['layer']==17}
        if len(choices)!=1 or len(kinds)!=1 or finals!={7}:
            unresolved.append(dict(shader=shader['tag'],reason='Non-unanimous static pass selection'));continue
        v10,v11=next(iter(choices));admitted.append(dict(shader=shader['tag'],v10=v10,v11=v11,
            utility_builtin=next(iter(kinds)),register_file_offsets=register_rows))
    result=dict(scope=__doc__,cache_sha256=hashlib.sha256(resident.DATA).hexdigest(),checked_fields=checked,
        materials=admitted,unresolved=unresolved,full_presentation_verified=False)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    output='// Generated from primary-checked original resident default_light bindings.\n'
    output+=f'constexpr std::array<ReferenceLightMaterial,{len(admitted)}> reference_light_materials{{{{\n'
    for row in admitted:
        floats=lambda values:'{'+','.join(format(v,'.9g')+('f'if'.'in format(v,'.9g')or'e'in format(v,'.9g')else'.0f')for v in values)+'}'
        output+='    {'+json.dumps(row['shader']['name'])+','+floats(row['v10'])+','+floats(row['v11'])+','+str(row['utility_builtin'])+'u},\n'
    output+='}};\n';args.include.write_text(output)
    print(json.dumps(dict(materials=len(admitted),unresolved=len(unresolved),checked_fields=checked)))


if __name__=='__main__':main()
