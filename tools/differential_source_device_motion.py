#!/usr/bin/env python3
"""Execute complete retail106790/11EED0 for ordinary group-driven devices.

Animation bank is explicitly NONE. Pose dirty/effect and initialization physics
services are captured boundaries, not emulated Havok or effect playback.
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
from unicorn.x86_const import *
from audit_rendering import Cache, file_sha
from audit_map_world_features import DISC, PACK

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
BANK, STACK, STOP = 0x2100000, 0x21fff00, 0x3000000
HEADER, POOL, OBJECT, TAGS, DEFINITION, GROUP_HEADER, GROUP_POOL, CLOCK = [BANK + n * 0x1000 for n in range(8)]
HANDLE = 0x10001
def fw(v):
    return struct.unpack("<I", struct.pack("<f", v))[0]
def ff(v):
    return struct.unpack("<f", struct.pack("<I", v))[0]

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--binary", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--write-fixture", type=Path)
    args = p.parse_args()
    x = Xbe(ROOT / "assets/disc/default.xbe")
    if hashlib.sha256(x.data).hexdigest() != PIN:
        raise ValueError("unadmitted original executable")
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x600000)
    for section in x.sections:
        u.mem_write(section.va, x.section_bytes(section))
    u.mem_map(BANK, 0x100000)
    u.mem_map(STOP, 0x1000)
    def put(at, v):
        u.mem_write(at, struct.pack("<I", v & 0xffffffff))
    def word(at):
        return struct.unpack("<I", u.mem_read(at, 4))[0]
    def ret(pop=0):
        sp = u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EIP, word(sp))
        u.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
    put(0x4e0300, HEADER); put(HEADER + 0x44, POOL)
    put(POOL + 20, OBJECT); u.mem_write(POOL + 15, b"\x07")
    put(OBJECT, 0x10002); u.mem_write(OBJECT + 0xaa, b"\x07")
    u.mem_write(OBJECT + 0x12a, b"\xff\xff")
    put(0x4e3b44, TAGS); put(TAGS + 40, DEFINITION)
    put(0x4e0328, GROUP_HEADER); put(GROUP_HEADER + 0x44, GROUP_POOL)
    put(0x510c54, CLOCK)
    observed = {}; visited = set(); service_counts = {}
    def hook(uc, pc, size, unused):
        if pc in (0x107520, 0x107980, 0xbd020, 0x115b10):
            service_counts[pc] = service_counts.get(pc, 0) + 1
            if pc == 0x107520:
                if word(uc.reg_read(UC_X86_REG_ESP) + 4) != HANDLE:
                    raise RuntimeError("motion dirty changed owner")
                put(OBJECT + 0x12c, word(OBJECT + 0x12c) | 4)
                observed["dirty"] += 1
            elif pc == 0x107980:
                if uc.reg_read(UC_X86_REG_EDX) != HANDLE:
                    raise RuntimeError("motion effect changed owner")
                observed["effects"].append(uc.reg_read(UC_X86_REG_EAX))
            ret(0 if pc == 0x107980 else 4)
            return
        if not (0x106790 <= pc < 0x106d67 or 0x11eed0 <= pc < 0x11f0c2):
            raise RuntimeError(f"unexpected original instruction {pc:08x}")
        if bytes(uc.mem_read(pc, size)) != x.read(pc, size):
            raise RuntimeError("original motion instruction changed")
        visited.add(pc)
    u.hook_add(UC_HOOK_CODE, hook)
    def original(row):
        for offset, v in zip((0xbc,0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0x10c),row[:8]):
            put(DEFINITION + offset, v)
        for offset, v in ((0xe0,0x101),(0xe8,0x102),(0xf0,0x103),(0xf8,0x104),(0x114,0x105)):
            put(DEFINITION + offset, v)
        for offset, v in zip((0x140,0x144,0x134,0x138,0x12c),row[8:13]):
            put(OBJECT + offset, v)
        u.mem_write(OBJECT + 0x148, struct.pack("<H", row[13] & 65535))
        put(OBJECT + 0x13c, 0x10000 if row[16] & 1 else 0xffffffff)
        put(OBJECT + 0x130, 0x10001 if row[16] & 2 else 0xffffffff)
        put(GROUP_POOL + 4, row[14]); put(GROUP_POOL + 16, row[15])
        put(CLOCK + 4, row[17]); u.mem_write(OBJECT + 0xb3, bytes([row[18]]))
        observed.update(dirty=0, effects=[])
        u.reg_write(UC_X86_REG_FPCW, 0x027f); u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        u.reg_write(UC_X86_REG_ESP, STACK); put(STACK, STOP); put(STACK + 4, HANDLE)
        u.emu_start(0x106790, STOP, count=5000)
        if u.reg_read(UC_X86_REG_EIP) != STOP or u.reg_read(UC_X86_REG_ESP) != STACK + 8:
            raise RuntimeError("motion original did not return")
        effects = observed["effects"]
        if len(effects) > 1:
            raise RuntimeError("motion original produced multiple position effects")
        return [word(OBJECT + offset) for offset in (0x140,0x144,0x134,0x138,0x12c)] + [
            struct.unpack("<H", u.mem_read(OBJECT + 0x148,2))[0],
            u.reg_read(UC_X86_REG_EAX) & 255, observed["dirty"], len(effects), effects[0] if effects else 0xffffffff]
    rows = []
    def add(definition, state, targets, mask=3, dt=1/30, retain=0):
        rows.append([definition[0], *map(fw, definition[1:]), *map(fw,state[:4]), *state[4:],
                     *map(fw,targets), mask, fw(dt), retain])
    base = [0, .5, 3, .2, 900, 0, 0, 0]
    for loop in (0,1):
        for pos in (0,.001,.25,.5,.99,1):
            for vel in (-2,-.2,0,.2,2):
                for target in (0,.25,.5,1):
                    for flags in (0,4,0x80,0x100,0x180):
                        add([loop,*base[1:]],[pos,vel,1,0,flags,0],[target,1])
    rng=random.Random(0x106790)
    for i in range(1500):
        d=[i%2, rng.uniform(0,4),rng.uniform(0,100),rng.uniform(0,4),rng.uniform(0,100),
           rng.uniform(0,4),rng.uniform(0,100),rng.uniform(0,.5)]
        s=[rng.random(),rng.uniform(-5,5),rng.random(),rng.uniform(-5,5),rng.choice((0,4,0x80,0x100)),rng.randrange(-3,20)]
        add(d,s,[rng.random(),rng.random()],i%4, rng.choice((1/30,1/60,.125)), i%2)
    # Delay/settle/zero-rate/wrapping-delay admission boundaries.
    for delay in (0,.033333333,.1,1):
        for ticks in (0,1,2,29,30,32767,65535):
            for speed,acc in ((0,0),(.2,0),(0,900),(.2,900)):
                add([0,1,900,speed,acc,0,0,delay],[0,0,1,0,0x80,ticks],[1,1])
    # Independently read every authored machine/control's ordinary controller.
    hashes={}; authored=0
    for directory,names in (("disc",DISC),("map-pack",PACK)):
        for name in names:
            path=ROOT/"assets"/directory/"maps"/(name+".map")
            c=Cache(path)
            try:
                hashes[name]=file_sha(path);_,s=c.tag(c.scenario,"scnr")
                for offset,stride,palette_offset,group in ((0xa8,0x48,0xb0,"mach"),(0xb8,0x44,0xc0,"ctrl")):
                    palette=c.block(s+palette_offset,40)
                    for q in c.block(s+offset,stride):
                        datum=c.u32(palette[c.u16(q)]+4);_,d=c.tag(datum,group)
                        definition=[c.u32(d+0xbc)]+[ff(c.u32(d+off)) for off in (0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0x10c)]
                        # Gear progression is a later callback; the common
                        # controller is independently exercised here.
                        for target in (0,1):
                            state=[1-target,0,1,0,0x80,0]
                            for tick in range(180):
                                add(definition,state,[target,1])
                                result=original(rows[-1]); state=[*map(ff,result[:4]),result[4],result[5]]
                        authored+=1
            finally:
                c.close()
    expected=[original(row) for row in rows]
    encoded=[" ".join(f"{v:x}" for v in r) for r in rows]
    result=subprocess.run([str(args.binary.resolve()),"--probe"], input="\n".join(encoded)+"\n",
                          text=True,capture_output=True,check=True)
    actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
    if len(actual)!=len(rows):
        raise RuntimeError("native device motion probe lost rows")
    failures=[{"case":i,"input":row,"source":a,"native":b} for i,(row,a,b) in enumerate(zip(rows,expected,actual)) if a!=b]
    if args.write_fixture and not failures:
        args.write_fixture.write_text("# Complete original106790/11EED0; XBE "+PIN+"\n# 19 hex input words then 10 original output words.\n"+
            "\n".join(a+" "+" ".join(f"{v:x}" for v in b) for a,b in zip(encoded,expected))+"\n")
    report={"original_executable_sha256":PIN,"native_binary_sha256":file_sha(args.binary),
            "cases":len(rows),"exact_cases":len(rows)-len(failures),"compared_words":len(rows)*10,
            "authored_device_placements":authored,"verified_original_instruction_addresses":len(visited),
            "services":{f"{k:08x}":v for k,v in service_counts.items()},"failures":failures[:20],
            "cache_sha256":hashes,"live_interaction_verified":False,
            "scope":"Ordinary common device state and event requests, animation bank NONE. Physics, animation events, playback, action admission and replication are separate consumers."}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps({k:v for k,v in report.items() if k!="cache_sha256"},indent=2))
    return bool(failures)
if __name__=="__main__":
    raise SystemExit(main())
