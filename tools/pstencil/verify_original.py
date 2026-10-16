#!/usr/bin/env python3
"""Compare the bounded pStencil CPU port with unchanged local 2003 i386 code.

Only the explicit graphics/service exits below are observed/stubbed. This does
not execute an Xbox GPU, infer caster topology, or certify a complete renderer.
"""
from __future__ import annotations

import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import random
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from disc import inspect_xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW

SOURCE_SHA = "5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18"
STOP, SCRATCH, COUNT, STACK = 0x2000100, 0x2001000, 0x2002000, 0x200f000


class Original:
    def __init__(self, path):
        self.metadata = inspect_xbe(path)
        if self.metadata["sha256"] != SOURCE_SHA:
            raise ValueError("This oracle requires the inspected May 3, 2003 debug XBE")
        self.data = path.read_bytes()
        self.guest = Uc(UC_ARCH_X86, UC_MODE_32)
        self.guest.reg_write(UC_X86_REG_FPCW,0x23f)  # 2003 real_math_reset_precision PC53
        base = self.metadata["base_address"]
        self.guest.mem_map(base, (self.metadata["image_size"] + 4095) & ~4095)
        for section in self.metadata["sections"]:
            self.guest.mem_write(section["address"], self.data[
                section["file_offset"]:section["file_offset"] + section["file_size"]])
        self.guest.mem_map(0x2000000, 0x100000)
        self.calls = []
        self.guest.hook_add(UC_HOOK_CODE, self.hook)

    def hook(self, guest, address, size, data):
        services = {0x956e0: ("state", 2), 0x33a20: ("clear", 4),
                    0x99660: ("pixel_program", 1), 0x1e160: ("memset", 3),
                    0xa4c50: ("light_begin", 5), 0xa3ca0: ("light_end", 0),
                    0xf25e0: ("lit_geometry", 0), 0xf1240: ("submit_scene", 1),
                    0x50870: ("geometry", 2), 0x33a90: ("target", 3),
                    0x305d0: ("vertex_program", 1)}
        if address not in services:
            return
        kind, count = services[address]
        esp = guest.reg_read(UC_X86_REG_ESP)
        words = struct.unpack("<" + "I" * (count + 1), guest.mem_read(esp, 4 * (count + 1)))
        self.calls.append((kind, list(words[1:])))
        if kind == "memset":
            destination, value, length = words[1:]
            guest.mem_write(destination, bytes([value & 255]) * length)
        guest.reg_write(UC_X86_REG_ESP, esp + 4)
        guest.reg_write(UC_X86_REG_EAX, 1)
        guest.reg_write(UC_X86_REG_EIP, words[0])

    def run(self, entry, arguments=(), eax=0, edx=0, budget=100000):
        self.calls.clear()
        self.guest.mem_write(STACK, struct.pack("<" + "I" * (len(arguments) + 1), STOP, *arguments))
        self.guest.reg_write(UC_X86_REG_ESP, STACK)
        self.guest.reg_write(UC_X86_REG_EAX, eax)
        self.guest.reg_write(UC_X86_REG_EDX, edx)
        self.guest.emu_start(entry, STOP, count=budget)
        if self.guest.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError(f"Original {entry:x} did not return within the instruction budget")
        return self.guest.reg_read(UC_X86_REG_EAX)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    original = Original(args.build / "game/halo2debug.xbe")
    native = ctypes.CDLL(str(args.native.resolve()))
    native.pstencil_stitch_quad.argtypes = [ctypes.c_uint16, ctypes.c_uint16,
        ctypes.c_uint32, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint16)]
    native.pstencil_stitch_quad.restype = ctypes.c_uint32
    native.pstencil_shadow_states.argtypes = [ctypes.POINTER(ctypes.c_uint32)]
    native.pstencil_shadow_states.restype = ctypes.c_uint32
    native.pstencil_light_passes.argtypes = [ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
    native.pstencil_light_passes.restype = ctypes.c_uint32
    native.pstencil_extrusion_distance.restype = ctypes.c_float
    native.pstencil_shadow_clear.restype = ctypes.c_uint32
    native.pstencil_shadow_casting_part.argtypes = [ctypes.c_uint16]
    native.pstencil_shadow_casting_part.restype = ctypes.c_uint32
    caster_types = []
    native.pstencil_shadow_stipple_level.argtypes = [ctypes.c_uint8]
    native.pstencil_shadow_stipple_level.restype = ctypes.c_uint32
    original.run(0xa7bf0,budget=500000)
    reciprocal = struct.unpack('<f',original.guest.mem_read(0x3e22cc,4))[0]
    for opacity in range(256):
        opacity_word = struct.unpack('<I',struct.pack('<f',opacity*reciprocal))[0]
        pointer = original.run(0xa7e00,(opacity_word,))
        expected = (pointer-0x92d940)//0x80
        if not 0<=expected<=16 or pointer != 0x92d940+expected*0x80 or native.pstencil_shadow_stipple_level(opacity)!=expected:
            raise AssertionError((opacity,hex(pointer),expected,native.pstencil_shadow_stipple_level(opacity)))
    for part_type in range(1,5):
        expected = original.run(0x1cb000,(part_type,)) & 255
        actual = native.pstencil_shadow_casting_part(part_type)
        if actual != expected:
            raise AssertionError((part_type,actual,expected))
        caster_types.append(dict(type=part_type,casts=bool(expected)))
    endpoints = [0, 1, 2, 0x3fff, 0x4000, 0x7fff, 0x8000, 0xffff]
    cases = [(a, b, fa, fb) for a in endpoints for b in endpoints for fa in (0, 1) for fb in (0, 1)]
    randomizer = random.Random(20030503)
    cases += [(randomizer.randrange(65536), randomizer.randrange(65536),
               randomizer.randrange(2), randomizer.randrange(2)) for _ in range(1024)]
    for a, b, fa, fb in cases:
        original.guest.mem_write(COUNT, struct.pack("<I", 0))
        original.guest.mem_write(SCRATCH, b"\xa5" * 8)
        result = original.run(0x1773d0, (a, b, fa, SCRATCH), eax=COUNT, edx=fb)
        expected = struct.unpack("<4H", original.guest.mem_read(SCRATCH, 8))
        expected_count = struct.unpack("<I", original.guest.mem_read(COUNT, 4))[0]
        actual = (ctypes.c_uint16 * 4)()
        actual_count = native.pstencil_stitch_quad(a, b, fa, fb, actual)
        if tuple(actual) != expected or actual_count != expected_count or result * 4 != expected_count:
            raise AssertionError((a, b, fa, fb, tuple(actual), expected, actual_count, expected_count, result))
    # The ordinary, enabled-light, non-debug corridor. Explicitly set each
    # controlling byte; no default-file initialization is taken on trust.
    for address, value in ((0x74389c, 1), (0x48f449, 1), (0x48f455, 0),
                           (0x48f44a, 0), (0x48f448, 0), (0x51e2a1, 0)):
        original.guest.mem_write(address, bytes([value]))
    original.run(0x1ce3a0)
    expected_states = [words for kind, words in original.calls if kind == "state"]
    expected_clears = [words for kind, words in original.calls if kind == "clear"]
    states = (ctypes.c_uint32 * 128)()
    state_count = native.pstencil_shadow_states(states)
    actual_states = [list(states[i * 2:i * 2 + 2]) for i in range(state_count)]
    if actual_states != expected_states:
        raise AssertionError((actual_states, expected_states))
    if expected_clears != [[4, 0, 0x3f800000, native.pstencil_shadow_clear()]]:
        raise AssertionError(expected_clears)
    # E81A0 and its actual __ftol2 execute with the source PC53 precision.
    # Independently compute the expected byte from the three backed globals;
    # the conversion is not stubbed. Observe graphics service exits only.
    values = [struct.unpack("<f", original.guest.mem_read(a,4))[0]
              for a in (0x3dc844,0x48f444,0x3e23c4)]
    expected_byte = int((values[0]-values[1])*values[2])
    if expected_byte != 152:
        raise AssertionError(values)
    native.pstencil_lightmap_shadow_rgb.restype = ctypes.c_uint32
    native.pstencil_lightmap_shadow_states.argtypes = [ctypes.POINTER(ctypes.c_uint32)]
    native.pstencil_lightmap_shadow_states.restype = ctypes.c_uint32
    original.run(0xe81a0)
    expected_lightmap_states = [words for kind,words in original.calls if kind == "state"]
    count = native.pstencil_lightmap_shadow_states(states)
    actual_lightmap_states = [list(states[i*2:i*2+2]) for i in range(count)]
    program_constant = struct.unpack("<I", original.guest.mem_read(0x51f824,4))[0]
    if actual_lightmap_states != expected_lightmap_states:
        raise AssertionError((actual_lightmap_states,expected_lightmap_states))
    if native.pstencil_lightmap_shadow_rgb() != expected_byte or program_constant != 0xff989898:
        raise AssertionError((expected_byte,program_constant))
    lightmap_shadow = dict(default_opacity=values[1], multiply_rgb=expected_byte,
        program_constant=program_constant, state_writes=expected_lightmap_states,
        scope="default conversion calculated from original globals; unchanged state and pixel program construction")
    schedules = []
    for shadows in (0, 1):
        original.run(0xf7f20, (0xffffffff, SCRATCH, SCRATCH + 256, SCRATCH + 512, shadows))
        expected = [words for kind, words in original.calls if kind == "geometry"]
        begin = [words for kind, words in original.calls if kind == "light_begin"]
        if len(begin) != 1:
            raise AssertionError(begin)
        extrusion = struct.unpack("<f", struct.pack("<I", begin[0][4]))[0]
        if native.pstencil_extrusion_distance() != extrusion:
            raise AssertionError(extrusion)
        count = native.pstencil_light_passes(shadows, states)
        actual = [list(states[i * 2:i * 2 + 2]) for i in range(count)]
        if actual != expected:
            raise AssertionError((shadows, actual, expected))
        if not shadows and ("state", [0x90, 0]) not in original.calls:
            raise AssertionError("Original shadowless light did not disable stencil")
        schedules.append(dict(shadows=bool(shadows), geometry_calls=expected, extrusion=extrusion))
    report = dict(schema="halo2-pstencil-original-shadow-v1", source_sha256=SOURCE_SHA,
                  native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),
                  stitch_cases=len(cases), state_writes=actual_states, clear=expected_clears,
                  light_schedules=schedules, lightmap_shadow=lightmap_shadow, caster_types=caster_types,
                  opacity_stipple_cases=256, passed=True,
                  scope="unchanged i386 CPU comparisons with observed graphics-service exits; no GPU or live-mode proof")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: {len(cases)} original stitch cases, {state_count} state writes, both light schedules, lightmap-shadow RGB152 and state sequence")


if __name__ == "__main__":
    main()
