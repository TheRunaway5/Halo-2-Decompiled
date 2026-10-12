#!/usr/bin/env python3
"""Unchanged text draw admission/animation clock operations, CPU only."""
import hashlib
import json
from pathlib import Path
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from xbe import Xbe

out = ROOT / 'analysis/frontend/settings-typewriter-original'
retail = Retail()
guest = retail.g
xbe = Xbe(ROOT / 'assets/disc/default.xbe')
widget = retail.alloc(bytes(0x100))
definition = retail.alloc(bytes(0x40))
visited = {}
visits = 0
def observe(machine, pc, size, context):
    global visits
    visits += 1
    if pc not in visited:
        raw = bytes(machine.mem_read(pc, size))
        assert raw == xbe.read(pc, size)
        visited[pc] = raw
guest.hook_add(UC_HOOK_CODE, observe)
cases = []
for label, end, clock, visible, enabled in (
        ('before-delay', 350, 349, 1, 1),
        ('at-delay', 350, 350, 1, 1),
        ('after-delay', 350, 351, 1, 1),
        ('hidden', 350, 351, 0, 1),
        ('disabled', 350, 351, 1, 0),
        ('unsigned-high-end', 0xffffffff, 0, 1, 1),
        ('unsigned-high-clock', 0, 0xffffffff, 1, 1),
        ('equal-wrap', 0xffffffff, 0xffffffff, 1, 1)):
    retail.w(widget + 0x50, end)
    guest.mem_write(widget + 0x6d, bytes([visible, enabled]))
    retail.w(0x54d5b8, clock)
    admitted = bool(retail.run(0x22e3b4, registers=[(UC_X86_REG_ECX, widget)]))
    cases.append(dict(label=label, end=end, clock=clock, value6d=visible, value6e=enabled,
        admitted=admitted))
assert [case['admitted'] for case in cases] == [False, True, True, False, False, False, True, True]
animation = []
for clock, delay in ((0, 0), (100, 0), (100, 250), (0xfffffff0, 250)):
    retail.w(widget + 0x70, delay)
    duration = retail.run(0x253aee, registers=[(UC_X86_REG_ECX, widget)])
    retail.w(definition + 0x10, duration)
    retail.w(0x54d5b8, clock)
    retail.run(0x22e89c, registers=[(UC_X86_REG_EAX, widget), (UC_X86_REG_ECX, definition)])
    start, end = retail.read(widget + 0x4c, '<II')
    assert start == clock and end == (clock + delay) & 0xffffffff
    animation.append(dict(clock=clock, authored_delay=delay, duration=duration,
        start_time=start, end_time=end, alpha_word=retail.r(widget + 0x64)))
authorities = {}
for name, start, size in (('draw-admission', 0x22e3b4, 25),
        ('set-animation', 0x22e89c, 148), ('build-animation', 0x22e65e, 574),
        ('text-delay', 0x253aee, 4), ('text-draw-caller', 0x2538f5, 382)):
    raw = xbe.read(start, size)
    (out / f'{name}.asm').write_text('\n'.join(
        f'{instruction.address:08X} {instruction.bytes.hex():24} {instruction.mnemonic} {instruction.op_str}'
        for instruction in Cs(CS_ARCH_X86, CS_MODE_32).disasm(raw, start)) + '\n')
    authorities[name] = hashlib.sha256(raw).hexdigest()
report = dict(passed=True, executable_sha256=PIN,
    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    source_clock='g54D5B8; setter reads it at22E89F, draw admission compares it at22E3C3',
    cases=cases, animation=animation, instruction_authorities=authorities,
    visits=visits, unique_instructions=len(visited), patched_instructions=[],
    scope='Unchanged22E3B4 unsigned admission,253AEE delay getter,22E89C animation-clock construction. Original22E65E/2538F5 retained as literal disassembly; full scheduler/widget allocation and GPU are not executed here.')
(out / 'text-admission-original.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('passed', 'visits', 'unique_instructions')}, indent=2))
