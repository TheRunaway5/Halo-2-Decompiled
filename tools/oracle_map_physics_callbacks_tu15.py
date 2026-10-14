#!/usr/bin/env python3
"""Compare the complete map physics callback and its query with original TU1.5.

The object, physics pool and caller are explicit inputs. No original functions
are hooked; this does not establish allocation, scheduling or map acceptance.
"""
import argparse
import hashlib
import json
import random
import struct
import subprocess
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_XMM0, UC_X86_REG_EFLAGS, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR,
)
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True,
                        help='source_map_world_startup_tu15_tests executable')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reset-only', action='store_true',
                        help='Compare 10B730 with NONE or inactive effect handles')
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    xbe = Xbe(NEW_DEFAULT)
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        uc.mem_write(section.va, xbe.section_bytes(section))
    uc.mem_map(0x2400000, 0x20000)
    uc.mem_map(0x3000000, 4096)
    regs = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
            UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    xregs = [UC_X86_REG_XMM0 + n for n in range(8)]
    trace, writes = [], []
    uc.hook_add(UC_HOOK_CODE, lambda u, pc, size, data: trace.append(pc))
    uc.hook_add(UC_HOOK_MEM_WRITE, lambda u, access, p, size, value, data:
                writes.append([p, value.to_bytes(size, 'little').hex()]))
    rng = random.Random(0x11e490)
    pool, headers, body, physics, records, output, stack = (
        0x2400000, 0x2401000, 0x2402000, 0x2403000,
        0x2404000, 0x2405000, 0x2410080)
    cases = []
    entries = (0x10b730,) if args.reset_only else (0x1db800, 0x11e490)
    for entry in entries:
        for n in range(256):
            banks = {}

            def bank(p, size):
                banks[p] = bytearray(rng.randbytes(size))

            def put(p, offset, value):
                struct.pack_into('<I', banks[p], offset, value & 0xffffffff)

            for p, size in ((pool, 0x80), (headers, 0x100),
                            (body, 0x200 if args.reset_only else 0x100),
                            (physics, 0x80), (records, 0x500), (output, 8),
                            (stack - 0x80, 0x100)):
                bank(p, size)
            banks[0x4ecbd0] = bytearray(struct.pack('<I', pool))
            banks[0x52be5c] = bytearray(struct.pack('<I', physics))
            banks[0x46a0c0] = bytearray(xbe.read(0x46a0c0, 0x20))
            datum = rng.getrandbits(16) << 16 | n % 8
            phys = rng.getrandbits(16) << 16 | (n // 8) % 8
            put(pool, 0x44, headers)
            put(headers, (datum & 65535) * 12 + 8, body)
            put(physics, 0x44, records)
            put(body, 0xb4, 0xffffffff if n % 8 == 0 else phys)
            row = (phys & 65535) * 0xa0
            value = (0, 1, 127, 128, 254, 255)[(n // 8) % 6]
            banks[records][row + 0x1a] = value
            banks[records][row + 0x1b] = 255 - value
            flags = rng.getrandbits(32) & ~0x2000
            put(records, row + 4, flags | (0x2000 if n % 2 else 0))
            query = (0xa000693, 0xd000690, 0xd000691,
                     0, 0xffffffff, 0xd000692, 0xa000693, 0xa000693)[n % 8]
            gp = [rng.getrandbits(32) for _ in regs]
            gp[4], gp[6], gp[1], gp[2] = stack, records + row, query, output
            put(stack - 0x80, 0x80, 0x3000000)
            if entry == 0x10b730:
                effect_pool, effect_headers = 0x2406000, 0x2407000
                bank(effect_pool, 0x80)
                bank(effect_headers, 0x100)
                banks[0x4ecbf8] = bytearray(struct.pack('<I', effect_pool))
                put(effect_pool, 0x44, effect_headers)
                for slot in range(8):
                    banks[effect_headers][slot * 12 + 2] &= ~4
                put(body, 0x130, 0xffffffff if n % 3 == 0 else 0xe2740003)
                put(body, 0x13c, 0xffffffff if n % 3 == 1 else 0xf3890004)
                put(stack - 0x80, 0x84, datum)
            elif entry == 0x1db800:
                put(stack - 0x80, 0x84, output + 4)
            else:
                for offset, value in ((0x84, datum), (0x88, query),
                                      (0x8c, output), (0x90, output + 4)):
                    put(stack - 0x80, offset, value)
            cases.append(dict(entry=entry, gp=gp, eflags=2, cw=0x37f,
                              xmm=[[rng.getrandbits(32) for _ in range(4)]
                                   for _ in xregs],
                              regions=[dict(address=p, bytes=b.hex())
                                       for p, b in sorted(banks.items())]))
    result = subprocess.run([str(args.probe.resolve())],
                            input=''.join(json.dumps(c) + '\n' for c in cases),
                            text=True, capture_output=True, check=True)
    native = [json.loads(line) for line in result.stdout.splitlines()]
    assert len(native) == len(cases)
    rows = []
    args.output.parent.mkdir(parents=True, exist_ok=True)
    for index, (case, got) in enumerate(zip(cases, native)):
        for region in case['regions']:
            uc.mem_write(region['address'], bytes.fromhex(region['bytes']))
        for reg, value in zip(regs, case['gp']):
            uc.reg_write(reg, value)
        for reg, value in zip(xregs, case['xmm']):
            uc.reg_write(reg, int.from_bytes(struct.pack('<4I', *value), 'little'))
        uc.reg_write(UC_X86_REG_EFLAGS, case['eflags'])
        uc.reg_write(UC_X86_REG_FPCW, case['cw'])
        uc.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        trace.clear()
        writes.clear()
        uc.emu_start(case['entry'], 0x3000000, count=1000)
        assert uc.reg_read(UC_X86_REG_EIP) == 0x3000000
        want = dict(gp=[uc.reg_read(reg) for reg in regs],
                    xmm=[list(struct.unpack('<4I', uc.reg_read(reg).to_bytes(16, 'little')))
                         for reg in xregs],
                    eflags=uc.reg_read(UC_X86_REG_EFLAGS),
                    instructions=trace.copy(), writes=writes.copy(),
                    banks=[bytes(uc.mem_read(b['address'], len(b['bytes']) // 2)).hex()
                           for b in case['regions']])
        if want != got:
            args.output.with_suffix('.failure.json').write_text(json.dumps(
                dict(index=index, input=case, original=want, native=got), indent=2) + '\n')
            raise AssertionError(f'Original map physics callback mismatch {index}')
        rows.append(dict(input=case, original=want))
    args.output.write_text(''.join(json.dumps(row) + '\n' for row in rows))
    report = dict(passed=True, cases=len(rows), entries=[f'{e:08x}' for e in entries],
                  original_sha256=NEW_PIN, hooked_functions=[], full_goal_complete=False,
                  boundary=(__doc__ + ' Reset-only mode covers NONE/inactive effect handles; '
                            'active effect retirement through 16E8A0 remains separate.'
                            if args.reset_only else __doc__),
                  probe_sha256=hashlib.sha256(args.probe.read_bytes()).hexdigest(),
                  fixtures_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.output.with_suffix('.verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
