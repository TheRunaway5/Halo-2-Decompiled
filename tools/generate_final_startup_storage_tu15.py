#!/usr/bin/env python3
"""Lower unchanged TU1.5 startup slots59..68 and their complete CPU constructor graph."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES = [0x1cfa40, 0x1cb130, 0x31d400, 0x219080, 0x137870, 0x22c980,
    0x1d0250, 0x1e7d90, 0x297640, 0x265e80, 0x208bf0, 0x261a10, 0x213c00,
    0x29be40, 0x1af970, 0x1afa90, 0x1f5ea0, 0x22cdc0, 0x1d6ee0, 0x149630, 0x149680,
    0x127210, 0x166a80, 0x166ae0, 0x16e450, 0x16e4b0, 0x140c80, 0x140cf0,
    0x11f3f0, 0x33b3ac, 0x32a4e0]



def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args()
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    bodies={entry:_trace(code,entry,set(ENTRIES)) for entry in ENTRIES}
    assert all(width == 4 for body in bodies.values() for _, width, _ in body.tables)
    instructions,addresses={},{}
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        op=code.at(pc)
        assert bytes(op.bytes)==image.read(pc,op.size)
        instructions[f'{pc:08x}']=pc
        addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
        for operand in op.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{entry:08x}':entry for entry in ENTRIES},
        body_sites={f'{entry:08x}':sorted(body.code) for entry,body in bodies.items()},
        instruction_map=instructions,address_map={f'{pc:08x}':value for pc,value in addresses.items()},
        routines=[dict(entry=entry, tables=[dict(disc=p, count=n) for p, width, n in bodies[entry].tables],
                       byte_tables=[]) for entry in ENTRIES])
    args.correspondence=args.manifest.with_name('final-startup-storage-bodies.json')
    args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_final_startup_storage_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_final_startup_storage_tu15').replace('source_physics_live_support_tu15_owns_entry','source_final_startup_storage_tu15_owns_entry')
    args.output.write_text(text)
    report=json.loads(args.manifest.read_text())
    report.update(final_startup_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))

if __name__=='__main__':main()
