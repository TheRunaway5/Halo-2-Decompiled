#!/usr/bin/env python3
"""Lower whole owned TU1.5 AI storage, actor and birth-state owners."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

# Existing scene-device storage owns1E7D90; live-support owns1E8410, CE440
# and the membership owners. Keep their actual external dispatch boundaries.
# State-context startup owns1E7710; its actual new-map owner1E7760 is here.
ENTRIES=(0x1cfa40,0x1cfc90,0x297640,0x265e80,0x208bf0,0x261a10,
    0x213c00,0x29be40,0x1af970,0x1d02c0,0x208df0,0x213c80,0x29f680,
    0x1e7e40,0x1ecc40,0x1b0cd0,0x1afa90,0x208ee0,0x208c90,0x20d540,
    0x1e9000,0x1ebe10,0x1ebcb0,0x275e30,0x271080,0x275b70,0x1ea1d0,0x1e7760,
    # Authored squad birth uses the real iterator/activation owners. The
    # campaign new-player continuation14E798 registers its full player datum.
    0x20cff0,0x1e9460,0x1e96a0,0x272520,0x1d03a0)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=Path('src/source_ai_actor_tu15_generated.cpp'))
    ap.add_argument('--directory',type=Path,required=True);a=ap.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert struct.unpack('<3I',image.read(0x44b6f8,12))==(0x1cfa40,0x1cfc70,0x1cfc90)
    assert struct.unpack('<3I',image.read(0x44b158,12))==(0x1e7710,0x31d400,0x1e7760)
    sites,instructions,addresses,routines={},{},{},[]
    for entry in sorted(ENTRIES):
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for operand in i.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_ai_actor_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_ai_actor_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_ai_actor_tu15').replace('source_physics_live_support_tu15_owns_entry','source_ai_actor_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)))
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete AI storage/actor owners',len(ENTRIES),'instructions',len(instructions))

if __name__=='__main__':main()
