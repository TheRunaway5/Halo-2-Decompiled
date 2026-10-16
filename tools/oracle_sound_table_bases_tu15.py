#!/usr/bin/env python3
"""Offline original table indexing and physical-base requests only.

The bounded38E0DB..38E131 interval starts AFTER the hardware FIFO poll and
stops BEFORE notifier/method configuration. No FIFO value, native hardware
bank, whole38E0CE return, voice processor or initializer success is supplied.
The descriptors come from the earlier authenticated SDK storage reference.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R


def run(image, storage, fill):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for address, size in [(0x10000, 0x600000), (0x2000000, 0x20000),
                          (0xfe800000, 0x3000)]:
        cpu.mem_map(address, size)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    for row in storage['regions']:
        cpu.mem_write(row['address'], bytes.fromhex(row['hex']))
    for row in storage['dma_allocations']:
        cpu.mem_map(row['source'], row['capacity'])
        cpu.mem_write(row['source'], bytes([fill]) * row['capacity'])
    registers = [getattr(R, 'UC_X86_REG_' + name)
                 for name in ['EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI']]
    gp = [0x11000000 + i * 0x111111 for i in range(8)]
    gp[4] = 0x2008000
    for register, value in zip(registers, gp):
        cpu.reg_write(register, value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    instructions, stores, executed = [], [], {}

    def instruction(m, pc, size, _):
        raw = bytes(m.mem_read(pc, size))
        assert raw == image.read(pc, size)
        assert executed.setdefault(f'{pc:08x}', raw.hex()) == raw.hex()
        instructions.append(pc)

    def write(m, access, address, size, value, _):
        stores.append([m.reg_read(R.UC_X86_REG_EIP), address, size,
                       bytes(m.mem_read(address, size)).hex(),
                       (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little').hex()])

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    cpu.emu_start(0x38e0db, 0x38e131, count=10000)
    assert cpu.reg_read(R.UC_X86_REG_EIP) == 0x38e131
    rows = storage['dma_allocations']
    voice = next(row for row in rows if row['source_descriptor'] == 0x391390)
    for index in range(256):
        assert struct.unpack('<I', cpu.mem_read(voice['source'] + index * 128 + 124, 4))[0] == index
    expected = [(0x38e104, 0xfe80202c, 0x391390),
                (0x38e10e, 0xfe802038, 0x3913d0),
                (0x38e118, 0xfe80203c, 0x3913e0),
                (0x38e122, 0xfe802030, 0x3913b0),
                (0x38e12c, 0xfe802034, 0x3913c0)]
    publications = [row for row in stores if row[1] >= 0xfe800000]
    assert len(publications) == 5
    for publication, (pc, address, descriptor) in zip(publications, expected):
        allocation = next(row for row in rows if row['source_descriptor'] == descriptor)
        assert publication[0:3] == [pc, address, 4]
        assert int.from_bytes(bytes.fromhex(publication[4]), 'little') == allocation['physical']
    return dict(fill=fill, entry=0x38e0db, end_exclusive=0x38e131,
                declared_input_gp=gp, instructions=instructions, stores=stores,
                physical_base_publications=publications,
                returned_gp=[cpu.reg_read(reg) for reg in registers],
                returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                voice_index_stores=256, executed_instruction_bytes=executed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--storage-reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    reference = json.loads(args.storage_reference.read_text())
    assert reference['passed'] and reference['original_xbe_sha256'] == NEW_PIN
    assert not reference['instruction_patches'] and not reference['supplied_game_or_sdk_results']
    storage = next(case for case in reference['cases'] if case['through_storage'])
    assert len(storage['dma_allocations']) == 16
    cases = [run(image, storage, fill) for fill in [0, 0x5a, 0xa5]]
    report = dict(schema='H2_TU15_TABLE_BASE_REFERENCE_ONLY_V1', passed=True, scope=__doc__,
                  original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  storage_reference_sha256=hashlib.sha256(args.storage_reference.read_bytes()).hexdigest(),
                  cases=cases, instruction_patches=[], supplied_game_or_sdk_results=[],
                  native_register_bank=False, supplied_fifo_space=False,
                  whole38e0ce_returned=False, initializer45_complete=False,
                  native_voice_or_sge_processing=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'{len(cases)} original bounded indexing/base request cases; no FIFO or whole function result')


if __name__ == '__main__':
    main()
