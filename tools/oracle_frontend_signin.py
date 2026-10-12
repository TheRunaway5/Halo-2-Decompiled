#!/usr/bin/env python3
"""Freeze original252554 screen30 visibility with declared controller inputs.

The original screen update, visibility writes, per-slot child-index selection,
model species visibility prefix, controller frame validation, and controller
iterator execute unchanged. Widget lookup, controller/profile retrieval,
string binding, appearance rendering, and base widget animation are supplied
environment seams. No native visibility implementation supplies output.
"""
import hashlib
import itertools
import json
import struct
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
                              UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP)

r = Retail()
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
screen_tag, screen_data = cache.tag(next(k for k, t in cache.tags.items()
    if t["group"] == "wgit" and cache.u16(t["address"]-cache.magic+4) == 30), "wgit")
pane = cache.block(screen_data+32, 76)[0]
definitions = cache.block(pane+36, 56)
controller_tag = cache.u32(definitions[7]+28)
_, bitmap_data = cache.tag(controller_tag, "bitm")
bitmap_header = r.alloc(cache.data[bitmap_data:bitmap_data+0x4c])
r.w(r.table+(controller_tag & 0xffff)*16+8, bitmap_header)
controller_definition = r.alloc(cache.data[definitions[7]:definitions[7]+56])
widgets = {(kind, index): r.alloc(bytes(0x100))
           for kind, count, first in [(6, 24, 2), (7, 8, 0), (8, 11, 0)]
           for index in range(first, first+count)}
reverse = {value: key for key, value in widgets.items()}
screen = r.alloc(bytes(0x2000))
states = [(False, False, False, 0, 0)]*4
strings = {}


def returned(value=0, pop=0):
    sp = r.g.reg_read(UC_X86_REG_ESP)
    r.ret(value)
    r.g.reg_write(UC_X86_REG_ESP, sp+4+pop)


def hook(machine, address, size, context):
    sp = machine.reg_read(UC_X86_REG_ESP)
    if address == 0x22ee92:
        returned(pop=4)  # No extra list-controller flags in the supplied screen.
    elif address == 0x18f8b8:
        returned(int(states[r.r(sp+4)][0]), 4)
    elif address == 0x18fc44:
        controller = machine.reg_read(UC_X86_REG_ECX)
        profile = machine.reg_read(UC_X86_REG_EAX)
        machine.mem_write(profile, bytes(0x1e0))
        machine.mem_write(profile+0x118+4, bytes([states[controller][4]]))
        returned(1)
    elif address == 0x22edb8:
        returned(widgets.get((r.r(sp+4), r.r(sp+8)), 0), 12)
    elif address == 0x253b1a:
        strings[reverse[machine.reg_read(UC_X86_REG_EDI)]] = r.r(sp+4)
        returned(pop=4)  # String loading records the original caller's choice.
    elif address == 0x1249d0:
        returned(states[machine.reg_read(UC_X86_REG_EAX)][3])
    elif address == 0x2b10c3:
        # Original2B10A3 has already written +6E from the supplied appearance.
        # Skip only the subsequent appearance/color rendering backend.
        machine.reg_write(UC_X86_REG_EIP, 0x2b110c)
    elif address == 0x22e391:
        returned()  # Base animation does not select sign-in widget visibility.


r.g.hook_add(UC_HOOK_CODE, hook)
cases = []
for slot, inputs in itertools.product(range(4), itertools.product(
        (False, True), (False, True), (False, True), (0, 1, 2, 3), (0, 1))):
    states = [(False, False, False, 0, 0)]*4
    states[slot] = inputs
    strings.clear()
    for key, pointer in widgets.items():
        r.g.mem_write(pointer, bytes(0x100))
        r.g.mem_write(pointer+0x6e, b"\x7f")
        r.g.mem_write(pointer+0x88, struct.pack("<h", 9))
        if key[0] == 8:
            r.w(pointer+0x70, controller_definition)
    for index, (connected, signed, live, kind, species) in enumerate(states):
        r.w(0x54e8e0+index*0xc70, (int(signed)<<4)|(int(live)<<5))
    r.run(0x252554, registers=[(UC_X86_REG_ECX, screen)])
    visible = {(kind, index): r.g.mem_read(pointer+0x6e, 1)[0]
               for (kind, index), pointer in widgets.items()}
    assert all(value in (0, 1) for (kind, index), value in visible.items()
               if kind != 8 or index >= 3)
    cases.append(dict(slot=slot, inputs=[int(x) for x in inputs],
        texts=[visible[6, index] for index in range(2, 26)],
        models=[visible[7, index] for index in range(8)],
        bitmaps=[visible[8, index] for index in range(3, 11)],
        frames=[r.read(widgets[8, index]+0x88, "<h")[0] for index in range(7, 11)],
        strings={str(index-2): f"{value:08x}" for (kind,index),value in strings.items()}))
out = ROOT / "analysis/frontend/profile-regression/frontend-signin-original.json"
out.write_text(json.dumps(dict(xbe_sha256=PIN, screen=screen_tag["name"],
    routine="252554", input_fields=["connected", "signed", "live", "gamepad_type", "species"],
    controller_bitmap_image_count=cache.u32(bitmap_data+0x44), cases=cases,
    scope=__doc__), indent=2)+"\n")
rows = []
for c in cases:
    fields = [str(c["slot"]), *(str(x) for x in c["inputs"]),
              "".join(map(str,c["texts"])), "".join(map(str,c["models"])),
              "".join(map(str,c["bitmaps"])), ",".join(map(str,c["frames"])),
              ",".join(f"{k}:{v}" for k,v in sorted(c["strings"].items(), key=lambda x:int(x[0])))]
    rows.append(" ".join(fields))
tsv = out.with_suffix(".tsv")
tsv.write_text("\n".join(rows)+"\n")
print(f"{len(cases)} original cases; TSV SHA256 {hashlib.sha256(tsv.read_bytes()).hexdigest()}")
