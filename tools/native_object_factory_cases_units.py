"""Oracle cases for factory track "units" (unit, biped and vehicle callbacks).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Fixture layout: unit bodies keep their animation manager at body +280
(+12A) and their aim block at +300 (+33E). Animation graphs are tag rows
24..27 whose data lives in the TRACK bank: animation rows (0x6C), resource
rows (20 bytes, never resident, so 126200 is unreachable), inheritance rows
(32 bytes) and the two-level IK point blocks at graph +34. IK entries never
carry both markers, so the refused marker solve (BAD60, 1E1390) is never
reached. The resource closure under 1E5C80 has virtual calls no case reaches;
they are declared like track nodebuild's.
"""
import struct

from native_object_factory_track_fixtures import borrow

NONE = 0xffffffff
ANY = 0x30000d9
MANAGER, AIM = 0x280, 0x300
# TRACK bank layout (offsets).
GRAPHS, ANIMATIONS, RESOURCES, INHERITED = 0x0000, 0x1000, 0x3000, 0x3800
IK_ROWS, IK_SUBROWS, IK_ENTRIES, SCRATCH = 0x4000, 0x5000, 0x8000, 0xf000
GRAPH_ROWS, GRAPH_COUNT, ANIMATION_COUNT, EMPTY_ROW = 24, 4, 16, 28
RESOURCE_VIRTUAL_OWNERS = {0x13fe50, 0x140310, 0x2dbc84, 0x2dbf74, 0x2e02c4, 0x2e0342, 0x2e0362}
# 113B70 zero set, 1139D0 aiming set and second (mode) set, C9550 rows.
LOOK_MODES = [0xa000767, 0x600008c, 0x50000c3, 0x400004a, 0x400076c, 0x400076d, 0x5000768, 0x5000769,
              0x500076a, 0x500076b, 0x700076e, 0x80000c4, 0x800076f, 0xe00002a, 0xc000043, 0xd00002b,
              0xd000042, 0xe00003b, 0xe0000c3, 0xf00003a]
AIM_STATES = [0xc000075, 0xa000040, 0x6000006, 0x6000007, NONE, 0x500000a, 0x9000004, 0x9000005, 0x9000008,
              0x9000009, 0xb0005b2, 0xa0005b9, 0xa0005bb, 0xc000073, 0xe00067d, 0xc0006cd, 0xc000077,
              0xc0006b3, 0xe000038, 0xe000607, 0xe000608, 0xe000609, 0xe00060a, 0x130005bc, 0x11000074,
              0x11000076, 0x140005b3]
AIM_NEAR = [0x9000006, 0x9000007, 0x900000a, 0x9000003, 0x6000005, 0x6000008, 0xe000606, 0xe00060b, 0x500000b,
            0xa0005ba, 0x11000075, 0x7fffffff, 0x80000000, 0]
AIM_MODES = [0xc00005b, 0xb000059, 0xb00005a, 0x400004a, 0x600008c, 0xa00003e, 0xc000043, 0xc000058, 0xe0000c2,
             0xd00003f, 0xd000042, 0xe00003b, 0xf00003a, 0x1000003c]
MODE_NEAR = [0x400076b, 0x400076e, 0x5000767, 0x500076c, 0xb000058, 0xb00005b, 0xd000040, 0x7000001,
             0x7000101, 0x80000000, 0x7fffffff, 0]
IK_MODES = [0x1000001, 0x2000002, ANY, 0x4000004, 0x80000005]
IK_CLASSES = [0x1100011, ANY, 0x3300033, 0x7fff0007]


