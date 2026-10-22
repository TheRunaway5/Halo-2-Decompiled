#!/usr/bin/env python3
"""Lower complete original TU1.5 helpers reached by live class3 vehicles."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body,generate,sha

ENTRIES={0x1da710:(21,{0x1cc310},set()),
         0xf85c0:(153,{0xbad60,0x148e20,0x16c620,0x122b60,0x179230,0x178e30},set()),
         0x288e70:(253,set(),{0x288f3b,0x289097,0x2891df}),
         0x305140:(27,{0x2f3620},{0x305163}),
         0x383130:(6,{0x30bae0,0x32b0b4},set()),
         0x30bae0:(4,set(),set()),
         0x383b30:(2,set(),set())}

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/gameplay/vehicles/source_vehicle_auxiliary_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0xf2420).bytes.hex()=='e8eb820e00'
    assert code.at(0xf85b2).bytes.hex()=='e809000000'
    assert image.read(0x46d768,4)==bytes.fromhex('30313800')
    sites,instructions,addresses,routines={},{},{},[]
    for entry,(count,calls,indirect) in ENTRIES.items():
        body=complete_body(code,entry)
        assert len(body.code)==count and not body.tables
        actual_calls={code.at(pc).operands[0].imm for pc in body.code if code.at(pc).mnemonic=='call' and code.at(pc).operands[0].type==2}
        actual_indirect={pc for pc in body.code if code.at(pc).mnemonic=='call' and code.at(pc).operands[0].type!=2}
        assert actual_calls==calls and actual_indirect==indirect
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[],byte_tables=[]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{q:08x}':q for q in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=False);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    manifest=a.directory/'generation.json'
    generate(SimpleNamespace(output=a.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    s=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_vehicle_auxiliary_tu15.py')
    s=s.replace('halo2/physics_live_support_tu15.hpp','halo2/source_vehicle_auxiliary_tu15.hpp')
    s=s.replace('source_run_physics_live_support_tu15','source_run_vehicle_auxiliary_tu15')
    s=s.replace('source_physics_live_support_tu15_owns_entry','source_vehicle_auxiliary_tu15_owns_entry')
    a.output.write_text(s);report=json.loads(manifest.read_text());report.update(generated_cpp_sha256=sha(a.output),generator_sha256=sha(Path(__file__)))
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
