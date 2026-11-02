#!/usr/bin/env python3
"""Compare the native22830 slot reset with the unchanged TU1.5 reset prefix.

This validates22830..2286C, stopping at the genuine2286E BSP continuation.
It does not validate the remaining BSP rotation/interpolation or a GPU frame.
Raw comparison files belong to HALO2_TEST_SCRATCH supplied by run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASE, SIZE = 0x4c51d0, 0x9c0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if not args.xbe.is_file():
        print('SKIP missing owned English TU1.5 executable')
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    if hashlib.sha256(args.xbe.read_bytes()).hexdigest() != PIN:
        raise ValueError('Present executable is not OG Xbox English TU1.5')
    sys.path.insert(0, str(Path(__file__).resolve().parent / 'reference/halo2-decompiled/tools'))
    from xbe import Xbe
    import unicorn
    from unicorn import x86_const as R
    image = Xbe(args.xbe)
    cpu = unicorn.Uc(unicorn.UC_ARCH_X86, unicorn.UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    stack = 0x62000000
    cpu.mem_map(stack, 4096)
    stores, authenticated = [], {}
    executed = 0

    def instruction(g, pc, size, _):
        nonlocal executed
        actual = bytes(g.mem_read(pc, size))
        if not 0x22830 <= pc < 0x2286e or actual != image.read(pc, size):
            raise ValueError('Model reset escaped its unchanged authenticated prefix')
        authenticated[pc] = actual.hex()
        executed += 1

    def write(g, _kind, address, size, value, _):
        if BASE <= address < BASE + SIZE:
            if size not in (2, 4) or address + size > BASE + SIZE:
                raise ValueError('Unexpected model slot store extent')
            before = bytes(g.mem_read(address, size)).ljust(4, b'\0')
            stores.append(struct.pack('<III', g.reg_read(R.UC_X86_REG_EIP), address, size) +
                          before + value.to_bytes(size, 'little').ljust(4, b'\0'))

    cpu.hook_add(unicorn.UC_HOOK_CODE, instruction)
    cpu.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)
    rng = random.Random(0x22830)
    banks = [bytes([fill]) * SIZE for fill in (0, 0x5a, 0xa5, 0xff)]
    banks += [rng.randbytes(SIZE) for _ in range(12)]
    requests, expected = scratch / 'model-slots.in', scratch / 'model-slots.expected'
    with requests.open('wb') as inputs, expected.open('wb') as outputs:
        for bank in banks:
            inputs.write(bank)
            cpu.mem_write(BASE, bank)
            cpu.reg_write(R.UC_X86_REG_ESP, stack + 0x800)
            cpu.reg_write(R.UC_X86_REG_EBX, 4)
            cpu.reg_write(R.UC_X86_REG_EBP, 6)
            cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
            stores.clear()
            executed = 0
            cpu.emu_start(0x22830, 0x2286e, count=4096)
            if cpu.reg_read(R.UC_X86_REG_EIP) != 0x2286e or len(stores) != 552:
                raise ValueError('Original reset did not reach its genuine BSP continuation')
            actual_gp = [cpu.reg_read(getattr(R, 'UC_X86_REG_' + name))
                         for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]
            if actual_gp != [0, 0x4c5b74, 0x4c5b94, 4, stack + 0x78c, 6, 0xffffffff, 0x4c5b74] or \
                    cpu.reg_read(R.UC_X86_REG_EFLAGS) & 0x8d5 != 0x44:
                raise ValueError('Original2286E continuation inputs differ: ' + str(actual_gp))
            # Four prologue instructions remain translated; the747 reset
            # instructions are replaced. REP STOSD includes its exit hook.
            if executed != 751:
                raise ValueError('Original model-reset instruction count changed: ' + str(executed))
            outputs.write(bytes(cpu.mem_read(BASE, SIZE)))
            outputs.write(struct.pack('<I', len(stores)) + b''.join(stores))
    actual = scratch / 'model-slots.actual'
    with requests.open('rb') as inputs, actual.open('wb') as outputs:
        subprocess.run([str(args.native), '--probe'], stdin=inputs, stdout=outputs, check=True)
    def digest(path):
        with path.open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual.stat().st_size != expected.stat().st_size or digest(actual) != digest(expected):
        raise ValueError('Native model-slot bytes/ordered stores differ from unchanged TU1.5 reset')
    args.report.write_text(json.dumps(dict(passed=True, cases=len(banks), slots_per_case=24,
        ordered_stores_per_case=552, whole_bank_bytes_per_case=SIZE,
        original_xbe_sha256=PIN, original_prefix_entry=0x22830, original_continuation=0x2286e,
        original_prefix_instructions=751, retired_reset_instructions=747,
        original_instruction_bytes={f'{pc:08x}': raw for pc, raw in sorted(authenticated.items())},
        comparison_sha256=digest(actual), compared_bytes=actual.stat().st_size,
        instruction_patches=[], supplied_results=[],
        bsp_rotation_interpolation_or_visual_parity_verified=False), indent=2, sort_keys=True) + '\n')
    print('PASS16 unchanged-TU1.5 model-reset prefixes: whole24-slot banks,552 ordered stores and continuation inputs')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
