#!/usr/bin/env python3
"""Execute original profile nested-skin placement and visibility instructions.

Allocation/child constructors and text-buffer methods are isolated seams.
The original layout, bounds union, visibility, sequence, and color kernels run
unchanged from default.xbe against copied retail skin and palette definitions.
"""
import hashlib
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "analysis/frontend/profile-regression"
XBE_HASH = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
xbe = (ROOT / "assets/disc/default.xbe").read_bytes()
assert hashlib.sha256(xbe).hexdigest() == XBE_HASH
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x10000, 0x600000)
u.mem_map(0x2000000, 0x200000)
u.mem_map(0x3000000, 0x10000)
base = struct.unpack_from("<I", xbe, 0x104)[0]
n, table = struct.unpack_from("<II", xbe, 0x11c)
for i in range(n):
    _, va, _, offset, size = struct.unpack_from("<5I", xbe, table-base+i*56)
    u.mem_write(va, xbe[offset:offset+size])

STATE, TAGS, STOP, STACK = 0x2000000, 0x2010000, 0x3000000, 0x21fff00
GET_TEXT, SET_TEXT, BUFFER = 0x3000100, 0x3000200, 0x2048000
VTABLE = 0x2047000
u.mem_write(VTABLE + 0x3c, struct.pack("<I", GET_TEXT))
u.mem_write(BUFFER, struct.pack("<I", VTABLE + 0x100))
u.mem_write(VTABLE + 0x104, struct.pack("<I", SET_TEXT))
u.mem_write(0x4e3b44, struct.pack("<I", TAGS))

metadata_cursor = STATE + 0x20000
groups = {}
bitmap_sizes = {}
group_rows = []
focus_track_defs = []
_, focus_source = cache.tag(0xe50202e2, "skin")
for record in cache.block(focus_source+20, 16):
    keys = cache.block(record+8, 20)
    focus_track_defs.append((record, keys))


def alloc_metadata(raw):
    global metadata_cursor
    p = metadata_cursor
    metadata_cursor += (len(raw) + 15) & ~15
    u.mem_write(p, bytes(raw))
    return p


def write_tag(datum, address):
    u.mem_write(TAGS + (datum & 65535)*16+8, struct.pack("<I", address))


def load_group(datum):
    if datum in groups:
        return groups[datum]
    tag, source = cache.tag(datum, "skin")
    header = bytearray(cache.data[source:source+64])
    p = alloc_metadata(header)
    groups[datum] = p
    write_tag(datum, p)
    for field, stride in [(28, 44), (36, 56), (44, 36), (52, 24)]:
        records = cache.block(source+field, stride)
        if records:
            child = alloc_metadata(cache.data[records[0]:records[0]+len(records)*stride])
            u.mem_write(p+field+4, struct.pack("<I", child))
        for i, original in enumerate(records):
            if field == 36:
                bitmap = cache.u32(original+28)
                if bitmap not in bitmap_sizes:
                    bitmap_tag, bitmap_source = cache.tag(bitmap, "bitm")
                    bitmap_records = cache.block(bitmap_source+68, 116)
                    header2 = bytearray(0x4c)
                    struct.pack_into("<I", header2, 0x44, len(bitmap_records))
                    write_tag(bitmap, alloc_metadata(header2))
                    bitmap_sizes[bitmap] = [struct.unpack_from("<hh", cache.data, b+4)
                                            for b in bitmap_records]
            elif field == 52:
                load_group(cache.u32(original+8))
    group_rows.append(dict(datum=f"{datum:08x}", name=tag["name"],
                           definition_hex=cache.data[source:source+64].hex()))
    return p


outer = load_group(0xe50202e2)
inner = load_group(0xe50402e4)
for i, (record, keys) in enumerate(focus_track_defs):
    definition = bytearray(cache.data[record:record+16])
    key_pointer = alloc_metadata(cache.data[keys[0]:keys[0]+len(keys)*20])
    struct.pack_into("<I", definition, 12, key_pointer)
    focus_track_defs[i] = alloc_metadata(definition)
