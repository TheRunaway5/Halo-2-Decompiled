#!/usr/bin/env python3
"""Lower complete original TU1.5 CRT floating formatter and its registration."""
import argparse,json,os
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

ENTRIES=(0x32cbc0,0x331e10,0x331abd,0x331b22,0x331a65,0x331b08,
    0x331d31,0x331d83,0x331c2c,0x332e2b,0x332e58,0x333482,0x333349,
    0x331c8a,0x331b83,0x332e85,0x332dff,0x332e15,0x3333c8,0x333b30,
    0x331b60,0x3337ca,0x333aba,0x332ca7,0x333898,0x33376f,0x33379d,
    0x333711,0x332c13,0x332bf8,0x332b86,0x332c2c,0x3336f0,0x332b07,0x332b39)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/source_crt_float_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0x32cbf8).bytes.hex()=='e8c3ffffff'
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4)for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        a.directory.mkdir(parents=True,exist_ok=True)
        (a.directory/f'{entry:08x}.asm').write_text(''.join(
            f'{pc:08x} {code.at(pc).bytes.hex()} {code.at(pc).mnemonic} {code.at(pc).op_str}\n'
            for pc in sorted(body.code)))
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
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_crt_float_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_crt_float_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_crt_float_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_crt_float_tu15_owns_entry')
    temporary=a.output.with_name(a.output.name+'.tmp');temporary.write_text(source);os.replace(temporary,a.output)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
