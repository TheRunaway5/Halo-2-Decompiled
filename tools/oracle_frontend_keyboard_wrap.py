#!/usr/bin/env python3
"""Execute original text wrapping for the retail profile-name description.

Only ready font-cache lookup/pixel readiness and the final glyph callback are
supplied. The original iterator, Unicode break predicates, measurement, and
draw traversal execute unchanged with the actual conduit-12 glyph records.
"""
import hashlib
import json
import struct
from pathlib import Path

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ECX, UC_X86_REG_ESP

original = Retail()
guest = original.g
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
_, matg = cache.tag(next(k for k, t in cache.tags.items()
                        if t["group"] == "matg"), "matg")
_, screen = cache.tag(next(k for k, t in cache.tags.items()
                          if t["group"] == "wgit" and
                          cache.u16(t["address"] - cache.magic + 4) == 167), "wgit")
_, strings = cache.tag(cache.u32(screen + 28), "unic")
count, size, indices, data = struct.unpack_from("<4I", cache.data, matg + 0x190)
start, length = struct.unpack_from("<2H", cache.data, strings + 16)
description = next(cache.string(data + offset, size - offset)
                   for index in range(start, start + length)
                   for sid, offset in [struct.unpack_from("<2I", cache.data,
                                                         indices + index * 8)]
                   if sid & 65535 == 0x44A)
text_definition = cache.block(cache.block(screen + 32, 76)[0] + 28, 44)[1]
authored = list(struct.unpack_from("<4h", cache.data, text_definition + 28))
assert authored == [149, -254, 89, 263]
assert cache.u16(text_definition + 10) == 1
bounds = [240 - authored[0], 320 + authored[1],
          240 - authored[2], 320 + authored[3]]
font = (ROOT / "assets/disc/maps/fonts/conduit-12").read_bytes()
guest.mem_write(0x4E2920, font[0x200:0x3C0])
guest.mem_write(0x4E2920 + 0x1C8, b"\1")
original.w(0x4E28F4 + 4, 0)
chars = list(dict.fromkeys(map(ord, description)))
pool = original.alloc(bytes(0x48))
records = original.alloc(bytes(0x38 * len(chars)))
original.w(pool + 0x44, records)
original.w(0x54D574, pool)
for index, codepoint in enumerate(chars):
    offset = (struct.unpack_from("<I", font, 0x400 + codepoint * 4)[0] + 0x4040) * 16
    original.w(records + index * 0x38 + 0x10, 4)
    guest.mem_write(records + index * 0x38 + 0x1C, font[offset:offset + 16])
guest.mem_write(0x4E73A0, bytes(0x70))
original.w(0x4E73A0, 1)
original.w(0x4E73A0 + 4, 1)
callback = 0x3000200
draws = []


def hook(machine, address, _size, _context):
    if address == 0x140B20:
        codepoint = machine.reg_read(UC_X86_REG_ECX)
        original.ret(chars.index(codepoint) if codepoint in chars else 0xFFFFFFFF)
    elif address == 0x140870:
        stack = machine.reg_read(UC_X86_REG_ESP)
        original.ret(1)
        machine.reg_write(UC_X86_REG_ESP, stack + 8)
    elif address == callback:
        stack = machine.reg_read(UC_X86_REG_ESP)
        args = bytes(machine.mem_read(stack + 4, 56))
        values = struct.unpack("<5I7f2I", args)
        draws.append(dict(character=chr(values[2]), x=values[5], y=values[6],
                          width=values[9], height=values[10]))
        original.ret(0)
        machine.reg_write(UC_X86_REG_ESP, stack + 60)


guest.hook_add(UC_HOOK_CODE, hook)
text = original.alloc(struct.pack(f"<{len(description)+1}I", *map(ord, description), 0))
cases = []
for width in [516, 517, 518, 525, 534]:
    box = original.alloc(struct.pack("<4h", bounds[0], bounds[1],
                                     bounds[2], bounds[1] + width))
    endpoint = original.alloc(bytes(4))
    draws.clear()
    original.run(0x13F0E0, argv=[callback, box, endpoint, 0, 0, 0x3F800000, text])
    lines = []
    for glyph in draws:
        if not lines or lines[-1]["y"] != glyph["y"]:
            lines.append(dict(y=glyph["y"], text=""))
        lines[-1]["text"] += glyph["character"]
    cases.append(dict(width=width, bounds=original.read(box, "<4h"),
                      lines=lines, endpoint=original.read(endpoint, "<2h"),
                      glyphs=list(draws)))
assert cases[1]["lines"][0]["text"].rstrip().endswith("be up")
assert cases[2]["lines"][0]["text"].rstrip().endswith("up to")
report = dict(xbe_sha256=PIN, mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
              font_sha256=hashlib.sha256(font).hexdigest(), description=description,
              definition_bounds=authored, projected_bounds=bounds, cases=cases,
              scope="Unchanged13F0E0/13EF40/13F470/13F5A0/13FD20/13F700 with actual retail conduit12 glyphs and localized profile-description. Ready font-cache lookup/pixel readiness supplied; glyph callback records output. No projection or framebuffer parity claim.")
output = ROOT / "analysis/frontend/profile-regression/keyboard-wrap-original.json"
output.write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({key: value for key, value in report.items() if key != "cases"}, indent=2))
print(json.dumps([dict(width=case["width"], lines=case["lines"])
                  for case in cases], indent=2))