wigl = next(t for t in cache.tags.values() if t["group"] == "wigl")
_, wigl_source = cache.tag(wigl["datum"], "wigl")
wigl_guest = alloc_metadata(cache.data[wigl_source:wigl_source+452])
matg = next(t for t in cache.tags.values() if t["group"] == "matg")
_, matg_source = cache.tag(matg["datum"], "matg")
palette_records = cache.block(matg_source+0x160, 12)
palette_raw = cache.data[palette_records[0]:palette_records[0]+len(palette_records)*12]
palette = alloc_metadata(palette_raw)
matg_guest = alloc_metadata(bytes(0x168))
u.mem_write(matg_guest+0x160, struct.pack("<II", len(palette_records), palette))
u.mem_write(0x4e034c, struct.pack("<I", matg_guest))
u.mem_write(0x4e0350, struct.pack("<I", 1))

heap_cursor = STATE+0x50000
nodes = []
text_sets = []
delegate_calls = []
next_after = {}


def dword(p):
    return struct.unpack("<I", u.mem_read(p, 4))[0]


def word(p):
    return struct.unpack("<h", u.mem_read(p, 2))[0]


def ret(pop, eax=None):
    sp = u.reg_read(UC_X86_REG_ESP)
    address = dword(sp)
    u.reg_write(UC_X86_REG_ESP, sp+4+pop)
    if eax is not None:
        u.reg_write(UC_X86_REG_EAX, eax)
    u.reg_write(UC_X86_REG_EIP, address)


def make_node(parent, kind, definition, index):
    global heap_cursor
    p = heap_cursor
    heap_cursor += 0x100
    u.mem_write(p, bytes(0x100))
    u.mem_write(p, struct.pack("<IIH", VTABLE, kind, 0))
    u.mem_write(p+0xa, struct.pack("<h", index))
    u.mem_write(p+0x70, struct.pack("<I", definition))
    u.mem_write(p+0x10, struct.pack("<I", parent))
    u.mem_write(p+0x6e, b"\x01")
    u.mem_write(p+0x28, struct.pack("<3f", 1, 1, 1))
    if kind == 8:
        bitmap = dword(definition+28)
        width, height = bitmap_sizes[bitmap][0]
        x, y = word(definition+12), word(definition+14)
        bounds = (y, x, y-height, x+width)
        placeholder = dword(definition+44)
    elif kind in (6, 9):
        bounds = struct.unpack("<4h", u.mem_read(definition+28, 8))
        placeholder = dword(definition+36) if kind == 6 else None
        if kind == 6:
            u.mem_write(p+0x28, bytes(u.mem_read(definition+16, 12)))
    else:
        bounds = (0, 0, 0, 0)
        placeholder = None
    u.mem_write(p+0x20, struct.pack("<4h", *bounds))
    if parent in next_after:
        u.mem_write(next_after[parent]+0x18, struct.pack("<I", p))
    else:
        u.mem_write(parent+0x14, struct.pack("<I", p))
    next_after[parent] = p
    nodes.append(dict(address=p, parent=parent, kind=kind, index=index,
                      placeholder=placeholder, definition=definition))
    return p


