#!/usr/bin/env python3
"""Lower whole authenticated TU1.5 speech/facial-curve owners, without supplied voices.

Actual object/tag/curve/resource banks and native PCM progress are separate
caller inputs. Whole original resource admission, unit lookup and graph binding
remain required owners. No mouth weights, sound handles or readiness are supplied.
"""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0x18f510,0x18f810,0x110b60,0x110520,0x18f7a0,0x21f860,0x380dd0,0xd3300,0x1d4400)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    if a.output.exists() or a.directory.exists():raise ValueError('Preserve generated evidence')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Expected English TU1.5')
    x=Xbe(NEW_DEFAULT);code=_Code(x,'.text')
    if int.from_bytes(x.read(0x46d964,4),'little')!=0x380dd0:
        raise ValueError('Original facial normalization initializer registration differs')
    for pc,target in ((0x18f5ee,0x18f810),(0x18f896,0x110b60),(0x110c02,0x110520)):
        i=code.at(pc)
        if i.mnemonic!='call' or i.operands[0].imm!=target:raise ValueError('Original speech call differs')
    retirement=_trace(code,0x18f6b0,{0x18f6b0})
    if not any(code.at(pc).mnemonic=='call' and code.at(pc).operands[0].imm==0xd3300 for pc in retirement.code):
        raise ValueError('Original sound retirement does not reach the matching-tag unit owner')
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        if not body.code or body.tables:raise ValueError('Unexpected original speech CFG')
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            i=code.at(pc)
            if x.read(pc,i.size)!=bytes(i.bytes):raise ValueError('Original instruction differs')
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True);manifest=a.directory/'bodies.json'
    manifest.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=manifest,
        additional_entries=Path(__file__)),paired,runtime_header='halo2/source_unit_speech_tu15.hpp',
        runtime_function='source_run_unit_speech_tu15')
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_unit_speech_tu15.py').replace('source_run_physics_live_support_tu15',
        'source_run_unit_speech_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_unit_speech_tu15_owns_entry')
    a.output.write_text(source)
    f=a.directory/'generation.json';d=json.loads(f.read_text())
    d.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),scope=__doc__)
    f.write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')
    print(len(ENTRIES),'whole original speech bodies;',len(instructions),'instructions')
if __name__=='__main__':main()
