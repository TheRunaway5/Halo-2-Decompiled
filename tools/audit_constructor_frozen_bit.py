#!/usr/bin/env python3
"""Literal D5060 request-bit producer and B7B40 C1 consumer, unchanged code."""
import hashlib
import json
import struct
import sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"


def main():
    x = Xbe(ROOT / "assets/disc/default.xbe")
    assert hashlib.sha256(x.data).hexdigest() == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x600000)
    for section in x.sections:u.mem_write(section.va, x.section_bytes(section))
    BASE, REQUEST, PLACEMENT, OBJECT, STACK = 0x2100000, 0x2110000, 0x2120000, 0x2130000, 0x2140000
    u.mem_map(BASE, 0x100000)
    def put(at, value):u.mem_write(at, struct.pack("<I", value & 0xffffffff))
    end = None
    def hook(g, address, size, user):
        if address == end:g.emu_stop()
    u.hook_add(UC_HOOK_CODE, hook)
    records = []
    for force in (0, 1):
        for object_type in range(32):
            for placement_frozen in (False, True):
                for request_flags in (0, 8, 1, 0xfffffff7, 0xffffffff, 0x12345678, 0x80000000, 0x55aa55aa):
                    u.mem_write(REQUEST, bytes(0x200));u.mem_write(PLACEMENT, bytes(0x200))
                    put(REQUEST + 0x18, request_flags)
                    put(PLACEMENT + 4, int(placement_frozen) << 8)
                    u.mem_write(PLACEMENT + 0x2e, bytes([object_type]))
                    u.mem_write(STACK, bytes(0x100));u.mem_write(STACK + 0x1c, bytes([force]))
                    u.mem_write(STACK + 0x14, bytes([1]));put(STACK + 0x18, 0x1234)
                    u.reg_write(UC_X86_REG_ESP, STACK);u.reg_write(UC_X86_REG_ESI, REQUEST)
                    u.reg_write(UC_X86_REG_EDI, PLACEMENT)
                    expected = bool((force and ((1 << object_type) & 0x1883)) or placement_frozen)
                    end = 0xd520e if expected else 0xd5233
                    u.emu_start(0xd51d4, end + 1, count=100)
                    assert u.reg_read(UC_X86_REG_EIP) == end
                    produced = struct.unpack("<I", u.mem_read(REQUEST + 0x18, 4))[0]
                    assert bool(produced & 8) == expected
                    assert (produced & ~8) == (request_flags & ~8)
                    u.mem_write(OBJECT, bytes([255]) * 0x200)
                    u.reg_write(UC_X86_REG_EBP, REQUEST);u.reg_write(UC_X86_REG_EDI, OBJECT)
                    end = 0xb7e72
                    u.emu_start(0xb7e4e, end + 1, count=100)
                    assert u.reg_read(UC_X86_REG_EIP) == end
                    c0, c1 = u.mem_read(OBJECT + 0xc0, 2)
                    assert c0 == 0 and c1 == int(expected)
                    records.append(dict(force=force, object_type=object_type,
                        placement_bit8=placement_frozen, request_before=request_flags,
                        request_after=produced, object_c0=c0, object_c1=c1))
    report = dict(xbe_sha256=PIN, cases=len(records), records=records,
        source_entries=["D5060 suffix D51D4..D5233", "B7B40 binder B7E4E..B7E72"],
        automatic_caller="D5560 D55FF/D5601 pushes literal1/literal1; BF0F0 BF12F/BF144 forwards its last argument to D5060 atBF152. Mask0x1883 includes source types7/11. Automatic BLOC therefore sets request+18bit3 and object+C1bit0 independent of placementbit8.",
        scope="Unchanged source request arithmetic/binder with explicit constructor inputs. Earlier factory admission/allocation and later physics/frozen transitions are separate. No source routine or result stub.")
    (ROOT / "analysis/multiplayer-shading/constructor-frozen-bit-original.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(dict(cases=len(records), errors=0), indent=2))


if __name__ == "__main__":main()
