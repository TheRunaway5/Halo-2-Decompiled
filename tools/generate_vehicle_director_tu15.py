#!/usr/bin/env python3
"""Lower complete original TU1.5 normal camera scheduling and transition owners."""
import argparse,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0x158290,0x157fe0,0x158450,0x158900,0x158790,0x171ff0,0x245400,0x245060,0x189f20,0xcad60,0x106160,0x37cde8,0x246cf0,0x247140,0x247020,0x247090,0x246e90,0x246f30,0xbcee0,0x382360,0x382380)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/source_vehicle_director_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    # Literal CRT registrations and complete MOVSS/MOVSS/RET owners. The
    # runtime must execute these before245400 consumes their BSS destinations.
    for entry,row,operand,destination in ((0x382360,0x46dc90,0x46a164,0x56d324),
                                         (0x382380,0x46dc94,0x46a5f8,0x56d328)):
        assert int.from_bytes(image.read(row,4),'little')==entry
        assert image.read(entry,17)==(b'\xf3\x0f\x10\x05'+operand.to_bytes(4,'little')+
            b'\xf3\x0f\x11\x05'+destination.to_bytes(4,'little')+b'\xc3')
    assert code.at(0x15812c).bytes.hex()=="ff53b6"
    assert code.at(0x1581e2).operands[0].imm==0x171ff0
    sites,instructions,addresses,routines={},{},{},[]
    a.directory.mkdir(parents=True,exist_ok=True)
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert body.tail_jumps==({0x106160} if entry==0xcad60 else set())
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        (a.directory/f'{entry:08x}.asm').write_text(''.join(
            f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
            for pc in sorted(body.code)))
        routines.append(dict(entry=entry,tables=[dict(disc=q,count=n) for q,w,n in body.tables if w==4],
            byte_tables=[dict(address=q,width=w,count=n,raw_hex=image.read(q,w*n).hex()) for q,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{q:08x}':q for q in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{q:08x}':v for q,v in addresses.items()},routines=routines)
    correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_vehicle_director_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_vehicle_director_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_vehicle_director_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_vehicle_director_tu15_owns_entry')
    temporary=a.output.with_name(a.output.name+'.tmp');temporary.write_text(source);os.replace(temporary,a.output)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
