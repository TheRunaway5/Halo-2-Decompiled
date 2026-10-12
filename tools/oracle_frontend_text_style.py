#!/usr/bin/env python3
"""Freeze original text color, private-glyph and clipped shadow draw behavior."""
import hashlib
import json
import random
import struct
from pathlib import Path
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP, UC_X86_REG_ESI, UC_X86_REG_EDI

OUT = ROOT / "analysis/frontend/profile-regression"
original = Retail()
guest = original.g
rows = []
colors = [(1, 1, 1, 1), (.5, .8, .9, 1), (1, .68, .9, 1),
          (1, .2, .7, 1), (1, 1.2, 1.3, 1.4), (-.2, -.1, .5, 1.3)]
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
_, wigl = cache.tag(next(k for k, t in cache.tags.items() if t["group"] == "wigl"), "wigl")
_, keyboard = cache.tag(next(k for k, t in cache.tags.items()
                            if t["group"] == "wgit" and cache.u16(t["address"]-cache.magic+4) == 167), "wgit")
key = cache.block(cache.block(keyboard+32, 76)[0]+4, 60)[0]
authored_colors = {
    "keyboard": struct.unpack_from("<4f", cache.data, key+12),
    "focused": struct.unpack_from("<4f", cache.data, wigl+0x168)}
colors += list(authored_colors.values())
rng = random.Random(0x13EC70)
colors += [tuple(rng.uniform(-.2, 1.4) for _ in range(4)) for _ in range(1024)]
for index, color in enumerate(colors):
    supplied = struct.pack("<4f", *color)
    pointer = original.alloc(supplied)
    original.run(0x13EC70, registers=[(UC_X86_REG_EAX, pointer)])
    result = bytes(guest.mem_read(0x4E73B0, 16))
    rows.append(f"C color-{index} {supplied.hex()} {result.hex()}")
for codepoint in list(range(0xDFFF, 0xE401)) + [0, 32, 65, 0xFFFFFFFF, 0x10FFFF]:
    result = original.run(0x13F660, argv=[codepoint]) & 255
    rows.append(f"G {codepoint:08x} {result}")

# Actual conduit-12 header and glyph records in a supplied ready cache.
font = (ROOT / "assets/disc/maps/fonts/conduit-12").read_bytes()
guest.mem_write(0x4E2920, font[0x200:0x3C0])
guest.mem_write(0x4E2920 + 0x1C8, b"\1")
original.w(0x4E28F4 + 4, 0)
chars = [ord("1"), ord("a"), 0xE100, 0xE101]
pool = original.alloc(bytes(0x48))
records = original.alloc(bytes(0x38 * len(chars)))
original.w(pool + 0x44, records)
original.w(0x54D574, pool)
for index, codepoint in enumerate(chars):
    offset = (struct.unpack_from("<I", font, 0x400 + codepoint * 4)[0] + 0x4040) * 16
    original.w(records + index * 0x38 + 0x10, 4)
    guest.mem_write(records + index * 0x38 + 0x1C, font[offset:offset + 16])
    original.w(records + index * 0x38 + 0x34, 0)
guest.mem_write(0x4B62B0, struct.pack("<I2h", 0, 8, 8))
draws = []
setup_definition = original.alloc(bytes(60))
setup_globals = original.alloc(bytes(0x180))
setup_vtable = original.alloc(bytes(0x100))
setup_buffer_vtable = original.alloc(bytes(0x40))
setup_widget = original.alloc(bytes(0x200))
setup_buffer = original.alloc(bytes(0x70))
setup_string = original.alloc(bytes(4))
GET_BUFFER, SET_STRING, GET_STRING = 0x3000100, 0x3000110, 0x3000120
original.w(setup_vtable+0x3c, GET_BUFFER)
original.w(setup_buffer_vtable+4, SET_STRING)
original.w(setup_buffer_vtable+8, GET_STRING)
guest.mem_write(setup_globals+0x4c, struct.pack("<f", .6))


