#!/usr/bin/env python3
"""Compare native weapon error kernels with untouched 1.1 instructions.

State/angle cases use oracle_weapon_error_1_1's pinned authored and explicitly
modified boundary fixtures. Random rotations execute the complete original
147A90 with no callee replacement. This is not a full trigger/gameplay test.
"""
import argparse
import ctypes as c
import hashlib
import json
import math
from pathlib import Path
import random
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX,
    UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW,
    UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)

XBE_PIN = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
CACHE_PIN = '14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9'
FIELD_OFFSETS = ('dc', 'e0', '50', '54', 'c4', 'c8', '70', '74', '78', '58', '5c', '60')


class Definition(c.Structure):
    _fields_ = [(name, c.c_float) for name in (
        'normal_acceleration', 'normal_deceleration', 'dual_acceleration',
        'dual_deceleration', 'recoil_acceleration', 'recoil_deceleration',
        'normal_minimum', 'normal_low', 'normal_high', 'dual_minimum',
        'dual_low', 'dual_high')]


class State(c.Structure):
    _fields_ = [('error', c.c_float), ('recoil', c.c_float)]


class Angles(c.Structure):
    _fields_ = [('minimum', c.c_float), ('maximum', c.c_float)]


class Vector(c.Structure):
    _fields_ = [('x', c.c_float), ('y', c.c_float), ('z', c.c_float)]


