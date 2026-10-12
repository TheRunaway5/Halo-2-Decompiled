#!/usr/bin/env python3
"""Execute original flags3 appearance-namebar binding and bitmap ARGB packing.

Actual mainmenu skin/bitmap definitions and matg palette are copied with host
pointer relocation. A ready synthetic group contains its authored bitmap child.
2AFEAE -> 7F790 -> 2B0A14 runs unchanged; the subsequent original bitmap packing
kernel 2B05C8..2B0635 consumes its actual written RGB and supplied widget alpha.
This excludes constructors, animation clocks, texture sampling and framebuffer.
"""
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_EBX,
                              UC_X86_REG_EDI, UC_X86_REG_ESI,
                              UC_X86_REG_XMM1)

r = Retail()
g = r.g
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
skin_tag = next(t for t in cache.tags.values() if t["group"] == "skin" and
                t["name"] == "ui\\player_skins\\player_skin_profile_appearance_edit")
_, skin_source = cache.tag(skin_tag["datum"], "skin")
skin = r.alloc(cache.data[skin_source:skin_source + 64])
bitmap_records = cache.block(skin_source + 36, 56)
assert len(bitmap_records) == 1
definition = r.alloc(cache.data[bitmap_records[0]:bitmap_records[0] + 56])
assert cache.u32(bitmap_records[0] + 44) == 0x1400069A
r.w(skin + 40, definition)
r.w(r.table + (skin_tag["datum"] & 65535) * 16 + 8, skin)
bitmap_datum = cache.u32(bitmap_records[0] + 28)
_, bitmap_source = cache.tag(bitmap_datum, "bitm")
bitmap_header = r.alloc(cache.data[bitmap_source:bitmap_source + 76])
r.w(r.table + (bitmap_datum & 65535) * 16 + 8, bitmap_header)
matg = next(t for t in cache.tags.values() if t["group"] == "matg")
_, matg_source = cache.tag(matg["datum"], "matg")
palette_records = cache.block(matg_source + 0x160, 12)
assert len(palette_records) == 18
palette_bytes = b"".join(cache.data[p:p + 12] for p in palette_records)
matg_guest = r.alloc(cache.data[matg_source:matg_source + 0x168])
palette = r.alloc(palette_bytes)
r.w(matg_guest + 0x164, palette)
r.w(0x4e034c, matg_guest)
r.w(0x4e0350, 1)
group = r.alloc(bytes(0x100))
widget = r.alloc(bytes(0x100))
grid = r.alloc(bytes(24))
item = r.alloc(bytes(0x70))
r.w(grid + 8, skin_tag["datum"])
r.w(group + 0x74, grid)
r.w(group + 0x14, widget)
r.w(widget + 4, 8)
r.w(widget + 0x70, definition)
g.mem_write(widget + 0x0a, struct.pack("<h", 0))
r.w(item, 3)
frame = r.alloc(bytes(0x200)) + 0x100
cases = []
order_nodes = []
for kind in (8, 6):
    node = r.alloc(bytes(0x100))
    r.w(node + 4, kind)
    order_nodes.append(r.alloc(struct.pack("<Ifh2x", node, 0., 10)))
bitmap_before_text = r.run(0x2356D8, argv=order_nodes)
assert bitmap_before_text == 0xffffffff
for index in (*range(18), -1, 18):
    g.mem_write(item + 0x48, bytes([index & 255, 0, 0, 10, 0, 0, 0, 0]))
    g.mem_write(widget + 0x6e, b"\0")
    g.mem_write(widget + 0x88, struct.pack("<h", -1))
    r.run(0x2AFEAE, argv=[group], registers=[(UC_X86_REG_EBX, item)])
    rgb = bytes(g.mem_read(widget + 0x28, 12))
    expected = palette_bytes[index * 12:index * 12 + 12] if 0 <= index < 18 else struct.pack("<3f", 1, 1, 1)
    assert rgb == expected
    assert bytes(g.mem_read(widget + 0x6e, 1)) == b"\x01"
    assert r.read(widget + 0x88, "<h")[0] == 0
    for alpha in (1., .5, .3):
        alpha_bits = struct.unpack("<I", struct.pack("<f", alpha))[0]
        r.run(0x2B05C8, stop=0x2B0635,
              registers=[(UC_X86_REG_EBP, frame), (UC_X86_REG_EDI, widget),
                         (UC_X86_REG_EBX, 0), (UC_X86_REG_XMM1, alpha_bits)])
        cases.append(dict(index=index, alpha=alpha, rgb_hex=rgb.hex(),
                          rgb=r.read(widget + 0x28, "<3f"),
                          argb=f"{g.reg_read(UC_X86_REG_ESI):08x}",
                          sequence=0, visible=True))
report = dict(xbe_sha256=PIN, map_sha256=hashlib.sha256(cache.data).hexdigest(),
              skin_datum=f"{skin_tag['datum']:08x}",
              bitmap_datum=f"{bitmap_datum:08x}", placeholder="1400069a",
              original_entries=["002afeae", "0007f790", "002b01da",
                                "002b0a14", "002b05c8..002b0635"],
              fpcw="027f", mxcsr="1f80", cases=cases,
              equal_depth_layer10_bitmap_vs_text=-1,
              supplied_boundaries=["ready group/bitmap child linkage and authored definition index",
                                   "flags3 appearance item", "widget alpha1/.5/.3"],
              excluded=["constructors", "animation clocks", "GPU/framebuffer"])
path = ROOT / "analysis/frontend/profile-regression/appearance-namebar-original.json"
path.write_text(json.dumps(report, indent=2) + "\n")
print(f"{len(cases)} original namebar RGB/sequence/visibility/ARGB cases; "
      f"SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
for case in cases:
    print(case)
