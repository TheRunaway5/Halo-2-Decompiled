#!/usr/bin/env python3
"""Lower complete original166C30 and compare declared inputs with unchanged x86.

This is an isolated integer/ABI reference, not live gameplay acceptance.
Use tools/run_test.py; retain only the generated source and small fixture/report.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate

ENTRY = 0x166c30


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    body = _trace(code, ENTRY, {ENTRY})
    assert len(body.code) == 33 and not body.tables
    instructions = [code.at(pc) for pc in sorted(body.code)]
    assert instructions[-1].address == 0x166c8c and instructions[-1].mnemonic == 'ret'
    addresses = {ENTRY: ENTRY}
    for instruction in instructions:
        addresses[instruction.address] = instruction.address
        addresses[instruction.address + instruction.size] = instruction.address + instruction.size
        for operand in instruction.operands:
            if operand.type == 2:
                addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
            elif operand.type == 3:
                addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{ENTRY:08x}': ENTRY}, body_sites={f'{ENTRY:08x}': sorted(body.code)},
                  instruction_map={f'{pc:08x}': pc for pc in body.code},
                  address_map={f'{p:08x}': q for p, q in addresses.items()},
                  routines=[dict(entry=ENTRY, tables=[], byte_tables=[])])
    args.work.mkdir(parents=True, exist_ok=True)
    correspondence = args.work / 'correspondence.json'
    correspondence.write_text(json.dumps(paired, indent=2) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.work / 'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_shadow_array_swap_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_shadow_array_swap_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_shadow_array_swap_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_shadow_array_swap_tu15_owns_entry')
    args.output.write_text(source)

    from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
    from unicorn import x86_const as R
    registers = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
                 R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]
    cases = []
    for ordinal, (left, right, alias) in enumerate(((0, 1, False), (1, 0, False), (1, 1, False), (-1, 2, False),
            (2, -1, False), (-1, -1, False), (0x12340001, 0x56780002, False),
            (3, 2, True), (0, 3, True))):
        cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        cpu.mem_map(0x10000, 0x800000)
        for section in image.sections:
            cpu.mem_write(section.va, image.section_bytes(section))
        cpu.mem_map(0x2400000, 0x10000)
        cpu.mem_map(0x2600000, 0x1000)
        stop = 0x2800000
        cpu.mem_map(stop, 4096)
        descriptor = bytearray(0x40)
        bases = [0x2401020 + n * 0x1000 for n in range(4)]
        for offset, address in zip((12, 8, 16, 20), bases):
            struct.pack_into('<I', descriptor, offset, address)
        if alias:
            struct.pack_into('<I', descriptor, 12, 0x2400000)
        initial = [(0x2400000, bytes(descriptor))]
        for n, address in enumerate(bases):
            initial.append((address - 0x20, bytes((k * 37 + n * 19 + ordinal) & 255 for k in range(0x80))))
        stack = bytearray([0xcd] * 0x80)
        struct.pack_into('<I', stack, 0x20, stop)
        initial.append((0x2600060, bytes(stack)))
        for address, data in initial:
            cpu.mem_write(address, data)
        gp = [0x72000000 + n * 0x1000 + ordinal * 16 for n in range(8)]
        gp[0], gp[1], gp[2], gp[4] = left & 0xffffffff, right & 0xffffffff, 0x2400000, 0x2600080
        for register, value in zip(registers, gp):
            cpu.reg_write(register, value)
        xmm = [[0x12340000 + n * 16 + k for k in range(4)] for n in range(8)]
        for n, lanes in enumerate(xmm):
            cpu.reg_write(R.UC_X86_REG_XMM0 + n, sum(v << (k * 32) for k, v in enumerate(lanes)))
        for register, value in ((R.UC_X86_REG_EFLAGS, 0x202), (R.UC_X86_REG_FPCW, 0x27f),
                                (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                                (R.UC_X86_REG_MXCSR, 0x1f80)):
            cpu.reg_write(register, value)
        trace = []
        def observe(machine, pc, size, _):
            assert bytes(machine.mem_read(pc, size)) == image.read(pc, size)
            trace.append(pc)
        cpu.hook_add(UC_HOOK_CODE, observe)
        cpu.emu_start(ENTRY, stop, count=1000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == stop and trace == sorted(body.code)
        cases.append(dict(left=left, right=right, alias=alias, entry_gp=gp, entry_xmm=xmm,
            exit_gp=[cpu.reg_read(register) for register in registers], eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
            instructions=trace, initial_banks=[dict(address=p, hex=b.hex()) for p, b in initial],
            final_banks=[dict(address=p, hex=bytes(cpu.mem_read(p, len(b))).hex()) for p, b in initial]))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(dict(original_sha256=NEW_PIN, entry=ENTRY, cases=cases),
                                       separators=(',', ':')) + '\n')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(dict(original_sha256=NEW_PIN, entry=hex(ENTRY),
        instructions=len(body.code), cases=len(cases), instruction_patches=[], replaced_services=[],
        generated_cpp_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
        generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        lowerer_sha256=hashlib.sha256((Path(__file__).parent / 'generate_physics_live_support_tu15.py').read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        original_instruction_bytes_sha256=hashlib.sha256(b''.join(bytes(i.bytes) for i in instructions)).hexdigest()),
        indent=2) + '\n')
    print('PASS unchanged original166C30:', len(cases), 'declared signed-index/alias cases')


if __name__ == '__main__':
    main()
