#!/usr/bin/env python3
"""Execute unchanged TU1.5 15CEF0 ->15CB60 with owned choices and rows."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from audit_rendering import Cache, file_sha
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EFLAGS, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_EDX


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    sites = set(complete_body(code, 0x15cb60).code) | set(complete_body(code, 0x15cef0).code)
    assert struct.unpack('<2I', image.read(0x15cfa8, 8)) == (0x15cefe, 0x15cf16)
    assert image.read(0x15cf0d, 5).hex() == 'e84efcffff'
    assert image.read(0x15cf25, 5).hex() == 'e836fcffff'
    covered, maps = set(), []
    for name in ['coagulation', 'headlong', 'zanzibar']:
        cache = Cache(Path('assets/disc/maps') / (name + '.map'))
        _, globals_at = cache.tag(next(t['datum'] for t in cache.tags.values() if t['group'] == 'matg'), 'matg')
        _, multiplayer_at = cache.tag(cache.u32(globals_at + 0x16c), 'mulg')
        runtime = cache.block(multiplayer_at + 8, 0x568)[0]
        choices = cache.block(runtime + 0x70, 8)
        raw_choices = bytes(cache.data[choices[0]:choices[0] + len(choices) * 8])
        _, scenario = cache.tag(cache.scenario, 'scnr')
        cases = []
        for ordinal, row in enumerate(cache.block(scenario + 0x120, 0x90)):
            classification = cache.data[row + 0x14]
            if classification not in (1, 2):
                continue
            definition, variant = 0xffffffff, 0
            collection = cache.u32(row + 0x5c)
            if collection != 0xffffffff:
                _, collection_at = cache.tag(collection, 'vehc')
                items = cache.block(collection_at, 0x10)
                assert len(items) == 1 and cache.u32(items[0] + 4) == 0x76656869
                definition, variant = cache.u32(items[0] + 8), cache.u32(items[0] + 12)
            for mode in [0, 1, 2, 3, 4, 5, 7]:
                cpu = Uc(UC_ARCH_X86, UC_MODE_32)
                cpu.mem_map(0x10000, 0x800000)
                for section in image.sections:
                    cpu.mem_write(section.va, image.section_bytes(section))
                cpu.mem_map(0x61000000, 0x6000)
                cpu.mem_map(0x600ef000, 0x2000)
                cpu.mem_map(0x60300000, 0x1000)
                cpu.mem_write(0x4ecc1c, struct.pack('<I', 0x61000000))
                cpu.mem_write(0x6100016c, struct.pack('<I', 0xeeff0001))
                cpu.mem_write(0x4f0414, struct.pack('<I', 0x61001000))
                cpu.mem_write(0x61001018, struct.pack('<I', 0x61002000))
                cpu.mem_write(0x6100200c, struct.pack('<I', 0x61003000))
                cpu.mem_write(0x61003074, struct.pack('<I', 0x61004000))
                cpu.mem_write(0x61004000, raw_choices)
                cpu.mem_write(0x61005000, struct.pack('<I', variant))
                cpu.mem_write(0x4f3218, struct.pack('<I', 0x61005100))
                cpu.mem_write(0x61005309 + classification - 1, bytes([mode]))
                cpu.mem_write(0x600f0000, struct.pack('<2I', 0x60300000, classification))
                cpu.reg_write(UC_X86_REG_ESP, 0x600f0000)
                cpu.reg_write(UC_X86_REG_EAX, 0x61005000)
                cpu.reg_write(UC_X86_REG_EDX, definition)
                cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
                trace = []

                def instruction(uc, pc, size, _):
                    assert pc in sites and bytes(uc.mem_read(pc, size)) == image.read(pc, size)
                    trace.append(pc)
                    covered.add(pc)

                cpu.hook_add(UC_HOOK_CODE, instruction)
                cpu.emu_start(0x15cef0, 0x60300000, count=1000)
                assert cpu.reg_read(UC_X86_REG_EIP) == 0x60300000
                cases.append(dict(row=ordinal, classification=classification, parameter=75 + classification,
                    mode=mode, definition=cpu.reg_read(UC_X86_REG_EAX),
                    variant=struct.unpack('<I', cpu.mem_read(0x61005000, 4))[0], trace=trace,
                    position_words=list(struct.unpack_from('<3I', cache.data, row + 0x40)),
                    rotation_words=list(struct.unpack_from('<3I', cache.data, row + 0x4c))))
        maps.append(dict(map=name, cache_path=str(cache.path.resolve()), cache_sha256=file_sha(cache.path), cases=cases))
    args.output.write_text(json.dumps(dict(reference='OG Xbox TU1.5', xbe_sha256=NEW_PIN,
        xbe_path=str(NEW_DEFAULT.resolve()), shared_maps=str(Path('assets/disc/maps').resolve()),
        entry=0x15cef0, selector=0x15cb60, maps=maps, instructions=len(sites), covered_instructions=len(covered),
        oracle_sha256=file_sha(Path(__file__)), scope='Original fixed/default/none light vehicle selector. '
        'Owned globals and singleton collection inputs; no replaced calls or supplied outputs. '
        'Random choice, original constructors, live controls and physics require separate evidence.'), indent=2) + '\n')
    print(f'PASS {sum(len(m["cases"]) for m in maps)} unchanged TU1.5 light choices, {len(covered)}/{len(sites)} instructions')


if __name__ == '__main__':
    main()
