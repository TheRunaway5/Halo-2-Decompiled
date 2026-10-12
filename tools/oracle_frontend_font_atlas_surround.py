#!/usr/bin/env python3
"""Freeze retail font-cache copying and the one-texel transparent-white surround.

Unchanged 1F5C0 -> 140960 -> 122610 consumes original conduit-12 glyph streams.
The pixel cache is declared ready and the texture-lock boundary supplies a
256x256 linear A4R4G4B4 surface. Execution stops after clear/copy, before upload.
No guest graphics calls or coordinate/color adjustments are supplied.
"""
import hashlib
import json
import struct

from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ESP

OUT = ROOT / "analysis/frontend/profile-regression"
font_path = ROOT / "assets/disc/maps/fonts/conduit-12"
font = font_path.read_bytes()
cases = []
rows = []
for codepoint in (0xE105, 0xE102, ord("C")):
    original = Retail()
    guest = original.g
    pool = original.alloc(bytes(0x48))
    record = original.alloc(bytes(0x38))
    original.w(pool + 0x44, record)
    original.w(0x54D574, pool)
    original.w(record + 0x10, 4)
    offset = (struct.unpack_from("<I", font, 0x400 + codepoint * 4)[0] + 0x4040) * 16
    header = font[offset:offset + 16]
    guest.mem_write(record + 0x1C, header)
    original.w(record + 0x34, 0xFFFFFFFF)
    width, height = struct.unpack_from("<2H", header, 4)
    size = struct.unpack_from("<H", header, 2)[0]
    encoded_at = struct.unpack_from("<I", header, 12)[0]
    encoded = font[encoded_at:encoded_at + size]
    pixel_pool = original.alloc(bytes(0x68))
    blocks = original.alloc(bytes(0x48))
    entries = original.alloc(bytes(24))
    original.w(pixel_pool + 0x64, blocks)
    original.w(blocks + 0x44, entries)
    original.w(0x54D580, pixel_pool)
    original.w(0x54D584, original.alloc(encoded))
    texture = original.alloc(bytes(0x74))
    guest.mem_write(texture + 4, struct.pack("<2H", 256, 256))
    original.w(0x4B62AC, texture)
    guest.mem_write(0x4B62A0, bytes(12))
    guest.mem_write(0x4B62A6, struct.pack("<2H", 8, 8))
    atlas = original.alloc(bytes(256 * 256 * 2))
    executed = {}

    def hook(_machine, address, _size, _context):
        if address in (0x1F5C0, 0x140960, 0x122610):
            executed[address] = executed.get(address, 0) + 1
        if address == 0x140870:  # Declared pixel cache readiness.
            stack = guest.reg_read(UC_X86_REG_ESP)
            original.ret(1)
            guest.reg_write(UC_X86_REG_ESP, stack + 8)
        elif address == 0x1D5E0:  # Declared texture lock/pitch.
            assert guest.reg_read(UC_X86_REG_EAX) == texture
            stack = guest.reg_read(UC_X86_REG_ESP)
            original.w(original.r(stack + 8), 256 * 2)
            original.ret(atlas)
            guest.reg_write(UC_X86_REG_ESP, stack + 12)

    guest.hook_add(UC_HOOK_CODE, hook)
    original.run(0x1F5C0, stop=0x1F9B5, argv=[0])
    assert executed == {0x1F5C0: 1, 0x140960: 1, 0x122610: 1}
    assert original.read(0x4B62B4, "<2H") == [8, 8]
    region = []
    for y in range(-1, height + 1):
        row = original.read(atlas + ((8 + y) * 256 + 7) * 2,
                            f"<{width + 2}H")
        region.append(row)
        for x, value in enumerate(row, -1):
            if x in (-1, width) or y in (-1, height):
                assert value == 0x0FFF
            rows.append(f"{codepoint:04x} {x} {y} {value:04x}")
    cases.append(dict(codepoint=f"{codepoint:04x}", width=width, height=height,
                      header_hex=header.hex(), compressed_sha256=hashlib.sha256(encoded).hexdigest(),
                      atlas_origin=[8, 8], surround_word="0fff",
                      executed={f"{function:08x}": count for function, count in executed.items()},
                      region_words=region))

OUT.mkdir(parents=True, exist_ok=True)
tsv = OUT / "frontend-font-atlas-surround-original.tsv"
tsv.write_text("\n".join(rows) + "\n")
report = dict(original_xbe_sha256=PIN, font_sha256=hashlib.sha256(font).hexdigest(),
              cases=cases, fixture_sha256=hashlib.sha256(tsv.read_bytes()).hexdigest(),
              scope=__doc__.splitlines())
(OUT / "frontend-font-atlas-surround-original.json").write_text(json.dumps(report, indent=2) + "\n")
print(f"{len(cases)} original glyph cache copies and {len(rows)} original atlas words passed")
print(f"fixture SHA-256 {report['fixture_sha256']}")
