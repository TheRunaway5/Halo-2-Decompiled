#!/usr/bin/env python3
"""Execute retail1496F6/1A0180/1A0350/1A02D0 on the actual subtitle range.

Only148350 supplies the relocated original wigl. The serialized English table
and unic range are relocated, with the language/table-loaded inputs explicit.
The original UTF8 converter and UTF16 copy also execute. No font/pixels here.
"""
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESI,
                              UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP)


def main():
    cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
    _, wigl = cache.tag(next(k for k, t in cache.tags.items()
                           if t["group"] == "wigl"), "wigl")
    _, matg = cache.tag(next(k for k, t in cache.tags.items()
                           if t["group"] == "matg"), "matg")
    string_datum = cache.u32(wigl + 0x144)
    _, unic = cache.tag(string_datum, "unic")
    count, size, refs, data = struct.unpack_from("<4I", cache.data, matg + 0x190)
    first, length = struct.unpack_from("<2H", cache.data, unic + 0x10)
    retail = Retail()
    globals_pointer = retail.alloc(cache.data[wigl:wigl + 0x1C0])
    references = retail.alloc(cache.data[refs:refs + count * 8])
    strings = retail.alloc(cache.data[data:data + size])
    table_globals = bytearray(0x188 + 0x1C)
    struct.pack_into("<4I", table_globals, 0x188,
                     references, strings, count, size)
    table_globals[0x188 + 0x18] = 1
    retail.w(0x4E034C, retail.alloc(table_globals))
    retail.w(0x47FF38, 0)  # English; table is loaded.
    retail.w(retail.table + (string_datum & 0xFFFF) * 16 + 8,
             retail.alloc(cache.data[unic:unic + 52]))
    buffer = retail.alloc(bytes(512))
    calls = []

    def hook(uc, address, _size, _context):
        if address == 0x148350:
            retail.ret(globals_pointer)
        elif address == 0x1A0180:
            calls.append({"tag": f"{uc.reg_read(UC_X86_REG_EAX):08x}",
                          "string_id": f"{uc.reg_read(UC_X86_REG_EDI):08x}"})

    retail.g.hook_add(UC_HOOK_CODE, hook)
    rows = []
    for key_type in range(-1, 24):
        calls.clear()
        retail.g.mem_write(buffer, bytes(512))
        retail.run(0x1496F6, argv=[buffer],
                   registers=[(UC_X86_REG_ESI, key_type & 0xFFFFFFFF)])
        raw = bytes(retail.g.mem_read(buffer, 512))
        value = raw.decode("utf-16le").split("\0", 1)[0]
        rows.append({"key_type": key_type, "calls": calls.copy(), "text": value})
    report = {
        "xbe_sha256": PIN,
        "mainmenu_sha256": hashlib.sha256(cache.data).hexdigest(),
        "scope": "Unchanged1496F6→1A0180→1A0350→1A02D0→140440/3207ED; "
                 "actual serialized English range, language0/tableloaded1; "
                 "148350 supplies relocated original wigl. No glyph/framebuffer claim.",
        "range": {"first": first, "count": length, "datum": f"{string_datum:08x}"},
        "rows": rows,
    }
    out = ROOT / "analysis/frontend/customization/subtitle-strings-original.json"
    out.write_text(json.dumps(report, indent=2) + "\n")
    assert rows[12]["key_type"] == 11
    assert rows[12]["calls"][0]["string_id"] == "180006d8"
    assert rows[12]["text"] == "\ue100 SELECT  \ue101 DONE"
    assert rows[6]["key_type"] == 5 and rows[6]["text"] == ""
    assert not rows[0]["calls"] and not rows[-1]["calls"]
    # Original subtitle setup and glyph traversal, with only font-cache
    # readiness/lookup and final submission supplied like the keyboard oracle.
    _, screen = cache.tag(next(k for k, t in cache.tags.items()
                              if t["group"] == "wgit" and
                              cache.u16(t["address"] - cache.magic + 4) == 47), "wgit")
    pane = cache.block(screen + 32, 76)[0]
    definition = retail.alloc(cache.data[screen:screen + 104])
    widget = retail.alloc(bytes(0x700))
    vtable = retail.alloc(bytes(0x100))
    text_buffer = retail.alloc(bytes(0x100))
    text_vtable = retail.alloc(bytes(0x40))
    GET_BUFFER, GET_STRING, CALLBACK = 0x3000100, 0x3000110, 0x3000200
    retail.w(widget + 0x338, vtable)
    retail.w(vtable + 0x3C, GET_BUFFER)
    retail.w(text_buffer, text_vtable)
    retail.w(text_vtable + 8, GET_STRING)
    font = (ROOT / "assets/disc/maps/fonts/conduit-12").read_bytes()
    font_pool = retail.alloc(bytes(0x48))
    retail.w(0x54D574, font_pool)
    retail.g.mem_write(0x4E2920, font[0x200:0x3C0])
    retail.g.mem_write(0x4E2920 + 0x1C8, b"\1")
    retail.w(0x4E28F4 + 4, 0)
    characters, glyphs = [], []

    def glyph_hook(uc, address, _size, _context):
        if address == 0x22F871:
            retail.ret(definition)
        elif address == 0x2300CF:
            retail.ret(cache.u16(pane + 2))
        elif address in (0x22EB18, 0x253BC9):
            retail.ret(0)  # String binding was executed independently above.
        elif address == GET_BUFFER:
            retail.ret(text_buffer)
        elif address == GET_STRING:
            retail.ret(buffer)
        elif address == 0x8CC20:
            uc.mem_write(uc.reg_read(UC_X86_REG_ESI), b"\0\0")
            retail.ret(0)
        elif address == 0x140B20:
            c = uc.reg_read(UC_X86_REG_ECX)
            retail.ret(characters.index(c) if c in characters else 0xFFFFFFFF)
        elif address == 0x140870:
            sp = uc.reg_read(UC_X86_REG_ESP)
            retail.ret(1)
            uc.reg_write(UC_X86_REG_ESP, sp + 8)
        elif address == CALLBACK:
            sp = uc.reg_read(UC_X86_REG_ESP)
            v = struct.unpack("<5I7f2I", bytes(uc.mem_read(sp + 4, 56)))
            glyphs.append(dict(codepoint=v[2], color=f"{v[3]:08x}",
                               shadow=f"{v[4]:08x}", x=v[5], y=v[6],
                               width=v[9], height=v[10]))
            retail.ret(0)
            uc.reg_write(UC_X86_REG_ESP, sp + 60)

    retail.g.hook_add(UC_HOOK_CODE, glyph_hook)
    retail.run(0x22FF53, stop=0x2538A6, argv=[widget])
    sp = retail.g.reg_read(UC_X86_REG_ESP)
    setup = dict(color=retail.read(retail.g.reg_read(UC_X86_REG_EDX), "<4f"),
                 bounds=retail.read(retail.g.reg_read(UC_X86_REG_EAX), "<4h"),
                 font=retail.r(sp + 12))
    assert setup["font"] == 1
    value = rows[12]["text"]
    characters[:] = list(dict.fromkeys(map(ord, value)))
    records = retail.alloc(bytes(0x38 * len(characters)))
    retail.w(font_pool + 0x44, records)
    for i, cp in enumerate(characters):
        offset = (struct.unpack_from("<I", font, 0x400 + cp * 4)[0] + 0x4040) * 16
        retail.w(records + i * 0x38 + 0x10, 4)
        retail.g.mem_write(records + i * 0x38 + 0x1C, font[offset:offset + 16])
    retail.g.mem_write(0x4E73A0, bytes(0x70))
    retail.w(0x4E73A0, 1)
    retail.w(0x4E73A0 + 4, 1)
    retail.w(0x4E73A0 + 12, 1)  # Right alignment, as22FF53 requested.
    retail.run(0x13EC70, registers=[(UC_X86_REG_EAX,
                  retail.alloc(struct.pack("<4f", *setup["color"])) )])
    retail.run(0x13ED50, registers=[(UC_X86_REG_EAX,
                  retail.alloc(struct.pack("<4f", 1, 0, 0, 0)))])
    top, left, bottom, right = setup["bounds"]
    box = retail.alloc(struct.pack("<4h", 240-top, 320+left, 240-bottom, 320+right))
    endpoint = retail.alloc(bytes(4))
    text = retail.alloc(struct.pack(f"<{len(value)+1}I", *map(ord, value), 0))
    retail.run(0x13F0E0, argv=[CALLBACK, box, endpoint, 0, 0, 0x3F800000, text])
    report["appearance_footer"] = dict(setup=setup, text=value, glyphs=glyphs,
                         font_sha256=hashlib.sha256(font).hexdigest(),
                         glyph_scope="Unchanged22FF53 setup; string-binding/linkage supplied. "
                         "Unchanged13EC70/13ED50/13F0E0/13EF40/13F700/122570; "
                         "actual font records, ready cache lookup/final callback supplied.")
    out.write_text(json.dumps(report, indent=2) + "\n")
    print(f"Executed {len(rows)} original selector/localization cases; {out}")
    print("Appearance footer:", [(g["codepoint"],g["x"],g["y"]) for g in glyphs])


if __name__ == "__main__":
    main()
