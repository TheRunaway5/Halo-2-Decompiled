#!/usr/bin/env python3
"""Lower complete unchanged TU1.5 sandbox special-action owners to native C++."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# Actual1ED997 reaches1F39D0 during the source-selected distant sword action.
# Its complete movement consumer calls1F3B80. Existing world owners retain
# their own direct callees; no target, physics result or service is supplied.
ENTRIES = (0x1f39d0, 0x1f3b80, 0x10f080, 0x30bd0,
           # Actual1F2553 records the source lunge contact. Preserve its
           # real recording gates, event identity, clock and ring consumers.
           0x19bb60, 0x19b8f0, 0x19de80, 0x19bb20,
           # Actual1F2565 continues the same lunge contact through retained
           # contact identity/history and its complete source consumers.
           0x1f2890, 0x1f1fc0, 0x1f20d0, 0x19b860, 0x19b2b0,
           # Actual1F2740 completes source lunge damage notifications through
           # the original game-engine/recording/attribution owner.
           0x15f8b0, 0x25bc90,
           # Literal Slayer slot78 reached by1600E4 after actual lunge damage.
           0x2c5dd0, 0x15e1e0, 0x161530, 0x248550, 0x19b7d0,
           # Actual1F2879 completes the original contact notification tail.
           0x1f2a50, 0x22c280, 0x165670,
           # Original TU1.5 animation event116BDF (legacy observer115CBF)
           # reaches the sound bit-bank consumer and its direct callees.
           0x213890, 0x21fe40, 0x140ea0,
           # Literal startup/map callback record44B524 owns the sound bit
           # bank. The XAPI wrapper retains its real native memory boundary.
           0x2131b0, 0x213260, 0x33b3ac,
           # Original116DDF (legacy observer115EBF) resolves the animation
           # event's authored sound metadata through the real ugh owner.
           0x116cd0, 0x213690, 0x21f670, 0x21f6b0,
           # The real contact collector at2F8B27 invokes literal41CA30+4.
           0x30b980,
           # ActualE4DAB finishes the source action's physical transition.
           0xe31c0, 0x1dd3d0, 0x1cc310)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/source_sandbox_special_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path,
                        default=Path('analysis/sandbox-tu15-goal-20261013/special-functions'))
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x1ed997).mnemonic == 'call'
    assert code.at(0x1ed997).operands[0].imm == 0x1f39d0
    assert code.at(0x1f2553).mnemonic == 'call'
    assert code.at(0x1f2553).operands[0].imm == 0x19bb60
    assert code.at(0x1f2565).mnemonic == 'call'
    assert code.at(0x1f2565).operands[0].imm == 0x1f2890
    assert code.at(0x1f2740).mnemonic == 'call'
    assert code.at(0x1f2740).operands[0].imm == 0x15f8b0
    # Original15FD63 calls the selected engine's literal slot7C. This is
    # the actual original two-instruction body, not a supplied host return.
    assert int.from_bytes(image.read(0x468be8+0x7c, 4), 'little') == 0x25bc90
    assert int.from_bytes(image.read(0x468be8+0x78, 4), 'little') == 0x2c5dd0
    assert code.at(0x1f2766).mnemonic == 'call'
    assert code.at(0x1f2766).operands[0].imm == 0x19b7d0
    assert code.at(0x1f2879).mnemonic == 'call'
    assert code.at(0x1f2879).operands[0].imm == 0x1f2a50
    assert code.at(0x116bdf).mnemonic == 'call'
    assert code.at(0x116bdf).operands[0].imm == 0x213890
    assert image.read(0x44b524, 12).hex() == 'b03121002032210060322100'
    assert int.from_bytes(image.read(0x41b6d4, 4), 'little') == 0x800000b8
    assert code.at(0x116ddf).mnemonic == 'call'
    assert code.at(0x116ddf).operands[0].imm == 0x116cd0
    assert code.at(0x2f8b27).bytes.hex() == 'ff5004'
    assert int.from_bytes(image.read(0x41ca34, 4), 'little') == 0x30b980
    assert code.at(0xe4dab).mnemonic == 'call'
    assert code.at(0xe4dab).operands[0].imm == 0xe31c0
    sites, instructions, addresses, routines = {}, {}, {}, []
    args.directory.mkdir(parents=True, exist_ok=True)
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n,
                raw_hex=image.read(p, width*n).hex())
                for p, width, n in body.tables if width == 1]))
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            listing.append(f'{pc:08x} {instruction.bytes.hex()} '
                           f'{instruction.mnemonic} {instruction.op_str}\n')
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
        (args.directory/f'{entry:08x}.asm').write_text(''.join(listing))
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': value for p, value in addresses.items()}, routines=routines)
    correspondence = args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py',
                            'tools/generate_sandbox_special_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_sandbox_special_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_sandbox_special_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_sandbox_special_tu15_owns_entry')
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report['generated_cpp_sha256'] = sha(args.output)
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
