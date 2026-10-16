#!/usr/bin/env python3
"""Lower whole original TU1.5 tag-block cache pressure and eviction owners.

Original LRU/header/cancellation/worker/free owners remain actual callees.
No successful allocation, completed pending read or released resource is supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRIES=(0x125fb0,0x1263c0,0x126400)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists()or a.directory.exists():raise ValueError('Preserve generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected English TU1.5')
    x=Xbe(NEW_DEFAULT);c=_Code(x,'.text');call=c.at(0x126319)
    if call.mnemonic!='call'or call.operands[0].imm!=0x125fb0:raise ValueError('Original resource caller differs')
    for pc,target in [(0x125fdf,0x126400),(0x126042,0x126400),(0x126049,0x1263c0)]:
        i=c.at(pc)
        if i.mnemonic!='mov'or i.operands[1].imm!=target:raise ValueError('Original cache callback registration differs')
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
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_resource_cache_pressure_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_resource_cache_pressure_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_resource_cache_pressure_tu15').replace('source_physics_live_support_tu15_owns_entry','source_resource_cache_pressure_tu15_owns_entry');a.output.write_text(s)
    f=a.directory/'generation.json';d=json.loads(f.read_text());d.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__);f.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'whole original resource-pressure bodies;',len(instructions),'instructions')
if __name__=='__main__':main()
