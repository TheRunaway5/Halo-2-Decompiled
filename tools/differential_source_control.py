#!/usr/bin/env python3
"""Compare native shared control/group changes with unchanged retail11BA50.

Complete1071E0 runs. Its object iterator is an explicitly empty caller service;
107980 captures the authored effect request without running the effect engine.
Player target/action admission, linked-object notifications, device motion and
networking are outside this finite comparison.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
import subprocess
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_EDI,
                              UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
from audit_rendering import Cache, file_sha
from audit_map_world_features import DISC, PACK

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe

PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
BANK, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000
HEADER, POOL, OBJECT, TAGS, DEFINITION, GROUP_HEADER, GROUP_POOL = [BANK + n * 0x1000 for n in range(7)]
HANDLE, GROUP = 0x10001, 0x10003


def fw(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--write-fixture", type=Path)
    p.add_argument("--write-assets", type=Path)
    args = p.parse_args()
    image = Xbe(ROOT / "assets/disc/default.xbe")
    if hashlib.sha256(image.data).hexdigest() != PIN:
        raise ValueError("unsupported original executable")
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    guest.mem_map(BANK, 0x100000)
    guest.mem_map(STOP, 0x1000)

    def put(at, value):
        guest.mem_write(at, struct.pack("<I", value & 0xffffffff))

    def word(at):
        return struct.unpack("<I", guest.mem_read(at, 4))[0]

    put(0x4e0300, HEADER)
    put(HEADER + 0x44, POOL)
    put(POOL + 20, OBJECT)
    put(OBJECT, 0x10002)
    put(0x4e3b44, TAGS)
    put(TAGS + 2 * 16 + 8, DEFINITION)
    put(0x4e0328, GROUP_HEADER)
    put(GROUP_HEADER + 0x44, GROUP_POOL)
    observed = {}
    visited = set()

    def ret(pop=0):
        sp = guest.reg_read(UC_X86_REG_ESP)
        guest.reg_write(UC_X86_REG_EIP, word(sp))
        guest.reg_write(UC_X86_REG_ESP, sp + 4 + pop)

    def hook(uc, pc, size, user):
        if pc == 0xbaeb0:
            # Empty native object enumeration; no linked-object wake/dirty
            # side effects are supplied or attributed to this comparison.
            uc.reg_write(UC_X86_REG_EAX, 0)
            ret(4)
            return
        if pc == 0x107980:
            if uc.reg_read(UC_X86_REG_EDX) != HANDLE:
                raise RuntimeError("source control effect owner changed")
            observed["effect"] = uc.reg_read(UC_X86_REG_EAX)
            ret()
            return
        if not (0x11ba50 <= pc < 0x11bb30 or 0x1071e0 <= pc < 0x10736d):
            raise RuntimeError(f"unexpected original instruction {pc:08x}")
        if bytes(uc.mem_read(pc, size)) != image.read(pc, size):
            raise RuntimeError("original control instruction changed")
        visited.add(pc)
        if pc == 0x1071e0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            if word(sp + 4) != GROUP:
                raise RuntimeError("source selected another position group")
            observed.update(attempted=1, requested=word(sp + 8))
        if pc == 0x11bae7:
            observed["changed"] = uc.reg_read(UC_X86_REG_EAX) & 255

    guest.hook_add(UC_HOOK_CODE, hook)

    def original(row):
        kind, call, current, flags, present, on, off, deny = row
        guest.mem_write(DEFINITION + 0x11c, struct.pack("<HHI", kind, 0, call))
        for at, value in ((0x12c, on), (0x134, off), (0x13c, deny)):
            put(DEFINITION + at, value)
        put(OBJECT + 0x13c, GROUP if present else 0xffffffff)
        guest.mem_write(GROUP_POOL + 36, struct.pack("<HHII", 1, flags, current, 0x55555555))
        observed.clear()
        observed.update(attempted=0, changed=0, requested=0, effect=0xffffffff)
        guest.reg_write(UC_X86_REG_EDI, HANDLE)
        guest.reg_write(UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(UC_X86_REG_ESP, STACK)
        put(STACK, STOP)
        guest.emu_start(0x11ba50, STOP, count=10000)
        if guest.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError("original control did not return")
        salt, final_flags, value, untouched = struct.unpack("<HHII", guest.mem_read(GROUP_POOL + 36, 12))
        if salt != 1 or untouched != 0x55555555:
            raise RuntimeError("original control altered unrelated group state")
        return [observed[n] for n in ("attempted", "changed", "requested", "effect")] + [value, final_flags]

    rows = []
    for kind in range(4):
        for current in [-.5, -0., 0., .25, struct.unpack("<f", struct.pack("<I", 0x3effffff))[0],
                        .5, struct.unpack("<f", struct.pack("<I", 0x3f000001))[0], .75, 1., 1.5]:
            for flags in (0, 1, 2, 3, 4, 0xff00, 0xffff):
                for call in (-2., -0., 0., .5, 1., 2.):
                    rows.append([kind, fw(call), fw(current), flags, 1, 0x10010, 0x10011, 0x10012])
        rows.append([kind, fw(.5), fw(.25), 1, 0, 0x10010, 0x10011, 0x10012])
    rng = random.Random(0x11ba50)
    for i in range(1024):
        rows.append([i % 4, fw(rng.uniform(-2, 2)), fw(rng.random()), rng.getrandbits(16),
                     1, rng.getrandbits(32), rng.getrandbits(32), rng.getrandbits(32)])
    assets, cache_hashes = [], {}
    for directory, names in (("disc", DISC), ("map-pack", PACK)):
        for name in names:
            path = ROOT / "assets" / directory / "maps" / (name + ".map")
            c = Cache(path)
            try:
                cache_hashes[name] = file_sha(path)
                _, s = c.tag(c.scenario, "scnr")
                palette = c.block(s + 0xc0, 40)
                groups = c.block(s + 0xa0, 40)
                for q in c.block(s + 0xb8, 0x44):
                    datum = c.u32(palette[c.u16(q)] + 4)
                    _, d = c.tag(datum, "ctrl")
                    fields = [datum, c.u32(d + 0x38), c.u16(d + 0x11c), c.u16(d + 0x11e),
                              c.u32(d + 0x120), c.u32(d + 0x124), c.u32(d + 0x12c),
                              c.u32(d + 0x134), c.u32(d + 0x13c)]
                    assets.append(str(path.relative_to(ROOT)) + " " + " ".join(f"{v:x}" for v in fields))
                    group = struct.unpack_from("<h", c.data, q + 0x36)[0]
                    if group >= 0:
                        current, flags = c.u32(groups[group] + 32), c.u32(groups[group] + 36) & 1
                    else:
                        current, flags = fw(1. if c.u32(q + 0x38) & 1 else 0.), ((c.u32(q + 0x38) & 4) | 0x10) >> 2
                    # Each authored switch: initial activation then repeat using
                    # the actual group output, preserving the one-use marker.
                    row = [fields[2], fields[4], current, flags, 1, *fields[6:9]]
                    rows.append(row)
                    result = original(row)
                    rows.append([row[0], row[1], result[4], result[5], *row[4:]])
            finally:
                c.close()
    expected = [original(row) for row in rows]
    encoded = [" ".join(f"{v:x}" for v in row) for row in rows]
    native = subprocess.run([str(args.binary.resolve()), "--probe"],
                            input="\n".join(encoded) + "\n", text=True, capture_output=True, check=True)
    actual = [[int(v, 16) for v in line.split()] for line in native.stdout.splitlines()]
    if len(actual) != len(rows):
        raise RuntimeError("native control probe lost cases")
    failures = [{"case": i, "input": row, "source": a, "native": b}
                for i, (row, a, b) in enumerate(zip(rows, expected, actual)) if a != b]
    if args.write_fixture and not failures:
        args.write_fixture.parent.mkdir(parents=True, exist_ok=True)
        args.write_fixture.write_text("# Retail11BA50 and full1071E0; source XBE " + PIN + "\n" +
            "# Eight hex input words then six original result words.\n" +
            "\n".join(a + " " + " ".join(f"{v:x}" for v in b) for a, b in zip(encoded, expected)) + "\n")
    if args.write_assets and not failures:
        args.write_assets.parent.mkdir(parents=True, exist_ok=True)
        args.write_assets.write_text("# Independent raw Xbox control metadata, all four authored placements.\n" + "\n".join(assets) + "\n")
    report = {"original_executable_sha256": PIN, "native_binary_sha256": file_sha(args.binary),
              "cases": len(rows), "compared_words": len(rows) * 6, "exact_cases": len(rows) - len(failures),
              "cache_sha256": cache_hashes, "authored_control_placements": len(assets),
              "verified_original_instruction_addresses": len(visited), "failures": failures[:12],
              "services": {"000BAEB0": "empty object iterator", "00107980": "captured effect request"},
              "live_interaction_verified": False,
              "scope": "Control activation/group values and flags only; action targeting, linked-object notifications, motion, physics, effects playback and networking are excluded."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "cache_sha256"}, indent=2))
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
