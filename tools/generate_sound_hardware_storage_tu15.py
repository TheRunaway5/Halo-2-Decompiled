#!/usr/bin/env python3
"""Lower unchanged TU1.5 APU storage requirements, allocations and tail pool."""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    args=parser.parse_args();image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    code=_Code(image,'DSOUND')
    entries=[0x38e996,0x38f53e,0x38a57d,0x38a603,0x38a313,0x383d70,0x383d92]
    bodies={entry:_trace(code,entry,set(entries)) for entry in entries}
    assert not any(body.tables for body in bodies.values())
    instructions,addresses={},{}
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
        for operand in op.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in entries},
        body_sites={f'{e:08x}':sorted(b.code) for e,b in bodies.items()},instruction_map=instructions,
        address_map={f'{p:08x}':v for p,v in addresses.items()},routines=[dict(entry=e,tables=[],byte_tables=[]) for e in entries])
    args.correspondence=args.manifest.with_name('sound-hardware-storage-bodies.json');args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n');args.additional_entries=Path(__file__)
    generate(args,paired,complete_listings=True,code_override=code)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_hardware_storage_tu15.hpp')
    text=text.replace('source_run_physics_live_support_tu15','source_run_sound_hardware_storage_tu15').replace('source_physics_live_support_tu15_owns_entry','source_sound_hardware_storage_tu15_owns_entry')
    args.output.write_text(text);report=json.loads(args.manifest.read_text())
    report.update(sound_hardware_storage_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(bodies=len(entries),instructions=len(instructions))))
if __name__=='__main__':main()
