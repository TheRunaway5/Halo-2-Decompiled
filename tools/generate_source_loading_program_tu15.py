#!/usr/bin/env python3
"""Lower complete authenticated Xbox loading compositor CPU/geometry bodies.

GPU/resources retain explicit sourceABI service boundaries; no instructions or
binary blobs are copied into the production executable as executable input.
Generate only under tools/run_test.py owned scratch, then publish explicitly.
"""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES=(0x288c0,0x28930,0x28960,0x289c0,0x29060,0x29350,0x296a0,
    0x298d0,0x29ae0,0x2a2d0,0x2a8b0,0x34410,0x350d0,0x134ad0,0x134a50,0x1349e0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--publish',action='store_true',help='publish generated source and small manifest in this checkout')
    parser.add_argument('--manifest',type=Path,required=True);args=parser.parse_args()
    owned=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(owned)and args.manifest.resolve().is_relative_to(owned)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    bodies={p:_trace(code,p,set(ENTRIES))for p in ENTRIES}
    instructions,addresses={},{}
    for pc in sorted(set().union(*(b.code for b in bodies.values()))):
        i=code.at(pc);assert bytes(i.bytes)==image.read(pc,i.size)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
        for o in i.operands:
            if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
            if o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites={f'{p:08x}':sorted(b.code)for p,b in bodies.items()},
        instruction_map=instructions,address_map={f'{p:08x}':v for p,v in addresses.items()},
        routines=[dict(entry=p,
            tables=[dict(disc=q,count=n)for q,width,n in b.tables if width==4],
            byte_tables=[dict(address=q,width=width,count=n,raw_hex=image.read(q,width*n).hex())
                for q,width,n in b.tables if width==1])
            for p,b in bodies.items()])
    args.correspondence=owned/'loading-tu15-bodies.json';args.correspondence.write_text(json.dumps(paired,indent=2)+'\n')
    args.additional_entries=Path(__file__);generate(args,paired,complete_listings=True)
    source=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_loading_program_tu15.hpp').replace(
        'source_run_physics_live_support_tu15','source_run_loading_program_tu15').replace(
        'source_physics_live_support_tu15_owns_entry','source_loading_program_tu15_owns_entry')
    source=source.replace('tools/generate_physics_live_support_tu15.py','tools/generate_source_loading_program_tu15.py')
    args.output.write_text(source)
    report=json.loads(args.manifest.read_text());report.update(scope=__doc__,original_sha256=NEW_PIN,
        generated_cpp_sha256=hashlib.sha256(source.encode()).hexdigest())
    args.manifest.write_text(json.dumps(report,indent=2)+'\n')
    if args.publish:
        root=Path(__file__).resolve().parents[1]
        target=root/'src/rendering/loading/source_loading_program_tu15_generated.cpp'
        target.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(args.output,target)
        receipt=root/'build/test-results/og-matchmaking-complete-20261025/original-loading-program-generation.json'
        receipt.parent.mkdir(parents=True,exist_ok=True);receipt.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(entries=len(ENTRIES),instructions=len(instructions),source_bytes=len(source))))


if __name__=='__main__':main()
