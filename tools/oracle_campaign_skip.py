#!/usr/bin/env python3
"""Original Cairo skip wrappers, unchanged HS interpreter and save controller.

Storage I/O, header compatibility, BSP admission and unrelated module/UI
callbacks are supplied. The saved game arena contains real compiled nodes,
script frames and the game clock. No native VM or native save implementation
is used as the expected-value authority.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

XBE_PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
MAP_PIN = '5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0'
x = Xbe(ROOT / 'assets/disc/default.xbe')
assert hashlib.sha256(x.data).hexdigest() == XBE_PIN
cache = (ROOT / 'assets/disc/maps/01b_spacestation.map').read_bytes()
assert hashlib.sha256(cache).hexdigest() == MAP_PIN

BASE, SIZE = 0x1000000, 0x3fe000
POOL, THREAD, NPOOL, NODES = BASE + 0x2000, BASE + 0x3000, BASE + 0x10000, BASE + 0x20000
SCENARIO, SCRIPTS, TIME, OPTIONS, IFACE = BASE + 0x100000, BASE + 0x110000, BASE + 0x120000, BASE + 0x121000, BASE + 0x124000
HUD, USERS, ACTION, UNIT = BASE + 0x125000, BASE + 0x126000, BASE + 0x127000, BASE + 0x128000
STOP, STACK = 0x3000000, 0x200ff00
g = Uc(UC_ARCH_X86, UC_MODE_32)
g.mem_map(0x10000, 0x600000)
for section in x.sections:
    g.mem_write(section.va, x.section_bytes(section))
g.mem_map(BASE, 0x400000)
g.mem_map(0x2000000, 0x10000)
g.mem_map(STOP, 0x1000)

def w(at, value):
    g.mem_write(at, struct.pack('<I', value & 0xffffffff))

def r(at):
    return struct.unpack('<I', g.mem_read(at, 4))[0]

def b(at, value):
    g.mem_write(at, bytes([value]))

def call(address, args=(), registers=(), stop=STOP):
    w(STACK, STOP)
    for i, arg in enumerate(args):
        w(STACK + 4 + i * 4, arg)
    g.reg_write(UC_X86_REG_ESP, STACK)
    g.reg_write(UC_X86_REG_FPCW, 0x027f)
    g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    for reg, value in registers:
        g.reg_write(reg, value)
    g.emu_start(address, stop, count=400000)
    assert g.reg_read(UC_X86_REG_EIP) == stop, hex(g.reg_read(UC_X86_REG_EIP))

def thread_address(index):
    return THREAD + index * 0x418

def start_thread(index, script):
    at = thread_address(index)
    g.mem_write(at, bytes(0x418))
    w(at + 4, script)
    w(at + 0x10, at + 0x18)

def run_thread(index):
    call(0x209850, registers=[(UC_X86_REG_EDI, index)])

def thread_state(index):
    at = thread_address(index)
    frame = r(at + 0x10)
    expressions = []
    while frame != at + 0x18:
        datum = r(frame + 4)
        expressions.append(hex(datum))
        frame = r(frame)
        assert len(expressions) < 64
    return dict(wake=r(at + 8), expression_stack=expressions,
                return_value=r(at + 0x26), finished=not expressions)

events = []
snapshot = None
skip_cache_blocked = False
checkpoint_writes = 0
checkpoint_reads = 0
module_callbacks = set(struct.unpack('<30I', g.mem_read(0x46e320, 120)))
module_callbacks -= {0x124620, 0x226030}

def supplied_return(uc, pop_bytes=0, result=None):
    sp = uc.reg_read(UC_X86_REG_ESP)
    ret = r(sp)
    uc.reg_write(UC_X86_REG_ESP, sp + 4 + pop_bytes)
    if result is not None:
        uc.reg_write(UC_X86_REG_EAX, result)
    uc.reg_write(UC_X86_REG_EIP, ret)

def hook(uc, address, size, _):
    global snapshot, checkpoint_writes, checkpoint_reads
    if address in [0x2089b0, 0x2a9d10, 0x2a9d40, 0x2ac3a0, 0x2a9780, 0x2a97a0, 0x2a9a10, 0x124620]:
        events.append(dict(at=f'{address:08x}', tick=r(TIME + 8)))
    if address in module_callbacks:
        supplied_return(uc, 4)
    elif address == 0x2151f0:
        checkpoint_writes += 1
        snapshot = bytes(uc.mem_read(BASE, SIZE))
        slot = uc.reg_read(UC_X86_REG_EAX) & 0xffff
        b(0x5020f2 + slot, 1)
        supplied_return(uc)
    elif address == 0x2152e0:
        assert snapshot is not None
        checkpoint_reads += 1
        uc.mem_write(BASE, snapshot)
        supplied_return(uc, 4, 1)
    elif address == 0x24cdaf:
        supplied_return(uc)
    elif address == 0x124520:
        supplied_return(uc, result=1)
    elif address == 0x1384a0:
        supplied_return(uc, 8, 1)
    elif address == 0x11c1b0:
        supplied_return(uc, 8, 1)
    elif address == 0x124590:
        supplied_return(uc, 4, 1)
    elif address == 0x163b60:
        supplied_return(uc, result=int(skip_cache_blocked))

g.hook_add(UC_HOOK_CODE, hook)

def initialize():
    global snapshot, checkpoint_writes, checkpoint_reads, skip_cache_blocked
    g.mem_write(BASE, bytes(SIZE))
    w(0x4f9384, POOL); w(POOL + 0x44, THREAD)
    w(0x4f9394, NPOOL); w(NPOOL + 0x44, NODES)
    w(0x4e0350, SCENARIO); w(SCENARIO + 0x1bc, SCRIPTS)
    w(0x510c54, TIME); w(TIME + 8, 100)
    g.mem_write(TIME + 2, struct.pack('<h', 30))
    g.mem_write(TIME + 4, struct.pack('<f', 1 / 30))
    w(0x4e6948, OPTIONS); b(OPTIONS + 0x1120, 1); w(OPTIONS + 8, 1)
    w(0x510c50, IFACE); b(IFACE + 5, 1)
    w(0x510c94, HUD); w(HUD + 0x394, -1); w(HUD + 0x3fc, -1)
    w(0x4e8c20, USERS)
    for i in range(4): w(USERS + 0xc + i * 4, -1)
    b(0x4f9388, 1); w(0x4e6090, -1); w(0x4e6094, BASE)
    w(0x4e6080, BASE); b(0x4e3b62, 0); b(0x4e3b63, 0)
    g.mem_write(0x4e3b64, b'\0\0'); w(0x5020e4, 0)
    g.mem_write(0x547f6e, bytes(0xc)); w(0x4701ec, 0)
    w(0x4701f0, 0); w(0x4701f4, 0); w(0x4701f8, 0)
    g.mem_write(SCRIPTS, cache[0xe6582d8:0xe6582d8 + 418 * 40])
    g.mem_write(NODES, cache[0xe65ebb8:0xe65ebb8 + 27091 * 20])
    start_thread(0, 7)
    snapshot = None; checkpoint_writes = 0; checkpoint_reads = 0
    skip_cache_blocked = False
    events.clear()

reports = []
for route in ['natural', 'input']:
    initialize()
    run_thread(0)
    initial = thread_state(0)
    assert initial['wake'] == 101 and not initial['finished']
    assert not any(e['at'] == '002a9d10' for e in events)
    assert r(0x4701ec) == 4
    call(0x226050)
    assert r(0x4701ec) == 0 and g.mem_read(0x547f70, 1) == b'\1'
    # Exact clock increment at the end of the original ordinary simulation tick.
    call(0x1380f3, stop=0x1380fb)
    assert r(TIME + 8) == 101
    # Original main-loop save service, with original123E20 and supplied storage.
    call(0x12bb20)
    saved = thread_state(0)
    assert snapshot is not None and saved == initial
    assert g.mem_read(0x547f70, 1) == b'\0'
    run_thread(0)
    first_completion = thread_state(0)
    assert first_completion['finished'] and first_completion['return_value'] == 1
    w(TIME + 8, 200)  # supplied elapsed cinematic; its world side effects are excluded
    if route == 'natural':
        start_thread(1, 8)
        run_thread(1)
        assert g.mem_read(0x547f6f, 1) == b'\1'
    else:
        # Valid/live unit and admitted player action are supplied before this fragment.
        b(UNIT + 0x134, 1); b(ACTION + 0x1d, 1)
        call(0x14d715, registers=[(UC_X86_REG_EDI, UNIT), (UC_X86_REG_ESI, ACTION)], stop=0x14d760)
        assert g.mem_read(0x547f72, 1) == b'\1'
        call(0x12bad0)
        assert g.mem_read(0x547f6f, 1) == b'\1'
    before_restore = dict(tick=r(TIME + 8), flags=list(g.mem_read(0x547f6f, 7)))
    call(0x12ba30)
    assert r(TIME + 8) == 101 and r(0x4e6090) == 101
    restored = thread_state(0)
    assert restored == saved
    run_thread(0)
    restored_completion = thread_state(0)
    assert restored_completion['finished'] and restored_completion['return_value'] == 0
    start_thread(1, 8)
    run_thread(1)
    assert g.mem_read(0x547f6f, 1) == b'\0'
    assert g.mem_read(IFACE + 6, 1) == b'\0'
    reports.append(dict(route=route, initial=initial, saved=saved,
                        first_completion=first_completion, before_restore=before_restore,
                        restored=restored, restored_completion=restored_completion,
                        restored_tick=r(TIME + 8), reverted_timestamp=r(0x4e6090),
                        checkpoint_writes=checkpoint_writes, checkpoint_reads=checkpoint_reads,
                        events=list(events)))

report = dict(executable_sha256=XBE_PIN, map_sha256=MAP_PIN, fpcw='027f', mxcsr='1f80',
              scope=__doc__.strip(), cases=reports)
out = ROOT / 'analysis/campaign/skip-research-fixture.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
