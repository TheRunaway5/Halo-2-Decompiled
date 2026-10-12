#!/usr/bin/env python3
"""Freeze original 52040 single-texture UI combiner construction.

Enter the unchanged pixel-program construction corridor after device/texture
binding; stop before device submission. The supplied descriptor has exactly
one texture and no effect/extra textures/tint inputs, as original 23675A.
"""
import hashlib
import json
import struct
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from audit_rendering import Cache
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EBP, UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESP

r = Retail()
descriptor = bytearray(0x98)
struct.pack_into("<I", descriptor, 0x0c, r.alloc(bytes(0x74)))
for offset in (0x28, 0x2c, 0x40, 0x44):
    struct.pack_into("<f", descriptor, offset, 1.)
p = r.alloc(descriptor)
r.run(0x52857, stop=0x52c62, registers=[(UC_X86_REG_EBP, p)])
program = bytes(r.g.mem_read(0x484f68, 240))
words = struct.unpack("<60I", program)
assert words[10] == 0xffffffff and words[18] == 0xffffffff
assert words[0:2] == (0x18111912, 0x1a111814)
assert words[26:28] == (0x89, 0xac)
assert words[34:36] == (0x08010902, 0x0a010804)
assert words[45:47] == (0x89, 0xac)
assert words[8:10] == (0xc, 0x1c00)
assert words[53:55] == (0x11102, 1)
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
tag, source = cache.tag(next(k for k, t in cache.tags.items()
                            if t["group"] == "bitm" and t["name"] == "rasterizer\\default_2d"), "bitm")
bitmaps = cache.block(source+0x44, 0x74)
actual = []


def hook(machine, address, size, context):
    if address == 0x12360:
        r.ret(1)  # Declared ready rasterizer, independent of descriptor content.
    elif address == 0x52040:
        stack = machine.reg_read(UC_X86_REG_ESP)
        actual.append(dict(descriptor=bytes(machine.mem_read(r.r(stack+4), 0x98)),
                           vertices=bytes(machine.mem_read(r.r(stack+8), 80))))
        r.ret(0)
        machine.reg_write(UC_X86_REG_ESP, stack+12)


r.g.hook_add(UC_HOOK_CODE, hook)
producers = []
for page, clamp, color, box in [(1, 1, 0xffffffff, (72.,137.,167.,189.)),
                               (0, 0, 0x80ffffff, (137.,138.,167.,187.))]:
    # Actual237C04 callers: 235E5E arg3 is UV clamp, arg4 is blend.
    # Both selection and caret pass blend0; selection alone passes clamp1.
    mode = 0
    bitmap = r.alloc(cache.data[bitmaps[page]:bitmaps[page]+0x74])
    width, height = struct.unpack_from("<2h", cache.data, bitmaps[page]+4)
    source_box = (0., float(width), 0., float(height))
    source_bounds = r.alloc(struct.pack("<4f", *source_box))
    bounds = r.alloc(struct.pack("<4f", *box))
    r.run(0x235e5e, argv=[bounds, color, clamp, mode],
          registers=[(UC_X86_REG_EAX, source_bounds), (UC_X86_REG_ECX, bitmap)])
    produced = actual[-1]
    supplied = bytearray(descriptor)
    struct.pack_into("<I", supplied, 0x0c, bitmap)
    struct.pack_into("<H", supplied, 0x94, mode)
    assert produced["descriptor"] == supplied
    for i in range(4):
        assert struct.unpack_from("<I", produced["vertices"], i*20+16)[0] == color
    r.run(0x52857, stop=0x52c62, registers=[(UC_X86_REG_EBP, r.alloc(produced["descriptor"]))])
    assert bytes(r.g.mem_read(0x484f68, 240)) == program
    producers.append(dict(page=page, clamp_uv=clamp, mode=mode, color=f"{color:08x}", box=box,
                          source_box=source_box,
                          bitmap_sha256=hashlib.sha256(cache.data[bitmaps[page]:bitmaps[page]+0x74]).hexdigest(),
                          descriptor_hex=produced["descriptor"].hex(),
                          vertices_hex=produced["vertices"].hex()))
out = ROOT / "analysis/frontend/profile-regression/frontend-bitmap-combiner-original.json"
out.write_text(json.dumps(dict(
    xbe_sha256=PIN, descriptor_hex=descriptor.hex(),
    program_sha256=hashlib.sha256(program).hexdigest(),
    bitmap_tag=tag["name"], original_235e5e_23675a_producers=producers,
    pixel_program=[f"{x:08x}" for x in words],
    scope="Original235E5E wrapper,23675A producer and52040[52857,52C62),131E50 execute unchanged. Actualdefault2d page0/1 imageheaders and full source rectangles; actual237C04 selection/caret clamp and blend arguments; supplied destination bounds/color; rasterizer readiness=true,device/texture binding andGPU submission excluded."), indent=2)+"\n")
print(out)
