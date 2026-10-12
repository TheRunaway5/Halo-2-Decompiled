#!/usr/bin/env python3
"""Execute original saved-profile row constructor/writer/emblem consumers.

Original2BACBC factories/linkage,24A09B→24A64D/24A691→22F042→2AFEAE→
2B12CA, row focus24AC80/24AD0B and2B11C9→1392F8 execute unchanged.
Text setup/copy, residency, profile-array source, lookup of literal focus tracks,
2D projection/device boundary and graphics submission are supplied explicitly.
"""
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

OUT = ROOT / "analysis/frontend/profile-row-appearance"
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
original = Retail()
g = original.g
xbe = Xbe(ROOT / "assets/disc/default.xbe")
loaded = {}


def tag(datum, kind):
    if datum in loaded:
        return loaded[datum]
    _, source = cache.tag(datum, kind)
    address = original.alloc(cache.data[source:source + (76 if kind == "bitm" else 64)])
    loaded[datum] = address
    original.w(original.table + (datum & 65535) * 16 + 8, address)
    blocks = ((68, 116),) if kind == "bitm" else ((28, 44), (36, 56), (44, 36), (52, 24))
    for field, stride in blocks:
        records = cache.block(source + field, stride)
        if records:
            original.w(address + field + 4, original.alloc(b"".join(
                cache.data[p:p + stride] for p in records)))
        if kind == "skin":
            for p in records:
                if field == 36:
                    tag(cache.u32(p + 28), "bitm")
                elif field == 44:
                    tag(cache.u32(p + 16), "bitm")
                elif field == 52:
                    tag(cache.u32(p + 8), "skin")
    return address


wigl_tag = next(t for t in cache.tags.values() if t["group"] == "wigl")
_, wigl_source = cache.tag(wigl_tag["datum"], "wigl")
wigl = original.alloc(cache.data[wigl_source:wigl_source + 452])
matg_tag = next(t for t in cache.tags.values() if t["group"] == "matg")
_, matg_source = cache.tag(matg_tag["datum"], "matg")
palette = cache.block(matg_source + 0x160, 12)
matg = original.alloc(bytes(0x168))
original.w(matg + 0x160, len(palette))
original.w(matg + 0x164, original.alloc(b"".join(cache.data[p:p + 12] for p in palette)))
original.w(0x4E034C, matg)
original.w(0x4E0350, 1)
context = original.alloc(bytes(0x200))
original.w(0x510C4C, context)
GET_TEXT, SET_TEXT, LIST_ARRAY = 0x3000100, 0x3000200, 0x3000300
text_vtable = original.alloc(bytes(0x60))
original.w(text_vtable + 0xC, 0x22E391)
original.w(text_vtable + 0x14, 0x22E930)
original.w(text_vtable + 0x18, 0x253AEE)
original.w(text_vtable + 0x3C, GET_TEXT)
buffer_vtable = original.alloc(bytes(0x20))
original.w(buffer_vtable + 4, SET_TEXT)
buffer = original.alloc(bytes(0x100))
original.w(buffer, buffer_vtable)
list_widget = original.alloc(bytes(0x100))
original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, list_widget), (UC_X86_REG_EAX, 1)])
list_vtable = original.alloc(bytes(0x80))
original.w(list_vtable + 0x58, LIST_ARRAY)
original.w(list_vtable + 0x5C, 0x24A64D)
original.w(list_vtable + 0x64, 0x24A691)
original.w(list_widget, list_vtable)
pool = original.alloc(bytes(0x48))
original.w(list_widget + 0x70, pool)
data = original.alloc(bytes(3 * 8))
original.w(pool + 0x44, data)
for i, profile in enumerate((-1, 7, 8)):
    original.w(data + i * 8 + 4, profile)
entries = original.alloc(bytes(0x248 * 2))
profiles = [("Saved Blue", bytes([10, 0, 0, 10, 0, 26, 9, 0]) + bytes(8)),
            ("Saved Tan", bytes([17, 0, 0, 10, 1, 3, 4, 1]) + bytes(8))]
for i, (name, appearance) in enumerate(profiles):
    entry = entries + i * 0x248
    original.w(entry, i + 7)
    g.mem_write(entry + 0xC, (name + "\0").encode("utf-16le"))
    g.mem_write(entry + 0x11C, appearance)
