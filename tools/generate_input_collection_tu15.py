#!/usr/bin/env python3
"""Lower complete TU1.5 command collection, continuation and input mapping."""
import argparse
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# Target collection1AD4F0/1AF490 remains in its separately verified native
# module. All other already compiled descendants remain required-owner calls.
BODY_COUNTS = {
    0x1889d0: 999, 0x1898a0: 429, 0xcae40: 49, 0x188920: 54,
    0x18a920: 213, 0x14bb3c: 76, 0x149e96: 9, 0x193ed6: 22,
    0x23bc5b: 12, 0x21e650: 558, 0x193a4c: 82, 0x21ef20: 51,
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
        default=Path('src/input/source_input_collection_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{entry:08x}': entry for entry in BODY_COUNTS}, body_sites={},
        instruction_map={}, address_map={}, routines=[])
    args.directory.mkdir(parents=True, exist_ok=True)
    repeated = []
    for entry, count in BODY_COUNTS.items():
        body = _trace(code, entry, set(BODY_COUNTS))
        assert len(body.code) == count
        assert body.tail_jumps == ({0x1060e0} if entry == 0xcae40 else set())
        assert all(width in (1, 4) for _, width, _ in body.tables)
        paired['body_sites'][f'{entry:08x}'] = sorted(body.code)
        paired['routines'].append(dict(entry=entry,
            tables=[dict(disc=at, count=n) for at, width, n in body.tables if width == 4],
            byte_tables=[dict(address=at, width=width, count=n, raw_hex=image.read(at, width*n).hex())
                         for at, width, n in body.tables if width == 1]))
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
            if instruction.mnemonic in ('rep movsd', 'rep stosd', 'rep movsb', 'rep stosb'):
                repeated.append(pc)
            listing.append(f'{pc:08x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
        (args.directory / f'{entry:08x}.asm').write_text(''.join(listing))
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, sort_keys=True, indent=2) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py', 'tools/generate_input_collection_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_input_collection_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_input_collection_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_input_collection_tu15_owns_entry')
    corrections = []
    for pc in repeated:
        old = f'--s.gp[1];if(s.services.instruction)s.services.instruction(0x{pc:x}u);'
        new = f'--s.gp[1];if(s.gp[1]&&s.services.instruction)s.services.instruction(0x{pc:x}u);'
        assert source.count(old) == 1
        source = source.replace(old, new, 1)
        corrections.append(dict(instruction=pc, old=old, new=new,
            scope='Omit only the terminal zero-count observation event; instruction results/flags/writes are unchanged'))
    temporary = args.output.with_name(args.output.name + '.tmp')
    temporary.write_text(source)
    os.replace(temporary, args.output)
    report = json.loads(manifest.read_text())
    report['generated_cpp_sha256'] = sha(args.output)
    report['hardware_rep_observer'] = corrections
    manifest.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')


if __name__ == '__main__':
    main()
