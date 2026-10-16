#!/usr/bin/env python3
"""Unchanged14830,158D0 and SDK texture-header/surface constructors.

XMemAllocDefault/XMemFreeDefault are declared platform allocation boundaries,
backed by real anonymous native pages. No game results or instructions are
supplied. Allocation faults either return the original false BOOL or stop at
the original unowned null texture read; no missing texture query is fabricated.
This checks cold resource semantics, not a live Xbox GPU frame.
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
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP


def run(image, fill, failure, executed):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(0x2100000, 0x1000)
    stack, stop = 0x2008000, 0x2100000
    cpu.mem_write(stack, struct.pack('<I', stop))
    cpu.reg_write(UC_X86_REG_ESP, stack)
    allocations, mappings, frees, null_reads, instructions = [], [], [], [], []

    def word(address):
        return struct.unpack('<I', cpu.mem_read(address, 4))[0]

    def hook(machine, pc, size, _):
        if pc in (0x2da8b0, 0x2da950):
            sp = machine.reg_read(UC_X86_REG_ESP)
            returned, first, flags = struct.unpack('<3I', machine.mem_read(sp, 12))
            if pc == 0x2da950:
                if flags != 0x24800000 or not any(row['source_token'] == first for row in allocations):
                    raise ValueError('Original header cleanup has no owned allocation')
                frees.append(first)
                # Keep diagnostic bytes mapped until the stopped reference
                # ends. The source must not use a freed object again.
            else:
                index = len(allocations)
                expected = ((20, 0x64800000), (16384, 0xb7800000), (24, 0x64800000))
                # A failed texture allocation causes an immediate later null
                # query. A failed surface allocation permits the second pair.
                slot = index % 3
                if (first, flags) != expected[slot]:
                    raise ValueError(f'Original texture allocation differs: {index} {first} {flags:x}')
                row = dict(index=index, bytes=first, flags=flags, fault_injected=index == failure,
                           source_token=0, native_backing=None)
                if index == failure:
                    pointer = 0
                else:
                    count = (first + 4095) & ~4095
                    pages = mmap.mmap(-1, count, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                                      prot=mmap.PROT_READ | mmap.PROT_WRITE)
                    pages[:] = bytes([fill]) * count
                    native = ctypes.addressof(ctypes.c_char.from_buffer(pages))
                    if native <= 0xffffffff:
                        raise ValueError('Reference allocation lacks a native64 backing')
                    pointer = 0x3000000 + index * 0x10000
                    machine.mem_map_ptr(pointer, count, 3, native)
                    mappings.append((pointer, count, native, pages))
                    row.update(source_token=pointer, native_backing=native)
                allocations.append(row)
                machine.reg_write(UC_X86_REG_EAX, pointer)
            machine.reg_write(UC_X86_REG_ESP, sp + 12)
            machine.reg_write(UC_X86_REG_EIP, returned)
            return
        raw = bytes(machine.mem_read(pc, size)).hex()
        if bytes.fromhex(raw) != image.read(pc, size):
            raise ValueError('Original texture instruction changed')
        if executed.setdefault(f'{pc:08x}', raw) != raw:
            raise ValueError('Original instruction identity differs across cases')
        instructions.append(pc)

    def invalid(machine, access, address, size, value, _):
        null_reads.append(dict(instruction=machine.reg_read(UC_X86_REG_EIP), address=address,
                               size=size, access=access))
        return False

    cpu.hook_add(UC_HOOK_CODE, hook)
    cpu.hook_add(UC_HOOK_MEM_INVALID, invalid)
    try:
        try:
            cpu.emu_start(0x14830, stop, count=10000)
        except UcError:
            if failure not in (0, 1, 3, 4) or not null_reads or null_reads[-1]['address'] >= 0x10000:
                raise
        completed = cpu.reg_read(UC_X86_REG_EIP) == stop
        cells = struct.unpack('<5I', cpu.mem_read(0x515e54, 20))
        headers = {}
        if failure is None or failure in (2, 5):
            if not completed or cpu.reg_read(UC_X86_REG_ESP) != stack + 4:
                raise ValueError('Original texture initializer did not return with its ABI')
            returned = cpu.reg_read(UC_X86_REG_EAX) & 255
            if returned != (failure is None):
                raise ValueError('Original texture initializer bool differs')
            for index in (0, 1):
                texture, surface = cells[index * 2:index * 2 + 2]
                header = struct.unpack('<5I', cpu.mem_read(texture, 20))
                expected_common = 0x1040001 if failure == index * 3 + 2 else 0x1040002
                if header != (expected_common, 0x3010000 + index * 0x30000, 0, 0x6610629, 0):
                    raise ValueError(f'Original swizzled ARGB texture header differs: {header}')
                headers[f'{0x515e54 + index * 8:08x}'] = list(header)
                if surface:
                    surface_header = struct.unpack('<6I', cpu.mem_read(surface, 24))
                    if surface_header != (0x1050001, header[1], 0, header[3], 0, texture):
                        raise ValueError('Original surface alias does not retain its texture parent')
                    headers[f'{0x515e58 + index * 8:08x}'] = list(surface_header)
                elif failure != index * 3 + 2:
                    raise ValueError('Original successful surface is absent')
            alias = struct.unpack('<6I', cpu.mem_read(0x491770, 24))
            if cells[4] != 0x491770 or alias != (0x50001, 0, 0, 0x11229, 0x1f1ff1ff, 0):
                raise ValueError('Original unbound linear surface differs')
        elif completed or not null_reads:
            raise ValueError('Original allocation failure bypassed its unowned null texture query')
        for address, count, native, pages in mappings:
            if bytes(cpu.mem_read(address, count)) != ctypes.string_at(native, count):
                raise ValueError('Original texture writes did not reach their native allocation')
        return dict(fill=fill, failed_allocation=failure, completed=completed,
                    return_bool=cpu.reg_read(UC_X86_REG_EAX) & 255 if completed else None,
                    publications=list(cells), headers=headers, allocations=allocations,
                    frees=frees, stopped_null_reads=null_reads, original_instructions=len(instructions))
    finally:
        for address, count, native, pages in mappings:
            cpu.mem_unmap(address, count)
            pages.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve existing evidence; select a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    executed = {}
    cases = [run(image, fill, None, executed) for fill in (0, 0x5a, 0xa5, 0xff)]
    cases += [run(image, 0, failure, executed) for failure in range(6)]
    args.output.mkdir(parents=True)
    report = dict(scope=__doc__, passed=True, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  engine_format11_d3d=struct.unpack('<I', image.read(0x45c488 + 11 * 4, 4))[0],
                  cases=cases, executed_instruction_bytes=executed,
                  instruction_patches=[], supplied_game_results=[],
                  allocation_platform_boundaries=['XMemAllocDefault', 'XMemFreeDefault'],
                  original_allocator_body_executed=False, live_gpu_or_frame_parity=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original texture pairs: {len(cases)} cases; {len(executed)} unchanged instruction sites')


if __name__ == '__main__':
    main()
