#!/usr/bin/env python3
"""Freeze keyboard label glyphs and the original subtitle color caller.

The unchanged 22FF53 caller is stopped at 2538A6 after choosing its font,
bounds and color. Font-cache readiness and final glyph submission are supplied;
13F0E0/13EF40/13F700/122570 execute unchanged on the retail font records.
"""
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
                               UC_X86_REG_ECX, UC_X86_REG_EDX,
                               UC_X86_REG_EDI, UC_X86_REG_ESI,
                               UC_X86_REG_ESP)

OUT = ROOT / "analysis/frontend/profile-regression"
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
_, wigl = cache.tag(next(k for k, t in cache.tags.items()
                        if t["group"] == "wigl"), "wigl")
_, matg = cache.tag(next(k for k, t in cache.tags.items()
                        if t["group"] == "matg"), "matg")
_, screen = cache.tag(next(k for k, t in cache.tags.items()
                          if t["group"] == "wgit" and
                          cache.u16(t["address"] - cache.magic + 4) == 167), "wgit")
pane = cache.block(screen + 32, 76)[0]
keys = cache.block(pane + 4, 60)
_, strings = cache.tag(cache.u32(screen + 28), "unic")
count, size, indices, data = struct.unpack_from("<4I", cache.data, matg + 0x190)
start, length = struct.unpack_from("<2H", cache.data, strings + 16)
localized = {sid: cache.string(data + offset, size - offset)
             for index in range(start, start + length)
             for sid, offset in [struct.unpack_from("<2I", cache.data,
                                                    indices + index * 8)]}
font = (ROOT / "assets/disc/maps/fonts/conduit-12").read_bytes()

original = Retail()
guest = original.g
globals_pointer = original.alloc(cache.data[wigl:wigl + 0x1C0])
definition = original.alloc(cache.data[screen:screen + 104])
widget = original.alloc(bytes(0x700))
vtable = original.alloc(bytes(0x100))
buffer = original.alloc(bytes(0x100))
buffer_vtable = original.alloc(bytes(0x40))
GET_BUFFER, GET_STRING = 0x3000100, 0x3000110
original.w(widget + 0x338, vtable)
original.w(vtable + 0x3C, GET_BUFFER)
original.w(buffer, buffer_vtable)
original.w(buffer_vtable + 8, GET_STRING)
string_pointer = original.alloc("\0".encode("utf-16le"))
font_pool = original.alloc(bytes(0x48))
original.w(0x54D574, font_pool)
guest.mem_write(0x4E2920, font[0x200:0x3C0])
guest.mem_write(0x4E2920 + 0x1C8, b"\1")
original.w(0x4E28F4 + 4, 0)
characters = []
draws = []
kerning_inputs = []
CALLBACK = 0x3000200


def hook(machine, address, _size, _context):
    if address == 0x148350:
        original.ret(globals_pointer)
    elif address == 0x22F871:
        original.ret(definition)
    elif address == 0x2300CF:
        original.ret(cache.u16(pane + 2))
    elif address in (0x22EB18, 0x253BC9):
        original.ret(0)  # Child linkage / localized subtitle binding.
    elif address == GET_BUFFER:
        original.ret(buffer)
    elif address == GET_STRING:
        original.ret(string_pointer)
    elif address == 0x8CC20:
        # The empty copied text is independent of the selected subtitle color.
        machine.mem_write(machine.reg_read(UC_X86_REG_ESI), b"\0\0")
        original.ret(0)
    elif address == 0x140B20:
        c = machine.reg_read(UC_X86_REG_ECX)
        original.ret(characters.index(c) if c in characters else 0xFFFFFFFF)
    elif address == 0x140870:
        stack = machine.reg_read(UC_X86_REG_ESP)
        original.ret(1)
        machine.reg_write(UC_X86_REG_ESP, stack + 8)
    elif address == 0x122570:
        # Actual register inputs, not the decompiled parameter names.
        kerning_inputs.append(dict(previous=machine.reg_read(UC_X86_REG_EDI),
                                   current=machine.reg_read(UC_X86_REG_EBX),
                                   header=f"{machine.reg_read(UC_X86_REG_EDX):08x}"))
    elif address == CALLBACK:
        stack = machine.reg_read(UC_X86_REG_ESP)
        v = struct.unpack("<5I7f2I", bytes(machine.mem_read(stack + 4, 56)))
        draws.append(dict(codepoint=v[2], color=f"{v[3]:08x}",
                          shadow=f"{v[4]:08x}", x=v[5], y=v[6],
                          source_x=v[7], source_y=v[8], width=v[9],
                          height=v[10], scale=v[11]))
        original.ret(0)
        machine.reg_write(UC_X86_REG_ESP, stack + 60)


guest.hook_add(UC_HOOK_CODE, hook)
original.run(0x22FF53, stop=0x2538A6, argv=[widget])
stack = guest.reg_read(UC_X86_REG_ESP)
subtitle = dict(color=original.read(guest.reg_read(UC_X86_REG_EDX), "<4f"),
                bounds=original.read(guest.reg_read(UC_X86_REG_EAX), "<4h"),
                font=original.r(stack + 12), flags=guest.reg_read(UC_X86_REG_ECX) & 65535,
                screen_flags=cache.u32(screen), definition_color_offset=8)
