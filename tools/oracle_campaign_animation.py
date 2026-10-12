#!/usr/bin/env python3
"""Bounded original XBE channel/query/matrix fixtures, not a gameplay emulator."""
from pathlib import Path
import hashlib, json, struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

root = Path(__file__).resolve().parents[1]
blob = (root / 'assets/disc/default.xbe').read_bytes()
expected = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
assert hashlib.sha256(blob).hexdigest() == expected
u = Uc(UC_ARCH_X86, UC_MODE_32)
u.mem_map(0x10000, 0x600000)
base = struct.unpack_from('<I', blob, 0x104)[0]
count, table = struct.unpack_from('<II', blob, 0x11c)
for i in range(count):
    _, address, _, offset, size = struct.unpack_from('<5I', blob, table - base + i * 56)
    u.mem_write(address, blob[offset:offset + size])
u.mem_map(0x2000000, 0x10000)
u.mem_map(0x3000000, 0x1000)
u.reg_write(UC_X86_REG_FPCW, 0x027f)
u.reg_write(UC_X86_REG_MXCSR, 0x1f80)

def write(address, fmt, *values):
    u.mem_write(address, struct.pack('<' + fmt, *values))

def read(address, fmt):
    return struct.unpack('<' + fmt, u.mem_read(address, struct.calcsize('<' + fmt)))

def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]

def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]

def call(address, *arguments, eax=0, ecx=0, edx=0, xmm0=0):
    u.reg_write(UC_X86_REG_ESP, 0x200f000)
    write(0x200f000, 'I' * (len(arguments) + 1), 0x3000000, *[value & 0xffffffff for value in arguments])
    for register, value in [(UC_X86_REG_EAX, eax), (UC_X86_REG_ECX, ecx), (UC_X86_REG_EDX, edx), (UC_X86_REG_XMM0, xmm0)]:
        u.reg_write(register, value & 0xffffffff)
    u.emu_start(address, 0x3000000, count=1000000)
    assert u.reg_read(UC_X86_REG_EIP) == 0x3000000, hex(u.reg_read(UC_X86_REG_EIP))
    return u.reg_read(UC_X86_REG_EAX)

tag_table, graph, animation, game_time = 0x2000000, 0x2001000, 0x2002000, 0x2002800
pool, headers, unit, scenery, anchor = 0x2003000, 0x2003800, 0x2004000, 0x2005000, 0x2006000
state, control, matrix = unit + 0x400, unit + 0x600, 0x2007000
unit_handle, scenery_handle, anchor_handle = 0x10000, 0x10001, 0x10002
write(0x4e3b44, 'I', tag_table)
write(tag_table + 1 * 16 + 8, 'I', graph)
write(graph + 0x2c, 'II', 1, animation)
write(0x510c54, 'I', game_time)
write(game_time + 2, 'Hf', 30, f32(1 / 30))
# All resources are resident and animation has no event tables. The retail
# resource-request gate is disabled; lookup/clock/event code remains original.
write(0x510c20, 'BB', 0, 0)
write(0x4e0300, 'I', pool)
write(pool + 0x24, 'I', 12)
write(pool + 0x38, 'I', 3)
write(pool + 0x44, 'I', headers)
for i, obj, kind in [(0, unit, 0), (1, scenery, 6), (2, anchor, 6)]:
    write(headers + i * 12, 'HBBHHI', 1, 0, kind, 0, 0, obj)
    write(obj + 0x116, 'h', 0x800)
    write(obj + 0x12a, 'h', 0x400)
write(unit + 0x33e, 'h', 0x600)
write(control + 0x28, 'I', anchor_handle)
write(scenery + 0x138, 'I', anchor_handle)

def initialize(frame_count=516, loop_frame=0, flags=0x4001, position=0, rate=1, delay=0):
    u.mem_write(animation, bytes(0x6c))
    write(animation + 0x14, 'h', frame_count)
    write(animation + 0x2e, 'h', loop_frame)
    u.mem_write(state, bytes(0x200))
    # animation ID stores graph index -1 and local animation index 0.
    write(state, 'Ihh', 1, -1, 0)
    write(state + 0x10, 'BBH', delay, 0, flags)
    write(state + 0x18, 'ff', rate, position)
    write(state + 0x68, 'I', 1)
    write(state + 0x7c, 'I', 0x0e0000c2)
    u.mem_write(scenery + 0x400, bytes(u.mem_read(state, 0x200)))

def channel_state():
    return dict(frame=read(state + 0x1c, 'f')[0], bits=f'{read(state + 0x1c, "I")[0]:08x}',
                completion_flags=read(state + 0x11, 'B')[0], delay=read(state + 0x10, 'B')[0])

