#!/usr/bin/env python3
"""Lower complete original TU1.5 lens request and retained pool owners."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES = (0x17c710, 0x3e9b0, 0x2db80, 0x2e210, 0x20990, 0x20b20, 0x20e30, 0x32cc0c, 0x134a50, 0x179a40, 0x179b40, 0x17cd10, 0x20680, 0x20700)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/source_lens_flare_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # Actual effect and direct-object callers request the same owner.
    calls = {0x12b4e:0x20680, 0x2b68c:0x20700, 0x20732:0x20e30, 0x40f14:0x179a40, 0x40f31:0x179b40, 0x179ac4:0x17cd10, 0x179abe:0x17c710, 0x179c38:0x17c710, 0x17cba6:0x3e9b0, 0x17cbb3:0x2db80, 0x41b0a:0x3e9b0,
             0x41b13:0x2db80, 0x2dcb9:0x2e210, 0x2dcc7:0x20990,
             0x2dcf7:0x20b20, 0x20a5d:0x20e30, 0x2e27d:0x32cc0c,
             0x2e3b8:0x134a50}
    for pc,target in calls.items():
        i=code.at(pc)
        assert i.mnemonic=='call' and i.operands[0].imm==target
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        if entry==0x17c710:
            assert {0x17caf9,0x17cba6,0x17cbb3} <= set(body.code)
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
                            'tools/generate_lens_flare_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_lens_flare_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_lens_flare_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_lens_flare_tu15_owns_entry')
    args.output.write_text(source)
    manifest = args.directory / 'generation.json'
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output),
                  module_generator_sha256=sha(Path(__file__)),
                  original_call_sites={f'{p:08x}':f'{q:08x}' for p,q in calls.items()},
                  boundary='Complete original request/pool bodies only; native pool scheduling, occlusion and final graphics consumption remain separate owners.')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete original lens request owners', len(ENTRIES),
          'instructions', len(instructions))


if __name__ == '__main__':
    main()
