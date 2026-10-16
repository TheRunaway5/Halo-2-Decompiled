#!/usr/bin/env python3
"""Run unchanged renderer CPU owners1FB90/20E30/20E90 and12560's12822 suffix.

The input arena is an explicitly declared actual native64 allocation. The
source CRC owners166A80/166AE0 run unchanged. Earlier renderer constructors,
including1303C0/12EBE0, are outside this bounded probe; it supplies no game
results, resource success, platform calls or instruction patches.
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


def checksum(value, raw):
    for byte in raw:
        value ^= byte
        for _ in range(8):
            value = (value >> 1) ^ (0xedb88320 if value & 1 else 0)
    return value


def run(image, fill, successful_prefix, executed, output):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(0x2100000, 0x1000)
    base, size, cursor = 0x81000000, 0x100000, 0x100
    pages = mmap.mmap(-1, size, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                      prot=mmap.PROT_READ | mmap.PROT_WRITE)
    pages[:] = bytes([fill]) * size
    native = ctypes.addressof(ctypes.c_char.from_buffer(pages))
    if native <= 0xffffffff:
        raise ValueError('Reference renderer tail arena lacks native64 pages')
    cpu.mem_map_ptr(base, size, 3, native)
    stack, stop, seed = 0x2008000, 0x2100000, 0x12345678
    put = lambda p, value: cpu.mem_write(p, struct.pack('<I', value))
    word = lambda p: struct.unpack('<I', cpu.mem_read(p, 4))[0]
    put(0x4f2950, base)
    put(0x4f2954, cursor)
    put(0x4f295c, seed)
    cpu.mem_write(0x52dfd8, bytes([fill]) * 0x40c0)
    # Declared loader state; every table entry is computed by166AE0.
    cpu.mem_write(0x56e225, b'\0')
    cpu.mem_write(0x56e258, bytes([fill]) * 0x400)
    instructions, stores, calls = [], [], []

    def hook(machine, pc, count, _):
        raw = bytes(machine.mem_read(pc, count))
        if image.read(pc, count) != raw:
            raise ValueError('Original renderer tail instruction changed')
        if executed.setdefault(f'{pc:08x}', raw.hex()) != raw.hex():
            raise ValueError('Original renderer tail instruction identity differs')
        instructions.append(pc)

    def store(machine, access, address, count, value, _):
        if (base <= address < base + size or 0x52dfd8 <= address < 0x532098 or
                0x56e258 <= address < 0x56e658 or address in
                (0x4f2954, 0x4f295c, 0x515ec0, 0x515ec4, 0x515e2c, 0x56e225, 0x4922a8)):
            stores.append((machine.reg_read(UC_X86_REG_EIP), address, count,
                int.from_bytes(machine.mem_read(address, count), 'little'), value))

    def execute(entry, frame, expected_stack):
        initial_count = len(instructions)
        cpu.reg_write(UC_X86_REG_ESP, frame)
        cpu.emu_start(entry, stop, count=1000000)
        if cpu.reg_read(UC_X86_REG_EIP) != stop or cpu.reg_read(UC_X86_REG_ESP) != expected_stack:
            raise ValueError('Original renderer tail lost its declared caller frame')
        calls.append(dict(entry=entry, original_instructions=len(instructions) - initial_count,
            returned_gp=[cpu.reg_read(register) for register in (UC_X86_REG_EAX, UC_X86_REG_ECX,
                UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)],
            returned_eflags=cpu.reg_read(UC_X86_REG_EFLAGS)))

    cpu.hook_add(UC_HOOK_CODE, hook)
    cpu.hook_add(UC_HOOK_MEM_WRITE, store)
    try:
        put(stack, stop)
        execute(0x1fb90, stack, stack + 4)
        state_first = base + cursor
        if word(0x515ec0) != state_first or word(0x4f2954) != cursor + 0x3fc:
            raise ValueError('Original1FB90 arena allocation differs')
        first = bytearray(0x3fc)
        for offset in (0x180, 0x184, 0x188, 0x18c, 0x3f8):
            struct.pack_into('<I', first, offset, 0x3f800000)
        if bytes(cpu.mem_read(state_first, 0x3fc)) != bytes(first):
            raise ValueError('Original1FB90 initial state differs')
        # Genuine12822..12891 suffix, entered after the preceding resource
        # owners. Its local success flag is declared input, not a fake return
        # from an unexecuted constructor. Validate both original flag paths.
        for offset, value in ((0, 0xeeeeeeee), (4, 0x51515151), (8, 0xbbbbbbbb), (20, stop)):
            put(stack + offset, value)
        cpu.mem_write(stack + 15, bytes([successful_prefix]))
        cpu.reg_write(UC_X86_REG_EBX, 0)
        execute(0x12822, stack, stack + 24)
        state_render = state_first + 0x3fc
        state_lens = state_render + 0x34
        if word(0x515e2c) != state_render or word(0x515ec4) != state_lens:
            raise ValueError('Original renderer suffix arena binding order differs')
        if word(0x4f2954) != cursor + 0x3fc + 0x34 + 0x24:
            raise ValueError('Original renderer suffix arena footprint differs')
        if bytes(cpu.mem_read(state_render, 0x34)) != bytes([fill]) * 0x34:
            raise ValueError('Original renderer suffix wrote unspecified0x34 bytes')
        lens = bytearray([fill] * 0x24)
        for offset, value in ((0, 0), (1, 0), (0x18, 1), (0x19, 0), (0x1a, 0)):
            lens[offset] = value
        for offset in (4, 8, 0xc, 0x10, 0x14, 0x1c, 0x20):
            struct.pack_into('<I', lens, offset, 0xbf800000)
        if bytes(cpu.mem_read(state_lens, 0x24)) != bytes(lens):
            raise ValueError('Original20E90 specified/unspecified byte behavior differs')
        query = bytearray(0x40c0)
        for index in range(511):
            struct.pack_into('<I', query, 4 + index * 32 + 4, 0xff80)
        if bytes(cpu.mem_read(0x52dfd8, 0x40c0)) != bytes(query):
            raise ValueError('Original query reset record or mask banks differ')
        expected_crc = seed
        for count in (0x3fc, 0x34, 0x24):
            expected_crc = checksum(expected_crc, struct.pack('<I', count))
        if word(0x4f295c) != expected_crc or cpu.mem_read(0x56e225, 1) != b'\1':
            raise ValueError('Original renderer tail arena checksum differs')
        if cpu.mem_read(0x4922a8, 1) != bytes([successful_prefix]):
            raise ValueError('Original renderer suffix admission flag differs')
        stem = f'fill-{fill:02x}-prior-{successful_prefix}'
        store_file = f'{stem}-stores.bin'
        arena_file, query_file, crc_file = f'{stem}-arena.bin', f'{stem}-query.bin', f'{stem}-crc.bin'
        (output / store_file).write_bytes(b''.join(struct.pack('<5I', *row) for row in stores))
        (output / arena_file).write_bytes(cpu.mem_read(state_first, 0x454))
        (output / query_file).write_bytes(cpu.mem_read(0x52dfd8, 0x40c0))
        (output / crc_file).write_bytes(cpu.mem_read(0x56e258, 0x400))
        return dict(fill=fill, declared_prior_success=bool(successful_prefix), native_arena=native,
                    calls=calls, ordered_resource_stores=store_file, resource_store_count=len(stores),
                    arena_storage=arena_file, query_storage=query_file, crc_storage=crc_file,
                    original_instructions=len(instructions), crc=word(0x4f295c),
                    arena_cursor=word(0x4f2954), state_1fb90=state_first,
                    state_12560=state_render, state_20e90=state_lens,
                    state_1fb90_hex=bytes(first).hex(), state_20e90_hex=bytes(lens).hex(),
                    query_bank_sha256=hashlib.sha256(query).hexdigest(),
                    record_resets=instructions.count(0x20e30),
                    initialized=bool(cpu.mem_read(0x4922a8, 1)[0]))
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
    for address, count in ((0x515ec0, 8), (0x515e2c, 4), (0x52dfd8, 0x40c0)):
        if any(image.read(address, count)):
            raise ValueError('Declared renderer tail cold extent is not loader-zeroed')
    if image.read(0x46a0dc, 4).hex() != '000080bf':
        raise ValueError('Original negative-one scalar differs')
    executed = {}
    args.output.mkdir(parents=True)
    cases = [run(image, fill, successful, executed, args.output) for fill in (0, 0x5a, 0xa5)
             for successful in (0, 1)]
    report = dict(scope=__doc__, passed=True, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), cases=cases,
                  executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
                  entire_initializer32_verified=False, gpu_or_frame_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Original renderer CPU tail: {len(cases)} cases; {len(executed)} unchanged instruction sites')


if __name__ == '__main__':
    main()
