#!/usr/bin/env python3
"""Freeze original control admission, row labels/writers and193 focus selectors.

Unchanged XBE constructors2C602E/2C647D execute through their allocation loops;
1A480D executes before the supplied pool allocator. Unchanged row label,
acceptance writer and initial-focus functions execute with supplied child,
save, channel and select-item services.193 executes24A886 and actual focused
record lookups; only its two control text children are present. Text rendering,
pool implementation, save persistence, progression and GPU output are excluded.
"""
import hashlib
import json
import struct

from oracle_multiplayer_hud_positions import Retail, ROOT, PIN, Xbe
from inventory import read_rows
from disasm import listing
from xbe import FUNCTIONS_CSV
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EDX,
                               UC_X86_REG_ESI, UC_X86_REG_ESP)

OUT = ROOT / "analysis/frontend/profile-row-appearance"
original = Retail()
g = original.g
list_widget = original.alloc(bytes(0x300))
item_widget = original.alloc(bytes(0x100))
text_widgets = [original.alloc(bytes(0x100)) for _ in range(2)]
pool = original.alloc(bytes(0x40))
vtable = original.alloc(bytes(0x80))
original.w(list_widget, vtable)
original.w(vtable + 0x2C, 0x3000100)
original.w(vtable + 0x30, 0x3000110)
original.w(vtable + 0x58, 0x3000120)
controller = original.alloc(bytes(8))
selected_item = original.alloc(bytes(4))
screen = original.alloc(bytes(0x1A00))
profile_list = screen + 0x614
original.w(profile_list, vtable)
original.w(profile_list + 0x70, pool)
datum = original.alloc(bytes(8))
records = original.alloc(bytes(0x248 * 2))
for i, name in enumerate(("Controls A", "Controls B")):
    p = records + i * 0x248
    original.w(p, 7 + i)
    g.mem_write(p + 0xC, (name + "\0").encode("utf-16le"))
    g.mem_write(p + 0x11C, bytes([10, 0, 0, 10, 0, 0, 0, 0, 0]))

phase = None
calls = []
executed = {}
FUNCTIONS = (0x2C602E, 0x2C647D, 0x1A480D, 0x2C60C3, 0x2C6512,
             0x2C6118, 0x2C6567, 0x2B434C, 0x2B463D,
             0x24A886, 0x24A855, 0x24A828, 0x24A763, 0x24A7EC)


def supplied_return(value=0, arguments=0):
    stack = g.reg_read(UC_X86_REG_ESP)
    original.ret(value)
    g.reg_write(UC_X86_REG_ESP, stack + 4 + arguments)


