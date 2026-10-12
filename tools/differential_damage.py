#!/usr/bin/env python3
"""Instruction differentials for armor lookup, shield damage and body damage.

Effect/parent services, maximum shield and material selection are explicit
external boundaries. This does not establish complete combat parity.
"""
import argparse
import ctypes as c
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX,
                              UC_X86_REG_EDI,
                              UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_XMM0,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
UPDATE_EXPECTED = 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
# Corresponding operands in the original instructions, not a uniform image
# relocation. Calls/globals were checked in D5BC0/D5D20/D8CB0/D9020/D9110/DB090.
# Both executables retain the object/tag layouts used by the fixtures below.
UPDATE_ADDRESSES = {
    0xd5bc0: 0xd71c0, 0xd5c90: 0xd7290, 0xd5d20: 0xd7320,
    0xd5d80: 0xd7380, 0xd6800: 0xd7e00, 0xd6a70: 0xd8070,
    0xd8cb0: 0xda2b0, 0xd9020: 0xda620, 0xd9110: 0xda710,
    0xdaaa0: 0xdc0a0, 0xdb090: 0xdc690, 0xdb110: 0xdc710,
    0xb58c0: 0xb6760, 0x176780: 0x1781f0, 0x188690: 0x18a0c0,
    0x1e9720: 0x1ed0b0,
    0x4e0300: 0x4e78d0, 0x4e034c: 0x4e791c,
    0x4e3b44: 0x4eb114, 0x4e6948: 0x4edf18, 0x510c54: 0x518444,
}


class Modifier(c.Structure):
    _fields_ = [('name', c.c_uint32), ('multiplier', c.c_float)]


class Group(c.Structure):
    _fields_ = [(name, c.c_uint32) for name in ('name', 'offset', 'count')]


class Shield(c.Structure):
    _fields_ = [('shield', c.c_float), ('stun', c.c_int), ('flags', c.c_uint16)]


class Input(c.Structure):
    _fields_ = [(name, c.c_float) for name in ('damage', 'maximum_shield',
                'multiplier', 'minimum_stun_damage', 'stun_seconds', 'damaged_threshold')]
    _fields_ += [('tick_rate', c.c_int), ('side_effect', c.c_int), ('radius', c.c_bool)]


class Result(c.Structure):
    _fields_ = [('body_damage', c.c_float), ('shield_damage', c.c_float),
                ('flags', c.c_uint32), ('depleted_effect', c.c_bool), ('damaged_effect', c.c_bool)]


class BodyInput(c.Structure):
    _fields_ = [(name, c.c_float) for name in ('damage', 'maximum_body',
                'death_maximum_body', 'multiplier', 'minimum_stun_damage',
                'stun_seconds', 'region_vitality')]
    _fields_ += [('tick_rate', c.c_int)]
    _fields_ += [(name, c.c_uint32) for name in ('info_flags', 'definition_flags',
                 'region_flags', 'accumulator_flags', 'event_flags')]
    _fields_ += [('reporting_type', c.c_uint8)]
    _fields_ += [(name, c.c_bool) for name in ('region_valid', 'hit_region',
                 'headshot_allowed', 'riders', 'impact_effect')]


