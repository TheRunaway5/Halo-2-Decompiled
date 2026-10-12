#!/usr/bin/env python3
"""Export only verified selected-image pressure/component/rebuild operands."""
import argparse,json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha
from export_object_construction_tables import read_image,IMAGES

PRESSURE=ROOT/'analysis/multiplayer/integration/spawn-source/object-preparation-pressure/u5/operand-packet.json'
COMPONENT=ROOT/'analysis/multiplayer/integration/physics-component-construction-native/paired/report.json'
REBUILD=ROOT/'analysis/multiplayer/integration/physics-component-rebuild-native/controls/operands.json'

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    assert sha(PRESSURE)=='114e5aa6a177a98c850415428a5f0308db1237236f5c611d4b455a06512dd567'
    pressure=json.loads(PRESSURE.read_text());component=json.loads(COMPONENT.read_text());rebuild=json.loads(REBUILD.read_text());rows=[]
    tick=next(m for o in component['owners']for m in o['memory']if m['old_va']=='00465198')
    for index,(path,dispatch)in enumerate(zip(IMAGES,rebuild['images'],strict=True),1):
        data=path.read_bytes();side='old'if index==1 else'u5'
        assert sha(path)==pressure['images'][index-1]['sha256']==dispatch['sha256']
        words=[]
        for operand in pressure['operands']:
            raw=read_image(data,int(operand[side+'_va'],16),4);assert raw.hex()==operand[side+'_hex'];words.append(int.from_bytes(raw,'little'))
        raw=read_image(data,int(tick['old_va'if index==1 else'new_va'],16),4)
        assert raw.hex()==tick['old_hex'if index==1 else'new_hex'];words.append(int.from_bytes(raw,'little'))
        types=read_image(data,int(dispatch['remap'],16),13);targets=read_image(data,int(dispatch['table'],16),24)
        assert types.hex()==dispatch['remap_hex']and targets.hex()==dispatch['table_hex']
        rows.append('I '+' '.join([str(index),*[f'{v:08x}'for v in words],dispatch['remap'],dispatch['table']]))
        rows.append('T '+' '.join(f'{int.from_bytes(targets[i:i+4],"little"):08x}'for i in range(0,24,4)))
        rows.append('B '+types.hex());rows.append('E')
    target=out/'operands.tsv';target.write_text('\n'.join(rows)+'\n')
    report=dict(schema='physics-construction-immutable-export-v1',exporter_sha256=sha(Path(__file__)),
        input_pins={str(p.relative_to(ROOT))if p.is_relative_to(ROOT)else str(p):sha(p)for p in[PRESSURE,COMPONENT,REBUILD]+IMAGES},
        fixture_sha256=sha(target),raw_scalar_bytes=56,raw_dispatch_bytes=74,images=2,
        expected_from_native=False,mutable_heap_pool_clock_world_readiness_imported=False)
    (out/'report.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(report_sha256=sha(out/'report.json'),fixture_sha256=sha(target))))

if __name__=='__main__':main()
