#!/usr/bin/env python3
"""Execute the original193 header caller, group consumers and placement.

Original bitmap/emblem/grid constructors, linkage, placement,24A886→22F042→
2AFEAE→2B12CA and animation start/update run unchanged on relocated authored
metadata. Text allocation/setup and buffer copy, profile-pool getters, bitmap
residency and unrelated model services are supplied. No native/GPU execution.
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


def copy_tag(datum, kind):
    _, source = cache.tag(datum, kind)
    if kind == "bitm":
        header = bytearray(cache.data[source:source + 76])
        records = cache.block(source + 68, 116)
        original.w(original.table + (datum & 65535) * 16 + 8,
                   address := original.alloc(header))
        original.w(address + 72, original.alloc(b"".join(
            cache.data[p:p + 116] for p in records)))
        assert cache.u32(source + 60) == 0  # Header icons/dummy have no sprites.
    elif kind == "skin":
        header = bytearray(cache.data[source:source + 64])
        address = original.alloc(header)
        original.w(original.table + (datum & 65535) * 16 + 8, address)
        for field, stride in ((28, 44), (36, 56), (44, 36), (52, 24)):
            records = cache.block(source + field, stride)
            if records:
                original.w(address + field + 4, original.alloc(b"".join(
                    cache.data[p:p + stride] for p in records)))
            if field == 36:
                for p in records:
                    copy_tag(cache.u32(p + 28), "bitm")
            elif field == 44:
                for p in records:
                    copy_tag(cache.u32(p + 16), "bitm")
        assert cache.u32(source + 52) == 0
    else:
        raise AssertionError(kind)
    return address


header = copy_tag(0xE5EF03CF, "skin")
wigl_tag = next(t for t in cache.tags.values() if t["group"] == "wigl")
_, wigl_source = cache.tag(wigl_tag["datum"], "wigl")
wigl = original.alloc(cache.data[wigl_source:wigl_source + 452])
animations = bytearray()
for p in cache.block(wigl_source + 0x120, 44):
    row = bytearray(cache.data[p:p + 44])
    for field in (8, 20, 36):
        keys = cache.block(p + field, 20)
        if keys:
            struct.pack_into("<I", row, field + 4, original.alloc(b"".join(
                cache.data[q:q + 20] for q in keys)))
    animations.extend(row)
original.w(wigl + 0x124, original.alloc(animations))
matg_tag = next(t for t in cache.tags.values() if t["group"] == "matg")
_, matg_source = cache.tag(matg_tag["datum"], "matg")
palette = cache.block(matg_source + 0x160, 12)
matg = original.alloc(bytes(0x168))
original.w(matg + 0x160, len(palette))
original.w(matg + 0x164, original.alloc(b"".join(
    cache.data[p:p + 12] for p in palette)))
original.w(0x4E034C, matg)
original.w(0x4E0350, 1)

GET_TEXT, SET_TEXT, LIST_ARRAY = 0x3000100, 0x3000200, 0x3000300
text_vtable = original.alloc(bytes(0x60))
original.w(text_vtable + 0xC, 0x22E391)
original.w(text_vtable + 0x18, 0x253AEE)
original.w(text_vtable + 0x3C, GET_TEXT)
buffer_vtable = original.alloc(bytes(0x20))
original.w(buffer_vtable + 4, SET_TEXT)
buffer = original.alloc(bytes(0x100))
original.w(buffer, buffer_vtable)
calls = {}
text_sets = []
payloads = []
allocations = []
WATCH = (0x22E27B, 0x2B01EB, 0x2B1111, 0x2BAF5D, 0x22EB18,
         0x2BACBC, 0x2AFDBC, 0x2BAFA4, 0x24A886, 0x24A855, 0x24A828,
         0x24A763, 0x22F042, 0x22EE37, 0x22EE47, 0x2AFEAE, 0x2B12CA,
         0x22E957, 0x22E65E, 0x22E89C, 0x22E391, 0x22E3CD, 0x22E3B4, 0x2B0328)


def supplied_return(value=0, arguments=0):
    stack = g.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    g.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def wide(pointer):
    result = bytearray()
    while (c := bytes(g.mem_read(pointer + len(result), 2))) != b"\0\0":
        result.extend(c)
        assert len(result) < 1024
    return result.decode("utf-16le")


def hook(_machine, address, _size, _context):
    stack = g.reg_read(UC_X86_REG_ESP)
    if address in WATCH:
        calls[address] = calls.get(address, 0) + 1
    if address == 0x1A47FD:
        size = original.r(stack + 4)
        pointer = original.alloc(bytes(size))
        allocations.append((pointer, size))
        supplied_return(pointer, 4)
    elif address in (0x23625D, 0x12360):
        supplied_return()  # Asset residency, not layout/animation.
    elif address == 0x16E5E0:
        supplied_return(1, 4)  # Type9 shader-residency walk, not UI placement.
    elif address == 0x148350:
        supplied_return(wigl)
    elif address == 0x2BAEB1:
        # Text allocation/setup is a supplied boundary. Its original base
        # constructor state is copied, then authored fields are installed.
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
        supplied_return(pointer, 8)
    elif address == GET_TEXT:
        supplied_return(buffer)
    elif address == SET_TEXT:
        text_sets.append(wide(original.r(stack + 4)))
        supplied_return(arguments=4)
    elif address == 0x24C447:
        assert g.reg_read(UC_X86_REG_EAX) == list_widget
        supplied_return(0x10000)
    elif address == 0x16BAE0:
        assert g.reg_read(UC_X86_REG_ESI) == pool
        supplied_return(datum)
    elif address == LIST_ARRAY:
        assert g.reg_read(UC_X86_REG_ECX) == list_widget
        original.w(original.r(stack + 4), 2)
        supplied_return(entries, 4)
    elif address == 0x22F042:
        item = g.reg_read(UC_X86_REG_EAX)
        payloads.append(bytes(g.mem_read(item, 0x74)).hex())
        # Continue through the literal group/visibility/appearance consumer.
    elif address in (0x2B102F, 0x2B0F76):
        supplied_return(arguments=4 if address == 0x2B102F else 12)
    elif address == 0x137550:
        # Type9 update's bitmap readiness resolves the actual authored page.
        image_datum = g.reg_read(UC_X86_REG_EAX)
        image_index = g.reg_read(UC_X86_REG_ECX) & 65535
        tag = original.r(original.table + (image_datum & 65535) * 16 + 8)
        count = original.r(tag + 0x44)
        supplied_return(original.r(tag + 0x48) + min(image_index, count - 1) * 116)


g.hook_add(UC_HOOK_CODE, hook)
original.w(0x54D5B8, 1000)
text_template = original.alloc(bytes(0x100))
original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, text_template), (UC_X86_REG_EAX, 6)])
base_text = bytes(g.mem_read(text_template, 0x100))
screen = original.alloc(bytes(0x1A00))
original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, screen), (UC_X86_REG_EAX, 0)])
original.w(screen + 0x70, 193)
list_widget = screen + 0x614
list_vtable = original.alloc(bytes(0x60))
original.w(list_widget, list_vtable)
original.w(list_vtable + 0x58, LIST_ARRAY)
pool = original.alloc(bytes(0x40))
original.w(list_widget + 0x70, pool)
datum = original.alloc(bytes(8))
entries = original.alloc(bytes(0x248 * 2))
profiles = [("Saved Blue", bytes([10, 0, 0, 10, 0, 26, 9, 0, 0]) + bytes(7)),
            ("Saved Tan", bytes([17, 0, 0, 10, 1, 3, 4, 1, 0]) + bytes(7))]
for index, (name, appearance) in enumerate(profiles):
    entry = entries + index * 0x248
    original.w(entry, 7 + index)
    g.mem_write(entry + 0xC, (name + "\0").encode("utf-16le"))
    g.mem_write(entry + 0x11C, appearance)
screen_tag = next(t for t in cache.tags.values() if t["group"] == "wgit" and
                  cache.u16(cache.tag(t["datum"], "wgit")[1] + 4) == 193)
_, screen_source = cache.tag(screen_tag["datum"], "wgit")
pane = cache.block(screen_source + 32, 76)[0]
# Original22F6CA sets the containing screen's animation from pane+2 minus1.
# A synthetic NONE screen would restart all child animations prematurely.
g.mem_write(screen + 0x68, struct.pack("<h", cache.u16(pane + 2) - 1))
grid_source = cache.block(pane + 68, 24)[0]
grid = original.alloc(cache.data[grid_source:grid_source + 24])
origin = original.alloc(bytes(4))
group = original.run(0x2BAF5D, argv=[screen, origin, 0, grid])
children = []
child = original.r(group + 0x14)
while child:
    children.append(child)
    child = original.r(child + 0x18)
assert [original.r(p + 4) for p in children] == [8, 9, 6]


def snapshot():
    return [dict(kind=original.r(p + 4), visible=bool(original.read(p + 0x6E, "<B")[0]),
                 drawing_admitted=bool(original.run(0x22E3B4, registers=[(UC_X86_REG_ECX, p)])),
                 bounds=original.read(p + 0x20, "<4h"),
                 sequence=original.read(p + 0x88, "<h")[0] if original.r(p + 4) == 8 else None,
                 appearance=bytes(g.mem_read(p + 0x74, 16)).hex() if original.r(p + 4) == 9 else None,
                 emblem_overrides=original.read(p + 0x84, "<2h") if original.r(p + 4) == 9 else None,
                 animation_index=original.read(p + 0x68, "<h")[0],
                 animation_delay=original.r(p + 0x44),
                 animation_end_time=original.r(p + 0x50),
                 animation_type=original.read(p + 0x34, "<i")[0],
                 animation_scale=original.read(p + 0x64, "<f")[0],
                 animation_offset=original.read(p + 0x58, "<3f"),
                 animation_words=bytes(g.mem_read(p + 0x34, 52)).hex()) for p in children]


placement = snapshot()
assert placement[0]["bounds"] == [163, -269, 35, -141]
assert placement[0]["sequence"] == 0  # Authored2 is not < actual image count1.
assert placement[1]["bounds"] == [163, -269, 35, -141]
assert placement[2]["bounds"] == [35, -269, 15, -141]
original.run(0x22E957, argv=[0], registers=[(UC_X86_REG_ECX, screen)])
intro_started = snapshot()
assert [n["animation_delay"] for n in intro_started] == [300, 250, 250], intro_started
intro = []
for elapsed in (0, 249, 250, 299, 300, 400, 470, 480, 510, 1000, 5000):
    original.w(0x54D5B8, 1000 + elapsed)
    try:
        original.run(0x22E391, registers=[(UC_X86_REG_ECX, screen)])
    except Exception:
        print("animation failure", elapsed, {name: hex(g.reg_read(reg)) for name, reg in
              (("eip", UC_X86_REG_EIP), ("eax", UC_X86_REG_EAX),
               ("ecx", UC_X86_REG_ECX), ("edx", UC_X86_REG_EDX),
               ("esi", UC_X86_REG_ESI), ("esp", UC_X86_REG_ESP))})
        raise
    intro.append(dict(elapsed_ms=elapsed, children=snapshot()))
assert all(n["animation_scale"] == 1 and n["animation_offset"] == [0, 0, 0]
           for n in intro[-1]["children"])
animation_before_update = [bytes(g.mem_read(p + 0x34, 52)) for p in children]
setup_executed = {f"{a:08x}": count for a, count in sorted(calls.items())}
cases = []
for label, focus in (("create_new_initial", -1), ("blue", 7), ("tan", 8),
                     ("create_new_after_tan", -1), ("blue_again", 7)):
    original.w(datum + 4, focus)
    text_sets.clear()
    payloads.clear()
    calls.clear()
    original.run(0x24A886, registers=[(UC_X86_REG_ECX, screen)])
    state = snapshot()
    assert [n["visible"] for n in state] == [focus == -1, focus != -1, focus != -1]
    assert state[0]["sequence"] == 0
    assert all(bytes(g.mem_read(p + 0x34, 52)) == before
               for p, before in zip(children, animation_before_update))
    if focus != -1:
        name, appearance = profiles[focus - 7]
        assert text_sets == [name]
        assert state[1]["appearance"] == appearance.hex()
        assert state[1]["emblem_overrides"] == [-1, -1]
    else:
        assert not text_sets
    assert len(payloads) == 1
    assert calls.get(0x22F042) == calls.get(0x2AFEAE) == 1
    assert calls.get(0x2B12CA, 0) == int(focus != -1)
    cases.append(dict(label=label, focus=focus, payload_hex=payloads[0],
                      children=state, text_sets=text_sets.copy(),
                      executed={f"{a:08x}": count for a, count in sorted(calls.items())}))

fixture = dict(scope=__doc__, original_xbe_sha256=PIN,
               mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
               screen=193, header_skin="e5ef03cf", icon_bitmap="e5f003d0",
               authored_icon_sequence=2, actual_icon_image_count=1,
               placement=placement, intro_started=intro_started, intro=intro, cases=cases,
               setup_executed=setup_executed,
               boundaries=["1A47FD allocation supplies zeroed memory; original bitmap/emblem/grid constructors and22EB18 linkage execute",
                           "2BAEB1 text allocation/setup and text-buffer getter/copy supplied",
                           "148350 supplies copied/relocated original wigl; original22E65E/22E957/22E89C/22E3CD execute",
                           "23625D/12360 bitmap and16E5E0 shader residency;137550 type9 bitmap readiness supplies actual descriptor",
                           "24C447/16BAE0/list array supply controlled saved profiles; original24A855/24A828/24A763 execute",
                           "Unrelated2B102F/2B0F76 model appearance services supplied when applicable"],
               clock_scope="Controlled original start at1000ms; widget-specific authored delay/intro executes. Actual channel-transition chronology and GPU drawing are excluded.",
               complete_framebuffer_parity=False)
OUT.mkdir(parents=True, exist_ok=True)
path = OUT / "profile-header-group-original.json"
path.write_text(json.dumps(fixture, indent=2) + "\n")
rows = []
for child in placement:
    rows.append("placement " + " ".join(map(str, [child["kind"], *child["bounds"],
                child["sequence"] if child["sequence"] is not None else -1,
                child["animation_index"]])))
for case in cases:
    rows.append("case " + " ".join(map(str, [case["label"], case["focus"],
                *[int(child["visible"]) for child in case["children"]],
                case["children"][0]["sequence"]])))
    if case["focus"] != -1:
        rows.append("appearance " + case["label"] + " " + case["children"][1]["appearance"])
for tick in intro:
    for child in tick["children"]:
        words = struct.unpack("<4I", struct.pack("<4f", child["animation_scale"],
                                                *child["animation_offset"]))
        rows.append("animation " + " ".join(map(str, [tick["elapsed_ms"], child["kind"],
                    int(child["drawing_admitted"])])) + " " +
                    " ".join(f"{word:08x}" for word in words))
(OUT / "profile-header-group-original.tsv").write_text("\n".join(rows) + "\n")
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
corridors = ((0x2B01EB, 0x13D), (0x2B1111, 0x59), (0x2BACBC, 0x1F5),
             (0x22F042, 0x4E), (0x2AFEAE, 0x299), (0x2B12CA, 0x40),
             (0x22E65E, 0x23B), (0x22E957, 0x51))
(OUT / "profile-header-group-original.asm").write_text("\n\n".join(
    "\n".join(f"{i.address:08x}: {i.mnemonic} {i.op_str}"
              for i in decoder.disasm(xbe.read(a, size), a)) for a, size in corridors) + "\n")
print(f"{len(cases)} unchanged original caller/group cases and{len(intro)} authored animation snapshots passed")
print(f"fixture SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