assert subtitle["color"] == list(struct.unpack_from("<4f", cache.data, screen + 8))
assert subtitle["font"] == 1 and subtitle["flags"] == 2


def projected(bounds):
    top, left, bottom, right = bounds
    return [240 - top, 320 + left, 240 - bottom, 320 + right]


def draw_case(index, value, bounds, color, justification):
    characters[:] = list(dict.fromkeys(map(ord, value)))
    records = original.alloc(bytes(0x38 * len(characters)))
    original.w(font_pool + 0x44, records)
    for i, codepoint in enumerate(characters):
        offset = (struct.unpack_from("<I", font, 0x400 + codepoint * 4)[0] + 0x4040) * 16
        original.w(records + i * 0x38 + 0x10, 4)
        guest.mem_write(records + i * 0x38 + 0x1C, font[offset:offset + 16])
    guest.mem_write(0x4E73A0, bytes(0x70))
    original.w(0x4E73A0, 1)
    original.w(0x4E73A0 + 4, 1)
    original.w(0x4E73A0 + 12, justification)
    original.run(0x13EC70, registers=[(UC_X86_REG_EAX,
                                      original.alloc(struct.pack("<4f", *color)))])
    original.run(0x13ED50, registers=[(UC_X86_REG_EAX,
                                      original.alloc(struct.pack("<4f", 1, 0, 0, 0)))])
    box = original.alloc(struct.pack("<4h", *bounds))
    endpoint = original.alloc(bytes(4))
    text = original.alloc(struct.pack(f"<{len(value)+1}I", *map(ord, value), 0))
    draws.clear()
    kerning_inputs.clear()
    original.run(0x13F0E0, argv=[CALLBACK, box, endpoint, 0, 0, 0x3F800000, text])
    return dict(index=index, text=value, bounds=bounds, color=color,
                justification=justification, endpoint=original.read(endpoint, "<2h"),
                glyphs=list(draws), kerning_inputs=list(kerning_inputs))


cases = []
for index in [36, 37, 38, 39, 40, 41, 42, 45, 46]:
    p = keys[index]
    value = localized[cache.u32(p + 48)]
    color = list(struct.unpack_from("<4f", cache.data, p + 12))
    color[0] = 1
    if index == 40:  # Captured default focus is DONE.
        color[1:] = struct.unpack_from("<3f", cache.data, wigl + 0x16C)
    cases.append(draw_case(index, value, projected(struct.unpack_from("<4h", cache.data, p + 28)),
                           color, 2))
# The live subtitle text is retained as UTF-16 in the immutable widget snapshot.
runtime_path = OUT / "original-keyboard-runtime-assets.json"
runtime = json.loads(runtime_path.read_text())
live_footer = next(w for w in runtime["widgets"] if w["address"] == "82545a08")
raw = bytes.fromhex(live_footer["text_buffer_at_74_hex"])
footer = raw[0x44:].decode("utf-16le").split("\0")[0]
assert footer == "\ue100 ENTER  \ue101 CANCEL"
cases.append(draw_case(-1, footer, projected(subtitle["bounds"]), subtitle["color"], 1))
caps = next(c for c in cases if c["index"] == 42)
assert [g["x"] for g in caps["glyphs"]] == [468, 474, 482, 489, 496, 500, 507, 515, 522]
assert [k["previous"] for k in caps["kerning_inputs"][:9]] == [0, 0, 65, 80, 83, 32, 76, 79, 67]
assert all(g["color"] == "ffadadad" for g in cases[-1]["glyphs"] if g["codepoint"] < 0xE000)
rows = []
for case in cases:
    for g in case["glyphs"]:
        rows.append(" ".join(map(str, [case["index"], g["codepoint"], g["x"], g["y"],
                                       g["width"], g["height"], g["color"], g["shadow"]])))
tsv = OUT / "keyboard-labels-original.tsv"
tsv.write_text("\n".join(rows) + "\n")
report = dict(xbe_sha256=PIN, mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
              font_sha256=hashlib.sha256(font).hexdigest(), subtitle_setup=subtitle,
              runtime_snapshot_sha256=hashlib.sha256(runtime_path.read_bytes()).hexdigest(),
              cases=cases, fixture_sha256=hashlib.sha256(tsv.read_bytes()).hexdigest(),
              scope="Unchanged22FF53 until2538A6 chooses subtitle color/font/bounds; unrelated child linkage/string binding supplied. Unchanged13EC70/13ED50/13F0E0/13EF40/13F700/122570 uses actual retail conduit12 glyphs. Ready font-cache lookup/pixel readiness and final glyph callback supplied. No projection/framebuffer parity claim; ordinary single-line keyboard labels only.")
(OUT / "keyboard-labels-original.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps(dict(subtitle=subtitle, fixture_sha256=report["fixture_sha256"],
                      glyphs=len(rows), caps=caps), indent=2))
