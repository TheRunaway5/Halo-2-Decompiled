#!/usr/bin/env python3
"""Lower complete original TU1.5 static geometry destructor paths."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# The three real383420/383440/383460 registrations name these callbacks.
# Preserve their complete shared destructors, including conditional releases.
# Actual allocator/virtual callees remain explicit required native owners.
ENTRIES = (0x383bf0, 0x383c00, 0x383c10, 0x31f660, 0x31f810,
           0x325af0, 0x315d10, 0x283450)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--output', type=Path,
                        default=Path('src/physics/source_havok_static_lifetime_tu15_generated.cpp'))
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    for site, callback in ((0x38342f, 0x383c10), (0x38344f, 0x383c00), (0x38346f, 0x383bf0)):
        assert code.at(site).mnemonic == 'call' and code.at(site).operands[0].imm == 0x32b0b4
        assert code.at(site - 5).mnemonic == 'push' and code.at(site - 5).operands[0].imm == callback
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in sorted(ENTRIES):
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n, raw_hex=image.read(p, width*n).hex())
                         for p, width, n in body.tables if width == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_havok_static_lifetime_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_havok_static_lifetime_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_havok_static_lifetime_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_havok_static_lifetime_tu15_owns_entry')
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)))
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete source static geometry lifetime owners', len(ENTRIES), 'instructions', len(instructions))


if __name__ == '__main__':
    main()
