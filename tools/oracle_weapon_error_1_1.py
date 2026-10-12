#!/usr/bin/env python3
"""Execute original disc 1.1 firing error, recovery, and shot-angle instructions.

No executable instructions or callees are replaced. Object/tag pools, clock,
barrel state, hand counters and a forward direction are supplied fixtures. Idle
and angle paths are bounded instruction segments, not a complete trigger tick.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_ESI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_XMM7,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

XBE_PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
CACHE_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
POOL, HEADERS, WEAPON, UNIT, OTHER, TAGS, DEF, BARREL, TIME, SEED, DIRECTION = [
    0x1000000 + n * 0x1000 for n in range(11)]
STATE = WEAPON + 0x1a4
STACK, STOP = 0x200d000, 0x3000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    x = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest() == XBE_PIN
    cache = Cache(ROOT / 'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest() == CACHE_PIN
    tag = next(t for t in cache.tags.values() if t['group'] == 'weap' and
               t['name'] == 'objects\\weapons\\rifle\\smg\\smg')
    barrel_offset, = cache.block(tag['offset'] + 0x2d0, 0xec)
    definition = bytes(cache.data[tag['offset']:tag['offset'] + 0x2d8])
    barrel = bytes(cache.data[barrel_offset:barrel_offset + 0xec])
    cache.data.close(); cache.stream.close()
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x800000)
    for section in x.sections:
        g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(POOL, 0x10000)
    g.mem_map(0x2000000, 0x10000)
    g.mem_map(STOP, 0x1000)

    def w(at, value): g.mem_write(at, struct.pack('<I', value & 0xffffffff))
    def f(at, value): g.mem_write(at, struct.pack('<f', value))
    def bits(at): return f'{struct.unpack("<I", g.mem_read(at, 4))[0]:08x}'
    def val(at): return struct.unpack('<f', g.mem_read(at, 4))[0]
    def output(at): return dict(value=val(at), bits=bits(at))
    def reset_cpu():
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        g.reg_write(UC_X86_REG_FPSW, 0)
        g.reg_write(UC_X86_REG_FPTAG, 0xffff)
    def run(start, end):
        g.emu_start(start, end, count=100_000)
        assert g.reg_read(UC_X86_REG_EIP) == end
    def entry(start, registers=(), argv=(), end=STOP):
        reset_cpu()
        w(STACK, STOP)
        for i, arg in enumerate(argv): w(STACK + 4 + i * 4, arg)
        g.reg_write(UC_X86_REG_ESP, STACK)
        for reg, value in registers: g.reg_write(reg, value)
        run(start, end)

    def setup(hand, counter, opposite, held, old_code, error, angle_change, overrides):
        g.mem_write(POOL, bytes(0x10000))
        g.mem_write(DEF, definition); g.mem_write(BARREL, barrel)
        w(DEF + 0x2d4, BARREL)
        for offset, value in overrides.items(): f(BARREL + offset, value)
        if old_code: w(DEF + 0x12c, struct.unpack_from('<I', definition, 0x12c)[0] | (1 << 27))
        w(0x4e78d0, POOL); w(POOL + 0x44, HEADERS)
        w(HEADERS + 8, WEAPON); w(HEADERS + 20, UNIT); w(HEADERS + 32, OTHER)
        w(0x4eb114, TAGS); w(TAGS + 8, DEF)
        for at in (WEAPON, OTHER):
            w(at, 0); w(at + 0x14, 1 if held else -1)
            w(at + 0x154, 1 if held else -1)
            g.mem_write(at + 0x12c, bytes([int(held)]))
        g.mem_write(UNIT + 0xaa, b'\0')
        g.mem_write(UNIT + 0x212, bytes([0, 1 if opposite else 255]))
        w(UNIT + 0x218, 0 if hand == 'primary' else 2)
        w(UNIT + 0x21c, 2 if hand == 'primary' else 0)
        # D2940 reads the opposite hand's timer for the queried weapon.
        g.mem_write(UNIT + 0x1fa, bytes([counter if hand == 'secondary' else 0,
                                      counter if hand == 'primary' else 0]))
        w(0x518444, TIME); f(TIME + 4, 1 / 30)
        g.mem_write(TIME + 2, struct.pack('<h', 30))
        f(STATE + 0x1c, error); f(STATE + 0x20, angle_change)
        w(0x4ee9d8, SEED); w(SEED, 0x12345678)

    rows = []
    scenarios = [('single_primary', 'primary', 0, False, True, False),
                 ('dual_primary_recent_secondary', 'primary', 9, True, True, False),
                 ('dual_secondary_recent_primary', 'secondary', 9, True, True, False),
                 ('partial_opposite_timer', 'primary', 3, True, True, False),
                 ('expired_opposite_timer', 'primary', 0, True, True, False),
                 ('unheld_boundary', 'primary', 0, False, False, False),
                 ('old_code_single_boundary', 'primary', 9, False, True, True),
                 ('old_code_dual_boundary', 'primary', 0, True, True, True)]
    scenarios = [(*s, {}) for s in scenarios]
    scenarios += [
        ('timer_saturation_boundary', 'primary', 255, True, True, False, {}),
        ('zero_rates_boundary', 'primary', 3, True, True, False,
         {o:0.0 for o in (0x50,0x54,0xc4,0xc8,0xdc,0xe0)}),
        ('high_rates_boundary', 'primary', 3, True, True, False,
         {o:120.0 for o in (0x50,0x54,0xc4,0xc8,0xdc,0xe0)}),
        ('zero_dual_angle_fallback_boundary', 'primary', 3, True, True, False,
         {0x58:0.005,0x5c:0.0,0x60:0.0,0x70:0.01,0x74:0.02,0x78:0.07}),
        ('one_dual_angle_fallback_boundary', 'primary', 3, True, True, False,
         {0x58:0.005,0x5c:0.0,0x60:0.09,0x70:0.01,0x74:0.02,0x78:0.07}),
        ('varied_rates_angles_boundary', 'secondary', 6, True, True, False,
         {0x50:0.37,0x54:2.19,0xc4:0.71,0xc8:0.43,0xdc:0.91,0xe0:3.17,
          0x58:0.013,0x5c:0.027,0x60:0.071,0x70:0.019,0x74:0.039,0x78:0.089}),
    ]
    for name, hand, counter, opposite, held, old_code, overrides in scenarios:
        for error, angle_change in [(0.0,0.0),(0.0,0.3),(0.2,0.3),(0.99,0.99),(1.0,1.0)]:
            setup(hand, counter, opposite, held, old_code, error, angle_change, overrides)
            # Complete original query and complete original fired-barrel helper.
            entry(0x105840, argv=(0,))
            dual = bool(g.reg_read(UC_X86_REG_EAX) & 255)
            entry(0xd2940, [(UC_X86_REG_EAX, 1 if held else 0xffffffff), (UC_X86_REG_ESI, 0)])
            raw_ramp_bits = g.reg_read(UC_X86_REG_XMM0) & 0xffffffff
            raw_ramp = struct.unpack('<f', struct.pack('<I', raw_ramp_bits))[0]
            entry(0x102090, [(UC_X86_REG_EAX, 0), (UC_X86_REG_ECX, 0)])
            fired = dict(error=output(STATE + 0x1c), angle_change=output(STATE + 0x20))
            # Original caller prefix computes/overrides the idle blend, then
            # the original idle block runs with its firing marker supplied false.
            setup(hand, counter, opposite, held, old_code, error, angle_change, overrides)
            entry(0x1021d0, argv=(0,), end=0x10227c)
            sp = g.reg_read(UC_X86_REG_ESP)
            blend = output(sp + 0x38)
            g.mem_write(sp + 0x15, b'\0')
            g.reg_write(UC_X86_REG_ESI, STATE); g.reg_write(UC_X86_REG_EBP, BARREL)
            run(0x10363e, 0x1036fd)
            idle = dict(error=output(STATE + 0x1c), angle_change=output(STATE + 0x20))
            # Angle interpolation and random deviation use original instructions.
            # Restore the requested error fraction rather than idle output.
            f(STATE + 0x1c, error)
            reset_cpu(); g.reg_write(UC_X86_REG_ESP, STACK)
            g.reg_write(UC_X86_REG_EBP, BARREL); g.reg_write(UC_X86_REG_EBX, STATE)
            g.reg_write(UC_X86_REG_XMM7, 0)
            f(STACK + 0x54, 0); f(STACK + 0x58, blend['value'])
            w(STACK + 0x60, WEAPON)
            g.mem_write(STACK + 0x12c, struct.pack('<3f', 1, 0, 0))
            run(0x108f6e, 0x10905c)
            callsp = g.reg_read(UC_X86_REG_ESP)
            angles = dict(minimum=output(callsp), maximum=output(callsp + 4))
            run(0x10905c, 0x109064)
            direction_bits = [f'{v:08x}' for v in struct.unpack('<3I', g.mem_read(STACK + 0x12c, 12))]
            direction = list(struct.unpack('<3f', g.mem_read(STACK + 0x12c, 12)))
            rows.append(dict(case=name, hand=hand, opposite_timer=counter, opposite_slot_present=opposite,
                             held=held, overridden_weapon_bit27=old_code,
                             barrel_field_overrides={f'{k:02x}':v for k,v in overrides.items()},
                             input_error=error,
                             input_angle_change=angle_change, dual_detected=dual,
                             raw_opposite_ramp=dict(value=raw_ramp,bits=f'{raw_ramp_bits:08x}'),
                             effective_blend=blend, firing=fired, idle=idle,
                             shot_angles=angles, supplied_forward=[1,0,0],
                             shot_direction=direction, shot_direction_bits=direction_bits,
                             initial_seed='12345678', final_seed=f'{struct.unpack("<I",g.mem_read(SEED,4))[0]:08x}'))
    assert len(rows) == 70
    assert all(r['raw_opposite_ramp']['value'] == 0 for r in rows if r['case']=='single_primary')
    authored = {f'{offset:02x}': dict(value=struct.unpack_from('<f',barrel,offset)[0],
                bits=f'{struct.unpack_from("<I",barrel,offset)[0]:08x}',
                cache_file_offset=barrel_offset+offset)
                for offset in (0x50,0x54,0x58,0x5c,0x60,0x70,0x74,0x78,0xc4,0xc8,0xdc,0xe0)}
    report = dict(passed=True, cases=len(rows), xbe_sha256=XBE_PIN, cache_sha256=CACHE_PIN,
                  authored_weapon=tag['name'], authored_barrel=authored,
                  dt=dict(value=1/30, supplied_float_bits=f'{struct.unpack("<I",struct.pack("<f",1/30))[0]:08x}'),
                  routines=['00102090 complete, actual callees00105840/000d2940/000ceab0/000cd1a0',
                            '001021d0..0010227c original blend prefix',
                            '0010363e..001036fd idle branch, firing marker supplied false',
                            '00108f6e..00109064 angle selection and actual00147a90 random-deviation helper'],
                  scope='Original instructions unchanged. Fixture hand timers and firing/idle branch boundary supplied. Bit27 override cases are explicit modified-fixture boundaries, not authored SMG tuning. Does not execute a full trigger state machine, projectile creation, live game tick or networking.',
                  rows=rows)
    out=json.dumps(report,indent=2,allow_nan=False)+'\n'
    if args.output: args.output.parent.mkdir(parents=True,exist_ok=True); args.output.write_text(out)
    else: print(out,end='')

if __name__=='__main__': main()
