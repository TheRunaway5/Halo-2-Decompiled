#!/usr/bin/env python3
"""Compare control targeting against unchanged original Xbox instructions.

Mode0 executes complete151110, enumerating one explicitly supplied control.
Mode1 enters complete1516C0 with an explicitly supplied incumbent action.
Both execute11E5E0,1078F0,107870 and151320 without instruction replacement.
Mode2 executes11B980 on a cold control to compare its initial flags.
BB050 spatial membership, CAFC0 unit center, B8D30 posed marker, BADC0 typed
lookup and B9DD0 world roots are explicit services, not proven producers.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                              UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
from audit_rendering import Cache, file_sha
from audit_map_world_features import DISC, PACK

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe

PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
BANK, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000
OBJECT_HEADER, POOL, UNIT, CONTROL, PLAYER_HEADER, PLAYER, GROUP_HEADER, GROUPS, BEST = [
    BANK + n * 0x1000 for n in range(9)]
PLACEMENT = BANK + 0x9000
UNIT_ID, CONTROL_ID, PREVIOUS_ID, PLAYER_ID = 0x10001, 0x10002, 0x10003, 0x10000


def fw(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def f32(value):
    return struct.unpack("<f", struct.pack("<f", value))[0]


def cache_float(cache, offset):
    return struct.unpack("<f", struct.pack("<I", cache.u32(offset)))[0]


def base(mode=0, **change):
    row = dict(mode=mode, parent=0xffffffff, unit_bound=[0., 0., 0.], unit_radius=.5,
               center=[0., 0., 0.], forward=[1., 0., 0.], unit_root=[0., 0., 0.],
               object=CONTROL_ID, bound=[.5, 0., 0.], radius=.2, root=[.5, 0., 0.],
               position_present=1, position_flags=1, power_present=1, power=1.,
               device_flags=0, control_flags=0, marker_count=1, marker=[-1., 0., 0.],
               action=0, previous=PREVIOUS_ID, previous_root=[1., 0., 0.])
    row.update(change)
    return row


def encode(r):
    return [r["mode"], r["parent"], *map(fw, r["unit_bound"]), fw(r["unit_radius"]),
            *map(fw, r["center"]), *map(fw, r["forward"]), *map(fw, r["unit_root"]),
            r["object"], *map(fw, r["bound"]), fw(r["radius"]), *map(fw, r["root"]),
            r["position_present"], r["position_flags"], r["power_present"], fw(r["power"]),
            r["device_flags"], r["control_flags"], r["marker_count"], *map(fw, r["marker"]),
            r["action"], r["previous"], *map(fw, r["previous_root"])]


class Oracle:
    def __init__(self):
        self.image = Xbe(ROOT / "assets/disc/default.xbe")
        if hashlib.sha256(self.image.data).hexdigest() != PIN:
            raise ValueError("unsupported original executable")
        self.g = Uc(UC_ARCH_X86, UC_MODE_32)
        self.g.mem_map(0x10000, 0x600000)
        for section in self.image.sections:
            self.g.mem_write(section.va, self.image.section_bytes(section))
        self.g.mem_map(BANK, 0x100000)
        self.g.mem_map(STOP, 0x1000)
        self.put(0x4e0300, OBJECT_HEADER)
        self.put(OBJECT_HEADER + 0x44, POOL)
        self.put(POOL + 20, UNIT)
        self.put(POOL + 32, CONTROL)
        self.put(0x4e8c24, PLAYER_HEADER)
        self.put(PLAYER_HEADER + 0x44, PLAYER)
        self.put(PLAYER + 0x2c, UNIT_ID)
        self.put(0x4e0328, GROUP_HEADER)
        self.put(GROUP_HEADER + 0x44, GROUPS)
        self.g.mem_write(CONTROL + 0xaa, b"\x08")
        self.visited, self.service_counts = set(), {}
        self.g.hook_add(UC_HOOK_CODE, self.hook)

    def put(self, address, value):
        self.g.mem_write(address, struct.pack("<I", value & 0xffffffff))

    def word(self, address):
        return struct.unpack("<I", self.g.mem_read(address, 4))[0]

    def vector(self, address, values):
        self.g.mem_write(address, struct.pack("<fff", *values))

    def ret(self, pop=0):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        self.g.reg_write(UC_X86_REG_EIP, self.word(sp))
        self.g.reg_write(UC_X86_REG_ESP, sp + 4 + pop)

    def hook(self, g, pc, size, user):
        r = self.row
        sp = g.reg_read(UC_X86_REG_ESP)
        if pc in (0xbb050, 0xcafc0, 0xb8d30, 0xbadc0, 0xb9dd0):
            self.service_counts[f"{pc:08x}"] = self.service_counts.get(f"{pc:08x}", 0) + 1
        if pc == 0xbb050:
            if (self.word(sp + 4), self.word(sp + 8), self.word(sp + 28)) != (0, 0xffffebcf, 64):
                raise RuntimeError("original spatial query contract changed")
            expected = fw(f32(f32(r["unit_radius"]) + f32(.4)) + f32(3.))
            if self.word(sp + 20) != expected:
                raise RuntimeError("original spatial query radius changed")
            if self.word(sp + 16) != UNIT + 0x30:
                raise RuntimeError("original spatial query center changed")
            self.put(self.word(sp + 24), r["object"])
            g.reg_write(UC_X86_REG_EAX, 1)
            self.ret(28)
            return
        if pc == 0xcafc0:
            if g.reg_read(UC_X86_REG_ECX) != UNIT_ID:
                raise RuntimeError("original unit-center owner changed")
            self.vector(g.reg_read(UC_X86_REG_EAX), r["center"])
            self.ret()
            return
        if pc == 0xbadc0:
            if self.word(sp + 4) != 0x100 or g.reg_read(UC_X86_REG_EAX) != r["object"]:
                raise RuntimeError("original typed control lookup changed")
            g.reg_write(UC_X86_REG_EAX, CONTROL)
            self.ret(4)
            return
        if pc == 0xb8d30:
            if [self.word(sp + n) for n in (4, 8, 12)] != [r["object"], 0x500008f, 1]:
                raise RuntimeError("original control marker query changed")
            self.vector(g.reg_read(UC_X86_REG_EBX) + 0x3c, r["marker"])
            g.reg_write(UC_X86_REG_EAX, r["marker_count"])
            self.ret(12)
            return
        if pc == 0xb9dd0:
            owner = g.reg_read(UC_X86_REG_EAX)
            points = {UNIT_ID: r["unit_root"], r["object"]: r["root"],
                      r["previous"]: r["previous_root"]}
            self.vector(g.reg_read(UC_X86_REG_ESI), points[owner])
            self.ret()
            return
        ranges = ((0x151110, 0x1512fc), (0x1516c0, 0x151774), (0x11e5e0, 0x11e6ef),
                  (0x107870, 0x1078ef), (0x1078f0, 0x107973), (0x151320, 0x151423),
                  (0x11b980, 0x11b9ce))
        if not any(start <= pc < end for start, end in ranges):
            raise RuntimeError(f"unexpected original targeting instruction {pc:08x}")
        if bytes(g.mem_read(pc, size)) != self.image.read(pc, size):
            raise RuntimeError("original targeting instruction changed")
        self.visited.add(pc)

    def original(self, row):
        self.row = row
        self.put(UNIT + 0x14, row["parent"])
        self.vector(UNIT + 0x30, row["unit_bound"])
        self.put(UNIT + 0x3c, fw(row["unit_radius"]))
        self.vector(UNIT + 0x168, row["forward"])
        self.vector(CONTROL + 0x30, row["bound"])
        self.put(CONTROL + 0x3c, fw(row["radius"]))
        self.put(CONTROL + 0x12c, row["device_flags"])
        self.put(CONTROL + 0x1cc, row["control_flags"])
        self.put(CONTROL + 0x130, 0x10000 if row["power_present"] else 0xffffffff)
        self.put(CONTROL + 0x13c, 0x10001 if row["position_present"] else 0xffffffff)
        self.g.mem_write(GROUPS, struct.pack("<HHII", 1, 0, fw(row["power"]), 0))
        self.g.mem_write(GROUPS + 12, struct.pack("<HHII", 1, row["position_flags"], 0, 0))
        before = (row["action"], row["previous"]) if row["mode"] else (0, 0xffffffff)
        self.g.mem_write(BEST, struct.pack("<III", *before, 0xffffffff))
        self.g.reg_write(UC_X86_REG_FPCW, 0x027f)
        self.g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        self.g.reg_write(UC_X86_REG_ESP, STACK)
        self.put(STACK, STOP)
        self.put(STACK + 4, PLAYER_ID)
        self.put(STACK + 8, BEST)
        self.g.reg_write(UC_X86_REG_EAX, row["object"])
        entry = 0x1516c0 if row["mode"] else 0x151110
        if row["mode"] == 2:
            self.put(CONTROL + 0x1cc, 0)
            self.put(PLACEMENT + 0x3c, row["control_flags"])
            self.put(PLACEMENT + 0x40, 0)
            self.put(STACK + 4, row["object"])
            self.put(STACK + 8, PLACEMENT)
            entry = 0x11b980
        self.g.emu_start(entry, STOP, count=10000)
        if self.g.reg_read(UC_X86_REG_EIP) != STOP:
            raise RuntimeError("original control targeting did not return")
        if row["mode"] == 2:
            return [0, self.word(CONTROL + 0x1cc), 0xffffffff]
        after = (self.word(BEST), self.word(BEST + 4))
        if self.word(BEST + 8) != 0xffffffff:
            raise RuntimeError("target ranking altered unrelated action-record word")
        return [int(after != before), *after]


def cases():
    rows = []
    for mode in (0, 1):
        for flags in (0, 1, 2, 3, 4, 7, 0xff00, 0xffff):
            for power in (-1., 0., .5, f32(1.-2**-24), 1., f32(1.+2**-23), 2., float("nan")):
                for device in (0, 1, 2, 3, 0xfffc, 0xffff):
                    for both in (0, 1):
                        rows.append(base(mode, position_flags=flags, power=power,
                                         device_flags=device, control_flags=both))
        rows.extend([base(mode, position_present=0), base(mode, position_present=0, power_present=0)])
        for count in (0, 1):
            for flags in (0, 1, 2, 3):
                for marker in ([1., 0., 0.], [-1., 0., 0.], [0., 1., 0.], [0., 0., 0.],
                               [1., -1., 1.], [2**-126, 0., 0.]):
                    rows.append(base(mode, marker_count=count, control_flags=flags, marker=marker))
        for x in (-.2, 0., .2, .5, .8, 1., 1.2, 1.4, 1.5, 3.):
            for y in (0., .2, f32(.2-2**-26), f32(.2+2**-26), 1.):
                for direction in ([1., 0., 0.], [-1., 0., 0.], [0., 0., 0.], [.5, 0., 0.], [2., 0., 0.]):
                    rows.append(base(mode, bound=[x, y, 0.], forward=direction, marker_count=0))
    for parent in (0, UNIT_ID, CONTROL_ID):
        rows.append(base(parent=parent))
    for priority in (-32768, -1, 0, 1, 2, 3, 4, 7, 32767):
        for previous in ([.5, 0., 0.], [-.5, 0., 0.], [.25, 0., 0.], [2., 0., 0.]):
            rows.append(base(1, action=(0x89ab << 16) | (priority & 0xffff), previous_root=previous))
    #151320 accumulates candidate and incumbent axes in different orders.
    # Equal geometric roots can therefore produce distinct rounded distances.
    for root in ([4096., 1., 1.], [1., 4096., 1.], [1., 1., 4096.],
                 [-4096., 1., 1.], [2**-60, 1., 2**-60]):
        rows.append(base(1, action=3, root=root, previous_root=root))
    for mode in (0, 1):
        for marker in ([-16777216., 16777216., 1.], [16777216., -16777216., 1.],
                       [-1., 1., 2**-24], [1., -1., 2**-24]):
            rows.append(base(mode, bound=[.3, .3, .3], forward=[1., 1., 1.], marker=marker))
    # Cancellation and strict endpoint/boundary cases retain binary32 inputs.
    rng = random.Random(0x1516c0)
    for n in range(4096):
        origin = [rng.uniform(-100., 100.) for _ in range(3)]
        forward = [rng.uniform(-2., 2.) for _ in range(3)]
        t = rng.uniform(-.5, 1.5)
        radius = rng.uniform(0., 1.)
        center = [f32(origin[i] + forward[i]*t + rng.uniform(-radius, radius)) for i in range(3)]
        rows.append(base(n % 2, center=origin, forward=forward, bound=center,
                         unit_bound=origin, unit_radius=rng.uniform(0., 1.), radius=radius,
                         marker=[rng.uniform(-1., 1.) for _ in range(3)], marker_count=n % 2,
                         position_flags=n % 4, power=1. if n % 4 else .99,
                         device_flags=0 if n % 5 else 2, control_flags=n % 2,
                         action=(0x1234 << 16) | (n % 8),
                         root=[rng.uniform(-100., 100.) for _ in range(3)],
                         previous_root=[rng.uniform(-100., 100.) for _ in range(3)]))
    rows += [base(2, control_flags=n) for n in range(256)]
    rows += [base(2, control_flags=rng.getrandbits(32)) for _ in range(256)]
    return rows


def asset_cases():
    rows, identities, hashes = [], [], {}
    for directory, names in (("disc", DISC), ("map-pack", PACK)):
        for name in names:
            path = ROOT / "assets" / directory / "maps" / (name + ".map")
            c = Cache(path)
            try:
                hashes[name] = file_sha(path)
                _, s = c.tag(c.scenario, "scnr")
                palette, groups = c.block(s + 0xc0, 40), c.block(s + 0xa0, 40)
                for index, p in enumerate(c.block(s + 0xb8, 0x44)):
                    datum = c.u32(palette[c.u16(p)] + 4)
                    _, d = c.tag(datum, "ctrl")
                    group = struct.unpack_from("<h", c.data, p + 0x36)[0]
                    flags = c.u32(groups[group] + 36) & 1 if group >= 0 else ((c.u32(p + 0x38) & 4) | 0x10) >> 2
                    power_group = struct.unpack_from("<h", c.data, p + 0x34)[0]
                    power = cache_float(c, groups[power_group] + 32) if power_group >= 0 else (0. if c.u32(p + 0x38) & 2 else 1.)
                    control_flags = (c.u32(p + 0x3c) & 1) | ((c.u32(p + 0x3c) & 0x10) >> 3)
                    placement = [cache_float(c, p + 8 + i*4) for i in range(3)]
                    identities.append(dict(map=name, placement=index, datum=f"{datum:08x}",
                                           position_group=group, power_group=power_group,
                                           control_flags=control_flags, position_flags=flags))
                    for mode in (0, 1):
                        for used in (False, True):
                            # Authored group/flags/radius with explicitly supplied
                            # near-front geometry and marker, not a pose producer.
                            rows.append(base(mode, radius=cache_float(c, d + 4), root=placement,
                                             position_flags=flags | (2 if used else 0), power=power,
                                             control_flags=control_flags, action=3 if mode else 0,
                                             previous_root=[100., 100., 100.]))
            finally:
                c.close()
    if len(identities) != 4:
        raise ValueError("expected all four supplied Xbox control placements")
    return rows, identities, hashes


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--write-fixture", type=Path)
    args = p.parse_args()
    oracle = Oracle()
    rows = cases()
    asset_rows, identities, hashes = asset_cases()
    rows += asset_rows
    expected = [oracle.original(row) for row in rows]
    inputs = [encode(row) for row in rows]
    if any(len(row) != 38 for row in inputs):
        raise RuntimeError("native control target input layout drifted")
    encoded = [" ".join(f"{word:x}" for word in row) for row in inputs]
    native = subprocess.run([str(args.binary.resolve()), "--probe"],
                            input="\n".join(encoded)+"\n", text=True, capture_output=True, check=True)
    actual = [[int(word, 16) for word in line.split()] for line in native.stdout.splitlines()]
    if len(actual) != len(expected):
        raise RuntimeError("native control target probe lost cases")
    failures = [dict(case=i, input=inputs[i], source=a, native=b)
                for i, (a, b) in enumerate(zip(expected, actual)) if a != b]
    if args.write_fixture and not failures:
        args.write_fixture.parent.mkdir(parents=True, exist_ok=True)
        args.write_fixture.write_text("# Explicit original service inputs, then replacement/action/object outputs.\n"+
                                      "\n".join(line+" "+" ".join(f"{v:x}" for v in result)
                                                for line, result in zip(encoded, expected))+"\n")
    report = dict(executable_sha256=PIN, binary_sha256=file_sha(args.binary), cases=len(rows),
                  output_words=len(rows)*3, mismatches=failures, accepted=sum(r[0] for r in expected),
                  complete_original_entries=["00151110", "001516c0", "0011e5e0", "001078f0", "00107870", "00151320", "0011b980"],
                  unique_original_instructions=len(oracle.visited),
                  instruction_bytes_verified=True, service_calls=oracle.service_counts,
                  authored_controls=identities, cache_sha256=hashes,
                  explicit_service_boundaries={"000bb050": "one supplied control candidate; real spatial membership/order not proven",
                    "000cafc0": "supplied actual unit-center result; observer/animation/contact producers not executed",
                    "000badc0": "supplied successful typed-control lookup",
                    "000b8d30": "supplied posed control-marker result, original maximum1",
                    "000b9dd0": "supplied actual world roots; parent transforms not executed"},
                  live_player_input_connected=False, objective_achieved=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2)+"\n")
    print(json.dumps(dict(cases=len(rows), output_words=len(rows)*3, mismatches=len(failures),
                          original_instructions=len(oracle.visited), accepted=report["accepted"])))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
