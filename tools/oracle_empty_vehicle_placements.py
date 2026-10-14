#!/usr/bin/env python3
"""Execute retail factory admission on Headlong's unused vehicle placements.

Byte-identical placement and stack inputs are supplied. Original instructions
run without return hooks or patches; downstream construction must stay absent.
This proves the NONE-placement gate, not complete object construction/gameplay.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "tools"),
               str(ROOT / "tools/reference/halo2-decompiled/tools")]
from audit_rendering import Cache
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--map", type=Path, default=ROOT / "assets/disc/maps/headlong.map")
    parser.add_argument("--xbe", type=Path, default=ROOT / "assets/disc/default.xbe")
    parser.add_argument("--output", type=Path,
                        default=ROOT / "analysis/tags/empty-vehicles-original.json")
    args = parser.parse_args()
    image = Xbe(args.xbe)
    pin = hashlib.sha256(image.data).hexdigest()
    if pin != "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d":
        raise ValueError("the pinned owned retail executable is required")
    cache = Cache(args.map)
    try:
        _, scenario = cache.tag(cache.scenario, "scnr")
        placements = cache.block(scenario + 0x70, 0x54)
        if len(placements) != 7 or cache.block(scenario + 0x78, 0x28):
            raise ValueError("expected Headlong's seven empty vehicle placements")
        guest = Uc(UC_ARCH_X86, UC_MODE_32)
        guest.mem_map(0x10000, 0x600000)
        for section in image.sections:
            guest.mem_write(section.va, image.section_bytes(section))
        placement, stack, stop = 0x1000000, 0x200ff00, 0x3000000
        guest.mem_map(placement, 0x1000)
        guest.mem_map(0x2000000, 0x10000)
        guest.mem_map(stop, 0x1000)
        calls = []

        def hook(_guest, pc, _size, _context):
            if pc in (0xd5060, 0xb7b40, 0x108b10, 0xbf050, 0xa7870):
                calls.append(f"{pc:08x}")
                raise RuntimeError("unused placement reached original construction")

        guest.hook_add(UC_HOOK_CODE, hook)
        rows = []
        for ordinal, offset in enumerate(placements):
            raw = bytes(cache.data[offset:offset + 0x54])
            if struct.unpack_from("<h", raw)[0] != -1:
                raise ValueError("vehicle placement has a palette definition")
            guest.mem_write(placement, raw)
            guest.mem_write(stack, struct.pack("<6I", stop, 0, 0, 0, 0, 0))
            guest.reg_write(UC_X86_REG_ESP, stack)
            guest.reg_write(UC_X86_REG_EAX, placement)
            guest.emu_start(0xbf0f0, stop, count=10000)
            result = guest.reg_read(UC_X86_REG_EAX)
            if guest.reg_read(UC_X86_REG_EIP) != stop or result != 0xffffffff:
                raise RuntimeError("original factory did not return NONE")
            rows.append(dict(placement=ordinal, raw_placement=raw.hex(),
                             palette_index=-1, original_result=f"{result:08x}"))
        with args.map.open("rb") as stream:
            map_pin = hashlib.file_digest(stream, "sha256").hexdigest()
        report = dict(scope=__doc__, xbe_sha256=pin, map_sha256=map_pin,
                      entry="000bf0f0", body_bytes=207,
                      body_sha256=hashlib.sha256(image.read(0xbf0f0, 207)).hexdigest(),
                      records=rows, downstream_calls=calls, passed=True)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(report, indent=2) + "\n")
        print("7 unchanged retail NONE-placement rejections; zero downstream constructors")
    finally:
        cache.close()


if __name__ == "__main__":
    main()
