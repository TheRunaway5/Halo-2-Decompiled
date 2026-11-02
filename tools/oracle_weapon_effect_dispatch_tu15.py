#!/usr/bin/env python3
"""Capture unchanged TU1.5 106F90/108A70 with declared callee-boundary inputs.

Run through tools/run_test.py. Original instructions select owners, branch,
construct call ABIs and return. Refresh/create/sound are explicit test services;
their actual allocation/audio behavior is outside this component reference.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_EBX, UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI,
    UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR)
from weapon_reference_versions import images

GP = (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EBX,
      UC_X86_REG_ESP, UC_X86_REG_EBP, UC_X86_REG_ESI, UC_X86_REG_EDI)
STACK, STOP = 0x2010000, 0x3000000
POOL, HEADERS, OBJECT, DEFINITION, TAGS = [0x1000000+n*0x2000 for n in range(5)]
NONE = 0xffffffff


def pack(words):
    return struct.pack('<'+'I'*len(words), *(v & NONE for v in words))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    _, image, pin = next(row for row in images() if row[0] == 'TU1.5')
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in image.sections:
        g.mem_write(section.va, image.section_bytes(section))
    g.mem_map(0x1000000, 0x100000)
    g.mem_map(0x2000000, 0x20000)
    g.mem_map(STOP, 4096)
    instructions, calls, rows = {}, [], []
    active = None

    def hook(g, address, size, _):
        if address in (0x17dff0, 0x179610, 0x18be50):
            sp = g.reg_read(UC_X86_REG_ESP)
            count = {0x17dff0: 1, 0x179610: 4, 0x18be50: 3}[address]
            calls.append(dict(target=address,
                gp=[g.reg_read(r) for r in GP],
                xmm0=g.reg_read(UC_X86_REG_XMM0).to_bytes(16, 'little').hex(),
                stack=bytes(g.mem_read(sp, (count+1)*4)).hex()))
            # Declared arbitrary callee outputs, shared with the native test.
            for r, v in zip((UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX), active['clobber']):
                g.reg_write(r, v)
            if address == 0x17dff0:
                g.reg_write(UC_X86_REG_EAX, active['refresh_result'])
            elif address == 0x179610:
                g.reg_write(UC_X86_REG_EAX, active['create_result'])
            ret = struct.unpack('<I', g.mem_read(sp, 4))[0]
            g.reg_write(UC_X86_REG_ESP, sp+4+count*4)
            g.reg_write(UC_X86_REG_EIP, ret)
            return
        raw = bytes(g.mem_read(address, size))
        if raw != image.read(address, size):
            raise ValueError('Original instruction bytes changed')
        instructions[f'{address:08x}'] = raw.hex()

    g.hook_add(UC_HOOK_CODE, hook)
    rng = random.Random(0x108a70)
    for entry in (0x106f90, 0x108a70):
        for index in range(256):
            gp = [rng.getrandbits(32) for _ in GP]
            xmm = [rng.getrandbits(128).to_bytes(16, 'little').hex() for _ in range(8)]
            effect = (0x9234000b if index % 16 else NONE)
            # Indices remain positive in ordinary tags; the native suite also
            # checks signed-index faults independently.
            weapon = 0xabcd0003
            gp[4] = STACK
            gp[0], gp[1] = (weapon, gp[1]) if entry == 0x106f90 else (effect, weapon)
            parent = NONE if index & 8 else 0x43210007
            group = (0x65666665, 0x736e6421, 0x6a707421, 0)[index % 4]
            memory = [[0x4ecbd0, POOL, 4], [POOL+0x44, HEADERS, 4],
                [HEADERS+3*12+2, 0x10 if index & 1 else 0, 1],
                [HEADERS+3*12+8, OBJECT, 4], [OBJECT, 7, 4],
                [OBJECT+4, index >> 1 & 1, 1], [OBJECT+0x14, parent, 4],
                [OBJECT+0x12c, index >> 2 & 1, 1], [OBJECT+0x154, 0x43210009, 4],
                [0x4f0414, TAGS, 4], [TAGS+7*16+8, DEFINITION, 4],
                [DEFINITION+0x38, NONE if index & 16 else 0x87650004, 4],
                [TAGS+11*16, group, 4], [0x474d08, 0x13572468, 4],
                [0x474ce8, 0x24681357, 4]]
            arguments = [NONE if index & 32 else 0x34560002,
                         rng.getrandbits(32), rng.getrandbits(32)]
            active = dict(entry=entry, gp=gp, xmm=xmm, memory=memory, arguments=arguments,
                refresh_result=(0xabcdef01 if index & 64 else 0xabcdef00),
                create_result=(NONE if index & 128 else 0x45670003),
                clobber=[rng.getrandbits(32) for _ in range(3)])
            for at, value, width in memory:
                g.mem_write(at, value.to_bytes(width, 'little'))
            g.mem_write(STACK-4096, bytes(8192))
            g.mem_write(STACK, pack([STOP, *arguments]))
            for r, value in zip(GP, gp):
                g.reg_write(r, value)
            for n, raw in enumerate(xmm):
                g.reg_write(UC_X86_REG_XMM0+n, int.from_bytes(bytes.fromhex(raw), 'little'))
            g.reg_write(UC_X86_REG_FPCW, 0x027f)
            g.reg_write(UC_X86_REG_FPSW, 0)
            g.reg_write(UC_X86_REG_FPTAG, 0xffff)
            g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            calls.clear()
            g.emu_start(entry, STOP, count=10000)
            if g.reg_read(UC_X86_REG_EIP) != STOP:
                raise ValueError('Original owner did not complete')
            active['expected_gp'] = [g.reg_read(r) for r in GP]
            active['expected_xmm'] = [g.reg_read(UC_X86_REG_XMM0+n).to_bytes(16, 'little').hex() for n in range(8)]
            active['calls'] = list(calls)
            rows.append(active)
    fixture = dict(schema='H2_TU15_NATIVE_WEAPON_EFFECT_DISPATCH_REFERENCE_V1',
        original_sha256=pin, scope=__doc__, rows=rows)
    data = (json.dumps(fixture, separators=(',', ':'))+'\n').encode()
    if len(data)>1024*1024:
        raise ValueError('Reference exceeds bounded fixture size')
    args.fixture.write_bytes(data)
    report = dict(passed=True, original_sha256=pin, cases=len(rows),
        fixture_sha256=hashlib.sha256(data).hexdigest(),fixture_bytes=len(data),
        instructions=instructions, instruction_patches=[],
        declared_service_boundaries=['17DFF0 refresh', '179610 creation', '18BE50 sound'],
        gameplay_or_output_parity=False, retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key:report[key] for key in ('passed','cases','fixture_bytes','fixture_sha256')}))


if __name__ == '__main__':
    main()
