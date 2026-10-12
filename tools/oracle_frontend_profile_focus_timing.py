#!/usr/bin/env python3
"""Execute original profile-row focus timing, recursion and common focus movement.

Only original-XBE instructions compute animation samples. Literal mainmenu
definitions are relocated, never fitted. Window lookup/current-focus storage,
shared-globals residency, current-list-definition lookup and sound are explicit
environment seams. Dynamic row-data binding is excluded from this timing probe.
No native build, GPU, emulator session or framebuffer capture is performed.
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
xbe = Xbe(ROOT / "assets/disc/default.xbe")
retail = Retail()
g = retail.g
CHANNEL, WINDOW = 0x3000100, 0x3000110
focus = 0
list_definition = 0
calls, lookup_tracks = {}, []
WATCH = (0x22E27B, 0x22EB18, 0x22E89C, 0x22E930, 0x22E391,
         0x22E3CD, 0x22E3B4, 0x22F0FF, 0x22E65E, 0x22E957,
         0x24AC80, 0x24AD0B, 0x24AE28, 0x24BD3B, 0x14837A, 0x22ECB4,
         0x22ED7A, 0x22EB8C, 0x24C024, 0x24C1C5, 0x24C610)


def supplied(value=0, arguments=0):
    stack = g.reg_read(UC_X86_REG_ESP)
    retail.ret(value)
    g.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    global focus
    if address in WATCH:
        calls[address] = calls.get(address, 0) + 1
    if address == 0x148350:
        supplied(wigl)
    elif address == 0x24BBF3:
        supplied(list_definition)
    elif address == CHANNEL:
        supplied(3)
    elif address == WINDOW:
        supplied(4)
    elif address == 0x148D91:
        supplied(focus)
    elif address == 0x148DFC:
        focus = g.reg_read(UC_X86_REG_EDX)
        supplied()
    elif address == 0x236299:
        supplied()
    elif address == 0x16BBC0:
        # Six ordered occupied records. Only the list data-iteration service
        # is supplied; the actual24C1C5 branch and datum_next wrapper execute.
        datum = g.reg_read(UC_X86_REG_EAX)
        supplied(0x10000 if datum == 0xFFFFFFFF else
                 datum + 1 if datum < 0x10005 else 0xFFFFFFFF)
    elif address == 0x16BCC0:
        datum = g.reg_read(UC_X86_REG_EDX)
        supplied(datum - 1 if 0x10000 < datum <= 0x10005 else 0xFFFFFFFF)
    elif address == 0x24BD3B:
        stack = g.reg_read(UC_X86_REG_ESP)
        lookup_tracks.append(retail.r(stack + 4))


g.hook_add(UC_HOOK_CODE, hook)
_, wigl_source = cache.tag(next(t["datum"] for t in cache.tags.values()
                              if t["group"] == "wigl"), "wigl")
wigl_data = bytearray(cache.data[wigl_source:wigl_source + 452])
global_animations = cache.block(wigl_source + 0x120, 44)
relocated = bytearray()
global_targets = {}
for animation_index, p in enumerate(global_animations):
    raw = bytearray(cache.data[p:p + 44])
    for field in (8, 20, 36):
        keys = cache.block(p + field, 20)
        if keys:
            target = retail.alloc(b"".join(cache.data[k:k + 20] for k in keys))
            struct.pack_into("<I", raw, field + 4, target)
            global_targets[target] = f"global{animation_index}_{field}"
    relocated.extend(raw)
struct.pack_into("<I", wigl_data, 0x124, retail.alloc(relocated))
skin_refs = cache.block(wigl_source + 0x138, 8)
struct.pack_into("<I", wigl_data, 0x13C, retail.alloc(b"".join(
    cache.data[p:p + 8] for p in skin_refs)))
wigl = retail.alloc(wigl_data)


def vtable(source):
    result = retail.alloc(xbe.read(source, 0x60))
    retail.w(result + 0x2C, CHANNEL)
    retail.w(result + 0x30, WINDOW)
    # Row v3's data/text/bitmap refresh is unrelated to animation timing.
    # The real recursive base v3 is retained for every node here.
    retail.w(result + 0x0C, 0x22E391)
    return result


base_vtable = vtable(0x458788)
row_vtable = vtable(0x459F10)
list_vtable = vtable(0x459E88)


def node(kind, parent=0, table=base_vtable):
    pointer = retail.alloc(bytes(0x100))
    retail.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, pointer),
                                            (UC_X86_REG_EAX, kind)])
    retail.w(pointer, table)
    if parent:
        retail.run(0x22EB18, registers=[(UC_X86_REG_ESI, parent),
                                       (UC_X86_REG_EAX, pointer)])
    return pointer


def set_clock(time):
    retail.w(0x54D5B8, time)


def animation(pointer):
    p = pointer + 0x34
    target = retail.r(p + 4)
    return dict(type=retail.read(p, "<i")[0],
                target=(target_names.get(target, "none" if not target else "global")),
                frames=retail.read(p + 8, "<h")[0],
                frame=retail.read(p + 10, "<h")[0],
                direction=retail.read(p + 12, "<h")[0],
                flags=retail.read(p + 14, "<H")[0],
                delay=retail.r(p + 16), mode=retail.r(p + 20),
                start_time=retail.r(p + 24), end_time=retail.r(p + 28),
                milliseconds=retail.r(p + 32),
                position_words=[f"{n:08x}" for n in retail.read(p + 36, "<3I")],
                alpha_word=f"{retail.r(p + 48):08x}",
                drawing_admitted=bool(retail.run(0x22E3B4,
                    registers=[(UC_X86_REG_ECX, pointer)])))


def start(row, focused):
    retail.run(0x24AC80 if focused else 0x24AD0B,
               registers=[(UC_X86_REG_ECX, row)])


def update(row, time):
    set_clock(time)
    retail.run(0x22E391, registers=[(UC_X86_REG_ECX, row)])


def reset(nodes, time):
    set_clock(time)
    definition = retail.alloc(struct.pack("<2I4h5I4f", 0xFFFFFFFF, 0, 0, 0, 1, 0,
                                           0, 0, 0, 0, 0, 0., 0., 0., 1.))
    for pointer in nodes:
        retail.run(0x22E89C, registers=[(UC_X86_REG_EAX, pointer),
                                       (UC_X86_REG_ECX, definition)])


screens, samples, controls, timelines = [], [], [], []
for screen_id, skin_datum in ((10, 0xE50202E2), (193, 0xE50C02EC)):
    _, skin_source = cache.tag(skin_datum, "skin")
    skin = bytearray(cache.data[skin_source:skin_source + 60])
    tracks, definitions, target_names = [], [], global_targets.copy()
    for index, p in enumerate(cache.block(skin_source + 20, 16)):
        raw = bytearray(cache.data[p:p + 16])
        keys = cache.block(p + 8, 20)
        target = retail.alloc(b"".join(cache.data[k:k + 20] for k in keys))
        target_names[target] = ("focus_in", "focus_out", "idle")[index]
        struct.pack_into("<I", raw, 12, target)
        tracks.append(raw)
        definitions.append(dict(index=index, milliseconds=cache.u32(p + 4),
            raw_hex=cache.data[p:p + 16].hex(),
            keys=[cache.data[k:k + 20].hex() for k in keys]))
    struct.pack_into("<I", skin, 24, retail.alloc(b"".join(tracks)))
    retail.w(retail.table + (skin_datum & 65535) * 16 + 8, retail.alloc(skin))
    source_screen = next(cache.tag(t["datum"], "wgit")[1]
        for t in cache.tags.values() if t["group"] == "wgit"
        and cache.u16(cache.tag(t["datum"], "wgit")[1] + 4) == screen_id)
    pane = cache.block(source_screen + 32, 76)[0]
    source_list = cache.block(pane + 12, 24)[0]
    list_definition = retail.alloc(cache.data[source_list:source_list + 24])
    list_animation = cache.u16(source_list + 12) - 1
    list_delay = cache.u16(source_list + 14)
    screen = node(0)
    retail.w(screen + 0x70, screen_id)
    g.mem_write(screen + 0x68, struct.pack("<h", cache.u16(pane + 2) - 1))
    list_node = node(1, screen, list_vtable)
    g.mem_write(list_node + 0x68, struct.pack("<h", list_animation))
    retail.w(list_node + 0x70, retail.alloc(bytes(0x48)))
    first = node(2, list_node, row_vtable)
    row = node(2, list_node, row_vtable)
    group = node(10, row)
    text = node(6, group)
    bitmap = node(8, group)
    emblem = node(9, group)
    nodes = [row, group, text, bitmap, emblem]
    for p in (first, row):
        retail.w(p + 0x70, 0x10001 if p == first else 0x10002)
        g.mem_write(p + 0x68, struct.pack("<h", list_animation))
        retail.w(p + 0x74, list_delay)
    screens.append(dict(screen=screen_id, skin=f"{skin_datum:08x}",
        list_definition_hex=cache.data[source_list:source_list + 24].hex(),
        list_animation_index=list_animation, list_delay=list_delay,
        pane_animation_index=cache.u16(pane + 2) - 1,
        pane_global_animation_hex=cache.data[global_animations[cache.u16(pane + 2) - 1]:
            global_animations[cache.u16(pane + 2) - 1] + 44].hex(),
        pane_global_key_hex={str(field): [cache.data[k:k + 20].hex()
            for k in cache.block(global_animations[cache.u16(pane + 2) - 1] + field, 20)]
            for field in (8, 20, 36)},
        global_animation_hex=cache.data[global_animations[list_animation]:
            global_animations[list_animation] + 44].hex(), tracks=definitions))

    # Independent fresh starts freeze every integer millisecond, both exact
    # endpoints and the immediate (pre-update) reset separately.
    for focused, label, maximum in ((True, "focus_in", 140),
                                     (False, "focus_out", 220)):
        for ms in range(maximum + 1):
            reset(nodes, 10000)
            lookup_tracks.clear()
            start(row, focused)
            if ms == 0:
                controls.append(dict(screen=screen_id, kind=label + "_before_update",
                    row=animation(row), descendant=animation(emblem),
                    lookups=lookup_tracks.copy()))
            update(row, 10000 + ms)
            states = [animation(p) for p in nodes]
            assert all(a == states[0] for a in states)
            samples.append(dict(screen=screen_id, kind=label, elapsed=ms,
                                animation=states[0]))

    # The gate is literal flags bits0/1, rather than remaining elapsed time.
    for flags in (0, 1, 2, 3, 4, 8):
        for focused in (False, True):
            reset(nodes, 20000)
            for p in nodes:
                g.mem_write(p + 0x42, struct.pack("<H", flags))
            before = [animation(p) for p in nodes]
            set_clock(20020); lookup_tracks.clear(); calls.clear(); start(row, focused)
            after = [animation(p) for p in nodes]
            blocked = bool(flags & 3)
            assert (before == after) == blocked
            assert calls.get(0x24AE28, 0) == int(focused)
            controls.append(dict(screen=screen_id, kind="flag_gate", flags=flags,
                focused=focused, blocked=blocked, before=before[0], after=after[0],
                lookups=lookup_tracks.copy(),
                executed={f"{a:08x}": n for a, n in sorted(calls.items())}))

    # The first item's special loss-of-focus branch checks its previous
    # sibling and the rolling end_time, not total animation elapsed time.
    for delta in (0, 1, 2, 10):
        reset([first], 30000)
        set_clock(30000 + delta); lookup_tracks.clear(); start(first, False)
        controls.append(dict(screen=screen_id, kind="first_item_unfocus",
            clock_delta=delta, animation=animation(first),
            lookups=lookup_tracks.copy()))
        assert animation(first)["frames"] == (0 if delta <= 1 else 2)

    reset(nodes, 40000)
    events = {0: True, 40: False, 80: True, 100: False}
    points = []
    for ms in range(0, 321, 10):
        set_clock(40000 + ms)
        if ms in events:
            start(row, events[ms])
        update(row, 40000 + ms)
        points.append(dict(elapsed=ms, focus_event=events.get(ms),
                           animation=animation(emblem)))
    timelines.append(dict(screen=screen_id, kind="rapid_reversal", points=points))

    # Exact real common focus entry and real directional event handler.
    reset([first, *nodes], 50000)
    focus = first
    start(first, True); update(first, 51000)
    set_clock(51010); calls.clear(); lookup_tracks.clear()
    event = retail.alloc(struct.pack("<3I", 3, 0, 0) + bytes(0x68))
    retail.run(0x24C024, argv=[event], registers=[(UC_X86_REG_ECX, list_node)])
    assert focus == row
    assert calls.get(0x24C1C5) == calls.get(0x22ECB4) == 1
    assert calls.get(0x24AC80) == calls.get(0x24AD0B) == 1
    controls.append(dict(screen=screen_id, kind="directional_down_event3",
        selected_row="second", old=animation(first), new=animation(row),
        lookups=lookup_tracks.copy(),
        executed={f"{a:08x}": n for a, n in sorted(calls.items())}))
    # At the bottom visible widget, data can scroll through the same focused
    # slot. This changes the profile datum without sending a new focus event.
    set_clock(51020); calls.clear(); lookup_tracks.clear()
    before = [animation(first), animation(row)]
    retail.run(0x24C024, argv=[event], registers=[(UC_X86_REG_ECX, list_node)])
    assert focus == row and retail.r(first + 0x70) == 0x10002
    assert retail.r(row + 0x70) == 0x10003
    assert calls.get(0x22ECB4, 0) == calls.get(0x24AC80, 0) == calls.get(0x24AD0B, 0) == 0
    assert before == [animation(first), animation(row)]
    controls.append(dict(screen=screen_id, kind="directional_scroll_same_slot",
        old=before, after=[animation(first), animation(row)],
        resulting_datums=[f"{retail.r(p + 0x70):08x}" for p in (first, row)],
        lookups=lookup_tracks.copy(),
        executed={f"{a:08x}": n for a, n in sorted(calls.items())}))
    # Direct focus target is the shared mechanism, not a claimed original
    # mouse-hover input implementation.
    set_clock(51040); calls.clear(); lookup_tracks.clear()
    retail.run(0x22ECB4, argv=[1], registers=[(UC_X86_REG_EAX, first)])
    assert focus == first
    controls.append(dict(screen=screen_id, kind="common_focus_target_first",
        old=animation(row), new=animation(first), lookups=lookup_tracks.copy(),
        executed={f"{a:08x}": n for a, n in sorted(calls.items())}))

    # Admission is a separate predicate: recursive animation updates do not
    # skip a descendant merely because its dynamic value6e is false.
    reset(nodes, 55000)
    g.mem_write(emblem + 0x6E, b"\0")
    start(row, True); update(row, 55060)
    assert animation(emblem)["alpha_word"] == animation(row)["alpha_word"]
    assert not animation(emblem)["drawing_admitted"]
    controls.append(dict(screen=screen_id, kind="hidden_descendant_updates",
        row=animation(row), hidden=animation(emblem)))
    g.mem_write(emblem + 0x6E, b"\1")

    # Isolated row updates alone never end its intro flag: only a type0
    # screen restarts to type4, then propagates that activation to rows.
    for intro_type in (0, 1):
        for selected in (False, True):
            reset(nodes, 60000)
            focus = row if selected else first
            retail.run(0x22E957, argv=[intro_type], registers=[(UC_X86_REG_ECX, row)])
            initial = animation(row)
            points = []
            for ms in range(0, 451):
                update(row, 60000 + ms)
                if ms in (0, 1, 40, 100, 119, 120, 121, 199, 200, 209,
                          210, 211, 212, 248, 249, 250, 251, 252, 300, 330,
                          331, 332, 360, 369, 370, 400, 450):
                    points.append(dict(elapsed=ms, row=animation(row),
                        descendant=animation(emblem)))
            assert animation(row)["flags"] == 1
            timelines.append(dict(screen=screen_id, kind="isolated_row_intro_control",
                intro_type=intro_type, selected=selected, initial=initial, points=points))

    # Execute the enclosing source screen's actual restart/recursion. The
    # cadence is explicit input, not claimed measured Xbox frame chronology.
    for intro_type in (0, 1):
        for selected in (False, True):
            for cadence in (1, 16, 33):
                reset([screen, list_node, first, *nodes], 70000)
                focus = row if selected else first
                retail.run(0x22E957, argv=[intro_type],
                    registers=[(UC_X86_REG_ECX, screen)])
                initial = animation(row)
                points, handoff = [], None
                for ms in range(0, 1001, cadence):
                    update(screen, 70000 + ms)
                    a, child = animation(row), animation(emblem)
                    if handoff is None and a["type"] == 4:
                        handoff = ms
                    if ms <= 2 * cadence or ms >= 1000 - cadence or (handoff is not None
                        and handoff <= ms <= handoff + 240):
                        points.append(dict(elapsed=ms, row=a, descendant=child))
                assert handoff is not None
                assert animation(row)["target"] == ("focus_in" if selected else "idle")
                timelines.append(dict(screen=screen_id, kind="screen_intro_handoff",
                    intro_type=intro_type, selected=selected, cadence=cadence,
                    initial=initial, handoff_elapsed=handoff, points=points))

report = dict(scope=__doc__, xbe_sha256=PIN,
    mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(), screens=screens,
    samples=samples, controls=controls, timelines=timelines,
    static_reentry_evidence=dict(executed=False,
        screen193_vtable=f"{0x45B0B8:08x}",
        screen193_vtable_hex=xbe.read(0x45B0B8, 0x70).hex(),
        screen193_restore_slot25=f"{struct.unpack('<I', xbe.read(0x45B0B8 + 25 * 4, 4))[0]:08x}",
        queued_back=f"{0x23553F:08x}", screen193_loader=f"{0x2B19DC:08x}"),
    boundaries=["148350 supplies relocated literal wigl residency;14837A/24BD3B execute unchanged",
        "24BBF3 supplies exact authored current list definition; full screen construction excluded",
        "148D91/148DFC supply window current-focus storage;22ECB4/22ED7A execute unchanged",
        "16BBC0/16BCC0 supply six ordered occupied datums only for scrolling control; actual24C1C5 executes",
        "Row v3 dynamic record/text/bitmap refresh excluded; recursive22E391/22E3CD execute",
        "Rows/base children constructed by22E27B and linked by22EB18; control hierarchy is explicit",
        "236299 sound sink supplied; no original Xbox mouse-hover authority claimed",
        "No source/native graphics, framebuffer equality, row scrolling or channel reconstruction executed"],
    complete_framebuffer_parity=False)
OUT.mkdir(parents=True, exist_ok=True)
path = OUT / "profile-focus-timing-original.json"
path.write_text(json.dumps(report, indent=2) + "\n")
(OUT / "profile-focus-timing-original.tsv").write_text("\n".join(
    "sample " + " ".join(map(str, [s["screen"], s["kind"], s["elapsed"],
        s["animation"]["frame"], s["animation"]["start_time"],
        s["animation"]["end_time"], s["animation"]["alpha_word"],
        *s["animation"]["position_words"]])) for s in samples) + "\n")
decoder = Cs(CS_ARCH_X86, CS_MODE_32)
corridors = ((0x24AC80, 0x8B), (0x24AD0B, 0xCD), (0x22F0FF, 0x19),
    (0x22E89C, 0x94), (0x22E930, 0x27), (0x22E391, 0x23),
    (0x22E3CD, 0x291), (0x22E957, 0x53), (0x22E65E, 0x23E),
    (0x22ECB4, 0x104), (0x24BD3B, 0x26), (0x24C024, 0x77),
    (0x24C1C5, 0x233), (0x234EBC, 0xD0), (0x23553F, 0x7E),
    (0x2B19DC, 0x40), (0x24A80D, 0x1B))
(OUT / "profile-focus-timing-original.asm").write_text("\n\n".join(
    "\n".join(f"{i.address:08x}: {i.mnemonic} {i.op_str}"
        for i in decoder.disasm(xbe.read(a, n), a)) for a, n in corridors) + "\n")
print(f"{len(samples)} original focus samples, {len(controls)} controls, {len(timelines)} timelines passed")
print("fixture SHA256", hashlib.sha256(path.read_bytes()).hexdigest())
