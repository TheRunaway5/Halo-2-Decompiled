"""Oracle cases for factory track "objtypes" (object type new/new-failed callbacks).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Every "new" callback runs with its three original stack arguments (object,
placement request, failure byte). Subtrees the native refuses by name are
closure_stops: 10F710 (scenery animation start, later animation work) and
1CE340 (Havok phantom wake, the havok track's refusal at 318550). Fixtures
never reach them, and the observer rejects any case that enters one.

1D9F10 and its 315CB0 body activation are the havok track's natives:
objtypes_object_wake_bodies runs BBBC0 over that track's physics world
(borrowed read only), so the composed BBBC0 -> 1D9F10 -> 315CB0 path is proven
against the original.
"""
import importlib.util
from pathlib import Path
import struct

from native_object_factory_track_fixtures import borrow

NONE = 0xffffffff
# Track bank layout (offsets from v.TRACK).
REQUEST = 0x0000
SCENARIO, SCENARIO_ROWS, SCENERY_OBJECTS = 0x0100, 0x0200, 0x0800
LIGHTMAP, LIGHTMAP_GROUP, LIGHTMAP_INFO, IDENTIFIER = 0x1000, 0x1100, 0x1200, 0x1400
COMPONENTS, COMPONENT_ROWS = 0x2000, 0x2100
NETWORK_OWNER, NETWORK_ROWS, NETWORK_STATE = 0x3000, 0x3100, 0x4000
DEVICE_GROUPS, DEVICE_GROUP_ROWS, DEVICE_GROUP_BITMAP = 0x7000, 0x7100, 0x7200
MACHINE_WORDS, MACHINE_BITS = 0x8000, 0x8800
STRUCTURE, STRUCTURE_PATHFINDING, PATHFINDING_TABLE = 0x9000, 0x9300, 0x9400
PHYSICS_TAGS, PHYSICS_ROWS = 0xa000, 0xb000
PHYSICS_TAG_ROWS = (12, 13, 14, 15)
UP_VECTOR, OPTIONS, GAME_GLOBALS, DIFFICULTY = 0xc000, 0xc100, 0xc200, 0xc400
WATER_BSP, WATER_CLUSTERS, WATER_PLANES, MATERIALS = 0xc800, 0xc900, 0xcc00, 0xd000
FLOATS = [0.0, -0.0, 1.0, 0.5, 2.0, 1 / 30, 0.0333, 1e-40, -1e-41, 1e-5, 1e-4, 3.0, -2.5, 30.0, 1e20,
          float('nan'), float('inf'), 0.99999994, 1.0000001]
RANDOM_UNIT = 0x37800080
# Animation graphs (tag rows 20..23, 0x400 bytes each) and a manager record.
GRAPHS, MANAGER = 0xe000, 0xf100
GRAPH_ROWS = (20, 21, 22, 23)
ANIMATION_NAMES = [0x1000011, 0x2000022, 0x5000033, 0x6000086, 0x400000c]
ABSENT_NAMES = [0x7000083, 0x30000d9, 0x400054b]
# Refused or other-track subtrees outside every closure here: modelstate's
# 1D3380 (with its census-unexecuted 1E3090/1D62C0), the 126200 residency
# request, core roots, and the Havok owners 1CC310, 1D9510 and 315CB0.
# 1D5320's closure is admitted with the weapon track's tables.
ANIMATION_STOPS = [0x1d3380, 0x126200, 0x1e3090, 0x1d62c0, 0xbf050, 0xbba80, 0xb95d0, 0xba630,
                   0x1cc310, 0x1d9510, 0x315cb0]