def hook(machine, address, size, context):
    if address == 0x140B20:
        codepoint = machine.reg_read(UC_X86_REG_ECX)
        original.ret(chars.index(codepoint) if codepoint in chars else 0xFFFFFFFF)
    elif address == 0x140870:
        stack = machine.reg_read(UC_X86_REG_ESP)
        original.ret(1)
        machine.reg_write(UC_X86_REG_ESP, stack + 8)
    elif address == 0x363A0:
        stack = machine.reg_read(UC_X86_REG_ESP)
        pointer = original.r(stack + 4)
        draws.append(bytes(machine.mem_read(pointer, 80)).hex())
        original.ret(0)
        machine.reg_write(UC_X86_REG_ESP, stack + 8)
    elif address == 0x22EEEE:
        original.ret(setup_globals)  # Declared widget screen parent.
    elif address == 0x253CC8:
        original.ret(setup_definition)  # Declared button definition lookup.
    elif address == 0x230134 or address == 0x22F8C7:
        original.ret(0)  # Empty localized text / no first-pane index binding.
    elif address == 0x148350:
        original.ret(setup_globals)
    elif address == GET_BUFFER:
        original.ret(setup_buffer)
    elif address == SET_STRING:
        stack = machine.reg_read(UC_X86_REG_ESP)
        original.ret(0)
        machine.reg_write(UC_X86_REG_ESP, stack+8)
    elif address == GET_STRING:
        original.ret(setup_string)
    elif address == 0x13EE20:
        stack = machine.reg_read(UC_X86_REG_ESP)
        original.ret(1)  # Font prediction is independent of setup flags.
        machine.reg_write(UC_X86_REG_ESP, stack+8)


guest.hook_add(UC_HOOK_CODE, hook)
guest.mem_write(0x4E73A0, bytes(0x70))
original.w(0x4E73A0, 1)
draw_cases = []
for codepoint in chars:
    text = original.alloc(struct.pack("<2I", codepoint, 0))
    for scale in (1., .6, 1.5):
        for shadow in (0, 0xFF000000, 0x80000000):
            bounds = original.alloc(struct.pack("<4f", 12., 30., 22., 44.))
            position = original.alloc(struct.pack("<2f", 10., 35.))
            draws.clear()
            original.run(0x13F700, argv=[0x1F490, position, 0xFF406080,
                                        shadow, text, 1,
                                        struct.unpack("<I", struct.pack("<f", scale))[0]],
                         registers=[(UC_X86_REG_EAX, bounds),
                                    (UC_X86_REG_ECX, 0), (UC_X86_REG_EDX, 0)])
            draw_cases.append(dict(codepoint=f"{codepoint:04x}", scale=scale,
                                   shadow=f"{shadow:08x}", quads=list(draws)))

# Execute the original complete draw corridor to freeze 13F470 packing and
# 13F700's texture-color override, after original 13EC70 normalization.
packed_cases = 0
packed_authored = {}
for color_index, color in enumerate(colors[:8] + colors[8:136]):
    if color_index in (6, 7):
        # UI setup copies authored RGB; 22CD48 supplies animation alpha.
        color = (1., *color[1:])
    supplied = struct.pack("<4f", *color)
    original.run(0x13EC70, registers=[(UC_X86_REG_EAX, original.alloc(supplied))])
    for enabled in (False, True):
        original.run(0x13ED50, registers=[(UC_X86_REG_EAX, original.alloc(struct.pack("<4f", 1., 0., 0., 0.)) if enabled else 0)])
        for codepoint in chars:
            bounds = original.alloc(struct.pack("<4h", 0, 0, 100, 200))
            position = original.alloc(bytes(4))
            text = original.alloc(struct.pack("<2I", codepoint, 0))
            draws.clear()
            original.run(0x13F0E0, argv=[0x1F490, bounds, position, 0, 0, 0x3F800000, text])
            fg = int.from_bytes(bytes.fromhex(draws[-1])[16:20], "little")
            shadow = int.from_bytes(bytes.fromhex(draws[0])[16:20], "little") if len(draws) == 2 else 0
            rows.append(f"P packed-{color_index}-{int(enabled)}-{codepoint:x} {supplied.hex()} {codepoint:08x} {int(enabled)} {fg:08x} {shadow:08x}")
            packed_cases += 1
            if color_index in (6, 7) and not enabled and codepoint == ord("1"):
                packed_authored["keyboard" if color_index == 6 else "focused"] = f"{fg:08x}"

