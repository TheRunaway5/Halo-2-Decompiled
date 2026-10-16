#!/usr/bin/env python3
"""Run unchanged TU1.5 renderer geometry/texture cache pool constructors.

Whole 1303C0/12EBE0, 11F3F0, 33B3AC, 16E450/16E4B0/16E600,
13FC50 and the optimized name-copy owner execute from authenticated bytes.
Only NtAllocateVirtualMemory is a supplied platform boundary backed by real
native64 pages. Three allocation fills are declared inputs. No retained GPU
callback, game result, renderer success, or Xbox kernel fill policy is supplied.
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
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)


def run(image, fill, executed, output):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(0x2100000, 0x1000)
    cpu.mem_map(0x80000000, 0x1000)
    stack, stop, next_source = 0x2008000, 0x2100000, 0x10000000
    put = lambda address, value: cpu.mem_write(address, struct.pack('<I', value))
    word = lambda address: struct.unpack('<I', cpu.mem_read(address, 4))[0]
    instructions, stores, allocations, pages, calls = [], [], [], [], []
    bindings = [0x4f2d5c, 0x4f2d60, 0x4f2d6c, 0x4f2d24, 0x4f2d28, 0x4f2d34]

    def instruction(machine, pc, count, _):
        nonlocal next_source
        if pc == 0x800000b8:
            sp = machine.reg_read(UC_X86_REG_ESP)
            returned, base_cell, zero_bits, size_cell, kind, protection = struct.unpack(
                '<6I', machine.mem_read(sp, 24))
            size, preferred = word(size_cell), word(base_cell)
            if returned != 0x33b3c5 or preferred or zero_bits or kind != 0x101000 or protection != 4:
                raise ValueError('Original renderer cache allocation crossed its declared platform ABI')
            rounded = (size + 4095) & ~4095
            backing = mmap.mmap(-1, rounded, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                                prot=mmap.PROT_READ | mmap.PROT_WRITE)
            backing[:] = bytes([fill]) * rounded
            native = ctypes.addressof(ctypes.c_char.from_buffer(backing))
            if native <= 0xffffffff:
                raise ValueError('Original renderer cache allocation lacks native64 pages')
            base = next_source
            machine.mem_map_ptr(base, rounded, 3, native)
            pages.append((base, rounded, backing))
            next_source += rounded
            put(base_cell, base)
            put(size_cell, rounded)
            allocations.append(dict(source=base, native_backing=native, requested_bytes=size,
                                    reserved_bytes=rounded, allocation_type=kind, protection=protection))
            machine.reg_write(UC_X86_REG_EAX, 0)
            machine.reg_write(UC_X86_REG_ESP, sp + 24)
            machine.reg_write(UC_X86_REG_EIP, returned)
            return
        raw = bytes(machine.mem_read(pc, count))
        if image.read(pc, count) != raw or executed.setdefault(f'{pc:08x}', raw.hex()) != raw.hex():
            raise ValueError('Original renderer cache instruction changed')
        instructions.append(pc)

    def store(machine, access, address, size, value, _):
        if address in bindings or any(base <= address < base + count for base, count, _ in pages):
            stores.append((machine.reg_read(UC_X86_REG_EIP), address, size,
                           int.from_bytes(machine.mem_read(address, size), 'little'), value))

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    try:
        for entry in (0x1303c0, 0x12ebe0):
            put(stack, stop)
            cpu.reg_write(UC_X86_REG_ESP, stack)
            initial_count = len(instructions)
            cpu.emu_start(entry, stop, count=1000000)
            if cpu.reg_read(UC_X86_REG_EIP) != stop or cpu.reg_read(UC_X86_REG_ESP) != stack + 4:
                raise ValueError('Original renderer cache owner lost its caller')
            calls.append(dict(entry=entry, original_instructions=len(instructions) - initial_count,
                returned_gp=[cpu.reg_read(register) for register in (UC_X86_REG_EAX, UC_X86_REG_ECX,
                    UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP,
                    UC_X86_REG_ESI, UC_X86_REG_EDI)], returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS)))
        if [row['requested_bytes'] for row in allocations] != [0x188c, 0xb68, 0x30fc, 0x508c, 0x6a8, 0x30fc]:
            raise ValueError('Original renderer cache allocation requests differ')
        if [word(cell) for cell in bindings] != [row['source'] for row in allocations]:
            raise ValueError('Original renderer cache pool bindings differ')
        stem = f'fill-{fill:02x}'
        store_file = f'{stem}-stores.bin'
        (output / store_file).write_bytes(b''.join(struct.pack('<5I', *row) for row in stores))
        for index, ((base, count, _), row) in enumerate(zip(pages, allocations)):
            row['storage'] = f'{stem}-allocation-{index}.bin'
            (output / row['storage']).write_bytes(cpu.mem_read(base, count))
        return dict(declared_allocation_fill=fill, allocations=allocations, calls=calls,
                    ordered_resource_stores=store_file, resource_store_count=len(stores),
                    original_instructions=len(instructions), binding_cells=bindings,
                    callbacks_executed=False)
    finally:
        for base, count, backing in pages:
            cpu.mem_unmap(base, count)
            backing.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve earlier evidence; select a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    for address, count in ((0x4f2d24, 8), (0x4f2d34, 4), (0x4f2d5c, 8), (0x4f2d6c, 4), (0x4c3ad4, 16)):
        if any(image.read(address, count)):
            raise ValueError('Original renderer cache cold extent is not loader-zeroed')
    literals = [(0x45f218, 20, b'xbox texture cache'), (0x45f22c, 24, b'xbox predicted texture'),
        (0x45f244, 16, b'xbox texture'), (0x45f254, 20, b'xbox geometry cache'),
        (0x45f268, 24, b'xbox predicted geometry'), (0x45f280, 16, b'xbox geometry')]
    for address, count, literal in literals:
        if image.read(address, count) != literal.ljust(count, b'\0'):
            raise ValueError('Original renderer cache name or alignment padding differs')
    for address, raw in ((0x474ce8, struct.pack('<I', 0x44abec)), (0x44abec, bytes(12)),
                         (0x41b6d4, struct.pack('<I', 0x800000b8))):
        if image.read(address, len(raw)) != raw:
            raise ValueError('Original renderer success suffix vector or allocation import differs')
    args.output.mkdir(parents=True)
    executed = {}
    cases = [run(image, fill, executed, args.output) for fill in (0, 0x5a, 0xa5)]
    report = dict(scope=__doc__, passed=True, original_xbe_sha256=NEW_PIN,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=cases,
        executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
        entire_initializer32_verified=False, gpu_callback_or_frame_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original renderer cache pools: {len(cases)} fills; {len(executed)} unchanged instruction sites')


if __name__ == '__main__':
    main()
