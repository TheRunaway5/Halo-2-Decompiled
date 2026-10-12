#!/usr/bin/env python3
"""Export only the two verified immutable map activation operands."""
import argparse
import hashlib
import json
from pathlib import Path
from export_mp_unit_constructor_constants import section


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority, images, output):
    expected='b2dfcb85ba6381ed83465e7ec1f2e7f9cf5f479a7997866443c9d56cc4e5a836'
    if sha(authority)!=expected:
        raise ValueError('Paired map activation operand packet changed')
    packet=json.loads(authority.read_text())
    if packet['runtime_environment_packet']['copied_into_constants']:
        raise ValueError('Mutable environment packet cannot be a constant provider')
    rows,inputs=[],[]
    for index,path in enumerate(images):
        if sha(path)!=packet['image_sha256'][str(path.resolve())]:
            raise ValueError('Owned original executable changed')
        image=path.read_bytes()
        key='old_va' if index==0 else 'u5_va'
        words=[]
        for name in ['one_bits','ambient_transition_bits']:
            entry=packet['constants'][name]
            raw=section(image,int(entry[key],16),4)
            if raw.hex()!=entry['raw4_hex']:
                raise ValueError('Actual complete-owner immutable operand differs')
            words.append(raw.hex())
        rows.append(f'C {index+1} '+' '.join(words))
        inputs.append(dict(path=str(path),sha256=sha(path)))
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'constants.tsv'
    fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='source-sound-map-activation-operand-provider-v1',
                authority=dict(path=str(authority),sha256=expected),inputs=inputs,
                fixture_sha256=sha(fixture),tool_sha256=sha(Path(__file__)),
                xbe_reader_sha256=sha(Path(__file__).with_name('export_mp_unit_constructor_constants.py')),
                supported_images=2,exact_operand_bytes=16,
                mutable_environment_or_saved_settings_copied=False,source_map_activation=False)
    (output/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(fixture),report_sha256=sha(output/'report.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authority',type=Path,required=True)
    parser.add_argument('--image-11',type=Path,required=True)
    parser.add_argument('--image-15',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    export(args.authority,[args.image_11,args.image_15],args.output)
