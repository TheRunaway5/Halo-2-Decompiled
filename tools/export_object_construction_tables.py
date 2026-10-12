#!/usr/bin/env python3
"""Export authenticated immutable provider bytes and actual group-query ABIs."""
import argparse,json,struct
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha

PACKET=ROOT/'analysis/multiplayer/integration/spawn-source/object-type-construction/u5/metadata-packet.json'
QUERIES=ROOT/'analysis/multiplayer/integration/spawn-source/object-type-construction/measured-v2/report.json'
ADMISSION=ROOT/'analysis/multiplayer/integration/sound-admission-native/paired-v2/providers.json'
CREATE=ROOT/'analysis/multiplayer/integration/sound-create-constants-provider-20261009/measured/constants.tsv'
IMAGES=[ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
    Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')]

def read_image(data,address,length):
    base=struct.unpack_from('<I',data,0x104)[0]
    count,table=struct.unpack_from('<II',data,0x11c)
    for i in range(count):
        start,_,raw,size=struct.unpack_from('<IIII',data,table-base+i*56+4)
        if address>=start and address-start+length<=size:
            result=data[raw+address-start:raw+address-start+length]
            assert len(result)==length
            return result
    raise ValueError('Immutable field outside resident image')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    assert sha(PACKET)=='11faa918528d554f4c62e0b6da16808dbfa92cc1e568ea5a95d27ce2188bf563'
    assert sha(QUERIES)=='6e861dc8f5892b513f6652dbe30a74dda3092a4596c45bc8df1e91d39ef1088c'
    packet=json.loads(PACKET.read_text());queries=json.loads(QUERIES.read_text())
    providers=json.loads(ADMISSION.read_text());words=[q for q in providers['words']if q['old_va']=='004651cc']
    assert len(words)==1 and words[0]['u5_va']=='0046a26c'
    controls=[c for c in queries['calls']if c['entry']=='0010ca30']
    assert len(controls)==12
    shared=[line.split()[2:]for line in CREATE.read_text().splitlines()]
    rows=[]
    for i,(tag,path)in enumerate(zip(('old','u5'),IMAGES),1):
        image=packet['images'][tag];assert sha(path)==image['image_sha256'];data=path.read_bytes()
        word=words[0];raw=read_image(data,int(word['old_va'if tag=='old'else'u5_va'],16),4)
        assert raw.hex()==word['raw_hex']
        rows.append(f'I {i} {image["bank"]} '+f'{int.from_bytes(raw,"little"):08x}')
        assert len(shared[i-1])==11
        rows.append('C '+' '.join(shared[i-1]))
        for record in image['types']:rows.append(f'T {record["type"]} {record["address"]}')
        for address,hexdata in image['ranges'].items():
            assert read_image(data,int(address,16),len(hexdata)//2).hex()==hexdata
            rows.append(f'R {address} {hexdata}')
        for control in controls:
            rows.append(f'Q {control["definition"]} {control["signed_index"]} {control["tag_group"]} {control["return_eax"]}')
        rows.append('E')
    target=out/'tables.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='object-construction-provider-export-v1',exporter_sha256=sha(Path(__file__)),
        input_pins={str(path.relative_to(ROOT))if path.is_relative_to(ROOT)else str(path):sha(path)
                    for path in [PACKET,QUERIES,ADMISSION,CREATE]+IMAGES},
        fixture_sha256=sha(target),immutable_construction_bytes=960,extra_admission_bytes=8,
        actual_old_group_queries=12,u5_queries_reuse_old_authored_group_abi=True,
        expected_from_native=False,mutable_source_state_imported=False)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(fixture_sha256=sha(target),report_sha256=sha(out/'report.json'))))

if __name__=='__main__':main()
