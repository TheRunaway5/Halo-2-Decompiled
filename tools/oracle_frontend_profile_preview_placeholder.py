#!/usr/bin/env python3
"""Execute original193 placeholder visibility independently of widget sorting.

The actual22FC08 bitmap loop, factories/base constructors/linkage create the
seven authored direct bitmaps in original order. Actual24A886 focus lookup,
22EE47/22EDB8,24A7EC/2B10A3 and22E3B4 admission execute unchanged. Pane lookup,
nonanimating construction context, allocation/residency, record-pool/list array,
unrelated base/header update and model colors are explicit supplied seams.
No native build, runtime, GPU replay or original live process is used.
"""
import hashlib
import json
import struct

from audit_rendering import Cache
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *

out = ROOT / "analysis/frontend/profile-row-appearance"
cache = Cache(ROOT / "assets/disc/maps/mainmenu.map")
original = Retail()
g = original.g
xbe = Xbe(ROOT / "assets/disc/default.xbe")
screen_tag = next(t for t in cache.tags.values() if t["group"] == "wgit" and
                  cache.u16(cache.tag(t["datum"], "wgit")[1] + 4) == 193)
_, screen_source = cache.tag(screen_tag["datum"], "wgit")
pane_source = cache.block(screen_source + 32, 76)[0]
bitmap_sources = cache.block(pane_source + 36, 56)
assert len(bitmap_sources) == 7
bitmap_definitions = original.alloc(b"".join(cache.data[p:p + 56] for p in bitmap_sources))
pane = original.alloc(cache.data[pane_source:pane_source + 76])
original.w(pane + 40, bitmap_definitions)
metadata = []
for index, source in enumerate(bitmap_sources):
    datum = cache.u32(source + 28)
    tag, tag_source = cache.tag(datum, "bitm")
    header = original.alloc(cache.data[tag_source:tag_source + 76])
    original.w(original.table + (datum & 65535) * 16 + 8, header)
    image_sources = cache.block(tag_source + 68, 116)
    original.w(header + 72, original.alloc(b"".join(
        cache.data[p:p + 116] for p in image_sources)))
    metadata.append(dict(index=index, datum=f"{datum:08x}", name=tag["name"],
                         definition_raw=cache.data[source:source + 56].hex(),
                         images=[dict(width=cache.u16(p + 4), height=cache.u16(p + 6),
                                      format=cache.u16(p + 12)) for p in image_sources]))
assert metadata[6]["datum"] == "e5ee03ce"
assert metadata[6]["name"] == "ui\\screens\\game_shell\\profile_picker\\unknown"

screen = original.alloc(bytes(0x1A00))
list_widget = screen + 0x614
list_vtable = original.alloc(bytes(0x60))
list_array_call = 0x3000100
original.w(list_widget, list_vtable)
original.w(list_vtable + 0x58, list_array_call)
pool = original.alloc(bytes(0x40))
original.w(list_widget + 0x70, pool)
focused_datum = original.alloc(bytes(8))
entries = original.alloc(bytes(0x248 * 3))
for index, species in enumerate((0, 1, 2)):
    record = entries + index * 0x248
    original.w(record, 7 + index)
    g.mem_write(record + 0x11C + 4, bytes([species]))
calls, visibility_requests, executed = [], [], {}
watched = (0x22FC08, 0x22FA30, 0x2B01EB, 0x22E27B, 0x22EB18,
           0x24A886, 0x24A855, 0x24A828, 0x24A763, 0x22EE47,
           0x22EDB8, 0x24A7EC, 0x2B10A3, 0x22E3B4)


