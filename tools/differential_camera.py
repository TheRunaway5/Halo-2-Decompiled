#!/usr/bin/env python3
"""Compare the menu observer with the original x86 observer and Euler helper.

Unicorn is a test oracle only; the native application never executes Xbox code.
This checks geometry transforms and the fixed-step camera channels, not images.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDI,
                              UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE = 0x4e9bd4


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT/'build/libhalo2_kernels.so')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/frontend/camera-differential.json')
    args = parser.parse_args()
    image = (ROOT/'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED:
        raise ValueError('unexpected retail XBE')
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, table = struct.unpack_from('<II', image, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', image, table-base+i*56)
        guest.mem_write(address, image[offset:offset+size])
    guest.mem_map(0x2000000, 0x10000)
    guest.mem_map(0x3000000, 0x1000)
    guest.reg_write(UC_X86_REG_FPCW, 0x027f)
    guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)

    def floats(address, values):
        guest.mem_write(address, struct.pack('<'+'f'*len(values), *values))

    def read(address, count):
        return list(struct.unpack('<'+'f'*count, guest.mem_read(address, count*4)))

    def call(address):
        guest.reg_write(UC_X86_REG_ESP, 0x200ff00)
        guest.mem_write(0x200ff00, struct.pack('<I', 0x3000000))
        guest.emu_start(address, 0x3000000, count=100000)
        assert guest.reg_read(UC_X86_REG_EIP) == 0x3000000, 'observer did not return'

    def basis(euler):
        guest.reg_write(UC_X86_REG_ESP, 0x200ff00)
        guest.mem_write(0x200ff00, struct.pack('<I3f', 0x3000000, *euler))
        guest.reg_write(UC_X86_REG_EAX, 0x2000100)
        guest.emu_start(0x141ce0, 0x3000000, count=10000)
        words = read(0x2000104, 9)
        return words[:3] + words[6:9]

    # Initialize the engine's runtime range globals. FOV has its own channel,
    # excluded from this comparison; position and rotation keep the full ABI.
    floats(0x54e854, [1.309, .01, 3.13, 1000.])
    guest.mem_write(STATE, bytes(0x358))
    guest.mem_write(STATE+4, struct.pack('<I', 0x2000200))
    guest.mem_write(0x2000200, struct.pack('<I', 9))
    library = ctypes.CDLL(str(args.library.resolve()))
    native = library.halo2_observer_step
    F32 = ctypes.c_float
    native.argtypes = [ctypes.POINTER(F32), ctypes.POINTER(F32), F32, F32,
                       ctypes.POINTER(F32)]
    native.restype = None
    rng = random.Random(0x16fe90)
    native_basis = library.halo2_authored_basis
    native_basis.argtypes = [ctypes.POINTER(F32), ctypes.POINTER(F32)]
    native_basis.restype = None
    basis_errors = []
    for i in range(1000):
        euler = [rng.uniform(-3.142, 3.142) for _ in range(3)]
        basis(euler)
        expected = read(0x2000104, 9)
        out = (F32*9)()
        native_basis((F32*3)(*euler), out)
        basis_errors.append(max(abs(a-b) for a, b in zip(expected, out)))
    assert max(basis_errors) < 1e-6, 'scenario rotation differs from 141CE0'
    failures = []
    errors = [0.] * 21
    cases = 0
    # A complete sequence carries both derivatives across overlapping targets.
    state = [0., 0., 0.] + basis([0., 0., 0.]) + [0.] * 12
    for transition in range(50):
        target = [rng.uniform(-40, 40) for _ in range(3)]
        target += basis([rng.uniform(-3, 3) for _ in range(3)])
        ticks = rng.choice([0, 1, 2, 3, 10, 30, 100, 300, 400, 500])
        for tick in range(max(1, ticks//2)):
            dt = F32(1/30).value
            seconds = F32(max(0, ticks-tick)*dt).value
            current = state[:3]+[0.]*6+[1.309]+state[3:9]
            destination = target[:3]+[0.]*6+[1.309]+target[3:9]
            floats(STATE+0xc, destination)
            floats(STATE+0x10c, current)
            floats(STATE+0x180, state[9:12]+[0.]*7+state[12:15])
            floats(STATE+0x1b4, state[15:18]+[0.]*7+state[18:21])
            floats(STATE+0x9c, [seconds, 0, 0, 0, 0, seconds])
            guest.mem_write(STATE+8, struct.pack('<I', 9))
            floats(0x4e9bd0, [dt])
            guest.reg_write(UC_X86_REG_EDI, 0)
            call(0x16fe90)
            expected = read(STATE+0x10c, 3)+read(STATE+0x134, 6)
            expected += read(STATE+0x180, 3)+read(STATE+0x1a8, 3)
            expected += read(STATE+0x1b4, 3)+read(STATE+0x1dc, 3)
            out = (F32*21)()
            native((F32*21)(*state), (F32*9)(*target), seconds, dt, out)
            actual = list(out)
            for i, (a, b) in enumerate(zip(expected, actual)):
                errors[i] = max(errors[i], abs(a-b))
            # Original sqrt/trig instructions and the host libm can differ by
            # a few ulps. Compare each step from identical input, avoiding a
            # tolerance that conceals accumulating path drift.
            if any(abs(a-b) > 2e-4 + abs(a)*2e-5 for a, b in zip(expected, actual)):
                if len(failures) < 12:
                    failures.append(dict(transition=transition, tick=tick,
                                         expected=expected, actual=actual))
            state = expected
            cases += 1
    result = dict(xbe_sha256=EXPECTED, retail_entry='0016fe90', cases=cases,
                  authored_basis=dict(retail_entry='00141ce0', cases=len(basis_errors),
                                      max_absolute_error=max(basis_errors)),
                  max_absolute_error=errors, failures=failures,
                  scope='fixed-step position/orientation observer and overlapping targets; no render, FOV or full director parity claimed')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result))
    return bool(failures)


if __name__ == '__main__':
    raise SystemExit(main())
