#!/usr/bin/env python3
"""Lower whole TU1.5 section/group collector44C80 from authenticated bytes."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRY=0x44c80
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path('src/source_transparent_collection_tu15_generated.cpp'))
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    parent=_trace(code,0x423b0,{0x423b0});call=code.at(0x423f1)
    assert 0x423f1 in parent.code and call.mnemonic=='call' and call.operands[0].imm==ENTRY
    body=_trace(code,ENTRY,{ENTRY});assert body.code and not body.tables
    addresses={};instructions={};calls={}
    for pc in sorted(body.code):
        i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for o in i.operands:
            if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
            elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
        if i.mnemonic=='call':
            assert i.operands[0].type==2;calls[f'{pc:08x}']=f'{i.operands[0].imm:08x}'
    assert set(calls.values())=={'00045cd0','00041030'}
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{ENTRY:08x}':ENTRY},
        body_sites={f'{ENTRY:08x}':sorted(body.code)},instruction_map=instructions,
        address_map={f'{pc:08x}':q for pc,q in addresses.items()},routines=[dict(entry=ENTRY,tables=[],byte_tables=[])])
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired,code_override=code)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_transparent_collection_tu15.py')
    s=s.replace('halo2/physics_live_support_tu15.hpp','halo2/source_transparent_collection_tu15.hpp')
    s=s.replace('source_run_physics_live_support_tu15','source_run_transparent_collection_tu15')
    s=s.replace('source_physics_live_support_tu15_owns_entry','source_transparent_collection_tu15_owns_entry')
    a.output.write_text(s)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        authenticated_parent=dict(entry=0x423b0,site=0x423f1,target=ENTRY),calls=calls,
        scope='Whole44C80 CPU collector; actual45CD0/41030 callees remain required; no shader or GPU results supplied')
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Whole44C80 collector:',len(body.code),'instructions;',calls)
if __name__=='__main__':main()
