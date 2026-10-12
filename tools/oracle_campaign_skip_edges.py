#!/usr/bin/env python3
"""Bounded unchanged-XBE skip controller edges; no native implementation oracle.

Runs the real Cairo-wrapper fixture first. The edge cases below execute the
stage-request fragment, cinematic stage service, input acceptance fragment,
input dispatch and reverted-time equality. Player action/unit admission and
next-map readiness are supplied where explicitly stated. The final helper
cases run original163B60 with only its next-map/location callbacks supplied.
"""
from pathlib import Path
import contextlib
import io
import itertools
import json
import runpy
import struct

path = Path(__file__).with_name('oracle_campaign_skip.py')
with contextlib.redirect_stdout(io.StringIO()):
    exported = runpy.run_path(str(path))
env = exported['hook'].__globals__
g, call, initialize = env['g'], env['call'], env['initialize']
w, r, b = env['w'], env['r'], env['b']
IFACE, TIME, OPTIONS = env['IFACE'], env['TIME'], env['OPTIONS']
ACTION, UNIT = env['ACTION'], env['UNIT']
rows = []
edge_stop = None

def stop_fragment(uc, address, size, user_data):
    if address == edge_stop:
        uc.emu_stop()

g.hook_add(env['UC_HOOK_CODE'], stop_fragment)

def bounded(address, **kwargs):
    global edge_stop
    edge_stop = kwargs['stop']
    call(address, **kwargs)
    edge_stop = None

for stage in range(6):
    initialize()
    w(0x4701ec, stage); w(0x4701f0, 17); w(0x4701f4, 29)
    g.mem_write(0x4701f8, struct.pack('<h', 43))
    bounded(0x2ac3a0, stop=0x2ac3d0)
    actual = [r(0x4701ec), r(0x4701f0), r(0x4701f4),
              struct.unpack('<h', g.mem_read(0x4701f8, 2))[0]]
    expected = [4, 0, 100, 0] if stage < 4 else [stage, 17, 29, 43]
    assert actual == expected
    rows.append(dict(kind='stage_request_fragment', initial_stage=stage,
                     stage_retry_start_consecutive=actual))

for stage, suppress in itertools.product(range(6), [0, 1]):
    initialize(); w(0x4701ec, stage); b(OPTIONS + 0x1121, suppress)
    call(0x226050)
    actual = [r(0x4701ec), g.mem_read(0x547f70, 1)[0]]
    expected = [0, int(not suppress)] if stage == 4 else [stage, 0]
    assert actual == expected
    rows.append(dict(kind='cinematic_stage_service', initial_stage=stage,
                     save_suppressed=suppress, resulting_stage_pending_save=actual))

for live, action, enabled, gated, blocked in itertools.product([0, 1], repeat=5):
    initialize(); b(UNIT + 0x134, live); b(ACTION + 0x1d, action)
    b(IFACE + 6, enabled); b(IFACE + 0x22, gated)
    b(0x547f73, 1); b(0x547f70, 1)
    env['skip_cache_blocked'] = bool(blocked)
    bounded(0x14d715,
         registers=[(env['UC_X86_REG_EDI'], UNIT), (env['UC_X86_REG_ESI'], ACTION)],
         stop=0x14d760 if live else 0x14d80b)
    admitted = bool(live and action and enabled and not (gated and blocked))
    actual = [g.mem_read(address, 1)[0] for address in [0x547f73, 0x547f70, 0x547f72]]
    assert actual == ([0, 0, 1] if admitted else [1, 1, 0])
    rows.append(dict(kind='input_acceptance_fragment', live_unit=live,
                     action_bit=action, enabled=enabled, next_map_gate=gated,
                     supplied_cache_blocked=blocked, bsp_save_dispatch=actual))

for enabled, gated, blocked in itertools.product([0, 1], repeat=3):
    initialize(); b(IFACE + 6, enabled); b(IFACE + 0x22, gated)
    b(0x547f72, 1); env['skip_cache_blocked'] = bool(blocked)
    call(0x12bad0)
    admitted = bool(enabled and not (gated and blocked))
    actual = [g.mem_read(address, 1)[0] for address in [0x547f72, 0x547f6f, 0x547f74, 0x547f75]]
    assert actual == [0, int(admitted), int(admitted), 0]
    rows.append(dict(kind='input_dispatch', enabled=enabled, next_map_gate=gated,
                     supplied_cache_blocked=blocked, dispatch_revert_74_75=actual))

initialize(); w(0x4e6090, 100)
for tick in [100, 101]:
    w(TIME + 8, tick)
    bounded(0x2a9d40, stop=0x2a9d65)
    actual = g.reg_read(env['UC_X86_REG_EAX'])
    assert actual == int(tick == 100)
    rows.append(dict(kind='reverted_timestamp_fragment', tick=tick,
                     reverted_timestamp=100, result=actual))

# Original helper includes a jump table beyond the manifest's 26-byte range.
h = env['Uc'](env['UC_ARCH_X86'], env['UC_MODE_32'])
h.mem_map(0x10000, 0x600000)
for section in env['x'].sections:
    h.mem_write(section.va, env['x'].section_bytes(section))
h.mem_map(0x3000000, 0x10000)
helper_stack, helper_stop = 0x300f000, 0x300e000
next_map, location = 0, 0

def helper_hook(uc, address, size, user_data):
    if address not in [0x163610, 0x213f10]:
        return
    sp = uc.reg_read(env['UC_X86_REG_ESP'])
    ret = struct.unpack('<I', uc.mem_read(sp, 4))[0]
    uc.reg_write(env['UC_X86_REG_ESP'], sp + 4)
    uc.reg_write(env['UC_X86_REG_EAX'], next_map if address == 0x163610 else location)
    uc.reg_write(env['UC_X86_REG_EIP'], ret)

h.hook_add(env['UC_HOOK_CODE'], helper_hook)
for next_map, location in [(0, 0)] + [(1, value) for value in range(5)]:
    h.mem_write(helper_stack, struct.pack('<I', helper_stop))
    h.reg_write(env['UC_X86_REG_ESP'], helper_stack)
    h.emu_start(0x163b60, helper_stop, count=100)
    assert h.reg_read(env['UC_X86_REG_EIP']) == helper_stop
    actual = h.reg_read(env['UC_X86_REG_EAX']) & 0xff
    assert actual == int(bool(next_map) and location in [0, 1, 2])
    rows.append(dict(kind='original_next_map_gate', next_map_exists=bool(next_map),
                     supplied_location=location, blocked=actual))

report = dict(executable_sha256=env['XBE_PIN'], map_sha256=env['MAP_PIN'],
              scope=__doc__.strip(), cases=rows, all_passed=True)
(env['ROOT'] / 'analysis/campaign/skip-research-controller-edges.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'{len(rows)} unchanged-XBE controller edge cases passed; original Cairo routes also passed')
