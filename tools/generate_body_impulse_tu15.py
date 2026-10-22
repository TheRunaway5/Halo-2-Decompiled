#!/usr/bin/env python3
"""Lower the complete TU1.5 rigid-body impulse virtual methods reached by rockets."""
import argparse
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha, complete_body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('src/physics/source_body_impulse_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Use tools/run_test.py'
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    cooldown = 0x56e208
    section = next(section for section in image.sections
                   if section.va <= cooldown and cooldown + 4 <= section.va + section.vsize)
    assert section.name == '.data' and section.flags & 1
    assert cooldown >= section.va + section.rsize and image.read(cooldown, 4) == bytes(4)
    assert code.at(0x151c1a).bytes.hex() == '3b2d08e25600'
    assert code.at(0x151c82).bytes.hex() == '892d08e25600'
    assert code.at(0x1d93fa).bytes.hex() == 'ff5270'
    assert code.at(0x1d94ce).bytes.hex() == 'ff5060'
    for table in (0x41f2e0, 0x41f360):
        assert int.from_bytes(image.read(table + 0x70, 4), 'little') == 0x324e60
        assert int.from_bytes(image.read(table + 0x60, 4), 'little') == 0x324f80
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry, count in ((0x324e60, 12), (0x324f80, 79)):
        body = complete_body(code, entry)
        assert len(body.code) == count and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            assert instruction.mnemonic != 'call'
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={entry: int(entry, 16) for entry in sites}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{pc:08x}': value for pc, value in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    manifest = args.directory / 'generation.json'
    generate(SimpleNamespace(output=args.output, manifest=manifest,
                             correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py', 'tools/generate_body_impulse_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp', 'halo2/source_body_impulse_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_body_impulse_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry', 'source_body_impulse_tu15_owns_entry')
    args.output.write_text(source)
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output), generator_sha256=sha(Path(__file__)),
                  observed_virtual_call='001d93fa', complete_instruction_count=91,
                  fallback_spawn_timestamp=dict(address=cooldown, bytes=4, initial='00000000',
                                               owner='original loader-zeroed .data,151BF0 stores'))
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    main()
