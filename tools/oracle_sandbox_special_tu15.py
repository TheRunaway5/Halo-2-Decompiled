#!/usr/bin/env python3
"""Execute unchanged TU1.5 special movement with explicit raw ABI inputs."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code
from generate_physics_live_support_tu15 import complete_body
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_sandbox_special_tu15 import ENTRIES
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    constant_pages = set()
    for entry in ENTRIES:
        for pc in complete_body(code, entry).code:
            for operand in code.at(pc).operands:
                if operand.type == 3 and not operand.mem.base and not operand.mem.index:
                    address = operand.mem.disp & 0xffffffff
                    if 0x10000 <= address < 0x800000:
                        constant_pages.add(address & ~4095)
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    machine.mem_map(0x1000000, 0x10000)
    registers = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
        UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    trace = []
    machine.hook_add(UC_HOOK_CODE, lambda _, pc, size, user: trace.append(pc))
    read_pages, image_writes = set(), []
    machine.hook_add(UC_HOOK_MEM_READ, lambda _, access, address, size, value, user:
        read_pages.add(address & ~4095) if 0x10000 <= address < 0x800000 else None)
    machine.hook_add(UC_HOOK_MEM_WRITE, lambda _, access, address, size, value, user:
        image_writes.append(address) if 0x10000 <= address < 0x800000 else None)
    def word(address, value):
        machine.mem_write(address, struct.pack('<I', value & 0xffffffff))
    def floats(address, *values):
        machine.mem_write(address, struct.pack('<'+'f'*len(values), *values))
    state, parameters, output, clock, platforms = (0x1001000, 0x1002000,
        0x1003000, 0x1004000, 0x1005000)
    spans = [(p, 4096) for p in sorted(constant_pages)] + [
        (state, 0x140), (parameters, 0x60), (output, 0x30),
        (clock, 0x24), (platforms, 0x1d84), (0x100e000, 0x100c)]
    cases = []
    for active, parent, flags, magnitude, distance in itertools.product(
            (0, 1), (0xffffffff, 0x12340002, 0x12340003), (0, 1),
            (0., .00001, .01, .25, 1., 2., 8.), (.25, 1., 2.4, 8.)):
        machine.mem_write(0x1000000, bytes(0x10000))
        word(0x51df54, clock)
        word(0x51daf0, platforms)
        word(clock, 30 << 16)
        floats(clock+4, 1/30)
        word(state+0xc, parent)
        machine.mem_write(state+0xbc, bytes([active]))
        floats(state+0xc0, 0., 0., 0.)
        floats(state+0xcc, magnitude, -.25*magnitude, .125*magnitude)
        floats(state+0xd8, distance)
        word(state+0xe8, 0xa5000004)
        floats(state+0x118, 2.4, .1, 0.)
        floats(state+0x124, .125, -.25, .0625)
        machine.mem_write(parameters+0xc, bytes([flags, flags]))
        floats(parameters+0x48, 0. if flags else 1.)
        word(platforms+0x1d80, 1)
        word(platforms+0x1d00, 0x12340003)
        floats(platforms+0x24, .1, -.2, .3)
        machine.mem_write(output, b'\xa5'*0x30)
        machine.mem_write(0x100e000, b'\xa5'*0x100c)
        word(0x100f000, 0x11000000)
        word(0x100f004, parameters)
        word(0x100f008, output)
        gp = [state, 0x12345678, 0x13572468, 0x89abcdef,
              0x100f000, 0x55667788, 0x66778899, 0x778899aa]
        for register, value in zip(registers, gp):
            machine.reg_write(register, value)
        for n in range(8):
            machine.reg_write(UC_X86_REG_XMM0+n, 0)
        machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
        machine.reg_write(UC_X86_REG_FPCW, 0x27f)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        initial = [dict(address=p, hex=bytes(machine.mem_read(p, n)).hex()) for p, n in spans]
        trace.clear();read_pages.clear();image_writes.clear()
        machine.emu_start(0x1f39d0, 0x11000000, count=10000)
        assert machine.reg_read(UC_X86_REG_EIP) == 0x11000000
        assert not image_writes
        # Indirect constant/table reads are original data too. These pages
        # remained unchanged, so append their exact bytes to both snapshots.
        actual_spans = spans + [(page, 4096) for page in sorted(read_pages-constant_pages)]
        initial += [dict(address=page, hex=bytes(machine.mem_read(page, 4096)).hex())
                    for page in sorted(read_pages-constant_pages)]
        cases.append(dict(entry=0x1f39d0, controls=dict(active=active, parent=parent,
            flags=flags, magnitude=magnitude, distance=distance), entry_gp=gp,
            initial_banks=initial, exit_gp=[machine.reg_read(r) for r in registers],
            exit_xmm=[f'{machine.reg_read(UC_X86_REG_XMM0+n):032x}' for n in range(8)],
            eflags=machine.reg_read(UC_X86_REG_EFLAGS), instructions=trace.copy(),
            final_banks=[dict(address=p, hex=bytes(machine.mem_read(p, n)).hex()) for p, n in actual_spans]))
    result = dict(scope=__doc__, original_sha256=NEW_PIN, entry=0x1f39d0,
        boundaries=['Explicit raw ABI controls, not a live object or Xbox movement capture.',
                    'All four original bodies and their actual callees execute without substituted returns.'],
        cases=cases)
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(result, separators=(',', ':'))+'\n')
    print(f'PASS unchanged TU1.5 special movement: {len(cases)} cases')


if __name__ == '__main__':
    main()
