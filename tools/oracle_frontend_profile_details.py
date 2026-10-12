#!/usr/bin/env python3
"""Freeze original193 focused-header payload and explicit static-text hiding.

24A886,24A855,24A828,24A763,24A7EC and2B10A3 execute unchanged. Record-pool,
list-array and child getters are supplied.22F042 payload and22EE47 visibility
requests are recorded before interception; text/group allocation and updating
are excluded. Existing2AFEAE fixtures separately prove the name/color consumer.
"""
import hashlib
import json
import struct

from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP

OUT = ROOT / "analysis/frontend/profile-row-appearance"
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
entries = original.alloc(bytes(0x248 * 2))
profiles = [("Saved Blue", 10, 0), ("Saved Tan", 17, 1)]
for index, (name, primary, species) in enumerate(profiles):
    entry = entries + index * 0x248
    original.w(entry, 7 + index)
    guest.mem_write(entry + 0xC, (name + "\0").encode("utf-16le"))
    guest.mem_write(entry + 0x11C, bytes([primary, 0, 0, 10, species, 0, 0, 0, 0]))
models = [original.alloc(bytes(0x100)) for _ in range(2)]
payloads = []
visibility_calls = []
executed = {}
FUNCTIONS = (0x24A886, 0x24A855, 0x24A828, 0x24A763, 0x24A7EC, 0x2B10A3)

def supplied_return(value=0, arguments=0):
    stack = guest.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    guest.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)

def hook(_machine, address, _size, _context):
    if address in FUNCTIONS:
        executed[address] = executed.get(address, 0) + 1
    if address == 0x22E391:
        supplied_return()
    elif address == 0x24C447:
        assert guest.reg_read(UC_X86_REG_EAX) == list_widget
        supplied_return(0x10000)
    elif address == 0x16BAE0:
        assert guest.reg_read(UC_X86_REG_ESI) == pool
        supplied_return(datum)
    elif address == 0x3000100:
        assert guest.reg_read(UC_X86_REG_ECX) == list_widget
        stack = guest.reg_read(UC_X86_REG_ESP)
        original.w(original.r(stack + 4), len(profiles))
        supplied_return(entries, 4)
    elif address == 0x22F042:
        stack = guest.reg_read(UC_X86_REG_ESP)
        item = guest.reg_read(UC_X86_REG_EAX)
        receiver, count = original.read(stack + 4, "<2I")
        flags = original.r(item)
        name = None
        if flags & 1:
            pointer = original.r(item + 4)
            name = bytes(guest.mem_read(pointer, 32)).decode("utf-16le").split("\0")[0]
        payloads.append(dict(receiver_is_screen=receiver == screen,
                             count=count, flags=flags, name=name,
                             appearance=bytes(guest.mem_read(item + 0x48, 9)).hex() if flags & 2 else None,
                             create_new=bool(original.read(item + 0x5E, "<B")[0]) if flags & 0x20 else None))
        supplied_return(arguments=8)
    elif address == 0x22EE47:
        stack = guest.reg_read(UC_X86_REG_ESP)
        kind, index, visible = original.read(stack + 4, "<3I")
        visibility_calls.append(dict(kind=kind, index=index, visible=bool(visible & 255)))
        supplied_return(arguments=12)
    elif address == 0x22EDB8:
        stack = guest.reg_read(UC_X86_REG_ESP)
        kind, index, recursive = original.read(stack + 4, "<3I")
        assert guest.reg_read(UC_X86_REG_ECX) == screen and recursive == 0
        supplied_return(models[index] if kind == 7 else 0, 12)
    elif address == 0x7F790:
        supplied_return(arguments=8)
    elif address == 0x2B102F:
        supplied_return(arguments=4)
    elif address == 0x2B0F76:
        supplied_return(arguments=12)

guest.hook_add(UC_HOOK_CODE, hook)
xbe = Xbe(ROOT / "assets/disc/default.xbe")
update = struct.unpack("<I", xbe.read(0x45B0B8 + 0xC, 4))[0]
assert update == 0x24A886
cases = []
for label, focus in [("blue", 7), ("tan", 8), ("create_new_after_tan", -1), ("blue_again", 7)]:
    original.w(datum + 4, focus)
    payloads.clear()
    visibility_calls.clear()
    executed.clear()
    original.run(update, registers=[(UC_X86_REG_ECX, screen)])
    assert len(payloads) == 1 and payloads[0]["receiver_is_screen"] and payloads[0]["count"] == 1
    assert visibility_calls == [dict(kind=6, index=3, visible=False), dict(kind=8, index=6, visible=focus == -1)]
    if focus == -1:
        assert payloads[0]["flags"] == 0x20 and payloads[0]["create_new"]
        assert payloads[0]["name"] is None and payloads[0]["appearance"] is None
    else:
        name, primary, species = profiles[focus - 7]
        assert payloads[0]["flags"] == 3 and payloads[0]["name"] == name
        assert payloads[0]["appearance"] == bytes([primary, 0, 0, 10, species, 0, 0, 0, 0]).hex()
    assert all(executed.get(function) == 1 for function in FUNCTIONS[:4])
    assert executed.get(0x24A7EC) == 2
    cases.append(dict(label=label, focused_index=focus, payload=payloads[0].copy(),
                      visibility_requests=visibility_calls.copy(),
                      executed={f"{function:08x}": executed.get(function, 0) for function in FUNCTIONS}))
fixture = dict(scope=__doc__, original_xbe_sha256=PIN, screen=193,
               vtable="0045b0b8", update="0024a886", cases=cases,
               framebuffer_parity=False)
OUT.mkdir(parents=True, exist_ok=True)
path = OUT / "profile-details-original.json"
path.write_text(json.dumps(fixture, indent=2) + "\n")
print(f"{len(cases)} unchanged original focused-header cases passed")
print(f"fixture SHA-256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
