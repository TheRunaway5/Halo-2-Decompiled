#!/usr/bin/env python3
"""Lower complete original TU1.5 particle-view target and viewport selectors."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from generate_lens_query_tu15 import QueryCode
from generate_physics_live_support_tu15 import generate, sha
from oracle_smg_firing_1_1 import Xbe, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ENTRIES = (0x14ba0, 0x1cd10, 0x1dcd0, 0x1dd00, 0x25940)
CHAIN = ((0x132b0, 0x1337a, 0x14ba0),
         (0x14ba0, 0x14d22, 0x1cd10),
         (0x14ba0, 0x14c1c, 0x1dcd0),
         (0x14ba0, 0x14c7f, 0x1dd00),
         (0x14ba0, 0x14d9a, 0x25940))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/source_particle_target_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = QueryCode(image)
    #132B0 is authenticated by the ordinary2B9F0 containing body, not the
    # discarded interior132D0 label. Follow every new owner from that root.
    assert 0x2bbfb in _trace(code, 0x2b9f0, {0x2b9f0}).code
    caller = code.at(0x2bbfb)
    assert caller.mnemonic == 'call' and caller.operands[0].imm == 0x132b0
    for owner, site, target in CHAIN:
        assert site in _trace(code, owner, {owner}).code
        call = code.at(site)
        assert call.mnemonic == 'call' and call.operands[0].imm == target
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n,
                             raw_hex=image.read(p, width*n).hex())
                         for p, width, n in body.tables if width == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=False)
    correspondence = args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
             correspondence=correspondence, additional_entries=Path(__file__)),
             paired, code_override=code)
    source = args.output.read_text().replace(
        'tools/generate_physics_live_support_tu15.py', 'tools/generate_particle_target_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_particle_target_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15', 'source_run_particle_target_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_particle_target_tu15_owns_entry')
    regions = ((0x14eb0, 24*4), (0x14f10, 39))
    source += '\nnamespace halo2 {\nvoid mount_source_particle_target_data_tu15(SourceRuntimeMemory &memory) {\n'
    for address, size in regions:
        raw = image.read(address, size)
        source += ' {constexpr std::array<std::byte,'+str(size)+'> bytes{'+','.join(
            'std::byte{0x%02x}' % value for value in raw)+'};\n'
        source += ' memory.mount(0x%xu,bytes,SourceRuntimeMemory::Access::ReadOnly,"original TU1.5 target selector table");}\n' % address
    source += '}\n}\n'
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)),
                  authenticated_caller_chain=[dict(owner=p, site=q, target=r) for p, q, r in CHAIN],
                  original_table_regions=[dict(address=p, bytes=n,
                    sha256=__import__('hashlib').sha256(image.read(p, n)).hexdigest()) for p, n in regions],
                  boundary='Original target/viewport CPU selection only. Native attachment binding, SDK headers, physical allocations and GPU consumption remain explicit owners.')
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print('Complete target selectors', len(ENTRIES), 'instruction sites', len(instructions))


if __name__ == '__main__':
    main()
