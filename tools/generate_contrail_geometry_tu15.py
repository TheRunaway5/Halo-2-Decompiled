#!/usr/bin/env python3
"""Lower the authenticated whole TU1.5 contrail geometry callback and callees."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

#437D6 registers42CE0;42CE0 callsBCD80/42C40;42CB3 calls17F8A0.
ENTRIES=(0x42ce0,0x42c40,0xbcd80,0x17f8a0,0x13a070,0x17ead0,0x17f3d0,0x219330,0x219e40,0x219e00,0x21a2a0)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/rendering/effects/source_contrail_geometry_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True);a=p.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,w,n in body.tables if w==4],byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex())for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for o in i.operands:
                if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
                elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True)
    correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_contrail_geometry_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_contrail_geometry_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_contrail_geometry_tu15').replace('source_physics_live_support_tu15_owns_entry','source_contrail_geometry_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)))
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original contrail geometry:',len(ENTRIES),'owners,',len(instructions),'instructions')
if __name__=='__main__':main()
