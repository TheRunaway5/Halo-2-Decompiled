#!/usr/bin/env python3
"""Compare native carrier animation writes with retail 17550/176A0/17960.

The original executable runs only in this differential test, never in the
native game. This checks authored constants, not final Xbox framebuffer parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ESI,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_XMM0)
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
STATE, STACK, STOP = 0x2100000, 0x21FFF00, 0x3000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build-frontend/shader_animation_tests")
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/frontend/shader-animation-differential.json")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    vectors = args.output.with_suffix(".bin")
    subprocess.run([str(args.binary.resolve()), str(ROOT / "assets/disc/maps"), str(vectors)], check=True)
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
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    mapped_base = cache.magic & ~4095
    mapped_size = (cache.magic + len(cache.data) - mapped_base + 4095) & ~4095
    guest.mem_map(mapped_base, mapped_size)
    guest.mem_write(cache.magic, bytes(cache.data))
    guest.mem_map(STATE, 0x100000)
    guest.mem_map(STOP, 0x1000)
    # Original stdcall callback returns floats on the x87 stack. Supply the
    # authored carrier control and the separately evaluated retail color.
    callback = bytes.fromhex("8b4424083de208001174103de10800077412")
    callback += b"\xd9\x05" + struct.pack("<I", STOP + 0x204) + b"\xc2\x08\x00"
    callback += b"\xd9\x05" + struct.pack("<I", STOP + 0x204) + b"\xc2\x08\x00"
    callback += b"\xd9\x05" + struct.pack("<I", STOP + 0x200) + b"\xc2\x08\x00"
    guest.mem_write(STOP + 0x100, callback)
    guest.mem_write(STOP + 0x204, struct.pack("<f", 1))
    guest.mem_write(STOP + 0x300, struct.pack("<II", 0, STOP + 0x100))
    guest.mem_write(0x485A58, struct.pack("<I", STOP + 0x300))
    _, carrier = cache.tag(0xE1D50060, "scen")
    function = cache.block(carrier + 0x64, 32)[0]

    def run(entry, words, registers=()):
        guest.mem_write(STACK, struct.pack("<" + "I" * (len(words) + 1), STOP, *words))
        guest.reg_write(UC_X86_REG_ESP, STACK)
        guest.reg_write(UC_X86_REG_FPCW, 0x027F)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        for reg, value in registers:
            guest.reg_write(reg, value)
        guest.emu_start(entry, STOP, count=1000000)
        if guest.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f"retail {entry:08x} failed to return")

    data = vectors.read_bytes()
    record_size = 28 + 752 * 2
    if len(data) % record_size:
        raise ValueError("malformed native animation vectors")
    failures = []
    cases = len(data) // record_size
    for index in range(cases):
        record = data[index * record_size:(index + 1) * record_size]
        datum, template_pass, implementation, seconds, object_seconds, native_color = struct.unpack_from("<IIIdff", record)
        # BA8C0's periodic curve input is tick * rate; its output bounds and
        # scale=grav_lift_control are evaluated without a native approximation.
        input_bits = struct.unpack("<I", struct.pack("<f", object_seconds))[0]
        run(0x13BB90, [input_bits, 0x3F800000], [(UC_X86_REG_EAX, cache.magic + function + 20)])
        color_bytes = int(guest.reg_read(UC_X86_REG_XMM0)).to_bytes(16, "little")[:4]
        guest.mem_write(STOP + 0x200, color_bytes)
        if color_bytes != struct.pack("<f", native_color):
            failures.append({"case": index, "surface": "object color function",
                "retail": struct.unpack("<f", color_bytes)[0], "native": native_color})
        _, shader = cache.tag(datum, "shad")
        definition = cache.block(shader + 32, 124)[0]
        animation_pass = cache.block(definition + 44, 2)[template_pass]
        packed = cache.u16(animation_pass)
        animated_impl = cache.block(definition + 52, 10)[(packed & 511) + implementation]
        _, stem = cache.tag(cache.u32(definition), "stem")
        template = cache.block(stem + 88, 40)[0]
        template_entry = cache.block(template + 16, 10)[template_pass]
        _, spas = cache.tag(cache.u32(template_entry + 4), "spas")
        pass_definition = cache.block(spas + 28, 88)[0]
        pass_impl = cache.block(pass_definition, 306)[implementation]
        guest.mem_write(STATE, bytes(0x1800))
        guest.mem_write(STATE + 12, struct.pack("<I", cache.magic + definition))
        guest.mem_write(STATE + 32, struct.pack("<II", cache.magic + pass_definition, cache.magic + pass_impl))
        switches = [0] * 8
        switches[4:6] = [cache.u16(shader + 0x3E), cache.u16(shader + 0x40)]
        guest.mem_write(STATE + 0x300, struct.pack("<8I", *switches))
        guest.mem_write(STATE + 0x1424, record[28:268])
        guest.mem_write(STATE + 0x1530, record[268:780])
        # Assemble's fragment offsets are zero for both carrier plasma passes.
        guest.mem_write(STATE + 0x1520, bytes(8))
        guest.mem_write(0x4858A0, struct.pack("<d", seconds))
        run(0x17550, [cache.magic + animated_impl], [(UC_X86_REG_EBX, STATE)])
        run(0x176A0, [cache.magic + animated_impl + 6], [(UC_X86_REG_ESI, STATE)])
        run(0x17960, [STATE, cache.magic + animated_impl + 8])
        actual = bytes(guest.mem_read(STATE + 0x1424, 240)) + bytes(guest.mem_read(STATE + 0x1530, 512))
        expected = record[780:]
        if actual != expected:
            differing = [i for i in range(188) if actual[i*4:i*4+4] != expected[i*4:i*4+4]]
            failures.append({"case": index, "datum": f"{datum:08x}", "seconds": seconds,
                "different_words": differing, "retail": [actual[i*4:i*4+4].hex() for i in differing],
                "native": [expected[i*4:i*4+4].hex() for i in differing]})
    report = {"xbe_sha256": EXPECTED, "binary_sha256": hashlib.sha256(args.binary.read_bytes()).hexdigest(),
        "retail_entries": ["0013bb90", "00017550", "000176a0", "00017960"],
        "cases": cases, "failures": len(failures), "examples": failures[:5],
        "scope": "carrier object function, plasma and emissive pixel constants and vertex transforms",
        "retail_frame_parity_verified": False}
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
