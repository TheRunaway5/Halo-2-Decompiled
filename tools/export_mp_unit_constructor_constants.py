#!/usr/bin/env python3
"""Export independently mapped constructor tail operands from owned updates."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def section(data,address,length):
    u=lambda at:struct.unpack_from('<I',data,at)[0]
    base,count,table=u(0x104),u(0x11c),u(0x120)
    if table<base or count>128:raise ValueError('Invalid executable section table')
    for index in range(count):
        at=table-base+index*56
        start,raw,size=u(at+4),u(at+12),u(at+16)
        if start<=address and address+length<=start+size:
            result=data[raw+address-start:raw+address-start+length]
            if len(result)!=length:raise ValueError('Truncated original operands')
            return result
    raise ValueError('Original operand outside executable section')


def export(authority,images,output):
    expected='e25b3c968c9fe7bf758ddd87cc58e0edf91227e219383025938c537de0bec2b4'
    if sha(authority)!=expected:raise ValueError('Constructor operand authority changed')
    packet=json.loads(authority.read_text());rows=[];inputs=[]
    for index,(record,path) in enumerate(zip(packet['images'],images,strict=True),1):
        if sha(path)!=record['image_sha256']:raise ValueError('Owned executable changed')
        data=path.read_bytes()
        one=section(data,0x465030 if index==1 else 0x46a0d0,4)
        zero=section(data,0x465048 if index==1 else 0x46a0e8,4)
        for key,length in [('normal_blend_bits',4),('special_blend_bits',4),('activity_pairs',32)]:
            raw=section(data,int(record[key]['source_address'],16),length)
            if raw.hex()!=record[key]['raw4_hex' if length==4 else 'raw20_hex']:
                raise ValueError('Original operands differ from independently mapped packet')
        rows.append(f'C {index} {one.hex()} {zero.hex()} {record["normal_blend_bits"]["raw4_hex"]} '
                    f'{record["special_blend_bits"]["raw4_hex"]} {record["activity_pairs"]["raw20_hex"]}')
        inputs.append(dict(path=str(path),sha256=sha(path)))
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'fixtures.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='constructor-tail-operand-provider-v1',authority=dict(path=str(authority),sha256=expected),
                inputs=inputs,tool_sha256=sha(Path(__file__)),fixture_sha256=sha(fixture),
                supported_images=2,exact_bytes=96,live_constructor_state=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(output/'report.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority',type=Path,required=True)
    parser.add_argument('--image-11',type=Path,required=True)
    parser.add_argument('--image-15',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();export(args.authority,[args.image_11,args.image_15],args.output)
