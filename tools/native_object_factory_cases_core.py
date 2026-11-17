"""Oracle cases for factory track "core": type table, B9960, BBE00, BABA0, C0200."""
import re
import struct
from pathlib import Path


def check_type_table(v):
    """The native 0x474B90 literal must equal the authenticated XBE."""
    source = (Path(__file__).resolve().parents[1] / 'src/gameplay/objects/native_object_factory_core_tu15.cpp').read_text()
    block = source[source.index('// BEGIN 0x474B90'):source.index('// END 0x474B90')]
    levels_text = block[:block.index('constexpr Entry entries')]
    levels = [[int(a, 16) for a in re.findall(r'0x([0-9a-f]+)u', row)] for row in re.findall(r'\{(0x[^{}]*)\}', levels_text)]
    entries = {(int(t), int(n), int(o, 16)): int(val, 16) for t, n, o, val in
               re.findall(r'\{(\d+),(\d+),0x([0-9a-f]+),0x([0-9a-f]+)u\}', block)}
    image = v.Xbe(v.NEW_DEFAULT)
    word = lambda a: struct.unpack('<I', image.read(a, 4))[0]
    expected_levels, expected_entries = [], {}
    for t in range(13):
        d = word(0x474b90 + 4 * t)
        chain = [word(d + 0x84 + 4 * k) for k in range(3)]
        chain = chain[:chain.index(0)] if 0 in chain else chain
        expected_levels.append(chain + [0] * (3 - len(chain)))
        for n, level in enumerate(chain):
            for offset in range(0x10, 0x84, 4):
                if word(level + offset):
                    expected_entries[(t, n, offset)] = word(level + offset)
    if levels != expected_levels or entries != expected_entries:
        raise ValueError('Native 0x474B90 type table differs from the authenticated XBE')