def update(delta=f32(1 / 30)):
    write(game_time + 4, 'f', delta)
    call(0x1c68c0, state, 0, 0, eax=state)

seeks = []
for frame in [-32768, -5, -1, 0, 1, 514, 514.5, 515, 515.5, 516, 32767]:
    initialize()
    call(0x1c6560, bits(frame), eax=state)
    seeks.append(dict(requested_frame=frame, **channel_state()))
script_seeks = []
for frame in [-32768, -1, 0, 1, 7, 515, 516, 32767]:
    initialize()
    call(0x1ccb80, eax=state, xmm0=bits(f32(f32(frame) * f32(1 / 30))))
    script_seeks.append(dict(requested_short_frame=frame, **channel_state()))
nonloop = []
initialize(position=513)
for i in range(6):
    nonloop.append(dict(update_index=i, **channel_state()))
    update()
looping = []
for start, loop_frame, delta in [(515, 0, f32(1 / 30)), (515, 40, f32(1 / 30)), (0, 0, 40)]:
    initialize(flags=0x4003, position=start, loop_frame=loop_frame)
    update(delta)
    looping.append(dict(start=start, loop_frame=loop_frame, delta=delta, **channel_state()))
rates = []
for rate, delta in [(0, f32(1 / 30)), (.5, f32(1 / 30)), (2, f32(1 / 30)), (-1, f32(1 / 30)), (1, f32(1 / 60))]:
    initialize(position=4, rate=rate)
    update(delta)
    rates.append(dict(rate=rate, tick_seconds=delta, **channel_state()))
initialize(delay=2)
delays = []
for i in range(4):
    delays.append(dict(update_index=i, **channel_state()))
    update()
initialize(position=4)
write(state + 0x6c, 'B', 1)
call(0x1cb5b0, 0, 0, ecx=state, edx=state)
paused = channel_state()

# HS parser and result return are fixture adapters. The unit helper, float32
# duration/subtraction, x87 rounding, clamp and short storage remain original.
hs_arguments, hs_result = 0x2007800, None
write(hs_arguments, 'I', unit_handle)

def adapter(uc, address, size, data):
    global hs_result
    if address not in (0x209d50, 0x209ae0):
        return
    sp = uc.reg_read(UC_X86_REG_ESP)
    target = read(sp, 'I')[0]
    if address == 0x209d50:
        uc.reg_write(UC_X86_REG_EAX, hs_arguments)
        adjustment = 16
    else:
        hs_result = uc.reg_read(UC_X86_REG_EAX)
        adjustment = 0
    uc.reg_write(UC_X86_REG_ESP, sp + 4 + adjustment)
    uc.reg_write(UC_X86_REG_EIP, target)

u.hook_add(UC_HOOK_CODE, adapter)
queries = []
for frame in [0, 1, 1.5, 2.5, 100.5, 513, 513.5, 514, 514.5, 515, 515.9998779296875]:
    initialize(position=frame)
    call(0x2a3450, 0xc7, 1, 0)
    scenery_result = call(0x10a460, ecx=scenery_handle)
    queries.append(dict(frame=frame, unit_ticks=hs_result & 0xffff, scenery_ticks=scenery_result))
invalid_queries = []
for name, address, fmt, value in [('unit_none', hs_arguments, 'I', 0xffffffff),
        ('wrong_unit_state', state + 0x7c, 'I', 0x0400000c),
        ('unit_no_graph', state + 0x68, 'I', 0xffffffff),
        ('unit_no_channel', state + 6, 'h', -1)]:
    initialize()
    write(hs_arguments, 'I', unit_handle)
    write(address, fmt, value)
    call(0x2a3450, 0xc7, 1, 0)
    invalid_queries.append(dict(condition=name, ticks=hs_result & 0xffff))
for name, address, fmt, value in [('wrong_scenery_type', headers + 12 + 3, 'B', 0),
        ('scenery_no_state', scenery + 0x12a, 'h', -1),
        ('scenery_no_graph', scenery + 0x400 + 0x68, 'I', 0xffffffff)]:
    initialize()
    write(address, fmt, value)
    invalid_queries.append(dict(condition=name, ticks=call(0x10a460, ecx=scenery_handle)))
    write(headers + 12 + 3, 'B', 6)
    write(scenery + 0x12a, 'h', 0x400)
invalid_queries.append(dict(condition='scenery_none', ticks=call(0x10a460, ecx=0xffffffff)))

