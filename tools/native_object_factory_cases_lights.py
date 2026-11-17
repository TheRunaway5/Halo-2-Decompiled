"""Oracle cases for factory track "lights": C3430 (the light tick) and C5290
(light deletion). See docs/native_object_factory_tracks/lights.md.

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

The light tick composes other tracks' natives: attachments' C2CB0
(`light_update`) and placement's C4D30 (`light_reconnect`). Its worlds are
built from those tracks' fixtures, borrowed read-only through
tools/native_object_factory_track_fixtures.py:

- `lights_update`: placement's light world (light pool, light partition,
  marker owners, collision and structure BSPs). The lights' C2CB0 inputs
  are kept on paths that need no object function machinery (literal
  function names, no definition functions), so C2CB0 lights them and C4D30
  reconnects them.
- `lights_update_functions`: attachments' light-update world (function
  driven intensities, owners, timed fades). Its lights are never linked
  ([[0x4EABC4]] is 0 and +2 bit 3 clear), so C4D30 and C5250 stay unreached.
- `light_delete`: C5290 directly, over placement's light world.
"""
import struct

from native_object_factory_track_fixtures import borrow

NONE = 0xffffffff
# 16D740's instanced-geometry and object queries (C4D30 never asks for them)
# are native refusals.
UNPORTED = {0x16cf00, 0x16d0e0}
LITERAL_NAMES = [0, 0x30005a6, 0x40005a7]     # BCB70 answers these without a function lookup


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def register(v):
    placement, placed = borrow(v, 'placement')
    attachments, attached = borrow(v, 'attachments')
    image = v.Xbe(v.NEW_DEFAULT)
    image_blocks = {address: bytes(image.read(address, size)) for address, size in attachments.IMAGE_BLOCKS}
    TRACK, DATA, DATA2 = v.TRACK, v.DATA, v.DATA2
    LIGHT_POOL, LIGHT_ROWS = TRACK + placement.LIGHT_POOL, TRACK + placement.LIGHT_ROWS
    LIGHT_DEF = TRACK + 0x7b00    # placement's light definition (tag row 65)
    placement_light = placed.TRACK_ARGUMENTS['placement_light']
    attachments_light = attached.TRACK_ARGUMENTS['at_light_update']

    def bank_of(world, address):
        for base, bank in ((DATA, world.data), (DATA2, world.data2), (v.DATA3, world.data3), (v.AI, world.ai),
                           (TRACK, world.track), *world.extra.items(), *world.absolute.items()):
            if bank is not None and base <= address < base + len(bank):
                return bank, base
        raise ValueError(f'Undeclared world address {address:x}')

    def byte(world, address, value):
        bank, base = bank_of(world, address)
        bank[address - base] = value & 0xff

    def half(world, address, value):
        bank, base = bank_of(world, address)
        struct.pack_into('<H', bank, address - base, value & 0xffff)

    def word(world, address):
        bank, base = bank_of(world, address)
        return struct.unpack_from('<I', bank, address - base)[0]

    def live_rows(world, pool, rows):
        count = word(world, pool + 0x38)
        bits = word(world, word(world, pool + 0x48))
        return [rows + 0x110 * n for n in range(min(count, 32)) if bits >> n & 1]

    def game_time(world, rng):
        """Ticks per second (C2B30 divides by it times 60), tick seconds and
        the current tick."""
        struct.pack_into('<h', world.data2, v.TIME + 2 - DATA2, rng.choice([30, 60, 30, 1, -7, 32767]))
        struct.pack_into('<f', world.data2, v.TIME + 4 - DATA2, rng.choice([1 / 30, 1 / 60, 0.0333]))
        world.put(v.TIME + 8, rng.choice([3000, 100000, 0x7fffffff, 0x80000001, 16777217, rng.getrandbits(32)]))
        ticks = struct.unpack_from('<h', world.data2, v.TIME + 2 - DATA2)[0]
        return word(world, v.TIME + 8), ticks

    def lifetimes(world, rng, rows, definition, now, ticks):
        """Timed lights (+2 bit 4) created around their lifetime: younger,
        exactly at it, one tick past, much older, or in the future."""
        lifetime = struct.unpack('<f', struct.pack('<I', word(world, definition + 0xb0)))[0]
        span = ticks * lifetime if lifetime == lifetime else 0.0
        for row in rows:
            if rng.random() < .45:
                byte(world, row + 2, (word(world, row) >> 16 & 0xff) | 0x10)
            age = rng.choice([0, 1, int(span), int(span) + 1, int(span) - 1, 10 * abs(int(span)) + 3,
                              -5, rng.getrandbits(31)])
            world.put(row + 0x14, (now - age) & NONE)

    def moved_objects(world, rng):
        for n in range(v.ROWS):
            row = v.RECORDS + 12 * n
            flags = world.data[row + 2 - DATA]
            world.data[row + 2 - DATA] = flags | 4 if rng.random() < .5 else flags & ~4

    def tick_world(name, world, rng):
        """Placement's light world with C2CB0 kept off the function lookups."""
        placement_light(name, world, rng)
        for address, data in image_blocks.items():
            world.extra[address] = bytearray(data)
        now, ticks = game_time(world, rng)
        definition = LIGHT_DEF
        # C2CB0's definition inputs: blend flags (bit 0, an HSV blend, is
        # refused), radii, colours and scales, the lifetime and fade curve,
        # and a function block without functions.
        world.put(definition + 0x34, rng.choice([0, 2, 0xfffffffe]))
        for offset in (0x8, 0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74):
            world.put(definition + offset, fbits(rng.choice([0.0, 0.25, 0.5, 1.0, 1.5, rng.uniform(0, 2), -0.5, 1e-40])))
        # A zero lifetime would make C2CB0's fade fraction infinite (37DC0A
        # then leaves the authenticated sites through its NaN/inf classes).
        world.put(definition + 0xb0, fbits(rng.choice([1.0, 0.5, 2.0, 0.0333, -1.5, 1e-3])))
        half(world, definition + 0xb6, rng.randrange(8))
        world.put(definition + 0xc0, rng.choice([0, 1, 2, 3]))
        for offset in (0xc4, 0xcc, 0xd4):
            world.put(definition + offset, rng.choice([0, 0, NONE]))
        world.globals[0x474c70] = TRACK + 0xf800
        for k in range(3):
            world.put(TRACK + 0xf800 + 4 * k, fbits(rng.choice([1.0, 0.5, 0.0])))
        # [[0x4EABC4]] (placement's counters byte) lets C2CB0 reach C4D30.
        byte(world, TRACK + placement.COUNTERS, rng.choice([1, 1, 1, 0x80, 0]))
        rows = live_rows(world, LIGHT_POOL, LIGHT_ROWS)
        for row in rows:
            world.put(row + 0x50, rng.choice(LITERAL_NAMES))
            half(world, row + 0x56, 0xffff)     # no change colour channel
            world.put(row + 0xcc, fbits(rng.choice([1.0, 0.5, 0.0, 2.0])))
            # Lit (bit 0 visible request), linked (bit 3) and timed (bit 4) mixes.
            byte(world, row + 2, rng.choice([1, 3, 9, 0xb, 0xa, 2, 0, 0x19, 0x1b, 0x13, 8]) |
                 (word(world, row + 2) & 4))
        lifetimes(world, rng, rows, definition, now, ticks)
        if rng.random() < .2:
            # A denormal or signed-zero lifetime: the expiry's COMISS under
            # DAZ. Only marker lights stay timed (C2CB0's fade divides by it).
            world.put(definition + 0xb0, fbits(rng.choice([-1e-42, 1e-42, -0.0])))
            for row in rows:
                if word(world, row + 0x54) & 0xffff == 0xffff:
                    byte(world, row + 2, word(world, row + 2) & 0xef)
                elif rng.random() < .5:
                    world.put(row + 0x14, now)   # age 0
        moved_objects(world, rng)
        return []

    def functions_world(name, world, rng):
        """Attachments' light-update world; every live row is a built one."""
        attachments_light(name, world, rng)
        LIGHTS, LIGHT_BITS, ROWS = attachments.FIXTURES['lights']
        world.put(LIGHTS + 0x38, rng.choice([8, 8, 7, 5]))
        world.put(LIGHT_BITS, rng.randrange(1, 256))
        now, _ = game_time(world, rng)
        rows = live_rows(world, LIGHTS, ROWS)
        for row in rows:
            age = rng.choice([0, 1, 30, 31, 60, 61, 1000, -5, rng.getrandbits(31)])
            world.put(row + 0x14, (now - age) & NONE)
        moved_objects(world, rng)
        return []

    def delete_case(name, world, rng):
        placement_light(name, world, rng)
        rows = live_rows(world, LIGHT_POOL, LIGHT_ROWS)
        # Parents shared by several lights, unique, or none.
        parents = rng.sample(world.handles[:v.ROWS - 1], 2)
        for row in rows:
            world.put(row + 0x58, rng.choice(parents + [NONE, rng.choice(world.handles[:v.ROWS - 1])]))
            byte(world, row + 2, rng.choice([0, 2, 8, 0xa, 0xb, 0x1a, 0xff]))
        for parent in parents:
            # The parent's "has lights" bit (body +4 bit 6) set or clear.
            body = world.body(parent & 0xffff)
            flags = word(world, body + 4)
            world.put(body + 4, flags | 0x40 if rng.random() < .5 else flags & ~0x40)
        chosen = rng.choice(rows)
        index = (chosen - LIGHT_ROWS) // 0x110
        salt = word(world, chosen) & 0xffff
        if rng.random() < .2:
            # A row size other than C5290's 0x110 literal: the scan follows
            # the header's size, the deleted light's own row the literal.
            world.put(LIGHT_POOL + 0x24, 0x120)
        return [salt << 16 | index, rng.choice([0, 1, 1, 0x100, 0xff])]

    stops = sorted(set(attachments.STOPS) - {0xc4d30, 0xc5250} | UNPORTED | {0x10de40})
    v.KERNELS['lights_update'] = (['lights_tick'], 'void')
    v.ROUTINES['lights_update'] = dict(entry=0xc3430, closure_stops=stops)
    v.TRACK_ARGUMENTS['lights_tick'] = tick_world
    v.TRACK_COUNTS['lights_update'] = 600

    v.KERNELS['lights_update_functions'] = (['lights_tick_functions'], 'void')
    v.ROUTINES['lights_update_functions'] = dict(entry=0xc3430, closure_stops=sorted(attachments.STOPS))
    v.TRACK_ARGUMENTS['lights_tick_functions'] = functions_world
    v.TRACK_COUNTS['lights_update_functions'] = 600
    # 10DE40's CALL ECX targets (BEC50 and the type callbacks) join the
    # function-driven tick's authenticated sites, under attachments' stops.
    attachments.ROUTINE_NAMES.append('lights_update_functions')
    attachments._indirect_metadata(v)

    v.KERNELS['light_delete'] = (['lights_delete'], 'void')
    v.ROUTINES['light_delete'] = dict(entry=0xc5290, stack=[0, 1])
    v.TRACK_ARGUMENTS['lights_delete'] = delete_case
    v.TRACK_COUNTS['light_delete'] = 600
