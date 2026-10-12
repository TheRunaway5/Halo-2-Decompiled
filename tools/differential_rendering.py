#!/usr/bin/env python3
"""Compare native texture mip selection with original Xbox retail instructions.

Requires Unicorn in the existing .tools/python test environment. The runtime
does not use Unicorn or execute Xbox code. This is a texture-kernel check,
not a shader/lighting or frame-parity claim.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, default=ROOT / "build/libhalo2_kernels.so")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/rendering-audit/mip-differential.json")
    args = parser.parse_args()
    image = (ROOT / "assets/disc/default.xbe").read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED:
        raise ValueError("unexpected retail executable hash")
    base = struct.unpack_from("<I", image, 0x104)[0]
    count, table = struct.unpack_from("<II", image, 0x11C)
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for i in range(count):
        _, address, virtual, offset, size = struct.unpack_from("<5I", image, table - base + i * 56)
        guest.mem_write(address, image[offset:offset + size])
    guest.mem_map(0x2000000, 0x10000)
    guest.mem_map(0x3000000, 0x1000)
    native = ctypes.CDLL(str(args.library.resolve())).halo2_bitmap_mip_count
    native.argtypes = [ctypes.c_uint] * 6
    native.restype = ctypes.c_uint
    rng = random.Random(0x359E0)
    cases = [(1 << w, 1 << h, 1, fmt, linear, maximum)
             for w in range(13) for h in range(13) for fmt in (1, 11, 14, 15, 16)
             for linear in (0, 1) for maximum in (0, 1, 3, 7, 9, 12)]
    cases += [(rng.randint(1, 4096), rng.randint(1, 4096), rng.randint(1, 512),
               rng.randrange(19), rng.randrange(2), rng.randrange(13)) for _ in range(10000)]
    cases += [(1 << rng.randrange(13), 1 << rng.randrange(13), 1 << rng.randrange(10),
               rng.randrange(19), rng.randrange(2), rng.randrange(13)) for _ in range(10000)]
    failures = []
    for width, height, depth, fmt, linear, maximum in cases:
        stack = 0x200FF00
        guest.mem_write(stack, struct.pack("<5I", 0x3000000, height, fmt, linear, maximum))
        guest.reg_write(UC_X86_REG_ESP, stack)
        guest.reg_write(UC_X86_REG_ECX, width)
        guest.reg_write(UC_X86_REG_EAX, depth)
        guest.emu_start(0x359E0, 0x3000000, count=1000)
        if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
            raise RuntimeError("retail mip-count kernel did not return")
        expected = guest.reg_read(UC_X86_REG_EAX) & 0xFFFF
        actual = native(width, height, depth, fmt, linear, maximum)
        if expected != actual:
            failures.append({"input": [width, height, depth, fmt, linear, maximum],
                             "expected": expected, "actual": actual})
    report = {"xbe_sha256": EXPECTED, "native_library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
              "retail_entry": "000359e0", "cases": len(cases), "bit_exact_matches": len(cases) - len(failures),
              "failures": failures[:20], "scope": "positive texture dimensions; mip-count kernel only",
              "retail_shader_lighting_parity_verified": False}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
