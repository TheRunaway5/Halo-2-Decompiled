#!/usr/bin/env python3
"""Run bounded unchanged-XBE campaign BSP request, commit and placement cases.

Module lifecycle callbacks, streaming, object iterator/protection/admission and
creation/destruction side effects are supplied. The original request/commit,
placement retention/hide decisions, automatic-placement selection, placement
construction, transition-volume search and unattached trigger containment run
unchanged. This is not a complete Havok/AI/renderer transition emulator.
"""
from pathlib import Path
import hashlib
import itertools
import json
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
x = Xbe(ROOT / 'assets/disc/default.xbe')
assert hashlib.sha256(x.data).hexdigest() == PIN
g = Uc(UC_ARCH_X86, UC_MODE_32)
g.mem_map(0x10000, 0x600000)
for section in x.sections:
    g.mem_write(section.va, x.section_bytes(section))
BASE = 0x1000000
g.mem_map(BASE, 0x400000)
STOP, STACK = BASE + 0x3f0000, 0x200ff00
g.mem_map(0x2000000, 0x10000)
SCNR, OPTIONS, TAGS, REFS = BASE + 0x1000, BASE + 0x2000, BASE + 0x4000, BASE + 0x6000
HEADER, POOL, HEADERS, OBJECT, PLACEMENT, PALETTE = [BASE + n for n in [0x8000, 0x10000, 0x11000, 0x12000, 0x14000, 0x15000]]
PLACEMENT_GLOBALS, GC, VOLUMES, PAIRS, IFACE, NAMES, ARGUMENT = [BASE + n for n in [0x16000, 0x17000, 0x18000, 0x19000, 0x1a000, 0x1b000, 0x1c000]]
VM_SENTINEL = BASE + 0x100000
events, creations, destruction = [], [], []
iterator_objects = []
protected = False
load_success = True
mode = ''
snapshot = None

def w(at, value): g.mem_write(at, struct.pack('<I', value & 0xffffffff))
def r(at): return struct.unpack('<I', g.mem_read(at, 4))[0]
def s(at, value): g.mem_write(at, struct.pack('<H', value & 0xffff))
def sr(at): return struct.unpack('<h', g.mem_read(at, 2))[0]
def b(at, value): g.mem_write(at, bytes([value & 0xff]))

def supplied_return(uc, pop=0, result=None):
    sp = uc.reg_read(UC_X86_REG_ESP); ret = r(sp)
    uc.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
    if result is not None: uc.reg_write(UC_X86_REG_EAX, result & 0xffffffff)
    uc.reg_write(UC_X86_REG_EIP, ret)

def ptr(at): return struct.unpack('<I', x.read(at, 4))[0]
pre = [ptr(0x440df0 + i * 36) for i in range(68)]
dispose = [ptr(0x440dec + i * 36) for i in range(68)]
initialize_callbacks = [ptr(0x440de8 + i * 36) for i in range(68)]
module_callbacks = set(pre + dispose + initialize_callbacks) - {0, 0xd4e60, 0xd4e30, 0x123d20}
checkpoint_callbacks = set(struct.unpack('<30I', x.read(0x46e320, 120))) - {0, 0x124620, 0x226030}

