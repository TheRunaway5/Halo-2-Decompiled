#!/usr/bin/env python3
"""Independently execute unchanged CRT381390, then the TU1.5 slots46..68 suffix.

This explicitly declared component suffix is NOT a cold startup that skips45.
One actual native64 arena and one raw allocator are retained across all23 slots.
The genuine initializer-table loop13A766..13A775 runs unchanged and ends before
the13A777 epilogue, with ESI990. Prior CRT construction occurs before the suffix
and has its own declared caller ABI. Only real raw pages and the authored CRT
formatting platform contract cross the reference execution boundary.
"""
import argparse
import ctypes
import hashlib
import json
import mmap
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *

GP = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
      UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
XMM = [UC_X86_REG_XMM0 + i for i in range(8)]
ARENA, ARENA_BYTES, OBJECT = 0x81000000, 0x3fe000, 0x2001000
STACK, STOP, PRIOR_RENDERER = 0x2008000, 0x2100000, 0x12000000
ENTRIES = [0x2131b0, 0x18b5c0, 0x22c440, 0x2293e0, 0x1563a0,
           0x14a1ff, 0x1410a7, 0x31d400, 0x13e9e0, 0x2129d0, 0x2035b0,
           0x31d400, 0xd7320, 0x1cfa40, 0x1cb130, 0x31d400, 0x31d400,
           0x31d400, 0x219080, 0x137870, 0x31d400, 0x22c980, 0x31d400]


def regions(values):
    rows = []
    for address in sorted(values):
        if not rows or address != rows[-1][0] + rows[-1][1]:
            rows.append([address, 1])
        else:
            rows[-1][1] += 1
    return rows


