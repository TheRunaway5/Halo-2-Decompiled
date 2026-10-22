#!/usr/bin/env python3
"""Lower complete original TU1.5 lens query geometry and completion owners."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ENTRIES = (0x20750, 0x20b90, 0x20d60, 0x2e520, 0x48af0, 0x3803b0,
           0x40b080, 0x405560, 0x32b199, 0x32ece9, 0x32ea95, 0x32eae8,
           0x32eb88, 0x32ebc3, 0x32cc0c, 0x20e30,
           0xc1e90, 0x402650, 0x4026b0, 0x402710)


class QueryCode:
    def __init__(self, image):
        self.sections = [_Code(image, '.text'), _Code(image, 'D3D')]
        self.never = set()
    def inside(self, address):
        return any(code.inside(address) for code in self.sections)
    def owner(self, address):
        return next(code for code in self.sections if code.inside(address))
    def at(self, address):
        return self.owner(address).at(address)
    def dword(self, address):
        return self.owner(address).dword(address)
    def byte(self, address):
        return self.owner(address).byte(address)
    def boundary(self, address):
        return self.owner(address).boundary(address)
    def prologue(self, address):
        return self.owner(address).prologue(address)



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/rendering/effects/source_lens_query_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path, required=True)
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = QueryCode(image)
    # Direct original callers and the authentic function-pointer callback.
    calls = {0x2c2f7:0x20750, 0x2ded4:0x20b90, 0x2e183:0x20b90,
             0x3c369:0x20b90, 0x2ea75:0x20d60, 0x2e6d9:0x48af0,
             0x2e811:0x40b080, 0x2e996:0x405560, 0x2e740:0x32b199,
             0x32b1ac:0x32ece9, 0x32b1ca:0x32ebc3, 0x32b1ea:0x32ea95,
             0x32b21f:0x32eb88, 0x32b260:0x32eae8, 0x2089c:0x32cc0c,
             0x20948:0x20e30, 0x20c87:0xc1e90, 0x20c9b:0x402710,
             0x20820:0x402710, 0x20cef:0x4026b0, 0x4026b1:0x402650}
    callback = code.at(0x2dec8)
    assert callback.mnemonic == 'push' and callback.operands[0].imm == 0x2e520
    assert int.from_bytes(image.read(0x46d818,4),'little') == 0x3803b0
    for pc,target in calls.items():
        i=code.at(pc)
        assert i.mnemonic=='call' and i.operands[0].imm==target
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
        additional_entries=Path(__file__)), correspondence, code_override=code)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py',
                            'tools/generate_lens_query_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_lens_query_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_lens_query_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_lens_query_tu15_owns_entry')
    args.output.write_text(source)
    manifest = args.directory / 'generation.json'
    report = json.loads(manifest.read_text())
    report.update(generated_cpp_sha256=sha(args.output),
                  module_generator_sha256=sha(Path(__file__)),
                  original_call_sites={f'{p:08x}':f'{q:08x}' for p,q in calls.items()},
                  query_callback_pointer=dict(site='0002dec8',entry='0002e520'),
                  initializer_pointer=dict(site='0046d818',entry='003803b0'),
                  boundary='Complete original quad/projection/completion arithmetic. Native view/presentation/GPU query services are separate real resource owners; no visibility result is supplied by this generator.')
    manifest.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete original lens query owners', len(ENTRIES),
          'instructions', len(instructions))


if __name__ == '__main__':
    main()