def supplied_return(value=0, arguments=0):
    stack = g.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    g.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    stack = g.reg_read(UC_X86_REG_ESP)
    if address in watched:
        executed[address] = executed.get(address, 0) + 1
    if address == 0x22F899:
        assert g.reg_read(UC_X86_REG_EAX) == screen
        supplied_return(pane)
    elif address == 0x22F0FF:
        supplied_return(0)  # Nonanimating construction; selection visibility only.
    elif address == 0x1A47FD:
        supplied_return(original.alloc(bytes(original.r(stack + 4))), 4)
    elif address in (0x23625D, 0x12360):
        supplied_return()  # Bitmap resource residency, not child identity.
    elif address == 0x22E391:
        supplied_return()  # Unrelated base child-animation update.
    elif address == 0x24C447:
        assert g.reg_read(UC_X86_REG_EAX) == list_widget
        supplied_return(0x10000)
    elif address == 0x16BAE0:
        assert g.reg_read(UC_X86_REG_ESI) == pool
        supplied_return(focused_datum)
    elif address == list_array_call:
        original.w(original.r(stack + 4), 3)
        supplied_return(entries, 4)
    elif address == 0x22F042:
        supplied_return(arguments=8)  # Separately proved header group consumer.
    elif address == 0x22EE47:
        kind, index, value = original.read(stack + 4, "<3I")
        visibility_requests.append(dict(kind=kind, index=index, visible=bool(value & 255)))
        # Continue through actual child lookup and byte write.
    elif address == 0x24A7EC:
        calls.append(dict(type=original.r(stack + 4),
                          hide=bool(original.r(stack + 8) & 255),
                          species=original.read(g.reg_read(UC_X86_REG_EAX) + 4, "<b")[0]))
    elif address == 0x7F790:
        supplied_return(arguments=8)  # Model color calculation excluded.
    elif address == 0x2B102F:
        supplied_return(arguments=4)
    elif address == 0x2B0F76:
        supplied_return(arguments=12)


g.hook_add(UC_HOOK_CODE, hook)
original.w(0x54D5B8, 1000)
original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, screen), (UC_X86_REG_EAX, 0)])
# Execute only the real bitmap-construction loop. Later unrelated grid/text/
# model allocation is excluded, rather than replacing the seven factories.
original.run(0x22FC08, stop=0x22FC71, argv=[screen])
bitmaps = []
child = original.r(screen + 0x14)
while child:
    assert original.r(child + 4) == 8
    bitmaps.append(child)
    child = original.r(child + 0x18)
assert len(bitmaps) == 7
for index, bitmap in enumerate(bitmaps):
    found = original.run(0x22EDB8, argv=[8, index, 0], registers=[(UC_X86_REG_ECX, screen)])
    assert found == bitmap
    assert original.r(bitmap + 0x70) == bitmap_definitions + index * 56
    assert original.read(bitmap + 0xA, "<h")[0] == index
    metadata[index]["constructed_bounds"] = original.read(bitmap + 0x20, "<4h")
    metadata[index]["runtime_image_index"] = original.read(bitmap + 0x88, "<h")[0]
mapping_calls = {f"{at:08x}": executed.get(at, 0) for at in watched}
assert mapping_calls["0022fa30"] == mapping_calls["002b01eb"] == 7

models = []
for index in range(2):
    model = original.alloc(bytes(0x100))
    original.run(0x22E27B, argv=[0], registers=[(UC_X86_REG_ESI, model), (UC_X86_REG_EAX, 7)])
    original.run(0x22EB18, registers=[(UC_X86_REG_ESI, screen), (UC_X86_REG_EAX, model)])
    models.append(model)
