#!/usr/bin/env python3
"""Lower complete TU1.5 conveyor derivative and live dynamic-prop query wrapper."""
import argparse,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha,complete_body

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=Path('src/source_conveyor_tu15_generated.cpp'));p.add_argument('--directory',required=True,type=Path);a=p.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    caller=code.at(0x184c9e);assert caller.mnemonic=='call' and caller.operands[0].imm==0x355d0
    entries=(0x355d0,0x1e1170)
    instructions={};addresses={};sites={};calls=set();routines=[]
    for entry in entries:
        body=complete_body(code,entry);assert body.code and not body.tables
        if entry==0x355d0:assert all(0x355d0<=q<0x3577d for q in body.code)
        else:assert all(0x1e1170<=q<0x1e11a6 for q in body.code)
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for q in sorted(body.code):
            i=code.at(q);assert image.read(q,i.size)==bytes(i.bytes);instructions[f'{q:08x}']=q;addresses[q]=q;addresses[q+i.size]=q+i.size
            if i.mnemonic=='call' and i.operands[0].type==2:calls.add(i.operands[0].imm)
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    assert calls=={0x13de70,0x315c90,0x315cb0}
    assert code.at(0x1e119f).bytes.hex()=='ff5260'
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{q:08x}':q for q in entries},body_sites=sites,instruction_map=instructions,address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_conveyor_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_conveyor_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_conveyor_tu15').replace('source_physics_live_support_tu15_owns_entry','source_conveyor_tu15_owns_entry')
    source=source.replace('#include "halo2/source_conveyor_tu15.hpp"','#include "halo2/source_conveyor_tu15.hpp"\n#include "halo2/source_physics_entry_observation.hpp"')
    source=source.replace('LiveState s{m,observe,services,image};s.load(abi);','LiveState s{m,observe,services,image};s.load(abi);\n    SourcePhysicsEntryObservation diagnostic(m,entry,[&]{return s.save();});')
    temporary=a.output.with_name(a.output.name+'.tmp');temporary.write_text(source);os.replace(temporary,a.output)
    report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),source_call=dict(site='00184c9e',target='000355d0',raw=caller.bytes.hex()),complete_sites=sum(map(len,sites.values())),existing_original_callees=[f'{q:08x}' for q in sorted(calls)],original_virtual_call=dict(site='001e119f',raw='ff5260',slot=96));(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('PASS complete TU1.5 dynamic prop query/conveyor:',sum(map(len,sites.values())),'sites; original callees retained')

if __name__=='__main__':main()