class BodyResult(c.Structure):
    _fields_ = [(name, c.c_float) for name in ('remaining', 'normalized_damage', 'region_damage')]
    _fields_ += [('flags', c.c_uint32), ('reporting_type', c.c_uint8)]
    _fields_ += [(name, c.c_bool) for name in ('region_update', 'depleted', 'impact_effect')]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=ROOT / 'build/libhalo2_kernels.so')
    parser.add_argument('--version', choices=('retail', 'map-pack-1.1'), default='retail')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    updated = args.version == 'map-pack-1.1'
    image_path = ROOT / ('assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
                         if updated else 'assets/disc/default.xbe')
    expected_hash = UPDATE_EXPECTED if updated else EXPECTED
    address_map = UPDATE_ADDRESSES if updated else {}
    original_addresses = {v: k for k, v in address_map.items()}

    def image_address(address):
        return address_map.get(address, address)

    if args.output is None:
        args.output = ROOT / ('analysis/multiplayer/map-pack/damage-1.1-differential.json'
                             if updated else 'analysis/systems/damage-differential.json')
    data = image_path.read_bytes()
    if hashlib.sha256(data).hexdigest() != expected_hash:
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
        guest.mem_write(image_address(at), struct.pack('<I', value & 0xffffffff))

    def f32(at, value):
        guest.mem_write(at, struct.pack('<f', value))

    def bits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    def execute(address, arguments=(), registers=()):
        stack = 0x200ff00
        u32(stack, 0x3000000)
        for i, argument in enumerate(arguments):
            u32(stack + 4 + i * 4, argument)
        guest.reg_write(UC_X86_REG_ESP, stack)
        guest.reg_write(UC_X86_REG_FPCW, 0x027f)
        guest.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for register, value in registers:
            guest.reg_write(register, value)
        guest.emu_start(image_address(address), 0x3000000, count=10000)
        if guest.reg_read(UC_X86_REG_EIP) != 0x3000000:
            raise RuntimeError(f'{address:x} failed to return')

    library = c.CDLL(str(args.library.resolve()))
    multiplier = library.halo2_damage_multiplier
    multiplier.argtypes = [c.POINTER(Group), c.c_uint, c.POINTER(Modifier)] + [c.c_uint32] * 4
    multiplier.restype = c.c_float
    shield_damage = library.halo2_shield_damage
    shield_damage.argtypes = [c.POINTER(Shield), c.POINTER(Input), c.POINTER(Result)]
    shield_damage.restype = None
    body_damage = library.halo2_body_damage
    body_damage.argtypes = [c.POINTER(Shield), c.POINTER(BodyInput), c.POINTER(BodyResult)]
    body_damage.restype = None
    rng = random.Random(0xd9110)
    globals_at, table_at, groups_at, modifiers_at = 0x1000000, 0x1000200, 0x1000400, 0x1001000
    u32(0x4e034c, globals_at)
    u32(globals_at + 0xd4, table_at)
    failures, total = [], 6000
    for case in range(total):
        group_keys = sorted(rng.choices(range(8), k=rng.randrange(10)))
        modifiers, groups = [], []
        for i, key in enumerate(group_keys):
            armor_keys = sorted(rng.choices(range(8), k=rng.randrange(8)))
            offset = len(modifiers)
            modifiers += [(armor, rng.choice([0, -0.0, .25, .5, 1, 2, rng.uniform(-2, 3)]))
                          for armor in armor_keys]
            groups.append(Group(key, offset, len(armor_keys)))
            guest.mem_write(groups_at + i * 12,
                            struct.pack('<III', key, len(armor_keys), modifiers_at + offset * 8))
        # Include wrapping signed comparisons and unsorted inputs: reproduce
        # the original search even when an arbitrary synthetic table is used.
        if case % 7 == 0:
            for i, group in enumerate(groups):
                group.name = rng.getrandbits(32)
                u32(groups_at + i * 12, group.name)
            modifiers = [(rng.getrandbits(32), value) for _, value in modifiers]
        native_modifiers = (Modifier * len(modifiers))(*(Modifier(*m) for m in modifiers))
        guest.mem_write(modifiers_at, bytes(native_modifiers))
        u32(table_at, len(groups)); u32(table_at + 4, groups_at)
        keys = [rng.randrange(10) for _ in range(4)]
        if groups and case % 7 == 0:
            keys[0] = rng.choice(groups).name
        execute(0xd5bc0, keys[2:], [(UC_X86_REG_ECX, keys[0]), (UC_X86_REG_EDX, keys[1])])
        expected = guest.reg_read(UC_X86_REG_XMM0) & 0xffffffff
        actual = bits(multiplier((Group * len(groups))(*groups), len(groups), native_modifiers, *keys))
        if actual != expected:
            failures.append(dict(case=case, expected=hex(expected), actual=hex(actual), keys=keys))
    table_result = dict(retail_entry=f'{image_address(0xd5bc0):08x}', cases=total, bit_exact_matches=total - len(failures),
                        failures=failures[:12], scope='original search/comparator executed; missing, duplicate and wrapping keys')

    state, info, definition, event, accumulator, material, time = [0x1003000 + i * 0x400 for i in range(7)]
    u32(0x4e0300, globals_at)
    u32(globals_at + 0x44, globals_at + 0x180)
    u32(globals_at + 0x188, state)
    u32(0x510c54, time)
    boundary = {}

    def callback(uc, address, size, _):
        boundary['calls'].append(hex(address))
        address = original_addresses.get(address, address)
        stack = uc.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        cleanup = 0
        if address == 0xd5d80:
            uc.reg_write(UC_X86_REG_XMM0, bits(boundary['input'].maximum_shield)); cleanup = 4
        elif address == 0x188690:
            uc.reg_write(UC_X86_REG_EAX, material); cleanup = 4
        elif address == 0xd5bc0:
            uc.reg_write(UC_X86_REG_XMM0, bits(boundary['input'].multiplier)); cleanup = 8
        elif address == 0xd6a70:
            flags = struct.unpack('<H', uc.mem_read(state + 0x10a, 2))[0]
            uc.mem_write(state + 0x10a, struct.pack('<H', flags | 8))
            f32(state + 0xf4, 0)
            boundary['depleted'] += 1
        elif address == 0x176780:
            boundary['damaged'] += 1; cleanup = 16
        uc.reg_write(UC_X86_REG_ESP, stack + 4 + cleanup)
        uc.reg_write(UC_X86_REG_EIP, target)

    hooks = [guest.hook_add(UC_HOOK_CODE, callback, begin=image_address(a), end=image_address(a))
             for a in (0xd5d80, 0x188690, 0xd5bc0, 0xd6a70, 0x176780)]
    # D5BC0 ran unmodified above. Invalidate its translated blocks so Unicorn
    # installs the newly supplied material-multiplier boundary at that entry.
    guest.ctl_remove_cache(image_address(0xd5bc0), image_address(0xd5c90))
    failures, total = [], 6000
    for case in range(total):
        original = Shield(rng.choice([-1, -0.0, 0, .25, 1, 2, rng.uniform(-1, 2)]),
                          rng.choice([-32768, -1, 0, 1, 32767]), rng.randrange(1024))
        input = Input(rng.choice([-1, -0.0, 0, .0001, 10, 100, rng.uniform(-1, 200)]),
                      rng.choice([0, -1, 50, 100]), rng.choice([0, .0001, .5, 1, 2, -1]),
                      rng.choice([0, 1, 10]), rng.choice([0, .05, .15, 1, 2]),
                      rng.choice([0, .25, .5, 1]), rng.choice([30, 60]), rng.choice([0, 1, 3]), bool(case % 2))
        boundary.update(input=input, depleted=0, damaged=0, calls=[])
        guest.mem_write(state, bytes(0x300)); guest.mem_write(accumulator, bytes(0x34))
        for offset in (0xd4, 0x12c):
            u32(state + offset, 0xffffffff)
        f32(state + 0xf0, original.shield)
        guest.mem_write(state + 0x104, struct.pack('<h', original.stun))
        guest.mem_write(state + 0x10a, struct.pack('<H', original.flags))
        u32(accumulator + 4, info); f32(accumulator + 0x1c, input.damage)
        for offset, value in ((0x94, input.minimum_stun_damage), (0x98, input.stun_seconds), (0xa0, input.damaged_threshold)):
            f32(info + offset, value)
        guest.mem_write(time + 2, struct.pack('<h', input.tick_rate))
        guest.mem_write(definition, struct.pack('<hh', input.side_effect, 0))
        guest.mem_write(event + 0x68, bytes([input.radius]))
        execute(0xd9110, (0, definition, event, accumulator))
        expected = [bytes(guest.mem_read(state + 0xf0, 4)).hex(),
                    struct.unpack('<h', guest.mem_read(state + 0x104, 2))[0],
                    struct.unpack('<H', guest.mem_read(state + 0x10a, 2))[0],
                    bytes(guest.mem_read(accumulator + 0x1c, 8)).hex(),
                    struct.unpack('<I', guest.mem_read(accumulator + 0x30, 4))[0],
                    boundary['depleted'], boundary['damaged']]
        result = Result()
        shield_damage(c.byref(original), c.byref(input), c.byref(result))
        actual = [struct.pack('<f', original.shield).hex(), original.stun, original.flags,
                  bytes(result)[:8].hex(), result.flags, int(result.depleted_effect), int(result.damaged_effect)]
        if expected != actual:
            failures.append(dict(case=case, expected=expected, actual=actual, input=bytes(input).hex(), callbacks=boundary['calls']))
    for hook in hooks:
        guest.hook_del(hook)
    shield_result = dict(retail_entry=f'{image_address(0xd9110):08x}', cases=total, bit_exact_matches=total - len(failures),
                         failures=failures[:12], scope='full routine with supplied maximum/material/effect boundaries; actor override and parent services excluded')

    # Body oracle executes D9020, D5BC0, D5D20 and DB090 unchanged. Only
    # team, region-response, rider/death and effect services are supplied.
    guest.ctl_flush_tb()
    game, region, entry, biped, tags = [0x1004c00 + i * 0x400 for i in range(5)]
    u32(0x4e6948, game); u32(0x4e3b44, tags); u32(tags + 8, biped)
    u32(globals_at + 0x150, 1); u32(globals_at + 0x154, material)
    u32(material + 0x10, 2); u32(material + 0x14, 0)
    u32(table_at, 1); u32(table_at + 4, groups_at)
    guest.mem_write(groups_at, struct.pack('<III', 1, 1, modifiers_at))
    u32(modifiers_at, 2)

    def body_callback(uc, address, size, _):
        stack = uc.reg_read(UC_X86_REG_ESP)
        target = struct.unpack('<I', uc.mem_read(stack, 4))[0]
        boundary['calls'].append(hex(address))
        address = original_addresses.get(address, address)
        cleanup = 0
        if address == 0x1e9720:
            uc.reg_write(UC_X86_REG_XMM0, bits(boundary['team_scale']))
        elif address == 0xdaaa0:
            boundary['region'] = bytes(uc.mem_read(stack + 0x14, 4)).hex()
            boundary['region_count'] += 1; cleanup = 24
        elif address == 0xdb110:
            uc.reg_write(UC_X86_REG_EAX, int(boundary['input'].riders)); cleanup = 12
        elif address == 0xd6800:
            flags = struct.unpack('<H', uc.mem_read(state + 0x10a, 2))[0]
            uc.mem_write(state + 0x10a, struct.pack('<H', flags | 4))
            boundary['depleted'] += 1; cleanup = 16
        elif address == 0xb58c0:
            cleanup = 8
        elif address == 0x176780:
            boundary['impact'] += 1; cleanup = 16
        uc.reg_write(UC_X86_REG_ESP, stack + 4 + cleanup)
        uc.reg_write(UC_X86_REG_EIP, target)

    hooks = [guest.hook_add(UC_HOOK_CODE, body_callback, begin=image_address(a), end=image_address(a))
             for a in (0x1e9720, 0xdaaa0, 0xdb110, 0xd6800, 0xb58c0, 0x176780)]
    failures, total = [], 8000
    for case in range(total):
        original = Shield(rng.choice([-1, -0.0, 0, .0001, .25, 1, rng.uniform(-1, 2)]),
                          rng.choice([-32768, -1, 0, 1, 32767]), rng.randrange(65536))
        raw_maximum = rng.choice([-1, 0, 30, 100])
        team_scale = rng.choice([.5, 1, 1.5]); category = rng.randrange(2)
        object_type = rng.randrange(3); game_state = rng.randrange(2)
        player_present = bool(case % 2)
        scaled = c.c_float(raw_maximum * team_scale).value if object_type < 2 else raw_maximum
        armor = c.c_float(rng.choice([-1, -0.0, 0, .5, 1, 2, rng.uniform(0, 3)])).value
        material_present = bool((case // 2) % 2)
        hit_entry = bool((case // 4) % 2)
        default_index, hit_index = rng.choice([-1, 0, 1]), rng.choice([-1, 0, 1])
        section_index = hit_index if hit_entry else default_index
        input = BodyInput(rng.choice([-1, -0.0, 0, .0001, 10, 100, rng.uniform(-1, 200)]),
                          scaled if category != 1 else raw_maximum, scaled,
                          armor if material_present else 1,
                          rng.choice([-1, 0, 1, 10]), rng.choice([0, .05, .15, 1, 2]),
                          rng.choice([-1, 0, .25, 1]), rng.choice([30, 60]),
                          rng.randrange(256), rng.randrange(256), rng.randrange(512),
                          rng.randrange(2048), rng.randrange(256), rng.randrange(256),
                          section_index == 0, hit_entry,
                          not (game_state == 1 and object_type == 0 and player_present),
                          bool((case // 8) % 2), object_type == 0 and bool((case // 16) % 2))
        boundary.update(input=input, team_scale=team_scale, depleted=0, impact=0,
                        region='00000000', region_count=0, calls=[])
        guest.mem_write(state, bytes(0x300)); guest.mem_write(accumulator, bytes(0x34))
        u32(state, 0)
        for offset in (0x12c,):
            u32(state + offset, 0xffffffff)
        u32(state + 0xd4, 0 if case % 3 else 0xffffffff)
        u32(state + 0x13c, 0 if player_present else 0xffffffff)
        guest.mem_write(state + 0xaa, bytes([object_type]))
        f32(state + 0xe4, raw_maximum); f32(state + 0xec, original.shield)
        guest.mem_write(state + 0x106, struct.pack('<h', original.stun))
        guest.mem_write(state + 0x10a, struct.pack('<H', original.flags))
        u32(info, input.info_flags)
        guest.mem_write(info + 8, struct.pack('<h', default_index))
        f32(info + 0x2c, input.minimum_stun_damage); f32(info + 0x30, input.stun_seconds)
        u32(info + 0xbc, 1); u32(info + 0xc0, region)
        u32(region + 4, input.region_flags); f32(region + 8, input.region_vitality)
        guest.mem_write(entry + 6, struct.pack('<h', hit_index))
        u32(accumulator, entry if hit_entry else 0); u32(accumulator + 4, info)
        f32(accumulator + 0x1c, input.damage); u32(accumulator + 0x30, input.accumulator_flags)
        guest.mem_write(definition + 2, struct.pack('<h', category)); u32(definition + 4, input.definition_flags)
        u32(definition + 0x40, 1); u32(definition + 0x44, 0)
        f32(modifiers_at + 4, armor)
        u32(event + 4, input.event_flags)
        guest.mem_write(event + 0x7c, struct.pack('<h', 0 if material_present else -1))
        guest.mem_write(event + 0x84, bytes([input.reporting_type]))
        guest.mem_write(time + 2, struct.pack('<h', input.tick_rate))
        u32(game + 8, game_state)
        u32(biped + 0x260, 0 if input.impact_effect else 0xffffffff)
        execute(0xd8cb0, (0, definition, event), [(UC_X86_REG_EDI, accumulator)])
        expected = [bytes(guest.mem_read(state + 0xec, 4)).hex(),
                    struct.unpack('<h', guest.mem_read(state + 0x106, 2))[0],
                    struct.unpack('<H', guest.mem_read(state + 0x10a, 2))[0],
                    bytes(guest.mem_read(accumulator + 0x1c, 4)).hex(),
                    bytes(guest.mem_read(accumulator + 0x28, 4)).hex(), boundary['region'],
                    struct.unpack('<I', guest.mem_read(accumulator + 0x30, 4))[0],
                    guest.mem_read(event + 0x84, 1)[0], boundary['region_count'],
                    boundary['depleted'], boundary['impact']]
        result = BodyResult()
        body_damage(c.byref(original), c.byref(input), c.byref(result))
        actual = [struct.pack('<f', original.shield).hex(), original.stun, original.flags,
                  struct.pack('<f', result.remaining).hex(), struct.pack('<f', result.normalized_damage).hex(),
                  struct.pack('<f', result.region_damage).hex(), result.flags, result.reporting_type,
                  int(result.region_update), int(result.depleted), int(result.impact_effect)]
        if expected != actual:
            failures.append(dict(case=case, expected=expected, actual=actual,
                                 input=bytes(input).hex(), callbacks=boundary['calls']))
    for hook in hooks:
        guest.hook_del(hook)
    body_result = dict(retail_entry=f'{image_address(0xd8cb0):08x}', cases=total, bit_exact_matches=total - len(failures),
                       failures=failures[:12], scope='Armor/body-vitality/region-selection callees execute unchanged; team/region response/rider/death/effect boundaries supplied; actor stun override excluded')
    result = dict(xbe_sha256=expected_hash, source_executable=str(image_path), version=args.version,
                  native_library=str(args.library.resolve()),
                  native_library_sha256=hashlib.sha256(args.library.read_bytes()).hexdigest(),
                  address_correspondence={f'{k:08x}': f'{v:08x}' for k, v in address_map.items()},
                  rounding='nearest even; x87 precision 53; MXCSR 0x1f80',
                  kernels=dict(damage_table=table_result, shield_damage=shield_result, body_damage=body_result),
                  scope='selected damage kernels; collision-region selection, region responses, difficulty services, effect execution and full combat parity excluded')
    result['passed'] = all(k['bit_exact_matches'] == k['cases'] for k in result['kernels'].values())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result))
    return bool(table_result['failures'] or shield_result['failures'] or body_result['failures'])


if __name__ == '__main__':
    raise SystemExit(main())