cases = []
for label, focused_index, expected_models in (
        ("create_new_initial", -1, [False, False]),
        ("focused_spartan", 7, [True, False]),
        ("focused_elite", 8, [False, True]),
        ("create_new_after_elite", -1, [False, False]),
        ("focused_spartan_again", 7, [True, False]),
        ("focused_unknown_species", 9, [False, False]),
        ("create_new_after_unknown", -1, [False, False])):
    original.w(focused_datum + 4, focused_index)
    visibility_requests.clear()
    calls.clear()
    executed.clear()
    original.run(0x24A886, registers=[(UC_X86_REG_ECX, screen)])
    bitmap_visibility = [bool(original.read(p + 0x6E, "<B")[0]) for p in bitmaps]
    model_visibility = [bool(original.read(p + 0x6E, "<B")[0]) for p in models]
    admitted = [bool(original.run(0x22E3B4, registers=[(UC_X86_REG_ECX, p)])) for p in bitmaps]
    assert bitmap_visibility == admitted == [True] * 6 + [focused_index == -1]
    assert model_visibility == expected_models
    assert visibility_requests == [dict(kind=6, index=3, visible=False),
                                   dict(kind=8, index=6, visible=focused_index == -1)]
    assert executed.get(0x22EE47) == 2 and executed.get(0x24A7EC) == 2
    assert executed.get(0x2B10A3, 0) == (0 if focused_index == -1 else 2)
    cases.append(dict(label=label, focused_index=focused_index,
                      bitmap_visibility=bitmap_visibility, bitmap_drawing_admitted=admitted,
                      model_visibility=model_visibility, visibility_requests=visibility_requests.copy(),
                      model_calls=calls.copy(), executed={f"{at:08x}": executed.get(at, 0) for at in watched}))

selected_tag = next(t for t in cache.tags.values() if t["group"] == "wgit" and
                    cache.u16(cache.tag(t["datum"], "wgit")[1] + 4) == 194)
_, selected_source = cache.tag(selected_tag["datum"], "wgit")
selected_panes = cache.block(selected_source + 32, 76)
selected_metadata = [dict(bitmap_datums=[f"{cache.u32(p + 28):08x}" for p in cache.block(q + 36, 56)],
                          model_count=cache.u32(q + 44)) for q in selected_panes]
assert all(p["model_count"] == 0 and "e5ee03ce" not in p["bitmap_datums"] for p in selected_metadata)

fixture = dict(scope=__doc__, original_xbe_sha256=PIN, mainmenu_sha256=hashlib.sha256(cache.data).hexdigest(),
               screen=193, screen_datum=f"{screen_tag['datum']:08x}",
               bitmap_constructor_mapping=metadata, mapping_execution=mapping_calls,
               placeholder_runtime_bitmap_index=6, placeholder_datum="e5ee03ce", cases=cases,
               screen194_authored=selected_metadata,
               screen194_scope="194 owns no model or unknown preview bitmap. Its lower193 channel transition/update chronology is not executed by this fixture.")
(out / "profile-preview-placeholder-original.json").write_text(json.dumps(fixture, indent=2) + "\n")
rows = ["# placement bitmap_index top left bottom right runtime_image_index",
        "placement 6 " + " ".join(map(str, metadata[6]["constructed_bounds"] +
                                         [metadata[6]["runtime_image_index"]])),
        "# case label focused_index placeholder_visible spartan_visible elite_visible"]
rows.extend(f"case {c['label']} {c['focused_index']} {int(c['bitmap_visibility'][6])} "
            f"{int(c['model_visibility'][0])} {int(c['model_visibility'][1])}" for c in cases)
(out / "profile-preview-placeholder-original.tsv").write_text("\n".join(rows) + "\n")
dis = Cs(CS_ARCH_X86, CS_MODE_32)
regions = ((0x22FC08, 0x69), (0x22FA30, 0x2B), (0x2B01EB, 0x13D), (0x22EB18, 0x2B),
           (0x22EDB8, 0x5F), (0x22EE47, 0x1D), (0x24AA84, 0x4D), (0x24A7EC, 0x20),
           (0x2B10A3, 0x92), (0x22E3B4, 0x21))
(out / "profile-preview-placeholder-original.asm").write_text("\n\n".join(
    "\n".join(f"{i.address:08x} {i.mnemonic:8s} {i.op_str}" for i in dis.disasm(xbe.read(at, size), at))
    for at, size in regions) + "\n")
print(f"{len(cases)} original placeholder/model cases passed; seven actual authored bitmap factories mapped")
print("JSON SHA256", hashlib.sha256((out / "profile-preview-placeholder-original.json").read_bytes()).hexdigest())
