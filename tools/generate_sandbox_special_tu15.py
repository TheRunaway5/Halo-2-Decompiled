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
ENTRIES = (0x1f39d0, 0x1f3b80)


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
