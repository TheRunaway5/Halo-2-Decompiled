#!/usr/bin/env python3
"""Retain bounded ABI cases from unchanged TU1.5 model child-row dispatch.

The original executable is an offline reference only. It supplies no runtime
asset, simulated particle, effect result, native metadata or replacement body.
Run through tools/run_test.py; only the small requested fixture/report remains.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent / 'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EFLAGS


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    image = Xbe(args.xbe)
    pin = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
    assert hashlib.sha256(image.data).hexdigest() == pin
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    cpu.mem_map(0x1000000, 0x10000)
    cpu.mem_map(0x70000000, 4096)
    word = lambda address: struct.unpack('<I', image.read(address, 4))[0]
    assert image.read(0x13a6c2, 6) == bytes.fromhex('81f94d545250')
    assert word(0x486048) == 0x45f2d8 and word(0x45f2d8 + 0x1c) == 0x22b710
    assert image.read(0x22b710, 16) == bytes.fromhex('8b4424048b49086bc03803417cc20400')
    registers = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
                 UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
    initial = (0x11111111, 0x1000400, 0x33333333, 0x44444444,
               0x100f000, 0x66666666, 0x77777777, 0x88888888)
    cases = []
    for index, base in ((0, 0x23450000), (1, 0x23450000), (13, 0x23450000),
                        (65535, 0x23450000), (0xffffffff, 0x23450000), (3, 0xffffffe0)):
        cpu.mem_write(0x1000408, struct.pack('<I', 0x1000800))
        cpu.mem_write(0x100087c, struct.pack('<I', base))
        cpu.mem_write(0x100f000, struct.pack('<2I', 0x70000000, index))
        for register, value in zip(registers, initial):
            cpu.reg_write(register, value)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.emu_start(0x22b710, 0x70000000, count=5)
        cases.append(dict(index=index, base=base,
                          gp=[cpu.reg_read(register) for register in registers],
                          eflags=cpu.reg_read(UC_X86_REG_EFLAGS)))
    assert image.read(0x178097, 6) == bytes.fromhex('81f974647362')
    surface_cases = []
    for index in (0, 1, 7, 65535):
        cpu.mem_write(0x4f0414, struct.pack('<I', 0x1002000))
        cpu.mem_write(0x1002010, struct.pack('<4I', 0x62736474, 0x10001, 0x1003000, 0x1c))
        cpu.mem_write(0x1003018, struct.pack('<I', 0x23450000))
        cpu.mem_write(0x100100e, struct.pack('<H2I', index, 0x10001, 0xffffffff))
        cpu.mem_write(0x100f000, struct.pack('<I', 0x70000000))
        for register, value in zip(registers, initial):
            cpu.reg_write(register, 0x1001000 if register == UC_X86_REG_ECX else value)
        cpu.reg_write(UC_X86_REG_EFLAGS, 0x202)
        cpu.emu_start(0x178070, 0x70000000, count=50)
        surface_cases.append(dict(index=index, gp=[cpu.reg_read(register) for register in registers],
                                  eflags=cpu.reg_read(UC_X86_REG_EFLAGS)))
    text = '// Unchanged TU1.5 ' + pin + ' complete22B710; offline ABI reference.\n'
    text += 'struct ModelChildOriginalCase { unsigned index,base; std::array<unsigned,8> gp; unsigned eflags; };\n'
    text += 'constexpr std::array<ModelChildOriginalCase,6> model_child_original_cases{{\n'
    for case in cases:
        text += '{' + f'0x{case["index"]:x}u,0x{case["base"]:x}u,' + '{{'
        text += ','.join(f'0x{value:x}u' for value in case['gp']) + '}' * 2
        text += f',0x{case["eflags"]:x}u' + '},\n'
    text += '}};\n'
    text += 'struct SurfaceChildOriginalCase { unsigned index; std::array<unsigned,8> gp; unsigned eflags; };\n'
    text += 'constexpr std::array<SurfaceChildOriginalCase,4> surface_child_original_cases{{\n'
    for case in surface_cases:
        text += '{' + f'0x{case["index"]:x}u,' + '{{'
        text += ','.join(f'0x{value:x}u' for value in case['gp']) + '}' * 2
        text += f',0x{case["eflags"]:x}u' + '},\n'
    text += '}};\n'
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(text)
    report = dict(passed=True, cases=cases, breakable_surface_cases=surface_cases, original_sha256=pin,
                  body='22B710', instruction_sites=[0x22b710, 0x22b714, 0x22b717, 0x22b71a, 0x22b71d],
                  model_object=0x486048, model_vtable=0x45f2d8, slot=0x1c,
                  original_instructions_modified=False, simulated_particles=0,
                  fixture_sha256=hashlib.sha256(text.encode()).hexdigest(), large_artifacts_retained=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS six model and four breakable-surface unchanged-original child-row ABI cases')


if __name__ == '__main__':
    main()
