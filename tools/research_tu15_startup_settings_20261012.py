#!/usr/bin/env python3
"""Authenticate the next TU1.5 startup settings and timer owners, without returns."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from oracle_tu15_startup_records_20261011 import ROOT,NEW_DEFAULT,NEW_PIN,Xbe,_Code,_trace

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');owners={}
    starts={0x2e00af,0x2e0fe2,0x2e101c,0x2e0082}
    for entry in sorted(starts):
        trace=_trace(code,entry,starts);rows=[]
        for p in sorted(trace.code):
            i=code.at(p);assert image.read(p,i.size)==i.bytes
            rows.append(dict(address=p,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str))
        text='\n'.join(f"{r['address']:08x} {r['bytes']} {r['mnemonic']} {r['operands']}" for r in rows)+'\n'
        listing=a.output/(f'{entry:08x}.asm');listing.write_text(text)
        owners[f'{entry:08x}']=dict(instructions=rows,listing_sha256=sha(listing))
    imports={}
    for site,cell,ordinal,name in ((0x2e101c,0x41b6f0,24,'ExQueryNonVolatileSetting'),
        (0x2e0ffa,0x41b610,301,'RtlNtStatusToDosError'),(0x2e0096,0x41b6f8,149,'KeSetTimer')):
        i=code.at(site);assert i.operands[0].mem.disp==cell
        target=int.from_bytes(image.read(cell,4),'little');assert target==0x80000000+ordinal
        imports[str(ordinal)]=dict(name=name,instruction=site,cell=cell,encoded_target=target,
            raw_cell=image.read(cell,4).hex(),instruction_bytes=i.bytes.hex())
    assert code.at(0x2e1001).mnemonic=='ret' and code.at(0x2e1001).operands[0].imm==20
    inspection=ROOT/'analysis/multiplayer/map-pack/smg-audio-host-contract/source-inspection.json'
    references=[]
    for row in json.loads(inspection.read_text())['primary_compatibility_references']:
        if Path(row['path']).name in ('EmuKrnlEx.cpp','EmuEEPROM.cpp','EmuEEPROM.h'):
            assert sha(ROOT/row['path'])==row['sha256'];references.append(row)
    assert len(references)==3
    types=ROOT/'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-types.h'
    assert sha(types)=='7223ea0a9fb7e31e804008d5b0b1b3167908ab9c16d43fc2c8a67375c0f58f88'
    assert re.search(r'XC_MISC\s*=\s*0x11\b',types.read_text())
    info=dict(schema=1,parity_target='English TU1.5 only',original_sha256=NEW_PIN,tool_sha256=sha(__file__),
        source_owners=owners,imports=imports,primary_compatibility_references=references,
        settings_request=dict(caller=0x2e00fc,wrapper=0x2e0fe2,thunk=0x2e101c,index=17,index_name='XC_MISC',
            frame_words=['return2E0101',17,'EBP-12 type pointer','EBP-4 value pointer',4,'EBP-8 length pointer']),
        required_next=['Compile the original wrapper/thunk and authenticate their dispatch wiring',
            'Implement the actual nonvolatile data resource query with validated checksum/extent/type semantics',
            'Recover status conversion and actual timer resource ownership; preserve all reached failures',
            'Continue fresh TU1.5 startup through its original heap installation; do not skip setup'],
        settings_returns_supplied=[],timer_returns_supplied=[],original_instruction_patches=[],older_image_executed=False,
        title_executed=False,heap_installed=False,live_physics_parity=False)
    (a.output/'report.json').write_text(json.dumps(info,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(bodies=len(owners),instructions=sum(len(v['instructions']) for v in owners.values()),
        imports=imports,report_sha256=sha(a.output/'report.json'))))

if __name__=='__main__':main()
