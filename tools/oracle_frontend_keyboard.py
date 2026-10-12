#!/usr/bin/env python3
"""Execute original keyboard bitmap layers, widget ordering, and string setup.

Resource loading, platform save existence, standard string routines, and the GPU
state sink are explicitly bounded seams. Original constructor, comparator,
generator loop, keyboard setup, UI clock, caret alpha and blend table execute
from the pinned retail executable.
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
u.mem_map(0, 0x10000)  # FS exception-registration storage for original constructors.
u.mem_map(0x10000, 0x600000)
u.mem_map(0x2000000, 0x200000)
u.mem_map(0x3000000, 0x10000)
base = struct.unpack_from("<I", xbe, 0x104)[0]
n, table = struct.unpack_from("<II", xbe, 0x11c)
for i in range(n):
    _, va, _, offset, size = struct.unpack_from("<5I", xbe, table-base+i*56)
    u.mem_write(va, xbe[offset:offset+size])
TAGS, HEAP, STOP, STACK = 0x2000000, 0x2010000, 0x3000000, 0x21fff00
u.mem_write(0x4e3b44, struct.pack("<I", TAGS))
cursor = HEAP


def alloc(raw):
    global cursor
    p = cursor
    cursor += (len(raw)+15)&~15
    u.mem_write(p, bytes(raw))
    return p


def word(p):
    return struct.unpack("<H", u.mem_read(p, 2))[0]


def dword(p):
    return struct.unpack("<I", u.mem_read(p, 4))[0]


def wide(p):
    raw = bytearray()
    while word(p+len(raw)):
        raw += u.mem_read(p+len(raw), 2)
        assert len(raw) < 1024
    return raw.decode("utf-16le")


def ret(pop=0, value=None):
    sp = u.reg_read(UC_X86_REG_ESP)
    u.reg_write(UC_X86_REG_EIP, dword(sp))
    u.reg_write(UC_X86_REG_ESP, sp+4+pop)
    if value is not None:
        u.reg_write(UC_X86_REG_EAX, value)


save_names = set()
attempts = []
render_states = []


def hook(guest, address, size, context):
    sp = guest.reg_read(UC_X86_REG_ESP)
    if address == 0x23625d:  # Resource loading is not a layout result.
        ret()
    elif address == 0x13fac0:  # Retail Unicode copy: ECX source, EDI dest, ESI capacity.
        source = guest.reg_read(UC_X86_REG_ECX)
        dest = guest.reg_read(UC_X86_REG_EDI)
        count = guest.reg_read(UC_X86_REG_ESI)
        raw = wide(source).encode("utf-16le")[:max(0, count-1)*2]+bytes(2)
        guest.mem_write(dest, raw)
        ret()
    elif address == 0x320870:  # Standard wcslen.
        ret(value=len(wide(dword(sp+4)).encode("utf-16le"))//2)
    elif address == 0x11c9c0:  # Root drive formatter; irrelevant to candidate string.
        guest.mem_write(guest.reg_read(UC_X86_REG_ESI), b"u:\\\0")
        ret(value=3)
    elif address == 0x13fb70:
        format_string = wide(dword(sp+4))
        assert format_string == "Halo%04d"
        name = format_string.replace("%04d", f"{dword(sp+8):04d}")
        guest.mem_write(guest.reg_read(UC_X86_REG_EDI), name.encode("utf-16le")+bytes(2))
        ret(value=len(name))
    elif address == 0x216da0:  # Full save-name construction passes supplied display name.
        display = wide(dword(sp+4))
        guest.mem_write(guest.reg_read(UC_X86_REG_EDI), display.encode("utf-16le")+bytes(2))
        ret(8, 1)
    elif address == 0x330df2:  # XCreateSaveGame: zero means existing name/collision.
        name = wide(dword(sp+8))
        attempts.append(name)
        ret(24, 0 if name in save_names else 1)
    elif address == 0x3f5cf0:  # GPU state sink; original caller/table executes.
        render_states.append(dict(method=f"{guest.reg_read(UC_X86_REG_ECX):08x}",
                                  value=f"{guest.reg_read(UC_X86_REG_EDX):08x}"))
        ret()


u.hook_add(UC_HOOK_CODE, hook)


def run(entry, args=(), **registers):
    u.mem_write(STACK, struct.pack("<"+"I"*(len(args)+1), STOP, *args))
    u.reg_write(UC_X86_REG_ESP, STACK)
    u.reg_write(UC_X86_REG_FPCW, 0x027f)
    u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    for name, value in registers.items():
        u.reg_write(globals()["UC_X86_REG_"+name.upper()], value)
    u.emu_start(entry, STOP, count=1000000)
    assert u.reg_read(UC_X86_REG_EIP) == STOP
    return u.reg_read(UC_X86_REG_EAX)


def slice_run(entry, end, args=(), **registers):
    u.mem_write(STACK, struct.pack("<"+"I"*(len(args)+1), STOP, *args))
    u.reg_write(UC_X86_REG_ESP, STACK)
    u.reg_write(UC_X86_REG_FPCW, 0x027f)
    u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    u.reg_write(UC_X86_REG_FPTAG, 0xffff)
    u.reg_write(UC_X86_REG_FPSW, 0)
    for name, value in registers.items():
        u.reg_write(globals()["UC_X86_REG_"+name.upper()], value)
    u.emu_start(entry, end, count=1000000)
    assert u.reg_read(UC_X86_REG_EIP) == end


tag = next(t for t in cache.tags.values() if t["group"] == "wgit" and
           cache.u16(cache.pointer(t["address"], cache.magic, t["size"])+4) == 167)
_, screen = cache.tag(tag["datum"], "wgit")
pane = cache.block(screen+32, 76)[0]
widgets = []
for index, record in enumerate(cache.block(pane+36, 56)):
    definition = alloc(cache.data[record:record+56])
    datum = cache.u32(record+28)
    bitmap_tag, bitmap_source = cache.tag(datum, "bitm")
    bitmap_records = cache.block(bitmap_source+68, 116)
    images = alloc(cache.data[bitmap_records[0]:bitmap_records[0]+len(bitmap_records)*116])
    header = bytearray(0x4c)
    struct.pack_into("<II", header, 0x44, len(bitmap_records), images)
    u.mem_write(TAGS+(datum&65535)*16+8, struct.pack("<I", alloc(header)))
    widget = alloc(bytes(0x100))
    run(0x2b01eb, [definition], eax=widget)
    layer = struct.unpack("<h", u.mem_read(widget+0x6a, 2))[0]
    assert layer == struct.unpack_from("<h", cache.data, record+32)[0]
    widgets.append(dict(index=index, name=bitmap_tag["name"], address=widget,
                        layer=layer, type=dword(widget+4),
                        authored_position=list(struct.unpack_from("<hh", cache.data, record+12)),
                        dimensions=[cache.u16(bitmap_records[0]+4), cache.u16(bitmap_records[0]+6)]))
entries = [alloc(struct.pack("<IfhH", w["address"], 0, w["layer"], 0)) for w in widgets]
comparison = run(0x2356d8, entries)
assert comparison == 1  # field layer1 must be submitted after dialog layer0.
reverse_comparison = run(0x2356d8, entries[::-1])
assert reverse_comparison == 0xffffffff

generator_cases = []
u.mem_write(0x4e61c0, struct.pack("<H", 1))
NAME = alloc(bytes(256))
for occupied in [[], ["Halo0001"], ["Halo0001", "Halo0002", "Halo0003"]]:
    save_names = set(occupied)
    attempts = []
    result = run(0x215f40, [0, NAME])
    name = wide(NAME)
    assert name == f"Halo{len(occupied)+1:04d}" and result&255 == 1
    generator_cases.append(dict(occupied=occupied, attempts=attempts, generated=name))

KEYBOARD = alloc(bytes(0x4000))
u.mem_write(KEYBOARD+0x610, bytes(4))  # Original profile-name type0.
u.mem_write(NAME, "Halo0001".encode("utf-16le")+bytes(2))
run(0x2381de, [NAME, 32], ebx=KEYBOARD)
setup = dict(original=wide(KEYBOARD+0x618), text=wide(dword(KEYBOARD+0x818)),
             cursor=(dword(KEYBOARD+0x81c)-dword(KEYBOARD+0x818))//2,
             maximum=word(KEYBOARD+0x820), selected=bool(u.mem_read(KEYBOARD+0x828, 1)[0]))
assert setup == dict(original="Halo0001", text="Halo0001", cursor=8, maximum=17, selected=True)

# Original cumulative UI clock: this corridor includes the untouched x87
# multiplication and integer conversion helper, stopping before other UI work.
clock_cases = []
for start, delta in [(0, 1/60), (0, 1/30), (1000, .016999999061226845),
                     (0xfffffff0, .025), (0, 0), (0, 10)]:
    u.mem_write(0x54d5b8, struct.pack("<I", start))
    delta_bits = struct.unpack("<I", struct.pack("<f", delta))[0]
    slice_run(0x1479c3, 0x1479da, [delta_bits])
    clock_cases.append(dict(start=start, delta_bits=f"{delta_bits:08x}",
                            end=dword(0x54d5b8)))

# Caret alpha uses unsigned UI milliseconds, x87 cosine, a float store, and
# nearest-even FISTP. Execute these instructions directly for fixture phases.
alpha_cases = []
local_frame = alloc(bytes(0x100)) + 0x80
for milliseconds in [0, 250, 500, 1000, 1047, 1500, 2094, 0x80000000, 0xffffffff]:
    for opacity in [1., .25]:
        u.mem_write(0x54d5b8, struct.pack("<I", milliseconds))
        u.mem_write(KEYBOARD+0x64, struct.pack("<f", opacity))
        slice_run(0x237d30, 0x237d6b, ebp=local_frame, ebx=KEYBOARD)
        alpha_cases.append(dict(milliseconds=milliseconds, opacity=opacity,
                                alpha=dword(local_frame-0x14)))

_, matg = cache.tag(next(k for k, t in cache.tags.items() if t["group"] == "matg"), "matg")
_, wigl = cache.tag(next(k for k, t in cache.tags.items() if t["group"] == "wigl"), "wigl")
interface = cache.block(matg+0x108, 0x130)[0]
overlay_assets = []
for role, datum, page in [("selection", cache.u32(interface+0x64), 1),
                          ("caret", cache.u32(wigl+0x114), 0)]:
    asset, source = cache.tag(datum, "bitm")
    record = cache.block(source+0x44, 0x74)[page]
    overlay_assets.append(dict(role=role, datum=f"{datum:08x}", name=asset["name"],
                               page=page, width=cache.u16(record+4),
                               height=cache.u16(record+6)))
blend_cases = []
for mode in [0, 1]:
    render_states = []
    run(0x142f0, eax=mode)
    blend_cases.append(dict(mode=mode, states=list(render_states)))
render_states = []
slice_run(0x52071, 0x52089, esi=0x10101)
color_write = list(render_states)
report = dict(xbe_sha256=XBE_HASH, map_sha256=hashlib.sha256(cache.data).hexdigest(),
              original_entries=["002b01eb", "0022e27b", "002b0a68", "002356d8", "00215f40", "002381de",
                                "001479c3", "00322b0c", "00237d30", "000142f0", "00052071"],
              widgets=widgets, comparator_forward=comparison, comparator_reverse=reverse_comparison,
              generator_cases=generator_cases, keyboard_setup=setup,
              clock_cases=clock_cases, caret_alpha_cases=alpha_cases,
              overlay_assets=overlay_assets,
              blend_cases=blend_cases, color_write=color_write,
              boundary_stubs=["23625d bitmap resource load", "13fac0 Unicode copy and 320870 wcslen",
                              "11c9c0 root formatter and 13fb70 candidate formatter with original format string",
                              "216da0 save-name platform construction and 330df2 save-existence result",
                              "3f5cf0 GPU state sink; original142f0 table and52071 color-mask setup execute"],
              scope="Original authored bitmap constructor/layers, comparator, collision loop, selected input setup, cumulative UI clock, caret alpha and render blend state. Overlay glyph bounds are independently frozen by oracle_frontend_text_style.py. No complete keyboard or GPU pixel parity claim.")
OUT.mkdir(parents=True, exist_ok=True)
(OUT / "keyboard-original.json").write_text(json.dumps(report, indent=2)+"\n")
print(json.dumps(report, indent=2))