def f2u(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def register(v):
    T = v.TRACK
    v.UNREACHED_VIRTUAL_OWNERS.update(RESOURCE_VIRTUAL_OWNERS)
    v.JUMP_TABLES[0x1a3bfc] = 4     # 1A3B90 (126200 closure, unreached)
    v.JUMP_TABLES[0x113b58] = 2     # 1139D0 aiming states 0x9000004..9
    v.JUMP_TABLES[0x1e16cc] = 4     # 1E1620 fade curves 1..4
    v.JUMP_TABLES[0x1e5350] = 4     # 1E5250 IK point stages
    v.JUMP_TABLES[0x1ed84c] = 5     # 1ED780 leaving biped physics modes 1..5
    v.JUMP_TABLES[0x1ed864] = 5     # 1ED780 entering biped physics modes 1..5
    v.JUMP_TABLES[0x1d3644] = 6     # 1D34E0 overlay restart kinds (unreached)

    def kernel(name, types, kind, entry, count, **routine):
        v.KERNELS[name] = (types, kind)
        v.ROUTINES[name] = dict(entry=entry, **routine)
        v.TRACK_COUNTS[name] = count

    kernel('units_nodes_changed', ['units_nodes'], 'void', 0x1157d0, 800, stack=[0, 1, 2])
    kernel('units_fade_start', ['units_fade'], 'void', 0x1e14f0, 600, regs={'ecx': 0, 'ebx': 1}, stack=[2])
    kernel('units_fade_weight', ['units_fade'], 'xmm0', 0x1e1620, 600, regs={'eax': 0})
    kernel('units_graph_find', ['units_find'], 'void', 0x1e5360, 400, regs={'edx': 0}, stack=[1, 2])
    kernel('units_weapon', ['units_weapon'], 'long', 0xce230, 100, regs={'eax': 0, 'ecx': 1})
    kernel('units_ik_next_weapon', ['units_ik'], 'boolean', 0x1e5250, 600, regs={'esi': 0, 'ebx': 1})
    kernel('units_ik_next_look', ['units_ik'], 'boolean', 0x1e51d0, 400, regs={'esi': 0, 'ebx': 1})

    banks = lambda world: [(v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (v.AI, world.ai),
                           (T, world.track), (v.FACTORY, world.factory), *world.extra.items(), *world.absolute.items()]

    def put(world, address, value, size=4):
        for base, bank in banks(world):
            if bank is not None and base <= address and address + size <= base + len(bank):
                bank[address - base:address - base + size] = (value & ((1 << 8 * size) - 1)).to_bytes(size, 'little')
                return
        raise ValueError(f'Undeclared world address {address:x}')

    def tag(row):
        return ((0x7100 + row) << 16) | row

    def fresh(world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        world.extra[0x51df20] = bytearray([rng.choice([0, 1, 1, 0x80]), rng.choice([0, 1, 1, 0xff]), 0, 0])

    def graph_address(g):
        return T + GRAPHS + 0x200 * g

    def graphs(world, rng):
        """Four animation graphs (tag rows 24..27) and tag row 28 without
        data (only manager +68 names it: 116060 stops there, while 1E3150
        would dereference it). Inheritance rows name other graphs; ids
        handed out below only use valid inheritance."""
        put(world, v.TAG_TABLE + 16 * EMPTY_ROW + 8, 0)
        for g in range(GRAPH_COUNT):
            graph = graph_address(g)
            put(world, v.TAG_TABLE + 16 * (GRAPH_ROWS + g) + 8, graph)
            put(world, graph + 0xc, rng.choice([0, 1, 20, 64, 256, 0xfffb, 300, 2048, 0xffe1]), 2)
            put(world, graph + 0x2c, rng.choice([0, 1, 4, ANIMATION_COUNT, ANIMATION_COUNT, 0xffffffff]))
            put(world, graph + 0x30, T + ANIMATIONS + 0x6c0 * g)
            put(world, graph + 0xb0, T + RESOURCES + 0x140 * g)
            inherited = rng.choice([0, 1, 2, 3, 3])
            put(world, graph + 0x4c, inherited)
            put(world, graph + 0x50, T + INHERITED + 0x80 * g)
            for k in range(4):
                put(world, T + INHERITED + 0x80 * g + 32 * k + 4,
                    tag(GRAPH_ROWS + rng.randrange(GRAPH_COUNT)) if rng.random() < .85 else NONE)
            for a in range(ANIMATION_COUNT):
                row = T + ANIMATIONS + 0x6c0 * g + 0x6c * a
                put(world, row, rng.choice([0x5000001, 0x5000002, 0x6000003, 0x7000004, rng.getrandbits(32)]))
                put(world, row + 0x16, rng.choice([0, 0x10, 0x10, 0xef, 0xff]), 1)
                put(world, row + 0x18, rng.choice([0, 4, 8, 0xc, 0xfff3, 0xffff]), 2)
                put(world, row + 0x24, rng.randrange(16))
                put(world, row + 0x30, rng.choice([0xffff, 0xffff, rng.randrange(ANIMATION_COUNT)]), 2)
                later = list(range(a + 1, ANIMATION_COUNT))
                put(world, row + 0x32, rng.choice(later) if later and rng.random() < .5 else 0xffff, 2)
            for r in range(16):
                put(world, T + RESOURCES + 0x140 * g + 20 * r + 0x10, 0, 1)
            ik_points(world, rng, graph, g)

    def ik_points(world, rng, graph, g):
        """graph +34: rows (0x14) keyed by mode, sorted; each row's +4 block
        holds rows keyed by weapon class, and +C/+10 the look channel's
        8-byte entries. Entries never carry both markers."""
        def entries(at, count):
            for e in range(count):
                marker = rng.choice([NONE, 0, 0x6600001 + e])
                target = rng.choice([NONE, 0, 0x6700001 + e, 0x6700002, 0x6700003])
                if marker not in (NONE, 0) and target not in (NONE, 0):
                    target = rng.choice([NONE, 0])
                put(world, at + 8 * e, marker)
                put(world, at + 8 * e + 4, target)

        keys = sorted(rng.sample(IK_MODES, rng.randrange(0, len(IK_MODES) + 1)),
                      key=lambda k: k - (1 << 32) if k & 0x80000000 else k)
        put(world, graph + 0x34, len(keys) if rng.random() < .9 else rng.choice([0, 0xffffffff]))
        put(world, graph + 0x38, T + IK_ROWS + 0x200 * g)
        for r, key in enumerate(keys):
            row = T + IK_ROWS + 0x200 * g + 0x14 * r
            put(world, row, key)
            classes = sorted(rng.sample(IK_CLASSES, rng.randrange(0, len(IK_CLASSES) + 1)))
            put(world, row + 4, len(classes))
            put(world, row + 8, T + IK_SUBROWS + 0x400 * g + 0x80 * r)
            look = rng.choice([0, 1, 2, 3, 0xffffffff])
            put(world, row + 0xc, look)
            put(world, row + 0x10, T + IK_ENTRIES + 0x800 * g + 0x100 * r)
            entries(T + IK_ENTRIES + 0x800 * g + 0x100 * r, 3)
            for c, key2 in enumerate(classes):
                sub = T + IK_SUBROWS + 0x400 * g + 0x80 * r + 0x14 * c
                put(world, sub, key2)
                put(world, sub + 0xc, rng.choice([0, 1, 2, 3, 0xfffffffe]))
                put(world, sub + 0x10, T + IK_ENTRIES + 0x800 * g + 0x100 * r + 0x20 * (c + 1))
                entries(T + IK_ENTRIES + 0x800 * g + 0x100 * r + 0x20 * (c + 1), 3)

    def animation_id(world, rng, g):
        """A packed id whose inheritance index (if any) names a valid graph."""
        high = rng.choice([0xffff, rng.randrange(ANIMATION_COUNT)])
        count = struct.unpack_from('<i', world.track, GRAPHS + 0x200 * g + 0x4c)[0]
        lows = [k for k in range(max(0, min(count, 4)))
                if struct.unpack_from('<I', world.track, INHERITED + 0x80 * g + 32 * k + 4)[0] != NONE]
        low = rng.choice([0xffff, 0xffff] + lows)
        return (high << 16) | low

    def unit_bodies(world, rng):
        for k in range(4):
            definition = v.OBJECT_TAGS + 0x200 * k
            put(world, definition + 0xbc, rng.choice([0, 0x800, 0xfffff7ff, rng.getrandbits(32)]))
            put(world, definition + 0x38, ((0x5000 + 4 + k) << 16) | (4 + k))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            g, g2 = rng.randrange(GRAPH_COUNT), rng.randrange(GRAPH_COUNT)
            manager, aim = body + MANAGER, body + AIM
            put(world, body + 0x12a, MANAGER, 2)
            put(world, body + 0x33e, AIM, 2)
            put(world, body + 0x1fc, rng.choice([0, 1, 2, 0xffff]), 2)
            put(world, body + 0x212, rng.choice([0xff, 0xff, 0, 1, 2, 3]), 1)
            put(world, body + 0x213, rng.choice([0xff, 0xff, 0, 1]), 1)
            for w in range(4):
                put(world, body + 0x218 + 4 * w, rng.choice(world.handles[:v.ROWS - 1] + [NONE, 0x12340009]))
            put(world, manager, tag(GRAPH_ROWS + g) if rng.random() < .9 else NONE)
            put(world, manager + 4, animation_id(world, rng, g) if rng.random() < .9 else 0xffff0000 | rng.getrandbits(16))
            put(world, manager + 0x68, rng.choice([tag(GRAPH_ROWS + g2)] * 8 + [NONE, tag(EMPTY_ROW)]))
            put(world, manager + 0x70, rng.choice(IK_MODES * 3 + [0x9999999]))
            put(world, manager + 0x74, rng.choice(IK_CLASSES * 3 + [0x8888888, NONE]))
            put(world, manager + 0x78, rng.choice([NONE, 0, 0x4440004]))
            put(world, manager + 0x7c, rng.choice(LOOK_MODES + AIM_MODES + MODE_NEAR))
            put(world, aim, rng.getrandbits(8), 1)
            for fade in (0x80, 0x84):
                put(world, aim + fade, rng.choice([0, 1, 2, 5, 0x40, 0x7f, 0x80, 0xff]), 1)
                put(world, aim + fade + 1, rng.choice([0, 0, 1, 2, 7, 0x20, 0x7e, 0x7f, 0x80, 0xff]), 1)
                put(world, aim + fade + 2, rng.choice([0, 1, 2, 3, 4, 5, 0xff]), 1)
                put(world, aim + fade + 3, rng.choice([0, 1, 1, 2, 0xff]), 1)
            g3 = rng.randrange(GRAPH_COUNT)
            put(world, aim + 0x9c, tag(GRAPH_ROWS + g3))
            put(world, aim + 0xa0, animation_id(world, rng, g3))
            put(world, aim + 0xa4, rng.choice(AIM_STATES + AIM_NEAR))
        for n in range(5):
            put(world, 0x473990 + 4 * n, rng.choice(LOOK_MODES + AIM_MODES + MODE_NEAR))
        struct.pack_into('<h', world.data2, v.TIME + 2 - v.DATA2, rng.choice([30, 30, 60, 0, -7, 1, 4, 8, 32767, -32768]))

    def nodes_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        unit_bodies(world, rng)
        unit = rng.choice(world.handles[:v.ROWS - 1])
        return [unit, rng.getrandbits(32), rng.getrandbits(32)]

    seconds = [0.267, 0.06675, 0.0, -0.0, 1.0, 0.5, 1e-40, -1e-40, 4.5, 100.0, -0.5, float('nan'), float('inf'),
               0.0166, 0.05, 2.1e-2]

    def fade_case(name, world, rng):
        fresh(world, rng)
        fade = T + SCRATCH
        for k, choices in enumerate(([0, 1, 2, 5, 0x40, 0x7f, 0x80, 0x81, 0xff], [0, 1, 2, 7, 0x20, 0x7e, 0x7f, 0x80, 0xff],
                                     [0, 1, 2, 3, 4, 5, 0xff, 0x80], [0, 1, 2, 3, 0xff])):
            put(world, fade + k, rng.choice(choices), 1)
        struct.pack_into('<h', world.data2, v.TIME + 2 - v.DATA2, rng.choice([30, 60, 0, -7, 1, 4, 8, 32767, -32768, 127, 500]))
        value = rng.choice(seconds) if rng.random() < .7 else rng.uniform(-1, 3)
        return [fade, rng.choice([0, 1, 1, 2, 0xff, 0x100]) | (rng.getrandbits(8) << 8), f2u(value)]

    def find_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        g = rng.randrange(GRAPH_COUNT)
        graph = rng.choice([graph_address(g)] * 9 + [0])
        # Inheritance rows must name live graphs: 1E5360 walks every one.
        for k in range(4):
            put(world, T + INHERITED + 0x80 * g + 32 * k + 4, rng.choice([tag(GRAPH_ROWS + rng.randrange(GRAPH_COUNT)), NONE]))
        for h in range(GRAPH_COUNT):
            put(world, v.TAG_TABLE + 16 * (GRAPH_ROWS + h) + 8, graph_address(h) if rng.random() < .9 else 0)
            count = rng.choice([0, 1, 4, ANIMATION_COUNT, 0xffffffff])
            put(world, graph_address(h) + 0x2c, count)
        names = [0x5000001, 0x5000002, 0x6000003, 0x7000004, 0x1234567]
        return [graph, T + SCRATCH + 0x10, rng.choice(names)]

    def weapon_case(name, world, rng):
        fresh(world, rng)
        index = rng.choice([0xffff, 0, 1, 2, 3, 0xfffe]) | (rng.getrandbits(16) << 16)
        return [rng.choice(world.handles[:v.ROWS - 1]), index]

    def ik_case(name, world, rng):
        """A cursor in the TRACK bank (the original keeps it on its stack):
        +8 index and +A stage words, +C mode, +10 weapon class."""
        fresh(world, rng)
        graphs(world, rng)
        cursor = T + SCRATCH + 0x40
        put(world, cursor + 8, rng.choice([0xffff, 0xffff, 0, 1, 2, 0xfffe]), 2)
        put(world, cursor + 0xa, rng.choice([0, 0, 0, 1, 2, 3, 4, 0xffff]), 2)
        put(world, cursor + 0xc, rng.choice(IK_MODES * 2 + [0x9999999]))
        put(world, cursor + 0x10, rng.choice(IK_CLASSES * 2 + [0x8888888, NONE]))
        return [cursor, graph_address(rng.randrange(GRAPH_COUNT))]

    register_owners(v, put, fresh, graphs, unit_bodies, tag, kernel, f2u, animation_id)
    _boundaries(v)
    v.TRACK_ARGUMENTS.update(units_nodes=nodes_case, units_ik=ik_case, units_fade=fade_case, units_find=find_case,
                             units_weapon=weapon_case)

# ---- Boundaries: kernels whose original stops at a refused callee's entry ----
# STOPS[kernel]: callee entries where the original's run ends (with STOP, its
# normal return); the native refuses each by name there and the harness
# kernel reports the original's boundary register. OPAQUE[kernel]: callees
# kept out of that kernel's authenticated closure (the validator's
# closure_stops: a stop entry or a refused branch no fixture reaches).
STOPS = {}
OPAQUE = {}


def _boundaries(v):
    """OPAQUE becomes the validator's closure_stops; STOPS rows end at their
    callee entries (or the normal return)."""
    for name, stops in OPAQUE.items():
        v.ROUTINES[name]['closure_stops'] = sorted(stops)
    original_cases = v.cases

    def cases(seed, evaluators, tails):
        rows = original_cases(seed, evaluators, tails)
        for row in rows:
            if row['kernel'] in STOPS:
                row['stop_before_entries'] = sorted(STOPS[row['kernel']] | {v.STOP})
                row['observe_stop_entry'] = True
        return rows

    v.cases = cases


def register_owners(v, put, fresh, graphs, unit_bodies, tag, kernel, f2u, animation_id):
    """Unit new/failed, biped and vehicle callbacks and their leaves."""
    T, D = v.TRACK, v.DATA
    floats = [0.0, -0.0, 1.0, -1.0, 0.5, 2.5, 1e-40, -1e-41, 3.4e38, float('inf'), float('nan'), 0.125, 12.75]

    nans = [0x7fc00000, 0xffc00001, 0x7fa00005, 0xff812345]

    def rfloat(rng):
        r = rng.random()
        if r < .05:
            return rng.choice(nans)
        return f2u(rng.choice(floats)) if r < .4 else f2u(rng.uniform(-20, 20))

    def vectors(world, rng):
        """[474D04] forward, [474D10] up, [474D30] default matrix, [48A0B0] word."""
        world.globals[0x474d04] = T + 0xe000
        world.globals[0x474d10] = T + 0xe010
        world.globals[0x474d30] = T + 0xe020
        world.globals[0x48a0b0] = rng.getrandbits(32)
        for k in range(3 + 3 + 13):
            put(world, T + 0xe000 + 4 * k if k < 3 else T + 0xe010 + 4 * (k - 3) if k < 6 else T + 0xe020 + 4 * (k - 6),
                rfloat(rng))

    def definitions(world, rng):
        for k in range(4):
            d = v.OBJECT_TAGS + 0x200 * k
            put(world, d + 0x38, ((0x5000 + 4 + k) << 16) | (4 + k) if rng.random() < .9 else v.NONE)
            put(world, d + 0x264, rng.choice([0, 1, 0x10, 0x11, 0xffffffef, rng.getrandbits(32)]))
            put(world, d + 0x270, rfloat(rng))
            put(world, d + 0x1f0, rng.choice([0, 2, 0xfffffffd, rng.getrandbits(32)]))
            put(world, d + 0x2ac, rng.getrandbits(32) & ~1)
            put(world, d + 0x1b4, rng.choice([0xffff, 0, 1, 2, 0x8000]) | rng.choice([0, 3, 0xff, 0x8000, 0x7f00]) << 16)
            put(world, d + 0xc0, rng.getrandbits(16), 2)
        for k in range(4):
            model = v.MODEL_TAGS + 0x80 * k
            put(world, model + 0x24, rng.choice([v.NONE, 0x12340001, 0]))
            put(world, model + 0x78, rng.choice([0, 1, 2, 4, 0xffffffff]))
            for node in range(4):
                put(world, v.MODEL_NODES + (4 * k + node) * 0x5c,
                    rng.choice([0x80001a2, 0x90001a3, 0x11110001, 0x80001a2 + 1]))

    def nodes_safe(world, rng):
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(world, body + 0x18, rng.choice([0, 1, 2, 3, 0xff]), 1)
            put(world, body + 0x10a, rng.getrandbits(8) & ~4, 1)
            put(world, body + 0xb4, v.NONE)

    # DE9E0 biped validate: stack request.
    kernel('units_biped_validate', ['units_biped_request'], 'void', 0xde9e0, 400, stack=[0])

    def biped_request_case(name, world, rng):
        fresh(world, rng)
        definitions(world, rng)
        request = T + 0xd000
        put(world, request, ((0x4000 + rng.randrange(4)) << 16) | rng.randrange(4))
        for k in range(1, 0x31):
            put(world, request + 4 * k, rfloat(rng))
        return [request]

    # DEA60 biped new: stack biped, request, failed byte.
    kernel('units_biped_new', ['units_biped'], 'boolean', 0xdea60, 600, stack=[0, 1, 2])
    OPAQUE['units_biped_new'] = {0x204610, 0x2045b0, 0x1cc310, 0xe65a0}

    def biped_case(name, world, rng):
        fresh(world, rng)
        definitions(world, rng)
        vectors(world, rng)
        nodes_safe(world, rng)
        return [rng.choice(world.handles[:v.ROWS - 1]), T + 0xd000, T + 0xd0f0]

    # 1ED780: EDI state, stack mode. Include transitions through 1CC310:
    # no component, active component (1D9510 refusal), and inactive component
    # (native 1D8470 reset and flag store, then the 1D73D0 refusal).
    kernel('units_physics_mode', ['units_physics_mode'], 'void', 0x1ed780, 400, regs={'edi': 0}, stack=[1])
    _, havok_cases = borrow(v, 'havok')
    physics_stops = {0x1d9510, 0x1d73d0, 0x1d97f0}
    STOPS['units_physics_mode'] = physics_stops
    OPAQUE['units_physics_mode'] = {0x204610, 0x2045b0} | physics_stops

    def physics_mode_case(name, world, rng):
        component, = havok_cases.TRACK_ARGUMENTS['havok_component_reset'](name, world, rng)
        # Keep the physics constants/header separate from the allocator's
        # free lists, which own the high end of TRACK in the borrowed world.
        constants = v.FACTORY
        world.factory = bytearray(rng.getrandbits(8) for _ in range(0x100))
        world.globals.update({0x474d04: constants, 0x474d10: constants + 0x10,
                              0x474d30: constants + 0x20, 0x48a0b0: rng.getrandbits(32),
                              0x52be5c: constants + 0x80})
        put(world, constants + 0xc4, component)
        n = rng.randrange(v.ROWS - 1)
        state = world.body(n) + 0x3dc
        old, new = rng.choice([(0, 1), (0, 2), (1, 1), (1, 2), (2, 1), (2, 2),
                               (3, 3), (0, 3), (1, 3), (2, 3), (3, 1), (3, 2)])
        kind = rng.randrange(3)
        put(world, world.body(n) + 0xb4, v.NONE if kind == 0 else 0x12340000)
        put(world, component + 4, (rng.getrandbits(32) & ~0x20) | (0x20 if kind == 1 else 0))
        put(world, state, old, 1)
        put(world, state + 4, world.handles[n])
        return [state, new | (rng.getrandbits(24) << 8)]

    v.TRACK_ARGUMENTS['units_physics_mode'] = physics_mode_case

    # DEA70 biped new failed / C68B0 unit new failed: stack object.
    kernel('units_biped_new_failed', ['units_failed'], 'void', 0xdea70, 200, stack=[0])
    kernel('units_new_failed', ['units_failed'], 'void', 0xc68b0, 200, stack=[0])
    OPAQUE['units_new_failed'] = {0xd0950, 0xb7e50, 0xd0ef0, 0xba570}

    def failed_case(name, world, rng):
        fresh(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(world, body + 0x34b, rng.choice([0, 1, 1, 2, 0xff]), 1)
            for w in range(4):
                put(world, body + 0x218 + 4 * w, v.NONE)
            put(world, body + 0x238, v.NONE)
        return [rng.choice(world.handles[:v.ROWS - 1])]

    v.TRACK_ARGUMENTS.update(units_biped_request=biped_request_case, units_biped=biped_case, units_failed=failed_case)

    # ---- C6440 unit new and its leaves ----
    NAMES = [0x800004d, 0x8000145, 0xc00024d]

    def animation_names(world, rng):
        """Some animation rows carry the names 111000/1102A0 look up."""
        for g in range(4):
            for a in range(16):
                if rng.random() < .15:
                    put(world, T + 0x1000 + 0x6c0 * g + 0x6c * a, rng.choice(NAMES))

    def variants(world, rng):
        """Model +50/+54 variant rows (0x38: +0 name, +2C override, +34
        dialogue tag) and +90/+A0; dialogue tags are rows 29..30 (+14)."""
        for k in range(4):
            model = v.MODEL_TAGS + 0x80 * k
            put(world, model + 0x50, rng.choice([0, 1, 2, 4, 0xffffffff]))
            put(world, model + 0x54, T + 0x6000 + 0x200 * k)
            put(world, model + 0x90, rng.choice([v.NONE, v.NONE, rng.getrandbits(32)]))
            put(world, model + 0xa0, rng.getrandbits(32))
            for r in range(4):
                row = T + 0x6000 + 0x200 * k + 0x38 * r
                put(world, row, rng.choice([0x1000101, 0x2000202, 0x3000303]))
                put(world, row + 0x2c, rng.choice([0, v.NONE, 0x7770007]))
                put(world, row + 0x34, rng.choice([v.NONE, tag(29), tag(30)]))
        for row in (29, 30):
            put(world, v.TAG_TABLE + 16 * row + 8, T + 0x6800 + 0x40 * (row - 29))

    # 1708F0: ECX model tag, EDI name.
    kernel('units_marker_index', ['units_marker'], 'long', 0x1708f0, 200, regs={'ecx': 0, 'edi': 1})

    def marker_case(name, world, rng):
        fresh(world, rng)
        definitions(world, rng)
        return [rng.choice([((0x5000 + 4 + k) << 16) | (4 + k) for k in range(4)] + [v.NONE]),
                rng.choice([0x80001a2, 0x90001a3, 0x11110001])]

    # 111940 unit animation reset: stack unit. 1175A0: EAX unit, stack variant.
    kernel('units_animation_reset', ['units_reset'], 'void', 0x111940, 400, stack=[0])
    kernel('units_variant_record', ['units_variant'], 'void', 0x1175a0, 400, regs={'eax': 0}, stack=[1])

    def reset_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        animation_names(world, rng)
        unit_bodies(world, rng)
        definitions(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(world, body + 0x280 + 0x68, rng.choice([tag(24 + rng.randrange(4))] * 4 + [tag(EMPTY_ROW)]))
            put(world, body + 0x12a, 0x280, 2)
        return [rng.choice(world.handles[:v.ROWS - 1])]

    def variant_case(name, world, rng):
        fresh(world, rng)
        definitions(world, rng)
        variants(world, rng)
        for k in range(4):
            put(world, v.OBJECT_TAGS + 0x200 * k + 0x38, ((0x5000 + 4 + k) << 16) | (4 + k))
        for n in range(v.ROWS - 1):
            put(world, world.body(n) + 0x342, rng.choice([0x200, 0x240, 0x1f0]), 2)
        return [rng.choice(world.handles[:v.ROWS - 1]), rng.choice([0x1000101, 0x2000202, 0x3000303, 0x4444])]

    v.TRACK_ARGUMENTS.update(units_marker=marker_case, units_reset=reset_case, units_variant=variant_case)

    # 113B70 / 1139D0 (EDI unit): every mode set member, focused.
    kernel('units_look_enabled', ['units_modes'], 'boolean', 0x113b70, 500, regs={'edi': 0})
    kernel('units_aim_enabled', ['units_modes'], 'boolean', 0x1139d0, 500, regs={'edi': 0})

    def modes_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        unit_bodies(world, rng)
        n = rng.randrange(1, v.ROWS - 1)
        body = world.body(n)
        if rng.random() < .8:
            put(world, body + 0x14, world.handles[rng.randrange(n)])
            put(world, body + 0x1fc, rng.choice([0, 1, 2]), 2)
        g = rng.randrange(4)
        put(world, body + 0x280, tag(24 + g))
        put(world, body + 0x284, animation_id(world, rng, g))
        pools = [LOOK_MODES, AIM_MODES, MODE_NEAR]
        put(world, body + 0x280 + 0x7c, rng.choice(rng.choice(pools)))
        put(world, body + 0x300 + 0xa4, rng.choice(rng.choice([AIM_STATES, AIM_NEAR])))
        if rng.random() < .7:
            for k in range(5):
                put(world, 0x473990 + 4 * k, rng.choice(MODE_NEAR))
        return [world.handles[n]]

    # C6440 unit new: stack unit, request, failed byte (+ the native's BE3B0
    # scratch slot: the original's stack slot SP - 0x40). The unit body is the
    # first block of a private object memory pool in the FACTORY bank.
    kernel('units_new', ['units_new'], 'boolean', 0xc6440, 600, stack=[0, 1, 2])
    STOPS['units_new'] = {0x111b10}
    OPAQUE['units_new'] = {0x111b10}

    def pool(world, rng, n, body_size):
        F = v.FACTORY
        world.factory = bytearray(rng.getrandbits(8) for _ in range(v.FACTORY_SIZE))
        world.globals[0x41b774] = v.TICK
        put(world, v.TICK, rng.getrandbits(32))
        put(world, 0x4eabbc, v.MEMORY_POOL)
        base = v.HEAP
        unit_block, total = base, 0x10 + ((body_size + 0xf) & ~0xf)
        blocks = [(unit_block, total, v.RECORDS + 12 * n + 8)]
        if rng.random() < .4:
            blocks.append((unit_block + total + rng.choice([0, 0x20]), 0x40, v.POOL_SLOTS))
        for k, (block, size, owner) in enumerate(blocks):
            put(world, block, size)
            put(world, block + 4, owner)
            put(world, block + 8, blocks[k + 1][0] if k + 1 < len(blocks) else 0)
            put(world, block + 0xc, blocks[k - 1][0] if k else 0)
            if owner == v.POOL_SLOTS:
                put(world, owner, block + 0x10)
        used = sum(size for _, size, _ in blocks)
        span = rng.choice([0x3000, 0x1000, 0x800, total + 0x40])
        put(world, v.MEMORY_POOL + 0x28, base)
        put(world, v.MEMORY_POOL + 0x2c, span)
        put(world, v.MEMORY_POOL + 0x30, span - used)
        put(world, v.MEMORY_POOL + 0x34, blocks[0][0])
        put(world, v.MEMORY_POOL + 0x38, blocks[-1][0])
        for k in range(4):
            world.factory[v.MEMORY_POOL + 0x3c + k - F] = 0
        row = v.RECORDS + 12 * n
        put(world, row + 8, unit_block + 0x10)
        put(world, row + 6, body_size, 2)
        return unit_block + 0x10

    def unit_new_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        animation_names(world, rng)
        definitions(world, rng)
        variants(world, rng)
        for k in range(4):
            put(world, v.OBJECT_TAGS + 0x200 * k + 0x38, ((0x5000 + 4 + k) << 16) | (4 + k))
        n = rng.randrange(v.ROWS - 1)
        body = pool(world, rng, n, 0x3e0)
        put(world, body, ((0x4000 + n % 4) << 16) | (n % 4))
        put(world, body + 0x12a, rng.choice([0x280] * 9 + [0xffff]), 2)
        put(world, body + 0x280 + 0x68, rng.choice([tag(24 + rng.randrange(4))] * 4 + [tag(EMPTY_ROW)]))
        request = T + 0xd000
        put(world, request + 0xc, rng.choice([0x1000101, 0x2000202, 0x3000303, 0x4444]))
        put(world, request + 0x64, rng.choice([0, 0xffff, 1, 7]), 2)
        put(world, v.PVS + 8, rng.choice([1, 1, 2, 0]))
        return [world.handles[n], request, T + 0xd0f0, v.SP - 0x40]

    # F10E0 vehicle new: stack vehicle, request, failed byte. 1D6200: EDI
    # manager. BBF20: EAX object, ECX out.
    kernel('units_vehicle_new', ['units_vehicle'], 'boolean', 0xf10e0, 600, stack=[0, 1, 2])
    STOPS['units_vehicle_new'] = {0x111b10}
    OPAQUE['units_vehicle_new'] = {0x111b10, 0x1d9f10}
    kernel('units_vehicle_settle', ['units_settle'], 'void', 0x1d6200, 400, regs={'edi': 0})
    kernel('units_root_origin', ['units_origin'], 'long', 0xbbf20, 400, regs={'eax': 0, 'ecx': 1})

    def havok(world, rng):
        """[52BE5C] component rows (0xA0: +18 rigid body index, +74 count, +70
        rows of 0x60 whose +40 body has its motion (+3C) origin at +70..+78)."""
        H = T + 0xa000
        world.globals[0x52be5c] = H
        put(world, H + 0x44, H + 0x100)
        for c in range(4):
            record = H + 0x100 + 0xa0 * c
            put(world, record + 0x18, rng.choice([0, 1, 2, 3, 0x80, 0xff]), 1)
            put(world, record + 0x70, H + 0x400 + 0x200 * c)
            put(world, record + 0x74, rng.choice([0, 1, 2, 3, 4]))
            for b in range(4):
                rigid, motion = H + 0xc00 + 0x80 * (4 * c + b), H + 0x1400 + 0x80 * (4 * c + b)
                put(world, H + 0x400 + 0x200 * c + 0x60 * b + 0x40, rigid)
                put(world, rigid + 0x3c, motion)
                for k in range(3):
                    put(world, motion + 0x70 + 4 * k, rfloat(rng))
        for n in range(v.ROWS - 1):
            put(world, world.body(n) + 0xb4, rng.choice([v.NONE, v.NONE, 0, 1, 2, 3 | 0x12340000]))

    def vehicle_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        definitions(world, rng)
        nodes_safe(world, rng)
        havok(world, rng)
        for k in range(4):
            put(world, v.OBJECT_TAGS + 0x200 * k + 0x38, ((0x5000 + 4 + k) << 16) | (4 + k))
        for n in range(v.ROWS - 1):
            put(world, world.body(n) + 0x12a, rng.choice([0x280, 0x280, 0xffff]), 2)
        unit = rng.choice(world.handles[:v.ROWS - 1])
        # F10E0 passes DL=1 to BBBC0. A vehicle header selects the deferred
        # physics/no-physics paths without the unrelated item network path.
        put(world, v.RECORDS + 12 * (unit & 0xffff) + 3, 1, 1)
        for k in range(4):
            put(world, v.OBJECT_TAGS + 0x200 * k + 0x2ac, rng.getrandbits(32))
        return [unit, T + 0xd000, T + 0xd0f0]

    def settle_case(name, world, rng):
        fresh(world, rng)
        graphs(world, rng)
        manager = T + 0xc000
        for channel in (manager, manager + 0x20):
            g = rng.randrange(4)
            put(world, channel, rng.choice([tag(24 + g)] * 3 + [v.NONE]))
            put(world, channel + 4, rng.choice([0xffff0000 | rng.getrandbits(16)] + [rng.randrange(16) << 16 | 0xffff] * 3))
        for at in (0x11, 0x12, 0x31, 0x32, 0x51, 0x52):
            put(world, manager + at, rng.getrandbits(8), 1)
        for g in range(4):
            for a in range(16):
                put(world, T + 0x1000 + 0x6c0 * g + 0x6c * a + 0x14, rng.choice([0, 1, 2, 30, 0x7fff, 0x8000, 0xffff]), 2)
        return [manager]

    def origin_case(name, world, rng):
        fresh(world, rng)
        nodes_safe(world, rng)
        havok(world, rng)
        return [rng.choice(world.handles[:v.ROWS - 1]), T + 0xd200]

    # C6850 unit place: stack unit. CEF90's starting weapons stop at
    # object_new (B9B70, refused). E8E50 (action 8: B9390, EAF00, 1EFA70)
    # and CECF0 are the unitactions track's natives, run over that track's
    # world (borrowed read only: definitions at tag rows 40..43, seats, the
    # object forest, action blocks, players); they stop where that track's
    # refused callees start (its REFUSED and refused table callbacks).
    ua, ua_cases = borrow(v, 'unitactions')
    table = ua.action_table(v)
    ua_stops = ua.REFUSED | ({c for row in table for c in row[1:] if c} - ua.NATIVE_CALLBACKS)
    kernel('units_place', ['units_place'], 'void', 0xc6850, 400, stack=[0])
    STOPS['units_place'] = {0xb9b70} | ua_stops
    OPAQUE['units_place'] = {0xb9b70, 0xcfaa0, 0xcf5a0, 0xba570} | ua_stops | ua.UNREACHED

    def place_case(name, world, rng):
        ua_cases.TRACK_ARGUMENTS['ua_seat_drivers'](name, world, rng)
        # B9960's default vectors and CEF90's rows (definition +1C0 count,
        # +1C4 rows of 8: +4 weapon tag) in TRACK space that world leaves free.
        world.globals[0x474d08] = T + 0xa000
        world.globals[0x474d10] = T + 0xa010
        for k in range(4):
            d = T + 0x1000 * k
            put(world, d + 0x1c0, rng.choice([0, 0, 0, 1, 2, 0xffffffff]))
            put(world, d + 0x1c4, T + 0xa100 + 0x40 * k)
            for r in range(2):
                put(world, T + 0xa100 + 0x40 * k + 8 * r + 4, rng.choice([v.NONE, v.NONE, v.NONE, ((0x4000 + 1) << 16) | 1]))
        live = world.handles[:v.ROWS - 1]
        return [rng.choice(live[:4] + live)]

    # ---- 111B10 leaves ----
    # 115E90: EAX state, ECX previous mode, DL forced; stack urgent byte,
    # minimum seconds. Returns XMM0.
    kernel('units_blend_seconds', ['units_blend'], 'xmm0', 0x115e90, 600, regs={'eax': 0, 'ecx': 1, 'edx': 2}, stack=[3, 4])
    BLEND_NAMES = [0x9000020, 0x900001f, 0x90006b2, 0x400000c, 0xa000014, 0x9000015, 0x9000016, 0xa000017, 0x7000001,
                   0x700005c, 0x12345]

    def blend_case(name, world, rng):
        fresh(world, rng)
        return [rng.choice(BLEND_NAMES), rng.choice(BLEND_NAMES), rng.choice([0, 0, 1, 0x100, 0xff]) | (rng.getrandbits(8) << 8),
                rng.choice([0, 0, 1, 0x80]) | (rng.getrandbits(24) << 8), rfloat(rng)]

    # 1E16E0: ECX fade, XMM0. 1D38A0: EAX count; stack manager, seconds,
    # destination, source. BC380: ESI object; stack seconds. 115E20: EAX unit.
    kernel('units_fade_remaining', ['units_remaining'], 'xmm0', 0x1e16e0, 400, regs={'ecx': 0})
    kernel('units_interpolation_start', ['units_interpolation'], 'boolean', 0x1d38a0, 600, regs={'eax': 0}, stack=[1, 2, 3, 4])
    kernel('units_object_interpolation', ['units_object_interpolation'], 'void', 0xbc380, 500, regs={'esi': 0}, stack=[1])
    kernel('units_overlays_restart', ['units_overlays'], 'void', 0x115e20, 200, regs={'eax': 0})
    OPAQUE['units_overlays_restart'] = {0x1e3150, 0x1e3420, 0x1d36c0, 0x1d3660, 0x1ce720}

    def remaining_case(name, world, rng):
        fresh(world, rng)
        fade = T + 0xd500
        for k, choices in enumerate(([0, 1, 5, 0x7f, 0x80, 0xff], [0, 1, 2, 7, 0x7f, 0x80, 0xff], [0, 1, 4], [0, 1, 2, 3, 0xff])):
            put(world, fade + k, rng.choice(choices), 1)
        put(world, v.TIME + 4, rfloat(rng))
        return [fade]

    def interpolation_fixture(world, rng, manager):
        put(world, manager + 0x64, rng.choice([0, 1, 5, 0x7f, 0x80, 0xff]), 1)
        put(world, manager + 0x65, rng.choice([0, 0, 1, 7, 0x7f, 0x80]), 1)
        put(world, manager + 0x66, rng.choice([0, 1, 4]), 1)
        put(world, manager + 0x67, rng.choice([0, 1, 2, 3, 0xff]), 1)
        put(world, v.TIME + 4, rfloat(rng))
        struct.pack_into('<h', world.data2, v.TIME + 2 - v.DATA2, rng.choice([30, 60, 0, 1, 127, 500, -7]))

    def interpolation_case(name, world, rng):
        fresh(world, rng)
        manager = T + 0xd600
        interpolation_fixture(world, rng, manager)
        seconds = rng.choice([rfloat(rng), f2u(0.267), f2u(0.1335), f2u(0.0), f2u(1e-40)])
        return [rng.choice([0, 1, 2, 4, 0xffffffff, 0x80000000]), manager, seconds, T + 0xd800, T + 0xda00]

    def object_interpolation_case(name, world, rng):
        fresh(world, rng)
        definitions(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(world, body + 0x12a, 0x280, 2)
            put(world, body + 0x10e, 0x100, 2)
            put(world, body + 0x110, rng.choice([0, 0x20, 0x40, 0x60, 0x1f]), 2)
            put(world, body + 0x112, rng.choice([0x180, 0x180, 0xffff]), 2)
            interpolation_fixture(world, rng, body + 0x280)
        return [rng.choice(world.handles[:v.ROWS - 1]), rng.choice([rfloat(rng), f2u(0.267), f2u(0.5)])]

    def overlays_case(name, world, rng):
        fresh(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(world, body + 0x12a, 0x280, 2)
            put(world, body + 0x33e, 0x300, 2)
            for channel in (0x9c, 0xdc, 0xbc):
                put(world, body + 0x300 + channel, v.NONE)
        return [rng.choice(world.handles[:v.ROWS - 1])]

    v.TRACK_ARGUMENTS.update(units_blend=blend_case, units_remaining=remaining_case, units_interpolation=interpolation_case,
                             units_object_interpolation=object_interpolation_case, units_overlays=overlays_case)

    # ---- 111B10 and 115AF0 on track deviceanim's animation fixture ----
    # (track weapon's graphs, managers and channels, plus deviceanim's
    # overlay tables). The unit's manager sits at body +280 and its aim block
    # at +310 (115AF0's stores reach the next body's first bytes).
    # 111B10: EAX flags; stack unit, mode, two keys, state, minimum, urgent.
    kernel('units_animation_state', ['units_animation'], 'boolean', 0x111b10, 600, regs={'eax': 0},
           stack=[1, 2, 3, 4, 5, 6, 7])
    kernel('units_channels_refresh', ['units_animation'], 'void', 0x115af0, 400, stack=[0])
    v.JUMP_TABLES[0x115e04] = 3     # 115AF0 mode classes 0x9000015..0x9000020
    UNIT_NAMES = [0xc000026, 0xe000028, 0xb000027, 0xd000029, 0x400004b, 0x400060b, 0xb00054a, 0x10000551,
                  0xe000044, 0xe000045, 0xb000046]
    UNIT_STATES = [0x400000c, 0x700005c, 0x5000049, 0x400004a, 0xa000014, 0x9000015, 0xb000018, 0xc000034,
                   0x900001f, 0x7000101, 0x7000001, 0xb00001b, 0xb00002e, 0xc000058, 0xd000032, 0xd000035,
                   0xa000015, 0xb00001c, 0xc000057, 0xd000033]

    import native_object_factory_cases_deviceanim as deviceanim
    import native_object_factory_cases_weapon as weapon
    v.UNREACHED_VIRTUAL_OWNERS |= weapon.UNREACHED
    v.JUMP_TABLES.update(weapon.TABLES)

    def animation_fixture(world, rng):
        # deviceanim's fixture can overflow the track bank for some draws;
        # each attempt rebuilds the bank, the graph tag rows and its statics.
        for attempt in range(8):
            try:
                fx = deviceanim.Fixture(v, world, rng)
                break
            except ValueError:
                if attempt == 7:
                    raise
        leaves = fx.main['leaves']
        # Extra unit keys on up to three leaves, while the track bank has room
        # (an allocation that does not fit writes nothing).
        for row in rng.sample(leaves, min(5, len(leaves))):
            try:
                if rng.random() < .5:
                    keys = rng.sample(UNIT_NAMES + deviceanim.NAMES[:2], rng.randrange(1, 4))
                    fx.table(row + 0xc, keys, 8, lambda entry, key: fx.p32(entry + 4, fx.handle(fx.main)))
                if rng.random() < .3:
                    keys = rng.sample(UNIT_NAMES + weapon.STATE_KEYS[:6], rng.randrange(1, 4))
                    fx.table(row + 4, keys, 8, lambda entry, key: fx.p32(entry + 4, fx.handle(fx.main)))
            except ValueError:
                break
        n = rng.randrange(1, v.ROWS - 1)
        body = world.body(n)
        put(world, body + 0x12a, 0x280, 2)
        put(world, body + 0x33e, 0x310, 2)
        fx.manager(at=body + 0x280, graph=fx.main)
        manager, aim = body + 0x280, body + 0x310
        if rng.random() < .5:
            put(world, manager + 0x7c, rng.choice(UNIT_STATES))
        put(world, aim, rng.getrandbits(8), 1)
        put(world, aim + 0x98, rng.choice(weapon.L3_KEYS + [v.NONE, 0x7000101]))
        for channel in (0x9c, 0xdc, 0xbc):
            put(world, aim + channel, v.NONE)
        put(world, body + 0x13c, rng.choice([v.NONE, 0x12340001]))
        world.extra[0x51daf8] = bytearray([rng.choice([0, 1]), 0, 0, 0])
        if rng.random() < .6:
            put(world, body + 0x10e, 0x1c0, 2)
            put(world, body + 0x110, rng.choice([0x20, 0x40]), 2)
            put(world, body + 0x112, 0x180, 2)
        else:
            put(world, body + 0x112, 0xffff, 2)
        return fx, world.handles[n], weapon

    def animation_case(name, world, rng):
        fresh(world, rng)
        fx, unit, weapon = animation_fixture(world, rng)
        if name == 'units_channels_refresh':
            if rng.random() < .3:
                put(world, world.body(unit & 0xffff) + 0x280 + 0x7c, rng.choice([0xc000058, 0xc000026, 0x400000c]))
            fx.finish()
            return [unit]
        flags = rng.choice([0, 2, rng.getrandbits(12)])
        mode = rng.choice(weapon.L1_KEYS + [0x7000101, 0x7000001, v.NONE])
        pairs = {0xa000014: 0x9000015, 0x9000015: 0xa000014, 0x9000016: 0xa000017, 0xa000017: 0x9000016}
        state = rng.choice(weapon.STATE_KEYS[:6] + UNIT_STATES + list(pairs) * 2 + [0x9000020, 0x90006b2, 0x900001f])
        if state in pairs and rng.random() < .5:
            put(world, world.body(unit & 0xffff) + 0x280 + 0x7c, pairs[state])
        minimum = rfloat(rng) if rng.random() < .5 else f2u(rng.choice([0.0, 0.1, 0.5]))
        values = [flags, unit, mode, rng.choice(weapon.L2_KEYS + [0x7000101]), rng.choice(weapon.L3_KEYS + [0x7000101]),
                  state, minimum, rng.choice([0, 0, 1, 0x100])]
        fx.finish()
        return values

    v.TRACK_ARGUMENTS.update(units_modes=modes_case, units_animation=animation_case, units_new=unit_new_case, units_vehicle=vehicle_case,
                             units_settle=settle_case, units_origin=origin_case, units_place=place_case)