def register(v):
    check_type_table(v)
    TRACK = v.TRACK

    def fresh(world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))

    def owner_handle(world, rng):
        return rng.choice(world.handles[:v.ROWS - 1] + [v.NONE, (0x4321 << 16) | 2, world.handles[v.ROWS - 1]])

    # B9960: esi request; stack tag, owner, location.
    v.KERNELS['placement_data_new'] = (['core_placement'], 'void')
    v.ROUTINES['placement_data_new'] = dict(entry=0xb9960, regs={'esi': 3}, stack=[0, 1, 2])

    def placement_case(name, world, rng):
        fresh(world, rng)
        world.globals[0x474d08] = TRACK + 0x10
        world.globals[0x474d10] = TRACK + 0x20
        for n in range(ROWS_MINUS := v.ROWS - 1):
            body = world.body(n)
            world.data[body + 0xc0 - v.DATA] = rng.choice([0, 4, 0xfb, 0xff])
            world.data[body + 0xaa - v.DATA] = rng.choice([0, 1, 2, 5, 0x21, 0xff])
        location = rng.choice([0, TRACK + 0x40])
        return [rng.getrandbits(32), owner_handle(world, rng), location, TRACK + 0x100]

    # BBE00: eax object, esi out.
    v.KERNELS['object_world_position'] = (['core_position'], 'long')
    v.ROUTINES['object_world_position'] = dict(entry=0xbbe00, regs={'eax': 0, 'esi': 1})
    floats = [0.0, 1.0, -1.0, 0.5, 2.5, 1e-40, -3.75, 1.0000001, 0.99999994, float('nan'), 123.25, -0.0]

    def fbits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    def position_case(name, world, rng):
        fresh(world, rng)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            for k in range(3):
                world.put(body + 0x64 + 4 * k, fbits(rng.choice(floats)) if rng.random() < .8 else rng.getrandbits(32))
            world.data[body + 0x18 - v.DATA] = rng.choice([0, 1, 2, 3, 0xff])
            struct.pack_into('<h', world.data, body + 0x116 - v.DATA, 0x200)
            for node in range(-1, 4):
                matrix = body + 0x200 + 0x34 * node
                for k in range(13):
                    world.put(matrix + 4 * k, fbits(rng.choice(floats)) if rng.random() < .85 else rng.getrandbits(32))
                world.put(matrix, fbits(rng.choice([1.0, 1.0, 2.0, 0.5, 1e-40, float('nan'), 1.0000001])))
        return [rng.choice(world.handles[:v.ROWS - 1]), TRACK + 0x80]

    # BABA0: esi object; spatial chain [0x4EABB0]: +0 head words, +4 record
    # array (+24 size, +44 rows: +4 key, +8 next, +C 0x14-byte packet),
    # +8 reference array (+24 size, +44 rows: +4 head word index, +8 next).
    v.KERNELS['object_refresh_spatial'] = (['core_spatial'], 'void')
    v.ROUTINES['object_refresh_spatial'] = dict(entry=0xbaba0, regs={'esi': 0})

    def spatial_case(name, world, rng):
        fresh(world, rng)
        put = world.put
        heads, records, refs = TRACK + 0x800, TRACK + 0x80, TRACK
        put(0x4eabb0, heads)
        put(0x4eabb4, records)
        put(0x4eabb8, refs)
        put(refs + 0x24, 12)
        put(refs + 0x44, TRACK + 0x100)
        put(records + 0x24, 0x20)
        put(records + 0x44, TRACK + 0x400)
        live = world.handles[:v.ROWS - 1]
        for r in range(16):
            row = TRACK + 0x400 + 0x20 * r
            put(row + 4, rng.choice(live + [v.NONE, 0x1234]))
            put(row + 8, rng.choice([((0x9000 + k) << 16) | k for k in range(r + 1, 16)] + [v.NONE]) if r < 15 else v.NONE)
        for h in range(16):
            put(heads + 4 * h, rng.choice([((0x9000 + k) << 16) | k for k in range(16)] + [v.NONE]))
        for a in range(8):
            row = TRACK + 0x100 + 12 * a
            struct.pack_into('<H', world.track, row + 4 - TRACK, rng.randrange(16))
            put(row + 8, ((0x8000 + a + 1) << 16) | (a + 1) if a < 7 and rng.random() < .7 else v.NONE)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.data[v.RECORDS + 12 * n + 2 - v.DATA] = rng.choice([0x40, 0x40, 0x50, 0, 0xff])
            put(body + 4, rng.getrandbits(32))
            put(body + 0x60, rng.choice([((0x8000 + a) << 16) | a for a in range(8)] + [v.NONE]))
            world.data[body + 0xaa - v.DATA] = rng.choice([0, 1, 2, 11, 5])
            world.data[body + 0x348 - v.DATA] = rng.choice([0, 8, 0xff, 0xf7])
        return [rng.choice(live)]

    # C0200: eax object; non-unit types only (unit 1157D0 is not native).
    v.KERNELS['object_notify_nodes'] = (['core_nodes'], 'void')
    v.ROUTINES['object_notify_nodes'] = dict(entry=0xc0200, regs={'eax': 0})
    v.UNREACHED_VIRTUAL_OWNERS.add(0x10e1f0)

    def nodes_case(name, world, rng):
        fresh(world, rng)
        for k in range(4):
            model = v.MODEL_TAGS + 0x80 * k
            world.put(model + 4, rng.choice([v.NONE, (0x6008 + k) << 16 | (8 + k)]))
            world.put(model + 0x14, rng.choice([v.NONE, 0x12340007]))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body, (rng.getrandbits(16) << 16) | rng.randrange(4))
            world.data[body + 0xaa - v.DATA] = rng.randrange(2, 13)
        return [rng.choice(world.handles[:v.ROWS - 1])]

    # 149AD0 (pressure level) and 1496F0 (guarded reserve below level 2).
    # The memory owner [0x48606C] is placed at OWNER so its +A8 weights and
    # +F0340 usage/block counters fall inside image blocks declared per case.
    OWNER = 0x420000
    v.KERNELS['physics_memory_pressure'] = (['core_pressure'], 'long')
    v.ROUTINES['physics_memory_pressure'] = dict(entry=0x149ad0)
    v.KERNELS['physics_memory_reserve'] = (['core_reserve'], 'void')
    v.ROUTINES['physics_memory_reserve'] = dict(entry=0x1496f0, closure_stops=[0x1cd130, 0x1ccbe0, 0x1cb330,
                                                                              0x1cb220, 0x149680, 0x1498a0])

    def pressure_world(world, rng, reserve):
        fresh(world, rng)
        world.extra[0x486058] = bytearray(rng.getrandbits(8) for _ in range(0x20))
        world.extra[0x48b870] = bytearray(rng.getrandbits(8) for _ in range(4))
        world.extra[OWNER + 0x68] = bytearray(rng.getrandbits(8) for _ in range(0x44))
        world.extra[OWNER + 0xf0340] = bytearray(rng.getrandbits(8) for _ in range(0x90))
        put = world.put
        for global_, at in ((0x486074, TRACK), (0x486070, TRACK + 0x40)):
            put(global_, rng.choice([0, at, at]))
            put(at + 0x20, rng.choice([0, 0, 1, v.NONE] if not reserve else [0, 0, v.NONE]))
        put(0x48606c, rng.choice([0, OWNER, OWNER, OWNER]))
        # Usage fraction around the 0.8 limit; weighted block counts around
        # the 0.65/0.75/0.85 levels (scale 1.0345e-6 per unit).
        put(OWNER + 0xf0340, rng.choice([0, 50, 94, 95, 100, 200, -5]) if not reserve else rng.choice([0, 50, 94]))
        for k in range(16):
            put(OWNER + 0xf0348 + 4 * k, rng.randrange(0, 4000))
            put(OWNER + 0xf038c + 4 * k, rng.randrange(0, 200))
            put(OWNER + 0x6c + 4 * k, rng.choice([0, 1, 16, 64, 160]))
        if reserve:
            # Below level 2: the 0.75 threshold needs weighted sums under ~725k.
            for k in range(16):
                put(OWNER + 0x6c + 4 * k, rng.choice([0, 1, 4]))
            world.extra[0x48b870][3] = rng.choice([0, 0, 0, 1])

    def pressure_case(name, world, rng):
        pressure_world(world, rng, False)
        return []

    def reserve_case(name, world, rng):
        pressure_world(world, rng, True)
        return []

    v.TRACK_ARGUMENTS.update(core_placement=placement_case, core_position=position_case,
                             core_spatial=spatial_case, core_nodes=nodes_case,
                             core_pressure=pressure_case, core_reserve=reserve_case)
    v.TRACK_COUNTS.update(physics_memory_pressure=500, physics_memory_reserve=300)
    v.TRACK_COUNTS.update(placement_data_new=300, object_world_position=400, object_refresh_spatial=400,
                          object_notify_nodes=200)