def weapon_cases():
    """The weapon track's oracle module (read only): its proven animation
    graph/manager Fixture and 1D5320's jump tables and virtual owners."""
    path = Path(__file__).resolve().parent / 'native_object_factory_cases_weapon.py'
    spec = importlib.util.spec_from_file_location('objtypes_weapon_fixture', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def register(v):
    T = v.TRACK
    weapon = weapon_cases()
    v.UNREACHED_VIRTUAL_OWNERS |= weapon.UNREACHED
    v.JUMP_TABLES.update(weapon.TABLES)

    def u32(world, address):
        for base, bank in ((v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (T, world.track)):
            if base <= address < base + len(bank):
                return struct.unpack_from('<I', bank, address - base)[0]
        raise ValueError(hex(address))

    def put16(world, address, value):
        for base, bank in ((v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (T, world.track)):
            if base <= address < base + len(bank):
                struct.pack_into('<H', bank, address - base, value & 0xffff)
                return
        raise ValueError(hex(address))

    def put8(world, address, value):
        for base, bank in ((v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (T, world.track)):
            if base <= address < base + len(bank):
                bank[address - base] = value & 0xff
                return
        raise ValueError(hex(address))

    def fresh(world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))

    def live(world):
        return world.handles[:v.ROWS - 1]

    def tag(row, rng):
        return (rng.getrandbits(16) << 16) | row

    def scenery_rows(world, rng):
        """Scenario scenery rows (0x5C) whose +58 word less one indexes the
        [0x52BE70] object table, and body +1A naming them."""
        world.globals[0x4ecc20] = T + SCENARIO
        world.put(T + SCENARIO + 0x54, T + SCENARIO_ROWS)
        for row in range(8):
            put16(world, T + SCENARIO_ROWS + 0x5c * row + 0x58, rng.choice([0, 0, 1, 2, 3, 5, 9, 0xfffe]))
        world.globals[0x52be70] = T + SCENERY_OBJECTS
        for n in range(v.ROWS - 1):
            put16(world, world.body(n) + 0x1a, rng.choice([0xffff, 0xffff, 0, 1, 2, 3, 7]))

    def network(world, rng):
        """B7E50 owner: +8 mode, +4 state (+20A4 flag rows, +20B8 masks)."""
        world.globals[0x4dc04c] = T + NETWORK_OWNER
        world.put(T + NETWORK_OWNER + 8, rng.choice([4, 4, 4, 3, 5, 0, 1]))
        world.put(T + NETWORK_OWNER + 4, T + NETWORK_STATE)
        world.put(T + NETWORK_STATE + 0x20a4, T + NETWORK_ROWS)
        for n in range(v.ROWS - 1):
            world.put(world.body(n) + 0xd4, rng.choice([NONE, NONE, rng.randrange(16), (rng.getrandbits(22) << 10) | rng.randrange(16)]))

    def lightmap(world, rng):
        """[0x4ECC14] lightmap: +80 group count, +84 group (+58 count, +5C
        12-byte scenery object rows); bodies' +A4 identifiers match some rows."""
        world.globals[0x4ecc14] = rng.choice([0] + [T + LIGHTMAP] * 9)
        world.put(T + LIGHTMAP + 0x80, rng.choice([1] * 8 + [2, 0, NONE]))
        world.put(T + LIGHTMAP + 0x84, T + LIGHTMAP_GROUP)
        count = rng.choice([0, NONE] + [1, 3, 8, 8, 16, 16] * 2)
        world.put(T + LIGHTMAP_GROUP + 0x58, count)
        world.put(T + LIGHTMAP_GROUP + 0x5c, T + LIGHTMAP_INFO)
        uniques = [0x1000 + k for k in range(3)]
        rows = []
        for row in range(16):
            ident = (rng.choice(uniques), rng.choice([0, 1, 0xffff]), rng.choice([0, 6, 9]), rng.choice([0, 0, 1, 2]))
            checksum = rng.choice([0x11111111, 0x22222222, rng.getrandbits(32)])
            rows.append((ident, checksum))
            struct.pack_into('<IHBBI', world.track, LIGHTMAP_INFO + 12 * row, *ident, checksum)
        # Identifiers mostly name rows inside the searched count.
        return rows[:count] if 0 < count <= 16 and rng.random() < .85 else rows

    def identifier(world, rng, at, rows):
        """An 8-byte identifier, usually a lightmap row's; returns that row's
        checksum (or a random one)."""
        checksum = rng.getrandbits(32)
        if rows and rng.random() < .8:
            (unique, bsp, kind, source), checksum = rng.choice(rows)
            # Vary the origin BSP word (compared only for source 0) and bytes.
            bsp = bsp if rng.random() < .7 else rng.choice([0, 1, 0xffff])
            kind = kind if rng.random() < .9 else 6
            source = source if rng.random() < .9 else rng.choice([0, 1])
        else:
            unique, bsp, kind, source = rng.getrandbits(32), rng.getrandbits(16), rng.getrandbits(8), rng.getrandbits(8)
        world.put(at, unique)
        put16(world, at + 4, bsp)
        put8(world, at + 6, kind)
        put8(world, at + 7, source)
        return checksum

    def render_checksums(world, rng, rows, wanted):
        for k in range(4):
            choices = [checksum for _, checksum in rows] + [0x11111111, rng.getrandbits(32)]
            world.put(v.RENDER_TAGS + 0x60 * k + 8, wanted if rng.random() < .6 else rng.choice(choices))
            world.put(v.MODEL_TAGS + 0x80 * k + 4, rng.choice([(0x6008 + k) << 16 | (8 + k)] * 3 + [NONE]))
            put16(world, v.OBJECT_TAGS + 0x200 * k + 0xc0, rng.choice([0, 1, 2, 2, 3, 0x102]))

    def object_types(world, rng):
        for n in range(v.ROWS - 1):
            world.data[v.RECORDS + 12 * n + 3 - v.DATA] = rng.choice(list(range(13)) + [0x26, 0x2b])

    def components(world, rng, wake_safe=True):
        """[0x52BE5C] physics components (0xA0 rows: +4 flags, +70 bodies, +74
        count). With wake_safe, none would activate a Havok body (315CB0)."""
        world.globals[0x52be5c] = T + COMPONENTS
        world.put(T + COMPONENTS + 0x44, T + COMPONENT_ROWS)
        for row in range(8):
            at = T + COMPONENT_ROWS + 0xa0 * row
            flags = rng.getrandbits(32)
            count = rng.choice([0, 1, 2, 0xffffffff, 0x80000000])
            if wake_safe and (flags >> 5) & 1 and (count & 0x80000000) == 0 and count:
                flags &= ~0x20 if rng.random() < .5 else 0xffffffff
                if (flags >> 5) & 1:
                    count = rng.choice([0, 0xffffffff, 0x80000000])
            world.put(at + 4, flags)
            world.put(at + 0x74, count)
            world.put(at + 0x70, T + COMPONENT_ROWS + 0x600)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body + 0xb4, rng.choice([NONE, NONE, rng.randrange(8), (rng.getrandbits(16) << 16) | rng.randrange(8)]))
            world.data[body + 0xc0 - v.DATA] = rng.choice([0, 0x40, 0x40, 0xff, 0xbf])

    def graph_tag(g):
        return (0x7700 + g) << 16 | GRAPH_ROWS[g]

    def animation_graphs(world, rng, names=ANIMATION_NAMES):
        """Graph data: +C node count, +2C/+30 animations (0x6C rows: +0
        name, +14 frames, +16 resource flags clear, +30/+32 no group, +4C/+50
        events), +4C/+50 imports (0x20 rows, tag +4)."""
        world.globals[0x51df20] = rng.choice([0, 1, 0x100, 0x101])
        for g in range(4):
            graph = T + GRAPHS + 0x400 * g
            world.put(v.TAG_TABLE + 16 * GRAPH_ROWS[g] + 8, graph)
            put16(world, graph + 0xc, rng.choice([1, 2, 4, 99]))
            count = rng.choice([0, 1, 3, 7, 7, NONE])
            world.put(graph + 0x2c, count)
            world.put(graph + 0x30, graph + 0x80)
            for a in range(7):
                row = graph + 0x80 + 0x6c * a
                world.put(row, rng.choice(names))
                put16(world, row + 0x14, rng.choice([1, 2, 10, 30, 31, 0]))
                put8(world, row + 0x16, 0)
                put16(world, row + 0x30, 0xffff)
                put16(world, row + 0x32, 0xffff)
                world.put(row + 0x4c, rng.choice([0, 1, 2, 4]))
                world.put(row + 0x50, graph + 0x380 + 0x10 * a)
                for e in range(4):
                    put8(world, graph + 0x380 + 0x10 * a + 4 * e, rng.choice([0, 1, 3, 7, 15]))
                    put16(world, graph + 0x380 + 0x10 * a + 4 * e + 2, rng.choice([0, 1, 5, 29, 30]))
            world.put(graph + 0x4c, rng.choice([0, 1, 2, 2, NONE]))
            world.put(graph + 0x50, graph + 0x40)
            for i in range(2):
                world.put(graph + 0x40 + 0x20 * i + 4, rng.choice([NONE, graph_tag(rng.randrange(4)), graph_tag(rng.randrange(4))]))

    def manager_record(world, rng, at, graph=None):
        """An animation manager: channel 0 at +0 (graph, packed, +11/+12
        flags, +18 speed, +1C frame) and the manager graph +68."""
        g = rng.randrange(4)
        world.put(at, rng.choice([graph_tag(g)] * 4 + [NONE]))
        index = rng.choice([0xffff, rng.randrange(7), rng.randrange(7), rng.randrange(7)])
        imports = struct.unpack_from('<i', world.track, GRAPHS + 0x400 * g + 0x4c)[0]
        live = [i for i in range(min(imports, 2))
                if struct.unpack_from('<I', world.track, GRAPHS + 0x400 * g + 0x40 + 0x20 * i + 4)[0] != NONE]
        world.put(at + 4, index << 16 | rng.choice([0xffff, 0xffff] + live))
        put8(world, at + 0x11, rng.choice([0, 1, 2, 8, 0xff]))
        put8(world, at + 0x12, rng.choice([0, 1, 1, 0xfe]))
        world.put(at + 0x18, fbits(rng.choice([1.0, 0.5, 0.0, -1.0, 2.0, 1e-40])))
        world.put(at + 0x1c, fbits(rng.choice([0.0, 1.0, 5.5, 29.0, -1.0])))
        world.put(at + 0x68, graph if graph is not None else graph_tag(rng.randrange(4)))

    # ---- "new" callbacks: stack (object, request, failure byte) ----
    def new_routine(entry, **extra):
        return dict(entry=entry, stack=[0, 1, 2], **extra)

    def new_args(world, rng, n=None):
        n = rng.randrange(v.ROWS - 1) if n is None else n
        return [world.handles[n], T + REQUEST, T + 0xf000]

    def device_case(name, world, rng):
        fresh(world, rng)
        return new_args(world, rng)

    def machine_case(name, world, rng):
        fresh(world, rng)
        for k in range(4):
            world.data3[v.OBJECT_TAGS + 0x200 * k + 0x11e - v.DATA3] = rng.choice([0, 4, 0xff, 0xfb, 0x0c])
        for n in range(v.ROWS - 1):
            world.put(world.body(n) + 4, rng.getrandbits(32))
        return new_args(world, rng)

    def sound_scenery_case(name, world, rng):
        fresh(world, rng)
        return new_args(world, rng)

    def crate_case(name, world, rng):
        """Request tag (definition +BC bit 0, model +24 physics model with
        0x90-byte rigid bodies, motion word +1E) and the object's own tag walk
        for 212C70; +18 node indices keep BC190's parent product in bank."""
        fresh(world, rng)
        for k in range(4):
            world.data3[v.OBJECT_TAGS + 0x200 * k + 0xbc - v.DATA3] = rng.choice([0, 1, 0xfe, 0xff])
            world.put(v.MODEL_TAGS + 0x80 * k + 0x24, rng.choice([tag(PHYSICS_TAG_ROWS[k], rng), tag(rng.choice(PHYSICS_TAG_ROWS), rng), NONE]))
        for k, row in enumerate(PHYSICS_TAG_ROWS):
            data = T + PHYSICS_TAGS + 0x100 * k
            world.put(v.TAG_TABLE + 16 * row + 8, data)
            world.put(data + 0x38, rng.choice([0, 1, 2, 4, 6, NONE]))
            world.put(data + 0x3c, T + PHYSICS_ROWS + 0x400 * k)
            for body in range(6):
                put16(world, T + PHYSICS_ROWS + 0x400 * k + 0x90 * body + 0x1e, rng.choice([1, 2, 2, 1, 0, 3, 0xffff]))
        for n in range(v.ROWS - 1):
            world.data[world.body(n) + 0x18 - v.DATA] = rng.choice([0, 1, 2, 3])
        world.put(T + REQUEST, rng.choice([tag(rng.randrange(4), rng), tag(rng.randrange(4), rng), NONE]))
        return new_args(world, rng)

    def scenery_case(name, world, rng):
        """Scenario slots, lightmap rows, deferred wake (types, physics,
        network marks) and animation blocks that never start (10F710)."""
        fresh(world, rng)
        scenery_rows(world, rng)
        rows = lightmap(world, rng)
        object_types(world, rng)
        components(world, rng)
        network(world, rng)
        wanted = {}
        animation_graphs(world, rng, names=ANIMATION_NAMES[:4])
        for n in range(v.ROWS - 1):
            body = world.body(n)
            wanted[n] = identifier(world, rng, body + 0xa4, rows)
            kind = rng.random()
            if kind < .35:
                put16(world, body + 0x12a, 0xffff)
            elif kind < .5:
                world.put(body + 0x380 + 0x68, NONE)
            else:
                # 10F710 runs and fails: 0x400000C is in no graph here.
                manager_record(world, rng, body + 0x380)
        n = rng.randrange(v.ROWS - 1)
        render_checksums(world, rng, rows, wanted[n])
        return new_args(world, rng, n)

    def fbits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    def garbage_seed(rng, rate):
        """A seed whose next draw puts (draw/65535 + 1) * 10 * rate within a
        float ULP of a half integer, so the FSTP dword rounding decides the
        FISTP result under round-to-nearest."""
        from fractions import Fraction
        unit = Fraction(struct.unpack('<f', struct.pack('<I', RANDOM_UNIT))[0])
        for _ in range(4000):
            seed = rng.getrandbits(32)
            draw = ((seed * 0x19660d + 0x3c6ef35f) & 0xffffffff) >> 16
            x = (draw * unit + 1) * 10 * rate
            ulp = Fraction(2) ** (abs(x).numerator.bit_length() - abs(x).denominator.bit_length() - 24)
            if abs(abs(x - int(x)) - Fraction(1, 2)) < ulp:
                return seed
        return rng.getrandbits(32)

    def garbage_case(name, world, rng):
        """BD980 delete schedule ([0x4EABC4] list, game time), primary seed
        and the x87 delay under every declared control word; half the cases
        sit on a float-rounding boundary of the delay."""
        fresh(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body + 4, rng.choice([0, 0x4000, 0x8000, 0xc000, rng.getrandbits(32)]))
            world.put(body + 0x20, rng.choice([0, 100, 0x7fffffff, 0x80000000, rng.getrandbits(32)]))
        rate = rng.choice([30, 60, 30, -7, 32767, 0, 29, -32768, 1])
        seed = rng.getrandbits(32)
        if rng.random() < .5:
            rate = rng.choice([32767, 30001, 25013, -32768, 16411, 9999])
            seed = garbage_seed(rng, rate)
        world.put(v.SEED, seed)  # [0x4F3CD8] -> SEED holds the seed value
        struct.pack_into('<h', world.data2, v.TIME + 2 - v.DATA2, rate)
        return new_args(world, rng)

    def projectile_floats(world, rng, at, count, pool=FLOATS):
        for k in range(count):
            world.put(at + 4 * k, fbits(rng.choice(pool)) if rng.random() < .85 else rng.getrandbits(32))

    def spin_bodies(world, rng):
        for n in range(v.ROWS - 1):
            body = world.body(n)
            kind = rng.random()
            if kind < .2:
                values = [0.0, 0.0, 0.0]
            elif kind < .35:
                values = [rng.choice([1e-5, -1e-5, 5e-5, 7.0710e-5, 1e-4, 1.0001e-4, 0.99e-4, 1e-40]) for _ in range(3)]
            elif kind < .45:
                values = [rng.choice(FLOATS) for _ in range(3)]
            else:
                values = [rng.uniform(-20, 20) for _ in range(3)]
            for k, value in enumerate(values):
                world.put(body + 0x94 + 4 * k, fbits(value))
            world.put(body + 0x12c, rng.getrandbits(32))
        struct.pack_into('<f', world.data2, v.TIME + 4 - v.DATA2,
                         rng.choice([1 / 30, 1 / 60, 0.0333, 1e-41, 1.0, 3.0e9, float('nan')]))

    def projectile_case(name, world, rng):
        """Definitions (+BC bits 2/10, +CC/+D4/+D8 rates, +94/+98 'cont'
        materials, +16C/+178/+17C/+184/+188), difficulty tables, cluster
        water planes, spin, owners with biped roots and +3E8 weapons."""
        fresh(world, rng)
        world.extra[0x48a0b0] = bytearray(rng.getrandbits(8) for _ in range(4))
        world.globals[0x474d10] = T + UP_VECTOR
        world.globals[0x4f3218] = T + OPTIONS
        world.put(T + OPTIONS + 8, rng.choice([1, 1, 0, 2]))
        put16(world, T + OPTIONS + 0x132, rng.choice([0xffff, 0, 1, 2, 3, 4, 7, 0x8000]))
        world.globals[0x4ecc1c] = rng.choice([0, T + GAME_GLOBALS, T + GAME_GLOBALS, T + GAME_GLOBALS])
        world.put(T + GAME_GLOBALS + 0xf8, rng.choice([0, 1, 1, 1]))
        world.put(T + GAME_GLOBALS + 0xfc, rng.choice([0, T + DIFFICULTY, T + DIFFICULTY]))
        world.globals[0x4ecc18] = T + WATER_BSP
        world.put(T + WATER_BSP + 0xa0, T + WATER_CLUSTERS)
        world.put(T + WATER_BSP + 0x68, T + WATER_PLANES)
        for cluster in range(4):
            world.track[WATER_CLUSTERS + 0xb0 * cluster + 0x70] = rng.choice([0xff, 0, 1, 0x80, 0x81, 0x82, 2])
        for plane in range(4):
            at = T + WATER_PLANES + 24 * plane
            put16(world, at + 2, rng.choice([0xffff, 0, 3, 0]))
            normal = rng.choice([(0.0, 0.0, 1.0), (1.0, 0.0, 0.0), (0.6, 0.8, 0.0)])
            for k, value in enumerate(normal):
                world.put(at + 4 + 4 * k, fbits(value))
            # 0x00800001 against a min-normal (0x00800000) position: the
            # distance is the negative denormal -2^-149 (DAZ compare).
            world.put(at + 0x10, rng.choice([fbits(rng.choice([0.0, 2.0, -3.0, 1e-40, float('nan')])), 0x00800001]))
        for k in range(4):
            definition = v.OBJECT_TAGS + 0x200 * k
            world.put(definition + 0xbc, rng.getrandbits(32))
            projectile_floats(world, rng, definition + 0xcc, 1)
            rates = [rng.choice(FLOATS + [0.25, 0.1]) for _ in range(2)]
            if rng.random() < .3:
                rates[1] = rates[0]
            world.put(definition + 0xd4, fbits(rates[0]))
            world.put(definition + 0xd8, fbits(rates[1]))
            world.put(definition + 0x94, rng.choice([0, 1, 3, 6, NONE]))
            world.put(definition + 0x98, T + MATERIALS + 0x100 * k)
            for row in range(6):
                world.put(T + MATERIALS + 0x100 * k + 24 * row, rng.choice([0x636f6e74, 0x6d657461, 0]))
            projectile_floats(world, rng, definition + 0x16c, 1)
            projectile_floats(world, rng, definition + 0x178, 2)
            fuse = [rng.choice(FLOATS + [4.0, 10.0]) for _ in range(2)]
            if rng.random() < .4:
                fuse[1] = fuse[0]
            world.put(definition + 0x184, fbits(fuse[0]))
            world.put(definition + 0x188, fbits(fuse[1]))
        projectile_floats(world, rng, T + DIFFICULTY, 48)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put16(world, body + 0x2c, rng.choice([0xffff, 0, 1, 2, 3]))
            for at in (0x30, 0x70, 0x88):
                projectile_floats(world, rng, body + at, 3)
            if rng.random() < .25:
                for k, value in enumerate((0, 0, 0x00800000)):
                    world.put(body + 0x30 + 4 * k, value)
            world.data[v.RECORDS + 12 * n + 3 - v.DATA] = rng.choice([0, 0, 1, 2, 5, 0x20])
            weapon = rng.choice([NONE, 0x12340005, 0x56780006])
            world.put(body + 0x3e8, weapon)
            world.put(body + 0x3e4, weapon if rng.random() < .5 else rng.choice([NONE, 0x12340005]))
            world.data[body + 0xc0 - v.DATA] = rng.choice([0, 2, 4, 6, 0xf9, 0xff])
        spin_bodies(world, rng)
        world.put(v.SEED, rng.getrandbits(32))
        put = world.put
        put(T + REQUEST + 0x5c, rng.choice([NONE, NONE, 0, rng.choice(live(world))]))
        put(T + REQUEST + 0x60, rng.choice([NONE] + live(world)))
        for k in range(3):
            put(T + REQUEST + 0x1c + 4 * k, rng.getrandbits(32))
        return new_args(world, rng)

    def spin_case(name, world, rng):
        fresh(world, rng)
        spin_bodies(world, rng)
        return [rng.choice(live(world))]

    # ---- "new failed" callbacks: stack (object) ----
    def device_failed_case(name, world, rng):
        """Device groups (+130/+13C) in a [0x4ECBF8] data array; flagged rows
        (+2 bit 2) are released through 16E8A0."""
        fresh(world, rng)
        world.globals[0x4ecbf8] = T + DEVICE_GROUPS
        pool = T + DEVICE_GROUPS
        world.put(pool + 0x24, 12)
        world.track[DEVICE_GROUPS + 0x2a] = rng.choice([0, 0, 8, 0xf7, 0xff])
        world.put(pool + 0x34, rng.randrange(0, 9))
        world.put(pool + 0x38, rng.choice([rng.randrange(0, 9), 8, 3]))
        world.put(pool + 0x3c, rng.randrange(0, 9))
        world.put(pool + 0x44, T + DEVICE_GROUP_ROWS)
        world.put(pool + 0x48, T + DEVICE_GROUP_BITMAP)
        groups = []
        for row in range(8):
            salt = rng.choice([0, 0x8001 + row, 0xc123 + row]) & 0xffff
            put16(world, T + DEVICE_GROUP_ROWS + 12 * row, salt)
            world.track[DEVICE_GROUP_ROWS + 12 * row + 2] = rng.choice([0, 4, 4, 0xff, 0xfb])
            groups.append(((salt or 0x7777) << 16) | row)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body + 0x130, rng.choice(groups + [NONE]))
            world.put(body + 0x13c, rng.choice(groups + [NONE]))
        return [rng.choice(live(world))]

    def machine_failed_case(name, world, rng):
        """+1DA/+1DC pathfinding range: [[0x4ECC18]+220]+C structure words
        name bits of [0x505E7C]; [0x4F9B58]+28 words are cleared."""
        fresh(world, rng)
        world.globals[0x4f9b58] = T + MACHINE_WORDS
        world.globals[0x505e7c] = T + MACHINE_BITS + 0x100
        world.globals[0x4ecc18] = T + STRUCTURE
        world.put(T + STRUCTURE + 0x220, T + STRUCTURE_PATHFINDING)
        world.put(T + STRUCTURE_PATHFINDING + 0xc, T + PATHFINDING_TABLE)
        for word in range(48):
            put16(world, T + PATHFINDING_TABLE + 2 * word, rng.choice([0, 1, 31, 32, 33, 63, 200, 0xffff, 0xffe0, 0xffdf]))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put16(world, body + 0x1da, rng.choice([0, 1, 3, 7, 16, 20]))
            put16(world, body + 0x1dc, rng.choice([0, 1, 1, 2, 5, 12, 0xffff, 0x8000]))
        return [rng.choice(live(world))]

    def scenery_failed_case(name, world, rng):
        fresh(world, rng)
        scenery_rows(world, rng)
        world.globals[0x52be74] = 0  # no Havok world: 1CE340 is never reached
        world.extra[0x46e468] = bytearray(rng.getrandbits(8) for _ in range(4))
        for n in range(v.ROWS - 1):
            world.data[world.body(n) + 0x12c - v.DATA] = rng.choice([0, 2, 0xfd, 0xff, 1])
        return [rng.choice(live(world))]

    # ---- owners ----
    havok, havok_cases = borrow(v, 'havok')
    PHYSICS_TYPES = [0, 1, 7, 11, 12]   # the 0x1883 type mask

    def wake_bodies_case(name, world, rng):
        """BBBC0 on objects whose component index 0 is the havok track's
        component row (+4 bit 5, +70 rows of 0x60 with body +40, +74 count)
        over its physics world: DL 0 activates every body (1D9F10, 315CB0)."""
        component = havok_cases.TRACK_ARGUMENTS['havok_component_activate'](name, world, rng)[0]
        object_types(world, rng)
        network(world, rng)
        header = T + 0x4800                    # [0x52BE5C]: +44 rows, row 0 = the component
        world.globals[0x52be5c] = header
        world.put(header + 0x44, component)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body + 0xb4, rng.choice([NONE, 0, 0, (rng.getrandbits(16) << 16)]))
            world.data[body + 0xc0 - v.DATA] = rng.choice([0x40, 0x40, 0x40, 0xff, 0, 0xbf])
        n = rng.randrange(v.ROWS - 1)
        if rng.random() < .8:
            world.data[v.RECORDS + 12 * n + 3 - v.DATA] = rng.choice(PHYSICS_TYPES)
        return [world.handles[n], rng.choice([0, 0, 0, 0x5a00, 1, 0x100])]

    def wake_case(name, world, rng):
        """BBBC0 with DL 0/1 over every type mask; DL 0 physics components
        never activate a body (315CB0 refuses)."""
        fresh(world, rng)
        object_types(world, rng)
        components(world, rng)
        network(world, rng)
        dl = rng.choice([0, 1, 0x100, 0xff01, 0x5a00])
        return [rng.choice(live(world)), dl]

    def set_animation_case(name, world, rng):
        """10F710's failure paths: NONE object/graph, name 0, no manager,
        and names absent from the graph and its imports. Another graph needs
        1D3380 (refused); a found animation ends in BF050 (a core stop)."""
        fresh(world, rng)
        animation_graphs(world, rng, names=ANIMATION_NAMES)
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        if rng.random() < .15:
            put16(world, body + 0x12a, 0xffff)
        manager_record(world, rng, body + 0x380)
        own = struct.unpack_from('<I', world.data, body + 0x380 + 0x68 - v.DATA)[0]
        graph = rng.choice([own, own, own, NONE])
        object_ = rng.choice([world.handles[n]] * 8 + [NONE])
        name_ = rng.choice(ABSENT_NAMES * 3 + [0])
        return [object_, graph, name_, rng.getrandbits(32), rng.choice([NONE, NONE, 0x12345]), rng.getrandbits(32),
                rng.getrandbits(32)]

    def manager_find_case(name, world, rng):
        fresh(world, rng)
        animation_graphs(world, rng)
        manager_record(world, rng, T + MANAGER)
        if rng.random() < .1:
            # Tag row 24 has no data: 1E5360 gets a null graph (1E5448).
            world.put(v.TAG_TABLE + 16 * 24 + 8, 0)
            world.put(T + MANAGER + 0x68, 0x77180018)
        return [T + MANAGER, rng.choice(ANIMATION_NAMES + ABSENT_NAMES + [0]), T + 0xf080]

    def manager_start_case(name, world, rng):
        """1D4D90 on the weapon track's managers and graphs, with the four
        flag words 10F710 builds. 10F710 only calls it with an attached
        manager (+68 is the graph it just checked)."""
        fx = weapon.Fixture(v, world, rng)
        manager = fx.manager()
        packed = fx.handle(fx.main)
        flags = rng.choice([0x3f, 0x3d, 0x4001, 0x4003])
        fx.finish()
        return [manager, packed, flags]

    def overlays_case(name, world, rng):
        fresh(world, rng)
        return [T + MANAGER]

    def set_frame_case(name, world, rng):
        fresh(world, rng)
        animation_graphs(world, rng)
        manager_record(world, rng, T + MANAGER)
        struct.pack_into('<f', world.data2, v.TIME + 4 - v.DATA2, rng.choice([1 / 30, 1 / 60, 0.0, 1.0, 1e-40]))
        seconds = rng.choice([0.0, -0.0, 1 / 30, 0.5, 0.9999, 1.0, 2.0, -1.0, 1e-40, rng.uniform(0, 1.1)])
        return [T + MANAGER, fbits(seconds)]

    def header_activate_case(name, world, rng):
        """B92C0: header flags/+4 word/parents/types, +4 bit 14 with the
        [0x4EABC4] counter, and components whose flag bit 17 keeps 1CC310
        (refused) unreached."""
        fresh(world, rng)
        world.globals[0x52be5c] = T + COMPONENTS
        world.put(T + COMPONENTS + 0x44, T + COMPONENT_ROWS)
        for row in range(8):
            world.put(T + COMPONENT_ROWS + 0xa0 * row + 4, rng.getrandbits(32) | 0x20000)
        for n in range(v.ROWS - 1):
            row = v.RECORDS + 12 * n
            world.data[row + 2 - v.DATA] = rng.choice([0, 0, 1, 2, 0xfe, 0xff])
            world.data[row + 3 - v.DATA] = rng.choice(list(range(13)) + [0x26])
            put16(world, row + 4, rng.choice([0xffff, 0, 3, 0x1234]))
            body = world.body(n)
            world.put(body + 4, rng.choice([0, 0x4000, 0xffffffff, rng.getrandbits(32)]))
            world.put(body + 0xb4, rng.choice([NONE, rng.randrange(8)]))
        return [rng.choice(live(world))]

    def deactivate_case(name, world, rng):
        fresh(world, rng)
        for n in range(v.ROWS - 1):
            world.put(world.body(n) + 0xb4, NONE)  # a component reaches 1D9510 (refused)
            world.data[world.body(n) + 0xc0 - v.DATA] = rng.choice([0, 0x40, 0xff, 0xbf])
        return [rng.choice(live(world))]

    def lightmap_row_case(name, world, rng):
        fresh(world, rng)
        rows = lightmap(world, rng)
        checksum = identifier(world, rng, T + IDENTIFIER, rows)
        if rng.random() < .3:
            checksum = rng.choice([c for _, c in rows] + [0x11111111, rng.getrandbits(32)])
        return [T + IDENTIFIER, checksum]

    specs = [
        ('objtypes_device_new', new_routine(0x10b5c0), device_case, 'boolean', 300),
        ('objtypes_machine_new', new_routine(0x117870), machine_case, 'boolean', 300),
        ('objtypes_sound_scenery_new', new_routine(0x20fe50), sound_scenery_case, 'boolean', 200),
        ('objtypes_crate_new', new_routine(0x11e320), crate_case, 'boolean', 500),
        ('objtypes_scenery_new', new_routine(0x10f260, closure_stops=ANIMATION_STOPS), scenery_case, 'boolean', 600),
        ('objtypes_object_set_animation', dict(entry=0x10f710, regs={'eax': 1}, stack=[0, 2, 3, 4, 5, 6],
         closure_stops=ANIMATION_STOPS), set_animation_case, 'boolean', 600),
        ('objtypes_animation_manager_find', dict(entry=0x1d3fc0, regs={'ecx': 0, 'eax': 1, 'esi': 2},
         closure_stops=ANIMATION_STOPS), manager_find_case, 'long', 500),
        ('objtypes_animation_manager_start', dict(entry=0x1d4d90, regs={'esi': 0}, stack=[1, 2],
         closure_stops=ANIMATION_STOPS), manager_start_case, 'boolean', 600),
        ('objtypes_animation_manager_clear_overlays', dict(entry=0x1d4f00, regs={'eax': 0}), overlays_case, 'void', 100),
        ('objtypes_animation_manager_set_frame', dict(entry=0x1d4e30, regs={'eax': 0}, xmm={0: 1},
         closure_stops=ANIMATION_STOPS), set_frame_case, 'void', 500),
        ('objtypes_object_header_activate', dict(entry=0xb92c0, regs={'edi': 0}, closure_stops=ANIMATION_STOPS),
         header_activate_case, 'void', 400),
        ('objtypes_object_physics_deactivate', dict(entry=0xba870, regs={'eax': 0}, closure_stops=ANIMATION_STOPS),
         deactivate_case, 'void', 100),
        ('objtypes_garbage_new', new_routine(0x11e270), garbage_case, 'boolean', 1000),
        ('objtypes_projectile_new', new_routine(0xfd200), projectile_case, 'boolean', 1200),
        ('objtypes_projectile_spin', dict(entry=0x100d40, regs={'eax': 0}), spin_case, 'void', 800),
        ('objtypes_device_new_failed', dict(entry=0x10b830, stack=[0]), device_failed_case, 'void', 500),
        ('objtypes_machine_new_failed', dict(entry=0x117900, stack=[0]), machine_failed_case, 'void', 400),
        ('objtypes_scenery_new_failed', dict(entry=0x10f2f0, stack=[0], closure_stops=[0x1ce340]), scenery_failed_case, 'void', 300),
        ('objtypes_object_wake', dict(entry=0xbbbc0, regs={'edx': 1}, stack=[0]), wake_case, 'void', 600),
        ('objtypes_object_wake_bodies', dict(entry=0xbbbc0, regs={'edx': 1}, stack=[0]), wake_bodies_case, 'void', 500),
        ('objtypes_lightmap_scenery_object_row', dict(entry=0x10f960, stack=[0, 1]), lightmap_row_case, 'long', 400),
    ]
    for name, routine, case, kind, count in specs:
        v.KERNELS[name] = ([name], kind)
        v.ROUTINES[name] = routine
        v.TRACK_ARGUMENTS[name] = case
        v.TRACK_COUNTS[name] = count