def hook(_machine, address, _size, _context):
    if address in FUNCTIONS:
        executed[address] = executed.get(address, 0) + 1
    stack = g.reg_read(UC_X86_REG_ESP)
    if address == 0x24BB00:
        assert phase == "constructor" and g.reg_read(UC_X86_REG_EAX) == list_widget
        supplied_return(list_widget, 4)
    elif address == 0x20670:
        assert phase == "constructor"
        supplied_return(arguments=16)
    elif address == 0x16B570:
        assert phase == "constructor"
        name, item_size, flags = original.read(stack + 4, "<3I")
        maximum_count = g.reg_read(UC_X86_REG_EAX)
        assert item_size == 4 and flags == 0
        label = bytes(g.mem_read(name, 64)).split(b"\0")[0].decode()
        calls.append(dict(kind="pool", name=label, maximum_count=maximum_count,
                          item_size=item_size, flags=flags))
        original.w(pool + 0x20, maximum_count)
        supplied_return(pool, 12)
    elif address == 0x16B790:
        assert phase == "constructor" and g.reg_read(UC_X86_REG_EAX) == pool
        supplied_return()
    elif address == 0x16B840:
        assert phase == "constructor"
        assert g.reg_read(UC_X86_REG_EDX) == pool
        calls.append(dict(kind="allocate", pool="supplied_pool"))
        supplied_return(0x10000)
    elif address == 0x22F4CD:
        assert phase == "constructor"
        supplied_return()
    elif address == 0x22EDB8:
        kind, index, recursive = original.read(stack + 4, "<3I")
        assert recursive == 0
        receiver = g.reg_read(UC_X86_REG_ECX)
        if phase == "row":
            assert receiver == item_widget and (kind, index) == (6, 0)
            result = text_widgets[0]
        else:
            assert phase == "details" and receiver == screen
            result = text_widgets[index - 4] if kind == 6 and index in (4, 5) else 0
        supplied_return(result, 12)
    elif address == 0x253B1A:
        widget = g.reg_read(UC_X86_REG_EDI)
        assert widget in text_widgets
        calls.append(dict(kind="text", child=4 + text_widgets.index(widget),
                          string=f"{original.r(stack+4):08x}"))
        supplied_return(arguments=4)
    elif address == 0x148B8C:
        assert phase == "writer"
        calls.append(dict(kind="save"))
        supplied_return()
    elif address in (0x3000100, 0x3000110):
        assert phase == "writer" and g.reg_read(UC_X86_REG_ECX) == list_widget
        supplied_return(4 if address == 0x3000100 else 1)
    elif address == 0x14800C:
        assert phase == "writer"
        calls.append(dict(kind="close", channel=g.reg_read(UC_X86_REG_EAX),
                          screen=g.reg_read(UC_X86_REG_ECX)))
        supplied_return()
    elif address == 0x24C591:
        assert phase == "initial"
        assert g.reg_read(UC_X86_REG_EAX) == profile_list
        calls.append(dict(kind="select", item=original.r(stack + 4)))
        supplied_return(arguments=4)
    elif address == 0x22F672:
        assert phase == "initial"
        supplied_return()
    elif address == 0x22E391:
        assert phase == "details"
        supplied_return()
    elif address == 0x24C447:
        assert phase == "details" and g.reg_read(UC_X86_REG_EAX) == profile_list
        supplied_return(0x10000)
    elif address == 0x16BAE0:
        assert phase == "details" and g.reg_read(UC_X86_REG_ESI) == pool
        supplied_return(datum)
    elif address == 0x3000120:
        assert phase == "details" and g.reg_read(UC_X86_REG_ECX) == profile_list
        original.w(original.r(stack + 4), 2)
        supplied_return(records, 4)
    elif address == 0x22F042:
        assert phase == "details"
        supplied_return(arguments=8)
    elif address == 0x22EE47:
        assert phase == "details"
        supplied_return(arguments=12)


g.hook_add(UC_HOOK_CODE, hook)
specs = [(41, "thumbsticks", 0x2C602E, 0x2C60C3, 0x2C6118,
          0x2B434C, 0x54E6D9, [0x07000001, 0x080002F5, 0x060002F6, 0x0F0002F7]),
         (40, "buttons", 0x2C647D, 0x2C6512, 0x2C6567,
          0x2B463D, 0x54E6D8, [0x07000001, 0x090003E4, 0x050003E5, 0x0B0003E6])]
constructors, rows, writers, initial = [], [], [], []
for screen_id, kind, ctor, row, writer, initial_focus, field, labels in specs:
    phase = "constructor"
    calls.clear()
    executed.clear()
    original.run(ctor, argv=[1], registers=[(UC_X86_REG_ESI, list_widget)])
    assert executed.get(ctor) == 1 and executed.get(0x1A480D) == 1
    assert calls[0]["kind"] == "pool" and calls[0]["maximum_count"] == 4
    assert len([call for call in calls if call["kind"] == "allocate"]) == 4
    constructors.append(dict(screen=screen_id, address=f"{ctor:08x}", calls=calls.copy()))
    original.w(list_widget, vtable)
    for index in (-1, 0, 1, 2, 3, 4, 255):
        phase = "row"
        calls.clear()
        executed.clear()
        g.mem_write(item_widget + 0x70, struct.pack("<h", index))
        original.run(row, argv=[item_widget, 0], registers=[(UC_X86_REG_ECX, list_widget)])
        assert executed.get(row) == 1
        expected = labels[index] if 0 <= index < 4 else 0xFFFFFFFF
        assert calls == [dict(kind="text", child=4, string=f"{expected:08x}")]
        rows.append(dict(screen=screen_id, item=index, string=calls[0]["string"]))
        for save_now in (False, True):
            phase = "writer"
            calls.clear()
            executed.clear()
            g.mem_write(selected_item, struct.pack("<h", index))
            g.mem_write(list_widget + 0x288, bytes([save_now]))
            g.mem_write(field, b"\xfe")
            original.run(writer, argv=[controller, selected_item],
                         registers=[(UC_X86_REG_ECX, list_widget)])
            assert executed.get(writer) == 1
            stored = original.read(field, "<B")[0]
            assert stored == (index if 0 <= index < 4 else 0)
            assert calls == ([dict(kind="save")] if save_now else []) + [
                dict(kind="close", channel=1, screen=4)]
            writers.append(dict(screen=screen_id, item=index, immediate_save=save_now,
                                stored=stored, calls=calls.copy()))
        # Initial focus runs on a complete screen; its list begins at+614.
        phase = "initial"
        calls.clear()
        executed.clear()
        g.mem_write(field, bytes([index & 255]))
        original.run(initial_focus, registers=[(UC_X86_REG_ECX, screen)])
        assert executed.get(initial_focus) == 1
        assert calls == [dict(kind="select", item=index if 0 <= index < 4 else 0)]
        initial.append(dict(screen=screen_id, stored=index & 255, selected=calls[0]["item"]))

