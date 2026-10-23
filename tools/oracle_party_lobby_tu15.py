#!/usr/bin/env python3
"""Run the genuine TU1.5 empty-slot, speaker, quantity and UI-sound leaves.

Widget lookup, voice queries and final render/audio submissions are explicit
providers. No native implementation supplies expected results. Run through
tools/run_test.py; only the bounded JSON report is retained when requested.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_XMM0)

PIN = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
DEFAULT = Path("/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
STOP, STACK = 0x3000000, 0x200f000
TEXT, GROUP, BACKGROUND, SPEAKER, ITEM = [0x1000000 + i * 0x1000 for i in range(5)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--xbe", type=Path, default=DEFAULT)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    assert os.environ.get("HALO2_TEST_SCRATCH"), "Use tools/run_test.py"
    xbe = Xbe(args.xbe)
    assert hashlib.sha256(xbe.data).hexdigest() == PIN
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x10000, 0x800000)
    for section in xbe.sections:
        uc.mem_write(section.va, xbe.section_bytes(section))
    uc.mem_map(0x1000000, 0x100000)
    uc.mem_map(0x2000000, 0x10000)
    uc.mem_map(STOP, 0x1000)
    read = lambda at: struct.unpack("<I", uc.mem_read(at, 4))[0]
    def write(at, value):
        uc.mem_write(at, struct.pack("<I", value & 0xffffffff))
    state = {}
    def returned(value=0, pop=0):
        sp = uc.reg_read(UC_X86_REG_ESP)
        uc.reg_write(UC_X86_REG_EAX, value)
        uc.reg_write(UC_X86_REG_EIP, read(sp))
        uc.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
    def hook(machine, address, size, data):
        sp = uc.reg_read(UC_X86_REG_ESP)
        if address == STOP:
            uc.emu_stop()
        elif address == 0x2357d5:  # find_child(type,ordinal,false)
            kind, index = read(sp+4), read(sp+8)
            owner = uc.reg_read(UC_X86_REG_ECX)
            value = TEXT if kind == 6 else GROUP if kind == 10 else (
                BACKGROUND if owner == GROUP and index == 1 else SPEAKER)
            returned(value, 12)
        elif address == 0x2ba733:  # final bitmap frame setter
            target = uc.reg_read(UC_X86_REG_ECX)
            state.setdefault("frames", []).append([target, uc.reg_read(UC_X86_REG_EDX) & 0xffff])
            returned()
        elif address in (0x53b70, 0x53780):
            returned(int(state["enabled"] if address == 0x53b70 else state["talking"]))
        elif address == 0x2b9eb2:  # unavailable rank provider, NONE
            assert uc.reg_read(UC_X86_REG_EDI) == 0xffffffff
            returned(pop=4)
        elif address == 0x235a5f:  # final player-skin submission
            item = uc.reg_read(UC_X86_REG_EAX)
            state["item_flags"] = read(item)
            state["blank_background"] = bool(uc.mem_read(item+0x5e, 1)[0])
            returned(pop=8)
        elif address == 0x14ae8d:
            returned(0x1080000)
        elif address == 0x18c4b0:  # nonlocalized sound submission
            state.setdefault("sounds", []).append([read(sp+4), uc.reg_read(UC_X86_REG_XMM0) & 0xffffffff])
            returned(pop=4)
    uc.hook_add(UC_HOOK_CODE, hook)
    def run(entry, registers=(), arguments=()):
        uc.mem_write(STACK, struct.pack("<"+"I"*(1+len(arguments)), STOP, *arguments))
        uc.reg_write(UC_X86_REG_ESP, STACK)
        for register, value in registers:
            uc.reg_write(register, value)
        uc.emu_start(entry, STOP+1, count=100000)
        assert uc.reg_read(UC_X86_REG_EIP) == STOP

    speakers = []
    for enabled, talking in [(False, False), (True, False), (True, True)]:
        state.clear(); state.update(enabled=enabled, talking=talking)
        uc.mem_write(SPEAKER, bytes(0x100))
        run(0x257c97, [(UC_X86_REG_EAX, 7), (UC_X86_REG_ECX, ITEM)])
        expected = int(not talking) if enabled else 2
        assert state["frames"] == [[SPEAKER, expected]]
        assert uc.mem_read(SPEAKER+0x6e, 1) == b"\1"
        speakers.append(dict(enabled=enabled, talking=talking, frame=expected))
    state.clear()
    uc.mem_write(SPEAKER+0x6e, b"\1")
    run(0x257a86, [(UC_X86_REG_ESI, ITEM)], [0])
    assert state["frames"] == [[BACKGROUND, 0]]
    assert state["item_flags"] == 0x20 and state["blank_background"]
    assert uc.mem_read(SPEAKER+0x6e, 1) == b"\0"
    blank = dict(flags=state["item_flags"], background_frame=0, speaker_visible=False)
    quantities = []
    for quantity in [-1, 0, 1, 2, 3, 4, 7]:
        uc.mem_write(ITEM+0x81, bytes([quantity & 255]))
        uc.reg_write(UC_X86_REG_ESI, ITEM)
        uc.reg_write(UC_X86_REG_EBP, STACK)
        uc.emu_start(0x25676c, 0x256785, count=20)
        result = struct.unpack("<i", uc.mem_read(STACK-8, 4))[0]
        assert result == (1 if quantity == 4 else quantity)
        quantities.append([quantity, result])
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    try:
        menu_sha=hashlib.sha256(cache.data).hexdigest()
        assert menu_sha == "1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b"
        _, skin = cache.tag(0xe5200300, "skin")
        children = cache.block(skin+36, 56)
        widgets=[]
        for index, position in [(1, (-20, 5)), (3, (0, 5))]:
            child=children[index]
            assert struct.unpack_from("<IHHHHhh", cache.data, child) == (1,11,250,0,0,*position)
            assert struct.unpack_from("<h", cache.data, child+32)[0] == 10
            widgets.append(dict(ordinal=index, definition_hex=bytes(cache.data[child:child+56]).hex(),
                                bitmap=cache.tags[cache.u32(child+28)]["name"]))
        tag = next(t for t in cache.tags.values() if t["group"] == "wigl")
        at = tag["address"] - cache.magic
        slots = [cache.u32(at+0x94+i*8) for i in range(13)]
    finally:
        cache.close()
    table = 0x1090000
    write(0x4f0414, table)
    for slot, datum in enumerate(slots):
        write(0x1080000+0x94+slot*8, datum)
        if datum != 0xffffffff:
            write(table+(datum & 0xffff)*16+8, 0x10a0000)
    requests = []
    for slot in range(14):
        state.clear()
        run(0x23cc7e, [(UC_X86_REG_EAX, slot)])
        expected = [[slots[slot], 0x3f800000]] if slot < 13 and slots[slot] != 0xffffffff else []
        assert state.get("sounds", []) == expected
        requests.append(dict(slot=slot, requests=expected))
    entries = [(0x257a86, 106), (0x257c97, 68), (0x25676c, 25), (0x23cc7e, 279), (0x2b9bbe, 665)]
    report = dict(passed=True, xbe_sha256=PIN, mainmenu_sha256=menu_sha,
                  empty_slot_widgets=widgets, speaker_cases=speakers, empty_slot=blank,
                  quantity_cases=quantities, sound_requests=requests,
                  original_leaves=[dict(entry=f"{a:08x}", bytes=n,
                    sha256=hashlib.sha256(uc.mem_read(a,n)).hexdigest()) for a,n in entries],
                  limits="Supplied widget/voice/rank/audio sinks; no original network event timing or physical audio proof")
    if args.report_output:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(report, indent=2)+"\n")
    print("PASS TU1.5: three speaker states, blank slot without speaker, seven quantities, fourteen sound slots/bounds")


if __name__ == "__main__":
    main()
