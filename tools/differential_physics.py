#!/usr/bin/env python3
"""Compare recovered physics kernels with the pinned retail XBE instructions.

The oracle supplies tags, contacts and engine globals. It does not emulate a
game session or establish Havok/contact-generation parity.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import random
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ECX,
                              UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP,
                              UC_X86_REG_EBP, UC_X86_REG_XMM1, UC_X86_REG_XMM2,
                              UC_X86_REG_XMM3,
                              UC_X86_REG_EIP, UC_X86_REG_XMM0,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'


class Vec3(c.Structure):
    _fields_ = [(axis, c.c_float) for axis in ('x', 'y', 'z')]


class Movement(c.Structure):
    _fields_ = [(name, c.c_float) for name in (
        'forward', 'backward', 'sideways', 'acceleration', 'crouch_forward',
        'crouch_backward', 'crouch_sideways', 'crouch_acceleration',
        'air_acceleration', 'limited_speed')]


class Intent(c.Structure):
    _fields_ = [('velocity', Vec3), ('acceleration', c.c_float),
                ('air_acceleration', c.c_float)]


class Slope(c.Structure):
    _fields_ = [(name, c.c_float) for name in (
        'downhill_scale', 'uphill_scale', 'downhill_full', 'downhill_start',
        'uphill_start', 'uphill_full')]


class Walking(c.Structure):
    _fields_ = [(name, Vec3) for name in ('input', 'current', 'facing', 'normal', 'basis')]
    _fields_ += [('slope', Slope)]
    _fields_ += [(name, c.c_float) for name in (
        'scale', 'acceleration', 'air_acceleration', 'gravity', 'dt', 'damping')]
    _fields_ += [('flags', c.c_uint32), ('suppress_ground_push', c.c_bool)]


class WalkingResult(c.Structure):
    _fields_ = [('velocity', Vec3), ('gravity', c.c_bool)]


class Jump(c.Structure):
    _fields_ = [('since_jump', c.c_int8), ('held_ticks', c.c_int8)]


class Recharge(c.Structure):
    _fields_ = [('body', c.c_float), ('shield', c.c_float),
                ('body_stun', c.c_int), ('shield_stun', c.c_int)]
    _fields_ += [(name, c.c_bool) for name in (
        'dead', 'paused', 'body_recharging', 'shield_recharging',
        'shield_damaged', 'shield_depleted')]


class Flight(c.Structure):
    _fields_ = [('velocity', Vec3), ('position', Vec3), ('displacement', Vec3),
                ('traveled', c.c_float), ('range_exhausted', c.c_bool)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT / 'build/libhalo2_kernels.so')
    parser.add_argument('--output', type=Path, default=ROOT / 'analysis/systems/physics-differential.json')
    args = parser.parse_args()
    data = (ROOT / 'assets/disc/default.xbe').read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise RuntimeError('unexpected executable')
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    base = struct.unpack_from('<I', data, 0x104)[0]
    count, table = struct.unpack_from('<II', data, 0x11c)
    for i in range(count):
        _, address, _, offset, size = struct.unpack_from('<5I', data, table - base + i * 56)
        guest.mem_write(address, data[offset:offset + size])
    guest.mem_map(0x1000000, 0x10000)
    guest.mem_map(0x2000000, 0x10000)
    guest.mem_map(0x3000000, 0x1000)

    def u32(at, value):
        guest.mem_write(at, struct.pack('<I', value & 0xffffffff))

    def f32(at, value):
        guest.mem_write(at, struct.pack('<f', value))

    def vector(at, value):
        guest.mem_write(at, bytes(value))

    def execute(address, arguments=(), registers=()):
        stack = 0x200ff00
        u32(stack, 0x3000000)
        for i, arg in enumerate(arguments):
            u32(stack + 4 + i * 4, arg)
        guest.reg_write(UC_X86_REG_ESP, stack)
        # CRT__control87(0x9001F, 0xFFFFF), invoked at retail tick startup,
        # selects nearest-even / 53-bit precision; SSE exceptions are masked.
        guest.reg_write(UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for register, value in registers:
            guest.reg_write(register, value)
        guest.emu_start(address, 0x3000000, count=10000)
        if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
            raise RuntimeError(f'{address:x} failed to return')

    def bits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    rng = random.Random(0x1f0870)
    library = c.CDLL(str(args.library.resolve()))
    intent = library.halo2_movement_intent
    intent.argtypes = [c.POINTER(Movement), c.POINTER(Vec3), c.c_float,
                       c.c_int, c.c_int, c.POINTER(Intent)]
    intent.restype = None
    walking = library.halo2_walking_velocity
    walking.argtypes = [c.POINTER(Walking), c.POINTER(WalkingResult)]
    walking.restype = None
    height = library.halo2_capsule_height
    height.argtypes = [c.c_float] * 4 + [c.c_int]
    height.restype = c.c_float
    acceleration = library.halo2_projectile_acceleration
    acceleration.argtypes = [c.c_float] * 2
    acceleration.restype = c.c_float
    jump = library.halo2_jump_gate
    jump.argtypes = [c.POINTER(Jump)] + [c.c_int] * 5 + [c.c_float, c.c_int]
    jump.restype = c.c_int
    recharge = library.halo2_recharge
    recharge.argtypes = [c.POINTER(Recharge)] + [c.c_float] * 5 + [c.c_int] * 2
    recharge.restype = c.c_int
    flight = library.halo2_projectile_flight
    flight.argtypes = [c.POINTER(Vec3), c.POINTER(Vec3)] + [c.c_float] * 4 + [c.POINTER(Flight)]
    flight.restype = None

    globals_at, settings, definition = 0x1005000, 0x1005200, 0x1005400
    state, output, history, current = 0x1006000, 0x1006300, 0x1006400, 0x1006500
    time, physics = 0x1004000, 0x1006600
    u32(0x4e034c, globals_at)
    u32(globals_at + 0x134, settings)
    u32(0x510c54, time)
    u32(time + 8, 42)
    u32(0x51e9c4, physics)
    results = {}

    def record(name, address, total, failures, scope):
        results[name] = dict(retail_entry=address, cases=total,
                             bit_exact_matches=total - len(failures),
                             failures=failures[:12], scope=scope)

    failures = []
    total = 5000
    for i in range(total):
        m = Movement(*(rng.uniform(.05, 12) for _ in range(10)))
        controls = Vec3(*(rng.uniform(-1.5, 1.5) for _ in range(3)))
        if i < 20:
            controls = Vec3([0, 1, -1, .00001, -0.0][i % 5], 1, 0)
        crouch, limited, ignore = rng.choice([0, 1, rng.random()]), i % 2, (i // 2) % 2
        guest.mem_write(state, bytes(0x180))
        u32(state + 8, definition)
        u32(definition, 4)
        u32(state + 0x14, ignore << 2)
        vector(state + 0xdc, controls)
        f32(settings + 0x24, m.limited_speed)
        guest.mem_write(settings + 0x2c, bytes(m)[:36])
        execute(0x1e6120, (0, limited, 0, bits(crouch)),
                ((UC_X86_REG_EAX, state), (UC_X86_REG_EDX, output)))
        expected = bytes(guest.mem_read(state + 0x138, 20))
        result = Intent()
        intent(c.byref(m), c.byref(controls), crouch, limited, ignore, c.byref(result))
        if expected != bytes(result):
            failures.append(dict(case=i, expected=expected.hex(), actual=bytes(result).hex(),
                                 control=list(struct.unpack('<3f', bytes(controls))), crouch=crouch))
    record('player_movement', '001e6120', total, failures,
           'player control branch, normalized axes, crouch/limited speed and acceleration; setter copies excluded')

    failures = []
    total = 6000
    for i in range(total):
        s = Walking()
        s.input = Vec3(*(rng.uniform(-8, 8) for _ in range(3)))
        s.current = Vec3(*(rng.uniform(-8, 8) for _ in range(3)))
        s.facing = Vec3(*(rng.uniform(-1, 1) for _ in range(3)))
        s.basis = Vec3(*(rng.uniform(-1, 1) for _ in range(3)))
        s.normal = Vec3(*(rng.uniform(-1, 1) for _ in range(2)), rng.choice([0, .001, rng.uniform(-1, 1)]))
        s.slope = Slope(rng.uniform(.2, 2), rng.uniform(.2, 2), -.7, -.2, .2, .7)
        s.scale, s.acceleration, s.air_acceleration = rng.uniform(0, 2), rng.uniform(0, 100), rng.uniform(0, 50)
        s.gravity, s.dt, s.damping = 4.1712594, rng.choice([0, 1 / 30, 1 / 60, .2]), rng.random()
        s.flags, s.suppress_ground_push = (i % 8) << 2, bool(i % 2)
        if i < 32:
            s.input = Vec3(0, 0, 0)
            s.current = Vec3(0, 0, 0)
            s.normal = Vec3(0, 0, 1)
        guest.mem_write(state, bytes(0x180))
        guest.mem_write(output, bytes(0x100))
        u32(state + 8, definition)
        u32(definition, 4)  # player definition bypasses animation velocity history
        u32(state + 0x14, s.flags | 1)
        f32(state + 0x20, s.scale)
        for offset, value in [(0x138, s.input), (0x9c, s.facing),
                              (0x8c, s.normal), (0x118, s.basis)]:
            vector(state + offset, value)
        for offset, value in [(0x144, s.acceleration), (0x148, s.air_acceleration),
                              (0x130, s.gravity), (0x98, s.damping)]:
            f32(state + offset, value)
        guest.mem_write(state + 0xa8, bytes([s.suppress_ground_push]))
        for offset, value in zip([0x4c, 0x50, 0x58, 0x5c, 0x60, 0x64],
                                 struct.unpack('<6f', bytes(s.slope))):
            f32(definition + offset, value)
        f32(time + 4, s.dt)
        u32(physics + 0x18, 0)
        vector(current, s.current)
        execute(0x1f0870, (history, current), ((UC_X86_REG_ESI, state), (UC_X86_REG_EDI, output)))
        expected = bytes(guest.mem_read(output + 0xc, 12))
        expected_gravity = bool(struct.unpack('<I', guest.mem_read(output, 4))[0] & 1)
        result = WalkingResult()
        walking(c.byref(s), c.byref(result))
        if expected != bytes(result.velocity) or expected_gravity != result.gravity:
            failures.append(dict(case=i, expected=expected.hex(), actual=bytes(result.velocity).hex(),
                                 expected_gravity=expected_gravity, actual_gravity=result.gravity,
                                 flags=s.flags, input=bytes(s).hex()))
    record('walking_velocity', '001f0870', total, failures,
           'player walking and airborne branches with supplied contacts and state; flying/carriers/history excluded')

    # DF5F0's origin query is an explicit external boundary; execute the real
    # shape routine with that query supplied as a no-op at its call boundary.
    def origin_boundary(uc, address, size, _):
        stack = uc.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        uc.reg_write(UC_X86_REG_ESP, stack + 4)
        uc.reg_write(UC_X86_REG_EIP, target)

    hook = guest.hook_add(UC_HOOK_CODE, origin_boundary, begin=0xb9dd0, end=0xb9dd0)
    u32(0x4e0300, globals_at)
    u32(globals_at + 0x44, globals_at + 0x180)
    u32(globals_at + 0x188, state)
    u32(state, 0)
    u32(0x4e3b44, settings)
    u32(settings + 8, definition)
    failures = []
    total = 3000
    for i in range(total):
        standing, crouching, radius, crouch = [rng.uniform(0, 4) for _ in range(4)]
        sphere = i % 2
        u32(definition + 0x264, sphere << 1)
        for offset, value in [(0x268, standing), (0x26c, crouching), (0x270, radius)]:
            f32(definition + offset, value)
        f32(state + 0x2c4, crouch)
        execute(0xdf5f0, (output + 16, output + 20), ((UC_X86_REG_EAX, 0), (UC_X86_REG_ECX, output)))
        expected = struct.unpack('<I', guest.mem_read(output + 16, 4))[0]
        actual = bits(height(standing, crouching, radius, crouch, sphere))
        if expected != actual:
            failures.append(dict(case=i, expected=hex(expected), actual=hex(actual)))
    guest.hook_del(hook)
    record('capsule_height', '000df5f0', total, failures,
           'shape dimensions with external origin query supplied; contact/crouch clearance excluded')

    failures = []
    values = [(a, b) for a in [-1, -0.0, 0, .5, 1, 2] for b in [-0.0, 0, 4.1712594]]
    values += [(rng.uniform(-10, 10), rng.uniform(0, 10)) for _ in range(3000)]
    for i, (scale, gravity) in enumerate(values):
        f32(definition + 0x164, scale)
        f32(physics, gravity)
        execute(0xfa170, registers=((UC_X86_REG_EAX, 0),))
        expected = guest.reg_read(UC_X86_REG_XMM0) & 0xffffffff
        actual = bits(acceleration(scale, gravity))
        if expected != actual:
            failures.append(dict(case=i, expected=hex(expected), actual=hex(actual)))
    record('projectile_acceleration', '000fa170', len(values), failures, 'gravity scalar only')

    boundary_values = {}

    def jump_boundary(uc, address, size, _):
        stack = uc.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        if address == 0xe4050:
            answer, cleanup = boundary_values['unsupported'], 0
        else:
            answer, cleanup = boundary_values['accepted'], 4
            if answer:
                uc.mem_write(state + 0x39c, b'\0')
        uc.reg_write(UC_X86_REG_EAX, int(answer))
        uc.reg_write(UC_X86_REG_ESP, stack + 4 + cleanup)
        uc.reg_write(UC_X86_REG_EIP, target)

    hooks = [guest.hook_add(UC_HOOK_CODE, jump_boundary, begin=a, end=a)
             for a in [0xe4050, 0xe4770]]
    failures = []
    total = 5000
    for i in range(total):
        original = Jump(rng.choice([0, 4, 5, 126, 127, rng.randrange(128)]),
                        rng.choice([0, 8, 9, 10, 126, 127, rng.randrange(128)]))
        held, player, unsupported, hard, accepted = [bool(rng.randrange(2)) for _ in range(5)]
        dt, rate = rng.choice([(1 / 30, 30), (1 / 60, 60)])
        boundary_values.update(unsupported=unsupported, accepted=accepted)
        u32(state + 0x148, int(held) << 1)
        u32(state + 0x13c, 0 if player else 0xffffffff)
        guest.mem_write(state + 0x39c, struct.pack('<b', original.since_jump))
        guest.mem_write(state + 0x39e, struct.pack('<b', original.held_ticks))
        guest.mem_write(state + 0x34c, struct.pack('<h', int(hard)))
        f32(time + 4, dt)
        guest.mem_write(time + 2, struct.pack('<h', rate))
        execute(0xe4680, registers=((UC_X86_REG_EDI, 0),))
        expected = [guest.reg_read(UC_X86_REG_EAX) & 255,
                    struct.unpack('<b', guest.mem_read(state + 0x39c, 1))[0],
                    struct.unpack('<b', guest.mem_read(state + 0x39e, 1))[0]]
        actual = [jump(c.byref(original), held, player, unsupported, hard, accepted, dt, rate)]
        actual += [original.since_jump, original.held_ticks]
        if actual != expected:
            failures.append(dict(case=i, expected=expected, actual=actual))
    for hook in hooks:
        guest.hook_del(hook)
    record('jump_gate', '000e4680', total, failures,
           'support/landing and kick acceptance supplied; actor permission and full impulse effects excluded')

    game, model, info = 0x1006800, 0x1007000, 0x1007400
    u32(0x4e6948, game)
    guest.mem_write(0x4f55e4, b'\0')
    u32(game + 8, 1)
    u32(state, 0)
    u32(definition + 0x38, 1)
    u32(settings + 0x18, model)
    u32(model + 0x60, 1)
    u32(model + 0x64, info)
    u32(info + 0xbc, 0)

    def recharge_boundary(uc, address, size, _):
        stack = uc.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        if address == 0x1e9720:
            uc.reg_write(UC_X86_REG_XMM0, bits(boundary_values['team_scale']))
            cleanup = 0
        else:
            cleanup = 16
        uc.reg_write(UC_X86_REG_ESP, stack + 4 + cleanup)
        uc.reg_write(UC_X86_REG_EIP, target)

    hooks = [guest.hook_add(UC_HOOK_CODE, recharge_boundary, begin=a, end=a)
             for a in [0x1e9720, 0x176780, 0xba7f0]]
    failures = []
    total = 6000
    for i in range(total):
        s = Recharge(rng.choice([0, .25, .5, 1, rng.random()]),
                     rng.choice([0, .25, .5, 1, 1.5, rng.random()]),
                     rng.choice([0, 1, 2, 32767, -1, -32768]),
                     rng.choice([0, 1, 2, 32767, -1, -32768]),
                     *(bool(rng.randrange(2)) for _ in range(6)))
        dt, body_rate, shield_rate = rng.choice([0, 1 / 30, .5]), rng.choice([-1, 0, .1, 1, 5]), rng.choice([-1, 0, .1, 1, 5])
        maximum, team_scale = rng.choice([0, .5, 1]), rng.choice([.5, 1, 1.5])
        body_enabled, shield_enabled = bool(i % 2), bool((i // 2) % 2)
        boundary_values['team_scale'] = team_scale
        guest.mem_write(state + 4, bytes(0x300))
        guest.mem_write(state + 0xaa, b'\0')
        for offset in [0x12c, 0x13c, 0x248, 0xd4]:
            u32(state + offset, 0xffffffff)
        guest.mem_write(state + 0x108, b'\xff\xff')
        for offset, value in [(0xe4, body_enabled), (0xe8, shield_enabled),
                              (0xec, s.body), (0xf0, s.shield)]:
            f32(state + offset, value)
        guest.mem_write(state + 0x104, struct.pack('<hh', s.shield_stun, s.body_stun))
        flags = (int(s.dead) << 2) | (int(s.shield_depleted) << 3) | (int(s.shield_damaged) << 1)
        flags |= (int(s.body_recharging) << 8) | (int(s.shield_recharging) << 9)
        guest.mem_write(state + 0x10a, struct.pack('<H', flags))
        guest.mem_write(game + 0xc, bytes([4 if s.paused else 0]))
        for offset, value in [(0x38, maximum), (0xd0, shield_rate), (0xd4, body_rate)]:
            f32(info + offset, value)
        f32(time + 4, dt)
        execute(0xd5de0, (0,))
        expected_changed = bool(guest.reg_read(UC_X86_REG_EAX) & 255)
        expected_fractions = bytes(guest.mem_read(state + 0xec, 8))
        expected_shield_stun, expected_body_stun = struct.unpack('<hh', guest.mem_read(state + 0x104, 4))
        expected_flags = struct.unpack('<H', guest.mem_read(state + 0x10a, 2))[0]
        actual_changed = bool(recharge(c.byref(s), dt, body_rate, shield_rate, maximum,
                                      team_scale, body_enabled, shield_enabled))
        actual_flags = (int(s.dead) << 2) | (int(s.shield_depleted) << 3) | (int(s.shield_damaged) << 1)
        actual_flags |= (int(s.body_recharging) << 8) | (int(s.shield_recharging) << 9)
        if (expected_changed != actual_changed or expected_fractions != bytes(s)[:8] or
                expected_shield_stun != s.shield_stun or expected_body_stun != s.body_stun or
                expected_flags != actual_flags):
            failures.append(dict(case=i, expected=[expected_changed, expected_fractions.hex(),
                                                   expected_body_stun, expected_shield_stun, expected_flags],
                                 actual=[actual_changed, bytes(s)[:8].hex(), s.body_stun, s.shield_stun, actual_flags]))
    for hook in hooks:
        guest.hook_del(hook)
    record('ordinary_recharge', '000d5de0', total, failures,
           'ordinary branch, team scale/effect/depletion callbacks supplied, no regions/recent damage/overshields')

    failures = []
    total = 5000
    for i in range(total):
        position = Vec3(*(rng.uniform(-1000, 1000) for _ in range(3)))
        velocity = Vec3(*(rng.uniform(-100, 100) for _ in range(3)))
        if i < 30:
            velocity = Vec3(0, 0, [0, .00001, -.00001][i % 3])
        dt = rng.choice([1 / 30, 1 / 60, .125])
        traveled = c.c_float(rng.uniform(0, 100)).value
        maximum = c.c_float(traveled + rng.uniform(0, 5)).value if i % 2 else 0
        world_gravity, scale = c.c_float(4.1712594).value, c.c_float(rng.uniform(0, 2)).value
        gravity = c.c_float(world_gravity * scale).value
        stack = 0x200e000
        guest.mem_write(stack, bytes(0x200))
        guest.mem_write(state + 4, bytes(0x300))
        u32(state + 0x144, 0xffffffff)
        u32(state + 0x12c, 0x100)
        u32(state + 4, 8 if i % 3 == 0 else 0)  # air/water gravity selection
        f32(state + 0x168, traveled)
        vector(state + 0x64, position)
        vector(state + 0x88, velocity)
        f32(definition + 0xe0, maximum)
        f32(definition + 0x164, scale)
        f32(definition + 0x170, scale)
        f32(physics, world_gravity)
        f32(stack + 0x20, dt)
        u32(stack + 0x3c, definition)
        u32(stack + 0x12c, 0)
        for register, value in [(UC_X86_REG_EBP, state), (UC_X86_REG_ESI, state + 0x88),
                                (UC_X86_REG_ESP, stack), (UC_X86_REG_XMM3, 0),
                                (UC_X86_REG_FPCW, 0x027f), (UC_X86_REG_MXCSR, 0x1f80)]:
            guest.reg_write(register, value)
        guest.emu_start(0xf909a, 0xf981a, count=10000)
        if guest.reg_read(UC_X86_REG_EIP) != 0xf981a:
            raise RuntimeError('ballistic segment failed to reach endpoint')
        expected_velocity = bytes(guest.mem_read(stack + 0x10, 12))
        expected_position = bytes(guest.mem_read(stack + 0x54, 12))
        expected_action = struct.unpack('<I', guest.mem_read(state + 0x130, 4))[0]
        for register, value in zip([UC_X86_REG_XMM2, UC_X86_REG_XMM1, UC_X86_REG_XMM0],
                                   struct.unpack('<3I', expected_position)):
            guest.reg_write(register, value)
        guest.emu_start(0xf99b4, 0xf9a2a, count=10000)
        if guest.reg_read(UC_X86_REG_EIP) != 0xf9a2a:
            raise RuntimeError('ballistic distance failed to reach store')
        expected_traveled = bytes(guest.mem_read(state + 0x168, 4))
        result = Flight()
        flight(c.byref(position), c.byref(velocity), gravity, dt, traveled, maximum, c.byref(result))
        if (expected_velocity != bytes(result.velocity) or expected_position != bytes(result.position) or
                expected_traveled != struct.pack('<f', result.traveled) or
                (expected_action == 1) != result.range_exhausted):
            failures.append(dict(case=i, expected=[expected_velocity.hex(), expected_position.hex(),
                                                   expected_traveled.hex(), expected_action],
                                 actual=[bytes(result.velocity).hex(), bytes(result.position).hex(),
                                         struct.pack('<f', result.traveled).hex(), result.range_exhausted]))
    record('ballistic_segment', '000f909a..000f981a; 000f99b4..000f9a2a', total, failures,
           'original unaccelerated segment and distance commit, air/water selection supplied, collision/guidance/detonation excluded')
    result = dict(xbe_sha256=EXPECTED, rounding='nearest even; x87 precision 53; MXCSR 0x1f80',
                  kernels=results, scope='kernel parity only; no entire sandbox, Havok or gameplay parity claim')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    return any(v['failures'] for v in results.values())


if __name__ == '__main__':
    raise SystemExit(main())