# Original field ink/endpoint measurement against real conduit-13 records.
font13 = (ROOT / "assets/disc/maps/fonts/conduit-13").read_bytes()
guest.mem_write(0x4E2920, font13[0x200:0x3C0])
chars[:] = list(dict.fromkeys(map(ord, "Halo0001")))
records = original.alloc(bytes(0x38 * len(chars)))
original.w(pool + 0x44, records)
for index, codepoint in enumerate(chars):
    offset = (struct.unpack_from("<I", font13, 0x400 + codepoint * 4)[0] + 0x4040) * 16
    original.w(records + index * 0x38 + 0x10, 4)
    guest.mem_write(records + index * 0x38 + 0x1C, font13[offset:offset+16])
    original.w(records + index * 0x38 + 0x34, 0)
bounds = original.alloc(struct.pack("<4h", 167, 72, 207, 560))
endpoint = original.alloc(bytes(4))
text = original.alloc(struct.pack("<9I", *map(ord, "Halo0001"), 0))
draws.clear()
original.run(0x13F0E0, argv=[0x1F490, bounds, endpoint, 0, 0, 0x3F800000, text])
field_quads = list(draws)
field_endpoint = original.read(endpoint, "<2h")
ink, line = original.alloc(bytes(8)), original.alloc(bytes(8))
original.run(0x13EA60, argv=[bounds, text, 0x3F800000],
             registers=[(UC_X86_REG_EDI, ink), (UC_X86_REG_ESI, line)])
field_measurement = dict(text="Halo0001", font="conduit-13",
                         font_sha256=hashlib.sha256(font13).hexdigest(),
                         bounds=original.read(bounds, "<4h"),
                         ink=original.read(ink, "<4h"), line=original.read(line, "<4h"),
                         endpoint=field_endpoint, quads=field_quads)
original.run(0x13E9C0, argv=[bounds, ink, line, 0x3F800000],
             registers=[(UC_X86_REG_ECX, original.alloc("Halo0001\0".encode("utf-16le")))])
assert original.read(ink, "<4h") == field_measurement["ink"]
assert original.read(line, "<4h") == field_measurement["line"]
field_measurement["original_utf16_measurement_matches"] = True
setup_cases = []
for button in (False, True):
    for flags in range(16):
        guest.mem_write(setup_widget, bytes(0x200))
        guest.mem_write(setup_buffer, bytes(0x70))
        original.w(setup_widget, setup_vtable)
        buffer = setup_widget+0x70 if button else setup_buffer
        original.w(buffer, setup_buffer_vtable)
        guest.mem_write(buffer+0x20, struct.pack("<f", 1.))
        guest.mem_write(setup_definition, struct.pack("<I", flags)+bytes(56))
        guest.mem_write(setup_definition+10, struct.pack("<h", 4))
        if button:
            original.run(0x253CFF, argv=[setup_widget])
        else:
            original.run(0x253765, argv=[setup_widget, 0, setup_definition])
        buffer_flags = original.read(buffer+0x14, "<H")[0]
        shadow = not (buffer_flags & 4)
        rows.append(f"B {int(button)} {flags} {int(shadow)}")
        setup_cases.append(dict(button=button, definition_flags=flags,
                                buffer_flags=buffer_flags, font=original.r(buffer+4),
                                scale=original.read(buffer+0x20, "<f")[0]))
fixture = OUT / "frontend-text-style-original.tsv"
fixture.write_text("\n".join(rows) + "\n")
report = dict(xbe_sha256=PIN,
              font_sha256=hashlib.sha256(font).hexdigest(),
              fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
              colors=len(colors), glyph_classification=1031,
              packed_cases=packed_cases, packed_authored=packed_authored,
              authored_colors=authored_colors, field_measurement=field_measurement,
              setup_cases=setup_cases,
              clipped_draw_cases=draw_cases,
              scope="Unchanged13EC70/13F660,13F0E0/13F700→1F490→1F3A0 final vertices,13E9C0/13EA60 conduit13 measurement,and253765/253CFF→22CD0A setup flags. Real font headers; ready-cache lookup/pixel readiness,empty localized strings,widget parent/definition lookup,font prediction supplied. Graphics363A0 records vertices instead of submitting GPU commands; framebuffer parity excluded.")
(OUT / "frontend-text-style-original.json").write_text(json.dumps(report, indent=2) + "\n")
print(json.dumps({key: value for key, value in report.items()
                  if key not in ("clipped_draw_cases", "field_measurement", "setup_cases")}, indent=2))
