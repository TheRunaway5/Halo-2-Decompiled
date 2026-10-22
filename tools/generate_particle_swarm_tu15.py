#!/usr/bin/env python3
"""Lower the complete owned TU1.5 swarm2 particle controller and helpers."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES = (0x2c33a0, 0x2c3480, 0x2c35e0, 0x2c3730, 0x2c3840)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/rendering/effects/source_particle_swarm_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # The real type2 dispatch and obstacle query identify these owners.
    assert code.at(0x2c2b17).operands[0].imm == 0x2c3840
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and not body.tables
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry, tables=[], byte_tables=[]))
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
    correspondence = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    bodies = args.directory / 'bodies.json'
    bodies.write_text(json.dumps(correspondence, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output,
        manifest=args.directory / 'generation.json', correspondence=bodies,
        additional_entries=Path(__file__)), correspondence)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py',
                            'tools/generate_particle_swarm_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_particle_swarm_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_particle_swarm_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_particle_swarm_tu15_owns_entry')
    args.output.write_text(source)
    manifest = args.directory / 'generation.json'
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output),
                  module_generator_sha256=sha(Path(__file__)))
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete original swarm owners', len(ENTRIES),
          'instructions', len(instructions))


if __name__ == '__main__':
    main()
