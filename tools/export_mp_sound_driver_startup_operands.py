#!/usr/bin/env python3
"""Export original immutable driver basis operands without mutable plugin state."""
import argparse
import hashlib
import json
from pathlib import Path
from export_mp_unit_constructor_constants import section


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def export(authority, images, output):
    expected='ca5fb91813a9691ef4cf1907082cb3a1b6324c05364f2124ebe64ba6795851b3'
    if sha(authority)!=expected:
        raise ValueError('Complete paired driver operand authority changed')
    packet=json.loads(authority.read_text())
    if packet['runtime_environment_packet']['copied_into_constants'] or not packet['source_selected_plugin_identity']['native_retained_mutable_plugin_owner_required']:
        raise ValueError('Mutable source environment or plugin cannot be imported as constants')
    rows,inputs=[],[]
    for index,path in enumerate(images):
        if sha(path)!=packet['image_sha256'][str(path.resolve())]:
            raise ValueError('Owned original executable changed')
        image=path.read_bytes()
        key='old' if index==0 else 'u5'
        one=packet['constants']['one_bits']
        raw_one=section(image,int(one[key+'_va'],16),4)
        if raw_one.hex()!=one['raw4_hex']:
            raise ValueError('Literal original driver one operand differs')
        words=[raw_one.hex()]
        for name in ['basis_forward','basis_up']:
            operand=packet['constants'][name]
            pointer=section(image,int(operand[key+'_pointer_va'],16),4)
            address=int.from_bytes(pointer,'little')
            if address!=int(operand[key+'_value_va'],16):
                raise ValueError('Actual original basis forwarding pointer differs')
            raw=section(image,address,12)
            if raw.hex()!=operand['raw12_hex']:
                raise ValueError('Complete original basis operand differs')
            words.append(raw.hex())
        rows.append(f'C {index+1} '+' '.join(words))
        inputs.append(dict(path=str(path),sha256=sha(path)))
    output.mkdir(parents=True,exist_ok=True)
    fixture=output/'operands.tsv'
    fixture.write_text('\n'.join(rows)+'\n')
    report=dict(schema='source-sound-driver-startup-immutable-operands-v1',
        authority=dict(path=str(authority),sha256=expected),inputs=inputs,
        fixture_sha256=sha(fixture),tool_sha256=sha(Path(__file__)),
        xbe_reader_sha256=sha(Path(__file__).with_name('export_mp_unit_constructor_constants.py')),
        supported_images=2,exact_immutable_operand_bytes=56,verified_forwarding_pointer_bytes=16,
        mutable_plugin_identity_copied=False,mutable_environment_packet_copied=False,
        source_saved_settings_device_caps_or_initialization=False)
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
