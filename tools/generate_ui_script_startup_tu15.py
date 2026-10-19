#!/usr/bin/env python3
"""Lower unchanged TU1.5 prior UI static constructor and startup slots51..58 and their reached CPU constructors."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRIES = [0x20650,0x59670,0x84890,0x848b0,0xd7320,0x11f390,0x11f3f0,
    0x126850,0x126890,0x127210,0x13d722,0x13e9e0,0x1410a7,0x14a17d,0x14a1ff,
    0x166a80,0x166ae0,0x168ba3,0x16e3d0,0x16e450,0x16e4b0,0x16e600,0x16e7f0,
    0x16e860,0x192380,0x192a05,0x19307a,0x193b22,0x193b63,0x193bc3,0x1976cf,
    0x1ab975,0x1e6fb0,0x2035b0,0x2112b0,0x2129d0,0x23b649,0x23b670,0x23b813,
    0x23b823,0x23b844,0x23b861,0x23be50,0x23c17a,0x23c1ae,0x23c1be,0x25a71c,
    0x25b5ca,0x25d841,0x25d886,0x29ff80,0x31d400,0x32a4e0,0x33b3ac,0x381390,0x13bc10,0x13d710,0x19e0ab,0x211360,0x212470,0x212a20,0x232a33,0x23be6b,
    # Complete row52 HUD new-map owner and its original local helpers.
    # 2C390 remains owned by the existing complete world/camera graph.
    0x1976fe,0x1410f5,0x13bc32,0x231048]


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
    args.correspondence=args.manifest.with_name('ui-script-startup-bodies.json')
    args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_ui_script_startup_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_ui_script_startup_tu15').replace('source_physics_live_support_tu15_owns_entry','source_ui_script_startup_tu15_owns_entry')
    args.output.write_text(text)
    report=json.loads(args.manifest.read_text())
    report.update(ui_script_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(bodies=len(ENTRIES),instructions=len(instructions))))

if __name__=='__main__':main()
