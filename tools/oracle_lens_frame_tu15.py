#!/usr/bin/env python3
"""Execute unchanged TU1.5 lens frame/view gates and occupied-slot retirement.

Frame stamps, render mode, view identity and retained records are declared
caller inputs. No original code, call or GPU result is replaced. These fixtures
do not establish the native display-stamp publisher or GPU query lifecycle.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture', type=Path, required=True)
    p.add_argument('--report', type=Path, required=True)
    a = p.parse_args()
    assert not a.fixture.exists() and not a.report.exists(), 'Preserve earlier evidence'
    x = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(x.data).hexdigest() == NEW_PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x800000)
    for section in x.sections:
        u.mem_write(section.va, x.section_bytes(section))
    original = bytes(u.mem_read(0x10000, 0x800000))
    stack, stop = 0x2600000, 0x2800000
    u.mem_map(stack, 0x8000)
    u.mem_map(stop, 0x1000)
    regs = [R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX,
            R.UC_X86_REG_EBX, R.UC_X86_REG_ESP, R.UC_X86_REG_EBP,
            R.UC_X86_REG_ESI, R.UC_X86_REG_EDI]
    trace = []

    def instruction(cpu, pc, size, _):
        assert bytes(cpu.mem_read(pc, size)) == x.read(pc, size)
        trace.append(pc)

    u.hook_add(UC_HOOK_CODE, instruction)
    cases = []
    for mode in (0, 3, 4, 7, 8):
        for stamp, retained in ((-1, 0), (0, 0), (1, 0), (5, 5), (6, 5), (4, 5)):
            cases.append(dict(entry=0x20680, mode=mode, stamp=stamp, retained=retained,
                              view=0, enabled=1, suppression=0, occupied=3))
    for view in (-1, 0, 3):
        for enabled, suppression, occupied in ((1, 0, 3), (0, 0, 3), (1, 1, 3), (0, 1, 0)):
            cases.append(dict(entry=0x20700, mode=0, stamp=5, retained=5,
                              view=view, enabled=enabled, suppression=suppression,
                              occupied=occupied))
    rows, banks, bank_ids = [], [], {}

    def reference(address, data):
        key = address, data.hex()
        if key not in bank_ids:
            bank_ids[key] = len(banks)
            banks.append(dict(address=address, hex=key[1]))
        return bank_ids[key]

    for case in cases:
        u.mem_write(0x10000, original)
        # Retained records carry independent keys, flags and all three query
        # completion bitmaps so a reset must retire their actual full state.
        pool = bytearray((n * 13 + 7) & 255 for n in range(0x6000))
        for slot in range(511):
            struct.pack_into('<II', pool, 0xfdc + slot * 32, slot * 8 + 1, 0xff83)
        struct.pack_into('<I', pool, 0x4fbc, case['occupied'])
        pool[0xfd8] = case['enabled']
        struct.pack_into('<i', pool, 0x5080, case['retained'])
        struct.pack_into('<i', pool, 0x5094, 2)
        globals = bytearray(u.mem_read(0x491000, 0x2000))
        struct.pack_into('<i', globals, 0x18e8 - 0x1000, case['suppression'])
        struct.pack_into('<i', globals, 0x20b8 - 0x1000, case['mode'])
        struct.pack_into('<i', globals, 0x22c0 - 0x1000, case['stamp'])
        scratch = bytearray([0xcd]) * 0x8000
        struct.pack_into('<I', scratch, 0x4000, stop)
        initial = [(0x491000, bytes(globals)), (0x52d000, bytes(pool)), (stack, bytes(scratch))]
        for address, data in initial:
            u.mem_write(address, data)
        gp = [0x72000000 + n * 0x1000 for n in range(8)]
        gp[0], gp[4] = case['view'] & 0xffffffff, stack + 0x4000
        for reg, value in zip(regs, gp):
            u.reg_write(reg, value)
        for n in range(8):
            u.reg_write(R.UC_X86_REG_XMM0 + n, 0)
        for reg, value in ((R.UC_X86_REG_EFLAGS, 0x202), (R.UC_X86_REG_FPCW, 0x27f),
                           (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff),
                           (R.UC_X86_REG_MXCSR, 0x1f80)):
            u.reg_write(reg, value)
        trace.clear()
        u.emu_start(case['entry'], stop, count=100000)
        assert u.reg_read(R.UC_X86_REG_EIP) == stop
        final = [(0x491000, bytes(u.mem_read(0x491000, 0x2000))),
                 (0x52d000, bytes(u.mem_read(0x52d000, 0x6000))),
                 (stack + 0x4000, bytes(u.mem_read(stack + 0x4000, 4)))]
        rows.append(dict(**case, entry_gp=gp, exit_gp=[u.reg_read(reg) for reg in regs],
                         eflags=u.reg_read(R.UC_X86_REG_EFLAGS), eflags_mask=0xcc5,
                         exit_xmm=[f'{u.reg_read(R.UC_X86_REG_XMM0+n):032x}' for n in range(8)],
                         x87_status=u.reg_read(R.UC_X86_REG_FPSW), mxcsr=u.reg_read(R.UC_X86_REG_MXCSR),
                         instructions=trace.copy(),
                         initial_banks=[reference(p, b) for p, b in initial],
                         final_banks=[reference(p, b) for p, b in final]))
    assert any(0x20e30 in r['instructions'] for r in rows)
    a.fixture.parent.mkdir(parents=True, exist_ok=True)
    a.report.parent.mkdir(parents=True, exist_ok=True)
    a.fixture.write_text(json.dumps(dict(entry=0x20680, original_sha256=NEW_PIN,
                                        banks=banks, cases=rows), separators=(',', ':')) + '\n')
    a.report.write_text(json.dumps(dict(passed=True, cases=len(rows),
        original_sha256=NEW_PIN, generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
        instruction_patches=[], replaced_services=[], full_goal_complete=False,
        boundary=__doc__), indent=2) + '\n')
    print('PASS original lens frame/view gates:', len(rows), 'cases')


if __name__ == '__main__':
    main()
