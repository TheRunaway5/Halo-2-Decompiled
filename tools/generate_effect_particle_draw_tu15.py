#!/usr/bin/env python3
"""Lower whole owned TU1.5 particle draw and transparent-command owners."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0xbcdf0,0x1e0f0,0x1e4c0,0x140c80,0x140cf0,0x135970,0x136ea0,0x1373f0,0x171070,0x1770c0,0x1772c0,0x17de60,0x167ef0,0x184690,0x252ec0,0x22ace0,0x22b720,0x380fa0)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=Path('src/source_effect_particle_draw_tu15_generated.cpp'));ap.add_argument('--directory',type=Path,required=True);a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');sites,instructions,addresses,routines={},{},{},[]
    assert int.from_bytes(image.read(0x46d990,4),'little')==0x380fa0
    for entry in sorted(ENTRIES):
        body=_trace(code,entry,set(ENTRIES));assert body.code and all(width in (1,4)for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,w,n in body.tables if w==4],byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex())for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for o in i.operands:
                if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
                elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_effect_particle_draw_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_effect_particle_draw_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_effect_particle_draw_tu15').replace('source_physics_live_support_tu15_owns_entry','source_effect_particle_draw_tu15_owns_entry');a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)));(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print('Complete particle draw owners',len(ENTRIES),'instructions',len(instructions))
if __name__=='__main__':main()
