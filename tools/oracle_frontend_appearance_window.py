#!/usr/bin/env python3
"""Execute original child-list focus selection and scrolling against 18 colors.

The authored choose-color list has four visible item widgets and 18 contiguous
used datums. Only record-pool stepping, focus virtual/predicate, notification,
and sound are supplied seams; original selection and row-assignment code runs.
"""
import hashlib
import json
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
                              UC_X86_REG_EDX, UC_X86_REG_EDI,
                              UC_X86_REG_ESI, UC_X86_REG_ESP)

retail = Retail()
guest = retail.g
listing = retail.alloc(bytes(0x100))
pool = retail.alloc(bytes(0x100))
virtuals = retail.alloc(bytes(0x100))
retail.w(listing, virtuals)
retail.w(listing + 0x70, pool)
retail.w(pool + 0x3c, 18)
retail.w(virtuals + 0x1c, 0x3000100)
retail.w(virtuals + 0x44, 0x22F559)
children = [retail.alloc(bytes(0x100)) for _ in range(4)]
retail.w(listing + 0x14, children[0])
guest.mem_write(listing + 0x7c, bytes([1, 0, 0]))
for index, child in enumerate(children):
    retail.w(child, virtuals)
    retail.w(child + 4, 2)
    retail.w(child + 0x70, index)
    if index < 3: retail.w(child + 0x18, children[index + 1])
    if index: retail.w(child + 0x1c, children[index - 1])
focused = children[0]
executed = {}
tracked = (0x24C515, 0x24C5D0, 0x24C177, 0x24C1A4, 0x24C461,
           0x24C1C5, 0x24BAE6, 0x24C447, 0x24C0C4, 0x24C71E, 0x24C73E)


def ret(value=0, arguments=0):
    stack = guest.reg_read(UC_X86_REG_ESP)
    retail.ret(value)
    guest.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    global focused
    if address in tracked: executed[address] = executed.get(address, 0) + 1
    if address == 0x3000100:
        stack = guest.reg_read(UC_X86_REG_ESP)
        assert guest.reg_read(UC_X86_REG_ECX) == listing
        focused = retail.r(stack + 4)
        assert focused in children
        ret(arguments=4)
    elif address == 0x22ED7A:
        ret(int(guest.reg_read(UC_X86_REG_ESI) == focused))
    elif address == 0x22F559:
        child = guest.reg_read(UC_X86_REG_ECX)
        ret(int(retail.r(child + 0x70) != 0xffffffff))
    elif address == 0x16BBC0:
        assert guest.reg_read(UC_X86_REG_ESI) == pool
        value = guest.reg_read(UC_X86_REG_EAX)
        ret(0 if value == 0xffffffff else value + 1 if value < 17 else -1)
    elif address == 0x16BCC0:
        assert guest.reg_read(UC_X86_REG_EAX) == pool
        value = guest.reg_read(UC_X86_REG_EDX)
        ret(value - 1 if 0 < value <= 17 else -1)
    elif address == 0x16BC40:
        ret(17, 4)
    elif address == 0x24C610:
        ret(arguments=4)
    elif address == 0x236299:
        ret()


guest.hook_add(UC_HOOK_CODE, hook)
cases = []


def capture(operation, argument):
    if operation == "select":
        retail.run(0x24C515, argv=[argument], registers=[(UC_X86_REG_EDI, listing)])
    else:
        retail.run(0x24C1C5, argv=[argument & 255], registers=[(UC_X86_REG_EDI, listing)])
    cases.append(dict(operation=operation, argument=argument,
                      focus_cell=children.index(focused),
                      focused_datum=retail.r(focused + 0x70),
                      cells=[retail.r(child + 0x70) for child in children]))


capture("select", 10)
for step in (-1, -1, -1, -1, -1, 1, 1, 1, 1, 1, 1): capture("move", step)
capture("select", 0)
capture("move", -1)
capture("move", 1)
capture("select", 6)
capture("select", 4)
capture("select", 1)
result = dict(xbe_sha256=PIN, authored_visible=4, used_datums=18,
              supplied_boundaries=["18 contiguous used pool datums from 2CB23F",
                                   "focus virtual and focused-child predicate", "datum validity virtual",
                                   "notification and sound"],
              executed={f"{k:08x}": v for k, v in executed.items()}, cases=cases,
              excluded=["focus transition alpha", "input repeat timing", "GPU"])
path = ROOT / "analysis/frontend/profile-regression/appearance-window-original.json"
path.write_text(json.dumps(result, indent=2) + "\n")
print(f"{len(cases)} original selection/movement cases; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}")
for row in cases: print(row)
