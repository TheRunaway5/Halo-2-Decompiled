#!/usr/bin/env python3
"""Execute bounded, unchanged Vista text/prompt/list dispatch instructions."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    exe = Path('Halo 2 Vista installer/halo2.exe').read_bytes()
    digest = hashlib.sha256(exe).hexdigest()
    assert digest == '33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x400000, 0x1000000)
    pe = struct.unpack_from('<I', exe, 0x3c)[0]
    count = struct.unpack_from('<H', exe, pe + 6)[0]
    opt = struct.unpack_from('<H', exe, pe + 20)[0]
    for i in range(count):
        p = pe + 24 + opt + i * 40
        _, rva, size, offset = struct.unpack_from('<4I', exe, p + 8)
        u.mem_write(0x400000 + rva, exe[offset:offset + size])
    stack, widget, shared, stop = 0x10008000, 0x10001000, 0x10002000, 0x1000f000
    u.mem_map(0x10000000, 0x10000)
    clock, navigation = 0, []

    def observe(g, pc, size, _):
        if pc == 0x6096ae:
            sp = g.reg_read(UC_X86_REG_ESP)
            g.reg_write(UC_X86_REG_EAX, clock)
            g.reg_write(UC_X86_REG_EIP, struct.unpack('<I', g.mem_read(sp, 4))[0])
            g.reg_write(UC_X86_REG_ESP, sp + 4)
        elif pc == 0x614537:
            sp = g.reg_read(UC_X86_REG_ESP)
            navigation.append(struct.unpack('<i', g.mem_read(sp + 4, 4))[0])
            g.reg_write(UC_X86_REG_EIP, struct.unpack('<I', g.mem_read(sp, 4))[0])
            g.reg_write(UC_X86_REG_ESP, sp + 8)
    u.hook_add(UC_HOOK_CODE, observe)
    u.reg_write(UC_X86_REG_ESP, stack)
    u.mem_write(stack + 0xd8, struct.pack('<I', 25))
    u.emu_start(0x60bee0, 0x60c0c9, count=1000)
    pairs = [struct.unpack('<II', u.mem_read(stack + 4 + i * 8, 8)) for i in range(26)]
    assert [p[0] for p in pairs] == list(range(26))
    assert pairs[1][1] == 0x0f0006cc and pairs[25][1] == 0x11000975
    timeline = []
    text = 'Halogen flashlights are bright'
    u.mem_write(widget + 0x3c, struct.pack('<hhI', 0, len(text), 0))
    u.mem_write(shared + 0x48, bytes.fromhex('3200db009a99993e'))
    for clock in [100, 149, 150, 150, 1000, 1000] + list(range(1050, 3001, 50)):
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ESI, widget)
        u.reg_write(UC_X86_REG_EBP, shared)
        u.reg_write(UC_X86_REG_EBX, 0xdb)
        u.mem_write(stack + 0x78, (text + '\0').encode('utf-16le') + bytes(2048))
        u.emu_start(0x6303b9, 0x630426, count=1000)
        units = struct.unpack('<1024H', u.mem_read(stack + 0x78, 2048))
        displayed = ''.join(chr(c) for c in units[:units.index(0)])
        cursor, length, last = struct.unpack('<hhI', u.mem_read(widget + 0x3c, 8))
        timeline.append(dict(clock=clock, displayed=displayed, cursor=cursor, last_clock=last))
    assert [r['displayed'] for r in timeline[:6]] == ['\xdb', '\xdb', '\xdb', 'H\xdb', 'H\xdb', 'Ha\xdb']
    assert timeline[-1]['displayed'] == text and timeline[-1]['cursor'] == -1
    # The actual list jump table handles wheel navigation events10/11 with
    # the same614537 calls as up/down1/3; only the navigation leaf is serviced.
    for event in (1, 3, 10, 11):
        u.mem_write(widget, struct.pack('<4I', event, 0, 0, 0))
        u.reg_write(UC_X86_REG_ESP, stack)
        u.reg_write(UC_X86_REG_ECX, shared)
        u.mem_write(stack, struct.pack('<II', stop, widget))
        u.emu_start(0x614ca9, stop, count=1000)
    assert navigation == [-1, 1, -1, 1]
    args.output.mkdir(parents=True, exist_ok=True)
    md = Cs(CS_ARCH_X86, CS_MODE_32)
    for a, z in [(0x60bee0, 0x60c0c9), (0x6303b9, 0x630426),
                 (0x60780b, 0x607837), (0x607b3e, 0x607b66),
                 (0x607ddc, 0x607e28), (0x614ca9, 0x614d90),
                 (0x46244e, 0x462488)]:
        (args.output / f'{a:08x}.asm').write_text(''.join(
            f'{i.address:08X} {i.mnemonic} {i.op_str}\n'
            for i in md.disasm(exe[a-0x400000:z-0x400000], a)))
    report = dict(executable_sha256=digest, prompt_pairs=pairs, typing=timeline,
                  list_events=[1, 3, 10, 11], navigation_calls=navigation,
                  scope='Unchanged prompt table construction, text reveal prefix and list event dispatch. Clock and navigation leaf are explicit services. Static wheel event production is retained. No Windows raster or whole input-system execution claim.')
    (args.output / 'source.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Vista prompt table,50ms typing prefix and one-row list dispatch passed')


if __name__ == '__main__':
    main()