def hook(uc, address, size, user_data):
    global events, snapshot
    sp = uc.reg_read(UC_X86_REG_ESP)
    if address in [0xd4e60, 0x137da0, 0x137d40, 0x123d20, 0xd4e30]:
        events.append(dict(at=f'{address:08x}', loaded=sr(0x4686c4), active=sr(OPTIONS + 2)))
    if address in module_callbacks or (mode == 'checkpoint' and address in checkpoint_callbacks):
        ret = r(sp)
        events.append(dict(at=f'{address:08x}', callback_return=f'{ret:08x}'))
        checkpoint_call = (0x123e20 <= ret < 0x123ec0 or 0x199520 <= ret < 0x199570)
        supplied_return(uc, 4 if checkpoint_call else 0)
    elif address == 0x122b40:
        events.append(dict(at='00122b40', reference=uc.reg_read(UC_X86_REG_EAX)))
        supplied_return(uc, result=int(load_success))
    elif address == 0x122bc0:
        events.append(dict(at='00122bc0', reference=uc.reg_read(UC_X86_REG_EDI)))
        supplied_return(uc)
    elif address == 0x16e2c0:
        supplied_return(uc)
    elif address == 0x18efa0:
        events.append(dict(at='0018efa0', supplied_failure_service=True)); supplied_return(uc)
    elif address == 0x1e95d0: supplied_return(uc, 4)
    elif address in [0x1e9650, 0xbf380, 0x18e470]: supplied_return(uc)
    elif address == 0xbaeb0:
        iterator = r(sp + 4)
        previous = r(iterator + 8)
        index = 0 if previous == 0xffffffff else previous + 1
        if index < len(iterator_objects):
            w(iterator + 8, index)
            supplied_return(uc, 4, iterator_objects[index])
        else: supplied_return(uc, 4, 0)
    elif address == 0xbeb30: supplied_return(uc, 4, int(protected))
    elif address == 0xb7300: supplied_return(uc)
    elif address == 0xb87b0: supplied_return(uc, 4)
    elif address == 0xb8600: supplied_return(uc, 8)
    elif address in [0xb83b0, 0xb8460]:
        destruction.append(f'{address:08x}'); supplied_return(uc, 8)
    elif address == 0xbadc0: supplied_return(uc, 4, 0)
    elif address == 0xb7b40:
        data = r(sp + 4)
        creations.append(dict(tag=f'{r(data):08x}', scenario_index=r(data + 0x10),
                              policy=g.mem_read(data + 0x14, 1)[0],
                              supplied_world_creation=True))
        # Supply only the minimal object prefix needed by subsequent original
        # name binding and placement retention, using original constructed data.
        w(OBJECT, r(data)); w(OBJECT + 0x14, -1); s(OBJECT + 0xac, -1)
        w(OBJECT + 0xa8, r(data + 8)); s(OBJECT + 0x1a, r(data + 0x10))
        b(OBJECT + 0xae, r(data + 8) & 0xff)
        b(OBJECT + 0xb0, g.mem_read(data + 0x14, 1)[0])
        g.mem_write(OBJECT + 0x30, bytes(g.mem_read(data + 0x1c, 12)))
        w(OBJECT + 0xd4, -1)
        supplied_return(uc, 4, 0)
    elif address in [0x108b10, 0xa7870]: supplied_return(uc, 4)
    elif address == 0x209d50:
        # Only evaluator ABI case: arguments and admission are supplied.
        assert mode == 'evaluator'
        supplied_return(uc, 16, ARGUMENT)
    elif address == 0x209ae0:
        assert mode == 'evaluator'
        supplied_return(uc)
    elif mode == 'checkpoint' and address == 0x2151f0:
        snapshot = bytes(uc.mem_read(BASE, 0x3fe000))
        slot = uc.reg_read(UC_X86_REG_EAX) & 0xffff
        b(0x5020f2 + slot, 1)
        events.append(dict(at='002151f0', supplied_storage_copy=True,
                           saved_bsp=sr(HEADER + 0x1248)))
        supplied_return(uc)
    elif mode == 'checkpoint' and address == 0x2152e0:
        assert snapshot is not None
        events.append(dict(at='002152e0', supplied_storage_copy=True,
                           resource_bsp_before_arena_copy=sr(0x4686c4)))
        uc.mem_write(BASE, snapshot)
        supplied_return(uc, 4, 1)
    elif mode == 'checkpoint' and address == 0x124520: supplied_return(uc, result=1)
    elif mode == 'checkpoint' and address == 0x1384a0: supplied_return(uc, 8, 1)
    elif mode == 'checkpoint' and address == 0x124590: supplied_return(uc, 4, 1)

g.hook_add(UC_HOOK_CODE, hook)

def call(address, args=(), registers=()):
    w(STACK, STOP)
    for i, value in enumerate(args): w(STACK + 4 + 4 * i, value)
    g.reg_write(UC_X86_REG_ESP, STACK)
    g.reg_write(UC_X86_REG_FPCW, 0x027f); g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
    for reg, value in registers: g.reg_write(reg, value & 0xffffffff)
    try:
        g.emu_start(address, STOP, count=1000000)
    except Exception as error:
        raise RuntimeError(f'original{address:x}: eip={g.reg_read(UC_X86_REG_EIP):x}, '
                           f'esp={g.reg_read(UC_X86_REG_ESP):x}, events={events[-6:]}') from error
    assert g.reg_read(UC_X86_REG_EIP) == STOP, hex(g.reg_read(UC_X86_REG_EIP))
    return g.reg_read(UC_X86_REG_EAX) & 0xff

