#!/usr/bin/env python3
"""Run unchanged1D750/1DAF0/1DF00/1D6D0 and XPhysicalProtect.

The declared input arena has actual native64 pages. MmSetAddressProtect is the
only supplied platform boundary and performs real host mprotect. This checks
the original CPU resource definitions and shared byte ranges; host cache policy
and GPU visibility are separate contracts. No game result or instruction patch.
"""
import argparse
import ctypes
import hashlib
import json
import mmap
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP,
                              UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EIP,
                              UC_X86_REG_EFLAGS)


def run(image, mode, executed):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(0x2100000, 0x1000)
    cpu.mem_map(0x80000000, 0x1000)
    base, size = 0x81000000, 0x4000000
    pages = mmap.mmap(-1, size, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                      prot=mmap.PROT_READ | mmap.PROT_WRITE)
    native = ctypes.addressof(ctypes.c_char.from_buffer(pages))
    if native <= 0xffffffff:
        raise ValueError('Reference arena lacks a native64 backing')
    cpu.mem_map_ptr(base, size, 3, native)
    libc = ctypes.CDLL(None, use_errno=True)
    libc.mprotect.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
    stack, stop = 0x2008000, 0x2100000
    put = lambda p, value: cpu.mem_write(p, struct.pack('<I', value))
    word = lambda p: struct.unpack('<I', cpu.mem_read(p, 4))[0]
    put(stack, stop)
    cpu.reg_write(UC_X86_REG_ESP, stack)
    put(0x4f2cf0, 0)
    put(0x4f2cfc, base & 0x7fffffff)
    put(0x4f2d10, (base & 0x7fffffff) + (4096 if mode == 'exhausted' else size))
    if mode == 'exhausted':
        put(0x56e190, 10)
    if mode == 'occupied':
        cpu.mem_write(0x4c140d + 0x98, b'\1')
    protections, instructions = [], []

    def hook(machine, pc, count, _):
        if pc == 0x800000b6:
            sp = machine.reg_read(UC_X86_REG_ESP)
            returned, address, count, protection = struct.unpack('<4I', machine.mem_read(sp, 16))
            if returned != 0x1db4b or protection != 0x404 or address < base or address + count > base + size:
                raise ValueError('Original target allocation requested an unowned protection')
            if libc.mprotect(native + address - base, count, mmap.PROT_READ | mmap.PROT_WRITE):
                raise OSError(ctypes.get_errno(), 'Reference target arena mprotect')
            protections.append(dict(address=address, bytes=count, protection=protection,
                                    native_backing=native + address - base))
            machine.reg_write(UC_X86_REG_ESP, sp + 16)
            machine.reg_write(UC_X86_REG_EIP, returned)
            return
        raw = bytes(machine.mem_read(pc, count)).hex()
        if image.read(pc, count) != bytes.fromhex(raw):
            raise ValueError('Original target-storage instruction changed')
        if executed.setdefault(f'{pc:08x}', raw) != raw:
            raise ValueError('Original target-storage instruction identity differs')
        instructions.append(pc)

    cpu.hook_add(UC_HOOK_CODE, hook)
    try:
        cpu.emu_start(0x1d750, stop, count=100000)
        if cpu.reg_read(UC_X86_REG_EIP) != stop or cpu.reg_read(UC_X86_REG_ESP) != stack + 4:
            raise ValueError('Original target-storage constructor lost its caller')
        result = cpu.reg_read(UC_X86_REG_EAX) & 255
        if (mode == 'normal' and result != 1) or (mode == 'occupied' and result != 0):
            raise ValueError(f'Original target-storage path differs: {mode} returned {result}')
        records = []
        for index in range(28):
            record = 0x4c1378 + index * 0x98
            pointer = word(record + 0x88)
            if not pointer:
                continue
            levels = word(record + 0x78)
            words = struct.unpack('<38I', cpu.mem_read(record, 0x98))
            records.append(dict(index=index, words=list(words), raw_base=pointer,
                                width=word(record + 0x80), height=word(record + 0x84),
                                surface_levels=levels, reserved_bytes=word(record + 0x7c)))
        if mode == 'normal':
            if len(records) != 18 or [row['bytes'] for row in protections] != [0x12c000, 0x100000, 0x4000]:
                raise ValueError('Original target bank has another population or allocation footprint')
            if word(0x4f2d10) != (base & 0x7fffffff) + size - 0x230000:
                raise ValueError('Original target-bank physical partition cursor differs')
            for row in records:
                p, n = row['raw_base'], row['reserved_bytes']
                if p < base or p + n > base + size:
                    raise ValueError('Original target data leaves its actual native arena')
        return dict(mode=mode, returned_bool=result, native_arena=native,
                    returned_gp=[cpu.reg_read(r) for r in (UC_X86_REG_EAX, UC_X86_REG_ECX,
                                 UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP,
                                 UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)],
                    returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS),
                    original_instructions=len(instructions), protections=protections,
                    physical_top=word(0x4f2d10), records=records,
                    bank_hex=bytes(cpu.mem_read(0x4c1378, 28 * 0x98)).hex(),
                    missing_storage_count=word(0x56e190))
    finally:
        cpu.mem_unmap(base, size)
        pages.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve earlier evidence; select a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    executed = {}
    cases = [run(image, mode, executed) for mode in ('normal', 'exhausted', 'occupied')]
    args.output.mkdir(parents=True)
    report = dict(scope=__doc__, passed=True, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=cases,
                  executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
                  gpu_visibility_or_frame_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original target storage: {len(cases)} cases; {len(executed)} unchanged instruction sites')


if __name__ == '__main__':
    main()