relative = []
local = [1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 3, 4, 5]
for label, anchor_matrix, valid in [
        ('live_anchor_identity', [1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 54, 0, 7], True),
        ('live_anchor_rotated_scaled', [2, 0, 1, 0, -1, 0, 0, 0, 0, 1, 60, -4, 9], True),
        ('destroyed_anchor', [2, 0, 1, 0, -1, 0, 0, 0, 0, 1, 60, -4, 9], False)]:
    write(anchor + 0x800, '13f', *anchor_matrix)
    write(headers + 24, 'H', 1 if valid else 0)
    row = dict(condition=label, local=local, live_anchor=anchor_matrix)
    for name, callback, handle in [('unit', 0x112dc0, unit_handle), ('scenery', 0x10a390, scenery_handle)]:
        write(matrix, '13f', *local)
        call(callback, handle, matrix)
        row[name + '_result'] = list(read(matrix, '13f'))
    relative.append(row)

write(0x4de2f4, 'I', 0x2008000)
start_failures = []
for name, obj, graph_datum, clip, state_offset in [
        ('object_none', -1, 1, 0x08000111, 0x400),
        ('graph_none', unit_handle, -1, 0x08000111, 0x400),
        ('missing_clip', unit_handle, 1, 0x08000112, 0x400),
        ('zero_clip', unit_handle, 1, 0, 0x400),
        ('missing_state', unit_handle, 1, 0x08000111, -1)]:
    initialize(position=100)
    write(animation, 'I', 0x08000111)
    write(control, 'H', 0x110)
    write(control + 0x28, 'I', anchor_handle)
    write(unit + 0x12a, 'h', state_offset)
    before = bytes(u.mem_read(state, 0x200))
    result_bool = call(0x11b520, clip, 0, anchor_handle, 0, eax=graph_datum, ecx=obj) & 0xff
    start_failures.append(dict(kind='unit', condition=name, result=bool(result_bool),
        old_state_preserved=before == bytes(u.mem_read(state, 0x200)),
        old_control_flags=read(control, 'H')[0], old_anchor=read(control + 0x28, 'I')[0]))
    write(unit + 0x12a, 'h', 0x400)
for name, obj, graph_datum, clip, state_offset in [
        ('object_none', -1, 1, 0x08000111, 0x400),
        ('graph_none', scenery_handle, -1, 0x08000111, 0x400),
        ('missing_clip', scenery_handle, 1, 0x08000112, 0x400),
        ('zero_clip', scenery_handle, 1, 0, 0x400),
        ('missing_state', scenery_handle, 1, 0x08000111, -1)]:
    initialize(position=100)
    write(animation, 'I', 0x08000111)
    write(scenery + 0x12a, 'h', state_offset)
    before = bytes(u.mem_read(scenery + 0x400, 0x200))
    result_bool = call(0x10a660, obj, clip, 0, anchor_handle, 1, 0, eax=graph_datum) & 0xff
    start_failures.append(dict(kind='scenery', condition=name, helper_result=bool(result_bool),
        old_state_preserved=before == bytes(u.mem_read(scenery + 0x400, 0x200))))
    write(scenery + 0x12a, 'h', 0x400)

sampling = []
sample_data, empty_sizes = 0x2008100, 0x2008200
write(sample_data, 'IIBBh', 0, empty_sizes, 0, 0, 516)
for frame, interpolate in [(1.5, False), (1.5, True), (1.00005, True),
        (1.99995, True), (514.5, True), (515, True), (515.5, True), (515.9998779296875, True)]:
    initialize()
    u.reg_write(UC_X86_REG_ESI, sample_data)
    u.reg_write(UC_X86_REG_XMM1, bits(frame))
    u.reg_write(UC_X86_REG_XMM4, bits(1))
    call(0x2798a0, graph, animation, 0, 0, 0, 0, int(interpolate), 0, bits(0), bits(0))
    sample_frame, next_frame, fraction = read(0x504464, 'iif')
    sampling.append(dict(requested_frame=frame, interpolate=interpolate,
        sample_frame=sample_frame, next_frame=next_frame, fraction=fraction))

result = dict(xbe_sha256=expected,
    scope='Original XBE numeric channel/query/matrix fixtures; synthetic resident graph/object arena, empty events, HS parser/result adapters; no codecs/physics/full-scene proof',
    fpcw='027f', mxcsr='1f80', seeks=seeks, script_short_seeks=script_seeks,
    nonloop_sequence=nonloop, loop_wrap=looping, rate_and_tick_duration=rates,
    delayed_start=delays, paused_state=paused, duration_queries=queries,
    invalid_queries=invalid_queries, live_anchor_composition=relative, start_failures=start_failures,
    sampler_index_fraction=sampling)
path = root / 'analysis/campaign/animation-playback-golden.json'
path.write_text(json.dumps(result, indent=2) + '\n')
print('Wrote', path)