row_vtable = original.alloc(xbe.read(0x459F10, 0x60))
WATCH = (0x22E27B, 0x2B01EB, 0x2B1111, 0x2BAF38, 0x2BAF5D, 0x22EB18,
         0x2BACBC, 0x2AFDBC, 0x2BAFA4, 0x24A09B, 0x24A64D, 0x24A691,
         0x22F042, 0x22EE37, 0x2AFEAE, 0x2B12CA, 0x24AC80, 0x24AD0B,
         0x22E930, 0x22E391, 0x22E3CD, 0x22E3B4, 0x2B11C9, 0x1392F8,
         0x22EA81, 0x2356D8)
calls, text_sets, payloads, draws = {}, [], [], []
focus_tracks = []


def supplied(value=0, arguments=0):
    stack = g.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    g.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def wide(pointer):
    raw = bytearray()
    while (c := bytes(g.mem_read(pointer + len(raw), 2))) != b"\0\0":
        raw.extend(c)
        assert len(raw) < 1024
    return raw.decode("utf-16le")


def hook(_machine, address, _size, _context):
    stack = g.reg_read(UC_X86_REG_ESP)
    if address in WATCH:
        calls[address] = calls.get(address, 0) + 1
    if address == 0x1A47FD:
        supplied(original.alloc(bytes(original.r(stack + 4))), 4)
    elif address in (0x23625D, 0x12360):
        supplied()
    elif address == 0x16E5E0:
        supplied(1, 4)
    elif address == 0x148350:
        supplied(wigl)
    elif address == 0x2BAEB1:
        parent, index = original.read(stack + 4, "<2I")
        definition = g.reg_read(UC_X86_REG_EBX)
        pointer = original.alloc(base_text)
        original.w(pointer, text_vtable)
        g.mem_write(pointer + 0xA, struct.pack("<h", index))
        g.mem_write(pointer + 0x20, bytes(g.mem_read(definition + 28, 8)))
        g.mem_write(pointer + 0x28, bytes(g.mem_read(definition + 16, 12)))
        g.mem_write(pointer + 0x68, struct.pack("<h", original.read(definition + 4, "<h")[0] - 1))
        g.mem_write(pointer + 0x6A, bytes(g.mem_read(definition + 40, 2)))
        original.w(pointer + 0x70, original.read(definition + 6, "<H")[0])
        original.w(pointer + 0x10, parent)
        last = original.r(parent + 0x14)
        if last:
            while original.r(last + 0x18):
                last = original.r(last + 0x18)
            original.w(last + 0x18, pointer)
            original.w(pointer + 0x1C, last)
        else:
            original.w(parent + 0x14, pointer)
        supplied(pointer, 8)
    elif address == GET_TEXT:
        supplied(buffer)
    elif address == SET_TEXT:
        text_sets.append(wide(original.r(stack + 4)))
        supplied(arguments=4)
    elif address == LIST_ARRAY:
        assert g.reg_read(UC_X86_REG_ECX) == list_widget
        original.w(original.r(stack + 4), 2)
        supplied(entries, 4)
    elif address == 0x24AAE8:
        supplied(0)  # Explicit local name-without-file-suffix environment.
    elif address == 0x253B1A:
        assert original.r(stack + 4) == 0x12000195
        supplied(arguments=4)  # CreateNew outer localized text, not emblem.
    elif address == 0x22F042:
        payloads.append(bytes(g.mem_read(g.reg_read(UC_X86_REG_EAX), 0x74)).hex())
    elif address == 0x24BD3B:
        supplied(focus_tracks[original.r(stack + 4)], 4)
    elif address == 0x137550:
        datum = g.reg_read(UC_X86_REG_EAX)
        index = g.reg_read(UC_X86_REG_ECX) & 65535
        bitmap = original.r(original.table + (datum & 65535) * 16 + 8)
        supplied(original.r(bitmap + 0x48) + min(index, original.r(bitmap + 0x44) - 1) * 116)
    elif address == 0x23618E:
        supplied(arguments=8)  # Preserve centered authored coordinates; depth/viewport projection excluded.
    elif address == 0x1BBD0:
        supplied()  # Device/view reset excluded; controlled HUD appearance context stays resident.
    elif address == 0x22A664:
        draws.append(dict(appearance=bytes(g.mem_read(context + 0x1C0, 16)).hex(),
                          sequence=original.read(context + 0x1D0, "<h")[0],
                          player_index=original.read(context + 0x1B4, "<i")[0],
                          opacity_word=f"{original.r(0x4E696C):08x}",
                          bitmap=f"{original.r(stack + 4):08x}",
                          page=original.r(stack + 8),
                          shader=f"{original.r(stack + 12):08x}",
                          quad=original.read(g.reg_read(UC_X86_REG_ECX), "<8f"),
                          uv=original.read(g.reg_read(UC_X86_REG_EAX), "<4f")))
        supplied(arguments=12)


