#!/usr/bin/env python3
"""Authenticate complete player carry restriction equivalence across Xbox images."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace,PIN
IMAGES=(('retail',ROOT/'assets/disc/default.xbe','03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'),
        ('tu11',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN),
        ('tu15',Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe'),'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'))
def sha(b):return hashlib.sha256(b).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    original=Xbe(IMAGES[1][1]);assert sha(original.data)==PIN
    anchor=original.read(0x152089,27);source=_Code(original,'.text');f=_trace(source,0x152080,{0x152080});ins=[source.at(q)for q in sorted(f.code)]
    assert len(ins)==294
    labels={0x4f01f4:'players',0x4e78d0:'objects',0x4eb114:'tags'};rows=[]
    for name,path,pin in IMAGES:
        x=Xbe(path);assert sha(x.data)==pin
        candidates=[]
        for section in x.sections:
            if section.name!='.text':continue
            data=x.section_bytes(section);start=0
            while (q:=data.find(anchor,start))>=0:candidates.append(section.va+q-9);start=q+1
        assert len(candidates)==1,(name,candidates)
        entry=candidates[0];code=_Code(x,'.text');g=_trace(code,entry,{entry});actual=[code.at(q)for q in sorted(g.code)]
        assert len(actual)==len(ins);globals_map={};normalized=[]
        for before,after in zip(ins,actual):
            assert before.address-0x152080==after.address-entry and before.mnemonic==after.mnemonic
            b,c=bytearray(before.bytes),bytearray(after.bytes)
            operands=[o for o in before.operands if o.type==3 and not o.mem.base and not o.mem.index and o.mem.disp in labels]
            if operands:
                assert len(operands)==1
                q=operands[0].mem.disp;assert before.disp_size==after.disp_size==4
                target=after.disp;assert globals_map.setdefault(q,target)==target
                b[before.disp_offset:before.disp_offset+4]=bytes(4);c[after.disp_offset:after.disp_offset+4]=bytes(4)
            assert b==c,(name,hex(before.address),before.op_str,after.op_str)
            normalized.append(bytes(b))
        assert set(globals_map)==set(labels) and len(set(globals_map.values()))==3
        rows.append({'image':name,'path':str(path),'sha256':pin,'entry':f'{entry:08x}','instructions':len(actual),
            'global_relocations':{labels[q]:f'{v:08x}'for q,v in globals_map.items()},
            'normalized_complete_body_sha256':sha(b''.join(normalized))})
    assert len({r['normalized_complete_body_sha256']for r in rows})==1
    result={'schema':1,'tool_sha256':sha(Path(__file__).read_bytes()),'images':rows,
        'scope':'Complete 294-instruction player restriction body is byte-identical after only its three actual pool/tag global relocations. This proves this owner equivalence, not whole inventory/gameplay/framebuffer parity.'}
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'entries':{r['image']:r['entry']for r in rows},'instructions_per_image':294,'normalized_complete_body_sha256':rows[0]['normalized_complete_body_sha256']}))
if __name__=='__main__':main()
