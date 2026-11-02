#!/usr/bin/env python3
"""Compare semantic slot retirement to unchanged OG Xbox English TU1.5 code.

Run inside tools/run_test.py (CTest does this automatically). Raw banks and
the probe output remain under HALO2_TEST_SCRATCH and are removed by its owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
BASE, SIZE = 0x52dfd8, 0x40c0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--bridge', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if not args.xbe.is_file():
        print('SKIP missing owned TU1.5 executable')
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
    stack, stop = 0x62000000, 0x63000000
    cpu.mem_map(stack, 4096)
    cpu.mem_map(stop, 4096)
    cpu.mem_write(stack + 0x800, struct.pack('<I', stop))
    stores, all_stores, authenticated = [], [], {}

    def instruction(g, pc, size, _):
        actual = bytes(g.mem_read(pc, size))
        if not 0x20e30 <= pc < 0x20e90 or actual != image.read(pc, size):
            raise ValueError('Original retirement escaped its unchanged authenticated body')
        authenticated[pc] = actual.hex()

    def write(g, _kind, address, size, value, _):
        event = struct.pack('<II', g.reg_read(R.UC_X86_REG_EIP), address) + \
            bytes(g.mem_read(address, size)) + value.to_bytes(size, 'little')
        all_stores.append(event)
        if BASE <= address < BASE + SIZE:
            if size != 4 or address + size > BASE + SIZE:
                raise ValueError('Unexpected original query store extent')
            stores.append(event)

    cpu.hook_add(unicorn.UC_HOOK_CODE, instruction)
    cpu.hook_add(unicorn.UC_HOOK_MEM_WRITE, write)
    requests, expected = scratch / 'slots.in', scratch / 'slots.expected'
    bridge_expected = scratch / 'slots-bridge.expected'
    case_count = 0
    with requests.open('wb') as inputs, expected.open('wb') as outputs, bridge_expected.open('wb') as bridges:
        # Every legal slot, including every word boundary and last real slot.
        # Nonuniform patterns expose neighboring-record/mask corruption.
        for pattern in (0, 0x5a, 0xa5, 0xff):
            bank = bytes(((n * 73 + pattern) & 255) for n in range(SIZE))
            for slot in range(511):
                inputs.write(struct.pack('<I', slot) + bank)
                cpu.mem_write(BASE, bank)
                registers = ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')
                initial_gp = (slot, 0x22222222, 0x33333333, 0x44444444,
                              stack + 0x800, 0x66666666, 0x77777777, 0x88888888)
                for name, value in zip(registers, initial_gp):
                    cpu.reg_write(getattr(R, 'UC_X86_REG_' + name), value)
                cpu.mem_write(stack + 0x7fc, bytes(4))
                cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
                stores.clear()
                all_stores.clear()
                cpu.emu_start(0x20e30, stop, count=128)
                if cpu.reg_read(R.UC_X86_REG_EIP) != stop or len(stores) != 12:
                    raise ValueError('Original retirement did not reach its genuine return')
                if (cpu.reg_read(R.UC_X86_REG_EAX) != 0xff80 or
                    cpu.reg_read(R.UC_X86_REG_ECX) != BASE + 4 + slot * 32 or
                    cpu.reg_read(R.UC_X86_REG_EDX) != (~(1 << (slot & 31)) & 0xffffffff) or
                    cpu.reg_read(R.UC_X86_REG_ESI) != 0x77777777 or
                    cpu.reg_read(R.UC_X86_REG_ESP) != stack + 0x804 or
                    cpu.reg_read(R.UC_X86_REG_EFLAGS) & 0x8d5 != 0):
                    raise ValueError('Original query retirement caller contract differs: ' +
                                     str({name: hex(cpu.reg_read(getattr(R, 'UC_X86_REG_' + name)))
                                          for name in ('EAX', 'ECX', 'EDX', 'ESI', 'ESP', 'EFLAGS')}))
                outputs.write(bytes(cpu.mem_read(BASE, SIZE)))
                outputs.write(struct.pack('<I', len(stores)) + b''.join(stores))
                if len(all_stores) != 13 or all_stores[0] != \
                        struct.pack('<II', 0x20e30, stack + 0x7fc) + bytes(4) + bytes([0x77]) * 4:
                    raise ValueError('Original saved-register stack store differs')
                bridges.write(bytes(cpu.mem_read(BASE, SIZE)))
                bridges.write(struct.pack('<I', len(all_stores)) + b''.join(all_stores))
                bridges.write(struct.pack('<8II',
                    *(cpu.reg_read(getattr(R, 'UC_X86_REG_' + name)) for name in registers),
                    cpu.reg_read(R.UC_X86_REG_EFLAGS) & 0x8d5))
                case_count += 1
    actual = scratch / 'slots.actual'
    with requests.open('rb') as inputs, actual.open('wb') as outputs:
        subprocess.run([str(args.native), '--probe'], stdin=inputs, stdout=outputs, check=True)
    def digest(path):
        with path.open('rb') as stream:
            return hashlib.file_digest(stream, 'sha256').hexdigest()
    if actual.stat().st_size != expected.stat().st_size or digest(actual) != digest(expected):
        raise ValueError('Native query banks or ordered before/after stores differ from unchanged TU1.5')
    bridge_routes = []
    if args.bridge:
        for route in range(3):
            with requests.open('rb') as inputs, actual.open('wb') as outputs:
                subprocess.run([str(args.bridge), '--probe', str(route)], stdin=inputs, stdout=outputs, check=True)
            if actual.stat().st_size != bridge_expected.stat().st_size or digest(actual) != digest(bridge_expected):
                raise ValueError('Native bridge differs from original complete store/GP/flag stream: ' + str(route))
            bridge_routes.append(dict(route=route, cases=case_count, ordered_stores_per_case=13,
                                      comparison_sha256=digest(actual), compared_bytes=actual.stat().st_size))
    report = dict(passed=True, cases=case_count, whole_bank_bytes_per_case=SIZE,
                  ordered_stores_per_case=12, original_xbe_sha256=PIN,
                  original_instruction_bytes={f'{pc:08x}': raw for pc, raw in sorted(authenticated.items())},
                  instruction_patches=[], supplied_results=[],
                  comparison_sha256=digest(expected), compared_bytes=expected.stat().st_size,
                  original_bridge_routes=bridge_routes,
                  physical_xbox_or_visual_parity_verified=False)
    if args.report:
        args.report.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
