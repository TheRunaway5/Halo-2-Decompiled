#!/usr/bin/env python3
"""Execute the complete TU1.5 placement-settings selector and inherited lookup.

The actual Cairo1E8BBF caller reaches20B980. Raw character/options records are
declared component inputs;1ECC80, the switch and every original store execute
unchanged. No instruction patch, callee stub or dependency result is supplied.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, STOP = 0x2000000, 0x3000000
TAGS, GAME, OUT, STACK = BASE+0x1000, BASE+0x2000, BASE+0x3000, BASE+0xf000
ROWS = [BASE+0x4000+i*0x100 for i in range(3)]
SETTINGS = BASE+0x5000
GP = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX,
      R.UC_X86_REG_EBX, R.UC_X86_REG_ESP, R.UC_X86_REG_EBP,
      R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--fixture', type=Path, required=True)
    ap.add_argument('--verify', action='store_true')
    args = ap.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        ap.error('Run through tools/run_test.py')
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    assert image.read(0x1e8bbf, 5).hex() == 'e8bc2d0200'
    code = _Code(image, '.text')
    sites = {pc for entry in (0x20b980, 0x1ecc80)
             for pc in _trace(code, entry, {entry}).code}
    # The only immutable data dependency is the actual four-way switch.
    table = image.read(0x20ba30, 16)
    cases, covered = [], set()
    specs = [(1, d, s, lookup, debug) for d in range(4)
             for s in (-1, 0, 1, 2, 3, 4, 5, 0x10003)
             for lookup in (-1, 0, 2, 3) for debug in (0, 1)]
    specs += [(mode, 3, s, 3, debug) for mode in (0, 2)
              for s in (1, 2, 3, 5) for debug in (0, 255)]
    for mode, difficulty, selector, lookup, debug in specs:
        g = Uc(UC_ARCH_X86, UC_MODE_32)
        g.mem_map(0x10000, 0x600000)
        for section in image.sections:
            g.mem_write(section.va, image.section_bytes(section))
        g.mem_map(BASE, 0x10000)
        g.mem_map(STOP, 4096)
        patches = []

        def put(address, value, width=4):
            data = (value & ((1 << (width*8))-1)).to_bytes(width, 'little')
            g.mem_write(address, data)
            patches.append([address, data.hex()])

        put(0x4f0414, TAGS)
        put(0x4f3218, GAME)
        put(GAME+8, mode)
        put(GAME+0x132, difficulty, 2)
        put(0x5020b7, debug, 1)
        for index, row in enumerate(ROWS):
            put(TAGS+index*16+8, row)
            put(row+8, (0x81010000 | (index+1)) if index<2 else 0xffffffff)
            put(row+0x44, 0xffffffff if lookup==0 else int(index==(0 if lookup==2 else 2)))
            put(row+0x48, SETTINGS)
        for group in range(3):
            for d in range(4):
                # Distinguish every table and difficulty with exact bit values.
                value = (0x3f000001, 0, 0xc0200000, 0x7fc12345)[d] if group==2 else 0x3f800001+group*0x100+d
                put(SETTINGS+4+group*16+d*4, value)
        put(OUT, 0x88776655)
        put(OUT+4, 0x44332211)
        put(OUT+8, 0x7fa12345)
        put(STACK, STOP)
        put(STACK+4, selector)
        put(STACK+8, OUT+4)
        initial = [0xaabbccdd, 0xffffffff if lookup==-1 else 0x815a0000,
                   0x12345678, 0x87654321, STACK, 0x44556677, OUT, OUT+8]
        for reg, value in zip(GP, initial):
            g.reg_write(reg, value)
        xmm = [[0x3f700000+i*4+lane for lane in range(4)] for i in range(8)]
        for index, lanes in enumerate(xmm):
            g.reg_write(R.UC_X86_REG_XMM0+index, int.from_bytes(struct.pack('<4I', *lanes), 'little'))
        g.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        g.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        g.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        trace, stores = [], []

        def instruction(_g, pc, _size, _user):
            assert pc in sites, hex(pc)
            trace.append(pc)
            covered.add(pc)

        def store(_g, _access, address, size, value, _user):
            stores.append([g.reg_read(R.UC_X86_REG_EIP), address,
                           bytes(g.mem_read(address, size)).hex(),
                           (value & ((1 << (size*8))-1)).to_bytes(size, 'little').hex()])

        g.hook_add(UC_HOOK_CODE, instruction)
        g.hook_add(UC_HOOK_MEM_WRITE, store)
        g.emu_start(0x20b980, STOP, count=1000)
        assert g.reg_read(R.UC_X86_REG_EIP) == STOP
        cases.append(dict(mode=mode, difficulty=difficulty, selector=selector,
            lookup=lookup, debug=debug, patches=patches, gp=initial, xmm=xmm,
            final_gp=[g.reg_read(reg) for reg in GP],
            final_xmm=[list(struct.unpack('<4I', g.reg_read(R.UC_X86_REG_XMM0+i).to_bytes(16, 'little'))) for i in range(8)],
            final_flags=g.reg_read(R.UC_X86_REG_EFLAGS), trace=trace, stores=stores,
            output=bytes(g.mem_read(OUT, 16)).hex()))
    assert covered == sites, [hex(p) for p in sorted(sites-covered)]
    fixture = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
        instruction_patches=[], supplied_dependency_returns=[],
        instructions={f'{pc:08x}': bytes(code.at(pc).bytes).hex() for pc in sorted(sites)},
        switch_table=[0x20ba30, table.hex()], cases=cases)
    data = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if args.verify:
        assert args.fixture.read_bytes() == data, 'Original placement settings changed'
    else:
        args.fixture.parent.mkdir(parents=True, exist_ok=True)
        args.fixture.write_bytes(data)
    print(json.dumps(dict(cases=len(cases), original_instructions=len(sites),
        all_original_sites_executed=True, fixture_sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))))


if __name__ == '__main__':
    main()
