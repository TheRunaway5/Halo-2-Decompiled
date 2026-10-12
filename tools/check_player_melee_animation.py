#!/usr/bin/env python3
"""Generate narrow EB000/EB700 differential fixtures from user-owned TU1.1.

169F3B receives an explicit availability mask. D0830 records a request boundary
and does not supply contacts or damage. EB080's real ordinary kind0 rejection
executes at expiry. This does not prove whole player/action/world admission.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('xbe', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    source_sha256=hashlib.sha256(args.xbe.read_bytes()).hexdigest()
    if source_sha256 != 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377':
        raise ValueError('This bounded original fixture requires the measured TU1.1 image')
    xbe = Xbe(args.xbe)
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    for address, size in [(0xe0000, 0x10000), (0x169000, 0x1000),
                          (0xd0000, 0x10000), (0x440000, 0x30000),
                          (0x4e7000, 0x8000), (0x60000000, 0x4000),
                          (0x70000000, 0x10000)]:
        uc.mem_map(address, size)
    for address, size in [(0xeb000, 0x80), (0xeb080, 0x4b0), (0xeb700, 0x130),
                          (0x446e34, 48), (0x465024, 4)]:
        uc.mem_write(address, xbe.read(address, size))

    uc.mem_write(0x169f3b, b"\xc2\x08\x00")
    uc.mem_write(0xd0830, b"\xc2\x10\x00")
    uc.mem_write(0x70000000, b"\xf4")

    def put(address, value):
        uc.mem_write(address, struct.pack('<I', value & 0xffffffff))

    def get(address):
        return struct.unpack('<I', uc.mem_read(address, 4))[0]

    unit, component = 0x60002000, 0x60002660
    put(0x4e78d0, 0x60000000)
    put(0x60000044, 0x60001000)
    put(0x60001008, unit)
    put(0x4ee9d8, 0x60003000)
    uc.mem_write(unit + 0x346, struct.pack('<h', 0x660))
    uc.mem_write(unit + 0x212, b'\xff')
    mask, impacts = 0, 0

    def returned(pop):
        esp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EIP, get(esp))
        uc.reg_write(UC_X86_REG_ESP, esp + 4 + pop)

    def instruction(_, address, size, context):
        nonlocal impacts
        if address == 0x169f3b:
            activity = get(uc.reg_read(UC_X86_REG_ESP) + 4)
            index = activity - 0x0e000607
            assert 0 <= index < 4
            uc.reg_write(UC_X86_REG_EAX, bool(mask & (1 << index)))
            returned(8)
        elif address == 0xd0830:
            impacts += 1
            returned(16)
        elif address == 0x70000000:
            uc.emu_stop()

    uc.hook_add(UC_HOOK_CODE, instruction)

    def run(entry, arguments):
        esp = 0x70008000
        put(esp, 0x70000000)
        for i, argument in enumerate(arguments):
            put(esp + 4 + i * 4, argument)
        uc.reg_write(UC_X86_REG_ESP, esp)
        try:
            uc.emu_start(entry, 0, count=10000)
        except Exception as error:
            raise RuntimeError(f'entry={entry:08x} eip={uc.reg_read(UC_X86_REG_EIP):08x} esp={uc.reg_read(UC_X86_REG_ESP):08x} mask={mask}') from error
        assert uc.reg_read(UC_X86_REG_EIP) == 0x70000000

    rows = []
    for seed in range(1, 65):
        for mask in range(16):
            initial = (seed * 0x1234567) & 0xffffffff
            put(0x60003000, initial)
            run(0xeb000, [0x12340000])
            selected = uc.reg_read(UC_X86_REG_EAX)
            index = -1 if not selected else (selected - 0x446e34) // 12
            rows.append(f'S {initial} {mask} {index} {get(0x60003000)}')
    for duration in [1, 6, 30, 60, 127]:
        for progress in range(duration):
            for impact in [-1, 0, 6, duration - 1]:
                impacts = 0
                put(component + 4, 1 << 27)
                put(component + 0x2c, 0x0e000607)
                uc.mem_write(component + 0x30, struct.pack('<BBbbbb', 0, 0, progress, impact, 25, duration))
                run(0xeb700, [0x12340000, 0])
                new_progress = struct.unpack('<b', uc.mem_read(component + 0x32, 1))[0]
                rows.append(f'T {progress} {impact} {duration} {new_progress} {get(component + 0x2c)} {impacts}')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(rows) + '\n')
    metadata={
        'source_xbe_sha256':source_sha256,
        'fixture_sha256':hashlib.sha256(args.output.read_bytes()).hexdigest(),
        'tool':'tools/check_player_melee_animation.py','cases':len(rows),
        'entries':['000eb000','000eb700','000eb080'],
        'boundaries':{'00169f3b':'Explicit availability mask; no whole player/weapon admission claim',
                      '000d0830':'Impact request boundary only; no contact or damage supplied'},
        'instruction_patches':'Boundary service entry thunks and return sentinel in test memory only; original EB000, EB700 and EB080 bytes unmodified.'}
    args.output.with_suffix('.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(f'{len(rows)} original melee selection/timer cases; explicit availability/impact boundaries')


if __name__ == '__main__':
    main()
