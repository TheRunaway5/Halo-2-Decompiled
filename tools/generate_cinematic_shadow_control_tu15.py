#!/usr/bin/env python3
"""Lower the complete original TU1.5 cinematic-shadow control bank owner."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body,generate,sha

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert not a.output.exists() and not a.directory.exists()
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');entry=0x30e10
    definition=struct.unpack('<I',image.read(0x480c00+146*4,4))[0]
    assert struct.unpack('<I',image.read(definition+4,4))[0]==0x2acca0
    call=code.at(0x2accd8);assert call.mnemonic=='call' and call.operands[0].imm==entry
    body=complete_body(code,entry);assert len(body.code)==49 and not body.tables
    instructions,addresses={},{}
    for pc in sorted(body.code):
        i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size) and i.mnemonic!='call'
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for op in i.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{entry:08x}':entry},
        body_sites={f'{entry:08x}':sorted(body.code)},instruction_map=instructions,
        address_map={f'{q:08x}':v for q,v in addresses.items()},routines=[dict(entry=entry,tables=[],byte_tables=[])])
    a.directory.mkdir(parents=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_cinematic_shadow_control_tu15.py')
    s=s.replace('halo2/physics_live_support_tu15.hpp','halo2/source_cinematic_shadow_control_tu15.hpp')
    s=s.replace('source_run_physics_live_support_tu15','source_run_cinematic_shadow_control_tu15')
    s=s.replace('source_physics_live_support_tu15_owns_entry','source_cinematic_shadow_control_tu15_owns_entry')
    a.output.write_text(s);report=json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        hs_registration=hex(definition),wrapper='002acca0',call_site='002accd8')
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