def reset(loaded=0, active=0):
    global iterator_objects, protected, load_success, mode
    g.mem_write(BASE, bytes(0x200000))
    w(0x4e0350, SCNR); w(SCNR + 0x210, 3); w(SCNR + 0x214, REFS)
    w(0x4e6948, OPTIONS); s(OPTIONS + 2, active); b(OPTIONS + 0xc, 1)
    s(0x4686c4, loaded); w(0x4e3b44, TAGS); w(0x4e6094, HEADER)
    for i in range(3):
        w(REFS + i * 0x44 + 0x14, i); w(REFS + i * 0x44 + 0x1c, -1)
        metadata = BASE + 0x30000 + i * 0x1000
        w(TAGS + i * 16 + 8, metadata); w(metadata + 0x18, metadata + 0x100)
    w(0x4e0300, POOL); w(POOL + 0x44, HEADERS); w(HEADERS + 8, OBJECT)
    w(0x4e0324, PLACEMENT_GLOBALS); s(PLACEMENT_GLOBALS, loaded)
    w(0x4de2f4, GC); w(0x510c50, IFACE); w(0x4de2d0, NAMES)
    g.mem_write(NAMES, b'\xff' * (640 * 4)); s(0x547f78, -1); b(0x547f73, 0)
    w(SCNR + 0x108, 1); w(SCNR + 0x10c, VOLUMES)
    s(VOLUMES + 4, -1)
    g.mem_write(VOLUMES + 0xc, struct.pack('<3f', 1, 0, 0))
    g.mem_write(VOLUMES + 0x18, struct.pack('<3f', 0, 0, 1))
    g.mem_write(VOLUMES + 0x30, struct.pack('<3f', 10, 10, 10))
    w(TAGS + 8, BASE + 0x30000); w(BASE + 0x30000 + 0x38, -1)
    w(VM_SENTINEL, 0x12345678)
    iterator_objects = []; protected = False; load_success = True; mode = ''
    events.clear(); creations.clear(); destruction.clear()

def placement(type=6, name=-1, flags=0, origin=1, source=1, policy=0, mask=0):
    definition = ptr(0x468630 + 4 * type)
    datums_offset, palettes_offset = struct.unpack('<hh', x.read(definition + 0xa, 4))
    w(SCNR + datums_offset, 1); w(SCNR + datums_offset + 4, PLACEMENT)
    w(SCNR + palettes_offset, 1); w(SCNR + palettes_offset + 4, PALETTE)
    w(PALETTE + 4, 0)
    s(PLACEMENT, 0); s(PLACEMENT + 2, name); w(PLACEMENT + 4, flags)
    s(PLACEMENT + 0x26, mask); s(PLACEMENT + 0x2c, origin)
    b(PLACEMENT + 0x2e, type); b(PLACEMENT + 0x2f, source); b(PLACEMENT + 0x30, policy)

def object_state():
    return dict(flags=f'{r(OBJECT + 4):08x}', bsp=g.mem_read(OBJECT + 0xae, 1)[0],
                destroyed_callbacks=list(destruction))

reports = []
for pending, target in itertools.product([0, 1], [-2, -1, 0, 1, 2, 3]):
    reset(loaded=1, active=1); b(0x547f73, pending); s(0x547f78, 2)
    call(0x12b850, registers=[(UC_X86_REG_EAX, target)])
    actual = [g.mem_read(0x547f73, 1)[0], sr(0x547f78)]
    if 0 <= target < 3:
        expected = [0, 1] if target == 1 and pending else ([pending, 2] if target == 1 else [1, target])
    else: expected = [pending, 2]
    assert actual == expected
    reports.append(dict(kind='request', pending=pending, target=target, pending_target=actual))

for opcode, value in itertools.product([508, 509], [0, 1, 2, 3, -1]):
    reset(); mode = 'evaluator'; w(ARGUMENT, value)
    call(0x2a9580, args=[opcode, 7, 1])
    expected = [1, value] if value in [1, 2] else [0, -1]
    actual = [g.mem_read(0x547f73, 1)[0], sr(0x547f78)]
    assert actual == expected
    reports.append(dict(kind='evaluator_supplied_arguments', opcode=opcode,
                        signed_short=value, pending_target=actual))

