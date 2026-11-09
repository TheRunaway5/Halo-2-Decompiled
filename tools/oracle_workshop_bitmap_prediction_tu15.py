"""Capture TU1.5 bitmap prediction through the effect and script scheduler.

Execute the original effect and cache routines without substituting calls.
These fixtures exercise nonpaged lookup and retry, resident paged lookup,
and signed cache-clock branches over the original 116-byte layout.
Cold allocation and MCC ownership need separate validation.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


ENTRY, LOOKUP, REQUEST = 0x2AA200, 0x12F800, 0x12F910
INITIALIZE, SCHEDULE, EVALUATOR, COMPLETE = 0x212470, 0x211B10, 0x2AC680, 0x211DA0
BASE, STACK, STOP = 0x20000000, 0x20001080, 0x20002000
TABLE, ROOT, ROWS, HEADER = BASE, BASE + 0x80, BASE + 0x100, BASE + 0x300
POOL, TRACKING, MANAGER, PAGE_POOL, PAGES = (BASE + offset for offset in
    (0x400, 0x480, 0x500, 0x580, 0x600))
THREAD_POOL, SYNTAX_POOL, THREAD, NODES, OUTPUT, HS_STACK = (BASE + offset for offset in
    (0x4000, 0x4100, 0x5000, 0x5100, 0x6000, 0x7800))
GP = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
      R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)
FLAGS = 0x202
BANKS = ((BASE, 0x680), (STACK - 0x80, 0x100), (0x4F0414, 4),
         (0x4F2D24, 0x38), (0x474D9C, 6), (0x46A0D0, 4), (0x46A11C, 4))


def capture(image, name, handle, clock, count, rows, lookups, requests, paged=False, *, script=False):
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(BASE, 0x8000)

    def put(address, value, size=4):
        guest.mem_write(address, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little'))

    def snapshot():
        return [[address, bytes(guest.mem_read(address, size)).hex()] for address, size in BANKS]

    def xmm():
        return [[(guest.reg_read(R.UC_X86_REG_XMM0 + register) >> (lane * 32)) & 0xffffffff
                 for lane in range(4)] for register in range(8)]

    trace, stores, parsed = [], [], []
    pages = {}

    def instruction(_, pc, size, __):
        assert bytes(guest.mem_read(pc, size)) == image.read(pc, size), hex(pc)
        trace.append(pc)
        if pc == 0x2AC6A5:
            parsed.append(guest.reg_read(R.UC_X86_REG_EAX))

    def track(address, size):
        first, last = address & ~4095, (address + size - 1) & ~4095
        for page in range(first, last + 1, 4096):
            if page not in pages:
                pages[page] = bytes(guest.mem_read(page, 4096))

    def read(_, _kind, address, size, _value, __):
        track(address, size)

    def store(_, _kind, address, size, value, __):
        if script:
            track(address, size)
        else:
            assert any(start <= address and address + size <= start + length
                       for start, length in BANKS), (name, hex(address), size)
        stores.append([trace[-1], address, size, value & ((1 << (size * 8)) - 1)])

    guest.hook_add(UC_HOOK_CODE, instruction)
    guest.hook_add(UC_HOOK_MEM_WRITE, store)
    if script:
        guest.hook_add(UC_HOOK_MEM_READ, read)
    guest.mem_write(0x474D9C, bytes(6))
    guest.mem_write(0x4F2D24, bytes(0x38))
    put(0x4F0414, TABLE)
    put(0x4F2D58, clock)
    put(TABLE + 16 + 8, ROOT)
    put(ROOT + 0x44, count)
    put(ROOT + 0x48, ROWS)
    for index, (stamp, has_header) in enumerate(rows):
        row = ROWS + index * 116
        put(row + 0x50, HEADER + index * 16 if has_header else 0)
        put(row + 0x70, stamp)
    if paged:
        # Explicit ready tracking entry at slot 1, mip selection from the
        # original thresholds, and separate page-age storage.
        guest.mem_write(ROWS + 0x0f, b'\x02')
        put(ROWS + 0x58, 0x3f800000)
        for level in range(3):
            put(ROWS + 0x28 + 4 * level, 0x90020001 if level == 0 else 0xffffffff)
        put(0x4F2D24, POOL)
        put(POOL + 0x44, TRACKING)
        guest.mem_write(TRACKING + 40 + 3, b'\x01')
        put(0x4F2D34, MANAGER)
        put(MANAGER + 0x38, 0x45)
        put(MANAGER + 0x64, PAGE_POOL)
        put(PAGE_POOL + 0x44, PAGES)
    stack = HS_STACK if script else STACK
    put(stack, STOP)
    initial_gp = [0x12345678 if script else handle, 0x23456789, 0x3456789A, 0x456789AB,
                  stack, 0x56789ABC, 0x6789ABCD, 0xA0010000 if script else 0x789ABCDE]
    if script:
        # Same scheduler and syntax bank ABI as the independent original
        # looping-prediction run, with Xbox declaration 119 and bitmap type 39.
        assert image.read(0x480C00 + 119 * 4, 4) == struct.pack('<I', 0x457434)
        assert image.read(0x457434, 16) == struct.pack('<HHIIHH', 4, 0, EVALUATOR, 0, 1, 39)
        guest.mem_write(0x502248, bytes(0x3c10))
        for address, value in ((0x505E5C, THREAD_POOL), (0x505E60, 1), (0x505E6C, SYNTAX_POOL),
                               (0x4F3218, 0), (THREAD_POOL + 0x44, THREAD), (SYNTAX_POOL + 0x20, 3),
                               (SYNTAX_POOL + 0x24, 20), (SYNTAX_POOL + 0x44, NODES)):
            put(address, value)
        for index, (opcode, kind, flags, next_node, value) in enumerate((
                (119, 4, 8, 0xffffffff, 0x90010001), (2, 2, 9, 0x90020002, 0),
                (39, 39, 9, 0xffffffff, handle))):
            guest.mem_write(NODES + index * 20,
                            struct.pack('<4H3I', 0x9000 + index, opcode, kind, flags, next_node, 0, value))
        frame = THREAD + 0x40
        put(THREAD + 2, 1, 1)
        put(THREAD + 3, 1, 1)
        put(THREAD + 0xc, 0x100, 2)
        for address, value in ((THREAD + 0x10, frame), (frame, THREAD + 0x18),
                               (frame + 4, 0x90000000), (THREAD + 0x20, OUTPUT), (OUTPUT, 0xdeadbeef)):
            put(address, value)
    for register, value in zip(GP, initial_gp, strict=True):
        guest.reg_write(register, value)
    for register in range(8):
        guest.reg_write(R.UC_X86_REG_XMM0 + register,
                        sum((0x3F800000 + register * 16 + lane) << (lane * 32) for lane in range(4)))
    fp = [(R.UC_X86_REG_FPCW, 0x27f), (R.UC_X86_REG_FPSW, 0),
          (R.UC_X86_REG_FPTAG, 0xffff), (R.UC_X86_REG_MXCSR, 0x1f80)]
    guest.reg_write(R.UC_X86_REG_EFLAGS, FLAGS)
    for register, value in fp:
        guest.reg_write(register, value)
    initial_banks, initial_xmm = snapshot(), xmm()
    script_state = {}
    if script:
        track(BASE, 0x680)
        guest.emu_start(INITIALIZE, STOP, count=10000)
        assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
        script_state['initialized_gp'] = [guest.reg_read(reg) for reg in GP]
        for register, value in zip(GP, initial_gp, strict=True):
            guest.reg_write(register, value)
        guest.reg_write(R.UC_X86_REG_EFLAGS, FLAGS)
        guest.emu_start(SCHEDULE, STOP, count=200000)
        assert bytes(guest.mem_read(OUTPUT, 4)) == bytes(4)
        assert bytes(guest.mem_read(THREAD + 0x10, 4)) == struct.pack('<I', THREAD + 0x18)
        assert len(parsed) == 2 and parsed[0] == 0 and parsed[1] != 0
        script_state['parser_results'] = parsed
        initial_banks = [[page, data.hex()] for page, data in sorted(pages.items())]
    else:
        guest.emu_start(ENTRY, STOP, count=3000)
    assert guest.reg_read(R.UC_X86_REG_EIP) == STOP
    assert guest.reg_read(R.UC_X86_REG_ESP) == stack + 4
    assert all(guest.reg_read(register) == value for register, value in fp)
    counts = Counter(trace)
    if script:
        assert (counts[EVALUATOR], counts[ENTRY], counts[COMPLETE]) == (2, 1, 1)
    assert (counts[LOOKUP], counts[REQUEST]) == (lookups, requests), (name, counts)
    if paged:
        assert bytes(guest.mem_read(ROWS + 0x50, 4)) == (TRACKING + 40 + 0x14).to_bytes(4, 'little')
        assert bytes(guest.mem_read(ROWS + 0x70, 4)) == ((clock + 2) & 0xffffffff).to_bytes(4, 'little')
        assert bytes(guest.mem_read(PAGES + 0x14 + 24, 4)) == b'\x45\x00\x00\x00'
    # TEST on the empty/negative count path leaves AF undefined.
    expected_banks = ([[page, bytes(guest.mem_read(page, 4096)).hex()] for page in sorted(pages)]
                      if script else snapshot())
    return dict(name=name, initial_banks=initial_banks, expected_banks=expected_banks,
                initial_gp=initial_gp, expected_gp=[guest.reg_read(reg) for reg in GP],
                initial_xmm=initial_xmm, expected_xmm=xmm(), initial_flags=FLAGS,
                expected_flags=guest.reg_read(R.UC_X86_REG_EFLAGS),
                defined_flags=0xed5 if not script and (handle == 0xffffffff or count > 0) else 0xec5,
                trace=trace, stores=stores, lookups=lookups, requests=requests, **script_state)


def main():
    if not __debug__:
        raise RuntimeError('The bitmap prediction oracle requires assertions')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--script', action='store_true')
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.fixture.resolve().is_relative_to(owner) and not args.fixture.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    handle = 0x90010001
    inputs = [
        ('none', 0xffffffff, 20, 0, [], 0, 0),
        ('empty', handle, 20, 0, [], 0, 0),
        ('negative_count', handle, 20, -1, [], 0, 0),
        ('future_header', handle, 20, 1, [(21, True)], 0, 0),
        ('equal_header', handle, 20, 1, [(20, True)], 1, 0),
        ('stale_header', handle, 20, 1, [(19, True)], 1, 0),
        ('future_missing_header', handle, 20, 1, [(21, False)], 1, 1),
        ('stale_missing_header', handle, 20, 1, [(19, False)], 1, 1),
        ('signed_clock', handle, -2147483648, 2, [(-1, True), (-2147483648, True)], 1, 0),
        ('mixed_stride', handle, 20, 4, [(30, True), (20, True), (-1, False), (19, True)], 3, 1),
        ('paged_resident', handle, 20, 1, [(19, False)], 1, 0, True),
        ('paged_resident_clock_wrap', handle, 0x7ffffffe, 1, [(19, False)], 1, 0, True),
    ]
    cases = [capture(image, *item, script=args.script) for item in inputs]
    args.fixture.write_text(json.dumps(dict(xbe_sha256=NEW_PIN,
        entry=SCHEDULE if args.script else ENTRY, cases=cases)) + '\n')
    print('Original bitmap prediction:', len(cases), 'cases,',
          sum(row['lookups'] for row in cases), 'lookups,',
          sum(row['requests'] for row in cases), 'requests')


if __name__ == '__main__':
    main()
