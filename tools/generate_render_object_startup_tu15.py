#!/usr/bin/env python3
"""Lower unchanged TU1.5 startup slots33..44 and their reached CPU constructors."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES = [0x2b480,0x31d400,0x186cb0,0x157e10,0x171e10,0x1800b0,0x17e640,
           0x1789f0,0x228260,0x3b890,0x3d260,0x16780,0x127210,0x166a80,
           0x166ae0,0x16e450,0x16e4b0,0x16e600,0x43880,0x176c80,0x13fc50,
           0x32a4e0,0x33b3ac]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args()
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'.text')
    bodies={entry:_trace(code,entry,set(ENTRIES)) for entry in ENTRIES}
    assert not any(body.tables for body in bodies.values())
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
        routines=[dict(entry=entry,tables=[],byte_tables=[]) for entry in ENTRIES])
    args.correspondence=args.manifest.with_name('render-object-startup-bodies.json')
    args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_render_object_startup_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_render_object_startup_tu15').replace('source_physics_live_support_tu15_owns_entry','source_render_object_startup_tu15_owns_entry')
    args.output.write_text(text)
    report=json.loads(args.manifest.read_text())
    report.update(render_object_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))

if __name__=='__main__':main()
