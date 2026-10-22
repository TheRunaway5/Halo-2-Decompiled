#!/usr/bin/env python3
"""Lower the complete original TU1.5 vehicle occupant relation guard."""
import argparse,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0xed500,0xb2dc0,0xd3470,0x154c70,0x1aa2b0,0x83050,0x19b180)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/gameplay/vehicles/source_vehicle_occupancy_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0xed705).bytes.hex()=='e8f6fdffff'
    assert code.at(0xed705).operands[0].imm==0xed500
    assert code.at(0xed777).bytes.hex()=='e84456fcff'
    assert code.at(0xed777).operands[0].imm==0xb2dc0
    assert code.at(0xed964).bytes.hex()=='e8075bfeff'
    assert code.at(0xed964).operands[0].imm==0xd3470
    assert code.at(0xedb0f).operands[0].imm==0x154c70
    assert code.at(0x830cc).operands[0].imm==0x1aa2b0
    assert code.at(0x8318e).operands[0].imm==0x83050
    assert code.at(0x15ac6f).operands[0].imm==0x19b180
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4)for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=q,count=n)for q,w,n in body.tables if w==4],
            byte_tables=[dict(address=q,width=w,count=n,raw_hex=image.read(q,w*n).hex())for q,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{q:08x}':q for q in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_vehicle_occupancy_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_vehicle_occupancy_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_vehicle_occupancy_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_vehicle_occupancy_tu15_owns_entry')
    temporary=a.output.with_name(a.output.name+'.tmp');temporary.write_text(source);os.replace(temporary,a.output)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
