#!/usr/bin/env python3
"""Compare complete TU1.5 barrel rate and denied admission with raw ABI banks."""
import argparse
import hashlib
import itertools
import json
import struct

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    entries = (0x1071d0, 0x107610)
    allowed = set().union(*(_trace(code, p, set(entries)).code for p in entries))
    machine = Uc(UC_ARCH_X86, UC_MODE_32)
    machine.mem_map(0x10000, 0x800000)
    for section in image.sections:
        machine.mem_write(section.va, image.section_bytes(section))
    machine.mem_map(0x1000000, 0x10000)
    registers = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
                 UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    trace, reads, writes = [], set(), []
    machine.hook_add(UC_HOOK_CODE, lambda _, pc, size, user: trace.append(pc))
    machine.hook_add(UC_HOOK_MEM_READ, lambda _, access, address, size, value, user:
        reads.add(address & ~4095) if address < 0x800000 else None)
    machine.hook_add(UC_HOOK_MEM_WRITE, lambda _, access, address, size, value, user:
        writes.append(address) if address < 0x800000 else None)
    def put(address, value, fmt='I'):
        machine.mem_write(address, struct.pack('<'+fmt, value))
    weapon, definition, barrel, clock = 0x1001000, 0x1002000, 0x1003000, 0x1004000
    pool, headers, tags, output, stack = 0x1005000, 0x1006000, 0x1007000, 0x100b000, 0x100f000
    cases = []
    def capture(entry, controls, index=0):
        put(stack, 0x11000000)
        put(stack+4, 0x12340002)
        put(stack+8, output if entry == 0x1071d0 else index & 0xffffffff)
        put(stack+12, output+4 if entry == 0x1071d0 else 0)
        gp = [0, 0x12345678, 0x13572468, 0x89abcdef, stack, 0x55667788, 0x66778899, 0x778899aa]
        for register, value in zip(registers, gp):
            machine.reg_write(register, value)
        for n in range(8):
            machine.reg_write(UC_X86_REG_XMM0+n, 0)
        machine.reg_write(UC_X86_REG_EFLAGS, 0x202)
        machine.reg_write(UC_X86_REG_FPCW, 0x27f)
        machine.reg_write(UC_X86_REG_FPTAG, 0xffff)
        machine.reg_write(UC_X86_REG_FPSW, 0)
        machine.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        before = bytes(machine.mem_read(0x1000000, 0x10000))
        trace.clear(); reads.clear(); writes.clear()
        machine.emu_start(entry, 0x11000000, count=10000)
        assert machine.reg_read(UC_X86_REG_EIP) == 0x11000000
        assert not writes and set(trace) <= allowed  # No substituted lower call.
        constants = [dict(address=p, hex=bytes(machine.mem_read(p, 4096)).hex()) for p in sorted(reads)]
        cases.append(dict(entry=entry, controls=controls, entry_gp=gp,
            initial_banks=constants+[dict(address=0x1000000, hex=before.hex())],
            final_banks=constants+[dict(address=0x1000000, hex=bytes(machine.mem_read(0x1000000, 0x10000)).hex())],
            exit_gp=[machine.reg_read(r) for r in registers],
            exit_xmm=[f'{machine.reg_read(UC_X86_REG_XMM0+n):032x}' for n in range(8)],
            eflags=machine.reg_read(UC_X86_REG_EFLAGS), instructions=trace.copy()))
    def reset():
        machine.mem_write(0x1000000, bytes(0x10000))
        put(0x4ecbd0, pool); put(pool+0x44, headers)
        put(headers+2*12+8, weapon)
        put(weapon, 0x45670003); put(weapon+0x14, 0xffffffff)
        put(0x4f0414, tags); put(tags+3*16+8, definition)
        put(definition+0x2d0, 1); put(definition+0x2d4, barrel)
        put(0x51df54, clock); put(clock, 30 << 16)
        put(weapon+0x184, .25, 'f')
        machine.mem_write(output, b'\xa5'*8)
    for rate, counter, shots, burst, paired in itertools.product(
            (0., .125, .5, 1., 3., 30., 45., 60.), (-1, 0, 127), (0, 1, 3), (0, 2), (0, 1)):
        reset()
        put(barrel+4, rate, 'f'); put(barrel+8, rate, 'f')
        put(weapon+0x1a4, counter, 'b'); put(weapon+0x1aa, shots, 'H')
        put(barrel+0x1e, burst, 'h')
        if paired:
            put(definition+0x2d0, 2); put(barrel, 1 << 9)
            put(weapon+0x1d9, 1, 'B')
        capture(0x1071d0, dict(rate=rate, counter=counter, shots=shots, burst=burst, paired=paired))
    for denial in ('negative_index', 'past_count', 'barrel_flag', 'barrel_mode', 'overheated', 'empty'):
        reset()
        index = -1 if denial == 'negative_index' else 1 if denial == 'past_count' else 0
        if denial == 'barrel_flag': put(weapon+0x1a8, 4, 'H')
        if denial == 'barrel_mode': put(weapon+0x1a5, 2, 'B')
        if denial == 'overheated': put(weapon+0x16c, 2, 'H')
        if denial == 'empty':
            put(barrel+0x28, 0, 'h'); put(barrel+0x2a, 1, 'h')
            put(definition+0x292, 1, 'h')
        capture(0x107610, dict(denial=denial), index)
    from pathlib import Path
    Path(args.fixture).write_text(json.dumps(dict(original_sha256=NEW_PIN, scope=__doc__,
        boundaries=['Explicit unparented raw weapon ABI, not live original gameplay.',
                    'Complete rate owner and six denied admission paths; positive admission requires live lower owners.'],
        cases=cases), separators=(',', ':'))+'\n')
    print(f'PASS unchanged TU1.5 barrel cases: {len(cases)}')


if __name__ == '__main__':
    main()