def hook(guest, address, size, user):
    sp = guest.reg_read(UC_X86_REG_ESP)
    if address == 0x22fa30:
        p = make_node(guest.reg_read(UC_X86_REG_ESI), 8, dword(sp+4), 0)
        ret(4, p)
    elif address == 0x2baf38:
        p = make_node(guest.reg_read(UC_X86_REG_ESI), 9,
                      guest.reg_read(UC_X86_REG_EBX), 0)
        ret(0, p)
    elif address == 0x2baeb1:
        p = make_node(dword(sp+4), 6, guest.reg_read(UC_X86_REG_EBX), dword(sp+8))
        ret(8, p)
    elif address == 0x2baf5d:
        # Run original place and nested bounds union after replacing only
        # construction/allocation. Its return ABI consumes the 4 factory args.
        parent, point, index, definition = struct.unpack("<4I", guest.mem_read(sp+4, 16))
        p = make_node(parent, 10, definition, index)
        u.mem_write(p+0x70, struct.pack("<II", index, definition))
        return_address = dword(sp)
        return_stub = 0x3000300+p-(STATE+0x50000)
        delegated_factory_returns[return_stub] = (return_address, sp+20, p)
        guest.mem_write(sp-8, struct.pack("<II", return_stub, point))
        guest.reg_write(UC_X86_REG_ESP, sp-8)
        guest.reg_write(UC_X86_REG_ECX, p)
        guest.reg_write(UC_X86_REG_EIP, 0x2afdbc)
    elif address in delegated_factory_returns:
        next_pc, next_sp, p = delegated_factory_returns.pop(address)
        guest.reg_write(UC_X86_REG_ESP, next_sp)
        guest.reg_write(UC_X86_REG_EAX, p)
        guest.reg_write(UC_X86_REG_EIP, next_pc)
    elif address == GET_TEXT:
        ret(0, BUFFER)
    elif address == SET_TEXT:
        pointer = dword(sp+4)
        chars = bytearray()
        while True:
            c = guest.mem_read(pointer+len(chars), 2)
            if c == b"\0\0":
                break
            chars += c
            assert len(chars) < 1024
        text_sets.append(chars.decode("utf-16le"))
        ret(4)
    elif address == 0x2b12ca:
        delegate_calls.append(dict(kind="appearance emblem update",
                                   node=guest.reg_read(UC_X86_REG_EAX)))
        ret(4)
    elif address == 0x24bd3b:
        ret(4, focus_track_defs[dword(sp+4)])
    elif address == 0x22ed7a:
        ret(0, int(button_focus))
    elif address == 0x148350:
        ret(0, wigl_guest)
    elif address == 0x253cc8:
        ret(0, button_definition)


delegated_factory_returns = {}
button_focus = False
button_definition = 0
u.hook_add(UC_HOOK_CODE, hook)