phase = "details"
details = []
for thumb in (0, 1, 2, 3, 4, 255):
    for button in (0, 1, 2, 3, 4, 255):
        for focused in (7, 8, -1):
            # Other row deliberately has different values, making focused
            # lookup distinct from any global or first-record substitute.
            for i in range(2):
                g.mem_write(records + i*0x248 + 0x104,
                            bytes([(button+i)&255, (thumb+i)&255]))
            original.w(datum + 4, focused)
            calls.clear()
            executed.clear()
            for widget in text_widgets:
                g.mem_write(widget + 0x6E, b"\0")
            original.run(0x24A886, registers=[(UC_X86_REG_ECX, screen)])
            value_thumb = (thumb + (focused-7)) & 255
            value_button = (button + (focused-7)) & 255
            thumb_string = 0x07000001 if focused == -1 else (
                specs[0][-1][value_thumb] if value_thumb < 4 else 0)
            button_string = 0x07000001 if focused == -1 else (
                specs[1][-1][value_button] if value_button < 4 else 0)
            assert calls == [dict(kind="text", child=4, string=f"{thumb_string:08x}"),
                             dict(kind="text", child=5, string=f"{button_string:08x}")]
            assert all(executed.get(at) == 1 for at in (0x24A886,0x24A855,0x24A828,0x24A763))
            assert all(original.read(widget+0x6E,"<B")[0] == 1 for widget in text_widgets)
            details.append(dict(focused=focused, first_thumb=thumb, first_button=button,
                                calls=calls.copy(), visible=[True,True]))

OUT.mkdir(parents=True, exist_ok=True)
xbe = Xbe(ROOT / "assets/disc/default.xbe")
inventory = read_rows(FUNCTIONS_CSV)
assembly = []
for address in FUNCTIONS:
    row = inventory[address]
    assembly.append(f"{address:08x} size {row['size']}")
    assembly.extend(listing(xbe.read(address,int(row["size"])),address,inventory))
(OUT / "profile-controls-original.asm").write_text("\n".join(assembly)+"\n")
fixture = dict(scope=__doc__, original_xbe_sha256=PIN, constructors=constructors,
               row_labels=rows, writers=writers, initial_focus=initial,
               focused_details=details, framebuffer_parity=False)
path = OUT / "profile-controls-original.json"
path.write_text(json.dumps(fixture,indent=2)+"\n")
lines = ["# Original XBE "+PIN]
for case in constructors:
    lines.append(f"count\t{case['screen']}\t{case['calls'][0]['maximum_count']}")
for case in rows:
    lines.append(f"row\t{case['screen']}\t{case['item']}\t{case['string']}")
for case in writers:
    lines.append(f"write\t{case['screen']}\t{case['item']}\t{int(case['immediate_save'])}\t{case['stored']}")
for case in initial:
    lines.append(f"initial\t{case['screen']}\t{case['stored']}\t{case['selected']}")
for case in details:
    lines.append("details\t"+"\t".join(str(case[key]) for key in ("focused","first_thumb","first_button"))+
                 "\t"+"\t".join(call["string"] for call in case["calls"]))
(OUT / "profile-controls-original.tsv").write_text("\n".join(lines)+"\n")
print(f"{len(constructors)} constructors, {len(rows)} row labels, {len(writers)} writers, "
      f"{len(initial)} initial selections and {len(details)} focused193 selectors passed")
print("fixture SHA-256 "+hashlib.sha256(path.read_bytes()).hexdigest())
