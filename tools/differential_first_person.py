#!/usr/bin/env python3
"""Compare first-person transform kernels with the original retail instructions.

The native runtime runs C++ only. Unicorn executes isolated math helpers here;
this gate does not establish animation blending or matched-frame parity.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
                              UC_X86_REG_EDX, UC_X86_REG_ESI,
                              UC_X86_REG_ESP, UC_X86_REG_EIP)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT/'build/libhalo2_kernels.so')
    parser.add_argument('--output', type=Path, default=ROOT/'analysis/first-person/transform-differential.json')
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
    library = ctypes.CDLL(str(args.library.resolve()))
    F32 = ctypes.c_float
    pointer = ctypes.POINTER(F32)
    kernels = [('inverse', 0x141590, library.halo2_transform_inverse, 2),
               ('compose', 0x142a60, library.halo2_transform_compose, 3),
               ('camera', 0x1420f0, library.halo2_camera_transform, 4)]
    rng = random.Random(0x165f16)
    reports = {}
    for name, address, native, argument_count in kernels:
        native.argtypes = [pointer] * argument_count
        native.restype = None
        failures = []
        cases = 0
        for case in range(10016):
            a = [rng.choice([0., -0., 1., -1., .5, 2., -2.])]
            a += [rng.uniform(-10, 10) for _ in range(12)]
            b = [rng.uniform(-4, 4)] + [rng.uniform(-10, 10) for _ in range(12)]
            if case < 16:
                a = [1., 1., 0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0.]
                a[0] = [0., -0., 1., -1.][case % 4]
                a[10:] = [0. if case & 4 else -0.] * 3
                b = [1., 1., 0., 0., 0., 1., 0., 0., 0., 1., 0., 0., 0.]
            guest.mem_write(0x2000100, struct.pack('<13f', *a))
            guest.mem_write(0x2000200, struct.pack('<13f', *b))
            guest.mem_write(0x2000300, b'\xcd' * 52)
            guest.mem_write(0x200fff0, struct.pack('<II', 0x3000000, 0x2000300))
            guest.reg_write(UC_X86_REG_ESP, 0x200fff0)
            out = (F32 * 13)()
            if name == 'inverse':
                guest.reg_write(UC_X86_REG_ECX, 0x2000100)
                guest.reg_write(UC_X86_REG_EAX, 0x2000300)
                native((F32 * 13)(*a), out)
            elif name == 'compose':
                guest.reg_write(UC_X86_REG_ECX, 0x2000100)
                guest.reg_write(UC_X86_REG_EDX, 0x2000200)
                native((F32 * 13)(*a), (F32 * 13)(*b), out)
            else:
                guest.reg_write(UC_X86_REG_ESI, 0x2000100)
                guest.reg_write(UC_X86_REG_ECX, 0x2000200)
                guest.reg_write(UC_X86_REG_EAX, 0x200020c)
                guest.reg_write(UC_X86_REG_EDX, 0x2000300)
                native((F32 * 3)(*a[:3]), (F32 * 3)(*b[:3]),
                       (F32 * 3)(*b[3:6]), out)
            guest.emu_start(address, 0x3000000, count=1000)
            if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
                raise RuntimeError(f'{name} kernel did not return')
            expected = bytes(guest.mem_read(0x2000300, 52))
            cases += 1
            if bytes(out) != expected:
                failures.append({'case': case, 'input_a': a, 'input_b': b,
                                 'different_words': [i for i in range(13)
                                                     if bytes(out)[i*4:i*4+4] != expected[i*4:i*4+4]],
                                 'expected': struct.unpack('<13I', expected),
                                 'actual': list(struct.unpack('<13I', bytes(out)))})
        reports[name] = {'retail_entry': f'{address:08x}', 'cases': cases,
                         'bit_exact_matches': cases-len(failures), 'failures': failures[:8]}
    report = {'xbe_sha256': EXPECTED,
              'native_library_sha256': hashlib.sha256(args.library.read_bytes()).hexdigest(),
              'kernels': reports,
              'scope': 'camera basis, uniform-scale inverse and composition; animation sampling and rendered frames excluded',
              'original_frame_parity_verified': False}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))
    return int(any(r['failures'] for r in reports.values()))


if __name__ == '__main__':
    raise SystemExit(main())
