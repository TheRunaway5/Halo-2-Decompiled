#!/usr/bin/env python3
"""Lower all targets of the three original TU1.5 animation composition dispatchers.

All rotation, translation and scale branches remain. Indirect codec callbacks
retain their actual source owners; no decoder, channel, mask or pose is supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
# Pin the exact lowering backend used for the tested module; other generators
# in the shared checkout have independent revisions.
from generate_animation_composition_backend_tu15 import generate,sha
ENTRIES=(0x284330,0x284360,0x284390,0x286b30,0x287130,0x287480,0x287a20,0x287d00,0x2883c0,0x2887f0,0x288e70,0x28a7d0,0x28ada0,0x28b080,0x28b5f0,0x28b870,0x28bf00,0x28c2c0,0x28c8f0,0x28e0b0,0x28e730,0x28eaf0,0x28f110,0x28f460,0x28fba0,0x290030,0x290710)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists()or a.directory.exists():raise ValueError('Preserve generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected English TU1.5')
    x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');calls={0x28415b:0x286b30,0x284160:0x287130,0x284169:0x287480,0x28416e:0x287a20,0x28418a:0x287d00,0x28418f:0x2883c0,0x284198:0x2887f0,0x28419d:0x288e70,0x2841a7:0x284330,0x2841cb:0x28a7d0,0x2841d0:0x28ada0,0x2841d9:0x28b080,0x2841de:0x28b5f0,0x2841fa:0x28b870,0x2841ff:0x28bf00,0x284208:0x28c2c0,0x28420d:0x28c8f0,0x284217:0x284360,0x28423b:0x28e0b0,0x284240:0x28e730,0x284249:0x28eaf0,0x28424e:0x28f110,0x28426a:0x28f460,0x28426f:0x28fba0,0x284278:0x290030,0x28427d:0x290710,0x284287:0x284390}
    for pc,target in calls.items():
        i=c.at(pc)
        if i.mnemonic!='jmp'or i.operands[0].imm!=target:raise ValueError('Original call differs')
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        b=_trace(c,entry,set(ENTRIES))
        if not b.code or b.tables:raise ValueError('Unexpected original CFG')
        sites[f'{entry:08x}']=sorted(b.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(b.code):
            i=c.at(pc)
            if x.read(pc,i.size)!=bytes(i.bytes):raise ValueError('Original instruction differs')
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True);manifest=a.directory/'bodies.json';manifest.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=manifest,additional_entries=Path(__file__)),paired)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_animation_composition_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_animation_composition_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_animation_composition_tu15').replace('source_physics_live_support_tu15_owns_entry','source_animation_composition_tu15_owns_entry');a.output.write_text(s)
    s=a.output.read_text().replace('#include "halo2/source_animation_composition_tu15.hpp"',
        '#include "halo2/source_animation_composition_tu15.hpp"\n#include "halo2/source_physics_entry_observation.hpp"')
    marker='FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);'
    assert s.count(marker)==1
    s=s.replace(marker,marker+'\n    SourcePhysicsEntryObservation phase(m,entry,[&]{return s.save();});')
    a.output.write_text(s)
    for entry in ENTRIES:
        (a.directory/f'{entry:08x}.asm').write_text(''.join(
            f'{pc:08x} {bytes(c.at(pc).bytes).hex()} {c.at(pc).mnemonic} {c.at(pc).op_str}\n'
            for pc in paired['body_sites'][f'{entry:08x}']))
    f=a.directory/'generation.json';d=json.loads(f.read_text());d.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__);f.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'whole original animation-composition bodies;',len(instructions),'instructions')
if __name__=='__main__':main()
