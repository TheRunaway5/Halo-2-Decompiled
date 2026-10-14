#!/usr/bin/env python3
"""Compare a captured live map callback with the unchanged TU1.5 executable.

RSQRTSS uses the runtime's explicit host estimate policy on both sides. This
checks the surrounding original body, not Xbox estimate bits or whole physics.
"""
import argparse
import ctypes
import hashlib
import json
import struct
import subprocess
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('capture', 'probe', 'estimate', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest() == NEW_PIN
    captured = json.loads(args.capture.read_text())
    case = captured['input']
    xbe = Xbe(NEW_DEFAULT)
    code = _Code(xbe, '.text')
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x10000, 0x600000)
    for section in xbe.sections:
        uc.mem_write(section.va, xbe.section_bytes(section))
    mapped = set(range(0x10000, 0x610000, 4096))
    known = set()
    for region in case['regions']:
        start = region['address']
        raw = bytes.fromhex(region['bytes'])
        for page in range(start & ~4095, (start + len(raw) + 4095) & ~4095, 4096):
            if page not in mapped:
                uc.mem_map(page, 4096)
                mapped.add(page)
        uc.mem_write(start, raw)
        known.update(range(start, start + len(raw)))
    gp = [UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
          UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI]
    xmm = [UC_X86_REG_XMM0 + n for n in range(8)]
    for reg, value in zip(gp, case['gp']):
        uc.reg_write(reg, value)
    for reg, value in zip(xmm, case['xmm']):
        uc.reg_write(reg, int.from_bytes(struct.pack('<4I', *value), 'little'))
    uc.reg_write(UC_X86_REG_EFLAGS, case['eflags'])
    uc.reg_write(UC_X86_REG_FPCW, case['cw'])
    uc.reg_write(UC_X86_REG_MXCSR, case['mxcsr'])
    library = ctypes.CDLL(str(args.estimate.resolve()))
    estimate = library.estimate
    estimate.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    estimate.restype = ctypes.c_uint32
    estimates, trace, writes, policy_sites = {}, [], [], []

    def require_known(address, size):
        assert all(p in known for p in range(address, address + size)), (
            'Original callback read uncaptured data', hex(address), size)

    def instruction(u, pc, size, data):
        trace.append(pc)
        i = code.at(pc)
        if i.mnemonic != 'rsqrtss':
            assert i.mnemonic != 'rsqrtps', 'No supplied vector estimate policy'
            return
        dst, src = i.operands
        assert dst.type == 1 and src.type == 1
        d = xmm[int(i.reg_name(dst.reg).removeprefix('xmm'))]
        s = xmm[int(i.reg_name(src.reg).removeprefix('xmm'))]
        bits = u.reg_read(s) & 0xffffffff
        value = estimate(bits, case['mxcsr'])
        assert str(bits) not in estimates or estimates[str(bits)] == value
        estimates[str(bits)] = value
        u.reg_write(d, (u.reg_read(d) & ~0xffffffff) | value)
        policy_sites.append(pc)
        u.reg_write(UC_X86_REG_EIP, pc + size)

    uc.hook_add(UC_HOOK_CODE, instruction)
    uc.hook_add(UC_HOOK_MEM_READ, lambda u, access, p, size, value, data: require_known(p, size))
    uc.hook_add(UC_HOOK_MEM_WRITE, lambda u, access, p, size, value, data:
                writes.append([p, (value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()]))
    stop = int.from_bytes(uc.mem_read(case['gp'][4], 4), 'little')
    uc.emu_start(case['entry'], stop, count=100000)
    assert uc.reg_read(UC_X86_REG_EIP) == stop
    original = dict(gp=[uc.reg_read(r) for r in gp],
                    xmm=[list(struct.unpack('<4I', uc.reg_read(r).to_bytes(16, 'little'))) for r in xmm],
                    eflags=uc.reg_read(UC_X86_REG_EFLAGS), instructions=trace, writes=writes,
                    banks=[bytes(uc.mem_read(b['address'], len(b['bytes']) // 2)).hex()
                           for b in case['regions']])
    case['rsqrt'] = estimates
    result = subprocess.run([str(args.probe.resolve())], input=json.dumps(case) + '\n',
                            text=True, capture_output=True, check=True)
    native = json.loads(result.stdout)
    for key, value in captured['actual_native'].items():
        assert native[key] == value, ('Captured invocation differs from replay', key)
    if native != original:
        args.output.with_suffix('.failure.json').write_text(json.dumps(
            dict(input=case, original=original, native=native), indent=2) + '\n')
        raise AssertionError('Original live callback mismatch')
    args.output.write_text(json.dumps(dict(input=case, original=original)) + '\n')
    report = dict(passed=True, cases=1, original_sha256=NEW_PIN,
                  entry=f"{case['entry']:08x}", original_instructions=len(trace),
                  captured_bytes=sum(len(b['bytes']) // 2 for b in case['regions']),
                  estimate_policy='Explicit existing host RSQRTSS estimate; Xbox bits unverified',
                  estimate_instruction_sites=policy_sites, replaced_functions=[],
                  full_goal_complete=False, boundary=__doc__,
                  fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest())
    args.output.with_suffix('.verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
