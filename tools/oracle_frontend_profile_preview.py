#!/usr/bin/env python3
"""Freeze screen193's focused-profile model visibility in the pinned retail XBE.

24A886, 24A855, 24A828, 24A763, 24A7EC and 2B10A3 execute unchanged.
The record-pool lookup, list-array virtual, child lookup, unrelated base/list/text
updates and appearance-color setters are supplied boundaries. The latter setters
are counted but their color behavior is deliberately outside this fixture.
"""
import hashlib
import json
import struct

from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
                               UC_X86_REG_EDI, UC_X86_REG_ESI,
                               UC_X86_REG_ESP)

OUT = ROOT / "analysis/frontend/profile-regression"
original = Retail()
guest = original.g
screen = original.alloc(bytes(0x1A00))
list_widget = screen + 0x614
vtable = original.alloc(bytes(0x60))
original.w(list_widget, vtable)
original.w(vtable + 0x58, 0x3000100)
pool = original.alloc(bytes(0x40))
original.w(list_widget + 0x70, pool)
datum = original.alloc(bytes(8))
entries = original.alloc(bytes(0x248 * 3))
for index, species in enumerate((0, 1, 2)):
    entry = entries + index * 0x248
    original.w(entry, 7 + index)
    guest.mem_write(entry + 0x11C + 4, bytes([species]))
models = [original.alloc(bytes(0x100)) for _ in range(2)]
calls = []
executed = {}
FUNCTIONS = (0x24A886, 0x24A855, 0x24A828, 0x24A763, 0x24A7EC, 0x2B10A3)


def supplied_return(value=0, arguments=0):
    stack = guest.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    guest.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    if address in FUNCTIONS:
        executed[address] = executed.get(address, 0) + 1
    if address == 0x22E391:  # Unrelated base-widget update.
        supplied_return()
    elif address == 0x24C447:  # Supplied focused datum handle.
        assert guest.reg_read(UC_X86_REG_EAX) == list_widget
        supplied_return(0x10000)
    elif address == 0x16BAE0:  # Supplied pool data; real datum index is consumed.
        assert guest.reg_read(UC_X86_REG_ESI) == pool
        supplied_return(datum)
    elif address == 0x3000100:  # Virtual list array getter.
        assert guest.reg_read(UC_X86_REG_ECX) == list_widget
        stack = guest.reg_read(UC_X86_REG_ESP)
        original.w(original.r(stack + 4), 3)
        supplied_return(entries, 4)
    elif address == 0x22F042:  # Other profile/list appearance update.
        supplied_return(arguments=8)
    elif address == 0x22EE47:  # Unrelated text/bitmap visibility.
        supplied_return(arguments=12)
    elif address == 0x22EDB8:  # Supplied model children, absent optional text.
        stack = guest.reg_read(UC_X86_REG_ESP)
        kind, index, recursive = original.read(stack + 4, "<3I")
        assert guest.reg_read(UC_X86_REG_ECX) == screen and recursive == 0
        supplied_return(models[index] if kind == 7 else 0, 12)
    elif address == 0x24A7EC:
        stack = guest.reg_read(UC_X86_REG_ESP)
        widget = guest.reg_read(UC_X86_REG_ECX)
        appearance = guest.reg_read(UC_X86_REG_EAX)
        calls.append(dict(type=original.r(stack + 4),
                          hide=bool(original.r(stack + 8) & 255),
                          widget=models.index(widget),
                          species=original.read(appearance + 4, "<b")[0]))
    elif address == 0x7F790:  # Color calculation excluded, but not visibility.
        supplied_return(arguments=8)
    elif address == 0x2B102F:  # Color/appearance setters excluded.
        supplied_return(arguments=4)
    elif address == 0x2B0F76:
        supplied_return(arguments=12)


guest.hook_add(UC_HOOK_CODE, hook)
xbe = Xbe(ROOT / "assets/disc/default.xbe")
screen_update = struct.unpack("<I", xbe.read(0x45B0B8 + 0xC, 4))[0]
assert screen_update == 0x24A886
cases = []
for label, focused_index, expected in (
        ("focused_spartan", 7, [True, False]),
        ("focused_elite", 8, [False, True]),
        ("create_new_after_elite", -1, [False, False]),
        ("focused_spartan_again", 7, [True, False]),
        ("focused_unknown_species", 9, [False, False]),
        ("create_new_after_unknown", -1, [False, False])):
    original.w(datum + 4, focused_index)
    for model in models:
        guest.mem_write(model + 0x6E, b"\1")
    calls.clear()
    executed.clear()
    original.run(screen_update, registers=[(UC_X86_REG_ECX, screen)])
    visibility = [bool(original.read(model + 0x6E, "<B")[0]) for model in models]
    assert visibility == expected
    assert [call["type"] for call in calls] == [0, 1]
    assert [call["widget"] for call in calls] == [0, 1]
    assert all(call["hide"] == (focused_index == -1) for call in calls)
    assert all(executed.get(function) == 1 for function in FUNCTIONS[:4])
    assert executed.get(0x24A7EC) == 2
    assert executed.get(0x2B10A3, 0) == (0 if focused_index == -1 else 2)
    cases.append(dict(label=label, focused_index=focused_index,
                      visibility=visibility, model_calls=calls.copy(),
                      executed={f"{function:08x}": executed.get(function, 0)
                                for function in FUNCTIONS},
                      cached_species=original.read(screen + 0x176C + 0x120, "<b")[0]))

fixture = dict(original_xbe_sha256=PIN, screen=193,
               original_screen_vtable="0045b0b8", update="0024a886",
               unchanged_functions=["0024a886", "0024a855", "0024a828",
                                    "0024a763", "0024a7ec", "002b10a3"],
               boundaries=__doc__.splitlines()[2:], cases=cases)
OUT.mkdir(parents=True, exist_ok=True)
path = OUT / "profile-preview-original.json"
path.write_text(json.dumps(fixture, indent=2) + "\n")
print(f"{len(cases)} original focused-profile preview cases passed")
print(f"fixture SHA-256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
