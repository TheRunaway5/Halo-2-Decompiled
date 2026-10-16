#!/usr/bin/env python3
"""Run unchanged TU1.5 145E0 with declared SDK surface inputs.

The complete original header constructor, AddRef and allocation wrapper run.
NtAllocateVirtualMemory is the named host-VM boundary: actual anonymous host
pages back successful allocations. Fault injection stops before the unowned
NTSTATUS-to-Win32 translation; no failure BOOL or game result is supplied.
Declared SDK inputs are reference fixtures, never production runtime objects.
This is resource-publication evidence, not a live Xbox draw or frame proof.
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
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP


def run(image, seed, zero_references, fail_allocation, executed):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x600000)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_map(0x1000000, 0x300000)
    cpu.mem_map(0x2000000, 0x20000)
    cpu.mem_map(0x2100000, 0x1000)
    cpu.mem_map(0x80000000, 0x1000)
    stack, stop, device = 0x2008000, 0x2100000, 0x1000000
    surfaces = (0x1100000, 0x1101000, 0x1102000)
    parent, grandparent = 0x1103000, 0x1104000
    common = 0x50000 if zero_references else 0x50001
    data = tuple((0x1200000 + i * 0x100000 + seed * 0x100) for i in range(3))
    before = [[common, data[i], 0xaabb0000 + seed, 0x11229 + seed,
               0x271df27f, parent if zero_references else 0] for i in range(3)]

    def word(address):
        return struct.unpack('<I', cpu.mem_read(address, 4))[0]

    def put(address, value):
        cpu.mem_write(address, struct.pack('<I', value))

    put(0x411568, device)
    for offset, pointer in zip((0x1a14, 0x1a18, 0x1a08), surfaces):
        put(device + offset, pointer)
    for address, words in zip(surfaces, before):
        cpu.mem_write(address, struct.pack('<6I', *words))
    cpu.mem_write(parent, struct.pack('<6I', 0x50000, 0, 0, 0, 0, grandparent))
    cpu.mem_write(grandparent, struct.pack('<6I', 0x50001, 0, 0, 0, 0, 0))
    cpu.mem_write(0x515e34, bytes(32))
    cpu.mem_write(stack, struct.pack('<I', stop))
    cpu.reg_write(UC_X86_REG_ESP, stack)
    allocation_import = struct.unpack('<I', image.read(0x41b6d4, 4))[0]
    error_import = struct.unpack('<I', image.read(0x41b610, 4))[0]
    allocations, mappings, instructions = [], [], []

    def hook(machine, pc, size, _):
        if pc == error_import:
            if fail_allocation is None:
                raise ValueError('Unexpected status-translation boundary')
            machine.emu_stop()
            return
        if pc == allocation_import:
            sp = machine.reg_read(UC_X86_REG_ESP)
            returned_to, base_cell, zero_bits, size_cell, kind, protection = struct.unpack('<6I', machine.mem_read(sp, 24))
            index, requested = len(allocations), word(size_cell)
            expected = (20, 20, 24, 20, 20)
            if (returned_to != 0x33b3c5 or word(base_cell) or zero_bits or
                    kind != 0x101000 or protection != 4 or requested != expected[index]):
                raise ValueError('Original allocation request differs')
            entry = dict(index=index, bytes=requested, kind=kind, protection=protection)
            if index == fail_allocation:
                entry.update(fault_injected=True, status=0xc0000017)
                status = 0xc0000017
            else:
                pages = mmap.mmap(-1, 4096, flags=mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS,
                                  prot=mmap.PROT_READ | mmap.PROT_WRITE)
                native = ctypes.addressof(ctypes.c_char.from_buffer(pages))
                if native <= 0xffffffff:
                    raise ValueError('Actual host allocation is not above 4GiB')
                address = 0x3000000 + index * 4096
                machine.mem_map_ptr(address, 4096, 3, native)
                mappings.append((address, native, pages))
                put(base_cell, address)
                put(size_cell, 4096)
                entry.update(fault_injected=False, status=0, source_token=address, native_backing=native)
                status = 0
            allocations.append(entry)
            machine.reg_write(UC_X86_REG_EAX, status)
            machine.reg_write(UC_X86_REG_ESP, sp + 24)
            machine.reg_write(UC_X86_REG_EIP, returned_to)
            return
        raw = bytes(machine.mem_read(pc, size)).hex()
        if bytes.fromhex(raw) != image.read(pc, size):
            raise ValueError('An original instruction changed')
        if executed.setdefault(f'{pc:08x}', raw) != raw:
            raise ValueError('Original instruction identity differs across cases')
        instructions.append(pc)

    cpu.hook_add(UC_HOOK_CODE, hook)
    try:
        cpu.emu_start(0x145e0, stop, count=10000)
        cells = struct.unpack('<8I', cpu.mem_read(0x515e34, 32))
        completed = cpu.reg_read(UC_X86_REG_EIP) == stop
        headers = {}
        for cell, pointer in zip(range(0x515e34, 0x515e54, 4), cells):
            if pointer and pointer not in surfaces:
                size = 24 if cell == 0x515e50 else 20
                headers[f'{cell:08x}'] = list(struct.unpack('<' + 'I' * (size // 4), cpu.mem_read(pointer, size)))
        if fail_allocation is None:
            if not completed or cpu.reg_read(UC_X86_REG_ESP) != stack + 4 or cpu.reg_read(UC_X86_REG_EAX) & 255 != 1:
                raise ValueError('Original successful constructor did not return true with its ABI')
            expected_cells = (0x3000000, 0x3001000, *surfaces, 0x3003000, 0x3004000, 0x3002000)
            if cells != expected_cells:
                raise ValueError('Original surface publications differ')
            for cell, physical, fmt in ((0x515e34, data[0], 0x11229), (0x515e38, data[1], 0x11229),
                                        (0x515e48, data[2], 0x11229), (0x515e4c, data[2], 0x13f29)):
                if headers[f'{cell:08x}'] != [0x40001, physical, 0, fmt, 0x271df27f]:
                    raise ValueError('Original texture header fields differ')
            expected_surface = before[0].copy()
            expected_surface[0] += 1
            expected_surface[1] = data[2]
            if headers['00515e50'] != expected_surface:
                raise ValueError('Original depth color surface copy differs')
            if any(word(p) != common + 1 for p in surfaces):
                raise ValueError('Original retained surface reference counts differ')
            if zero_references and (word(parent), word(grandparent)) != (0x50003, 0x50002):
                raise ValueError('Original recursive retained parent counts differ')
        elif completed or cpu.reg_read(UC_X86_REG_EIP) != error_import or not allocations[-1]['fault_injected']:
            raise ValueError('Allocation failure did not stop at its unowned status translation')
        for address, native, pages in mappings:
            if bytes(cpu.mem_read(address, 4096)) != ctypes.string_at(native, 4096):
                raise ValueError('Reference writes did not reach their real host allocation')
        return dict(seed=seed, zero_reference_parents=zero_references, failed_allocation=fail_allocation,
                    completed=completed, return_bool=(cpu.reg_read(UC_X86_REG_EAX) & 255) if completed else None,
                    publications=dict(zip((f'{p:08x}' for p in range(0x515e34, 0x515e54, 4)), cells)),
                    headers=headers, allocations=allocations, original_instruction_count=len(instructions),
                    unowned_failure_boundary=f'{error_import:08x}' if not completed else None)
    finally:
        for address, _, pages in mappings:
            cpu.mem_unmap(address, 4096)
            pages.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise ValueError('Preserve existing evidence; choose a new output directory')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Original TU1.5 executable identity differs')
    executed, cases = {}, []
    for seed, zero in ((0, False), (1, False), (255, False), (0, True)):
        cases.append(run(image, seed, zero, None, executed))
    for failed in range(5):
        cases.append(run(image, 0, False, failed, executed))
    args.output.mkdir(parents=True)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  complete_success_cases=4, stopped_allocation_failure_cases=5, cases=cases,
                  executed_instruction_bytes=executed, instruction_patches=[], supplied_game_results=[],
                  declared_platform_boundary='NtAllocateVirtualMemory with real host-backed pages',
                  failure_status_translation_verified=False, live_gpu_or_frame_parity=False, passed=True)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'4 complete original constructors and 5 stopped allocation-failure probes; {len(executed)} unchanged instruction addresses')


if __name__ == '__main__':
    main()