def run(image, fill, executed):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(STOP, 0x1000)
    cpu.mem_map(0x80000000, 0x1000)
    pages, allocations, instructions, stores, formats = [], [], [], [], []
    accessed, written, steps, simd_precision_samples = {}, {}, [], []
    simd_pending = None
    next_source = 0x10000000
    active = None

    def backing(source, size):
        raw = mmap.mmap(-1, size, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                        prot=mmap.PROT_READ | mmap.PROT_WRITE)
        raw[:] = bytes([fill]) * size
        native = ctypes.addressof(ctypes.c_char.from_buffer(raw))
        assert native > 0xffffffff
        cpu.mem_map_ptr(source, size, 3, native)
        pages.append((source, size, raw))
        return native

    native_arena = backing(ARENA, ARENA_BYTES)
    backing(PRIOR_RENDERER, 0x46000)
    put = lambda p, v: cpu.mem_write(p, struct.pack('<I', v))
    word = lambda p: struct.unpack('<I', cpu.mem_read(p, 4))[0]
    put(0x51df2c, OBJECT)
    put(OBJECT, 0x45f1d8)
    put(0x51df44, PRIOR_RENDERER)
    put(0x4f2950, ARENA)
    put(0x4f2954, 0x100)
    put(0x4f2958, 0x200)
    put(0x4f295c, 0x12345678)
    put(0x474c20, 0xffffffff)
    assert word(0x4ecc20) == 0 and word(0x46da0c) == 0x381390
    assert word(0x45f1d8) == 0x127210 and word(0x45f048) == 0x11f3f0
    assert word(0x474cbc) == 0x486038
    for slot, entry in enumerate(ENTRIES, 46):
        assert word(0x44aed0 + (slot - 1) * 0x24) == entry
    cpu.mem_write(0x56e225, b'\0')
    cpu.mem_write(0x56e258, bytes([fill]) * 0x400)

    def seed():
        for i, reg in enumerate(GP):
            cpu.reg_write(reg, 0x11000000 + i * 0x111111)
        for i, reg in enumerate(XMM):
            lanes = [0x3f000000 + i * 0x10000 + lane * 0x100 for lane in range(4)]
            cpu.reg_write(reg, sum(v << (32 * lane) for lane, v in enumerate(lanes)))
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.reg_write(UC_X86_REG_FPCW, 0x27f)
        cpu.reg_write(UC_X86_REG_MXCSR, 0x1f80)

    def abi():
        return dict(gp=[cpu.reg_read(r) & 0xffffffff for r in GP],
                    xmm=[[cpu.reg_read(r) >> (32 * lane) & 0xffffffff for lane in range(4)] for r in XMM],
                    eflags=cpu.reg_read(UC_X86_REG_EFLAGS),
                    x87_status=cpu.reg_read(UC_X86_REG_FPSW),
                    x87_control=cpu.reg_read(UC_X86_REG_FPCW),
                    mxcsr=cpu.reg_read(UC_X86_REG_MXCSR))

    def state():
        return dict(abi=abi(), arena_cursor=word(0x4f2954), arena_top=word(0x4f2958), crc=word(0x4f295c))

    def string(p):
        raw = bytearray()
        for i in range(256):
            b = cpu.mem_read(p + i, 1)[0]
            if not b:
                return bytes(raw)
            raw.append(b)
        raise ValueError('CRT source name exceeds declared owner')

    def track(m, access, address, size, value, _):
        if 0x410000 <= address and address + size <= 0x600000 or 0x1afb48 <= address < 0x1afb5c:
            for p in range(address, address + size):
                accessed[p] = image.read(p, 1)[0]

    def write(m, access, address, size, value, _):
        before = bytes(m.mem_read(address, size)).hex()
        track(m, access, address, size, value, _)
        if 0x410000 <= address and address + size <= 0x600000:
            for p in range(address, address + size):
                written[p] = image.read(p, 1)[0]
        stores.append([m.reg_read(UC_X86_REG_EIP), address, size, before,
                       (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()])

    def hook(m, pc, count, _):
        nonlocal next_source, active, simd_pending
        if pc == 0x800000b8:
            sp = m.reg_read(UC_X86_REG_ESP)
            returned, base_cell, zero, size_cell, kind, protection = struct.unpack('<6I', m.mem_read(sp, 24))
            size = word(size_cell)
            assert returned == 0x33b3c5 and not word(base_cell) and zero == 0 and kind == 0x101000 and protection == 4
            rounded = (size + 4095) & ~4095
            base = next_source
            native = backing(base, rounded)
            next_source += rounded
            put(base_cell, base)
            put(size_cell, rounded)
            allocations.append(dict(source=base, native=native, requested=size, reserved=rounded))
            m.reg_write(UC_X86_REG_EAX, 0)
            m.reg_write(UC_X86_REG_ESP, sp + 24)
            m.reg_write(UC_X86_REG_EIP, returned)
            return
        if pc == 0x32ba80:
            sp = m.reg_read(UC_X86_REG_ESP)
            returned, dst, capacity, fmt, args = struct.unpack('<5I', m.mem_read(sp, 20))
            assert returned == 0x11f3a4 and capacity == 255 and fmt == 0x466e00 and string(fmt) == b'%s reference'
            raw = string(word(args)) + b' reference'
            assert len(raw) < capacity
            m.mem_write(dst, raw + b'\0')
            formats.append(dict(destination=dst, bytes=(raw + b'\0').hex(), count=len(raw)))
            m.reg_write(UC_X86_REG_EAX, len(raw))
            m.reg_write(UC_X86_REG_ESP, sp + 4)
            m.reg_write(UC_X86_REG_EIP, returned)
            return
        raw = bytes(m.mem_read(pc, count))
        assert image.read(pc, count) == raw, (hex(pc), raw.hex())
        assert executed.setdefault(f'{pc:08x}', raw.hex()) == raw.hex()
        if pc == 0x29bf64:
            assert raw.hex() == 'f30f59e9'
            simd_pending = dict(instruction=pc, instruction_hex=raw.hex(),
                left=[m.reg_read(UC_X86_REG_XMM5) >> (32 * lane) & 0xffffffff for lane in range(4)],
                right=[m.reg_read(UC_X86_REG_XMM1) >> (32 * lane) & 0xffffffff for lane in range(4)],
                before_mxcsr=m.reg_read(UC_X86_REG_MXCSR))
        if pc == 0x29bf68:
            assert simd_pending is not None
            simd_pending.update(result=[m.reg_read(UC_X86_REG_XMM5) >> (32 * lane) & 0xffffffff for lane in range(4)],
                                after_mxcsr=m.reg_read(UC_X86_REG_MXCSR))
            simd_precision_samples.append(simd_pending)
            simd_pending = None
        if pc == 0x13a766:
            offset = m.reg_read(UC_X86_REG_ESI)
            slot = offset // 0x24 + 1
            assert 46 <= slot <= 68 and offset % 0x24 == 0
            active = dict(slot=slot, entry=word(0x44aed0 + offset), source_offset=offset,
                          start_instructions=len(instructions), start_stores=len(stores), incoming=state())
        if pc == 0x13a76c:
            assert active is not None and m.reg_read(UC_X86_REG_ESP) == STACK
            active.update(returned=state(), instructions=len(instructions) - active.pop('start_instructions'),
                          stores=len(stores) - active.pop('start_stores'))
            steps.append(active)
            active = None
        instructions.append(pc)

    cpu.hook_add(UC_HOOK_CODE, hook)
    cpu.hook_add(UC_HOOK_MEM_READ, track)
    cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    try:
        seed()
        put(STACK, STOP)
        cpu.reg_write(UC_X86_REG_ESP, STACK)
        prior_incoming = state()
        cpu.emu_start(0x381390, STOP, count=2000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == STOP and cpu.reg_read(UC_X86_REG_ESP) == STACK + 4
        prior = dict(entry=0x381390, incoming=prior_incoming, returned=state(),
                     instructions=len(instructions), stores=len(stores))
        # This component's retained startup caller is explicitly declared. CRT
        # machine registers are not erroneously used as later main inputs.
        seed()
        cpu.reg_write(UC_X86_REG_ESP, STACK)
        cpu.reg_write(UC_X86_REG_ESI, 0x654)
        suffix_incoming = state()
        cpu.emu_start(0x13a766, 0x13a777, count=4000000)
        assert cpu.reg_read(UC_X86_REG_EIP) == 0x13a777
        assert cpu.reg_read(UC_X86_REG_ESI) == 0x990 and cpu.reg_read(UC_X86_REG_ESP) == STACK
        assert len(steps) == 23
        assert [r['requested'] for r in allocations] == [0x1880, 0x124c, 0x1450, 0x850, 0x1409b, 0x106470]
        fields = [dict(address=a, bytes=n, initial_hex=image.read(a, n).hex(), hex=bytes(cpu.mem_read(a, n)).hex())
                  for a, n in regions(written)]
        data = [dict(source=a, bytes=n, hex=bytes(cpu.mem_read(a, n)).hex()) for a, n, _ in pages if a != ARENA]
        return dict(fill=fill, declared_input=dict(arena_source=ARENA, arena_bytes=ARENA_BYTES, arena_object=OBJECT,
                    cursor=0x100, top=0x200, crc=0x12345678, scenario_index=0xffffffff, prior_renderer_source=PRIOR_RENDERER,
                    prior_renderer_reserved=0x46000, component_suffix_only=True), native_arena=native_arena,
                    allocations=allocations, prior_static_constructor=prior, suffix_incoming=suffix_incoming,
                    simd_precision_samples=simd_precision_samples,
                    steps=steps, final=state(), fields=fields, allocations_bytes=data, formats=formats,
                    accessed_static_regions=[dict(address=a, bytes=n, initial_hex=image.read(a, n).hex()) for a, n in regions(accessed)],
                    arena_hex=bytes(cpu.mem_read(ARENA, ARENA_BYTES)).hex(),
                    crc_table_hex=bytes(cpu.mem_read(0x56e258, 0x400)).hex(), instructions=len(instructions),
                    source_store_count=len(stores), source_stores=stores,
                    source_store_sha256=hashlib.sha256(json.dumps(stores, separators=(',', ':')).encode()).hexdigest())
    finally:
        for base, size, raw in pages:
            cpu.mem_unmap(base, size)
            raw.close()


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    assert not args.output.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    executed = {}
    cases = [run(image, fill, executed) for fill in [0, 0x5a, 0xa5]]
    args.output.mkdir(parents=True)
    report = dict(schema='H2_ORIGINAL_LATE_STARTUP_COMPONENT_V1', passed=True, scope=__doc__,
                  original_xbe_sha256=NEW_PIN, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  cases=cases, executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
                  skipped_cold_startup_initializers=[], component_suffix_start_slot=46,
                  cold_startup_verified=False, slot45_verified=False, all68_startup_verified=False,
                  gameplay_or_gpu_parity_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(cases)} original component cases; {len(executed)} unchanged instruction sites; authentic finalESI990')


if __name__ == '__main__':
    main()
