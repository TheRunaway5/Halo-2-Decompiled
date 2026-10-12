#!/usr/bin/env python3
"""Export the literal paired driver operand bank from owned executables."""
import argparse
import hashlib
import json
from pathlib import Path
from export_mp_unit_constructor_constants import section


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority,images,output):
    expected='c92d5a3624b21aa7b41d36a658023e4427082aca1541dad1c7f8c684fe9e3d9d'
    if sha(authority)!=expected:raise ValueError('Paired driver operands changed')
    packet=json.loads(authority.read_text())
    if len(packet['constants'])!=16:raise ValueError('Incomplete driver operand bank')
    rows=[];inputs=[]
    for index,path in enumerate(images,1):
        if sha(path)!=packet['image_sha256'][str(path.resolve())]:raise ValueError('Owned executable changed')
        data=path.read_bytes();words=[]
        for record in packet['constants']:
            raw=section(data,int(record['old_va' if index==1 else 'u5_va'],16),4)
            if raw.hex()!=record['raw4_hex']:raise ValueError('Original paired operand differs')
            words.append(raw.hex())
        rows.append(f'C {index} '+ ' '.join(words));inputs.append(dict(path=str(path),sha256=sha(path)))
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'constants.tsv';fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='source-sound-driver-constant-provider-v1',authority=dict(path=str(authority),sha256=expected),
                inputs=inputs,fixture_sha256=sha(fixture),tool_sha256=sha(Path(__file__)),
                xbe_reader_sha256=sha(Path(__file__).with_name('export_mp_unit_constructor_constants.py')),
                supported_images=2,exact_bytes=128,driver_or_voice_state=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(output/'report.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority',type=Path,required=True)
    parser.add_argument('--image-11',type=Path,required=True)
    parser.add_argument('--image-15',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();export(args.authority,[args.image_11,args.image_15],args.output)