def bits(value):
    return f'{struct.unpack("<I", struct.pack("<f", value))[0]:08x}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path,
                        default=ROOT / 'build-multiplayer/libhalo2_weapon_kernels.so')
    parser.add_argument('--fixtures', type=Path,
                        default=ROOT / 'analysis/multiplayer/map-pack/weapon-error-1.1-oracle.json')
    parser.add_argument('--output', type=Path,
                        default=ROOT / 'analysis/multiplayer/map-pack/weapon-error-1.1-differential.json')
    parser.add_argument('--export-fixtures', type=Path,
                        help='Export passing original cases as offline C++ test fixtures')
    args = parser.parse_args()
    fixtures = json.loads(args.fixtures.read_text())
    if (fixtures['xbe_sha256'] != XBE_PIN or fixtures['cache_sha256'] != CACHE_PIN
            or fixtures['cases'] != len(fixtures['rows']) or not fixtures['passed']):
        raise ValueError('unexpected original instruction fixtures')
    native = c.CDLL(str(args.library.resolve()))
    step = native.halo2_weapon_error_step
    step.argtypes = [c.POINTER(State), c.POINTER(Definition), c.c_float, c.c_float, c.c_int]
    step.restype = None
    angles = native.halo2_weapon_error_angles
    angles.argtypes = [c.POINTER(Definition), c.c_float, c.c_float, c.POINTER(Angles)]
    angles.restype = None
    deviation = native.halo2_weapon_random_deviation
    deviation.argtypes = [c.POINTER(Vector), c.POINTER(Angles), c.POINTER(c.c_uint32), c.POINTER(Vector)]
    deviation.restype = None
    image = Xbe(ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    if hashlib.sha256(image.data).hexdigest() != XBE_PIN:
        raise ValueError('unexpected update executable')
    direction_table = (Vector * 1026).from_buffer_copy(image.read(0x447e40, 1026 * 12))
    dt = struct.unpack('<f', bytes.fromhex(fixtures['dt']['supplied_float_bits'])[::-1])[0]
    for row in fixtures['rows']:
        fields = {k: v['value'] for k, v in fixtures['authored_barrel'].items()}
        fields.update(row['barrel_field_overrides'])
        definition = Definition(*(fields[k] for k in FIELD_OFFSETS))
        blend = row['effective_blend']['value']
        for firing, key in [(True, 'firing'), (False, 'idle')]:
            state = State(row['input_error'], row['input_angle_change'])
            step(c.byref(state), c.byref(definition), dt, blend, firing)
            actual = (bits(state.error), bits(state.recoil))
            expected = (row[key]['error']['bits'], row[key]['angle_change']['bits'])
            if actual != expected:
                raise AssertionError((row['case'], key, row['input_error'], actual, expected))
        out = Angles()
        angles(c.byref(definition), row['input_error'], blend, c.byref(out))
        actual = (bits(out.minimum), bits(out.maximum))
        expected = tuple(row['shot_angles'][k]['bits'] for k in ('minimum', 'maximum'))
        if actual != expected:
            raise AssertionError((row['case'], 'angles', actual, expected))
        direction = Vector(*row['supplied_forward'])
        seed = c.c_uint32(int(row['initial_seed'], 16))
        deviation(c.byref(direction), c.byref(out), c.byref(seed), direction_table)
        actual = tuple(bits(getattr(direction, name)) for name in ('x', 'y', 'z'))
        if actual != tuple(row['shot_direction_bits']) or f'{seed.value:08x}' != row['final_seed']:
            raise AssertionError((row['case'], 'authored rotation', actual, row['shot_direction_bits']))

    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x800000)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    arena, stack, stop = 0x1000000, 0x200d000, 0x3000000
    guest.mem_map(arena, 0x1000)
    guest.mem_map(0x2000000, 0x10000)
    guest.mem_map(stop, 0x1000)
    def original(direction, limits, seed):
        guest.mem_write(arena, struct.pack('<3f', *direction))
        guest.mem_write(arena + 16, struct.pack('<I', seed))
        guest.mem_write(stack, struct.pack('<I2f', stop, *limits))
        guest.reg_write(UC_X86_REG_EAX, arena)
        guest.reg_write(UC_X86_REG_ECX, arena)
        guest.reg_write(UC_X86_REG_EDI, arena + 16)
        guest.reg_write(UC_X86_REG_ESP, stack)
        guest.reg_write(UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        guest.reg_write(UC_X86_REG_FPSW, 0)
        guest.reg_write(UC_X86_REG_FPTAG, 0xffff)
        guest.emu_start(0x147a90, stop, count=1000)
        if guest.reg_read(UC_X86_REG_EIP) != stop:
            raise RuntimeError('original rotation did not return')
        return bytes(guest.mem_read(arena, 12)), bytes(guest.mem_read(arena + 16, 4))

    rng = random.Random(0x147a90)
    cases = []
    # Every table direction is also used as a supplied parallel direction:
    # the original must keep it and consume only the first seed sample.
    inverse_lcg = pow(1664525, -1, 1 << 32)
    for index, vector in enumerate(struct.iter_unpack('<3f', image.read(0x447e40, 1026 * 12))):
        upper = (index * 65536 + 1025) // 1026
        seed = (((upper << 16) - 1013904223) * inverse_lcg) & 0xffffffff
        cases.append((vector, (0.01, 0.09), seed))
    for scale in (0, 1e-8, .0001, .001, 1, 10):
        for direction in ((1, 0, 0), (0, 1, 0), (0, 0, 1), (1, 1, 1)):
            for seed in (0, 0xffffffff, 0x12345678):
                cases.append((tuple(value * scale for value in direction), (0, 0), seed))
    for i in range(12000):
        direction = [rng.uniform(-1, 1) for _ in range(3)]
        length = math.sqrt(sum(value * value for value in direction))
        direction = tuple(value / length for value in direction)
        low = rng.uniform(0, .2)
        high = low if i % 5 == 0 else rng.uniform(low, .5)
        cases.append((direction, (low, high), rng.getrandbits(32)))
    failures = []
    for index, (vector, limits, initial) in enumerate(cases):
        expected_vector, expected_seed = original(vector, limits, initial)
        out = Vector(*vector)
        angle = Angles(*limits)
        seed = c.c_uint32(initial)
        deviation(c.byref(out), c.byref(angle), c.byref(seed), direction_table)
        actual_vector = bytes(out)
        actual_seed = struct.pack('<I', seed.value)
        if actual_vector != expected_vector or actual_seed != expected_seed:
            failures.append(dict(index=index, direction=list(vector), angles=list(limits),
                seed=f'{initial:08x}', actual=actual_vector.hex(), expected=expected_vector.hex(),
                actual_seed=actual_seed.hex(), expected_seed=expected_seed.hex()))
    report = dict(passed=not failures, original_xbe_sha256=XBE_PIN,
        native_library=str(args.library.resolve()),
        native_library_sha256=hashlib.sha256(args.library.read_bytes()).hexdigest(),
        fixture_sha256=hashlib.sha256(args.fixtures.read_bytes()).hexdigest(),
        state_and_angle_cases=len(fixtures['rows']),
        authored_rotation_cases=len(fixtures['rows']), rotation_cases=len(cases),
        rotation_matched=len(cases)-len(failures), rounding='nearest-even; original x87 53-bit',
        scope='Finite supplied inputs. Original state marker, opposite-hand blend and normal angle branch supplied by oracle fixtures. Full trigger/hand timer dispatch, projectile creation, networking and live gameplay excluded.',
        mismatches=failures[:10])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, allow_nan=False) + '\n')
    print(json.dumps({key: value for key, value in report.items() if key != 'mismatches'}, indent=2))
    if failures:
        raise AssertionError(f'{len(failures)} original rotation mismatches; see {args.output}')
    if args.export_fixtures:
        lines = [
            '# Untouched Xbox 1.1 instruction outputs; finite supplied boundary inputs.',
            f'# XBE SHA256 {XBE_PIN}; Lockout SHA256 {CACHE_PIN}',
            f'# Oracle SHA256 {report["fixture_sha256"]}',
            '# Caller firing marker and opposite-hand blend supplied; not a full trigger tick.',
            '# Modified bit27/barrel fixtures retain boundary labels in their case names.',
            '# case input(error,recoil,dt,blend) definition(12 floats in C++ order)',
            '# fired(2) idle(2) angles(2) forward(3) direction(3) initial_seed final_seed',
            '# Each float is its exact 8-digit IEEE754 bits; seeds are uint32 hex.',
        ]
        for row in fixtures['rows']:
            fields = {k: v['value'] for k, v in fixtures['authored_barrel'].items()}
            fields.update(row['barrel_field_overrides'])
            tokens = [row['case'], bits(row['input_error']), bits(row['input_angle_change']),
                      fixtures['dt']['supplied_float_bits'], row['effective_blend']['bits']]
            tokens += [bits(fields[k]) for k in FIELD_OFFSETS]
            for key in ('firing', 'idle'):
                tokens += [row[key]['error']['bits'], row[key]['angle_change']['bits']]
            tokens += [row['shot_angles'][k]['bits'] for k in ('minimum', 'maximum')]
            tokens += [bits(v) for v in row['supplied_forward']]
            tokens += row['shot_direction_bits'] + [row['initial_seed'], row['final_seed']]
            lines.append('\t'.join(tokens))
        args.export_fixtures.parent.mkdir(parents=True, exist_ok=True)
        args.export_fixtures.write_text('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