g.hook_add(UC_HOOK_CODE, hook)
text_template = original.alloc(bytes(0x100))
original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, text_template), (UC_X86_REG_EAX, 6)])
base_text = bytes(g.mem_read(text_template, 0x100))
point = original.alloc(bytes(4))
bounds = original.alloc(bytes(8))
all_cases, placements, orders = [], [], []
for screen_id, outer_datum in ((10, 0xE50202E2), (193, 0xE50C02EC)):
    skin = tag(outer_datum, "skin")
    _, skin_source = cache.tag(outer_datum, "skin")
    focus_tracks.clear()
    for p in cache.block(skin_source + 20, 16):
        raw = bytearray(cache.data[p:p + 16])
        keys = cache.block(p + 8, 20)
        struct.pack_into("<I", raw, 12, original.alloc(b"".join(cache.data[k:k + 20] for k in keys)))
        focus_tracks.append(original.alloc(raw))
    row = original.alloc(bytes(0x100))
    original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, row), (UC_X86_REG_EAX, 2)])
    original.w(row, row_vtable)
    original.w(row + 0x10, list_widget)
    # Nonfirst row branch so24AD0B applies the authored unfocus track.
    original.w(row + 0x1C, list_widget)
    original.run(0x2BACBC, argv=[row, skin], registers=[(UC_X86_REG_EDI, point)])

    def descend(parent):
        result = []
        child = original.r(parent + 0x14)
        while child:
            result.append(child)
            result.extend(descend(child))
            child = original.r(child + 0x18)
        return result

    nodes = descend(row)
    emblem = next(p for p in nodes if original.r(p + 4) == 9)
    original.run(0x2BAFA4, registers=[(UC_X86_REG_EAX, row), (UC_X86_REG_ECX, bounds)])
    row_bounds = original.read(bounds, "<4h")
    definition = original.r(emblem + 0x70)
    placement = dict(screen=screen_id, outer_skin=f"{outer_datum:08x}",
                     emblem_definition=bytes(g.mem_read(definition, 36)).hex(),
                     flags=original.r(definition),
                     animation_index=original.read(emblem + 0x68, "<h")[0],
                     layer=original.read(emblem + 0x6A, "<h")[0],
                     bounds=original.read(emblem + 0x20, "<4h"),
                     row_bounds=row_bounds, row_pitch=row_bounds[0] - row_bounds[2])
    assert placement["bounds"] == [26, 12, -6, 44]
    assert placement["flags"] == 1 and placement["row_pitch"] == 21
    placements.append(placement)
    # Literal equal-depth collector entries for each descendant; no qsort tie
    # order or broader scene/widget depth-order claim.
    collector = [(p, original.alloc(struct.pack("<Ifh2x", p, 0., original.read(p + 0x6A, "<h")[0])))
                 for p in nodes if original.r(p + 4) in (6, 8, 9)]
    for p, entry in collector:
        if original.r(p + 4) == 8:
            a = original.run(0x2356D8, argv=[next(e for n, e in collector if n == emblem), entry])
            b = original.run(0x2356D8, argv=[entry, next(e for n, e in collector if n == emblem)])
            assert a == 0xFFFFFFFF and b == 1
            orders.append(dict(screen=screen_id, emblem_before_bitmap=True,
                               bitmap_layer=original.read(p + 0x6A, "<h")[0]))
    for label, index, focused in (("create_initial", 0, False), ("blue_focused", 1, True),
                                  ("tan_unfocused", 2, False), ("tan_focused", 2, True),
                                  ("blue_unfocused", 1, False), ("create_after_tan", 0, True)):
        calls.clear(); text_sets.clear(); payloads.clear(); draws.clear()
        original.w(row + 0x70, 0x10000 | index)
        original.w(0x54D5B8, 1000)
        for p in [row, *nodes]:
            g.mem_write(p + 0x34, bytes(52))
            g.mem_write(p + 0x64, struct.pack("<f", .25))
        original.run(0x24A09B, argv=[row, 0], registers=[(UC_X86_REG_ECX, list_widget)])
        original.run(0x24AC80 if focused else 0x24AD0B, registers=[(UC_X86_REG_ECX, row)])
        original.w(0x54D5B8, 2000)
        original.run(0x22E391, registers=[(UC_X86_REG_ECX, row)])
        visible = bool(original.read(emblem + 0x6E, "<B")[0])
        admitted = bool(original.run(0x22E3B4, registers=[(UC_X86_REG_ECX, emblem)]))
        bank = bytes(g.mem_read(emblem + 0x74, 16)).hex()
        overrides = original.read(emblem + 0x84, "<2h")
        opacity_word = f"{original.r(emblem + 0x64):08x}"
        offset = original.read(emblem + 0x58, "<3f")
        assert visible == admitted == (index != 0)
        assert opacity_word == ("3f800000" if focused else "3e99999a")
        assert offset == ([17., 0., 0.] if focused else [0., 0., 0.])
        if admitted:
            original.run(0x2B11C9, argv=[0], registers=[(UC_X86_REG_ECX, emblem)])
            expected = profiles[index - 1][1].hex()
            assert bank == expected and overrides == [-1, -1]
            assert len(draws) == 1 and draws[0]["appearance"] == expected
            assert draws[0]["opacity_word"] == opacity_word
            assert draws[0]["bitmap"] == "e49c027c" and draws[0]["shader"] == "e4e302c3"
            assert draws[0]["page"] == 0 and draws[0]["sequence"] == -1
            assert draws[0]["uv"] == [0., 1., 0., 1.]
            assert calls.get(0x24A64D) == calls.get(0x24A691) == calls.get(0x2B12CA) == 1
        else:
            assert calls.get(0x2B12CA, 0) == 0
        assert calls.get(0x24A09B) == calls.get(0x22F042) == 1
        all_cases.append(dict(screen=screen_id, label=label, record_index=index,
                              saved=index != 0, focused=focused, visible=visible,
                              drawing_admitted=admitted, appearance=bank, overrides=overrides,
                              opacity_word=opacity_word, offset=offset,
                              payload_hex=payloads[0], draws=draws.copy(), text_sets=text_sets.copy(),
                              executed={f"{a:08x}": count for a, count in sorted(calls.items())}))

