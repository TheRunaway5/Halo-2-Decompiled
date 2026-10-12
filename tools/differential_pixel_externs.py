#!/usr/bin/env python3
"""Compare native material extern writes with retail Xbox 00019CA0.

This executes the original binder only in a test harness. It compares program
constants, including skipped permutations and overlapping extern writes; it
does not certify shader arithmetic, pass scheduling or framebuffer parity.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", type=Path, default=ROOT / "build-appearance/libhalo2_kernels.so")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/appearance/pixel-externs.json")
    args = parser.parse_args()
    image = (ROOT / "assets/disc/default.xbe").read_bytes()
    if hashlib.sha256(image).hexdigest() != EXPECTED:
        raise ValueError("unexpected retail executable hash")
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    base = struct.unpack_from("<I", image, 0x104)[0]
    count, table = struct.unpack_from("<II", image, 0x11C)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from("<5I", image, table - base + i * 56)
        guest.mem_write(address, image[offset:offset + size])
    guest.mem_map(0x2100000, 0x100000)
    guest.mem_map(0x3000000, 0x1000)
    U32, I32, U8, F32 = ctypes.c_uint32, ctypes.c_int32, ctypes.c_uint8, ctypes.c_float
    native = ctypes.CDLL(str(args.library.resolve())).halo2_remap_pixel_externs
    native.argtypes = [ctypes.POINTER(U32), ctypes.POINTER(F32), ctypes.POINTER(F32),
                       ctypes.POINTER(U32), U32, ctypes.POINTER(U32), U32,
                       ctypes.POINTER(I32), ctypes.POINTER(U8), ctypes.POINTER(U32)]
    native.restype = ctypes.c_int
    rng = random.Random(0x19CA0)
    scenarios = [(fragmented, component, mode, bank, slot, signed)
                 for fragmented in (False, True) for component in range(8)
                 for mode in range(8) for bank in range(2)
                 for slot in (0, 8) for signed in (False, True)]
    scenarios += [None] * 1000
    failures = []
    for case, scenario in enumerate(scenarios):
        initial = [rng.getrandbits(32) for _ in range(60)]
        scalars = [F32(rng.uniform(-2, 2)).value for _ in range(43)]
        colors = [[F32(rng.uniform(-2, 2)).value for _ in range(4)] for _ in range(42)]
        # Include exact quantization boundaries and the authored Cairo values.
        scalars[:8] = [0, 1, .5, 1.5, -1, .25, 128 / 255, 1 / 255]
        colors[3] = [117 / 255, 133 / 255, 80 / 255, 0]
        fragmented = scenario[0] if scenario else rng.choice((False, True))
        switches = [rng.randrange(8) for _ in range(8)]
        fragments = [(8 << 25) | 0] if fragmented else []
        offsets = [rng.randrange(8) for _ in range(8)]
        if scenario:
            offsets[0] = 0
        mappings = []
        for _ in range(1 if scenario else rng.randrange(1, 65)):
            if scenario:
                _, component, mode, bank, slot, signed = scenario
            else:
                component, mode, bank = rng.randrange(8), rng.randrange(8), rng.randrange(2)
                slot = rng.choice(list(range(8 - offsets[0])) + [8]) if fragmented else rng.randrange(9)
                signed = rng.choice((False, True))
            source = rng.randrange(42 if component == 4 else 43)
            permutation = switches[0] if scenario else rng.randrange(8)
            destination = permutation << 3 | slot << 6 | bank << 10 | int(signed) << 11
            mappings.append(destination | (component | mode << 3) << 16 | source << 24)
        guest.mem_write(0x2100000, bytes(0x1800))
        guest.mem_write(0x2100020, struct.pack("<II", 0x2120000, 0x2130000))
        guest.mem_write(0x2120024, struct.pack("<I", 0x2140000))
        guest.mem_write(0x212003C, struct.pack("<I", 0x2150000))
        guest.mem_write(0x21300F6, struct.pack("<H", len(fragments) << 9))
        guest.mem_write(0x2140000, struct.pack("<" + "I" * len(fragments), *fragments))
        guest.mem_write(0x2150000, struct.pack("<" + "I" * len(mappings), *mappings))
        guest.mem_write(0x2160000, struct.pack("<H", len(mappings) << 9))
        guest.mem_write(0x210005C, struct.pack("<43f", *scalars))
        guest.mem_write(0x2100108, struct.pack("<126f", *(v for color in colors for v in color[:3])))
        guest.mem_write(0x2100300, struct.pack("<8i", *switches))
        guest.mem_write(0x2101424, struct.pack("<60I", *initial))
        guest.mem_write(0x2101520, bytes(offsets))
        guest.mem_write(0x21FFF00, struct.pack("<II", 0x3000000, 0x2160000))
        guest.reg_write(UC_X86_REG_ESI, 0x2100000)
        guest.reg_write(UC_X86_REG_ESP, 0x21FFF00)
        guest.emu_start(0x19CA0, 0x3000000, count=100000)
        if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
            raise RuntimeError("retail pixel extern binder did not return")
        expected = bytes(guest.mem_read(0x2101424, 240))
        out = (U32 * 60)()
        scalar_input = scalars + [0] * (256 - len(scalars))
        color_input = [v for color in colors for v in color] + [0] * ((256 - len(colors)) * 4)
        ok = native((U32 * 60)(*initial), (F32 * 256)(*scalar_input),
                    (F32 * 1024)(*color_input), (U32 * len(mappings))(*mappings), len(mappings),
                    (U32 * len(fragments))(*fragments), len(fragments),
                    (I32 * 8)(*switches), (U8 * 8)(*offsets), out)
        if not ok or bytes(out) != expected:
            failures.append({"case": case, "native_ok": ok, "mappings": mappings,
                "different_words": [i for i in range(60) if bytes(out)[i*4:i*4+4] != expected[i*4:i*4+4]]})
    report = {"xbe_sha256": EXPECTED, "library_sha256": hashlib.sha256(args.library.read_bytes()).hexdigest(),
              "retail_entry": "00019ca0", "cases": len(scenarios),
              "bit_exact_matches": len(scenarios) - len(failures), "failures": failures[:10],
              "scope": "extern constant packing, operations and permutation filtering only",
              "retail_frame_parity_verified": False}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
