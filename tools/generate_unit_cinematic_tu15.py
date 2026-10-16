#!/usr/bin/env python3
"""Lower whole original TU1.5 cinematic-unit graph reset and emotion owners.

Graph binding, facial lookup and body/controller callbacks remain actual source
owners. All branches of the graph reset and emotional control setters remain;
no animation graph, facial channel or completed callee result is supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0x1118c0,0x111940,0x111080,0x1110d0,0x380df0)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists()or a.directory.exists():raise ValueError('Preserve generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected English TU1.5')
    x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');calls={0x11de6c:0x1118c0,0x11192f:0x111940,0x2ad99f:0x111080}
    if int.from_bytes(x.read(0x46d968,4),'little')!=0x380df0:
        raise ValueError('Original facial normalization initializer registration differs')
    for pc,target in calls.items():
        i=c.at(pc)
        if i.mnemonic!='call'or i.operands[0].imm!=target:raise ValueError('Original call differs')
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
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_unit_cinematic_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_unit_cinematic_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_unit_cinematic_tu15').replace('source_physics_live_support_tu15_owns_entry','source_unit_cinematic_tu15_owns_entry');a.output.write_text(s)
    f=a.directory/'generation.json';d=json.loads(f.read_text());d.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__);f.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'whole original cinematic-unit bodies;',len(instructions),'instructions')
if __name__=='__main__':main()