for loaded, active, target, success in [(0, 0, 1, True), (0, 0, 1, False),
        (1, 1, 1, True), (1, -1, 1, True), (-1, -1, 1, True),
        (0, 0, -1, True), (0, 0, -2, True), (0, 0, 3, True)]:
    reset(loaded, active); load_success = success
    result = call(0x11c1b0, args=[target, 1])
    if target in [-2, 3]: expected = [0, loaded, active]
    elif not success: expected = [0, -1, -1]
    else: expected = [1, target, target]
    actual = [result, sr(0x4686c4), sr(OPTIONS + 2)]
    assert actual == expected, (loaded, active, target, actual, expected)
    assert r(VM_SENTINEL) == 0x12345678
    reports.append(dict(kind='commit', loaded=loaded, active=active, target=target,
                        supplied_streaming_success=success, result_loaded_active=actual,
                        checkpoint_header_bsp=sr(HEADER + 0x1248), events=list(events),
                        vm_sentinel_preserved=True))

for inside, type, named in itertools.product([False, True], [0, 5, 6], [False, True]):
    reset(); iterator_objects = [OBJECT]
    w(OBJECT, 0); w(OBJECT + 0x14, -1); s(OBJECT + 0xac, 13 if named else -1)
    b(OBJECT + 0xaa, type); b(OBJECT + 0xab, 2); b(OBJECT + 0xae, 0); b(OBJECT + 0xb0, 0)
    w(OBJECT + 0xd4, -1)
    g.mem_write(OBJECT + 0x30, struct.pack('<3f', 5 if inside else 0, 5, 5))
    w(SCNR + 0x380, 1); w(SCNR + 0x384, PAIRS)
    g.mem_write(PAIRS, struct.pack('<Ihh', 0x10000, 0, 0))
    call(0xd4e60, args=[1])
    expected_flags = 0 if inside or (not named and type != 0) else 0x8000080
    expected_destroyed = [] if inside or named or type == 0 else ['000b83b0', '000b8460']
    state = object_state()
    assert int(state['flags'], 16) == expected_flags
    assert state['destroyed_callbacks'] == expected_destroyed
    assert state['bsp'] == (1 if inside else 0)
    reports.append(dict(kind='dynamic_lifetime', inside_transition_volume=inside,
                        type=type, named=named, result=state))

for variant in ['missing_pair', 'sentinel_only', 'protected', 'child', 'source_none',
                'hidden_by_bsp', 'hidden_by_other', 'scoped_origin', 'manual_in', 'manual_out']:
    reset(); iterator_objects = [OBJECT]
    w(OBJECT, 0); w(OBJECT + 0x14, -1); s(OBJECT + 0xac, 13)
    b(OBJECT + 0xaa, 6); b(OBJECT + 0xab, 2); b(OBJECT + 0xae, 0); b(OBJECT + 0xb0, 0)
    w(OBJECT + 0xd4, -1); g.mem_write(OBJECT + 0x30, struct.pack('<3f', 5, 5, 5))
    w(SCNR + 0x380, 1); w(SCNR + 0x384, PAIRS)
    g.mem_write(PAIRS, struct.pack('<Ihh', 0x10000, 0, 0))
    expected_flags, expected_bsp = 0, 1
    if variant == 'missing_pair': w(PAIRS, 0x20000)
    if variant == 'sentinel_only': s(PAIRS + 4, -1)
    if variant == 'protected':
        protected = True; g.mem_write(OBJECT + 0x30, struct.pack('<3f', 0, 5, 5))
    if variant == 'child': w(OBJECT + 0x14, 17); expected_bsp = 0
    if variant == 'source_none': b(OBJECT + 0xab, -1); expected_bsp = 0
    if variant == 'hidden_by_bsp': w(OBJECT + 4, 0x8000080)
    if variant == 'hidden_by_other': w(OBJECT + 4, 0x80); expected_flags = 0x80
    if variant in ['scoped_origin', 'manual_in', 'manual_out']:
        b(OBJECT + 0xab, 1); s(OBJECT + 0x1a, 0)
        placement(origin=0, policy=2 if variant.startswith('manual') else 0,
                  mask=2 if variant == 'manual_in' else 1)
        b(OBJECT + 0xb0, 2 if variant.startswith('manual') else 1)
        if variant != 'manual_in': expected_flags, expected_bsp = 0x8000080, 0
    call(0xd4e60, args=[1]); state = object_state()
    assert int(state['flags'], 16) == expected_flags and state['bsp'] == expected_bsp, (variant, state)
    assert not destruction
    reports.append(dict(kind='lifetime_edge', variant=variant, result=state))

for difficulty, flags in itertools.product(range(4), [0, 1, 2, 4, 8, 14, 16, 32, 64]):
    reset(loaded=1, active=1); w(OPTIONS + 8, 1); s(OPTIONS + 0x132, difficulty)
    placement(flags=flags)
    call(0xd5640)
    expected = 0 if flags & (1 | 64) else 1
    assert len(creations) == expected, (difficulty, flags, creations)
    reports.append(dict(kind='automatic_placement', difficulty=difficulty, authored_flags=flags,
                        creation_count=len(creations), creations=list(creations)))

