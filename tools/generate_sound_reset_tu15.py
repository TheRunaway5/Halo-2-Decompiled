#!/usr/bin/env python3
"""Lower the complete unchanged TU1.5 APU reset and CPU cleanup bodies.

Control accesses are admitted by a separate real native resource owner.
The generated code supplies no register image, device readiness or DSP result.
"""
import argparse
import hashlib
import json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--manifest', type=Path, required=True)
    args = parser.parse_args()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, 'DSOUND')
    entries = [0x38ec0e, 0x38eb16, 0x38e53d, 0x38e574, 0x38e5ab,
               0x38e606, 0x38f3d4, 0x38f395, 0x383d70, 0x383d92]
    bodies = {entry: _trace(code, entry, set(entries)) for entry in entries}
    assert not any(body.tables for body in bodies.values())
    instructions, addresses = {}, {}
    for pc in sorted(set().union(*(body.code for body in bodies.values()))):
        op = code.at(pc)
        assert bytes(op.bytes) == image.read(pc, op.size)
        instructions[f'{pc:08x}'] = pc
        addresses[pc] = pc
        addresses[pc + op.size] = pc + op.size
        for operand in op.operands:
            if operand.type == 2:
                addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
            if operand.type == 3:
                addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{entry:08x}': entry for entry in entries},
                  body_sites={f'{entry:08x}': sorted(body.code) for entry, body in bodies.items()},
                  instruction_map=instructions,
                  address_map={f'{pc:08x}': value for pc, value in addresses.items()},
                  routines=[dict(entry=entry, tables=[], byte_tables=[]) for entry in entries])
    args.correspondence = args.manifest.with_name('sound-reset-bodies.json')
    args.correspondence.parent.mkdir(parents=True, exist_ok=True)
    args.correspondence.write_text(json.dumps(paired, sort_keys=True, indent=2) + '\n')
    args.additional_entries = Path(__file__)
    generate(args, paired, complete_listings=True, code_override=code)
    text = args.output.read_text().replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_sound_reset_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_sound_reset_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_sound_reset_tu15_owns_entry')
    args.output.write_text(text)
    report = json.loads(args.manifest.read_text())
    report.update(sound_reset_generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.manifest.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(bodies=len(entries), instructions=len(instructions))))


if __name__ == '__main__':
    main()
