#!/usr/bin/env python3
"""Produce bounded numerical fixtures by executing the unchanged TU1.5 methods."""
import argparse
import json
import os
from pathlib import Path
import random
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body, sha
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP,
                              UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EFLAGS,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert os.environ.get('HALO2_TEST_SCRATCH'), 'Use tools/run_test.py'
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    machine.mem_map(0x1000000, 0x10000)
    body, impulse, point, stack, returned = 0x1001000, 0x1001200, 0x1001220, 0x1003000, 0x1100000
    registers = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
                 UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
    trace = []
    machine.hook_add(UC_HOOK_CODE, lambda _machine, address, _size, _data: trace.append(address))
    random_ = random.Random(0x324e60)
    cases = []
    for entry in (0x324e60, 0x324f80):
        original = complete_body(code, entry)
        for ordinal in range(16):
            values = [random_.uniform(-16, 16) for _ in range(68)]
            before = struct.pack('<68f', *values)
            force = struct.pack('<4f', *(random_.uniform(-4, 4) for _ in range(4)))
            position = struct.pack('<4f', *(random_.uniform(-8, 8) for _ in range(4)))
            gp = [0, body, 0, 0, stack, 0, 0, 0]
            machine.mem_write(body, before)
            machine.mem_write(impulse, force)
            machine.mem_write(point, position)
            machine.mem_write(stack, struct.pack('<III', returned, impulse, point))
            for register, value in zip(registers, gp):
                machine.reg_write(register, value)
            machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
            machine.reg_write(UC_X86_REG_FPCW, 0x27f)
            machine.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            trace.clear()
            machine.emu_start(entry, returned, count=1000)
            assert trace == sorted(original.code)
            cases.append(dict(entry=entry, ordinal=ordinal, gp=gp,
                              body_before=before.hex(), impulse=force.hex(), point=position.hex(),
                              body_after=bytes(machine.mem_read(body, 272)).hex(),
                              returned_gp=[machine.reg_read(register) for register in registers],
                              instructions=list(trace)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(dict(reference_sha256=NEW_PIN, cases=cases), indent=2) + '\n')
    print(f'PASS original TU1.5 impulse fixture: {len(cases)} cases, {sum(len(row["instructions"]) for row in cases)} instructions')


if __name__ == '__main__':
    main()
