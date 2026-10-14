#!/usr/bin/env python3
"""Run complete untouched TU1.5 136E10 over declared visibility-row inputs."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

ENTRY, BANK, STACK, STOP = 0x136e10, 0x2100000, 0x2110000, 0x3000000
REGISTERS = [getattr(R, 'UC_X86_REG_' + name)
             for name in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert not args.output.exists(), 'Preserve previous original execution evidence'
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    assert image.read(0x1372c5, 5) == bytes.fromhex('e846fbffff')
    args.output.mkdir(parents=True)
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x136000, 0x1000)
    cpu.mem_write(0x136000, image.read(0x136000, 0x1000))
    cpu.mem_map(BANK, 0x3000)
    cpu.mem_map(STACK - 0x1000, 0x2000)
    cpu.mem_map(STOP, 0x1000)
    before = bytes((n * 73 + 17) & 255 for n in range(0x3000))
    gp = [0x11223344, 0x55667788, BANK, 0xaabbccdd, STACK,
          0x13579bdf, 0x2468ace0, 0xfedcba98]
    xmm = [[0x7f800001 + n * 4 + lane for lane in range(4)] for n in range(8)]
    trace, stores, instructions = [], [], {}

    def instruction(uc, address, size, _):
        raw = bytes(uc.mem_read(address, size))
        assert raw == image.read(address, size)
        instructions[f'{address:08x}'] = raw.hex()
        trace.append(address)

    def write(uc, access, address, size, value, _):
        old = bytes(uc.mem_read(address, size))
        new = (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little')
        stores.append([uc.reg_read(R.UC_X86_REG_EIP), address, old.hex(), new.hex()])

    cpu.hook_add(UC_HOOK_CODE, instruction)
    cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    fixture = args.output / 'cases.jsonl'
    cases = 0
    with fixture.open('w') as output:
        for cursor in (0, 1, 127, 511, 512, 513, 0x7fff, 0xffff):
            for bit in range(128):
                cpu.mem_write(BANK, before)
                cpu.mem_write(BANK + 0xa5e, struct.pack('<H', cursor))
                cpu.mem_write(STACK - 0x1000, bytes(0x2000))
                cpu.mem_write(STACK, struct.pack('<II', STOP, bit))
                for register, value in zip(REGISTERS, gp):
                    cpu.reg_write(register, value)
                for n, values in enumerate(xmm):
                    cpu.reg_write(getattr(R, f'UC_X86_REG_XMM{n}'),
                                  int.from_bytes(struct.pack('<4I', *values), 'little'))
                cpu.reg_write(R.UC_X86_REG_EFLAGS, 2)
                trace.clear()
                stores.clear()
                cpu.emu_start(ENTRY, STOP, count=1000)
                assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP
                assert cpu.reg_read(R.UC_X86_REG_ESP) == STACK + 8
                # Confirm the complete after-bank can be reconstructed solely
                # from the observed original stores; no output algorithm is supplied.
                expected = bytearray(before)
                struct.pack_into('<H', expected, 0xa5e, cursor)
                for _, address, _, raw in stores:
                    value = bytes.fromhex(raw)
                    if BANK <= address and address + len(value) <= BANK + len(expected):
                        expected[address - BANK:address - BANK + len(value)] = value
                assert bytes(expected) == bytes(cpu.mem_read(BANK, len(expected)))
                row = dict(cursor=cursor, bit=bit, gp=gp, xmm=xmm,
                    original=dict(gp=[cpu.reg_read(reg) for reg in REGISTERS],
                        eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                        instructions=list(trace), writes=list(stores)))
                output.write(json.dumps(row, separators=(',', ':')) + '\n')
                cases += 1
    report = dict(passed=True, cases=cases, original_xbe_sha256=NEW_PIN,
        entry=ENTRY, original_call_site=0x1372c5, supplied_game_services=[],
        instruction_patches=[], executed_instruction_bytes=instructions,
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
        scope='Complete original visibility-row allocation over declared cursor/128-bit inputs; all writes, paths, register state and complete retained bank checked. Live view production and graphics submission are separate.')
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', cases, 'complete original visibility-row calls')


if __name__ == '__main__':
    main()
