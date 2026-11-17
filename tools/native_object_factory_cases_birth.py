"""Oracle cases for factory track "birth": B9B70 object_new.

object_new_early_failure: births whose body-block allocation fails (the
0x114 node block cannot fit) or whose datum/body allocation is refused. They
run B9B70's header, placement, flag and block stores, then BE330, without
reaching owners other tracks still port (closure_stops)."""
import struct


def register(v):
    TRACK = v.TRACK
    LATER = [0x1cd130, 0x1ccbe0, 0x1cb330, 0x1cb220, 0x149680, 0x1498a0, 0x1d3380, 0x10db40, 0x10dca0, 0xbc5c0, 0xc0270, 0xc0910, 0xbf050, 0xc1540, 0xba630,
             0xc0200, 0xba8c0, 0xc06c0, 0x10dc30, 0x179610, 0xba570, 0xc0f60, 0xbc570, 0xd80a0, 0xbc400, 0xb9390,
             0x2e1c0, 0x10e440, 0xba850, 0x1d3270]
    v.UNREACHED_VIRTUAL_OWNERS.add(0x10daf0)  # validate slots exist only for bipeds
    v.KERNELS['object_new_early_failure'] = (['birth_early'], 'long')
    v.ROUTINES['object_new_early_failure'] = dict(entry=0xb9b70, stack=[0], closure_stops=LATER)
    REQUEST = TRACK + 0x1000
    floats = [0.0, 1.0, -2.5, 0.5, 1e-40, 3.0]

    def fbits(x):
        return struct.unpack('<I', struct.pack('<f', x))[0]

    def early_case(name, world, rng):
        v.factory_fixture(world, rng)
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        put = world.put
        k = rng.randrange(4)
        definition = v.OBJECT_TAGS + 0x200 * k
        physics_type = rng.random() < .4
        struct.pack_into('<h', world.data3, definition - v.DATA3,
                         rng.choice([1, 7, 11, 12]) if physics_type else rng.choice([2, 3, 4, 5, 6, 8, 9, 10]))
        # 10DAF0 resolves the type by group tag: never the biped (validate) chain.
        put(v.TAG_TABLE + 16 * k, rng.choice(v.TYPE_GROUPS[1:]))
        # 1496F0 pressure inputs at level 0; 1CC4D0 capacity globals.
        world.extra[0x486058] = bytearray(0x20)
        world.extra[0x48b870] = bytearray([rng.choice([0, 0, 1]), 0, 0, rng.choice([0, 0, 0, 1])])
        world.globals[0x52be44] = TRACK + 0x3100
        world.globals[0x52be5c] = TRACK + 0x3140
        put(TRACK + 0x3100, rng.choice([0x200, 0x300, 0x1ff, 0]))
        put(TRACK + 0x3140 + 0x3c, rng.choice([5, 9]))
        put(TRACK + 0x3140 + 0x20, rng.choice([5, 9]))
        world.data3[definition + 2 - v.DATA3] = rng.choice([0, 1, 0xfe, 0xff])
        put(definition + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
        put(definition + 0x94, rng.choice([0, 1, 4, 20]))
        struct.pack_into('<H', world.data3, definition + 0xac - v.DATA3, rng.choice([0, 1, 3]))
        world.data3[definition + 0xbc - v.DATA3] = rng.getrandbits(8)
        model = v.MODEL_TAGS + 0x80 * k
        put(model + 0xc, rng.choice([v.NONE, 0x60090009]))
        put(model + 0x14, v.NONE)
        # Every case fails before the type constructors: the 0x114 node block
        # (nodes*0x34 a positive int16 larger than any pool), the 0x118 region
        # block, or 1CC4D0 for physics types without capacity.
        scenario = rng.choice(['nodes', 'regions', 'capacity'] if physics_type else ['nodes', 'regions'])
        put(model + 0x78, rng.choice([300, 400, 600, 0x200]) if scenario == 'nodes' else rng.choice([0, 1, 2]))
        put(model + 0x70, rng.choice([2000, 3000, 3200]) if scenario == 'regions' else rng.choice([0, 1, 3]))
        if scenario == 'capacity':
            put(TRACK + 0x3100, rng.choice([0x200, 0x300]))
            world.extra[0x48b870][0] = 0
        put(model + 0x60, rng.choice([0, 0, 1, v.NONE]))
        put(model + 0x64, TRACK + 0x2000)
        put(TRACK + 0x2000 + 0xbc, rng.choice([0, 1, 2, 8]))
        put(0x4eabc4, TRACK + 0x3000)
        for offset in range(0, 0xc4, 4):
            put(REQUEST + offset, fbits(rng.choice(floats)) if rng.random() < .5 else rng.getrandbits(32))
        put(REQUEST, (rng.getrandbits(16) << 16) | k)
        put(REQUEST + 0x18, rng.getrandbits(32) | (0x10 if rng.random() < .5 else 0))
        world.track[REQUEST + 0xb - TRACK] = rng.choice([0xff, 0xff, 0, 7])
        return [REQUEST, v.SP]

    v.TRACK_ARGUMENTS['birth_early'] = early_case
    v.TRACK_COUNTS['object_new_early_failure'] = 600

    # C0790 without children: no variant (+B1 0xFF), an empty variant, or
    # every child tag NONE. Child births (B9B70 + BAF10) are excluded.
    v.KERNELS['object_place_variant_children'] = (['birth_place'], 'void')
    v.ROUTINES['object_place_variant_children'] = dict(entry=0xc0790, stack=[0], closure_stops=[0xb9b70, 0xbaf10])

    def place_case(name, world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        put = world.put
        for k in range(4):
            put(v.OBJECT_TAGS + 0x200 * k + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
            put(v.MODEL_TAGS + 0x80 * k + 0x54, TRACK + 0x100 * k)
            for variant in range(3):
                row = TRACK + 0x100 * k + 0x38 * variant
                put(row + 0x1c, rng.choice([0, 0, 1, 3, v.NONE, 0x80000000]))
                put(row + 0x20, TRACK + 0x1000 + 0x100 * (3 * k + variant))
                for child in range(3):
                    put(TRACK + 0x1000 + 0x100 * (3 * k + variant) + 0x10 * child + 0xc, v.NONE)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            put(body, (rng.getrandbits(16) << 16) | rng.randrange(4))
            world.data[body + 0xb1 - v.DATA] = rng.choice([0xff, 0xff, 0, 1, 2])
        return [rng.choice(world.handles[:v.ROWS - 1])]

    v.TRACK_ARGUMENTS['birth_place'] = place_case
    v.TRACK_COUNTS['object_place_variant_children'] = 300

