#!/usr/bin/env python3
"""Execute the retail Appearance row binding, bounds union, and title casing.

Widget lookup and localized-string setters are supplied seams. Original
2C8BF5 and its species/color/emblem string lookup functions execute unchanged.
2BAFA4 consumes synthetic child objects containing the actual authored child
bounds (including the source 2BACBC bitmap bottom-anchor placement). No GPU or
animation timing is included.
"""
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
                              UC_X86_REG_EDX, UC_X86_REG_EDI,
                              UC_X86_REG_ESP)

OUT = ROOT / "analysis/frontend/profile-regression"
retail = Retail()
guest = retail.g
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
row = retail.alloc(bytes(0x100))
text_nodes = [retail.alloc(bytes(0x100)) for _ in range(2)]
setters = []
executed = {}
group_bindings = []
group_mode = False
tracked = (0x2C8BF5, 0x2365E0, 0x14986F, 0x149B5A, 0x149DA5,
           0x2CB200, 0x2CB220, 0x2BAFA4, 0x2B0A48, 0x22E9C6, 0x13FAE0,
           0x2C8D7C)


def supplied_return(value=0, arguments=0):
    stack = guest.reg_read(UC_X86_REG_ESP)
    retail.ret(value)
    guest.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    if address in tracked:
        executed[address] = executed.get(address, 0) + 1
    if address == 0x22EDB8:
        stack = guest.reg_read(UC_X86_REG_ESP)
        kind, index, recursive = retail.read(stack + 4, "<3I")
        assert kind == (7 if group_mode else 6) and index in (0, 1) and not recursive
        assert guest.reg_read(UC_X86_REG_ECX) == row
        supplied_return(text_nodes[index], 12)
    elif address == 0x253B1A:
        stack = guest.reg_read(UC_X86_REG_ESP)
        setters.append(dict(child=text_nodes.index(guest.reg_read(UC_X86_REG_EDI)),
                            string_id=f"{retail.r(stack + 4):08x}"))
        supplied_return(arguments=4)
    elif address == 0x22E391 and group_mode:
        supplied_return()
    elif address == 0x22F042 and group_mode:
        stack = guest.reg_read(UC_X86_REG_ESP)
        widget, count = retail.read(stack + 4, "<2I")
        item = guest.reg_read(UC_X86_REG_EAX)
        assert widget == row and count == 1
        group_bindings.append(dict(flags=retail.r(item), name_pointer=f"{retail.r(item+4):08x}",
                                   appearance=list(guest.mem_read(item + 0x48, 16))))
        supplied_return(arguments=8)
    elif address == 0x2B10A3 and group_mode:
        supplied_return(arguments=4)


guest.hook_add(UC_HOOK_CODE, hook)
bindings = []
for values in ((10, 0, 0, 10, 0, 0, 0, 0), (8, 12, 9, 1, 1, 26, 9, 1),
               (17, 16, 15, 14, 0, 63, 31, 0)):
    guest.mem_write(0x54E6F0, bytes(values))
    for index in range(7):
        setters.clear()
        guest.mem_write(row + 0x70, struct.pack("<h", index))
        retail.run(0x2C8BF5, argv=[row, index])
        assert [s["child"] for s in setters] == [0, 1]
        bindings.append(dict(appearance=list(values), row=index,
                             assignments=list(setters)))

group_mode = True
guest.mem_write(0x54E5E0, "Appearance UI\0".encode("utf-16le"))
retail.run(0x2C8D7C, registers=[(UC_X86_REG_ECX, row)])
group_mode = False

masks = []
for mode in (0, 1):
    bits = retail.alloc(bytes(8))
    retail.run(0x2CB200, argv=[mode], registers=[(UC_X86_REG_EDX, bits)])
    masks.append(dict(mode=mode, words=retail.read(bits, "<2I")))

wigl = next(t for t in cache.tags.values() if t["group"] == "wigl")
_, globals_source = cache.tag(wigl["datum"], "wigl")
skins = cache.block(globals_source + 0x138, 8)
layout = []
for screen_id in (47, 49, 50, 52, 233):
    screen = next(t for t in cache.tags.values() if t["group"] == "wgit" and
                  cache.u16(t["address"] - cache.magic + 4) == screen_id)
    p = screen["address"] - cache.magic
    pane = cache.block(p + 32, 76)[0]
    definition = cache.block(pane + 12, 24)[0]
    skin_datum = cache.u32(skins[cache.u16(definition + 4)] + 4)
    tag, skin_source = cache.tag(skin_datum, "skin")
    group = retail.alloc(bytes(0x100))
    children = []
    for source in cache.block(skin_source + 28, 44):
        child = retail.alloc(bytes(0x100))
        retail.w(child + 4, 6)
        guest.mem_write(child + 0x20, cache.data[source + 28:source + 36])
        children.append(child)
    for source in cache.block(skin_source + 36, 56):
        child = retail.alloc(bytes(0x100))
        retail.w(child + 4, 8)
        copied = retail.alloc(cache.data[source:source + 56])
        retail.w(child + 0x70, copied)
        _, bitmap_source = cache.tag(cache.u32(source + 28), "bitm")
        image = cache.block(bitmap_source + 68, 116)[0]
        width, height = struct.unpack_from("<2h", cache.data, image + 4)
        x, y = struct.unpack_from("<2h", cache.data, source + 12)
        # Literal 2BACBC adds the bitmap's height to its constructor rectangle.
        guest.mem_write(child + 0x20, struct.pack("<4h", y + height, x, y, x + width))
        children.append(child)
    retail.w(group + 0x14, children[0])
    for a, b in zip(children, children[1:]):
        retail.w(a + 0x18, b)
    bounds = retail.alloc(bytes(8))
    retail.run(0x2BAFA4, registers=[(UC_X86_REG_EAX, group),
                                  (UC_X86_REG_ECX, bounds)])
    rectangle = retail.read(bounds, "<4h")
    layout.append(dict(screen=screen_id, skin=tag["name"],
                       list_position=list(struct.unpack_from("<2h", cache.data, definition + 8)),
                       visible=cache.u16(definition + 6), bounds=rectangle,
                       row_height=rectangle[0] - rectangle[2]))

titles = []
for text in ("Appearance", "Player Profiles", "éó àß ", "\ue100 select"):
    encoded = text.encode("utf-16le") + b"\0\0"
    buffer = retail.alloc(encoded)
    retail.run(0x13FAE0, registers=[(UC_X86_REG_EAX, buffer),
                                  (UC_X86_REG_ECX, 1024)])
    upper = bytes(guest.mem_read(buffer, len(encoded) - 2)).decode("utf-16le")
    titles.append(dict(input=text, result=upper))

result = dict(xbe_sha256=PIN,
              mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
              executed={f"{k:08x}": v for k, v in executed.items()},
              supplied_boundaries=["22EDB8 text child lookup", "253B1A localized string assignment",
                                   "actual authored child rectangles after 2BACBC placement"],
              appearance_rows=bindings, emblem_available_masks=masks,
              profile_group_assignments=group_bindings,
              list_layout=layout, title_uppercase=titles,
              excluded=["localized glyph rendering", "GPU", "animation and scrolling phase",
                        "appearance persistence", "model materials", "emblem composition"])
path = OUT / "appearance-ui-original.json"
path.write_text(json.dumps(result, indent=2) + "\n")
print(f"{len(bindings)} row bindings; masks {masks}; layout {[(v['screen'],v['row_height']) for v in layout]}")
print(f"sha256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
