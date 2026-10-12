#!/usr/bin/env python3
"""Export owned-image immutable looping fields and generic-constructor operand."""
import argparse,json,struct
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha
from export_object_construction_tables import read_image,IMAGES

LOOP=ROOT/'analysis/multiplayer/integration/sound-looping-native/paired-v5/providers.json'
GENERIC=ROOT/'analysis/multiplayer/integration/spawn-source/generic-object-constructor/u5/operand-packet.json'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    assert sha(LOOP)=='8a8011149d6d72b9eda830d484fd41dbb3da33b7064b41436af5b9ccd0c10eb3'
    assert sha(GENERIC)=='4a4c5e5b2a11ae5006350e29f5fa339e275c49a5641f01b52fc7c9aa9de6026e'
    looping=json.loads(LOOP.read_text());generic=json.loads(GENERIC.read_text());rows=[];raw_bytes=0
    for index,(path,fields,operand,source)in enumerate(zip(IMAGES,looping['bindings'],generic['operands'],generic['source_images'],strict=True),1):
        assert sha(path)==source['sha256'];data=path.read_bytes()
        one_at=0x465030 if index==1 else 0x46a0d0
        minimum_at=0x45120c if index==1 else 0x455b0c
        one=read_image(data,one_at,4);minimum=read_image(data,minimum_at,4)
        assert one.hex()==fields['one_bits']and minimum.hex()==fields['minimum_db_bits']
        sentinel=read_image(data,int(operand['address'],16),2)
        assert operand['bytes']==2 and sentinel.hex()==operand['raw_hex']
        method=int(fields['game_allocator_method'],16);assert read_image(data,method,1)
        rows.append(f'I {index} {int.from_bytes(one,"little"):08x} {int.from_bytes(minimum,"little"):08x} {method:08x} {int.from_bytes(sentinel,"little"):04x}')
        pointers=read_image(data,int(fields['name_table'],16),216)
        assert pointers.hex()==fields['name_table216_hex']
        for n,field in enumerate(fields['names']+[fields[k]for k in('pool_name','empty_selector','dialogue_selector')]):
            address=int(field.get('pointer',field.get('address')),16);raw=bytes.fromhex(field['raw_hex'])
            assert raw.endswith(b'\0')and b'\0'not in raw[:-1]
            assert read_image(data,address,len(raw))==raw
            if n<54:assert struct.unpack_from('<I',pointers,n*4)[0]==address
            rows.append(f'S {n} {raw.hex()}');raw_bytes+=len(raw)
        rows.append('E')
    assert raw_bytes==1244
    target=out/'operands.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='looping-construction-immutable-export-v1',exporter_sha256=sha(Path(__file__)),
        input_pins={str(path.relative_to(ROOT))if path.is_relative_to(ROOT)else str(path):sha(path)for path in [LOOP,GENERIC]+IMAGES},
        fixture_sha256=sha(target),original_string_fields=114,original_string_bytes_including_nul=1244,
        extra_sentinel_bytes=4,shared_arithmetic_words_reused=True,expected_from_native=False,
        mutable_runtime_values_imported=False,method_is_opaque_original_identity=True)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(target),report_sha256=sha(out/'report.json'))))

if __name__=='__main__':main()