for cinematic, suppress in itertools.product([0, 1], repeat=2):
    reset(loaded=1, active=1); placement(); b(IFACE + 5, cinematic); b(IFACE + 7, suppress)
    call(0xd4e30)
    assert len(creations) == int(not (cinematic and suppress))
    reports.append(dict(kind='cinematic_placement_gate', cinematic=cinematic,
                        suppress_automatic_placement=suppress, creation_count=len(creations)))

for type, named, policy in itertools.product([0, 6, 7, 10], [False, True], [0, 1, 2]):
    reset(loaded=-1, active=-1)
    placement(type=type, name=13 if named else -1, origin=0, policy=policy, mask=1)
    call(0xd5560, args=[0])
    runtime_policy = 2 if policy == 2 else (0 if policy == 1 or type == 0 else 1)
    persistent = named or type in [0, 7]
    expected = int(persistent or runtime_policy != 1)
    assert len(creations) == expected, (type, named, policy, creations)
    if expected:
        iterator_objects = [OBJECT]
        call(0xd4e60, args=[1])
        should_hide = runtime_policy in [1, 2]
        should_destroy = should_hide and not persistent
        state = object_state()
        assert int(state['flags'], 16) == (0x8000080 if should_hide and persistent else 0)
        assert state['destroyed_callbacks'] == (['000b83b0', '000b8460'] if should_destroy else [])
    else: state = None
    reports.append(dict(kind='initial_persistent_then_bsp', type=type, named=named,
                        authored_policy=policy, creation_count=len(creations), result=state))

for type, damage_count in itertools.product(range(13), [0, 1]):
    reset(); w(BASE + 0x30000 + 0x38, 1)
    w(BASE + 0x31000 + 0x60, damage_count)
    result = call(0xd5990, registers=[(UC_X86_REG_ECX, type), (UC_X86_REG_EDX, 0)])
    expected = int(bool((1 << type) & 0x199f) or (type == 6 and damage_count > 0))
    assert result == expected
    reports.append(dict(kind='persistent_definition_predicate', type=type,
                        model_damage_info_count=damage_count, result=bool(result)))

# Original save/controller and original11C1B0 admission of the saved BSP run
# before arena replacement. Storage and non-BSP module effects are supplied.
reset(loaded=0, active=0); mode = 'checkpoint'; snapshot = None
w(0x4e6080, BASE); b(0x4e3b62, 0); b(0x4e3b63, 0); s(0x4e3b64, 0)
w(0x5020e4, 0); w(0x4e6090, -1)
w(0x510c54, BASE + 0x1d000); w(BASE + 0x1d008, 100)
b(OPTIONS + 0x1120, 1); s(HEADER + 0x1248, 0)
call(0x12bb20)
assert snapshot is not None
call(0x11c1b0, args=[1, 1])
assert sr(0x4686c4) == 1 and sr(OPTIONS + 2) == 1
w(VM_SENTINEL, 0x87654321)
# An unrelated pending request to2 is cancelled by main revert before loading0.
b(0x547f73, 1); s(0x547f78, 2)
b(0x547f6f, 1); b(0x547f74, 1); b(0x547f75, 1)
events.clear(); call(0x12ba30)
assert sr(0x4686c4) == 0 and sr(OPTIONS + 2) == 0
assert r(VM_SENTINEL) == 0x12345678 and g.mem_read(0x547f73, 1) == b'\0'
assert sr(0x547f78) == -1
assert r(0x4e6090) == 100
reports.append(dict(kind='cross_bsp_checkpoint_restore', saved_bsp=0,
                    cinematic_bsp=1, cancelled_pending_bsp=2, restored_bsp=sr(0x4686c4),
                    vm_sentinel_restored=True, pending_switch_cleared=True,
                    pending_target_cleared=True, reverted_timestamp=r(0x4e6090),
                    events=list(events)))

report = dict(executable_sha256=PIN, fpcw='027f', mxcsr='1f80',
              scope=__doc__.strip(), cases=reports, all_passed=True)
out = ROOT / 'analysis/campaign/bsp-oracle.json'
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, indent=2) + '\n')
print(f'{len(reports)} unchanged-XBE BSP request/commit/lifetime/placement cases passed')
