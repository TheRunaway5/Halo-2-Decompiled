#!/usr/bin/env python3
"""Original Vista loading-progress and loading-renderer fade instruction oracle.

Only external result boundaries are supplied. No original instruction is patched;
this does not run Direct3D, an actual loader, or a session/world handoff.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_FPCW,
    UC_X86_REG_FPSW, UC_X86_REG_FPTAG, UC_X86_REG_MXCSR)

SOURCE_SHA = "33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109"
BASE, STOP, DATA, STACK = 0x400000, 0x1800000, 0x1810000, 0x1900000
SP = STACK + 0x8000
IMPORT, FLOAT_RETURN = STOP + 0x100, STOP + 0x200
PATH = DATA + 0x100
PATH_BYTES = b"levels\\declared\\declared\0"
FP_CONTEXTS = [(0x027F, 0x1F80), (0x037F, 0x1F80), (0x007F, 0x1F80),
               (0x067F, 0x3F80), (0x0A7F, 0x5F80), (0x0E7F, 0x7F80),
               (0x027F, 0x9FC0)]


def word(value: int) -> int:
    return value & 0xFFFFFFFF


def bits(value: float) -> int:
    return struct.unpack("<I", struct.pack("<f", value))[0]


class Oracle:
    def __init__(self, image: bytes):
        self.uc = u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(BASE, (len(image) + 0xFFF) & ~0xFFF)
        u.mem_write(BASE, image)
        u.mem_map(STOP, 0x1000)
        u.mem_map(DATA, 0x4000)
        u.mem_map(STACK, 0x10000)
        # Declared floating query service: FLD dword [DATA]; RET. Original
        # callers execute their own arithmetic/rounding after this boundary.
        u.mem_write(FLOAT_RETURN, b"\xd9\x05" + struct.pack("<I", DATA) + b"\xc3")
        u.mem_write(PATH, "levels\\declared\\declared\0".encode("utf-16le"))
        self.mode = ""
        self.calls = []
        self.values = {}
        self.stop = STOP
        u.hook_add(UC_HOOK_CODE, self.hook)

    def read(self, address: int) -> int:
        return struct.unpack("<I", bytes(self.uc.mem_read(address, 4)))[0]

    def write(self, address: int, value: int):
        self.uc.mem_write(address, struct.pack("<I", word(value)))

    def ret(self, result: int = 0, pop: int = 0):
        u = self.uc
        sp = u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EAX, word(result))
        u.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
        u.reg_write(UC_X86_REG_EIP, self.read(sp))

    def floating(self, value: int):
        self.write(DATA, value)
        self.uc.reg_write(UC_X86_REG_EIP, FLOAT_RETURN)

    def reset(self, cw: int, mxcsr: int):
        u = self.uc
        self.calls = []
        u.reg_write(UC_X86_REG_FPCW, cw)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        u.reg_write(UC_X86_REG_MXCSR, mxcsr)
        u.reg_write(UC_X86_REG_ESP, SP)
        u.reg_write(UC_X86_REG_EBX, 0)
        u.mem_write(SP - 0x2000, bytes(0x3000))
        self.write(SP, STOP)

    def run(self, begin: int, stop: int = STOP):
        self.stop = stop
        self.uc.emu_start(begin, stop + 1, count=20000)
        if self.uc.reg_read(UC_X86_REG_EIP) != stop:
            raise RuntimeError(f"Original window {begin:08x} did not reach {stop:08x}")

    def hook(self, u: Uc, address: int, size: int, _: object):
        if address == self.stop:
            u.emu_stop()
            return
        sp = u.reg_read(UC_X86_REG_ESP)
        if self.mode == "progress":
            if address == IMPORT:
                args = [self.read(sp + 4 + i * 4) for i in range(8)]
                if args[:4] != [65001, 0, PATH, 0xFFFFFFFF] or args[5:] != [0x104, 0, 0]:
                    raise RuntimeError("Unexpected original path conversion arguments")
                self.calls.append("path")
                u.mem_write(args[4], PATH_BYTES)
                self.ret(len(PATH_BYTES), 32)
            elif address == 0x4317D2:
                if self.read(sp + 4) != 0 or bytes(u.mem_read(self.read(sp + 8), len(PATH_BYTES))) != PATH_BYTES:
                    raise RuntimeError("Unexpected original loading progress query arguments")
                self.calls.append("map")
                self.floating(self.values["map"])
            elif address == 0x60B183:
                self.calls.append("duration")
                self.ret(self.values["duration"])
            elif address == 0x437E51:
                self.calls.append("clock")
                self.ret(self.values["now"])
            elif address == 0x60B2B9:
                self.calls.append("secondary")
                self.floating(self.values["secondary"])
        elif self.mode == "begin" and address == IMPORT:
            self.calls.append("clock")
            self.ret(self.values["now"])
        elif self.mode == "fade" and address == 0x782052:
            self.calls.append(f"gain:{self.read(sp + 4):08x}")
            self.ret()

    def progress(self, cw: int, mxcsr: int, before: tuple[int, int, int],
                 supplied: tuple[int, int, int, int]) -> dict:
        self.reset(cw, mxcsr)
        self.mode = "progress"
        self.values = dict(zip(("map", "duration", "now", "secondary"), supplied))
        self.uc.mem_write(0xD78E08, struct.pack("<3I", *before))
        self.write(0x86D7D8, IMPORT)  # declared Windows conversion import boundary
        self.write(SP + 4, PATH)
        self.run(0x647F38)
        after = struct.unpack("<3I", bytes(self.uc.mem_read(0xD78E08, 12)))
        return {"after": list(after), "calls": self.calls.copy()}

    def begin(self, cw: int, mxcsr: int, active: int, start: int,
              gain: int, origin: int, now: int) -> dict:
        self.reset(cw, mxcsr)
        self.mode, self.values = "begin", {"now": now}
        self.uc.mem_write(0xE4AFDC, bytes([active]))
        self.write(0xE4AFE0, start)
        self.write(0x868530, gain)
        self.write(0xE4AF30, origin)
        self.write(0x79B22C, IMPORT)  # declared timeGetTime result boundary
        self.run(0x66A0B0)
        return {"after": [self.uc.mem_read(0xE4AFDC, 1)[0], self.read(0xE4AFE0), self.read(0x868530)],
                "calls": self.calls.copy()}

    def fade(self, cw: int, mxcsr: int, active: int, start: int,
             gain: int, origin: int, now: int) -> dict:
        self.reset(cw, mxcsr)
        self.mode = "fade"
        self.uc.mem_write(0xE4AFDC, bytes([active]))
        self.write(0xE4AFE0, start)
        self.write(0x868530, gain)
        self.write(0xE4AF30, origin)
        self.write(0xE3C6B4, DATA + 0x1000)
        self.write(DATA + 0x1000, DATA + 0x2000)
        self.uc.reg_write(UC_X86_REG_ESI, now)
        # Actual original uint32 clock conversion, retaining the original x87
        # multiply and float spill. Stop before the unowned device call.
        self.run(0x66A585, 0x66A5BB)
        self.uc.reg_write(UC_X86_REG_ESP, SP)
        # Entry initializes the actual local completion byte to zero.
        self.uc.mem_write(SP + 0x13, b"\0")
        self.run(0x66A9B7, 0x66AA07)
        result = {"after": [self.uc.mem_read(0xE4AFDC, 1)[0], self.read(0xE4AFE0), self.read(0x868530)],
                  "complete": self.uc.mem_read(SP + 0x13, 1)[0], "calls": self.calls.copy(),
                  "elapsed": self.read(SP + 0x14), "packed": None}
        if active:
            self.mode = "pack"
            self.write(DATA + 0x300, bits(1.0))
            for offset in (4, 8, 12):
                self.write(DATA + 0x300 + offset, result["after"][2])
            self.write(SP, STOP)
            self.write(SP + 4, DATA + 0x300)
            self.uc.reg_write(UC_X86_REG_ESP, SP)
            self.run(0x47602F)
            result["packed"] = self.uc.reg_read(UC_X86_REG_EAX)
        return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--fixture", type=Path, default=Path("tests/fixtures/vista_loading_original.tsv"))
    parser.add_argument("--report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/loading-original.json"))
    args = parser.parse_args()
    image = args.exe.read_bytes()
    if hashlib.sha256(image).hexdigest() != SOURCE_SHA:
        raise SystemExit("Unexpected Vista executable identity")
    oracle, rows = Oracle(image), []
    baseline = [bits(-1), bits(0), bits(.5), bits(.999), bits(1), bits(1.5), 0x80000000]
    queries = [bits(-.25), bits(0), bits(.25), bits(.75), bits(1), bits(1.25)]
    clocks = [(0, 0), (1, 1), (0xFFFFFFFE, 1), (1, 0xFFFFFFFF)]
    for cw, mxcsr in FP_CONTEXTS:
        for start in baseline:
            for query in queries:
                for duration in (0, 1, 1000, -1, 0x7FFFFFFF):
                    for previous, now in clocks:
                        before = (start, bits(.4), previous)
                        supplied = (query, word(duration), now, bits(.6))
                        result = oracle.progress(cw, mxcsr, before, supplied)
                        rows.append(dict(kind="P", label=f"p_{len(rows):05d}", cw=cw,
                                         mxcsr=mxcsr, before=list(before), supplied=list(supplied), **result))
        # Raw exceptional/subnormal and poisoned retained fields are explicit
        # controls, not assigned normal loader inputs.
        for raw in (1, 0x80000001, 0x7F7FFFFF, 0xFF7FFFFF, 0x7F800000,
                    0xFF800000, 0x7FC12345, 0x7F812345):
            for slot in range(4):
                before = [bits(-1), bits(0), 100]
                supplied = [bits(.5), 1000, 101, bits(.5)]
                if slot < 2:
                    before[slot] = raw
                else:
                    supplied[0 if slot == 2 else 3] = raw
                rows.append(dict(kind="P", label=f"raw_{len(rows):05d}", cw=cw,
                    mxcsr=mxcsr, before=before, supplied=supplied,
                    **oracle.progress(cw, mxcsr, tuple(before), tuple(supplied))))
        retained = [bits(-1), bits(0), 0xFFFFFF00]
        for index, (query, secondary, now) in enumerate(((0,.0,0xFFFFFF00),(.25,.2,0xFFFFFF10),
                (.5,.6,0),(.4,.4,16),(1,1,1000),(1,1,2000))):
            supplied = (bits(query), 1000, now, bits(secondary))
            result = oracle.progress(cw, mxcsr, tuple(retained), supplied)
            rows.append(dict(kind="P", label=f"sequence_{cw:04x}_{mxcsr:04x}_{index}", cw=cw,
                mxcsr=mxcsr, before=retained.copy(), supplied=list(supplied), **result))
            retained = result["after"]
        for origin, now in clocks:
            result = oracle.begin(cw, mxcsr, 0, bits(-3), bits(.8), origin, now)
            rows.append(dict(kind="B", label=f"begin_{len(rows):05d}", cw=cw, mxcsr=mxcsr,
                before=[0,bits(-3),bits(.8)], supplied=[origin,now], **result))
        for active in (0, 1, 255):
            for start in (bits(-1), bits(0), bits(.5), bits(1.5)):
                for now in (0, 1, 499, 500, 1499, 1500, 1501, 1999, 2000, 2001, 0xFFFFFFFF):
                    result = oracle.fade(cw, mxcsr, active, start, bits(.8), 0, now)
                    rows.append(dict(kind="F", label=f"fade_{len(rows):05d}", cw=cw,
                        mxcsr=mxcsr, before=[active,start,bits(.8)], supplied=[0,now], **result))
    lines = ["# H2V_LOADING_ORIGINAL_V1",
        "# P label cw mxcsr before_baseline progress previous_clock map duration now secondary after_baseline progress clock call_count calls",
        "# B label cw mxcsr before_active start gain origin now after_active start gain call_count calls",
        "# F label cw mxcsr before_active start gain origin now after_active start gain elapsed complete packed_or_dash call_count calls"]
    for row in rows:
        fields = [row["kind"], row["label"], f'{row["cw"]:04x}', f'{row["mxcsr"]:08x}']
        fields += [f"{word(x):08x}" for x in row["before"] + row["supplied"] + row["after"]]
        if row["kind"] == "F":
            fields += [f'{row["elapsed"]:08x}', str(row["complete"]),
                       "-" if row["packed"] is None else f'{row["packed"]:08x}']
        fields += [str(len(row["calls"])), ",".join(row["calls"]) or "-"]
        lines.append("\t".join(fields))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text("\n".join(lines) + "\n")
    ranges = {"progress": (0x647F38,0x6480D9), "secondary_getter": (0x638D4C,0x638DF8),
        "fade_begin": (0x66A0B0,0x66A0E2), "elapsed_conversion": (0x66A585,0x66A5BB),
        "fade_branch": (0x66A9B7,0x66AA07), "fade_arithmetic": (0x66AB1B,0x66AB5D),
        "fade_draw": (0x66B551,0x66B70F), "color_pack": (0x47602F,0x4760A0),
        "loading_fade_pixel_program": (0x7DCED0,0x7DD01C)}
    report = dict(version="H2V_LOADING_ORIGINAL_V1", source=str(args.exe.resolve()),
        source_sha256=SOURCE_SHA, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        counts={k:sum(r["kind"]==k for r in rows) for k in ("P","B","F")},
        fp_contexts=[dict(x87_control_word=f"{a:04x}",mxcsr=f"{b:08x}") for a,b in FP_CONTEXTS],
        instruction_ranges={k:dict(begin=f"{a:08x}",end_exclusive=f"{z:08x}",
            sha256=hashlib.sha256(image[a-BASE:z-BASE]).hexdigest()) for k,(a,z) in ranges.items()},
        boundary=dict(original_instructions_unchanged=True, gpu=False, imports_executed=False,
            progress="Whole647F38 executes with explicit Windows path-conversion/map-progress/duration/clock/secondary-result boundaries. Query float returns are loaded by a declared service thunk. Original stack cookie code executes.",
            fade="Whole66A0B0 executes with declared clock result. Separate original elapsed conversion and fade branch execute on actual retained globals/stack fields; no Direct3D call executes. Original47602F packs the resulting active RGBA color.",
            fp="Seven explicit harness FPCW/MXCSR contexts, not a claim about actual Vista loading-thread initialization.",
            excluded=["unowned query/import side effects and reentrancy", "loading owner admission/lifetime", "physical blending/raster output", "gameplay fade", "match-to-loading-to-PCR join", "Project Cartographer patches"]),
        pixel_program=dict(version="ps_2_0", pointer_table_slot="00868524", runtime_shader_slot="00e4afd4",
            arithmetic=["DCL v0", "MOV oC0,v0"], samplers=0, constants=0), cases=rows)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"counts":report["counts"],"fixture":str(args.fixture),
                      "fixture_sha256":report["fixture_sha256"]}))


if __name__ == "__main__":
    main()
