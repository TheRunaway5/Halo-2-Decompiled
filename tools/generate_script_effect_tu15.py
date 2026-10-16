#!/usr/bin/env python3
"""Lower the complete original script marker-effect request bodies to C++."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES=(0x2aa360,0x179700,0x1791c0)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=Path('src/source_script_effect_tu15_generated.cpp'))
    parser.add_argument('--directory',type=Path,required=True)
    a=parser.parse_args()
    if a.output.exists() or a.directory.exists():raise ValueError('Preserve existing generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected original English TU1.5 executable')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    calls={0x2aaeb5:0x2aa360,0x2aa380:0xbad60,0x2aa3a3:0x179700,
        0x1797c0:0xbad60,0x1797d0:0x1791c0,0x1797e6:0x178e30}
    for pc,target in calls.items():
        instruction=code.at(pc)
        if instruction.mnemonic!='call' or instruction.operands[0].imm!=target:
            raise ValueError('Original call correspondence changed')
    sites={};instructions={};addresses={};routines=[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        if not body.code or body.tables:raise ValueError('Unexpected script-effect control flow')
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            instruction=code.at(pc)
            if image.read(pc,instruction.size)!=bytes(instruction.bytes):raise ValueError('Original instruction differs')
            instructions[f'{pc:08x}']=pc
            addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    correspondence=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True)
    manifest=a.directory/'bodies.json';manifest.write_text(json.dumps(correspondence,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',
        correspondence=manifest,additional_entries=Path(__file__)),correspondence)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_script_effect_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_script_effect_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_script_effect_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_script_effect_tu15_owns_entry')
    a.output.write_text(source)
    path=a.directory/'generation.json';report=json.loads(path.read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        original_call_sites={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},
        boundary='Complete script request bodies; original marker lookup/allocation and all frame/GPU consumers remain external owners.')
    path.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'complete script-effect bodies;',len(instructions),'original instructions')

if __name__=='__main__':main()
