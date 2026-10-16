#!/usr/bin/env python3
"""Lower unchanged TU1.5 recorded-animation storage and object query bodies.

The original startup registration and constructor vtable select the roots.
All direct callees retain their whole CFG. No pool pointer, record, allocation
result or query return is supplied by this module.
"""
import argparse
import json
import re
import struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ROOTS = {0x2035b0, 0x2035f0, 0x203610, 0x2039b0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    registration = struct.unpack('<9I', image.read(0x44b68c, 36))
    assert registration == (0x2035b0, 0x31d400, 0x2035f0, 0x203610, 0, 0, 0, 0, 0)
    assert image.read(0x4621e8, 20) == b'recorded animations\0'
    # Original arena object's vtable selects this complete allocation body.
    assert struct.unpack('<I', image.read(0x45f1d8, 4))[0] == 0x127210
    entries = ROOTS | {0x127210}
    while True:
        bodies = {entry: _trace(code, entry, entries) for entry in entries}
        additional = set().union(*(set(body.calls) for body in bodies.values())) - entries
        assert all(code.inside(entry) for entry in additional)
        if not additional:
            break
        entries.update(additional)
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry, body in sorted(bodies.items()):
        assert body.code and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == instruction.bytes
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{entry:08x}': entry for entry in entries}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{pc:08x}': value for pc, value in addresses.items()},
                  routines=routines)
    args.directory.mkdir(parents=True, exist_ok=False)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory / 'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
                                            'tools/generate_recorded_animation_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_recorded_animation_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_recorded_animation_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_recorded_animation_tu15_owns_entry')
    source, count = re.subn(r'bool source_recorded_animation_tu15_owns_entry\(std::uint32_t p\) \{[^\n]+',
                            'bool source_recorded_animation_tu15_owns_entry(std::uint32_t p) {return ' +
                            ' || '.join(f'p==0x{entry:x}u' for entry in sorted(ROOTS)) + ';}', source)
    assert count == 1
    args.output.write_text(source)
    report_path = args.directory / 'generation.json'
    report = json.loads(report_path.read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
                  registration_address=0x44b68c, registration=list(registration),
                  constructor_vtable=0x45f1d8, exported_entries=sorted(ROOTS),
                  scope=__doc__, full_weapon_parity=False)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete recorded-animation bodies:', len(entries), 'instructions:', len(instructions))


if __name__ == '__main__':
    main()
