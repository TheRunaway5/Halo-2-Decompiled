#!/usr/bin/env python3
"""Lower original TU1.5 inverted contact query reached by live multiplayer."""
import argparse,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0x2f9d70,0x2f7480,0x2f6ca0,0x2e44d0,0x3040d0,0x303da0,0x303e20,0x303f80,0x304ac0,0x325d80,0x303ea0,0x303930,0x3039b0,0x303a30,0x2f4880,0x2f4580,0x2f3260,0x2f4690,0x2f3de0,0x2f3190,0x2f70d0,0x2ee660,0x2ebbe0,0x2ebc70,0x1ce2c0,0x2f1930,0x2f1590,0x2fc5d0,0x2fc3e0,0x2e46e0)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/physics/collision/source_collision_query_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0x2f74a0).bytes.hex()=='e8fbf7ffff'
    assert code.at(0x2f6cd6).bytes.hex()=='e8f5d7feff'
    # Actual Midship contact dispatch and immutable agent registration.
    assert code.at(0x2f76f3).bytes.hex()=='ffd7'
    assert code.at(0x304b63).bytes.hex()=='c7442410d0403000'
    assert image.read(0x41d03c,32).hex()=='a0513000a03d3000203e3000803f3000c04a300000312f00805d3200a03e3000'

    # Actual occupied Spectre contact reached this dynamic dispatch slot.
    # The complete wrapper retains its original stack collector/interface and
    # calls the existing original inverted-query owner,2F6FC0.
    assert code.at(0x2f62d9).bytes.hex()=='ff94818c110000'
    assert code.at(0x2f7114).bytes.hex()=='e8a7feffff'
    assert image.read(0x41ca40,8).hex()=='500a2f0060b93000'

    # Actual1CE210 invokes the broadphase's +3C query from ordinary grenade
    # collision. Retain its complete quantized query and scoped scratch owners.
    assert code.at(0x1ce2b4).bytes.hex()=='ff523c'
    assert int.from_bytes(image.read(0x41c6fc,4),'little')==0x2ee660
    assert int.from_bytes(image.read(0x461610,4),'little')==0x1ce2c0
    assert code.at(0x2eea39).bytes.hex()=='ff5204'
    assert code.at(0x186965).bytes.hex()=='ff94828c210000'
    assert code.at(0x2f197a).operands[0].imm==0x2f1590
    assert code.at(0x2f7038).bytes.hex()=='ff94b18c110000'
    assert code.at(0x2fc614).operands[0].imm==0x2fc3e0
    assert code.at(0x2fc436).operands[0].imm==0x2e46e0
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
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_collision_query_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_collision_query_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_collision_query_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_collision_query_tu15_owns_entry')
    source=source.replace('#include "halo2/source_collision_query_tu15.hpp"','#include "halo2/source_collision_query_tu15.hpp"\n#include "halo2/source_physics_entry_observation.hpp"')
    source=source.replace('LiveState s{m,observe,services,image};s.load(abi);','LiveState s{m,observe,services,image};s.load(abi);\n    SourcePhysicsEntryObservation diagnostic(m,entry,[&]{return s.save();});')
    temporary=a.output.with_name(a.output.name+'.tmp');temporary.write_text(source);os.replace(temporary,a.output)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
