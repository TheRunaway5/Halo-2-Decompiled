#!/usr/bin/env python3
"""Lower the authenticated whole TU1.5 kind3 transparent callback."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
ENTRY=0x423b0
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--directory',type=Path,required=True)
    p.add_argument('--output',type=Path,default=Path('src/source_transparent_group_tu15_generated.cpp'))
    p.add_argument('--test-manifest',type=Path,help='Explicit optional test fixture output; omitted by read-only provenance audits')
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    parent=_trace(code,0x42750,{0x42750})
    callbacks=[]
    for pc in parent.code:
        i=code.at(pc)
        if any(o.type==2 and o.imm==ENTRY for o in i.operands):callbacks.append(pc)
    assert callbacks,'No original kind3 queue callback publication'
    body=_trace(code,ENTRY,{ENTRY});assert body.code and not body.tables
    calls={0x423e1:0x352d0,0x423f1:0x44c80,0x42703:0x44360}
    for pc,target in calls.items():
        i=code.at(pc);assert pc in body.code and i.mnemonic=='call'and i.operands[0].imm==target
    addresses={};instruction_map={}
    for pc in sorted(body.code):
        i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
        instruction_map[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for o in i.operands:
            if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
            elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{ENTRY:08x}':ENTRY},
        body_sites={f'{ENTRY:08x}':sorted(body.code)},instruction_map=instruction_map,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=ENTRY,tables=[],byte_tables=[])])
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_transparent_group_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_transparent_group_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_transparent_group_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_transparent_group_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        callback_publication=dict(owner=0x42750,sites=sorted(callbacks),target=ENTRY),
        authenticated_call_sites={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},
        scope='Whole kind3 CPU callback only; required external graphics/collector/selector owners are not supplied')
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    if a.test_manifest:
        a.test_manifest.parent.mkdir(parents=True,exist_ok=True)
        a.test_manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Whole TU1.5 kind3 group callback:',len(body.code),'instructions')
if __name__=='__main__':main()