report = dict(scope=__doc__, xbe_sha256=PIN, mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
              placements=placements, cases=all_cases, ordering=orders,
              boundaries=["1A47FD zeroed allocation; text2BAEB1 setup/copy supplied; bitmap/emblem/grid constructors execute",
                          "LIST_ARRAY provides two explicitly controlled saved records;24A64D/24A691 execute;24AAE8 supplies false local suffix policy",
                          "24BD3B supplies relocated original skin focus tracks; actual focus recursion/update execute",
                          "Residency and137550 page-readiness supplied from actual descriptors",
                          "23618E viewport/depth projection and1BBD0 device/view reset supplied; actual22EA81 centered bounds+offset execute",
                          "22A664 graphics submission records actual appearance/context/opacity inputs; GPU and final framebuffer excluded"],
              complete_framebuffer_parity=False)
OUT.mkdir(parents=True, exist_ok=True)
path = OUT / "profile-row-emblems-original.json"
path.write_text(json.dumps(report, indent=2) + "\n")
rows = []
for p in placements:
    rows.append("placement " + " ".join(map(str, [p["screen"], p["flags"], p["animation_index"], p["layer"], *p["bounds"], p["row_pitch"]])))
for c in all_cases:
    rows.append("case " + " ".join(map(str, [c["screen"], c["label"], c["record_index"], int(c["visible"]), int(c["drawing_admitted"])])) +
                " " + c["opacity_word"] + " " + " ".join(map(str, c["offset"])) + " " + c["appearance"])
(OUT / "profile-row-emblems-original.tsv").write_text("\n".join(rows) + "\n")
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
corridors = ((0x24A09B, 0xB3), (0x24A64D, 0x42), (0x24A691, 0xD2),
             (0x2B1111, 0x59), (0x2BACBC, 0x1F5), (0x2AFEAE, 0x299),
             (0x2B12CA, 0x40), (0x24AC80, 0x8B), (0x24AD0B, 0xCD),
             (0x2B11C9, 0xF1), (0x1392F8, 0x53), (0x2356D8, 0x97))
(OUT / "profile-row-emblems-original.asm").write_text("\n\n".join(
    "\n".join(f"{i.address:08x}: {i.mnemonic} {i.op_str}"
              for i in decoder.disasm(xbe.read(a, size), a)) for a, size in corridors) + "\n")
print(f"{len(all_cases)} original row writer/admission/focus cases passed; {len(placements)} constructor placements")
print("fixture SHA256", hashlib.sha256(path.read_bytes()).hexdigest())
