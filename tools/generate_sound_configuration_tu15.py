#!/usr/bin/env python3
"""Lower exact TU1.5 configuration and synchronous output-recovery CPU bodies.

Native policy/clock/EP/AC97/voice resource providers remain required. No register
image, ignored unknown request, FIFO readiness or hardware success is supplied.
"""
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
    code=_Code(image,'DSOUND');entries=[0x38e8c9,0x38e75b,0x388a1d]
    bodies={entry:_trace(code,entry,set(entries))for entry in entries}
    assert not any(body.tables for body in bodies.values())
    instructions,addresses={},{}
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        op=code.at(pc);assert bytes(op.bytes)==image.read(pc,op.size)
        instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+op.size]=pc+op.size
        for operand in op.operands:
            if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
            if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{e:08x}':e for e in entries},
        body_sites={f'{e:08x}':sorted(body.code)for e,body in bodies.items()},instruction_map=instructions,
        address_map={f'{p:08x}':v for p,v in addresses.items()},
        routines=[dict(entry=e,tables=[],byte_tables=[])for e in entries])
    args.correspondence=args.manifest.with_name('sound-configuration-bodies.json')
    args.correspondence.parent.mkdir(parents=True,exist_ok=True)
    args.correspondence.write_text(json.dumps(paired,sort_keys=True,indent=2)+'\n')
    args.additional_entries=Path(__file__);generate(args,paired,complete_listings=True,code_override=code)
    text=args.output.read_text().replace('halo2/physics_live_support_tu15.hpp','halo2/source_sound_configuration_tu15.hpp')
    text=text.replace('source_run_physics_live_support_tu15','source_run_sound_configuration_tu15')
    text=text.replace('source_physics_live_support_tu15_owns_entry','source_sound_configuration_tu15_owns_entry')
    args.output.write_text(text)
    cold=[]
    for p in [0x391460,0x38f848,0x390bdc]:
        raw=image.read(p,4);assert raw==bytes(4)
        cold.append(dict(source=p,bytes=4,hex=raw.hex(),access='mutable-exclusive-cold'))
    report=json.loads(args.manifest.read_text())
    report.update(configuration_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        cold_mutable_globals=cold,native_configuration_or_recovery_owner=False,
        full_configuration_return_supplied=False)
    args.manifest.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(bodies=len(entries),instructions=len(instructions))))


if __name__=='__main__':main()