def run(entry, args=(), **registers):
    u.mem_write(STACK, struct.pack("<"+"I"*(len(args)+1), STOP, *args))
    u.reg_write(UC_X86_REG_ESP, STACK)
    u.reg_write(UC_X86_REG_FPCW, 0x027f)
    u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    for k, v in registers.items():
        u.reg_write(globals()["UC_X86_REG_"+k.upper()], v)
    u.emu_start(entry, STOP, count=1000000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    assert not delegated_factory_returns


ROOT_NODE, POINT, BOUNDS, ITEM = STATE+0x42000, STATE+0x43000, STATE+0x43100, STATE+0x40000
u.mem_write(ROOT_NODE, bytes(0x100))
u.mem_write(POINT, struct.pack("<hh", 0, 0))
run(0x2bacbc, [ROOT_NODE, outer], edi=POINT)
run(0x2bafa4, eax=ROOT_NODE, ecx=BOUNDS)
row_bounds = struct.unpack("<4h", u.mem_read(BOUNDS, 8))
placement = [dict(kind=n["kind"], index=word(n["address"]+0xa),
                  definition_flags=dword(n["definition"]),
                  placeholder=f'{n["placeholder"]:08x}' if n["placeholder"] is not None else None,
                  bounds=list(struct.unpack("<4h", u.mem_read(n["address"]+0x20, 8))),
                  parent="outer" if n["parent"] == ROOT_NODE else "nested") for n in nodes]
nested = next(n["address"] for n in nodes if n["kind"] == 10)
cases = []
for name, flags, background, rank, explicit_color, appearance in [
    ("create-new", 0x20, 1, -1, None, None),
    ("create-new-background-off", 0x20, 0, -1, None, None),
    ("no-fields", 0, 0, -1, None, None),
    ("saved-name-only", 1, 0, -1, None, None),
    ("saved-name-explicit-rgb", 0x201, 0, -1, (.2, .3, .4), None),
    ("saved-name-rank-8", 0x41, 0, 8, None, None),
    ("saved-appearance-color-10", 3, 0, -1, None, (10, 0, 0, 0)),
]:
    raw = bytearray(0x74)
    struct.pack_into("<II", raw, 0, flags, STATE+0x40300)
    struct.pack_into("<h", raw, 0x60, rank)
    raw[0x5e] = background
    if explicit_color:
        struct.pack_into("<3f", raw, 0x68, *explicit_color)
    if appearance:
        raw[0x48:0x4c] = bytes(appearance)
    u.mem_write(ITEM, bytes(raw))
    u.mem_write(STATE+0x40300, "Source Profile\0".encode("utf-16le"))
    for node in nodes:
        p = node["address"]
        u.mem_write(p+0x6e, b"\x01")
        u.mem_write(p+0x88, struct.pack("<h", 0))
        u.mem_write(p+0x28, struct.pack("<3f", .12, .34, .56))
    text_sets.clear()
    delegate_calls.clear()
    run(0x2afeae, [nested], ebx=ITEM)
    run(0x2bafa4, eax=ROOT_NODE, ecx=BOUNDS)
    after_bounds = struct.unpack("<4h", u.mem_read(BOUNDS, 8))
    assert after_bounds == row_bounds, (name, after_bounds, row_bounds)
    states = []
    for node in nodes:
        if node["parent"] != nested:
            continue
        p = node["address"]
        states.append(dict(kind=node["kind"], index=word(p+0xa),
                           placeholder=f'{node["placeholder"]:08x}' if node["placeholder"] is not None else None,
                           enabled=bool(u.mem_read(p+0x6e, 1)[0]), sequence=word(p+0x88),
                           enabled_source_verified=not(node["kind"] == 9 and bool(appearance)),
                           rgb=list(struct.unpack("<3f", u.mem_read(p+0x28, 12))),
                           rgb_hex=u.mem_read(p+0x28, 12).hex()))
    cases.append(dict(name=name, flags=f"{flags:08x}", background=background,
                      rank=rank, text_sets=text_sets.copy(), nodes=states,
                      row_bounds_after_visibility=list(after_bounds),
                      delegated_calls=delegate_calls.copy()))

assert row_bounds == (28, -11, 7, 273)
assert [n["enabled"] for n in cases[0]["nodes"]] == [False, True, False, False, False]
assert [n["enabled"] for n in cases[3]["nodes"]] == [False, False, False, False, True]
assert [n["enabled"] for n in cases[5]["nodes"]] == [False, False, True, False, True]
assert cases[5]["nodes"][2]["sequence"] == 8
assert cases[3]["nodes"][4]["rgb_hex"] == "abaa4a3fc871203fc871203f"
assert cases[6]["nodes"][4]["rgb_hex"] == "c871203f83b8373fabaa4a3f"

# The retail item vtable 459F10 dispatches +14 to 22E930. That virtual
# recursively replaces each descendant animation with the same definition.
u.mem_write(VTABLE+0x14, struct.pack("<I", 0x22e930))
u.mem_write(VTABLE+0xc, struct.pack("<I", 0x22e391))
LIST_PARENT = STATE+0x44000
u.mem_write(LIST_PARENT, bytes(0x100))
u.mem_write(LIST_PARENT+4, struct.pack("<I", 1))
u.mem_write(LIST_PARENT+0x58, struct.pack("<4f", 100, 200, 300, .25))
u.mem_write(ROOT_NODE, struct.pack("<II", VTABLE, 2))
u.mem_write(ROOT_NODE+0x10, struct.pack("<I", LIST_PARENT))
u.mem_write(ROOT_NODE+0x1c, struct.pack("<I", LIST_PARENT))
u.mem_write(ROOT_NODE+0x78, bytes(4))
focus_cases = []
for name, entry, duration in [("focused", 0x24ac80, 120), ("unfocused", 0x24ad0b, 200)]:
    u.mem_write(0x54d5b8, struct.pack("<I", 1000))
    for node in [ROOT_NODE]+[n["address"] for n in nodes]:
        u.mem_write(node+0x34, bytes(52))
        u.mem_write(node+0x64, struct.pack("<f", .25))
    run(entry, ecx=ROOT_NODE)
    snapshots = []
    for offset in [0, duration//2, duration, duration*2]:
        u.mem_write(0x54d5b8, struct.pack("<I", 1000+offset))
        run(0x22e391, ecx=ROOT_NODE)
        snapshots.append(dict(elapsed_ms=offset, nodes=[
            dict(kind=n["kind"], parent="outer" if n["parent"] == ROOT_NODE else "nested",
                 index=word(n["address"]+0xa),
                 scale=struct.unpack("<f", u.mem_read(n["address"]+0x64, 4))[0],
                 offset=list(struct.unpack("<3f", u.mem_read(n["address"]+0x58, 12))))
            for n in nodes]))
    focus_cases.append(dict(name=name, parent_scale=.25, parent_offset=[100, 200, 300],
                            snapshots=snapshots))
    expected_scale, expected_x = (1, 17) if name == "focused" else (.3, 0)
    for node in snapshots[-1]["nodes"]:
        assert struct.pack("<f", node["scale"]) == struct.pack("<f", expected_scale)
        assert node["offset"] == [expected_x, 0, 0]
u.mem_write(ROOT_NODE+0x34, bytes(52))
u.mem_write(ROOT_NODE+0x42, struct.pack("<H", 1))
u.mem_write(ROOT_NODE+0x64, struct.pack("<f", .25))
run(0x24ac80, ecx=ROOT_NODE)
intro_gate = dict(animation_flags=word(ROOT_NODE+0x42),
                  scale=struct.unpack("<f", u.mem_read(ROOT_NODE+0x64, 4))[0],
                  target=dword(ROOT_NODE+0x38))
assert intro_gate == dict(animation_flags=1, scale=.25, target=0)

keyboard = next(t for t in cache.tags.values() if t["group"] == "wgit" and
                cache.u16(cache.pointer(t["address"], cache.magic, t["size"])+4) == 167)
_, keyboard_source = cache.tag(keyboard["datum"], "wgit")
keyboard_pane = cache.block(keyboard_source+32, 76)[0]
keyboard_keys = cache.block(keyboard_pane+4, 60)
button_definition = alloc_metadata(cache.data[keyboard_keys[0]:keyboard_keys[0]+60])
BUTTON = STATE+0x45000
u.mem_write(BUTTON, bytes(0x100))
u.mem_write(BUTTON, struct.pack("<II", VTABLE, 5))
button_colors = []
for button_focus in [False, True]:
    u.mem_write(BUFFER+8, struct.pack("<3f", .1, .2, .3))
    run(0x253e24, ecx=BUTTON)
    rgb = bytes(u.mem_read(BUFFER+8, 12))
    expected = cache.data[(wigl_source+0x16c) if button_focus else (keyboard_keys[0]+16):
                          ((wigl_source+0x16c) if button_focus else (keyboard_keys[0]+16))+12]
    assert rgb == expected
    button_colors.append(dict(focused=button_focus, rgb=list(struct.unpack("<3f", rgb)),
                              rgb_hex=rgb.hex()))

report = dict(xbe_sha256=XBE_HASH, map_sha256=hashlib.sha256(cache.data).hexdigest(),
              original_entries=["002bacbc", "002afdbc", "002bafa4", "002afeae",
                                "002b014f", "002b015b", "002b016d", "002b0191",
                                "002b01c7", "002b01da", "002b0a14", "001318d0", "00131a00", "0007f790", "001a06a0",
                                "0024ac80", "0024ad0b", "0022e930", "0022e89c", "0022e391", "0022e3cd", "0022f0ff", "00253e24"],
              groups=group_rows, placement=placement, row_bounds=list(row_bounds),
              row_height=row_bounds[0]-row_bounds[2], cases=cases,
              focus_cases=focus_cases, intro_gate=intro_gate, button_colors=button_colors,
              boundary_stubs=["22fa30/2baf38/2baeb1/2baf5d: child allocation, constructor inputs, child linkage",
                              "Text virtual getter and text-buffer copy",
                              "24bd3b skin-animation lookup supplies copied original track; focus/update executes unchanged",
                              "22ed7a focused predicate and 148350/253cc8 metadata lookup for button 253e24 color probe",
                              "2b12ca appearance/emblem update: record request only; no emblem visibility claim"],
              scope="Unchanged original layout/bounds/visibility/sequence/HSV, row focus/update, intro gate, and button RGB instructions on authored retail definitions and controlled widget inputs. Constructors are initialized from documented bounds, then original placement/union execute. No native pixel or full frontend parity claim.")
OUT.mkdir(parents=True, exist_ok=True)
(OUT/"profile-skin-original.json").write_text(json.dumps(report, indent=2)+"\n")
print(json.dumps(dict(row_bounds=report["row_bounds"], row_height=report["row_height"],
                      cases=[dict(name=c["name"], nodes=c["nodes"], text_sets=c["text_sets"],
                                  delegated_calls=c["delegated_calls"]) for c in cases]), indent=2))
