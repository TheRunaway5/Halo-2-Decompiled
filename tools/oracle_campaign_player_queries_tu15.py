#!/usr/bin/env python3
"""Execute unchanged TU1.5 players() and cooperative-query owners.

Raw options/player/list pools are declared component inputs, not a cooperative
session. Original list/reference allocation and slot iteration execute without
callee stubs or instruction patches. The cooperative evaluator stops at its
actual HS result publisher, without supplying that owner's result.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, STOP = 0x2000000, 0x3000000
PLAYERS, ROWS, BITS = BASE+0x1000, BASE+0x2000, BASE+0x5000
HEADERS, HEADER_ROWS, HEADER_BITS = BASE+0x6000, BASE+0x6100, BASE+0x6400
REFERENCES, REFERENCE_ROWS, REFERENCE_BITS = BASE+0x7000, BASE+0x7100, BASE+0x7800
GAME, STACK = BASE+0x8000, BASE+0x3f000


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    if not args.output.resolve().is_relative_to(scratch):
        raise ValueError('Generated oracle output must be inside HALO2_TEST_SCRATCH')
    args.output.mkdir(parents=True, exist_ok=False)
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    assert image.read(0x456cf0, 14).hex() == '1f00000020aa2a00000000000000'
    assert image.read(0x4596e8, 14).hex() == '0500000090372b00000000000000'
    bodies = {}
    for entry in (0x2aaa20, 0x2a9190, 0x1e7010, 0x16e6a0, 0x16ea60, 0x2b3790):
        body = _trace(code, entry, {entry})
        bodies[f'{entry:08x}'] = {f'{pc:08x}': bytes(code.at(pc).bytes).hex()
                                 for pc in sorted(body.code)}
    visited = set()

    def guest():
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x40000)
        g.mem_map(STOP, 4096)
        g.hook_add(UC_HOOK_CODE, lambda _g, pc, _n, _u: visited.add(pc))
        return g

    def put(g, p, value, width=4):
        g.mem_write(p, (value & ((1 << (8*width))-1)).to_bytes(width, 'little'))

    def word(g, p):
        return struct.unpack('<I', g.mem_read(p, 4))[0]

    def pool(g, p, rows, bitmap, capacity, stride, salt):
        put(g, p+0x20, capacity)
        put(g, p+0x24, stride)
        put(g, p+0x29, 1, 1)
        put(g, p+0x2a, 4, 1)
        put(g, p+0x40, salt, 2)
        put(g, p+0x44, rows)
        put(g, p+0x48, bitmap)

    def run(g, entry, until=STOP):
        put(g, STACK, STOP)
        for offset, value in ((4, 0x239), (8, 0x12345678), (12, 1)):
            put(g, STACK+offset, value)
        g.reg_write(R.UC_X86_REG_ESP, STACK)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        g.emu_start(entry, until, count=100000)
        assert g.reg_read(R.UC_X86_REG_EIP) == until
        return g.reg_read(R.UC_X86_REG_EAX)

    cooperative = []
    for mode in (0, 1, 2, 0xffffffff, 0x80000000):
        for flag in (0, 1, 2, 128, 255):
            g = guest()
            put(g, 0x4f3218, GAME)
            put(g, GAME+8, mode)
            put(g, GAME+0x134, flag, 1)
            result = run(g, 0x2b3790, 0x211da0)
            assert g.reg_read(R.UC_X86_REG_ECX) == 0x12345678
            cooperative.append(dict(mode=mode, flag=flag, result=result))

    masks = [0, 0xffff, 0x5555, 0xaaaa, *[1 << i for i in range(16)]]
    seed = 0x12345678
    for _ in range(16):
        seed = (1664525*seed+1013904223) & 0xffffffff
        masks.append(seed & 0xffff)
    specs = [(mask, 16, variant) for mask in masks for variant in range(4)]
    specs += [(0xffff, high_water, 3) for high_water in range(17)]
    players = []
    for mask, high_water, variant in specs:
        g = guest()
        pool(g, PLAYERS, ROWS, BITS, 16, 0x21c, 0xec70)
        pool(g, HEADERS, HEADER_ROWS, HEADER_BITS, 48, 12, 0xe26f)
        pool(g, REFERENCES, REFERENCE_ROWS, REFERENCE_BITS, 128, 12, 0xe26f)
        put(g, 0x4f54f4, PLAYERS)
        put(g, 0x5020b0, HEADERS)
        put(g, 0x5020ac, REFERENCES)
        put(g, PLAYERS+0x38, high_water)
        put(g, BITS, mask)
        units = []
        for slot in range(16):
            unit = ((0x8100+slot) << 16) | slot
            if variant == 1 and slot % 3 == 0:
                unit = 0xffffffff
            elif variant == 2:
                unit = 0x92340005  # Original preserves duplicate current units.
            elif variant == 3:
                unit = (0, 0xffffffff, 0x80120009, 0x12345678)[slot % 4]
            units.append(unit)
            put(g, ROWS+slot*0x21c, 0xec70+slot, 2)
            put(g, ROWS+slot*0x21c+2, (0, 1, 2, 0xffff)[slot % 4], 2)
            put(g, ROWS+slot*0x21c+0x2c, unit)
        result = run(g, 0x2a9190)
        assert g.reg_read(R.UC_X86_REG_ESP) == STACK+4
        assert result == 0xe26f0000
        count = int.from_bytes(g.mem_read(HEADER_ROWS+6, 2), 'little')
        objects, next_ref = [], word(g, HEADER_ROWS+8)
        while next_ref != 0xffffffff:
            assert len(objects) < 128
            row = REFERENCE_ROWS+(next_ref & 0xffff)*12
            objects.append(word(g, row+4))
            next_ref = word(g, row+8)
        assert count == len(objects)
        players.append(dict(mask=mask, high_water=high_water, units=units,
            result=result, objects=objects, header=bytes(g.mem_read(HEADER_ROWS, 12)).hex(),
            references=bytes(g.mem_read(REFERENCE_ROWS, count*12)).hex()))
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        instruction_patches=[], supplied_dependency_returns=[],
        cooperative=cooperative, players=players,
        original_bodies=bodies)
    encoded = json.dumps(fixture, indent=2, sort_keys=True)+'\n'
    (args.output/'original.json').write_text(encoded)
    if args.verify:
        assert args.fixture.read_text() == encoded, 'Original replay differs from the retained fixture'
    else:
        if args.fixture.exists():
            raise ValueError('Preserve the existing original fixture; use --verify')
        args.fixture.write_text(encoded)
    print(json.dumps(dict(cooperative_cases=len(cooperative), player_cases=len(players),
        executed_original_sites=len(visited), fixture_sha256=hashlib.sha256(encoded.encode()).hexdigest(),
        instruction_patches=0, supplied_dependency_returns=0)))


if __name__ == '__main__':
    main()
