#!/usr/bin/env python3
"""Run unchanged1E0F0 and its complete transparent-container constructor graph.

Whole140C80/140CF0,11F3F0,33B3AC and32A4E0 run from authenticated TU1.5
instructions. The only supplied boundary is kernel184 NtAllocateVirtualMemory,
which allocates actual native64 mmap pages and publishes logical source tokens.
Allocation fills are declared inputs to prove the source container writes;
this does not claim Xbox kernel initial-byte policy or execute draw callbacks.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_EFLAGS)


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
    put = lambda p, value: cpu.mem_write(p, struct.pack('<I', value))
    word = lambda p: struct.unpack('<I', cpu.mem_read(p, 4))[0]
    put(stack, stop)
    cpu.reg_write(UC_X86_REG_ESP, stack)
    allocations, pages, instructions, source_stores = [], [], [], []

    def hook(machine, pc, count, _):
        nonlocal next_source
        if pc == 0x800000b8:
            sp = machine.reg_read(UC_X86_REG_ESP)
            returned, base_cell, zero_bits, size_cell, kind, protection = struct.unpack(
                '<6I', machine.mem_read(sp, 24))
            size, preferred = word(size_cell), word(base_cell)
            if returned != 0x33b3c5 or preferred or zero_bits or kind != 0x101000 or protection != 4:
                raise ValueError('Original transparent allocation crossed its declared platform ABI')
            rounded = (size + 4095) & ~4095
            backing = mmap.mmap(-1, rounded, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                                prot=mmap.PROT_READ | mmap.PROT_WRITE)
            backing[:] = bytes([fill]) * rounded
            native = ctypes.addressof(ctypes.c_char.from_buffer(backing))
            if native <= 0xffffffff:
                raise ValueError('Transparent reference allocation lacks native64 pages')
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
        if image.read(pc, count) != raw:
            raise ValueError('Original transparent instruction changed')
        if executed.setdefault(f'{pc:08x}', raw.hex()) != raw.hex():
            raise ValueError('Original transparent instruction identity differs')
        instructions.append(pc)

    def store(machine, access, address, size, value, _):
        if (0x4c2aa0 <= address < 0x4c2ab2) or address == 0x52dfd0 or any(
                start <= address < start + count for start, count, _ in pages):
            source_stores.append((machine.reg_read(UC_X86_REG_EIP), address, size,
                int.from_bytes(machine.mem_read(address, size), 'little'), value))

    cpu.hook_add(UC_HOOK_CODE, hook)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    try:
        cpu.emu_start(0x1e0f0, stop, count=1000000)
        if cpu.reg_read(UC_X86_REG_EIP) != stop or cpu.reg_read(UC_X86_REG_ESP) != stack + 4:
            raise ValueError('Original transparent constructor lost its caller')
        if cpu.reg_read(UC_X86_REG_EAX) & 255 != 1:
            raise ValueError('Original transparent constructor returned failure')
        if [row['requested_bytes'] for row in allocations] != [0x1c000, 0x800, 0x1203c]:
            raise ValueError('Original transparent allocation sizes differ')
        planes, indices, queue = (row['source'] for row in allocations)
        if word(0x4c2aa8) != planes or word(0x4c2aac) != indices or word(0x52dfd0) != queue:
            raise ValueError('Original transparent allocation bindings differ')
        if bytes(cpu.mem_read(planes, 0x1c000)) != bytes([fill]) * 0x1c000 or bytes(
                cpu.mem_read(indices, 0x800)) != bytes([fill]) * 0x800:
            raise ValueError('Original transparent constructor wrote unspecified plane/index bytes')
        expected = bytearray(0x1203c)
        expected[:18] = b'transparent planes'
        for offset, value in ((0x20, 0x1000), (0x24, 0x800), (0x28, 0x10),
                              (0x2c, 0x1e8a0), (0x30, 0x1e8d0), (0x34, 0x486034),
                              (0x38, queue + 0x403c + 0x7ff * 0x1c)):
            struct.pack_into('<I', expected, offset, value)
        for index in range(0x800):
            struct.pack_into('<I', expected, 0x403c + index * 0x1c + 8,
                             queue + 0x403c + (index - 1) * 0x1c if index else 0)
        raw = bytes(cpu.mem_read(queue, 0x1203c))
        if raw != bytes(expected):
            raise ValueError('Original transparent container header, hash buckets or free list differs')
        if bytes(cpu.mem_read(0x4c2aa0, 8)) != bytes(8) or cpu.mem_read(0x4c2ab0, 2) != b'\0\1':
            raise ValueError('Original transparent idle counters or flags differ')
        normalized = bytearray(raw)
        struct.pack_into('<I', normalized, 0x38, word(queue + 0x38) - queue)
        for index in range(1, 0x800):
            offset = 0x403c + index * 0x1c + 8
            struct.pack_into('<I', normalized, offset, word(queue + offset) - queue)
        stem = f'fill-{fill:02x}'
        store_file = f'{stem}-stores.bin'
        (output / store_file).write_bytes(b''.join(struct.pack('<5I', *row) for row in source_stores))
        for index, ((base, count, _), row) in enumerate(zip(pages, allocations)):
            row['storage'] = f'{stem}-allocation-{index}.bin'
            (output / row['storage']).write_bytes(cpu.mem_read(base, count))
        gp = [cpu.reg_read(register) for register in (UC_X86_REG_EAX, UC_X86_REG_ECX,
            UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)]
        return dict(declared_allocation_fill=fill, allocations=allocations,
                    returned_gp=gp, returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS),
                    ordered_resource_stores=store_file,
                    original_instructions=len(instructions), original_resource_store_count=len(source_stores),
                    resource_store_sha256=hashlib.sha256(json.dumps(source_stores, separators=(',', ':')).encode()).hexdigest(),
                    source_container_sha256=hashlib.sha256(raw).hexdigest(),
                    normalized_container_sha256=hashlib.sha256(normalized).hexdigest(),
                    header_words=list(struct.unpack('<15I', raw[:0x3c])),
                    bucket_count=0x1000, free_node_count=0x800, node_stride=0x1c,
                    retained_callback_identities=[0x1e8a0, 0x1e8d0], callbacks_executed=False)
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
    for address, count in ((0x4c2aa0, 0x12), (0x52dfd0, 4)):
        if any(image.read(address, count)):
            raise ValueError('Original transparent cold extent is not loader-zeroed')
    if image.read(0x45c548, 20) != b'transparent planes\0\0' or image.read(0x41b6d4, 4) != struct.pack('<I', 0x800000b8):
        raise ValueError('Original transparent name or allocation import differs')
    executed = {}
    args.output.mkdir(parents=True)
    cases = [run(image, fill, executed, args.output) for fill in (0, 0x5a, 0xa5)]
    report = dict(scope=__doc__, passed=True, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=cases,
                  executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
                  entire_initializer32_verified=False, graphics_submission_or_frame_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original transparent storage: {len(cases)} cases; {len(executed)} unchanged instruction sites')


if __name__ == '__main__':
    main()
