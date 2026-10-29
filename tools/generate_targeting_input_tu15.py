#!/usr/bin/env python3
"""Lower the complete TU1.5 local weapon-target producer and position helper."""
import argparse
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha


ENTRIES = (0x1ad4f0, 0x1af490)
BODY_INSTRUCTIONS = (208, 129)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
        default=Path('src/gameplay/units/source_targeting_input_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in ENTRIES}, body_sites={},
        instruction_map={}, address_map={}, routines=[])
    args.directory.mkdir(parents=True, exist_ok=True)
    for entry, count in zip(ENTRIES, BODY_INSTRUCTIONS):
        body = _trace(code, entry, set(ENTRIES))
        assert len(body.code) == count and not body.tables and not body.tail_jumps
        paired['body_sites'][f'{entry:08x}'] = sorted(body.code)
        paired['routines'].append(dict(entry=entry, tables=[], byte_tables=[]))
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            paired['instruction_map'][f'{pc:08x}'] = pc
            for at in (pc, pc + instruction.size):
                paired['address_map'][f'{at:08x}'] = at
            for operand in instruction.operands:
                if operand.type in (2, 3):
                    at = (operand.imm if operand.type == 2 else operand.mem.disp) & 0xffffffff
                    paired['address_map'][f'{at:08x}'] = at
            listing.append(f'{pc:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
        (args.directory / f'{entry:08x}.asm').write_text(''.join(listing))
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, sort_keys=True, indent=2) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py', 'tools/generate_targeting_input_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_targeting_input_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_targeting_input_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_targeting_input_tu15_owns_entry')
    temporary = args.output.with_name(args.output.name + '.tmp')
    temporary.write_text(source)
    os.replace(temporary, args.output)
    report = json.loads(manifest.read_text())
    report['generated_cpp_sha256'] = sha(args.output)
    manifest.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')


if __name__ == '__main__':
    main()
