"""Oracle cases for factory track "nodebuild" (BF050 and what it executes).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py
as a module and adds the leaf owners (masks, animation lookup, matrices,
type hooks). Fixtures lay their objects, tags and matrices out in the TRACK
bank; original .data/.bss blocks are per-case world.extra banks (statics
0x56E680, 0x56EB88, 0x51DF20 and the literal type definition table
0x473E48..0x474BC4 with its original bytes).

Indirect calls: 10E180 calls type hooks through the type table. Those
targets (1154A0, 10F440) are authenticated as their own kernels
(nodebuild_unit_hook, nodebuild_scenery_hook), which must run in the same
oracle invocation as nodebuild_type_hooks; 10E180 is declared in
UNREACHED_VIRTUAL_OWNERS only so the static closure admits its CALL EAX.
The 126200 resource closure under 1E5C80 has Havok/streaming virtuals that no
case reaches (resource bytes are zero), declared likewise.
"""
import math
import struct

NONE = 0xffffffff
SPECIAL = [0.0, -0.0, 1.0, -1.0, 0.5, 2.0, 1e-40, -1e-41, float('inf'), float('-inf'), 3.4e38, 1e-4, 0.70710677]
NANS = [0x7fc00000, 0xffc00001, 0x7fa00005, 0xff812345]


def f2u(x):
    return struct.unpack('<I', struct.pack('<f', x))[0]


def rfloat(rng, special=0.08, nan=0.02, span=4.0):
    r = rng.random()
    if r < nan:
        return rng.choice(NANS)
    if r < nan + special:
        return f2u(rng.choice(SPECIAL))
    return f2u(rng.uniform(-span, span))


# 1E5C80 -> 126200 closure (resource admission, never reached: zero bytes)
# and the bounded switch at 1A3B90 (CMP EAX,3 / JA).
RESOURCE_VIRTUAL_OWNERS = {0x13fe50, 0x140310, 0x2dbc84, 0x2dbf74, 0x2e02c4, 0x2e0342, 0x2e0362}
# Literal object type definitions: chain levels 0x473E48.. and the table.
TYPE_DEFINITIONS, TYPE_DEFINITIONS_END = 0x473e48, 0x474bc4
_image_bytes = {}


def original_bytes(v, address, size):
    key = (address, size)
    if key not in _image_bytes:
        _image_bytes[key] = bytes(v.Xbe(v.NEW_DEFAULT).read(address, size))
    return bytearray(_image_bytes[key])


def register(v):
    T, D = v.TRACK, v.DATA
    v.UNREACHED_VIRTUAL_OWNERS.update(RESOURCE_VIRTUAL_OWNERS | {0x10e180})
    v.JUMP_TABLES[0x1a3bfc] = 4
    v.JUMP_TABLES[0x11e09c] = 4  # 11E060 (16918F's first-person model entry, through placement's C4D30)
    v.JUMP_TABLES[0x1715a8] = 9  # 171440's nine prediction kinds (past the 171459 residual stop)
    # 331ECB's method dispatch for 37DC0A (as track modelstate declares it).
    v.REGISTER_JUMPS[0x331f55] = tuple(sorted(set(v.REGISTER_JUMPS.get(0x331f55, ())) | {0x37dc14, 0x331f68}))

    def kernel(name, types, kind, entry, **routine):
        v.KERNELS[name] = (types, kind)
        v.ROUTINES[name] = dict(entry=entry, **routine)

    kernel('nodebuild_mask_empty', ['nb_mask_empty'], 'boolean', 0xc1f40, regs={'ecx': 0})
    kernel('nodebuild_mask_invert', ['nb_mask_invert'], 'void', 0xc1eb0, regs={'eax': 0})
    kernel('nodebuild_mask_limit', ['nb_mask_limit'], 'void', 0xc1f00, regs={'esi': 0, 'eax': 1})
    kernel('nodebuild_mask_last', ['nb_mask_last'], 'long', 0x141010, regs={'edi': 0, 'eax': 1})
    kernel('nodebuild_flags_bit', ['nb_flags_bit'], 'void', 0xc1e50, regs={'eax': 0, 'ecx': 1, 'edx': 2})
    kernel('nodebuild_frozen', ['nb_frozen'], 'boolean', 0xc1cc0, regs={'ecx': 0})
    kernel('nodebuild_manager_valid', ['nb_manager'], 'long', 0xb87b0, regs={'eax': 0})
    kernel('nodebuild_animation', ['nb_animation'], 'long', 0x1e3150, regs={'edx': 0}, stack=[1])
    kernel('nodebuild_manager_animation', ['nb_manager_animation'], 'long', 0xb8780, regs={'ecx': 0})
    kernel('nodebuild_height', ['nb_height'], 'xmm0', 0xe3bc0, regs={'eax': 0})
    kernel('nodebuild_default_orientations', ['nb_default'], 'void', 0x1707a0, stack=[0, 1])
    kernel('nodebuild_orientation', ['nb_orientation'], 'void', 0x144cf0, stack=[0, 1])
    kernel('nodebuild_product', ['nb_product'], 'long', 0x145560, regs={'ecx': 0, 'edx': 1}, stack=[2])
    kernel('nodebuild_root', ['nb_root'], 'void', 0xbfc70, regs={'eax': 0, 'ecx': 1, 'ebx': 2}, stack=[3, 4, 5, 6, 7])
    kernel('nodebuild_type_hooks', ['nb_hooks'], 'void', 0x10e180, regs={'ebx': 0}, stack=[1])
    kernel('nodebuild_unit_hook', ['nb_hooks'], 'void', 0x1154a0, stack=[0, 1])
    kernel('nodebuild_scenery_hook', ['nb_hooks'], 'void', 0x10f440, stack=[0, 1])

    def track(world, rng):
        if world.track is None:
            world.track = bytearray(rng.randbytes(v.TRACK_SIZE))

    def put16(world, address, value):
        struct.pack_into('<H', world.track, address - T, value & 0xffff)

    def floats(world, address, count, rng, **kw):
        for n in range(count):
            world.put(address + 4 * n, rfloat(rng, **kw))

    def mask_words(world, address, rng, density=.5):
        for n in range(8):
            r = rng.random()
            if r < density:
                word = 0
            elif r < density + .25:
                word = 1 << rng.randrange(32)
            else:
                word = rng.getrandbits(32)
            world.put(address + 4 * n, word)

    def mask_empty(name, world, rng):
        track(world, rng)
        mask = T + 0x100 + 4 * rng.randrange(8)
        mask_words(world, mask, rng, density=rng.choice([1.0, 1.0, .9, .5]))
        return [mask]

    def mask_invert(name, world, rng):
        track(world, rng)
        mask = T + 0x100 + 4 * rng.randrange(8)
        mask_words(world, mask, rng)
        return [mask]

    def mask_limit(name, world, rng):
        track(world, rng)
        mask = T + 0x200
        mask_words(world, mask - 0x40, rng, density=0)
        mask_words(world, mask, rng, density=.2)
        count = rng.choice([0, 1, 5, 31, 32, 33, 63, 64, 65, 100, 128, 200, 224, 231, 232, 255, 256, 257, 300,
                            0xffffff00, 0xfffffff0])
        return [mask, count]

    def mask_last(name, world, rng):
        track(world, rng)
        mask = T + 0x200
        mask_words(world, mask - 0x20, rng)
        mask_words(world, mask, rng, density=rng.choice([.5, .8, 1.0]))
        mask_words(world, mask + 0x20, rng)
        count = rng.choice([0, 1, 5, 31, 32, 33, 64, 100, 200, 255, 256, 257, 288])
        return [mask, count]

    def flags_bit(name, world, rng):
        track(world, rng)
        flags = T + 0x100 + 4 * rng.randrange(8)
        bit = rng.choice([0, 1, 10, 29, 30, 31, 32, 0x41, 0x1e, NONE, rng.getrandbits(32)])
        return [flags, bit, rng.choice([0, 1, 0x100, 0xff, 0x80000001, 0x80])]

    def frozen(name, world, rng):
        for n in range(v.ROWS - 1):
            world.data[world.body(n) + 0xb3 - D] = rng.choice([0, 0, 1, 0x80, 0xff])
        return [rng.choice(world.handles[:v.ROWS - 1] + [NONE, NONE])]

    def manager(name, world, rng):
        track(world, rng)
        at = T + 0x300
        world.put(at, rng.choice([NONE, 0x12340010, 0]))
        put16(world, at + 6, rng.choice([0xffff, 0, 3]))
        world.put(at + 0x68, rng.choice([NONE, 0x55550011, 0]))
        return [at]

    # Animation graphs (jmad): three graphs in tag rows 16..18, eight 0x6C
    # animations each with acyclic +30/+32 chains, inheritance lists of
    # valid graphs and zero resource bytes (126200 is never entered).
    GRAPHS, ANIMATIONS = 3, 8

    def graph_fixture(world, rng):
        track(world, rng)
        world.extra[0x51df20] = bytearray([rng.choice([0, 1, 1, 1]), rng.choice([0, 1, 1, 0x80]), 0, 0])
        graphs = []
        for g in range(GRAPHS):
            data = T + 0x6000 + 0x400 * g
            graphs.append(data)
            world.put(v.TAG_TABLE + 16 * (16 + g) + 8, data)
        for g, data in enumerate(graphs):
            rows = T + 0x7000 + 0x400 * g
            world.put(data + 0x30, rows)
            inherited = rng.choice([0, 1, 2, 3])
            world.put(data + 0x4c, inherited)
            world.put(data + 0x50, data + 0x100)
            for k in range(4):
                world.put(data + 0x100 + 32 * k + 4, (0x7700 + k) << 16 | (16 + rng.randrange(GRAPHS)))
            world.put(data + 0xb0, data + 0x200)
            for k in range(8):
                world.track[data + 0x200 + 20 * k + 0x10 - T] = 0
            for a in range(ANIMATIONS):
                row = rows + 0x6c * a
                put16(world, row + 0x14, rng.choice([1, 2, 30, 0, 0x7fff]))
                world.track[row + 0x16 - T] = rng.choice([0, 0x10, 0x12, 0x02, 0xff, 0xef])
                world.put(row + 0x24, rng.randrange(8))
                put16(world, row + 0x30, rng.choice([0xffff, 0xffff, rng.randrange(ANIMATIONS)]))
                later = list(range(a + 1, ANIMATIONS))
                put16(world, row + 0x32, rng.choice([0xffff] + later) if later else 0xffff)
        return graphs

    def animation_id(world, rng, graph):
        inherited = struct.unpack_from('<I', world.track, graph + 0x4c - T)[0]
        high = rng.choice([0xffff, rng.randrange(ANIMATIONS), rng.randrange(ANIMATIONS)])
        low = rng.choice([0xffff, 0xffff] + list(range(inherited)))
        return high << 16 | low

    def animation(name, world, rng):
        graphs = graph_fixture(world, rng)
        graph = rng.choice(graphs)
        return [graph, animation_id(world, rng, graph)]

    def manager_animation(name, world, rng):
        graphs = graph_fixture(world, rng)
        g = rng.randrange(GRAPHS)
        at = T + 0x300
        world.put(at, (0x6600 + g) << 16 | (16 + g))
        world.put(at + 4, animation_id(world, rng, graphs[g]))
        return [at]

    def height(name, world, rng):
        tick = struct.unpack_from('<I', world.data2, v.TIME + 8 - v.DATA2)[0]
        if rng.random() < .5:
            struct.pack_into('<I', world.data2, v.TIME + 4 - v.DATA2, rfloat(rng, special=.2, nan=.05, span=.1))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.data[body + 0x34b - D] = rng.choice([1, 1, 0, 2])
            world.data[body + 0x3dc - D] = rng.choice([3, 3, 0, 1])
            world.put(body + 0x3ec + 0x5c, rng.choice([NONE, tick, tick - 1, tick - 5, tick - 30, tick - 1000, tick + 3,
                                                     tick - rng.randrange(1, 200), rng.getrandbits(32)]) & 0xffffffff)
        return [rng.choice(world.handles[:v.ROWS - 1])]

    def model_nodes(world, rng, model, records, count):
        world.put(model + 0x48, count)
        world.put(model + 0x4c, records)
        for n in range(max(0, min(count, 8))):
            floats(world, records + 0x60 * n, 0x18, rng)

    def default_orientations(name, world, rng):
        track(world, rng)
        model = T + 0x6000
        model_nodes(world, rng, model, T + 0x7000, rng.choice([0, 1, 3, 6, NONE, 0x80000000]))
        return [model, T + 0x100]

    def epsilon(world, rng):
        world.extra[0x56e680] = bytearray(rng.randbytes(0x38))   # + 296540/2962F0 key statics
        world.put(0x56e684, rng.choice([0, 1, 1, 0xfffffffe, 0xffffffff, 0x100]))
        world.put(0x56e680, rng.choice([f2u(1e-4), f2u(1e-4), 0, f2u(-1.0), f2u(1.0), NANS[0], f2u(1e-40), f2u(3.0)]))

    def quaternion(world, rng, at):
        kind = rng.random()
        if kind < .45:
            q = [rng.uniform(-1, 1) for _ in range(4)]
            n = sum(x * x for x in q) ** .5 or 1.0
            q = [f2u(x / n) for x in q]
        elif kind < .6:
            scale = rng.choice([1e-3, 5e-3, 1e-2, 0.0])
            q = [f2u(rng.uniform(-1, 1) * scale) for _ in range(4)]
        elif kind < .7:
            q = [f2u(0.0)] * 3 + [f2u(1.0)]
        else:
            q = [rfloat(rng, special=.25, nan=.05, span=3) for _ in range(4)]
        for n, word in enumerate(q):
            world.put(at + 4 * n, word)
        floats(world, at + 0x10, 4, rng, span=50)

    def orientation(name, world, rng):
        track(world, rng)
        epsilon(world, rng)
        q = T + 0x100
        quaternion(world, rng, q)
        return [T + 0x200, q]

    def matrix(world, rng, at, scale=None):
        floats(world, at, 13, rng)
        world.put(at, scale if scale is not None else rng.choice([f2u(1.0), f2u(1.0), rfloat(rng, special=.4)]))

    def product(name, world, rng):
        track(world, rng)
        a, b = T + 0x100, T + 0x140
        matrix(world, rng, a)
        matrix(world, rng, b)
        if rng.random() < .15:
            # NaN scales with distinct payloads select the destination operand.
            world.put(a, rng.choice(NANS))
            world.put(b, rng.choice(NANS))
        if rng.random() < .1:
            b = a
        # Exact and partial aliases of A and B: B rows are read around the
        # stores of the preceding rows.
        out = rng.choice([T + 0x180, T + 0x180, a, b, b - 4, b + 4, b - 0xc, b + 0xc, a + 8, b - 0x18])
        return [a, b, out]

    def root(name, world, rng):
        track(world, rng)
        up, forward, position = T + 0x100, T + 0x110, T + 0x120
        floats(world, up, 3, rng)
        floats(world, forward, 3, rng)
        floats(world, position, 3, rng, span=100)
        scale = rng.choice([f2u(1.0), f2u(1.0), f2u(2.0), rfloat(rng, special=.5, nan=.1)])
        mirror = rng.choice([0, 0, 1, 0x100, 0xffffff01, 0x400, 2, 0x80, 0xfe])
        parent = rng.choice([0, T + 0x200, T + 0x200])
        if parent:
            matrix(world, rng, parent, scale=rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), rfloat(rng, special=.5, nan=.1)]))
        out = rng.choice([T + 0x300, T + 0x300, T + 0x300, parent or T + 0x300])
        return [up, forward, rng.choice([0, 0, 1, 0x100, 0xff]), position, scale, mirror, parent, out]

    def hooks(name, world, rng):
        track(world, rng)
        world.extra[TYPE_DEFINITIONS] = original_bytes(v, TYPE_DEFINITIONS, TYPE_DEFINITIONS_END - TYPE_DEFINITIONS)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.data[body + 0xaa - D] = rng.choice([0, 1, 6, 6, 2, 7, 11, 12, 9])
            struct.pack_into('<h', world.data, body + 0x33e - D, rng.choice([0x100, 0x180, -0x40]))
            other = rng.choice(world.handles + [NONE, NONE, (world.salts[1] ^ 0x10) << 16 | 1, 0x7fff0020])
            for offset in (0x128, 0x1a8, -0x18):
                world.put(body + offset, other)
            world.put(body + 0x138, rng.choice(world.handles + [NONE, (world.salts[2] ^ 1) << 16 | 2]))
        object_ = rng.choice(world.handles[:v.ROWS - 1])
        nodes = rng.choice([world.body(object_ & 0xffff) + 0x260, T + 0x100])
        if nodes == T + 0x100:
            matrix(world, rng, nodes)
        return [object_, nodes]

    # ---- BFB90 subtree leaves ----
    v.JUMP_TABLES[0x1e16cc] = 4
    kernel('nodebuild_fraction', ['nb_fraction'], 'xmm0', 0x1e1620, regs={'eax': 0})
    kernel('nodebuild_crossfade', ['nb_crossfade'], 'void', 0x1e1720, regs={'eax': 0, 'ecx': 1, 'ebx': 2}, stack=[3, 4])
    kernel('nodebuild_blend_fraction', ['nb_blend'], 'xmm0', 0x1d61b0, regs={'eax': 0})
    kernel('nodebuild_codec_entry', ['nb_codec_entry'], 'long', 0x1e32a0, regs={'edi': 0}, stack=[1])

    def interpolation_state(world, rng, at):
        world.track[at - T] = rng.choice([0, 1, 5, 15, 29, 30, 0x7f, 0x80, 0xff, rng.randrange(256)])
        world.track[at + 1 - T] = rng.choice([0, 1, 2, 10, 30, 0x7f, 0x80, 0xff])
        world.track[at + 2 - T] = rng.choice([0, 1, 2, 3, 4, 5, 0x80, 0xff])
        world.track[at + 3 - T] = rng.choice([0, 2, 0xfd, 0xff])

    def fraction(name, world, rng):
        track(world, rng)
        interpolation_state(world, rng, T + 0x100)
        return [T + 0x100]

    def blend(name, world, rng):
        track(world, rng)
        world.put(T + 0x300 + 0x80, rng.choice([f2u(rng.uniform(-.5, 1.5)), f2u(rng.uniform(0, 1)), f2u(0.9999),
                                                f2u(0.99989), f2u(-1e-4), f2u(-2e-4), rfloat(rng, special=.6, nan=.2)]))
        return [T + 0x300]

    def crossfade(name, world, rng):
        track(world, rng)
        state = T + 0x100
        interpolation_state(world, rng, state)
        if rng.random() < .8:
            world.track[state + 1 - T] = rng.choice([1, 2, 10, 30])
        count = rng.choice([NONE, 0x80000000, 0, 1, 2, 5, 8, 33])
        current, previous = T + 0x1000, T + 0x2000
        for n in range(40):
            quaternion(world, rng, current + 0x20 * n)
            if rng.random() < .3:
                for k in range(8):
                    world.put(previous + 0x20 * n + 4 * k, struct.unpack_from('<I', world.track, current + 0x20 * n + 4 * k - T)[0])
                if rng.random() < .5:
                    for k in range(4):  # opposite hemisphere / cancelling quaternions
                        word = struct.unpack_from('<I', world.track, current + 0x20 * n + 4 * k - T)[0]
                        world.put(previous + 0x20 * n + 4 * k, word ^ 0x80000000)
            else:
                quaternion(world, rng, previous + 0x20 * n)
        mask = rng.choice([0, 0, T + 0x200])
        if mask:
            mask_words(world, mask, rng, density=.2)
        previous = rng.choice([previous, previous, previous, current, current + 0x10, current + 4, current - 4, current - 8])
        return [state, current, count, mask, previous]

    def codec_entry(name, world, rng):
        graphs = graph_fixture(world, rng)
        graph = rng.choice(graphs)
        if rng.random() < .3:
            world.extra[0x51df20][0] = 0
        gated = not (world.extra[0x51df20][0] and world.extra[0x51df20][1])
        for k in range(4):
            entry = graph + 0x100 + 32 * k
            if gated and rng.random() < .3:
                world.put(entry + 4, NONE)
            for offset in (8, 0xc, 0x10, 0x14):
                world.put(entry + offset, rng.choice([0, 1, 5, NONE, 0x80000000, 0x40]))
        inherited = struct.unpack_from('<I', world.track, graph + 0x4c - T)[0]
        low = rng.choice([0xffff, 0x8000, 0x8001, inherited, inherited + 1] + list(range(inherited)) * 3)
        high = rng.choice([0xffff, rng.randrange(ANIMATIONS), rng.randrange(ANIMATIONS)])
        return [graph, high << 16 | low]

    for kind_, fn_ in (('nb_fraction', fraction), ('nb_blend', blend), ('nb_crossfade', crossfade),
                       ('nb_codec_entry', codec_entry)):
        v.TRACK_ARGUMENTS[kind_] = fn_
    for name_, count_ in (('nodebuild_fraction', 400), ('nodebuild_blend_fraction', 300),
                          ('nodebuild_crossfade', 800), ('nodebuild_codec_entry', 400)):
        v.TRACK_COUNTS[name_] = count_

    # ---- Codec per-track decoders (0x48C33C table entries) ----
    # State block 0x510F30 (+14 frame, +18 next, +1C fraction, +30 data,
    # +64/+68/+6C tracks, +70 output); data and output 16-byte aligned.
    CODEC_DECODERS = (0x296180, 0x296680, 0x296450, 0x296240, 0x2963d0, 0x2961f0, 0x296410, 0x296220, 0x2cec60)
    for decoder in CODEC_DECODERS:
        kernel(f'nodebuild_codec_{decoder:x}', ['nb_codec'], 'void', decoder)

    def codec(name, world, rng):
        track(world, rng)
        world.extra[0x510f30] = bytearray(rng.randbytes(0x74))
        data, out = T + 0x1000, T + 0x2000
        state = lambda offset, value: world.put(0x510f30 + offset, value)
        state(0x30, data)
        state(0x70, out)
        frame = rng.randrange(4)
        state(0x14, frame)
        state(0x18, rng.choice([min(frame + 1, 3), frame, 0]))
        state(0x1c, rng.choice([f2u(rng.uniform(0, 1)), f2u(rng.uniform(0, 1)), 0, f2u(1.0), rfloat(rng, special=.5, nan=.2)]))
        for offset in (0x64, 0x68, 0x6c):
            state(offset, rng.randrange(4))
        world.put(data + 0xc, 0x200)
        world.put(data + 0x10, 0x300)
        world.put(data + 0x14, 0x40)
        world.put(data + 0x18, 0x30)
        world.put(data + 0x1c, 0x10)
        for n in range(0x40):  # rotation keys: quantized words or float lanes
            at = data + 0x20 + 4 * n
            if name.endswith(('296240', '2963d0')):
                world.put(at, rfloat(rng, special=.1, nan=.02, span=1.0))
            else:
                words = [rng.choice([rng.randrange(-32768, 32768), 0, 32767, -32768, rng.randrange(-200, 200)]) for _ in range(2)]
                if rng.random() < .03:
                    words = [0, 0]
                world.put(at, (words[0] & 0xffff) | (words[1] & 0xffff) << 16)
        for n in range(0x40):
            world.put(data + 0x200 + 4 * n, rfloat(rng, span=50))
        return []

    v.TRACK_ARGUMENTS['nb_codec'] = codec
    for decoder in CODEC_DECODERS:
        v.TRACK_COUNTS[f'nodebuild_codec_{decoder:x}'] = 300

    # Sparse-key decoders: per-track packed (first << 12 | count) entries,
    # ascending key-frame bytes and their float values.
    # 296890/2969E0: the forced forms (frame + fraction, RCPSS of the key gap).
    SPARSE_DECODERS = (0x296c60, 0x296db0, 0x296890, 0x2969e0)
    for decoder in SPARSE_DECODERS:
        kernel(f'nodebuild_codec_{decoder:x}', ['nb_sparse'], 'void', decoder)

    def sparse(name, world, rng):
        codec(name, world, rng)
        data = T + 0x1000
        for offset, value in ((0xc, 0x100), (0x10, 0x120), (0x18, 0x200), (0x1c, 0x300), (0x24, 0x400), (0x28, 0x600)):
            world.put(data + offset, value)
        for table in (0x100, 0x120):
            for k in range(4):
                count = rng.choice([0, 1, 2, 3, 5, 8, 16, 16])
                world.put(data + table + 4 * k, rng.randrange(0, 40) << 12 | count)
        for base in (0x200, 0x300):
            keys = sorted(rng.choice([rng.randrange(0, 60), rng.randrange(0, 256)]) for _ in range(64))
            for n, key in enumerate(keys):
                world.track[data + base + n - T] = key
        # NaN-rich values in some cases make the blends' operand order
        # (NaN payload selection) observable.
        nan = rng.choice([.02, .02, .4])
        for n in range(0x200 // 4):
            world.put(data + 0x400 + 4 * n, rfloat(rng, special=.05, nan=nan, span=20))
        world.put(0x510f30 + 0x14, rng.choice([rng.randrange(0, 60), rng.randrange(0, 256), 0, 255, 0x80000000, 1000]))
        return []

    v.TRACK_ARGUMENTS['nb_sparse'] = sparse
    for decoder in SPARSE_DECODERS:
        v.TRACK_COUNTS[f'nodebuild_codec_{decoder:x}'] = 600

    # Sparse rotations (296AD0 byte keys, 297270 word keys): packed entries
    # at data+30 (+64 track), keys at data+[14], quantized rows at data+[20].
    # 297400: word keys for the translation layout of 296C60.
    kernel('nodebuild_codec_296ad0', ['nb_sparse_rotation'], 'void', 0x296ad0)
    kernel('nodebuild_codec_297270', ['nb_sparse_rotation'], 'void', 0x297270)
    kernel('nodebuild_codec_296700', ['nb_sparse_rotation'], 'void', 0x296700)  # forced (frame + fraction)
    kernel('nodebuild_codec_297400', ['nb_sparse_words'], 'void', 0x297400)

    def sparse_keys(world, rng, at, count, words):
        span = rng.choice([60, 256, 1000, 65536]) if words else rng.choice([60, 256])
        keys = sorted(rng.choice([rng.randrange(0, min(span, 60)), rng.randrange(0, span)]) for _ in range(count))
        if rng.random() < .3:  # repeated keys
            for n in range(1, count):
                if rng.random() < .3:
                    keys[n] = keys[n - 1]
        for n, key in enumerate(keys):
            if words:
                put16(world, at + 2 * n, key)
            else:
                world.track[at + n - T] = key
        return keys

    def sparse_frame(world, rng, keys):
        world.put(0x510f30 + 0x14, rng.choice([rng.choice(keys), rng.choice(keys), rng.choice(keys) + 1,
                                               rng.randrange(0, 60), rng.randrange(0, 1100), 0, 255, 0x80000000,
                                               0x7fffffff, NONE]) & 0xffffffff)

    def sparse_rotation(name, world, rng):
        codec(name, world, rng)
        data = T + 0x1000
        words = name.endswith('297270')
        world.put(data + 0x14, 0x200)
        world.put(data + 0x20, 0x400)
        for k in range(4):
            count = rng.choice([0, 1, 2, 3, 5, 8, 16, 16])
            world.put(data + 0x30 + 4 * k, rng.randrange(0, 40) << 12 | count)
        keys = sparse_keys(world, rng, data + 0x200, 64, words)
        for n in range(0x200 // 4):
            words_ = [rng.choice([rng.randrange(-32768, 32768), 0, 32767, -32768, rng.randrange(-200, 200)])
                      for _ in range(2)]
            world.put(data + 0x400 + 4 * n, (words_[0] & 0xffff) | (words_[1] & 0xffff) << 16)
        sparse_frame(world, rng, keys)
        return []

    def sparse_words(name, world, rng):
        sparse(name, world, rng)
        data = T + 0x1000
        keys = sparse_keys(world, rng, data + 0x200, 64, True)
        sparse_frame(world, rng, keys)
        return []

    v.TRACK_ARGUMENTS['nb_sparse_rotation'] = sparse_rotation
    v.TRACK_ARGUMENTS['nb_sparse_words'] = sparse_words
    for name_ in ('nodebuild_codec_296ad0', 'nodebuild_codec_297270', 'nodebuild_codec_297400', 'nodebuild_codec_296700'):
        v.TRACK_COUNTS[name_] = 600

    for kind, fn in (('nb_mask_empty', mask_empty), ('nb_mask_invert', mask_invert), ('nb_mask_limit', mask_limit),
                     ('nb_mask_last', mask_last), ('nb_flags_bit', flags_bit), ('nb_frozen', frozen),
                     ('nb_manager', manager), ('nb_animation', animation),
                     ('nb_manager_animation', manager_animation), ('nb_height', height),
                     ('nb_default', default_orientations), ('nb_orientation', orientation),
                     ('nb_product', product), ('nb_root', root), ('nb_hooks', hooks)):
        v.TRACK_ARGUMENTS[kind] = fn
    for name, count in (('nodebuild_mask_empty', 120), ('nodebuild_mask_invert', 60), ('nodebuild_mask_limit', 200),
                        ('nodebuild_mask_last', 300), ('nodebuild_flags_bit', 100), ('nodebuild_frozen', 100),
                        ('nodebuild_manager_valid', 100), ('nodebuild_animation', 400),
                        ('nodebuild_manager_animation', 300), ('nodebuild_height', 300),
                        ('nodebuild_default_orientations', 100), ('nodebuild_orientation', 600),
                        ('nodebuild_product', 600), ('nodebuild_root', 800), ('nodebuild_type_hooks', 400),
                        ('nodebuild_unit_hook', 200), ('nodebuild_scenery_hook', 200)):
        v.TRACK_COUNTS[name] = count


# ---------------------------------------------------------------------------
# Composite owners (BFF20, BF0C0, BF050). Their static closures reach the
# whole factory (BA630 -> B9B70) and CRT/codec jump forms that the shared
# validator cannot bound, so they are proven by this module's own runner:
#
#   python3 tools/run_test.py -- .tools/python/bin/python \
#       tools/native_object_factory_cases_nodebuild.py --native <harness> --report <json>
#
# It reuses the validator's World, reference build and physical observer.
# The static closure stops at the declared boundary entries (callees owned
# by other tracks or not native yet) and the original stops before entering
# one. A case that reaches a boundary must make the native refuse with that
# owner's name after the identical ordered store prefix; every other case
# must complete with identical stores, result and final banks.
# ---------------------------------------------------------------------------
# Boundary PC -> refusal owner. Function entries refuse under their own
# address; an inner PC names the owner whose path it opens.
BOUNDARY_OWNERS = {0xbfa3d: 'BF9A0'}
# Indirect call sites whose possible targets are declared (type table hooks).
INDIRECT_TARGETS = {0x10e180: [0x1154a0, 0x10f440]}
COMPOSITE = {}


def composite(name, entry, kind, stops, count, cut=(), native=None, **routine):
    """`stops`: PCs the original stops before (refusal boundaries); `cut`:
    further entries reached only through a stop, left out of the closure;
    `native`: the harness kernel (default `name`)."""
    COMPOSITE[name] = dict(entry=entry, kind=kind, stops=set(stops), cut=set(cut), count=count, native=native or name,
                           routine=dict(entry=entry, **routine))


# TRACK layout: eleven 0xA00-byte bodies (9 node matrices +460, 9
# orientations +6C0, animation manager +7E0, unit block +868, 9 previous
# orientations +8A0),
# object definitions, models, render models and their node records.
TRACK_BASE = 0x1500000
BODIES, BODY = TRACK_BASE, 0xa00
NODES, ORIENTATIONS, MANAGER, UNIT_BLOCK, PREVIOUS = 0x460, 0x6c0, 0x7e0, 0x868, 0x8a0
MODELS, MODES = TRACK_BASE + 0x7200, TRACK_BASE + 0x7500
IDENTITY, PHYSICS_POOL, RECORDS_ = TRACK_BASE + 0x7680, TRACK_BASE + 0x76c0, TRACK_BASE + 0x7800
# Object definitions (0x400 bytes each: the hooks read up to +210) live in
# the FACTORY bank; physics components (BC200: +18 root slot, +70 rows, +74
# count), their rigid body and motion follow the node records.
FACTORY_BASE = 0x1400000
DEFINITIONS, DEFINITION = FACTORY_BASE, 0x400
COMPONENTS, COMPONENT_ROWS = TRACK_BASE + 0x8100, TRACK_BASE + 0x8240
RIGID_BODY, MOTION, ZERO_VECTOR = TRACK_BASE + 0x8300, TRACK_BASE + 0x8344, TRACK_BASE + 0x83a0
GRAPHS_, GRAPH_ROWS = TRACK_BASE + 0x8400, TRACK_BASE + 0x9000


def scene(v, world, rng, purpose):
    """Objects 0..10 with bodies in the TRACK bank (stride 0xA00), their
    definitions (tag rows 16..19), models (20..22), render models (23..25),
    an acyclic parent/child forest and the statics BF0C0 publishes."""
    T, NONE_ = v.TRACK, NONE
    world.track = bytearray(rng.randbytes(v.TRACK_SIZE))
    world.factory = bytearray(rng.randbytes(v.FACTORY_SIZE))
    put = world.put

    def put16(address, value):
        struct.pack_into('<H', world.track, address - T, value & 0xffff)

    def put8(address, value):
        world.track[address - T] = value & 0xff

    def floats(address, count, **kw):
        for n in range(count):
            put(address + 4 * n, rfloat(rng, **kw))

    def put8_any(address, value):
        world.put(address & ~3, world_word(v, world, address & ~3) & ~(0xff << 8 * (address & 3)) |
                  (value & 0xff) << 8 * (address & 3))

    def unit(address):
        x = [rng.uniform(-1, 1) for _ in range(3)]
        n = sum(c * c for c in x) ** .5 or 1.0
        for k in range(3):
            put(address + 4 * k, f2u(x[k] / n) if rng.random() < .9 else rfloat(rng, special=.5, nan=.1))

    def quaternion(at):
        r = rng.random()
        if r < .6:
            q = [rng.uniform(-1, 1) for _ in range(4)]
            n = sum(c * c for c in q) ** .5 or 1.0
            q = [f2u(c / n) for c in q]
        elif r < .7:
            q = [0, 0, 0, f2u(1.0)]
        elif r < .8:
            q = [f2u(rng.uniform(-1e-3, 1e-3)) for _ in range(4)]
        else:
            q = [rfloat(rng, special=.3, nan=.05, span=2) for _ in range(4)]
        for k in range(4):
            put(at + 4 * k, q[k])
        floats(at + 0x10, 3, span=2)
        put(at + 0x1c, rng.choice([f2u(1.0), f2u(1.0), f2u(1.0), rfloat(rng, special=.3)]))

    # Statics: epsilon/flag (144CF0), render-only mask + flag (BF0C0), the
    # 255-node orientation scratch, the identity orientation pointer, the
    # literal type definitions and the physics component pool pointer.
    world.extra[0x56e680] = bytearray(rng.randbytes(0x38))   # + 296540/2962F0 key statics
    put(0x56e684, rng.choice([0, 1, 1, 1, 0xfffffffe]))
    put(0x56e680, rng.choice([f2u(1e-4), f2u(1e-4), f2u(1e-4), 0, f2u(-1.0)]))
    world.extra[0x56eb88] = bytearray(rng.randbytes(0x28))
    put(0x56ebac, rng.choice([0, 1, 1, 0x80000000, 0xfffffffe]))
    world.extra[0x4e8bc0] = bytearray(rng.randbytes(0x1fe0))
    for n in range(10):
        quaternion(0x4e8bc0 + 0x20 * n)
    world.extra[0x474d38] = bytearray(4)
    put(0x474d38, IDENTITY)
    quaternion(IDENTITY)
    world.extra[TYPE_DEFINITIONS] = original_bytes(v, TYPE_DEFINITIONS, TYPE_DEFINITIONS_END - TYPE_DEFINITIONS)
    world.extra[0x51df20] = bytearray([1, 1, 0, 0])
    world.extra[0x47c1b4] = bytearray([1, 1, 0, 0])
    world.extra[0x56e060] = bytearray(rng.randbytes(0x20))  # 1CF210 node-mask intersection scratch
    # An empty light pool for C3750 (owned lights of a +4 bit 6 object
    # reconnected or detached by BFF20); the connected world declares its own.
    world.extra[0x56f000] = bytearray(0x50)
    world.globals[0x4ecbdc] = 0x56f000
    world.globals[0x52be5c] = PHYSICS_POOL
    put(PHYSICS_POOL + 0x44, COMPONENTS - 5 * 0xa0)  # body +B4 datums 5 and 6
    for c in range(2):
        put8(COMPONENTS + 0xa0 * c + 0x18, rng.choice([0, 0, 1, 0xff, 2]))
        put(COMPONENTS + 0xa0 * c + 0x70, COMPONENT_ROWS)
        put(COMPONENTS + 0xa0 * c + 0x74, 2)
    for slot in range(2):
        put(COMPONENT_ROWS + 0x60 * slot + 0x40, RIGID_BODY)
    put8(RIGID_BODY + 0x40, rng.choice([0, 0, 0, 1]))
    put(RIGID_BODY + 0x3c, MOTION)
    floats(MOTION + 0x40, 3, span=20)
    floats(MOTION + 0x50, 3, span=4)
    world.extra[0x474d24] = bytearray(4)  # 114750's default aim angles
    put(0x474d24, AIM_DEFAULTS)
    for k in range(2):
        put(AIM_DEFAULTS + 4 * k, f2u(rng.choice([0.0, rng.uniform(-1, 1)])))
    world.extra[0x474d04] = bytearray(4)  # the zero vector pointer (BC200)
    put(0x474d04, ZERO_VECTOR)
    for k in range(3):
        put(ZERO_VECTOR + 4 * k, 0 if rng.random() < .8 else rfloat(rng))

    # Render models: node counts and records (parent index below the node).
    modes, counts = [], []
    for k in range(3):
        mode = MODES + 0x80 * k
        modes.append(mode)
        put(v.TAG_TABLE + 16 * (23 + k) + 8, mode)
        count = rng.choice([1, 2, 3, 5, 8, 8, 0]) if k else rng.choice([1, 3, 6])
        counts.append(count)
        records = RECORDS_ + 0x300 * k
        put(mode + 0x48, count)
        put(mode + 0x4c, records)
        for n in range(8):
            record = records + 0x60 * n
            put16(record + 4, rng.randrange(n) if n else 0xffff)
            floats(record + 0xc, 3, span=2)
            quaternion(record + 0x18)
    models = []
    for k in range(3):
        hlmt = MODELS + 0x100 * k
        models.append(hlmt)
        put(v.TAG_TABLE + 16 * (20 + k) + 8, hlmt)
        put(hlmt + 4, (0x5300 + k) << 16 | (23 + k) if rng.random() < .9 else NONE_)
        for n in range(8):
            put(hlmt + 0xa4 + 4 * n, 0)
        if rng.random() < .3:
            put(hlmt + 0xa4 + 4 * rng.randrange(8), 1 << rng.randrange(32))
    definitions = []
    for k in range(4):
        definition = DEFINITIONS + DEFINITION * k
        definitions.append(definition)
        put(v.TAG_TABLE + 16 * (16 + k) + 8, definition)
        put8_any(definition, rng.choice([0, 1, 2, 3, 6, 7, 9, 11, 12]))
        bank, at = world_bank(v, world, definition + 2)
        struct.pack_into('<H', bank, at, rng.choice([0, 0x1000, 0xefff, 0xffff]))
        put(definition + 4, f2u(rng.choice([0.5, 1.0, 2.5, 0.0])))
        floats(definition + 8, 3, span=1, special=.3, nan=.1)
        put(definition + 0x20, rng.choice([0, f2u(-1.0), f2u(1.5), f2u(0.25), NANS[0]]))
        floats(definition + 0x24, 3, span=1)
        put(definition + 0x38, rng.choice([NONE_] + [(0x5200 + m) << 16 | (20 + m) for m in range(3)] * 3))
        # Unit flags +BC (bit 10 no aiming, bit 11 no overlays) and seats
        # (+1CC rows of 0xB0, weight +1C) for CB4A0.
        put(definition + 0xbc, rng.choice([0, 0, 0, 1 << 10, 1 << 11, rng.getrandbits(32)]))
        put(definition + 0x1cc, SEATS + 0x200 * k)
        for seat in range(2):
            put(SEATS + 0x200 * k + 0xb0 * seat + 0x1c, rng.choice([f2u(rng.uniform(0, 1)), 0, f2u(-1.0), NANS[0]]))

    def word(address):
        return struct.unpack_from('<I', world.track, address - T)[0]

    def rendered(k):
        model = world_word(v, world, definitions[k] + 0x38)
        return model != NONE_ and word(models[(model & 0xffff) - 20] + 4) != NONE_

    def node_count(k):
        model = world_word(v, world, definitions[k] + 0x38)
        if model == NONE_:
            return rng.choice([1, 2, 4])
        render = word(models[(model & 0xffff) - 20] + 4)
        return counts[(render & 0xffff) - 23] if render != NONE_ else rng.choice([1, 2, 4])

    animation_graphs(v, world, rng)
    codec_banks(v, world, rng)
    codec_cache(v, world, rng)
    handles = world.handles[:v.ROWS - 1]
    parents = {}
    for n in range(v.ROWS - 1):
        body = BODIES + BODY * n
        put(v.RECORDS + 12 * n + 8, body)
        k = rng.randrange(4)
        put(body, (0x4100 + k) << 16 | (16 + k))
        nodes = node_count(k)
        flags = rng.getrandbits(32) & ~((1 << 8) | (1 << 10) | (1 << 29))
        if rng.random() < .1:
            flags |= 1 << 10
        if rng.random() < .2:
            flags |= 1 << 29
        put(body + 4, flags)
        kind = rng.choice([world_word(v, world, definitions[k]) & 0xff] * 3 + [0, 1, 6, 7, 11])
        if kind & 31 in (0, 1) and not rendered(k):  # 114750 and F2CC0 read the render model unchecked
            kind = rng.choice([2, 6, 7, 11])
        put8(body + 0xaa, kind)
        put8(body + 0xb3, rng.choice([0, 0, 0, 0, 1]))
        put(body + 0xb4, rng.choice([NONE_] * 8 + [5, 6]))
        put8(body + 0xc0, rng.choice([0, 0, 0x40, 0xbf, 0xff]))
        world.track[body + 0xc1 - T] &= ~2  # BCB70's 10FB70 override (+C0 bit 9) stays off
        put(body + 0xd4, rng.choice([NONE_] * 8 + [3]))
        put8(body + 0xd8, rng.choice([0, 0, 1, 0xfe]))
        floats(body + 0x64, 3, span=50)
        unit(body + 0x70)
        unit(body + 0x7c)
        put(body + 0xa0, rng.choice([f2u(1.0)] * 6 + [f2u(2.0), f2u(0.5), rfloat(rng, special=.6, nan=.2)]))
        # A zero-node block leaves BF0C0's node mask unzeroed (the original
        # then tests uninitialized stack), so blocks hold at least one node.
        size = max(nodes, 1) * 0x34 if rng.random() < .95 else 0x34 * rng.choice([8, 9])
        struct.pack_into('<hh', world.track, body + 0x114 - T, size, NODES)
        own = rng.random() < .8
        struct.pack_into('<hh', world.track, body + 0x110 - T, nodes * 0x20, ORIENTATIONS if own else -1)
        struct.pack_into('<h', world.track, body + 0x12a - T, rng.choice([-1, -1, -1, MANAGER]))
        animate(v, world, rng, body)
        put(body + MANAGER + 0x68, rng.choice([NONE_, NONE_, (0x6800 + rng.randrange(3)) << 16 | 26 + rng.randrange(3)]))
        put(body + 0x138, NONE_)
        # Device position (+18C) and overlay (+1AC) channels (10C0B0).
        channel_record(v, world, rng, body + 0x18c, .5)
        channel_record(v, world, rng, body + 0x1ac, .3)
        struct.pack_into('<h', world.track, body + 0x33e - T, UNIT_BLOCK)
        put(body + UNIT_BLOCK + 0x28, NONE_)
        put8(body + 0x34b, rng.choice([0] * 6 + [1, 2, 3]))
        put8(body + 0x3dc, rng.choice([0, 3]))
        put(body + 0x3ec + 0x5c, NONE_)
        for node in range(9):
            floats(body + NODES + 0x34 * node, 13, span=8)
            put(body + NODES + 0x34 * node, rng.choice([f2u(1.0)] * 4 + [f2u(2.0), rfloat(rng, special=.5)]))
        for node in range(9):
            quaternion(body + ORIENTATIONS + 0x20 * node)
            quaternion(body + PREVIOUS + 0x20 * node)
        unit_fields(v, world, rng, body)
        # Previously registered cluster sphere: the node 0 origin for an
        # unchanged sphere, a small or a large move, or arbitrary.
        floats(body + 0x30, 12, span=8)
        parent = NONE_ if n == 0 or rng.random() < .45 else handles[rng.randrange(n)]
        parents[n] = parent
        put(body + 0x14, parent)
        put8(body + 0x18, rng.choice([0, 0, 1, 2, 3]))
        put(body + 0xc, NONE_)
        put(body + 0x10, NONE_)
    for n in range(v.ROWS - 1):
        body = BODIES + BODY * n
        if rng.random() < .1:
            put(body + 0x138, rng.choice(handles))
        if rng.random() < .1:
            put(body + UNIT_BLOCK + 0x28, rng.choice(handles))
    for n in reversed(range(v.ROWS - 1)):
        parent = parents[n]
        if parent != NONE_:
            pbody = BODIES + BODY * (parent & 0xffff)
            put(BODIES + BODY * n + 0xc, word(pbody + 0x10))
            put(pbody + 0x10, handles[n])
    physics_fixture(v, world, rng, handles, models)
    return handles


# ---- Havok-driven nodes (1DBBD0) and crate bounds (1DC7A0) ----
# FACTORY layout after the definitions and seats: component rows, record
# index arrays, rigid bodies, motions (16-byte aligned transforms at +80),
# motion vtables, physics models (hlmt +24, tag rows 29..31) with their
# +3C records (0x90) and +CC node tables (12 bytes), shapes, the hkWorld
# [0x52BE48] and its hk3AxisSweep (+C4) with nodes and axes, and a mask.
PHYS_ROWS, PHYS_INDICES = FACTORY_BASE + 0x1800, FACTORY_BASE + 0x1a40
PHYS_BODIES, PHYS_MOTIONS = FACTORY_BASE + 0x1a80, FACTORY_BASE + 0x1cc0
MOTION_VTABLES, PHYS_MODELS = FACTORY_BASE + 0x21a0, FACTORY_BASE + 0x2260
PHYS_RECORDS, PHYS_NODES = FACTORY_BASE + 0x24d0, FACTORY_BASE + 0x30a0
PHYS_SHAPES, HK_WORLD = FACTORY_BASE + 0x3200, FACTORY_BASE + 0x3280
HK_BROADPHASE, HK_BROADPHASE_VTABLE = FACTORY_BASE + 0x3380, FACTORY_BASE + 0x3400
HK_NODES, HK_AXES = FACTORY_BASE + 0x3440, FACTORY_BASE + 0x34a0
PHYS_MASK, PHYS_OUT = FACTORY_BASE + 0x3600, FACTORY_BASE + 0x3640
MOTION_GETTERS = [0x31d520, 0x31e2f0, 0x2e8cf0, 0x31dd70, 0x31edf0, 0x31ede0]  # types 2..7
PHYSICS_MODEL_ROWS = 29
PHYS_RECORD_COUNT, PHYS_NODE_COUNT = 6, 9


def physics_fixture(v, world, rng, handles, models):
    """Components (datums 5, 6) with up to three rows each, their rigid
    bodies and motions, a physics model per object model (node tree with
    parent < child), and an hk3AxisSweep whose handles index six nodes."""
    put = world.put

    def flt(x):
        return f2u(x)

    # Motion vtables: +18 is one of the six literal getType owners.
    for k, getter in enumerate(MOTION_GETTERS):
        for w in range(8):
            put(MOTION_VTABLES + 0x20 * k + 4 * w, getter if w == 6 else 0)
    # Physics models: records (index -1 is a padding record) and node trees.
    for m in range(3):
        phmo = PHYS_MODELS + 0xd0 * m
        put(v.TAG_TABLE + 16 * (PHYSICS_MODEL_ROWS + m) + 8, phmo)
        records = PHYS_RECORDS + 0x90 + 0x90 * (PHYS_RECORD_COUNT + 1) * m
        nodes = PHYS_NODES + 12 * PHYS_NODE_COUNT * m
        put(phmo + 0x3c, records)
        put(phmo + 0xcc, nodes)
        count = rng.choice([1, 2, 3, 5, 9, 9])
        parents = [0xffff] + [rng.randrange(n) for n in range(1, PHYS_NODE_COUNT)]
        if rng.random() < .2:
            parents[rng.randrange(1, PHYS_NODE_COUNT)] = 0xffff  # a second root
        children = {n: [c for c in range(PHYS_NODE_COUNT) if parents[c] == n and c < count] for n in range(PHYS_NODE_COUNT)}
        for n in range(PHYS_NODE_COUNT):
            siblings = children.get(parents[n], []) if parents[n] != 0xffff else []
            after = [c for c in siblings if c > n]
            struct.pack_into('<hhhhhh', world.factory, nodes + 12 * n - FACTORY_BASE, 0, 0, 0,
                             -1 if parents[n] == 0xffff else parents[n], after[0] if after else -1,
                             children[n][0] if children[n] else -1)
        for r in range(-1, PHYS_RECORD_COUNT):
            record = records + 0x90 * r
            node = rng.choice([0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0xffff]) if r >= 0 else rng.choice([0, 1, 0xffff])
            struct.pack_into('<H', world.factory, record - FACTORY_BASE, node)
            put(record + 0x80, rng.choice([0, 0, flt(rng.uniform(0, 2)), flt(rng.uniform(0, 2)), f2u(-0.0), NANS[0],
                                           0x00000003, flt(1.5)]))
    for hlmt in models:
        put(hlmt + 0x24, (0x7000 + PHYSICS_MODEL_ROWS) << 16 | (PHYSICS_MODEL_ROWS + rng.randrange(3)))
    # Rigid bodies, motions and shapes (6 of each; rows pick among them).
    for b in range(6):
        body = PHYS_BODIES + 0x60 * b
        motion = PHYS_MOTIONS + 0xd0 * b
        shape = PHYS_SHAPES + 0x10 * (b % 4)
        put(body + 0xc, shape)
        put(body + 0x1c, rng.randrange(6))  # collidable +10: broadphase handle
        put(body + 0x3c, motion)
        put(body + 0x40, rng.choice([0, 0, 0, 1]))
        put(motion, MOTION_VTABLES + 0x20 * rng.choice([0, 1, 2, 3, 4, 4, 5]))
        for k in range(2):
            for lane in range(3):
                put(motion + 0x40 + 0x10 * k + 4 * lane, rfloat(rng, span=4))
        for row in range(4):
            for lane in range(4):
                put(motion + 0x80 + 0x10 * row + 4 * lane,
                    rfloat(rng, special=.05, nan=.01 if row < 3 else .15, span=1 if row < 3 else 20)
                    if lane < 3 or rng.random() < .5 else 0)
    for s in range(4):
        put(PHYS_SHAPES + 0x10 * s, PHYS_SHAPES + 0x40)  # vtable: +34 is never called (a stop)
    # Components: rows of 0x60 (+40 rigid body, +48 record indices, +4C count).
    # Their +8 object must have a model and a render model (1DBBD0/1DC7A0
    # dereference both unchecked); without one the component is empty.
    def modeled(handle):
        body = BODIES + BODY * (handle & 0xffff)
        definition = DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)
        model = world_word(v, world, definition + 0x38)
        return model != NONE and world_word(v, world, models[(model & 0xffff) - 20] + 4) != NONE
    valid = [h for h in handles if modeled(h)]
    for c in range(2):
        component = COMPONENTS + 0xa0 * c
        rows = PHYS_ROWS + 0x120 * c
        flags = rng.getrandbits(32)
        flags &= ~(1 << 9) if rng.random() < .9 else ~0
        flags |= (1 << 5) | (1 << 11) if rng.random() < .85 else 0
        put(component + 4, flags)
        put(component + 8, rng.choice(valid or handles))
        for k in range(16):
            put(component + 0x30 + 4 * k, rfloat(rng, special=.05, nan=.01, span=2))
        put(component + 0x70, rows)
        put(component + 0x74, rng.choice([1, 1, 2, 2, 3, 0, NONE]) if valid else 0)
        for r in range(3):
            row = rows + 0x60 * r
            indices = PHYS_INDICES + 8 * (3 * c + r)
            put(row + 0x40, PHYS_BODIES + 0x60 * rng.randrange(6))
            put(row + 0x48, indices)
            put(row + 0x4c, rng.choice([1, 1, 2, 3, 4, 5, 6, 0, NONE]))
            for k in range(8):
                world.factory[indices + k - FACTORY_BASE] = rng.choice([0, 1, 2, 3, 4, 5, 5, 0xff])
    # hkWorld and its broadphase.
    world.globals[0x52be48] = HK_WORLD
    put(HK_WORLD + 0xc4, HK_BROADPHASE)
    put(HK_BROADPHASE, HK_BROADPHASE_VTABLE)
    for w in range(16):
        put(HK_BROADPHASE_VTABLE + 4 * w, 0x2eb0c0 if w == 8 else 0)
    for lane in range(4):
        put(HK_BROADPHASE + 0x10 + 4 * lane, rfloat(rng, special=.05, nan=.01, span=200))
    for k in range(3):
        put(HK_BROADPHASE + 0x30 + 4 * k, rng.choice([flt(rng.uniform(1, 300)), flt(rng.uniform(1, 300)),
                                                       flt(65532 / 1000.0), flt(3.0), f2u(-2.0), 0]))
    put(HK_BROADPHASE + 0x40, HK_NODES)
    for a, at in enumerate((0x4c, 0x58, 0x64)):
        put(HK_BROADPHASE + at, HK_AXES + 0x60 * a)
        for e in range(16):
            struct.pack_into('<HH', world.factory, HK_AXES + 0x60 * a + 4 * e - FACTORY_BASE,
                             rng.choice([0, 1, 0xfffe, rng.randrange(0x10000), rng.randrange(0x10000)]), rng.randrange(6))
    for n in range(6):
        struct.pack_into('<6H', world.factory, HK_NODES + 16 * n - FACTORY_BASE, *[rng.randrange(16) for _ in range(6)])


def physics_nodes_case(v, world, rng):
    """1DBBD0 on a scene component: its object, flags, rows and a world mask."""
    handles = scene(v, world, rng, 'physics')
    component = COMPONENTS + 0xa0 * rng.randrange(2)
    for k in range(8):
        world.put(PHYS_MASK + 4 * k, rng.getrandbits(32) & rng.getrandbits(32) & 0x1ff if rng.random() < .3 else 0)
    height = rng.choice([0, 0, f2u(-0.125), rfloat(rng, special=.3, nan=.05), rng.choice(NANS)])
    return [component, PHYS_MASK, height]


def crate_sphere_case(v, world, rng):
    """1DC7A0 on a scene component into a scratch center/radius."""
    scene(v, world, rng, 'crate')
    component = COMPONENTS + 0xa0 * rng.randrange(2)
    if rng.random() < .15:
        definition = DEFINITIONS + DEFINITION * rng.randrange(4)
        world.put(definition + 0x1c, world_word(v, world, definition + 0x1c) | 2)
    for k in range(4):
        world.put(PHYS_OUT + 4 * k, rng.getrandbits(32))
    return [component, PHYS_OUT, PHYS_OUT + 0xc]


def bounds_case(v, world, rng):
    handles = scene(v, world, rng, 'bounds')
    # No attachment blocks or owned lights (+4 bits 4 and 6) and no cluster
    # references (+60) for BA7E0's C0F60/1D30F0: the light and partition
    # worlds are nodebuild_bounds_connected's (placement's fixtures).
    for k in range(v.ROWS - 1):
        at = BODIES + BODY * k
        world.put(at + 4, struct.unpack_from('<I', world.track, at + 4 - v.TRACK)[0] & ~0x50)
        world.put(at + 0x60, NONE)
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    if rng.random() < .6:
        # Center = node 0 position exactly: unit scale, zero offsets.
        definition = DEFINITIONS + DEFINITION * (struct.unpack_from('<I', world.track, body - v.TRACK)[0] & 3)
        nodes = body + NODES
        world.put(nodes, f2u(1.0))
        for k in range(3):
            world.put(definition + 8 + 4 * k, 0)
            world.put(definition + 0x24 + 4 * k, 0)
        world.put(body + 0xa0, f2u(1.0))
        radius = world_word(v, world, definition + 4)
        if rng.random() < .5:
            world.put(definition + 0x20, 0)
        alt = world_word(v, world, definition + 0x20)
        flags = world_word(v, world, definition) >> 16
        chosen = alt if (flags >> 12) & 1 and struct.unpack('<f', struct.pack('<I', alt))[0] > 0 else radius
        delta = rng.choice([0.0, 0.0, 0.05, 0.2, 5.0])
        for k in range(3):
            position = struct.unpack('<f', world.track[nodes + 0x28 + 4 * k - v.TRACK:nodes + 0x2c + 4 * k - v.TRACK])[0]
            world.put(body + 0x40 + 4 * k, f2u(position + (delta if k == 0 else 0.0)))
        same = rng.random() < .8
        world.put(body + 0x4c, chosen if same else f2u(9.0))
        # Connected objects (unchanged or moved) are nodebuild_bounds_connected's:
        # BA7E0/BA630 need the placement world it builds.
    return [handles[n]]


_placement = {}


def world_bank(v, world, address):
    for base, bank in ((v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (v.TRACK, world.track),
                       (v.FACTORY, world.factory), *world.extra.items()):
        if bank is not None and base <= address < base + len(bank):
            return bank, address - base
    raise ValueError(f'undeclared {address:x}')


def world_word(v, world, address):
    return struct.unpack_from('<I', *world_bank(v, world, address))[0]


def bounds_connected_case(v, world, rng):
    """BFF20 on the placement track's reconnect world (map, clusters,
    partitions, lights and light attachments): a connected object whose
    cluster sphere moved disconnects (BA7E0) and reconnects (BA630)."""
    if not _placement:
        import native_object_factory_cases_placement as placement
        placement.register(v)
        _placement['reconnect'] = v.TRACK_ARGUMENTS['placement_reconnect_lights']
    object_ = _placement['reconnect']('object_reconnect_lights', world, rng)[0]
    D = v.DATA
    body = world.body(object_ & 0xffff)
    # Node 0 matrix at body+380 (+116), scale +A0, the definition's bounds.
    struct.pack_into('<h', world.data, body + 0x116 - D, 0x380)
    for k in range(13):
        world.put(body + 0x380 + 4 * k, rfloat(rng, special=.05, nan=.01, span=4))
    world.put(body + 0x380, rng.choice([f2u(1.0), f2u(1.0), f2u(2.0), rfloat(rng, special=.4)]))
    world.put(body + 0xa0, rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), rfloat(rng, special=.3)]))
    definition = world_word(v, world, v.TAG_TABLE + 16 * (world_word(v, world, body) & 0xffff) + 8)
    for offset, value in ((4, f2u(rng.choice([0.5, 1.0, 2.5, 0.0]))), (8, rfloat(rng, span=1)), (0xc, rfloat(rng, span=1)),
                          (0x10, rfloat(rng, span=1)), (0x20, rng.choice([0, f2u(1.5), f2u(-1.0), f2u(0.25)])),
                          (0x24, rfloat(rng, span=1)), (0x28, rfloat(rng, span=1)), (0x2c, rfloat(rng, span=1))):
        world.put(definition + offset, value)
    bank, at = world_bank(v, world, definition + 2)
    struct.pack_into('<H', bank, at, struct.unpack_from('<H', bank, at)[0] & ~0x1000 | rng.choice([0, 0x1000]))
    connected = world_word(v, world, body + 4)
    world.put(body + 4, connected | 1 << 8 if rng.random() < .85 else connected & ~(1 << 8))
    return [object_]


def animated_root(v, world, rng, n):
    """Object n samples an animation whose channel 0 is resource-gated
    (1CF210 returns through 1E5C80), with no other channels, overlays or
    orientation hooks, so BF0C0 reaches B87B0/B8780 and, for animations
    flagged +16 bit 1, the world-relative root."""
    T, put = v.TRACK, world.put
    body = BODIES + BODY * n
    struct.pack_into('<h', world.track, body + 0x12a - T, MANAGER)
    put(body + 4, struct.unpack_from('<I', world.track, body + 4 - T)[0] & ~(1 << 29))
    world.track[body + 0xb3 - T] = 0
    world.track[body + 0x34b - T] = 0
    world.track[body + 0xaa - T] = rng.choice([2, 3, 6, 11, 12])
    g, high = rng.randrange(3), rng.randrange(8)
    manager = body + MANAGER
    put(manager, (0x6800 + g) << 16 | (26 + g))
    put(manager + 4, high << 16 | 0xffff)
    world.track[manager + 0x10 - T] = 1
    world.track[GRAPH_ROWS + 0x400 * g + 0x6c * high + 0x10 - T] = 1
    world.track[GRAPH_ROWS + 0x400 * g + 0x6c * high + 0x16 - T] = rng.choice([0x02, 0x12, 0x00])
    put(manager + 0x20, NONE)
    put(manager + 0x40, NONE)
    put(manager + 0x68, (0x6800 + g) << 16 | (26 + g))
    put(GRAPHS_ + 0x400 * g + 0x44, 0)


def node_case(v, world, rng):
    handles = scene(v, world, rng, 'nodes')
    n = rng.randrange(v.ROWS - 1)
    if rng.random() < .3:
        animated_root(v, world, rng, n)
    return [handles[n]]


def rebuild_case(v, world, rng):
    handles = scene(v, world, rng, 'rebuild')
    if rng.random() < .3:
        animated_root(v, world, rng, rng.randrange(v.ROWS - 1))
    roots = [h for n, h in enumerate(handles)
             if struct.unpack_from('<I', world.track, BODIES + BODY * n + 0x14 - v.TRACK)[0] == NONE]
    return [rng.choice(roots if rng.random() < .8 else handles)]


# ---- Animation codec (283A90) ----
# Codec banks: the second-frame and decoded orientations 0x510EF0/0x510F10
# and the state block 0x510F30..0x510FA3; 2835B0's cleared block
# 0x56E000..0x56E05F; the literal per-format decoder table 0x48C33C.
CODEC_BANK, CODEC_BANK_SIZE = 0x510ef0, 0xb4
CODEC_CLEARED = 0x56e000
FORMAT_TABLE, FORMATS = 0x48c33c, 9
NATIVE_TRACK_DECODERS = [0x296180, 0x296680, 0x296450, 0x296240, 0x2963d0, 0x2961f0, 0x296410, 0x296220, 0x2cec60,
                         0x296c60, 0x296db0, 0x296ad0, 0x297270, 0x297400, 0x296700, 0x296540, 0x2962f0, 0x296620,
                         0x296890, 0x2969e0]
UNPORTED_TRACK_DECODERS = {0x2cebe0, 0x2cec20, 0x297550, 0x296e90, 0x297030, 0x297180}
FRAME_DECODERS = [0x2847d0, 0x284d30, 0x285a70, 0x2861a0, 0x2868a0, 0x28ada0, 0x28b5f0, 0x28e730, 0x28f110, 0x290710,
                  0x2912a0, 0x291e30, 0x28b080, 0x28eaf0, 0x28a7d0, 0x28e0b0, 0x28c8f0, 0x28bf00,
                  # kind 4 (283E10's event orientations): modes 0/1/2, unmasked and masked
                  0x293290, 0x2928e0, 0x2947b0, 0x293cb0, 0x295da0, 0x295280,
                  # kind 1 (284140/284330, 2026-11-15): mode 0 (with its second-frame
                  # loops), mode 1 and mode 2, masked and unmasked
                  0x287130, 0x287a20, 0x286b30, 0x287480, 0x2883c0, 0x288e70, 0x289920, 0x28a3f0]
# Frame loops 283A90 never selects (second-frame flag, kind 1) or that no
# census executes; each is refused under its own address. Kind 4's
# second-frame loops are 292C90/292280, 2940F0/2935C0 and 2956D0/294B80.
FRAME_REFUSALS = {0x2887f0, 0x287d00, 0x289d60, 0x289240, 0x2843f0, 0x2849f0, 0x284ee0, 0x285370, 0x285630, 0x285cf0, 0x286460,
                  0x28b870, 0x28c2c0, 0x284360, 0x28f460, 0x28fba0, 0x290030, 0x290b50, 0x291740,
                  0x292c90, 0x292280, 0x2940f0, 0x2935c0, 0x2956d0, 0x294b80}
# Resource cache miss (125FB0), pending request release (1262F0's call of
# 16E8A0 at 0x126307: placement's datum deletes run 16E8A0 natively), load
# wait (1236A0), admission (126200). 283E10's event path is native (its
# 120570/120740 calls included).
# A cache miss now runs natively up to its resource read (1A3B70 at
# 0x126139); the cache age rollover (13FD10) is refused.
RESOURCE_STOPS = {0x126139, 0x13fd10, 0x126307, 0x1236a0, 0x126200}
BOUNDARY_OWNERS[0x126307] = '16E8A0'
BOUNDARY_OWNERS[0x126139] = '1A3B70'
INDIRECT_TARGETS[0x13fe50] = [0x1263c0]
INDIRECT_TARGETS[0x140310] = [0x126400]
CODEC_STOPS = FRAME_REFUSALS | UNPORTED_TRACK_DECODERS | RESOURCE_STOPS
CODEC_CUT = {0x1a3b70, 0x2dbf74}
for _site in FRAME_DECODERS:
    INDIRECT_TARGETS[_site] = NATIVE_TRACK_DECODERS

# TRACK tail: codec blobs (static block +0, animated block +800, flag
# streams +1000), inherited-entry remaps and masks, and the resource cache.
CODEC_BLOBS, BLOB_STRIDE, BLOBS = TRACK_BASE + 0xa000, 0x1100, 3  # a fourth blob follows (+28 offsets)
ENTRY_REMAPS, ENTRY_MASKS = TRACK_BASE + 0xe400, TRACK_BASE + 0xe800
REQUESTS, CACHE_ENTRIES, CACHE, CACHE_RECORDS = TRACK_BASE + 0xf000, TRACK_BASE + 0xf200, TRACK_BASE + 0xf400, TRACK_BASE + 0xf500
CODEC_FORMATS = [1, 1, 3, 3, 4, 4, 6, 8, 8, 5, 7, 2]


def put16_(world, address, value):
    struct.pack_into('<H', world.track, address - TRACK_BASE, value & 0xffff)


def put8_(world, address, value):
    world.track[address - TRACK_BASE] = value & 0xff


def quantized(world, rng, at, words):
    for n in range(words):
        w = [rng.choice([rng.randrange(-32768, 32768), 0, 32767, -32768, rng.randrange(-200, 200)]) for _ in range(2)]
        if rng.random() < .02:
            w = [0, 0]
        world.put(at + 4 * n, (w[0] & 0xffff) | (w[1] & 0xffff) << 16)


def codec_floats(world, rng, at, count, span=4.0, nan=.01):
    for n in range(count):
        world.put(at + 4 * n, rfloat(rng, special=.05, nan=nan, span=span))


def sorted_keys(world, rng, at, count, words, frames):
    keys = sorted(rng.choice([rng.randrange(0, frames + 1), rng.randrange(0, frames + 1), rng.randrange(0, 300)])
                  for _ in range(count))
    for n, key in enumerate(keys):
        if words:
            put16_(world, at + 2 * n, key)
        else:
            put8_(world, at + n, key)


def codec_block(world, rng, at, fmt, frames=4, nan=.01):
    """One 0x800-byte codec block of format `fmt` at `at` for up to 12
    tracks of each kind and frame indices below `frames`."""
    put8_(world, at, fmt)
    if fmt == 1:
        world.put(at + 0xc, 0x200)
        world.put(at + 0x10, 0x300)
        quantized(world, rng, at + 0x20, 24)
        codec_floats(world, rng, at + 0x200, 36, span=8, nan=nan)
        codec_floats(world, rng, at + 0x300, 12, span=2)
    elif fmt in (2, 3, 8):  # format 2 shares 2CEC60 (its rotation/translation decoders are refused)
        rotation = 0x40 if fmt == 8 else 0x20
        translation, scale = (0x340, 0x580) if fmt == 8 else (0x280, 0x600)
        for offset, value in ((0xc, translation), (0x10, scale), (0x14, rotation), (0x18, 0x30), (0x1c, 0x10)):
            world.put(at + offset, value)
        if fmt != 8:
            quantized(world, rng, at + 0x20, 12 * rotation // 4)
        else:
            for n in range(0x300 // 16):
                q = [rng.uniform(-1, 1) for _ in range(4)]
                norm = sum(c * c for c in q) ** .5 or 1.0
                for k in range(4):
                    world.put(at + 0x20 + 16 * n + 4 * k, f2u(q[k] / norm) if rng.random() < .95 else rfloat(rng, nan=.2))
        codec_floats(world, rng, at + translation, 12 * 0x30 // 4, span=8, nan=nan)
        codec_floats(world, rng, at + scale, 12 * 0x10 // 4, span=2, nan=nan)
    elif fmt in (4, 5, 6, 7):
        words = fmt in (5, 7)
        for offset, value in ((0xc, 0x60), (0x10, 0x90), (0x14, 0x100), (0x18, 0x140), (0x1c, 0x180), (0x20, 0x200),
                              (0x24, 0x300), (0x28, 0x400)):
            world.put(at + offset, value)
        for table in (0x30, 0x60, 0x90):
            for t in range(12):
                world.put(at + table + 4 * t, rng.randrange(0, 9) << 12 | rng.choice([0, 1, 2, 3, 5, 8]))
        sorted_keys(world, rng, at + 0x100, 18, words, frames)
        sorted_keys(world, rng, at + 0x140, 18, words, frames)
        sorted_keys(world, rng, at + 0x180, 18, False, frames)
        quantized(world, rng, at + 0x200, 18 * 2)
        codec_floats(world, rng, at + 0x300, 18 * 3, span=8, nan=nan)
        codec_floats(world, rng, at + 0x400, 18, span=2)


def codec_streams(world, rng, at, fmt):
    for n in range(0x40):
        flags = 0 if rng.random() < .25 else rng.getrandbits(8)
        # 297550 (format 5/7 scale) is not native: keep its stream mostly clear.
        if fmt in (5, 7) and rng.random() < .9 and n % 12 >= 8:
            flags = 0
        put8_(world, at + n, flags)


def codec_blob(world, rng, at, frames=4, nan=.01):
    """Static block (+0), animated block (+800) and flag streams (+1000)."""
    static, animated = rng.choice(CODEC_FORMATS), rng.choice(CODEC_FORMATS)
    codec_block(world, rng, at, static, frames, nan)
    codec_block(world, rng, at + 0x800, animated, frames, nan)
    codec_streams(world, rng, at + 0x1000, animated)
    return static, animated


# 283E10 publishes event orientations (256 rows of 0x20) at 0x50EF10 for
# the kind-4 loops; its event rows (animation +64 count, +68 rows of 0x1C)
# and the graph's node rows (+C count, +10 rows of 0x20: +4 sibling, +6
# child, +8 parent) live in the TRACK bank.
EVENT_ORIENTATIONS, EVENT_ORIENTATIONS_SIZE = 0x50ef10, 0x1fe0  # 255 rows: row 255 is the codec bank (0x510EF0)
EVENT_ROWS, EVENT_ROW_COUNT = TRACK_BASE + 0xe980, 0x38
GRAPH_NODES = TRACK_BASE + 0x9c00  # two tables of 16 nodes


def event_bank(world, rng, nan=None):
    bank = bytearray(rng.randbytes(EVENT_ORIENTATIONS_SIZE))
    nan = rng.choice([.02, .02, .3]) if nan is None else nan
    for n in range(16):
        q = [rng.uniform(-1, 1) for _ in range(4)]
        norm = sum(c * c for c in q) ** .5 or 1.0
        words = [f2u(c / norm) if rng.random() < .9 else rfloat(rng, nan=nan) for c in q]
        words += [rfloat(rng, special=.1, nan=nan, span=4) for _ in range(3)]
        words.append(rng.choice(NANS) if rng.random() < nan else
                     rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), rfloat(rng, special=.3, nan=nan)]))
        struct.pack_into('<8I', bank, 0x20 * n, *words)
    world.extra[EVENT_ORIENTATIONS] = bank


def graph_nodes(world, rng, table, count=16):
    """A node forest over a random node order (a parent precedes its
    children in that order, so ancestry walks end; a child may have a lower
    index than its parent): +4 next sibling, +6 first child, +8 parent, -1
    for none; rarely an index beyond the codec count."""
    order = list(range(count))
    if rng.random() < .7:
        rng.shuffle(order)
    parent = [-1] * count
    for i in range(1, count):
        if rng.random() < .9:
            parent[order[i]] = rng.choice([-1] + order[:i])
    children = {n: [c for c in range(count) if parent[c] == n] for n in range(count)}
    for n in range(count):
        row = table + 0x20 * n
        kids = children[n]
        put16_(world, row + 6, kids[0] if kids else rng.choice([-1, -1, -1, -2]))
        p = parent[n]
        sibs = children[p] if p >= 0 else []
        later = [c for c in sibs if c > n]
        put16_(world, row + 4, later[0] if later else rng.choice([-1, -1, -1, -5]))
        put16_(world, row + 8, p)


def event_rows(world, rng, at, count, nodes=12):
    for n in range(count):
        row = at + 0x1c * n
        # Negative nodes stay near the entry's mask and remap tables (283E10
        # reads them before its sign test).
        put16_(world, row, rng.choice(list(range(nodes)) + [rng.randrange(nodes + 4), -1, -2, rng.randrange(-64, 0)]))
        put8_(world, row + 2, rng.getrandbits(8))
        quantized(world, rng, row + 4, 2)
        codec_floats(world, rng, row + 0xc, 3, span=3, nan=.05)
        world.put(row + 0x18, rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), 0, f2u(2.0), rfloat(rng, special=.3, nan=.05)]))


def codec_banks(v, world, rng):
    world.extra[CODEC_BANK] = bytearray(rng.randbytes(CODEC_BANK_SIZE))
    world.extra[CODEC_CLEARED] = bytearray(rng.randbytes(0x60))
    event_bank(world, rng)
    # 296540/2962F0 stage their keys at 0x56E688..0x56E6B7 (with 144CF0's
    # epsilon/flag at 0x56E680, which other builders may declare first).
    statics = bytearray(rng.randbytes(0x38))
    if 0x56e680 not in world.extra or len(world.extra[0x56e680]) < 0x38:
        world.extra[0x56e680] = statics
    world.extra[FORMAT_TABLE] = original_bytes(v, FORMAT_TABLE, FORMATS * 40)
    for n in range(2):  # decoded and second-frame orientations: unit quaternions mostly
        at = CODEC_BANK + 0x20 * n
        q = [rng.uniform(-1, 1) for _ in range(4)]
        norm = sum(c * c for c in q) ** .5 or 1.0
        for k in range(4):
            struct.pack_into('<I', world.extra[CODEC_BANK], at - CODEC_BANK + 4 * k, f2u(q[k] / norm))


def orientation_rows(world, rng, at, count):
    nan = rng.choice([.01, .01, .3])  # NaN-rich rows make operand orders observable
    for n in range(count):
        q = [rng.uniform(-1, 1) for _ in range(4)]
        norm = sum(c * c for c in q) ** .5 or 1.0
        for k in range(4):
            world.put(at + 0x20 * n + 4 * k, f2u(q[k] / norm) if rng.random() < .95 else rfloat(rng, nan=.2))
        codec_floats(world, rng, at + 0x20 * n + 0x10, 3, span=8, nan=nan)
        world.put(at + 0x20 * n + 0x1c, rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), rfloat(rng, special=.3), 0]))


def inherited_entry(world, rng, entry, remap, mask, nodes=12, total=12):
    world.put(entry + 0xc, remap)
    world.put(entry + 0x14, mask)
    world.put(entry + 0x18, rng.choice([f2u(1.0), f2u(0.5), f2u(2.0), rfloat(rng, special=.3, nan=.05)]))
    put8_(world, entry + 0x1c, rng.choice([0, 1, 2, 3, 0xff]))
    for n in range(nodes):
        put16_(world, remap + 2 * n, rng.choice([n, n, rng.randrange(-2, total + 2), 0xffff, 0x8000]))
    for k in range(8):
        world.put(mask + 4 * k, rng.choice([NONE, NONE, 0, rng.getrandbits(32)]))


def codec_frames_case(v, world, rng, second_masked=False, events=False, partial=False):
    """283570 over a direct state block: any kind/mode/mask/flag, a codec
    block of a random format and its flag streams. `second_masked` forces
    the masked second-frame loops (28A7D0/28E0B0) with NaN-rich tracks, so
    their layering operand order is observable."""
    T = v.TRACK
    world.track = bytearray(rng.randbytes(v.TRACK_SIZE))
    codec_banks(v, world, rng)
    blob = T + 0x1000
    # NaN-rich translations on both sides make the layering operand order
    # (NaN payload selection) observable.
    nan = .4 if second_masked or events or partial else rng.choice([.01, .01, .4])
    static, animated = codec_blob(world, rng, blob, nan=nan)
    fmt, block = rng.choice([(static, blob), (animated, blob + 0x800)])
    table = original_bytes(v, FORMAT_TABLE, FORMATS * 40)
    forced = rng.random() < .1
    state = lambda offset, value: struct.pack_into('<I', world.extra[CODEC_BANK], 0x40 + offset, value & 0xffffffff)
    state(0x00, rng.choice([0, 0, 0, 2, 2, 3, 3, 3, 1, 4, 5]))
    state(0x04, rng.choice([0, 0, 1, 1, 2, 2, 3]))
    count = rng.choice([0, 1, 2, 3, 5, 8, 9, 12, 12, 0x80000000])
    state(0x0c, count)
    state(0x10, rng.choice([0, 1, 3, 8, 12, 12]))
    state(0x14, rng.randrange(4))
    state(0x18, rng.randrange(4))
    state(0x1c, rfloat(rng, special=.2, nan=.05, span=1))
    state(0x20, rng.choice([f2u(rng.uniform(0, 1)), f2u(1.0), f2u(0.5), rfloat(rng, special=.4, nan=.1, span=2)]))
    entry = T + 0x200
    state(0x28, entry)
    inherited_entry(world, rng, entry, T + 0x240, T + 0x280, total=12)
    mask = rng.choice([0, 0, T + 0x80])
    if mask:
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([NONE, NONE, 0, rng.getrandbits(32)]))
    state(0x2c, mask)
    state(0x30, block)
    for k in range(3):
        state(0x34 + 4 * k, struct.unpack_from('<I', table, fmt * 40 + 12 * forced + 4 * k)[0])
    streams = blob + 0x1000 + rng.choice([0, 0, 4, 8])
    for k in range(3):
        state(0x40 + 4 * k, streams + 4 * k)
    state(0x4c, T + 0x3000)
    world.extra[CODEC_BANK][0x40 + 0x50] = 1 if second_masked or rng.random() < .25 else 0
    if partial:
        # Kind 1 (284140): every mode, masked or not; the second-frame flag
        # mostly with mode 0 (modes 1/2 with it are refused boundaries).
        state(0x00, 1)
        mode = rng.choice([0, 0, 1, 2])
        state(0x04, mode)
        world.extra[CODEC_BANK][0x40 + 0x50] = int(rng.random() < (.4 if mode == 0 else .05))
        state(0x20, rng.choice([f2u(rng.uniform(0, 1)), f2u(0.0), f2u(1.0), rfloat(rng, special=.4, nan=.2, span=2)]))
        if rng.random() < .5:
            mask = T + 0x80
            for k in range(8):
                world.put(mask + 4 * k, rng.choice([NONE, NONE, 0, rng.getrandbits(32)]))
            state(0x2c, mask)
    if events:
        # Kind 4 (283E10's event planes and rows): every mode, masked or
        # not, without the second-frame flag; the planes mostly set.
        state(0x00, 4)
        state(0x04, rng.choice([0, 1, 2]))
        world.extra[CODEC_BANK][0x40 + 0x50] = 0
        if rng.random() < .5:
            mask = T + 0x80
            for k in range(8):
                world.put(mask + 4 * k, rng.choice([NONE, NONE, 0, rng.getrandbits(32)]))
            state(0x2c, mask)
        event_bank(world, rng, nan=.4)
        planes = world.extra[CODEC_CLEARED]
        for k in range(0x18):
            struct.pack_into('<I', planes, 4 * k, rng.choice([NONE, NONE, rng.getrandbits(32), 0]))
    if second_masked:
        state(0x00, rng.choice([2, 3]))
        state(0x04, 0)
        mask = T + 0x80
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([NONE, 0, rng.getrandbits(32)]))
        state(0x2c, mask)
    # 2835B0 supplies both frame pairs before enabling the second-frame
    # loops. Keep this direct-state fixture equally valid now that 28B080
    # and 28EAF0 reach the second pair; random bank bytes here index outside
    # the four-frame codec block and fault the unchanged original.
    state_bytes = world.extra[CODEC_BANK]
    state_word = lambda offset: struct.unpack_from('<I', state_bytes, 0x40 + offset)[0]
    state(0x54, state_word(0x18))
    state(0x58, state_word(0x14))
    state(0x5c, state_word(0x1c))
    state(0x60, state_word(0x20))
    if partial and rng.random() < .6:
        # A second-frame fraction apart from the weight (kind 1 uses both).
        state(0x60, rng.choice([f2u(rng.uniform(0, 1)), rfloat(rng, special=.4, nan=.2, span=2)]))
    orientation_rows(world, rng, T + 0x3000, 16)
    if nan > .1:
        for n in range(16):
            for k in range(3):
                if rng.random() < .4:
                    world.put(T + 0x3000 + 0x20 * n + 0x10 + 4 * k, rng.choice(NANS))
    if partial:
        # NaN rotation lanes of both signs: kind 1's sign-alignment dot
        # ((p0+p2)+(p1+p3)) then keeps an observable NaN sign.
        for n in range(16):
            if rng.random() < .3:
                for k in range(4):
                    if rng.random() < .5:
                        world.put(T + 0x3000 + 0x20 * n + 4 * k, rng.choice([0x7fc00000, 0xffc00000, 0x7fc12345, 0xffc54321]))
    return []


def codec_setup_case(v, world, rng):
    """2835B0 with a 1E5DF0 record over an animation row and codec blob."""
    T = v.TRACK
    world.track = bytearray(rng.randbytes(v.TRACK_SIZE))
    codec_banks(v, world, rng)
    blob = T + 0x1000
    frames = rng.choice([1, 2, 3, 4, 4, 0])
    codec_blob(world, rng, blob, frames=max(frames, 1))
    animation = T + 0x400
    nodes = rng.choice([0, 1, 3, 5, 9, 12, 12])
    put8_(world, animation + 0x10, rng.choice([0, 0, 1, 1, 2]))
    put8_(world, animation + 0x13, nodes)
    put16_(world, animation + 0x14, frames)
    world.put(animation + 0x64, rng.choice([0] * 5 + [NONE, 0x80000000, 1, 1, 2, 3]))
    world.put(animation + 0x68, T + 0x4000 + 0x1c * rng.randrange(4))
    event_rows(world, rng, T + 0x4000, 8)
    graph = T + 0x4c00
    graph_nodes(world, rng, T + 0x4800)
    world.put(graph + 0xc, 16)
    world.put(graph + 0x10, T + 0x4800)
    put8_(world, animation + 0x3c, rng.choice([0, 0, 4, 8]))
    put16_(world, animation + 0x42, rng.choice([0x800, 0x800, 0x800, 0]))
    world.put(animation + 0x48, rng.choice([0x800, 0x800, 0x800, 0]))
    block = T + 0x500
    world.put(block, blob)
    world.put(block + 4, animation + 0x3c)
    put8_(world, block + 8, nodes if rng.random() < .9 else rng.choice([0, 33, 200]))
    entry = rng.choice([0, 0, T + 0x200])
    inherited_entry(world, rng, T + 0x200, T + 0x240, T + 0x280, total=12)
    mask = rng.choice([0, 0, T + 0x80])
    for k in range(8):
        world.put(T + 0x80 + 4 * k, rng.choice([NONE, NONE, 0, rng.getrandbits(32)]))
    orientation_rows(world, rng, T + 0x3000, 16)
    frame = rng.choice([f2u(rng.uniform(0, max(frames, 1))), f2u(rng.randrange(max(frames, 1))),
                        f2u(rng.randrange(max(frames, 1)) + rng.choice([0.99995, 0.00005, 0.5])),
                        f2u(-1.0), f2u(rng.uniform(-3, 8)), NANS[0], f2u(3e9)])
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), f2u(0.99995), f2u(0.99990), f2u(1e-4), f2u(5e-5), 0,
                         NANS[1], f2u(rng.uniform(0, 1)), f2u(2.0)])
    flag = 1 if rng.random() < .3 else 0
    second = rng.choice([f2u(rng.uniform(-1, 5)), f2u(1.0), NANS[0], f2u(rng.randrange(4))])
    return [block, frame, weight, graph, animation, rng.choice([0, 1, 3, 5, 9, 12]), entry, mask,
            T + 0x3000, rng.choice([0, 0, 1, 0x100, 0xff]), flag, second, f2u(rng.uniform(0, 1))]


def event_setup_case(v, world, rng):
    """2835B0 forced through 283E10: kind 0 at full weight, two or three
    event rows (nodes up to and including the count), no entry or mask, a
    shuffled node forest and orientations with zero, NaN and NaN-payload
    scales and translations (120740's zero-scale reciprocal and sum
    orders)."""
    T = v.TRACK
    values = codec_setup_case(v, world, rng)
    animation = values[4]
    count = rng.choice([3, 5, 9, 12])
    put8_(world, animation + 0x10, 0)
    world.put(animation + 0x48, 0x800)
    world.put(animation + 0x64, rng.choice([2, 3]))
    world.put(animation + 0x68, T + 0x4000)
    for n in range(3):
        row = T + 0x4000 + 0x1c * n
        put16_(world, row, rng.choice([count, count, rng.randrange(count), rng.randrange(count)]))
    order = list(range(16))
    rng.shuffle(order)
    parent = [-1] * 16
    for i in range(1, 16):
        parent[order[i]] = rng.choice([-1] + order[:i])
    for n in range(16):
        row = T + 0x4800 + 0x20 * n
        kids = sorted(c for c in range(16) if parent[c] == n)
        sibs = sorted(c for c in range(16) if parent[c] == parent[n] and c > n) if parent[n] >= 0 else []
        put16_(world, row + 6, kids[0] if kids else -1)
        put16_(world, row + 4, sibs[0] if sibs else -1)
        put16_(world, row + 8, parent[n])
    orientations = values[8]
    for n in range(16):
        at = orientations + 0x20 * n
        for k in range(3):
            world.put(at + 0x10 + 4 * k, rng.choice([rfloat(rng, span=4), rng.choice(NANS), rfloat(rng, span=4)]))
        world.put(at + 0x1c, rng.choice([0, 0, f2u(1.0), f2u(0.5), rng.choice(NANS), f2u(-0.0)]))
    values[2] = f2u(1.0)
    values[5] = count
    values[6] = values[7] = 0
    values[9] = values[10] = 0
    return values


def codec_cache(v, world, rng):
    """The resource cache (0x4F0418/1C/20/24): three cached blocks holding
    codec blobs, an occasional pending request and unfinished load."""
    T = v.TRACK
    shift = rng.choice([0, 4, 8])
    world.globals[0x4f0420] = CODEC_BLOBS
    world.globals[0x4f0418] = CACHE_ENTRIES
    world.globals[0x4f041c] = REQUESTS
    world.globals[0x4f0424] = CACHE
    for n in range(BLOBS + 1):
        codec_blob(world, rng, CODEC_BLOBS + BLOB_STRIDE * n)
    for n in range(BLOBS):
        entry = CACHE_ENTRIES + 0x100 + 16 * n
        put8_(world, entry + 4, rng.choice([1, 1, 1, 0]))
        world.put(entry + 8, rng.choice([NONE, NONE, NONE, 0x12340001]))
        record = CACHE_RECORDS + 24 * n
        world.put(record + 8, (BLOB_STRIDE * n) >> shift)
    world.put(CACHE_ENTRIES + 0x44, CACHE_ENTRIES + 0x100)
    world.put(CACHE + 0x34, shift | rng.choice([0, 0, 0x20, 0xffffff00]))
    world.put(CACHE + 0x64, CACHE + 0x80)
    world.put(CACHE + 0x80 + 0x44, CACHE_RECORDS)
    world.put(REQUESTS + 0x24, 0x10)
    world.put(REQUESTS + 0x38, rng.choice([0] * 9 + [2]))
    world.put(REQUESTS + 0x44, REQUESTS + 0x100)
    world.put(REQUESTS + 0x48, REQUESTS + 0x140)
    world.put(REQUESTS + 0x140, rng.choice([0, 1, 2, 3]))
    for n in range(2):  # pending requests name other rows
        world.put(REQUESTS + 0x100 + 16 * n + 8, GRAPHS_ + 0x400 * rng.randrange(3) + 0x200 + 20 * rng.randrange(8) +
                  rng.choice([0, 2, 2, 2]))
    cache_pages_fixture(v, world, rng, shift)


# A cache miss (1262F0 -> 125FB0) allocates pages through 13FE50 over the
# cache owner (+20 delete 126400 / +24 busy 1263C0 callbacks, +2C policy,
# +30 total pages, +38 tick, +3C/+40 record list, +44 budgets, +64 the
# 24-byte page records: +4 pages, +8 start, +C next, +10 previous, +14
# tick), claims the entry at the page's index (16E770) and reads the
# resource (1260C0, which stops at its 1A3B70 read).
CACHE_RECORD_BITS, CACHE_ENTRY_BITS = CACHE + 0xe0, CACHE_ENTRIES + 0xc0


def cache_pages_fixture(v, world, rng, shift):
    put = world.put
    # 1260C0's literal per-mode priority/age table (12-byte rows).
    world.extra[0x474d70] = original_bytes(v, 0x474d70, 0x24)
    pages = BLOB_STRIDE >> shift
    handles = [(0x5100 + n) << 16 | n for n in range(BLOBS)]
    put(CACHE + 0x20, 0x126400)  # the live cache always deletes entries with their pages
    put(CACHE + 0x24, rng.choice([0x1263c0, 0x1263c0, 0x1263c0, 0]))
    put(CACHE + 0x2c, rng.randrange(3))
    put(CACHE + 0x30, BLOBS * pages + rng.choice([0, 1, 0x100 >> shift, 0x2000 >> shift, 0x4000 >> shift]))
    tick = rng.choice([1000, 1000, 300, 0x7ffffffe])  # records are never younger than the cache tick
    put(CACHE + 0x38, tick)
    put(CACHE + 0x3c, handles[0])
    put(CACHE + 0x40, handles[-1])
    for k in range(8):
        put(CACHE + 0x44 + 4 * k, 0x7fffffff if rng.random() < .9 else rng.choice([0, 1, 4]))
    pool = CACHE + 0x80
    for at, value in ((0x20, 16), (0x24, 24), (0x28, 0), (0x2c, 0), (0x30, 0), (0x34, 0), (0x38, BLOBS), (0x3c, BLOBS),
                      (0x40, 0x8000), (0x48, CACHE_RECORD_BITS)):
        put(pool + at, value)
    put(CACHE_RECORD_BITS, (1 << BLOBS) - 1)
    for n in range(16):
        record = CACHE_RECORDS + 24 * n
        if n >= BLOBS:
            for k in range(6):
                put(record + 4 * k, 0)
            continue
        put(record, 0x5100 + n)
        put(record + 4, pages)
        put(record + 8, n * pages)
        put(record + 12, handles[n + 1] if n + 1 < BLOBS else NONE)
        put(record + 16, handles[n - 1] if n else NONE)
        put(record + 20, max(0, tick - rng.choice([0, 1, 10, 290, 990])))
    for at, value in ((0x20, 16), (0x24, 16), (0x28, 0), (0x2c, 0), (0x30, 0), (0x34, 0), (0x38, BLOBS), (0x3c, BLOBS),
                      (0x40, 0x8000), (0x48, CACHE_ENTRY_BITS)):
        put(CACHE_ENTRIES + at, value)
    put(CACHE_ENTRY_BITS, (1 << BLOBS) - 1)
    for n in range(16):
        entry = CACHE_ENTRIES + 0x100 + 16 * n
        if n >= BLOBS:
            for k in range(4):
                put(entry + 4 * k, 0)
            continue
        put(entry, (world_word(v, world, entry) & 0xffff0000) | (0x5100 + n))
        put(entry + 0xc, GRAPHS_ + 0x400 * rng.randrange(3) + 0x200 + 20 * rng.randrange(8))
    # Resource rows: block sizes the cache can hold (+4 bytes).
    for g in range(3):
        for k in range(8):
            put(GRAPHS_ + 0x400 * g + 0x200 + 20 * k + 4, rng.choice([1, 0x200, 0x201, rng.randrange(1, 0x2000)]))


OVERLAY_ROWS, OVERLAY_EVENTS = FACTORY_BASE + 0x3700, FACTORY_BASE + 0x3800
# BFB90's boundaries: channel 2 post-processing, the node-mask
# intersection, the resource admission, the type orientation hooks, the
# first overlay entry of BF9A0 (whose loop callees are cut) and the codec's
# remaining refusals.
# The CRT floor/ceil (32B199/32B0C6, from 283B50) of a NaN pitch row run
# their error reporters after changing the x87 control word, where no
# boundary can stop: the fixtures keep pitch rows numeric (283B50 refuses a
# NaN one) and the reporters are cut.
# BF9A0's overlays are native: BCB70 runs only its literal-name answers
# (its 10FB70/10DE40/BC8F0 subtrees are cut), 37DC0A its FPREM/FLDZ
# methods (the CRT error methods stop at 3334F0).
SAMPLE_STOPS = {0x1d63c0, 0x126200, 0x3334f0} | CODEC_STOPS
SAMPLE_CUT = {0x10fb70, 0x10de40, 0xbc8f0, 0x32ebc3, 0x32ea95, 0x32eae8} | CODEC_CUT
BOUNDARY_OWNERS[0x3334f0] = '3334F0'
# 1DBBD0's non-root record child shape transform (shape vtable +34) is refused.
PHYSICS_STOPS = {0x1dbe23}
BOUNDARY_OWNERS[0x1dbe23] = '1DBBD0'
INDIRECT_TARGETS[0x1dbbd0] = MOTION_GETTERS
INDIRECT_TARGETS[0x1dc7a0] = [0x2eb0c0]
NODE_STOPS = {0xb3ae0, 0xb3b70} | SAMPLE_STOPS | PHYSICS_STOPS
# 10E100's orientation hooks: unit 114750, vehicle F34B0, device 10C0B0.
INDIRECT_TARGETS[0x10e100] = [0x114750, 0x10c0b0, 0xf34b0]
INDIRECT_TARGETS[0xf2cc0] = [0xf2c40]  # F34B0's channel callback
composite('nodebuild_codec_frames', 0x283570, 'void', CODEC_STOPS, 1200, cut=CODEC_CUT)
composite('nodebuild_codec_events', 0x283570, 'void', CODEC_STOPS, 400, cut=CODEC_CUT,
          native='nodebuild_codec_frames')
composite('nodebuild_codec_second_masked', 0x283570, 'void', CODEC_STOPS, 300, cut=CODEC_CUT,
          native='nodebuild_codec_frames')
composite('nodebuild_codec_partial', 0x283570, 'void', CODEC_STOPS, 600, cut=CODEC_CUT,
          native='nodebuild_codec_frames')
composite('nodebuild_codec_setup', 0x2835b0, 'void', CODEC_STOPS, 800, cut=CODEC_CUT, regs={'esi': 0}, xmm={1: 1, 4: 2},
          stack=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12])
composite('nodebuild_codec_event_setup', 0x2835b0, 'void', CODEC_STOPS, 300, cut=CODEC_CUT, regs={'esi': 0}, xmm={1: 1, 4: 2},
          stack=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12], native='nodebuild_codec_setup')
composite('nodebuild_resource_data', 0x1e5a70, 'long', CODEC_STOPS, 400, cut=CODEC_CUT, regs={'ecx': 0}, stack=[1])
composite('nodebuild_codec', 0x283a90, 'void', CODEC_STOPS, 800, cut=CODEC_CUT, regs={'eax': 0},
          stack=[1, 2, 3, 4, 5, 6, 7, 8])
composite('nodebuild_sample', 0xbfb90, 'void', SAMPLE_STOPS, 800, cut=SAMPLE_CUT,
          regs={'eax': 0, 'ecx': 1, 'edx': 2}, stack=[3, 4, 5])
composite('nodebuild_device_hook', 0x10c0b0, 'void', SAMPLE_STOPS, 400, cut=SAMPLE_CUT, stack=[0, 1, 2, 3])
composite('nodebuild_vehicle_hook', 0xf34b0, 'void', SAMPLE_STOPS, 800, cut=SAMPLE_CUT, stack=[0, 1, 2, 3])
composite('nodebuild_unit_orientation_hook', 0x114750, 'void', SAMPLE_STOPS, 1000, cut=SAMPLE_CUT, stack=[0, 1, 2, 3])
composite('nodebuild_aim_channel', 0x1cf3b0, 'void', SAMPLE_STOPS, 600, cut=SAMPLE_CUT, regs={'esi': 0},
          stack=[1, 2, 3, 4, 5, 6])
composite('nodebuild_fraction_channel', 0x1cf4b0, 'void', SAMPLE_STOPS, 400, cut=SAMPLE_CUT, regs={'ebx': 0, 'edx': 1},
          stack=[2, 3, 4, 5])
# BA7E0/BA630 are the placement track's natives; their closures stop at
# placement's own boundaries (Havok, unported light paths, other tracks).
PLACEMENT_HAVOK = {0x1d97f0, 0x1d9510, 0x1d8470, 0x1d73d0, 0x1dc270, 0x1d7ca0, 0x1cd130, 0x1ccbe0, 0x1cb330, 0x1cb220,
                   0x149680, 0x1498a0}
PLACEMENT_UNPORTED = {0xba570, 0xe3e20, 0x16d740}  # C3750, 1692E0, 145070, FC510, C2870, C39B0 native
PLACEMENT_OTHER = {0xb9b70, 0x179610, 0xbaf10, 0xbbac0, 0xcea70, 0x1d3380, 0xc0270, 0xbc5c0, 0xc0910, 0xfa820, 0x102ad0}
PLACEMENT_STOPS = PLACEMENT_HAVOK | PLACEMENT_UNPORTED | PLACEMENT_OTHER
INDIRECT_TARGETS[0x10dde0] = [0x118120]  # machine type callback (placement)
BOUNDS_STOPS = PLACEMENT_STOPS
composite('nodebuild_physics_nodes', 0x1dbbd0, 'void', PHYSICS_STOPS, 800, stack=[0, 1, 2])
composite('nodebuild_crate_sphere', 0x1dc7a0, 'boolean', set(), 600, stack=[0, 1, 2])
composite('nodebuild_bounds', 0xbff20, 'boolean', BOUNDS_STOPS, 600, stack=[0])
composite('nodebuild_bounds_connected', 0xbff20, 'boolean', BOUNDS_STOPS, 600, native='nodebuild_bounds', stack=[0])
composite('nodebuild_node_matrices', 0xbf0c0, 'void', NODE_STOPS, 1200, cut=SAMPLE_CUT, stack=[0])
composite('nodebuild_rebuild', 0xbf050, 'void', NODE_STOPS | BOUNDS_STOPS, 600, cut=SAMPLE_CUT, stack=[0])


def animation_graphs(v, world, rng):
    """Three jmad graphs (tag rows 26..28) for the scene's managers: eight
    0x6C animations, inheritance entries (+8..+14 codec block fields),
    resource rows with zero bytes, render masks at +5C/+7C and overlay
    counts (+44) that are usually empty."""
    T = v.TRACK
    put = world.put
    graphs = []
    for g in range(3):
        data = GRAPHS_ + 0x400 * g
        graphs.append(data)
        put(v.TAG_TABLE + 16 * (26 + g) + 8, data)
    for table in range(2):
        graph_nodes(world, rng, GRAPH_NODES + 0x200 * table)
    event_rows(world, rng, EVENT_ROWS, EVENT_ROW_COUNT)
    for g, data in enumerate(graphs):
        rows = GRAPH_ROWS + 0x400 * g
        put(data + 0x30, rows)
        put(data + 0xc, 16)
        put(data + 0x10, GRAPH_NODES + 0x200 * (g & 1))
        put(data + 0x44, rng.choice([0, 0, 0, 0, 0, NONE, 0x80000000, 1, 2, 4]))
        inherited = rng.choice([0, 1, 2, 3])
        put(data + 0x4c, inherited)
        put(data + 0x50, data + 0x100)
        for k in range(4):
            entry = data + 0x100 + 32 * k
            put(entry + 4, (0x7800 + k) << 16 | (26 + rng.randrange(3)))
            # 1E32A0 admits an entry with +8/+10 counts and +C/+14 tables.
            inherited_entry(world, rng, entry, ENTRY_REMAPS + 0x40 * (4 * g + k), ENTRY_MASKS + 0x20 * (4 * g + k),
                            total=9)
            put(entry + 8, rng.choice([9, 9, 9, 0, NONE]))
            put(entry + 0x10, rng.choice([9, 9, 9, 0]))
            if rng.random() < .15:
                put(entry + rng.choice([0xc, 0x14]), 0)
        put(data + 0xb0, data + 0x200)
        for k in range(8):
            world.track[data + 0x200 + 20 * k + 0x10 - T] = 0
            # Cache handle (+C): one of the three cached blocks, rarely a miss.
            n = rng.randrange(BLOBS)
            put(data + 0x200 + 20 * k + 0xc, NONE if rng.random() < .03 else (0x5100 + n) << 16 | n)
        for k in range(16):
            put(data + 0x5c + 4 * k, rng.choice([0, 0, NONE, 1 << rng.randrange(32)]))
        for a in range(8):
            row = rows + 0x6c * a
            world.track[row + 0x10 - T] = rng.choice([0, 1])
            world.track[row + 0x11 - T] = rng.getrandbits(8)
            world.track[row + 0x13 - T] = rng.choice([1, 3, 5, 9, 9, 12, 0])
            struct.pack_into('<H', world.track, row + 0x14 - T, rng.choice([1, 2, 3, 4]))
            world.track[row + 0x16 - T] = rng.choice([0x10, 0x10, 0x12, 0x10, 0x02, 0])
            put(row + 0x24, rng.randrange(8))
            # Codec data (1E5A70): +28 offset into the cached block, +38 the
            # data of a non-resource animation; header +3C (+0 stream shift,
            # +6 animated block, +C streams) and 283E10's event count +64.
            put(row + 0x28, rng.choice([0, 0, 0, BLOB_STRIDE]))
            put(row + 0x38, CODEC_BLOBS + BLOB_STRIDE * rng.randrange(BLOBS + 1))
            world.track[row + 0x3c - T] = rng.choice([0, 0, 4, 8])
            # 283A90 prefetches header-sized spans (+0..+F summed): sizes.
            world.track[row + 0x3d - T] = rng.randrange(16)
            struct.pack_into('<HH', world.track, row + 0x3e - T, rng.randrange(0x100), rng.randrange(0x100))
            put(row + 0x44, rng.randrange(0x100))
            struct.pack_into('<H', world.track, row + 0x42 - T, rng.choice([0x800, 0x800, 0x800, 0]))
            put(row + 0x48, rng.choice([0x800, 0x800, 0x800, 0x800, 0]))
            put(row + 0x64, rng.choice([0] * 9 + [1, 1, 2, 3, NONE]))
            put(row + 0x68, EVENT_ROWS + 0x1c * rng.randrange(EVENT_ROW_COUNT - 3))
            struct.pack_into('<H', world.track, row + 0x30 - T, rng.choice([0xffff, 0xffff, rng.randrange(8)]))
            later = list(range(a + 1, 8))
            struct.pack_into('<H', world.track, row + 0x32 - T, rng.choice([0xffff] + later) if later else 0xffff)
            # Aim record index (+12, 1E31E0): 0xFF for none.
            world.track[row + 0x12 - T] = rng.choice([0xff, 0xff, 0, 1, 2])
        hook_tables(v, world, rng, data, inherited)
    # BF9A0 function overlays (+48 rows of 0x14: +4 animation, +A mode, +C
    # function). Functions are none or the literal names BCB70 answers
    # without object state (0x30005A6 -> 1.0, 0x40005A7 -> 0.0); bodies keep
    # +C0 bit 9 clear (no 10FB70 override).
    for g, data in enumerate(graphs):
        rows = OVERLAY_ROWS + 0x50 * g
        put(data + 0x48, rows)
        for k in range(4):
            row = rows + 0x14 * k
            for w in range(5):
                put(row + 4 * w, rng.getrandbits(32))
            a = rng.randrange(8)
            put(row + 4, rng.choice([a << 16 | 0xffff, a << 16 | 0xffff, a << 16 | 0xffff, NONE]))
            put(row + 8, (world_word(v, world, row + 8) & 0xffff) | rng.choice([0, 0, 1, 1, 2]) << 16)
            put(row + 0xc, rng.choice([0, 0x30005a6, 0x30005a6, 0x40005a7]))
            # 37DC0A runs the fmod by the animation's frame count (nonzero);
            # 1CE810 -> 1CEC60 reads its +4C events (4 bytes, +2 frame).
            animation = GRAPH_ROWS + 0x400 * g + 0x6c * a
            if struct.unpack_from('<H', world.track, animation + 0x14 - v.TRACK)[0] == 0:
                put16(world, animation + 0x14, rng.choice([1, 30]))
        # Every animation (random variants and aliases pick any of them).
        for b in range(8):
            animation = GRAPH_ROWS + 0x400 * g + 0x6c * b
            put(animation + 0x4c, rng.choice([0, 0, 1, 3, 6]))
            put(animation + 0x50, OVERLAY_EVENTS)
    for e in range(8):
        put(OVERLAY_EVENTS + 4 * e, rng.randrange(0x10000) | rng.choice([0, 1, 2, 5, 29, 31]) << 16)
    return graphs


# Keys of the scene graphs' mode tables (graph +34) and of the managers'
# +70/+74/+78; 0x30000D9 is the tables' fallback key.
MODE_KEYS = [[0x1000001, 0x30000d9], [0x2000002, 0x30000d9], [0x3000003, 0x30000d9]]
OVERLAY_NAMES = [0x8000078, 0x4000079, 0x800007a, 0x800007b, 0xc00007d, 0x110005c9, 0xe000044, 0xe000045, 0xb000046,
                 0x400004c]


def scene_handle(rng, inherited):
    return rng.randrange(8) << 16 | rng.choice([0xffff, 0xffff] + list(range(inherited)))


def hook_tables(v, world, rng, data, inherited):
    """A graph's tables for the orientation hooks: the aim records (+28 rows
    of 0x1C, 1E31E0/283B50), a one-path mode table (+34 -> level 2 -> one
    0x34 leaf whose +C overlay table maps names to handles, 1E3F30) and the
    vehicle's +40 rows (count +3C, 0x28 bytes, handle +4, 1E5160)."""
    put = world.put
    aim = data + 0x330
    put(data + 0x28, aim)
    for k in range(3):
        record = aim + 0x1c * k + 4
        # Pitch scales stay finite: a NaN pitch row reaches the CRT floor's
        # error path (283B50 refuses it; the original changes the x87 control
        # word before it, so no boundary can stop there).
        for offset in (0, 4, 0xc, 0x10):
            put(record + offset, rng.choice([f2u(rng.uniform(.2, 2)), f2u(rng.uniform(.2, 2)), f2u(1.0), 0, f2u(5e-5),
                                             f2u(-1.0)] + ([NANS[0]] if offset < 8 else [])))
        for offset in (8, 0xa, 0x14, 0x16):
            put16_(world, record + offset, rng.choice([0, 1, 2, 3, 4, rng.randrange(0x10000)]))
    level1, level2, leaf, entries = data + 0x2a0, data + 0x2b4, data + 0x2c8, data + 0x2fc
    put(data + 0x34, rng.choice([1, 1, 1, 0]))
    put(data + 0x38, level1)
    put(level1, rng.choice(MODE_KEYS[0]))
    put(level1 + 4, 1)
    put(level1 + 8, level2)
    put(level2, rng.choice(MODE_KEYS[1]))
    put(level2 + 4, 1)
    put(level2 + 8, leaf)
    for k in range(0, 0x34, 4):
        put(leaf + k, 0)
    put(leaf, rng.choice(MODE_KEYS[2]))
    names = sorted(rng.sample(OVERLAY_NAMES, rng.randrange(7)))
    put(leaf + 0xc, len(names))
    put(leaf + 0x10, entries)
    for n, name in enumerate(names):
        put(entries + 8 * n, name)
        put(entries + 8 * n + 4, scene_handle(rng, inherited) if rng.random() < .9 else NONE)
    rows = data + 0x384
    put(data + 0x3c, rng.choice([0, 1, 2, 3, 3]))
    put(data + 0x40, rows)
    for k in range(3):
        put(rows + 0x28 * k + 4, scene_handle(rng, inherited) if rng.random() < .9 else NONE)


GRAPH_TAGS = [(0x6800 + g) << 16 | (26 + g) for g in range(3)]
AIM_DEFAULTS, SEATS = TRACK_BASE + 0x83b0, FACTORY_BASE + 0x1000


def graph_handle(rng):
    """An animation of the graph itself (valid in every scene graph)."""
    return rng.randrange(8) << 16 | 0xffff if rng.random() < .7 else NONE


def unit_fields(v, world, rng, body):
    """The unit (114750) and vehicle (F2CC0) state every scene body carries:
    the aim block (+33E = UNIT_BLOCK, overlapping the previous orientations)
    with flags, fades +80..+93, overlay channels +9C/+BC/+DC (mostly unset)
    and handles +FC..+120; the seat (+1FC), moving weight (+208), weapon
    slots (+212/+213), +13C, the vehicle overlay cache (+400..+424) and the
    vehicle values (+288..+290, +354..+364, +398)."""
    T = v.TRACK
    put = world.put
    aim = body + UNIT_BLOCK
    world.track[aim - T] = rng.getrandbits(8)
    struct.pack_into('<H', world.track, aim + 0x36 - T, rng.choice([0xffff, 0xffff, 0]))
    for k in range(5):
        fade = aim + 0x80 + 4 * k
        length = rng.choice([0, 0, 1, 2, 5, 10, 30])
        world.track[fade - T] = rng.randrange(length + 1) if rng.random() < .9 else rng.getrandbits(8)
        world.track[fade + 1 - T] = length
        world.track[fade + 2 - T] = rng.choice([1, 2, 3, 4, 0, 5])
        world.track[fade + 3 - T] = rng.choice([0, 1])
    for channel in (0x9c, 0xbc, 0xdc):
        channel_record(v, world, rng, aim + channel, 0)
        if rng.random() < .6:  # unset (113C70 reads +9C's animation from its handle alone)
            put(aim + channel, NONE)
            put(aim + channel + 4, NONE)
    # The aim and look handles (+FC, +100, +114) stay unset here: their
    # pitch reaches the CRT floor, whose NaN path cannot be bounded, and the
    # scene's axes and targets may be NaN. unit_hook_case sets them.
    for offset in range(0xfc, 0x124, 4):
        put(aim + offset, NONE if offset in (0xfc, 0x100, 0x114) else graph_handle(rng))
    for offset in (0x168, 0x16c, 0x170, 0x208, 0x288, 0x28c, 0x290, 0x354, 0x35c, 0x360, 0x364):
        put(body + offset, rfloat(rng, special=.2, nan=.03, span=3))
    if rng.random() < .6:
        put(body + 0x208, 0)
    struct.pack_into('<H', world.track, body + 0x1fc - T, rng.choice([0xffff, 0xffff, 0, 1]))
    world.track[body + 0x212 - T] = rng.choice([0xff, 0xff, 0, 1])
    world.track[body + 0x213 - T] = rng.choice([0xff, 0xff, 0, 1])
    put(body + 0x13c, rng.choice([NONE, NONE, 0x12340001]))
    put(body + 0x400, rng.choice([NONE, *GRAPH_TAGS]))
    for k in range(9):
        put(body + 0x404 + 4 * k, graph_handle(rng))


def channel_record(v, world, rng, channel, presence):
    """A 1CF210 channel (0x20 bytes): graph tag, packed id, resource gate
    +10, controls +12, frame +18, codec state +1C."""
    T = v.TRACK
    put = world.put
    present = rng.random() < presence
    g = rng.randrange(3)
    graph = GRAPH_TAGS[g] if present else rng.choice([NONE, GRAPH_TAGS[g]])
    put(channel, graph)
    inherited = struct.unpack_from('<I', world.track, GRAPHS_ + 0x400 * g + 0x4c - T)[0]
    # A set animation always has its graph (113C70 reads it from +6 alone).
    high = rng.choice([rng.randrange(8), rng.randrange(8), 0xffff]) if present else \
        rng.choice([0xffff, 0xffff if graph == NONE else 3])
    low = rng.choice([0xffff, 0xffff] + list(range(inherited)))
    put(channel + 4, high << 16 | low)
    # Resource-gated channels return early (1E5C80 request), letting BF0C0
    # reach its manager-animation (world-relative) root.
    world.track[channel + 0x10 - T] = rng.choice([0, 0, 1]) if high != 0xffff else 0
    struct.pack_into('<H', world.track, channel + 0x12 - T, rng.choice([0, 1, 0x40, 0x41, 0x400, 0x800, 0xc01, 0xffff]))
    put(channel + 0x18, rng.choice([0, f2u(1.0), f2u(0.5), f2u(7.0), NANS[0]]))
    put(channel + 0x1c, rng.getrandbits(32))


def animate(v, world, rng, body):
    """An animation manager at body+MANAGER: channels +0/+20/+40 (graph tag,
    packed id, resource gate +10, controls +12, frame +18, codec state +1C),
    interpolation +64, graph +68 and blend fraction +80."""
    T = v.TRACK
    put = world.put
    manager = body + MANAGER
    graph_rows = [(0x6800 + g) << 16 | (26 + g) for g in range(3)]
    for c in range(3):
        channel_record(v, world, rng, manager + 0x20 * c, .6 if c == 0 else .2)
    world.track[manager + 0x64 - T] = rng.randrange(256)
    world.track[manager + 0x65 - T] = rng.choice([0, 0, 1, 4, 30])
    world.track[manager + 0x66 - T] = rng.choice([0, 1, 2, 3, 4, 5])
    world.track[manager + 0x67 - T] = rng.choice([0, 0, 2, 0xfd])
    put(manager + 0x68, rng.choice(graph_rows))
    put(manager + 0x80, rng.choice([f2u(rng.uniform(0, 1)), f2u(0.0), f2u(0.99995), f2u(1.0), NANS[0]]))
    struct.pack_into('<h', world.track, body + 0x10e - T, PREVIOUS)
    return manager


def sample_case(v, world, rng):
    handles = scene(v, world, rng, 'sample')
    world.extra[0x47c1b4] = bytearray([rng.choice([1, 1, 1, 0]), rng.choice([1, 1, 1, 0]), 0, 0])
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    struct.pack_into('<h', world.track, body + 0x12a - v.TRACK, MANAGER)  # BF0C0 passes the body's manager
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    count = rng.choice([0, 1, 2, 3, 5, 8, 8])
    mask = rng.choice([0, 0, 0x56eb8c, 0x56eb8c])
    if mask:
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([0, NONE, 0xffff, 1 << rng.randrange(8), rng.getrandbits(32)]))
    mode = MODES + 0x80 * rng.randrange(3)
    return [count, mask, handles[n], mode, manager, body + ORIENTATIONS]


def graph_animation(v, world, rng):
    """A scene graph and a packed animation id (inherited index or NONE)."""
    g = rng.randrange(3)
    graph = GRAPHS_ + 0x400 * g
    inherited = struct.unpack_from('<I', world.track, graph + 0x4c - v.TRACK)[0]
    low = rng.choice([0xffff, 0xffff] + list(range(inherited)))
    return graph, rng.randrange(8) << 16 | low


def resource_data_case(v, world, rng):
    scene(v, world, rng, 'codec')
    return list(graph_animation(v, world, rng))


def codec_case(v, world, rng):
    """283A90 on a scene graph: an inherited entry or none, a node mask,
    and frame/weight edges."""
    handles = scene(v, world, rng, 'codec')
    graph, id_ = graph_animation(v, world, rng)
    entry = 0
    if id_ & 0xffff != 0xffff and rng.random() < .7:
        entry = graph + 0x100 + 32 * (id_ & 0xffff)
        word = lambda at: struct.unpack_from('<I', world.track, at - v.TRACK)[0]
        if not (word(entry + 0xc) and word(entry + 0x14)):  # 1E32A0 admits only these
            entry = 0
    mask = rng.choice([0, 0, 0x56eb8c])
    if mask:
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([0, NONE, 0xffff, 1 << rng.randrange(8), rng.getrandbits(32)]))
    body = BODIES + BODY * rng.randrange(v.ROWS - 1)
    frame = rng.choice([f2u(rng.uniform(0, 4)), f2u(rng.randrange(4)), f2u(rng.randrange(4) + 0.99995), f2u(-0.5),
                        NANS[0]])
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), f2u(rng.uniform(0, 1)), f2u(1e-5), f2u(0.99995)])
    return [graph, id_, rng.choice([0, 1, 3, 5, 8, 9]), frame, weight, entry, mask, body + ORIENTATIONS,
            rng.choice([0, 0, 1, 0x100])]


def hook_arguments(v, world, rng, body):
    """(mask, count, orientations) as BFB90 passes them to 10E100."""
    count = rng.choice([0, 1, 2, 3, 5, 8, 8])
    mask = rng.choice([0, 0, 0x56eb8c, 0x56eb8c])
    if mask:
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([0, NONE, 0xffff, 1 << rng.randrange(8), rng.getrandbits(32)]))
    return [mask, count, body + ORIENTATIONS]


def device_hook_case(v, world, rng):
    """10C0B0 on a scene object: its +18C/+1AC channels (scene) and manager
    offset, a device or not (the hook reads only the body)."""
    handles = scene(v, world, rng, 'device')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    if rng.random() < .8:
        struct.pack_into('<h', world.track, body + 0x12a - v.TRACK, MANAGER)
    for channel, presence in ((0x18c, .8), (0x1ac, .6)):
        if rng.random() < .7:
            channel_record(v, world, rng, body + channel, presence)
    return [handles[n]] + hook_arguments(v, world, rng, body)


def hook_value(rng, span, nan=.1):
    return rng.choice([f2u(rng.uniform(-span, span)), f2u(rng.uniform(-span, span)), f2u(rng.uniform(0, 1)), 0,
                       f2u(-0.0), f2u(1.0), f2u(1e-5), rfloat(rng, special=.5, nan=nan, span=2 * span)])


def manager_keys(world, rng, manager):
    """Mode keys +70/+74/+78 (1E3F30's r0..r2) and a graph (+68)."""
    for k, offset in enumerate((0x70, 0x74, 0x78)):
        world.put(manager + offset, rng.choice(MODE_KEYS[k] + [MODE_KEYS[k][0], NONE if k else 0x7000101]))


def vehicle_hook_case(v, world, rng):
    """F34B0 on a scene object with a model and render model: steering,
    speed and aim fields, the cached (+400 = manager graph) or refreshed
    overlay handles, definition ramp scales and the graph's +40 rows."""
    handles = scene(v, world, rng, 'vehicle')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    put = world.put
    definition = DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)
    model = rng.randrange(3)
    put(definition + 0x38, (0x5200 + model) << 16 | (20 + model))
    put(MODELS + 0x100 * model + 4, (0x5300 + model) << 16 | (23 + model))
    for offset in (0x1f4, 0x1f8, 0x20c):
        put(definition + offset, rng.choice([f2u(rng.uniform(1, 30)), f2u(rng.uniform(1, 30)), 0, f2u(-2.0), NANS[1]]))
    struct.pack_into('<h', world.track, body + 0x12a - v.TRACK, MANAGER if rng.random() < .9 else -1)
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    graph = world_word(v, world, manager + 0x68)
    inherited = world_word(v, world, GRAPHS_ + 0x400 * ((graph & 0xffff) - 26) + 0x4c)
    # Cached handles stay valid: a refresh needs both keys +74/+78.
    for k in range(9):
        put(body + 0x404 + 4 * k, scene_handle(rng, inherited) if rng.random() < .6 else NONE)
    if rng.random() < .5:
        put(body + 0x400, graph)
    else:
        put(body + 0x400, rng.choice([t for t in [NONE, NONE, *GRAPH_TAGS, 0x12345678] if t != graph]))
    for offset in (0x354, 0x35c, 0x360, 0x364, 0x288, 0x28c, 0x290):
        put(body + offset, hook_value(rng, 3))
    for k in range(4):
        world.track[body + 0x398 + k - v.TRACK] = rng.choice([0xff, 0, 0x80, rng.getrandbits(8)])
    return [handles[n]] + hook_arguments(v, world, rng, body)


def unit_hook_case(v, world, rng):
    """114750 on a scene object with a rendered model: its manager (state
    +7C, flags +6C, keys), the aim and look handles (+FC/+100, +114) and
    the other aim block handles in the manager's graph, the aim and look
    targets (+168, +18C) and the weapon slots."""
    handles = scene(v, world, rng, 'unit')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    T = v.TRACK
    put = world.put
    definition = DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)
    model = rng.randrange(3)
    put(definition + 0x38, (0x5200 + model) << 16 | (20 + model))
    put(MODELS + 0x100 * model + 4, (0x5300 + model) << 16 | (23 + model))
    if rng.random() < .7:
        put(definition + 0xbc, world_word(v, world, definition + 0xbc) & ~(3 << 10))
    struct.pack_into('<h', world.track, body + 0x12a - T, MANAGER)
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    put(manager + 0x7c, rng.choice([0x700005c, 0x700005d, 0xe0000c2, 0x1000001, 0x1000001, rng.getrandbits(32)]))
    world.track[manager + 0x6c - T] = rng.choice([0, 0, 1, 0x10])
    graph = world_word(v, world, manager + 0x68)
    inherited = world_word(v, world, GRAPHS_ + 0x400 * ((graph & 0xffff) - 26) + 0x4c)
    aim = body + UNIT_BLOCK
    for offset in (0xfc, 0x100, 0x114):
        put(aim + offset, scene_handle(rng, inherited) if rng.random() < .85 else NONE)
    for offset in (0x104, 0x108, 0x10c, 0x110, 0x118, 0x11c, 0x120):
        put(aim + offset, scene_handle(rng, inherited) if rng.random() < .3 else NONE)
    for offset in (0x168, 0x16c, 0x170, 0x18c, 0x190, 0x194):
        put(body + offset, f2u(rng.uniform(-10, 10)) if rng.random() < .9 else
            f2u(rng.choice([0.0, -0.0, 1e-40, 1e-4, 1e6])))
    if rng.random() < .3:  # on target: tiny angles (1e-4 tests)
        for k in range(3):
            put(body + 0x18c + 4 * k, world_word(v, world, body + 0x168 + 4 * k))
    # A seated unit: a unit parent, seat +1FC and the +DC overlay live
    # (+EE bit 0, no +ED bits 0/3, aim byte bit 7) for CB4A0/111F70.
    if rng.random() < .3:
        parent = handles[rng.choice([m for m in range(v.ROWS - 1) if m != n])]
        put(body + 0x14, parent)
        world.track[BODIES + BODY * (parent & 0xffff) + 0xaa - T] = rng.choice([0, 1, 1, 6])
        struct.pack_into('<H', world.track, body + 0x1fc - T, rng.choice([0, 1, 1, 0xffff]))
        channel_record(v, world, rng, aim + 0xdc, 1)
        world.track[aim + 0xee - T] |= 1
        world.track[aim + 0xed - T] &= rng.choice([0xf6, 0xf6, 0xff])
        world.track[aim - T] |= rng.choice([0x80, 0x80, 0])
    # Numeric axes and parent node (BBFF0): a NaN aim pitch reaches the CRT
    # floor's error path (refused natively, unbounded in the original).
    axes = []
    for axis in (0x70, 0x7c):
        x = [rng.uniform(-1, 1) for _ in range(3)]
        norm = sum(c * c for c in x) ** .5 or 1.0
        axes.append([c / norm for c in x])
    if rng.random() < .25:  # orthonormal axes, the aim target straight ahead (tiny angles)
        forward, other = axes
        dot = sum(a * b for a, b in zip(forward, other))
        up = [b - dot * a for a, b in zip(forward, other)]
        norm = sum(c * c for c in up) ** .5 or 1.0
        axes = [forward, [c / norm for c in up]]
        scale = rng.uniform(1, 20)
        for k in range(3):
            put(body + 0x168 + 4 * k, f2u(forward[k] * scale))
        put(body + 0x14, NONE)
    for axis, x in zip((0x70, 0x7c), axes):
        for k in range(3):
            put(body + axis + 4 * k, f2u(x[k]))
    parent = world_word(v, world, body + 0x14)
    if parent != NONE:
        node = BODIES + BODY * (parent & 0xffff) + NODES + 0x34 * world.track[body + 0x18 - T]
        for k in range(13):
            put(node + 4 * k, f2u(rng.uniform(-2, 2)))
    return [handles[n]] + hook_arguments(v, world, rng, body)


def channel_case(v, world, rng):
    """A scene object's channel record (+18C) for 1CF3B0/1CF4B0."""
    scene(v, world, rng, 'channel')
    channel = BODIES + BODY * rng.randrange(v.ROWS - 1) + 0x18c
    channel_record(v, world, rng, channel, .9)
    if world_word(v, world, channel) == NONE:  # these samplers read the graph of any set handle
        world.put(channel, GRAPH_TAGS[0])
        world.put(channel + 4, 0xffffffff)
    if rng.random() < .7:
        bank, at = world_bank(v, world, channel + 0x12)
        struct.pack_into('<H', bank, at, rng.choice([0, 0, 0x400, 0x800]))
    return channel


def aim_channel_case(v, world, rng):
    channel = channel_case(v, world, rng)
    mask, count, orientations = hook_arguments(v, world, rng, channel - 0x18c)
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), f2u(rng.uniform(0, 1)), f2u(1e-5), NANS[0]])
    return [channel, hook_value(rng, 3), hook_value(rng, 3, nan=0), weight, mask, count, orientations]


def fraction_channel_case(v, world, rng):
    channel = channel_case(v, world, rng)
    handle = world_word(v, world, channel + 4)
    if handle >> 16 == 0xffff:  # 1CF4B0's callers start the channel first (283D70 reads its animation)
        world.put(channel + 4, rng.randrange(8) << 16 | handle & 0xffff)
    mask, count, orientations = hook_arguments(v, world, rng, channel - 0x18c)
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(0.5), f2u(rng.uniform(0, 1)), f2u(1e-5), NANS[0]])
    return [channel, mask, hook_value(rng, 1.5), weight, count, orientations]


def clamp_channel_fixture(v, world, rng, aimed):
    """A four-frame animation with no resource wait or inherited mask.

    This keeps the upper-clamp inputs observable before codec sampling:
    random channels often stop at a resource/mask boundary first.
    """
    graph, row = GRAPHS_, GRAPH_ROWS
    world.put(graph + 0x4c, 0)
    put8_(world, row + 0x10, 0)
    put8_(world, row + 0x12, 0 if aimed else 0xff)
    put8_(world, row + 0x13, 1)
    put16_(world, row + 0x14, 4)
    put8_(world, row + 0x16, 0)
    world.put(row + 0x24, 0)
    world.put(row + 0x28, 0)
    world.put(row + 0x38, CODEC_BLOBS)
    put8_(world, row + 0x3c, 0)
    put16_(world, row + 0x42, 0x800)
    world.put(row + 0x48, 0x800)
    world.put(row + 0x64, 0)
    for address in (CODEC_BLOBS, CODEC_BLOBS + 0x800):
        codec_block(world, rng, address, 1, nan=0)  # format 1: native decoders, forced or not
    for address in range(CODEC_BLOBS + 0x1000, CODEC_BLOBS + 0x1040, 4):
        world.put(address, NONE)
    return graph


def vehicle_aim_limit_case(v, world, rng):
    values = vehicle_hook_case(v, world, rng)
    body = BODIES + BODY * (values[0] & 0xffff)
    graph = clamp_channel_fixture(v, world, rng, True)
    put16_(world, body + 0x12a, MANAGER)
    world.put(body + MANAGER + 0x68, GRAPH_TAGS[0])
    world.put(body + 0x400, GRAPH_TAGS[0])
    for offset in range(0x404, 0x428, 4):
        world.put(body + offset, NONE)
    world.put(graph + 0x3c, 0)
    world.put(body + 0x41c, 0xffff)  # animation 0, no inherited graph
    world.put(body + 0x288, f2u(3.0))  # maps to 2.0, then clamps to 1.0
    aim = world_word(v, world, graph + 0x28) + 4
    for offset in (0, 4):
        world.put(aim + offset, f2u(2.0))
    for offset in (0xc, 0x10):
        world.put(aim + offset, f2u(1.0))
    for offset, value in ((8, 0), (0xa, 3), (0x14, 0), (0x16, 0)):
        put16_(world, aim + offset, value)
    return [values[0], 0, 1, body + ORIENTATIONS]


def fraction_overflow_case(v, world, rng):
    channel = fraction_channel_case(v, world, rng)[0]
    body = channel - 0x18c
    clamp_channel_fixture(v, world, rng, False)
    world.put(channel, GRAPH_TAGS[0])
    world.put(channel + 4, 0xffff)
    put16_(world, channel + 0x12, 0)
    put8_(world, channel + 0x10, 0)
    # Clamping the float before CVTTSS2SI differs from clamping the integer
    # afterwards: positive infinity converts to 0x80000000, not the last key.
    return [channel, 0, f2u(float('inf')), f2u(1.0), 1, body + ORIENTATIONS]


def unit_yaw_case(v, world, rng):
    """Full aiming weight, zero pitch and nonzero yaw must still sample."""
    values = unit_hook_case(v, world, rng)
    body = BODIES + BODY * (values[0] & 0xffff)
    graph = clamp_channel_fixture(v, world, rng, True)
    manager, aim = body + MANAGER, body + UNIT_BLOCK
    world.put(body + 0x14, NONE)
    world.put(manager + 0x68, GRAPH_TAGS[0])
    world.put(manager + 0x7c, 0x1000001)
    world.put(body + 0x208, 0)
    world.put(manager, GRAPH_TAGS[0])
    world.put(manager + 4, 0xffff)
    for offset in (0x20, 0x40):
        world.put(manager + offset, NONE)
        world.put(manager + offset + 4, NONE)
    definition = DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)
    world.put(definition + 0xbc, 0)
    for offset in (0x9c, 0xbc, 0xdc):
        world.put(aim + offset, NONE)
        world.put(aim + offset + 4, NONE)
    for offset in range(0xfc, 0x124, 4):
        world.put(aim + offset, NONE)
    world.put(aim + 0xfc, 0xffff)
    world.put(aim + 0x90, 0x00030101)  # linear fade, elapsed == duration
    put8_(world, aim, 0)
    put8_(world, body + 0x212, 0xff)
    world.put(body + 0x13c, NONE)
    for offset, vector in ((0x70, (1, 0, 0)), (0x7c, (0, 0, 1)), (0x168, (1, 1, 0))):
        for k, value in enumerate(vector):
            world.put(body + offset + 4 * k, f2u(float(value)))
    record = world_word(v, world, graph + 0x28) + 4
    for offset in (0, 4, 0xc, 0x10):
        world.put(record + offset, f2u(1.0))
    for offset, value in ((8, 0), (0xa, 3), (0x14, 0), (0x16, 0)):
        put16_(world, record + offset, value)
    return [values[0], 0, 1, body + ORIENTATIONS]


composite('nodebuild_vehicle_aim_limit', 0xf34b0, 'void', SAMPLE_STOPS, 16, cut=SAMPLE_CUT,
          native='nodebuild_vehicle_hook', stack=[0, 1, 2, 3])
composite('nodebuild_fraction_overflow', 0x1cf4b0, 'void', SAMPLE_STOPS, 16, cut=SAMPLE_CUT,
          native='nodebuild_fraction_channel', regs={'ebx': 0, 'edx': 1}, stack=[2, 3, 4, 5])
composite('nodebuild_unit_yaw', 0x114750, 'void', SAMPLE_STOPS, 16, cut=SAMPLE_CUT,
          native='nodebuild_unit_orientation_hook', stack=[0, 1, 2, 3])


# ---- BF640 deferred node matrices (1112E0, 110C10, 2130C0, 1159D0, 115500, 1447E0) ----
# TRACK tail after the cache records: the selected section bytes, a player
# pool (+C four datums), marker groups (render +58/+5C) and rows, a 32-byte
# flag mask, the direct 115500 operands, flag-stream blocks (2130C0).
DEFERRED_FREE = TRACK_BASE + 0xf600
SELECTED, PLAYERS_, MARKER_GROUPS_ = DEFERRED_FREE, DEFERRED_FREE + 0x20, DEFERRED_FREE + 0x40
MARKER_ROWS_, OVERLAY_FLAGS = DEFERRED_FREE + 0x80, DEFERRED_FREE + 0x400
IK_MARKER, IK_TARGET, IK_NODE = DEFERRED_FREE + 0x440, DEFERRED_FREE + 0x480, DEFERRED_FREE + 0x490
FLAG_BLOCK, FLAG_HEADER, FLAG_DATA = DEFERRED_FREE + 0x4d0, DEFERRED_FREE + 0x4e0, DEFERRED_FREE + 0x500
EULER_OUT = DEFERRED_FREE + 0x600
REGIONS = 0x9c0  # body +11A region bytes (BAD60)
HAND_MARKER = 0x4000095
DEFERRED_MATRICES, PLAYER_CAMERAS = 0x4e57e8, 0x4f655c


def angle_value(rng):
    return rng.choice([f2u(rng.uniform(-3.5, 3.5)), f2u(rng.uniform(-.6, .6)), 0, f2u(-0.0), f2u(1e-30),
                       f2u(1e20), f2u(-7e18), f2u(float('inf')), NANS[rng.randrange(4)], f2u(3.1415927)])


def deferred_scene(v, world, rng, purpose):
    """The sample scene plus BF640's statics and the overlay, IK and marker
    state every body carries."""
    handles = scene(v, world, rng, purpose)
    T = v.TRACK
    put = world.put
    world.extra[DEFERRED_MATRICES] = bytearray(rng.randbytes(0x4e8bc0 - DEFERRED_MATRICES))
    world.extra[0x5564d8] = bytearray(8)
    put(0x5564d8, rng.choice([f2u(1 / 127)] * 4 + [f2u(-1.0), 0, rfloat(rng, special=.5, nan=.2)]))
    put(0x5564dc, rng.choice([f2u(1 / 255)] * 4 + [f2u(2.0), 0, rfloat(rng, special=.5, nan=.2)]))
    world.extra[0x474c04] = bytearray([rng.choice([1, 1, 1, 0, 0x80]), 0, 0, 0])
    put(0x474c24, rng.choice([0x1ffff, 0x10000, 0x10001, 0x1ffff]))
    world.globals[0x4f54f0] = PLAYERS_
    for k in range(4):
        put(PLAYERS_ + 0xc + 4 * k, rng.choice([NONE, NONE, 0x10002 + k]))
    world.extra[PLAYER_CAMERAS] = bytearray(rng.randbytes(0x358 * 4))
    for k in range(4):
        for c in range(3):
            put(PLAYER_CAMERAS + 0x358 * k + 4 * c, f2u(rng.uniform(-10, 10)) if rng.random() < .95 else rfloat(rng))
    world.extra[0x56e8dc] = bytearray(12)
    put(0x56e8e4, rng.choice([0, 0, 1, 2, 3, 3, 3, 0xfffffffc]))
    put(0x56e8e0, rng.choice([0x3f060a92, 0x3f060a92, f2u(rng.uniform(0, 1)), NANS[0]]))
    put(0x56e8dc, rng.choice([0x3e860a92, 0x3e860a92, f2u(rng.uniform(0, 1)), 0]))
    for k in range(3):
        model = MODELS + 0x100 * k
        # BF640 reads the render model of any modelled object unchecked.
        put(model + 4, (0x5300 + k) << 16 | (23 + k))
        for n in range(8):
            put(model + 0xc4 + 4 * n, rng.choice([0, 0, NONE, rng.getrandbits(32), 1 << rng.randrange(32)]))
        if rng.random() < .5:
            put(model + 0xa4 + 4 * rng.randrange(8), rng.getrandbits(32))
        mode = MODES + 0x80 * k
        put(mode + 0x1c, rng.choice([0, 1, 2, 4, 6, 16]))
        put(mode + 0x58, rng.choice([0, 1, 2, 4, 4]))
        put(mode + 0x5c, MARKER_GROUPS_)
    names = rng.sample([HAND_MARKER, 0x1000001, 0x2000002, 0x3000003], 4)
    for g in range(4):
        group = MARKER_GROUPS_ + 12 * g
        put(group, names[g] if rng.random() < .9 else 0)
        count = rng.choice([0, 1, 1, 2, 3])
        put(group + 4, count)
        rows = MARKER_ROWS_ + 0xc0 * g
        put(group + 8, rows)
        for n in range(3):
            marker = rows + 0x24 * n
            world.track[marker - T] = rng.choice([0xff, 0xff, 0xff, 0, 1])
            world.track[marker + 1 - T] = rng.choice([0, 1, 2])
            world.track[marker + 2 - T] = rng.randrange(9)
            for k in range(3):
                put(marker + 4 + 4 * k, f2u(rng.uniform(-2, 2)))
            q = [rng.uniform(-1, 1) for _ in range(4)]
            norm = sum(c * c for c in q) ** .5 or 1.0
            for k in range(4):
                put(marker + 0x10 + 4 * k, f2u(q[k] / norm) if rng.random() < .95 else rfloat(rng))
    # Some animations long enough for 110C10's stepped frame (17) and 1112E0's frames.
    for g in range(3):
        for a in range(8):
            if rng.random() < .3:
                put16_(world, GRAPH_ROWS + 0x400 * g + 0x6c * a + 0x14, rng.choice([18, 20, 30]))
    for n in range(v.ROWS - 1):
        body = BODIES + BODY * n
        overlay_fields(v, world, rng, body)
        struct.pack_into('<h', world.track, body + 0x11a - T, REGIONS)
        for k in range(16):
            world.track[body + REGIONS + k - T] = rng.choice([0, 1, 2])
    return handles


def overlay_fields(v, world, rng, body):
    """The aim block's overlay state (1112E0/110C10/110240/1114B0) and the
    hand IK state (115940/1159D0)."""
    T = v.TRACK
    put = world.put
    aim = body + UNIT_BLOCK
    world.track[aim + 2 - T] = rng.choice([0, 0, 1, 2, 3, 4, 5, 6, 7, 0xff])
    graph = world_word(v, world, body + MANAGER + 0x68)
    inherited = world_word(v, world, GRAPHS_ + 0x400 * ((graph & 0xffff) - 26) + 0x4c) if graph != NONE else 0
    for offset in (0x30, 0x34, 0x5c):
        put(aim + offset, scene_handle(rng, inherited) if rng.random() < .6 else
            rng.choice([NONE, 0xffff0003, 0x2ffff]))
    put(aim + 0x40, rng.choice([0, 0, 0, f2u(1e-4), 0x38d1b718, f2u(5e-5), f2u(.5), f2u(1.0), f2u(-.3), f2u(3.0),
                                NANS[0], 1, f2u(-0.0)]))
    for k in range(12):
        world.track[aim + 0x44 + k - T] = rng.choice([0, 1, 2, 3, 0xff, 17, 25, 0x7f, 0x80])
        world.track[aim + 0x50 + k - T] = rng.choice([0, 0, 64, 127, 0x80, 0xfb, 1])
    for k in range(13):
        world.track[aim + 0x6e + k - T] = rng.choice([0] * 6 + [255, 128, 1])
    if rng.random() < .3:
        for k in range(13):
            world.track[aim + 0x6e + k - T] = 0
    world.track[body + 0x137 - T] = rng.choice([0, 1, 0xfe, 0xff])
    world.track[body + 0x149 - T] = rng.choice([0, 0x80, 0x7f, 0xff])
    world.track[body + 0x1f6 - T] = rng.choice([0xff, 0xff, 0, 1, 2, 5, 8, 0x80])
    world.track[body + 0x1f7 - T] = rng.choice([0xff, 0xff, 0, 3, 7, 9])
    # BAD60's owner (BACD0): an item in parent mode (+12C, FC510) names +154.
    world.track[body + 0x12c - T] = rng.choice([0, 0, 1, 3, 2])
    put(body + 0x154, rng.choice(world.handles[:v.ROWS - 1] + [NONE]))


def unit_object(v, world, rng, n):
    """Make object n a unit (row +3 types 0/1) with a rendered model and an
    animated manager."""
    T = v.TRACK
    body = BODIES + BODY * n
    kind = rng.choice([0, 1, 0x20, 0x21])
    world.data[v.RECORDS + 12 * n + 3 - v.DATA] = kind
    world.track[body + 0xaa - T] = kind & 31
    # The first-person target +1A4 overlaps the device channels (+18C/+1AC):
    # only units carry it.
    for k in range(3):
        world.put(body + 0x1a4 + 4 * k, f2u(rng.uniform(-10, 10)) if rng.random() < .95 else rfloat(rng))
    definition = DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)
    model = rng.randrange(3)
    world.put(definition + 0x38, (0x5200 + model) << 16 | (20 + model))
    world.put(MODELS + 0x100 * model + 4, (0x5300 + model) << 16 | (23 + model))
    if rng.random() < .6:
        world.put(definition + 0xbc, world_word(v, world, definition + 0xbc) & ~(1 << 11))
    struct.pack_into('<h', world.track, body + 0x12a - T, MANAGER)
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    overlay_fields(v, world, rng, body)
    return body, manager, MODES + 0x80 * model, MODELS + 0x100 * model


def deferred_case(v, world, rng):
    handles = deferred_scene(v, world, rng, 'deferred')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    T = v.TRACK
    # A unit (row +3 types 0/1) always has its manager: BF640's overlay and
    # IK paths read it unchecked. Objects of other types may be units too.
    if (1 << (world.data[v.RECORDS + 12 * n + 3 - v.DATA] & 31)) & 3 or rng.random() < .5:
        _, _, _, model = unit_object(v, world, rng, n)
        if rng.random() < .35:  # hand IK live (1159D0 -> 115500 at the 0x4000095 marker)
            world.track[body + 0x149 - T] |= rng.choice([0x80, 0])
            world.track[body + 0x137 - T] |= 1
            world.track[body + 0x1f6 - T] = rng.choice([0, 1, 2, 5, 0xff])
            world.track[body + 0x1f7 - T] = rng.choice([0, 3, 7, 0xff])
            world.put(model + 0xa4, world_word(v, world, model + 0xa4) | 0xff)
            world.put(0x474c04, 1)
    flags = world_word(v, world, body + 4) & ~(1 << 30)
    # BF0C0 defers a sample (+4 bit 30) only for an object with a manager
    # and a graph (BFB90 -> BF9A0 reads the graph unchecked).
    if rng.random() < .65 and struct.unpack_from('<h', world.track, body + 0x12a - T)[0] == MANAGER and \
            world_word(v, world, body + MANAGER + 0x68) != NONE:
        flags |= 1 << 30
    if rng.random() < .8:
        flags &= ~(1 << 29)
    world.put(body + 4, flags)
    if rng.random() < .8:
        world.track[body + 0xb3 - T] = 0
    if rng.random() < .6:
        world.track[body + 0x34b - T] = 0
    for k in range(16):
        world.track[SELECTED + k - T] = rng.choice([0xff, 0xff, 0, 1, 2, 5, 31, rng.randrange(256)])
    matrices = rng.choice([body + NODES, body + NODES, body + NODES, BODIES + BODY * rng.randrange(v.ROWS - 1) + NODES])
    # No matrices (BF640 then skips the rebuild and the hands): only for a
    # non-unit, whose overlay copy would read address 0.
    if not (1 << (world.data[v.RECORDS + 12 * n + 3 - v.DATA] & 31)) & 3 and rng.random() < .15:
        matrices = 0
    return [handles[n], SELECTED, matrices]


def overlay_case(v, world, rng):
    handles = deferred_scene(v, world, rng, 'overlays')
    n = rng.randrange(v.ROWS - 1)
    body, manager, mode, _ = unit_object(v, world, rng, n)
    world.put(manager + 0x68, rng.choice([world_word(v, world, manager + 0x68)] * 5 + [NONE]))
    for k in range(8):
        world.put(OVERLAY_FLAGS + 4 * k, rng.choice([0, 0, NONE, rng.getrandbits(32)]))
    sample_mask = rng.choice([0, 0, 0, 0x56eb8c])
    if sample_mask:
        for k in range(8):
            world.put(sample_mask + 4 * k, rng.choice([0, NONE, 1 << rng.randrange(8), rng.getrandbits(32)]))
    count = rng.choice([1, 2, 3, 5, 8, 8])
    orientations = rng.choice([0x4e8bc0, body + ORIENTATIONS])
    return [handles[n], manager, mode, sample_mask, count, orientations, OVERLAY_FLAGS]


def unit_hand_ik_case(v, world, rng):
    handles = deferred_scene(v, world, rng, 'hand-ik')
    n = rng.randrange(v.ROWS - 1)
    body, manager, mode, model = unit_object(v, world, rng, n)
    T = v.TRACK
    if rng.random() < .6:  # live: first person or camera-held, a hand node, the marker
        world.track[body + 0x149 - T] |= rng.choice([0x80, 0])
        world.track[body + 0x137 - T] |= 1
        world.track[body + 0x1f6 - T] = rng.choice([0, 1, 2, 5, 8, 0xff, 0x80])
        world.track[body + 0x1f7 - T] = rng.choice([0, 3, 7, 0xff])
        world.put(model + 0xa4 + 4 * rng.randrange(8), rng.getrandbits(32))
    mask = rng.choice([model + 0xa4, model + 0xa4, 0])
    count = rng.choice([1, 3, 6, 9, 9])
    return [handles[n], mask, count, body + NODES]


def hand_ik_case(v, world, rng):
    handles = deferred_scene(v, world, rng, 'hand-ik-leaf')
    put = world.put

    def matrix(at):
        put(at, rng.choice([f2u(1.0), f2u(1.0), f2u(2.0), 0, f2u(.5), rfloat(rng, special=.5, nan=.1)]))
        axes = []
        for _ in range(2):
            x = [rng.uniform(-1, 1) for _ in range(3)]
            norm = sum(c * c for c in x) ** .5 or 1.0
            axes.append([c / norm for c in x])
        for k in range(3):
            put(at + 4 + 4 * k, f2u(axes[0][k]) if rng.random() < .97 else rfloat(rng))
            put(at + 0x10 + 4 * k, f2u(rng.uniform(-1, 1)))
            put(at + 0x1c + 4 * k, f2u(axes[1][k]) if rng.random() < .97 else rfloat(rng))
            put(at + 0x28 + 4 * k, f2u(rng.uniform(-5, 5)))
    matrix(IK_MARKER)
    matrix(IK_NODE)
    target = rng.random()
    for k in range(3):
        if target < .1:  # on the node position: zero offset
            put(IK_TARGET + 4 * k, world_word(v, world, IK_NODE + 0x28 + 4 * k))
        else:
            put(IK_TARGET + 4 * k, f2u(rng.uniform(-8, 8)) if rng.random() < .95 else rfloat(rng))
    return [IK_MARKER, IK_TARGET, IK_NODE]


def euler_case(v, world, rng):
    deferred_scene(v, world, rng, 'euler')
    return [EULER_OUT, angle_value(rng), angle_value(rng), rng.choice([0, 0, angle_value(rng)])]


def node_flags_case(v, world, rng):
    deferred_scene(v, world, rng, 'node-flags')
    T = v.TRACK
    put = world.put
    for k in range(8):
        put(OVERLAY_FLAGS + 4 * k, rng.choice([0, 0, NONE, rng.getrandbits(32)]))
    for k in range(0x100):
        world.track[FLAG_DATA + k - T] = rng.choice([0, 0, 1, 0x80, rng.getrandbits(8)])
    put(FLAG_BLOCK, FLAG_DATA + rng.choice([0, 4, 0x10]))
    put(FLAG_BLOCK + 4, FLAG_HEADER)
    static = rng.choice([0, 0, 3, 6, 9, 24, 96, 0x80, 0xfd, 2])
    animated = rng.choice([0, 3, 6, 9, 24, 96, 0x81, 0xff, 1, 5])
    world.track[FLAG_HEADER - T] = static
    world.track[FLAG_HEADER + 1 - T] = animated
    put16_(world, FLAG_HEADER + 6, rng.choice([0, 0, 8, 0x20, 0xfff0]))
    put(FLAG_HEADER + 0xc, rng.choice([0, 0x10, 0x20]))
    return [FLAG_BLOCK, OVERLAY_FLAGS]


composite('nodebuild_deferred', 0xbf640, 'long', SAMPLE_STOPS, 1200, cut=SAMPLE_CUT, stack=[0, 1, 2])
composite('nodebuild_frame_overlays', 0x1112e0, 'void', SAMPLE_STOPS, 500, cut=SAMPLE_CUT, regs={'eax': 0},
          stack=[1, 2, 3, 4, 5, 6])
composite('nodebuild_aim_overlays', 0x110c10, 'void', SAMPLE_STOPS, 800, cut=SAMPLE_CUT, regs={'eax': 0},
          stack=[1, 2, 3, 4, 5, 6], native='nodebuild_aim_overlays')
composite('nodebuild_unit_hand_ik', 0x1159d0, 'void', set(), 600, regs={'eax': 0}, stack=[1, 2, 3])
composite('nodebuild_hand_ik', 0x115500, 'void', set(), 500, regs={'ecx': 0, 'ebx': 1, 'edi': 2})
composite('nodebuild_euler', 0x1447e0, 'void', set(), 400, regs={'eax': 0}, stack=[1, 2, 3])
composite('nodebuild_node_flags', 0x2130c0, 'void', set(), 300, stack=[0, 1])


GENERATORS = {'nodebuild_sample': sample_case, 'nodebuild_device_hook': device_hook_case,
              'nodebuild_deferred': deferred_case, 'nodebuild_frame_overlays': overlay_case,
              'nodebuild_aim_overlays': overlay_case, 'nodebuild_unit_hand_ik': unit_hand_ik_case,
              'nodebuild_hand_ik': hand_ik_case, 'nodebuild_euler': euler_case, 'nodebuild_node_flags': node_flags_case,
              'nodebuild_vehicle_hook': vehicle_hook_case, 'nodebuild_unit_orientation_hook': unit_hook_case, 'nodebuild_aim_channel': aim_channel_case,
              'nodebuild_fraction_channel': fraction_channel_case, 'nodebuild_bounds': bounds_case, 'nodebuild_node_matrices': node_case,
              'nodebuild_rebuild': rebuild_case, 'nodebuild_bounds_connected': bounds_connected_case,
              'nodebuild_codec_frames': codec_frames_case,
              'nodebuild_codec_second_masked': lambda v, world, rng: codec_frames_case(v, world, rng, True),
              'nodebuild_codec_events': lambda v, world, rng: codec_frames_case(v, world, rng, events=True),
              'nodebuild_codec_partial': lambda v, world, rng: codec_frames_case(v, world, rng, partial=True),
              'nodebuild_codec_setup': codec_setup_case, 'nodebuild_resource_data': resource_data_case,
              'nodebuild_codec_event_setup': event_setup_case,
              'nodebuild_codec': codec_case, 'nodebuild_vehicle_aim_limit': vehicle_aim_limit_case,
              'nodebuild_fraction_overflow': fraction_overflow_case, 'nodebuild_unit_yaw': unit_yaw_case,
              'nodebuild_physics_nodes': physics_nodes_case, 'nodebuild_crate_sphere': crate_sphere_case}


# ---- BF640 review probes (mutation survivors of the randomized cohort) ----
def hand_camera_case(v, world, rng):
    """1159D0 through the first local player's camera: +149 bit 7 clear or
    set against +148's, +137 bit 0 often clear, the camera gate [0x474C24]
    open or closed, several players, valid hands and the hand marker."""
    handles = deferred_scene(v, world, rng, 'hand-camera')
    n = rng.randrange(v.ROWS - 1)
    body, manager, mode, model = unit_object(v, world, rng, n)
    T = v.TRACK
    world.put(0x474c04, 1)
    world.put(0x474c24, rng.choice([0x10000, 0x10001, 0x1ffff]))
    for k in range(4):
        world.put(PLAYERS_ + 0xc + 4 * k, rng.choice([NONE, 0x10002 + k, 0x10002 + k]))
    first_person = rng.random() < .3
    world.track[body + 0x149 - T] = 0x80 if first_person else rng.choice([0, 0x7f])
    world.track[body + 0x148 - T] = rng.choice([0, 0x80, 0xff, 0x7f])
    world.track[body + 0x137 - T] = rng.choice([1, 1, 0, 0xfe])
    world.track[body + 0x1f6 - T] = rng.choice([0, 1, 2, 5, 0xff])
    world.track[body + 0x1f7 - T] = rng.choice([0, 3, 7, 0xff])
    world.put(MARKER_GROUPS_, HAND_MARKER)
    world.put(MARKER_GROUPS_ + 4, rng.choice([1, 2, 3]))
    world.put(mode + 0x58, rng.choice([1, 2, 4]))
    for k in range(8):
        world.put(model + 0xa4 + 4 * k, rng.choice([NONE, NONE, rng.getrandbits(32)]))
    world.put(definition_of(v, world, body) + 0xbc, world_word(v, world, definition_of(v, world, body) + 0xbc) & ~(1 << 11))
    return [handles[n], rng.choice([model + 0xa4, 0]), 9, body + NODES]


def definition_of(v, world, body):
    return DEFINITIONS + DEFINITION * (world_word(v, world, body) & 3)


def hand_nan_case(v, world, rng):
    """115500 with distinct NaN payloads in the marker axes, the node
    position and the target (y and z operands carrying different NaNs)."""
    values = hand_ik_case(v, world, rng)
    for at in [IK_MARKER + 4 + 4 * k for k in range(3)] + [IK_MARKER + 0x1c + 4 * k for k in range(3)] + \
            [IK_TARGET + 4 * k for k in range(3)] + [IK_NODE + 0x28 + 4 * k for k in range(3)]:
        if rng.random() < .3:
            world.put(at, NANS[rng.randrange(4)])
    return values


def aim_stepped_case(v, world, rng):
    """110C10's stepped overlay: a zero blend, states 1..6, the +34 handle
    in the manager's graph and long animations (frame 17)."""
    values = overlay_case(v, world, rng)
    body = BODIES + BODY * (values[0] & 0xffff)
    T = v.TRACK
    aim = body + UNIT_BLOCK
    world.put(aim + 0x40, 0)
    world.track[aim + 2 - T] = rng.choice([1, 2, 3, 3, 4, 5, 6])
    graph = world_word(v, world, values[1] + 0x68)
    inherited = world_word(v, world, GRAPHS_ + 0x400 * ((graph & 0xffff) - 26) + 0x4c) if graph != NONE else 0
    world.put(aim + 0x34, scene_handle(rng, inherited))
    for g in range(3):
        for a in range(8):
            put16_(world, GRAPH_ROWS + 0x400 * g + 0x6c * a + 0x14, rng.choice([18, 20, 30, 17]))
    return values


composite('nodebuild_hand_ik_camera', 0x1159d0, 'void', set(), 300, regs={'eax': 0}, stack=[1, 2, 3],
          native='nodebuild_unit_hand_ik')
composite('nodebuild_hand_ik_nan', 0x115500, 'void', set(), 300, regs={'ecx': 0, 'ebx': 1, 'edi': 2},
          native='nodebuild_hand_ik')
composite('nodebuild_aim_overlays_stepped', 0x110c10, 'void', SAMPLE_STOPS, 300, cut=SAMPLE_CUT, regs={'eax': 0},
          stack=[1, 2, 3, 4, 5, 6], native='nodebuild_aim_overlays')
GENERATORS.update({'nodebuild_hand_ik_camera': hand_camera_case, 'nodebuild_hand_ik_nan': hand_nan_case,
                   'nodebuild_aim_overlays_stepped': aim_stepped_case})


def deferred_frames_case(v, world, rng):
    """BF640 on a unit whose only live overlay is a frame weight (1114B0),
    often the 13th (+7A), with 110240 false."""
    values = deferred_case(v, world, rng)
    n = values[0] & 0xffff
    body = BODIES + BODY * n
    T = v.TRACK
    unit_object(v, world, rng, n)
    aim = body + UNIT_BLOCK
    struct.pack_into('<H', world.track, aim + 0x36 - T, 0xffff)
    for k in range(13):
        world.track[aim + 0x6e + k - T] = 0
    world.track[aim + 0x6e + rng.choice([12, 12, 12, rng.randrange(13)]) - T] = rng.choice([1, 128, 255])
    if values[2] == 0:
        values[2] = body + NODES
    return values


def aim_requests_case(v, world, rng):
    """110C10's request path (a blend neither above 1e-4 nor zero) with
    resource-admitted animations (126200 stops) on +30 or +34."""
    values = overlay_case(v, world, rng)
    body = BODIES + BODY * (values[0] & 0xffff)
    T = v.TRACK
    aim = body + UNIT_BLOCK
    world.put(aim + 0x40, rng.choice([f2u(5e-5), f2u(-0.3), NANS[0], f2u(1e-4), f2u(-0.0) | 1]))
    graph = world_word(v, world, values[1] + 0x68)
    if graph != NONE:
        data = GRAPHS_ + 0x400 * ((graph & 0xffff) - 26)
        for k in range(8):
            world.track[data + 0x200 + 20 * k + 0x10 - T] = rng.choice([0, 0, 1])
        for a in range(8):
            world.track[GRAPH_ROWS + 0x400 * ((graph & 0xffff) - 26) + 0x6c * a + 0x16 - T] |= rng.choice([0, 0x10])
        for offset in (0x30, 0x34):
            world.put(aim + offset, rng.randrange(8) << 16 | 0xffff)
    return values


composite('nodebuild_deferred_frames', 0xbf640, 'long', SAMPLE_STOPS, 300, cut=SAMPLE_CUT, stack=[0, 1, 2],
          native='nodebuild_deferred')
composite('nodebuild_aim_overlays_requests', 0x110c10, 'void', SAMPLE_STOPS, 300, cut=SAMPLE_CUT, regs={'eax': 0},
          stack=[1, 2, 3, 4, 5, 6], native='nodebuild_aim_overlays')
GENERATORS.update({'nodebuild_deferred_frames': deferred_frames_case, 'nodebuild_aim_overlays_requests': aim_requests_case})


# ---- 16A4DD's bound callees at their own entries: 1D5840, BF9A0, 1CF210 ----
# The first-person animation sampler 16A4DD (169C46's per-tick update and
# the 168DED draw) reaches them at 16A6E5, 16A6FD and 16A796..16AC26.
def entry_mask(world, rng):
    mask = rng.choice([0, 0, 0x56eb8c])
    if mask:
        for k in range(8):
            world.put(mask + 4 * k, rng.choice([0, NONE, 0xffff, 1 << rng.randrange(8), rng.getrandbits(32)]))
    return mask


def channels_case(v, world, rng):
    handles = scene(v, world, rng, 'channels')
    world.extra[0x47c1b4] = bytearray([rng.choice([1, 1, 1, 0]), rng.choice([1, 1, 1, 0]), 0, 0])
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    valid_manager(v, world, rng, manager)  # 16A4DD tests B87B0 first
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(.5), 0, rfloat(rng, special=.5, nan=.1)])
    count = rng.choice([0, 1, 2, 3, 5, 8, 8])
    return [manager, handles[n], weight, entry_mask(world, rng), body + ORIENTATIONS, 0, 0, count]


def valid_manager(v, world, rng, manager):
    """B87B0: a graph (+68), channel 0's graph tag and animation (+6)."""
    graph = rng.choice(GRAPH_TAGS)
    world.put(manager + 0x68, graph)
    channel_record(v, world, rng, manager, 1)
    world.put(manager, graph)
    world.put(manager + 4, rng.randrange(8) << 16 | 0xffff)


def function_overlays_case(v, world, rng):
    handles = scene(v, world, rng, 'function-overlays')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    struct.pack_into('<h', world.track, body + 0x12a - v.TRACK, MANAGER)
    manager = animate(v, world, rng, body)
    manager_keys(world, rng, manager)
    valid_manager(v, world, rng, manager)
    count = rng.choice([0, 1, 2, 3, 5, 8, 8])
    return [handles[n], rng.getrandbits(32), manager, entry_mask(world, rng), count, body + ORIENTATIONS]


def channel_entry_case(v, world, rng):
    handles = scene(v, world, rng, 'channel-entry')
    n = rng.randrange(v.ROWS - 1)
    body = BODIES + BODY * n
    channel = body + rng.choice([0x18c, 0x1ac])
    channel_record(v, world, rng, channel, 1)  # 16A4DD tests B8760 first
    world.put(channel + 4, rng.randrange(8) << 16 | (world_word(v, world, channel + 4) & 0xffff))
    weight = rng.choice([f2u(1.0), f2u(1.0), f2u(.5), 0, rfloat(rng, special=.5, nan=.1)])
    count = rng.choice([0, 1, 2, 3, 5, 8, 8])
    return [channel, weight, entry_mask(world, rng), count, body + ORIENTATIONS]


composite('nodebuild_sample_channels', 0x1d5840, 'void', SAMPLE_STOPS, 600, cut=SAMPLE_CUT, regs={'esi': 7},
          stack=[0, 1, 2, 3, 4, 5, 6])
composite('nodebuild_function_overlays', 0xbf9a0, 'void', SAMPLE_STOPS, 600, cut=SAMPLE_CUT, stack=[0, 1, 2, 3, 4, 5])
composite('nodebuild_sample_channel', 0x1cf210, 'void', SAMPLE_STOPS, 500, cut=SAMPLE_CUT, regs={'ebx': 0},
          stack=[1, 2, 3, 4])
GENERATORS.update({'nodebuild_sample_channels': channels_case, 'nodebuild_function_overlays': function_overlays_case,
                   'nodebuild_sample_channel': channel_entry_case})


# ---- 16A4DD: the first-person animation sampler (with 16AD45, 169863) ----
# The view, its slot and orientation bank live in the AI bank (the scene does
# not use it below USER_BLOCK at +6000): [0x4F6498] and [0x4F6494] are set so
# that the case's user and slot land there. The first-person and character
# models are the scene's render models (23..25, +48 count, +4C records of
# 0x60: +0 name, +C translation, +18 orientation); the slot's manager runs on
# a scene graph whose node table (+C count, +10 rows) is replaced by a named
# tree rooted at node 0 (+4 sibling, +6 child, +8 parent, +A flags).
FP_VIEW, FP_BANK, FP_NODES = 0x1300000, 0x1302400, 0x1303400
FP_ENTRIES, FP_MAGAZINES, CHAR_ROWS = 0x1303800, 0x1303840, 0x1303c00
EFFECT_ARRAY, EFFECT_BITMAP, EFFECT_DATA, TEAM_WORD = 0x1304000, 0x1304080, 0x1304100, 0x1304800
FP_REMAPS, FP_LEAF_STATES, FP_OVERLAYS = 0x1304900, 0x1304a00, 0x1304c00
FP_OVERLAY_NAMES = [0x8000061, 0xa000069, 0xe0000de, 0x12000068]
FP_NODE_ROWS = 24
# Residual owners outside the factory (see the header): 17A570 stops where an
# effect follows the weapon, 171440 where the prediction block is not empty,
# 128E70 at its entry. Their other callees are cut.
FP_RESIDUAL_STOPS = {0x17a5fd, 0x17a63f, 0x171459, 0x128e70}
FP_RESIDUAL_CUT = {0x17b1f0, 0x3bca0, 0x12f720, 0x1718c0, 0x1717b0, 0x1307c0, 0x1715d0, 0x1716f0, 0x171750,
                   0x171610, 0x171680}
BOUNDARY_OWNERS.update({0x17a5fd: '17A570', 0x17a63f: '17A570', 0x171459: '171440'})
# 1D5320 (1D3380, 169863): the weapon track's refused physical-query paths.
FP_STATE_STOPS = {0x1e4c39, 0x1d55aa}
FP_STOPS = SAMPLE_STOPS | FP_RESIDUAL_STOPS | FP_STATE_STOPS
FP_CUT = SAMPLE_CUT | FP_RESIDUAL_CUT
# Entries a first-person kernel executes that other kernels stop before.
FP_THROUGH = {0x1d3380}


def fp_bank(v, world, address):
    if v.AI <= address < v.AI + len(world.ai):
        return world.ai, address - v.AI
    return world_bank(v, world, address)


def fp_put(v, world, address, value, size=4):
    bank, at = fp_bank(v, world, address)
    bank[at:at + size] = (value & ((1 << 8 * size) - 1)).to_bytes(size, 'little')


def fp_word(v, world, address, size=4):
    bank, at = fp_bank(v, world, address)
    return int.from_bytes(bank[at:at + size], 'little')


def fp_channel(v, world, rng, channel, presence):
    """channel_record on any bank: graph tag, packed id, resource gate +10,
    controls +12, frame +18, codec state +1C."""
    g = rng.randrange(3)
    present = rng.random() < presence
    graph = GRAPH_TAGS[g] if present else rng.choice([NONE, GRAPH_TAGS[g]])
    fp_put(v, world, channel, graph)
    inherited = world_word(v, world, GRAPHS_ + 0x400 * g + 0x4c)
    high = rng.choice([rng.randrange(8), rng.randrange(8), 0xffff]) if present else \
        rng.choice([0xffff, 0xffff if graph == NONE else 3])
    low = rng.choice([0xffff, 0xffff] + list(range(inherited)))
    fp_put(v, world, channel + 4, high << 16 | low)
    fp_put(v, world, channel + 8, rng.choice([NONE, 0x7000101, rng.getrandbits(32)]))
    fp_put(v, world, channel + 0x10, rng.choice([0, 0, 1]) if high != 0xffff else 0, 1)
    fp_put(v, world, channel + 0x12, rng.choice([0, 1, 0x40, 0x41, 0x400, 0x800, 0xc01, 0xffff]), 2)
    fp_put(v, world, channel + 0x18, rng.choice([0, f2u(1.0), f2u(0.5), f2u(7.0), NANS[0]]))
    fp_put(v, world, channel + 0x1c, rng.getrandbits(32))


def fp_manager(v, world, rng, manager, graph):
    """The slot's animation manager (0x84 bytes) on `graph`."""
    for c in range(3):
        fp_channel(v, world, rng, manager + 0x20 * c, .7 if c == 0 else .2)
    if rng.random() < .85:  # channel 2 mostly unset (1D5840 refuses its 1D63C0 post-process)
        fp_put(v, world, manager + 0x40, NONE)
    if rng.random() < .8:  # B87B0: a graph, channel 0's graph tag and animation
        fp_put(v, world, manager, graph)
        fp_put(v, world, manager + 4, rng.randrange(8) << 16 | 0xffff)
    for k, at in enumerate((0x64, 0x65, 0x66, 0x67)):
        fp_put(v, world, manager + at, rng.choice([[rng.randrange(256)], [0, 0, 1, 4, 30], [0, 1, 2, 3, 4, 5],
                                                   [0, 0, 2, 0xfd]][k]), 1)
    fp_put(v, world, manager + 0x68, graph)
    for k, offset in enumerate((0x70, 0x74, 0x78)):
        fp_put(v, world, manager + offset, rng.choice(MODE_KEYS[k] + [MODE_KEYS[k][0], NONE if k else 0x7000101]))
    fp_put(v, world, manager + 0x7c, rng.choice(FP_STATE_KEYS))
    fp_put(v, world, manager + 0x80, rng.choice([f2u(rng.uniform(0, 1)), f2u(0.0), f2u(0.99995), f2u(1.0), NANS[0]]))


FP_STATE_KEYS = [0x5000024, 0x6000006, 0x8000063, 0x9000062, 0x1000006e, 0x400000c, 0x600005f, 0xb0005b1, 0x900006a,
                 0x900006b, 0x500000a, 0x6000005, 0x6000006, 0x6000007, 0x6000008, 0xa000066, 0xb000065, 0xc000064,
                 0xc000073, 0xc0005ae, 0xe000606, 0xe000607, 0xe00060a, 0xe00060b, 0xc000075, 0xc000077, 0x7000101,
                 0x30000d9, 0x1000001, 0x2000002, NONE, 0x80000001]


def fp_nodes(v, world, rng, count):
    """A named node tree rooted at node 0 (parent below child), sibling
    chains in increasing order; the +A flags drive 16AD45's mirror search."""
    parent = [-1] + [rng.randrange(n) for n in range(1, FP_NODE_ROWS)]
    names = [0x6100000 + rng.getrandbits(12) * 16 + n for n in range(FP_NODE_ROWS)]
    for n in range(FP_NODE_ROWS):
        row = FP_NODES + 0x20 * n
        for k in range(8):
            fp_put(v, world, row + 4 * k, rng.getrandbits(32))
        kids = [c for c in range(count) if parent[c] == n] if n < count else []
        sibs = [c for c in range(count) if c > n and parent[c] == parent[n] and parent[n] >= 0] if n < count else []
        fp_put(v, world, row, names[n])
        fp_put(v, world, row + 4, sibs[0] if sibs else 0xffff, 2)
        fp_put(v, world, row + 6, kids[0] if kids else 0xffff, 2)
        fp_put(v, world, row + 8, parent[n] & 0xffff, 2)
        fp_put(v, world, row + 0xa, rng.choice([0, 0, 8, 0x10, 0x18, 0x20, 0x28, 0x30]), 1)
    return names


def fp_scene(v, world, rng, purpose, rows_optional=True):
    """The deferred scene plus a first-person view (user, slot) over two
    scene objects (the unit and its weapon) and the residual owners' state.
    Returns (user, slot, view, slot record, unit, weapon)."""
    handles = deferred_scene(v, world, rng, purpose)
    world.extra[0x47c1b8] = bytearray(original_bytes(v, 0x47c1b8, 0x20))
    user, slot = rng.randrange(4), rng.choice([0, 0, 1])
    world.globals[0x4f6498] = (FP_VIEW - user * 0x20cc) & NONE
    world.globals[0x4f6494] = (FP_BANK - (slot + 2 * user) * 0x1000) & NONE
    view = FP_VIEW
    s = view + 0xc + 0x1010 * slot
    units = rng.sample(range(v.ROWS - 1), 2)
    unit, weapon = handles[units[0]], handles[units[1]]
    ubody, wbody = BODIES + BODY * units[0], BODIES + BODY * units[1]
    k = world_word(v, world, wbody) & 3
    definition = DEFINITIONS + DEFINITION * k
    # The manager's graph: one of the scene graphs with the named node tree.
    g = rng.randrange(3)
    graph_data = GRAPHS_ + 0x400 * g
    count = rng.choice([1, 2, 3, 5, 9, 12, 16, 16, 20, 24])
    world.put(graph_data + 0xc, count)
    world.put(graph_data + 0x10, FP_NODES)
    names = fp_nodes(v, world, rng, count)
    # The mode leaves' state animations (leaf +4: {count, 8-byte rows of
    # key, handle} sorted by key), so 1D5320 (1D3380, 169863) can enter a
    # state; the scene's leaves have none.
    for r in range(3):
        leaf = GRAPHS_ + 0x400 * r + 0x2c8
        keys = sorted(rng.sample([k for k in FP_STATE_KEYS if k < 0x80000000], rng.choice([0, 3, 6, 10])))
        rows = FP_LEAF_STATES + 0x60 * r
        world.put(leaf + 4, len(keys))
        world.put(leaf + 8, rows)
        for n, key in enumerate(keys):
            fp_put(v, world, rows + 8 * n, key)
            fp_put(v, world, rows + 8 * n + 4, rng.randrange(8) << 16 | 0xffff if rng.random() < .9 else NONE)
    # The mode leaves' overlay tables (leaf +C/+10: 8-byte {name, handle}
    # rows sorted by name) name 16AD45's overlays too (1D37D0 0x12000068,
    # 1D3C80 0x8000061, 0xA000069, 0xE0000DE), each usually present.
    for r in range(3):
        leaf = GRAPHS_ + 0x400 * r + 0x2c8
        inherited = world_word(v, world, GRAPHS_ + 0x400 * r + 0x4c)
        overlay_names = [n for n in FP_OVERLAY_NAMES if rng.random() < .8] + rng.sample(OVERLAY_NAMES, rng.randrange(3))
        overlay_names = sorted(set(overlay_names))
        rows = FP_OVERLAYS + 0x80 * r
        world.put(leaf + 0xc, len(overlay_names))
        world.put(leaf + 0x10, rows)
        for n, name in enumerate(overlay_names):
            fp_put(v, world, rows + 8 * n, name)
            fp_put(v, world, rows + 8 * n + 4, scene_handle(rng, inherited) if rng.random() < .9 else NONE)
    # 1D5320's key remaps (graph +54: {count, 8-byte rows sorted by key}).
    for r in range(3):
        remaps = FP_REMAPS + 0x40 * r
        keys = sorted(rng.sample([0x7000101, 0x7000001, 0x30000d9, 0x400054b, 0x2000002, 0x6100005], 3))
        world.put(GRAPHS_ + 0x400 * r + 0x54, rng.choice([0, 0, 1, 3]))
        world.put(GRAPHS_ + 0x400 * r + 0x58, remaps)
        for n, key in enumerate(keys):
            fp_put(v, world, remaps + 8 * n, key)
            fp_put(v, world, remaps + 8 * n + 4, rng.choice(MODE_KEYS[1] + [0x7000001]))
    # Render models as first-person and character models: named records.
    models = []
    for r in range(3):
        mode = MODES + 0x80 * r
        nodes = rng.choice([0, 1, 2, 3, 5, 8, 8, 8])
        world.put(mode + 0x48, nodes)
        records = world_word(v, world, mode + 0x4c)
        for n in range(8):
            world.put(records + 0x60 * n, names[rng.randrange(count)] if rng.random() < .92 else rng.getrandbits(32))
        models.append(((0x5300 + r) << 16 | (23 + r), nodes, records))
    # Character variants ([[0x4ECC1C]+13C] rows of 0xBC, +4 model).
    globals_ = world.globals[0x4ecc1c]
    world.put(globals_ + 0x138, rng.choice([1, 2, 3, 4, 4, 0]))
    world.put(globals_ + 0x13c, CHAR_ROWS)
    for c in range(4):
        fp_put(v, world, CHAR_ROWS + 0xbc * c + 4, rng.choice([models[r][0] for r in range(3)] * 4 + [NONE]))
    variant = rng.choice([0, 0, 1, 2, 3, 3, 4, 5, 0xffffffff] if rows_optional else [0, 0, 1, 2, 3, 3])
    body_row = CHAR_ROWS + 0xbc * (variant if variant < 4 else 0)
    if variant >= 4 and rng.random() < .7:
        variant = 0
    if not rows_optional:  # 16A4DD reads the variant's character and first-person models unchecked
        world.put(globals_ + 0x138, rng.choice([variant + 1, 4]))
        fp_put(v, world, body_row + 4, rng.choice([models[r][0] for r in range(3)]))
    # The weapon definition: first-person rows (+2A8/+2AC), +292 kind,
    # magazines (+2C0/+2C4), +308 offset, +12C flags and the prediction
    # block (+2B8, usually empty).
    # 16A4DD reads the first-person row unchecked (11E060 returns 0 without
    # rows); 16AD45 and 169863 test it.
    # Every definition carries them: the unit's other weapons (16AD45's
    # CE230) may be any scene object.
    for e in range(2):
        fp_put(v, world, FP_ENTRIES + 0x10 * e + 4, rng.choice([models[r][0] for r in range(3)] * 6 +
                                                                ([NONE] if rows_optional else [])))
        fp_put(v, world, FP_ENTRIES + 0x10 * e + 0xc, rng.choice([GRAPH_TAGS[g]] * 8 + [NONE]))
    for m in range(2):
        fp_put(v, world, FP_MAGAZINES + 0x5c * m + 0xa, rng.choice([12, 30, 60, 0, 0x7fff, 0x8000]), 2)
        fp_put(v, world, FP_MAGAZINES + 0x5c * m + 0xc, rng.choice([4, 36, 120, 0, 0xffff]), 2)
    for d in range(4):
        other = DEFINITIONS + DEFINITION * d
        world.put(other + 0x2a8, rng.choice([1, 2, 2, 2, 0, 3] if rows_optional or other == definition else [1, 2]))
        world.put(other + 0x2ac, FP_ENTRIES)
        fp_put(v, world, other + 0x292, rng.choice([2, 2, 0, 1]), 2)
        world.put(other + 0x2b8, rng.choice([0, 0, 0, 0, 1, 3]))
        world.put(other + 0x2bc, rng.getrandbits(32))
        world.put(other + 0x2c0, rng.choice([1, 2, 2, 0]))
        world.put(other + 0x2c4, FP_MAGAZINES)
        for o in range(3):
            world.put(other + 0x308 + 4 * o, rfloat(rng, special=.2, nan=.05, span=2))
        world.put(other + 0x12c, rng.getrandbits(32))
    if not rows_optional:
        world.put(definition + 0x2a8, rng.choice([1, 2, 2, 2, 3]))
    entries = world_word(v, world, definition + 0x2a8)
    # The unit (+212/+213 slot indices, +218..+224 weapons) and the weapon
    # (+188 rate, +226..+22C rounds, +154 owner, +12C item flags).
    put8 = lambda address, value: fp_put(v, world, address, value, 1)
    put16 = lambda address, value: fp_put(v, world, address, value, 2)
    put8(ubody + 0x212, rng.choice([0, 1, 2, 3, 0xff]))
    put8(ubody + 0x213, rng.choice([0, 1, 0xff, 0xff]))
    for w in range(4):
        world.put(ubody + 0x218 + 4 * w, rng.choice([weapon, weapon, NONE, handles[rng.randrange(v.ROWS - 1)]]))
    world.put(wbody + 0x188, rfloat(rng, special=.3, nan=.05, span=2))
    for o, choices in ((0x226, [0, 5, 100]), (0x228, [0, 0x2c, 0x30, 0x50, 0x200, 0x8000]), (0x22a, [0, 7, 30]),
                       (0x22c, [0, 3, 12, 0xffff, 0x7fff]), (0x23a, [0, 9]), (0x23c, [1, 4])):
        put16(wbody + o, rng.choice(choices))
    put8(wbody + 0x12c, rng.choice([0, 1, 1, 3]))
    world.put(wbody + 0x154, rng.choice([unit, unit, NONE]))
    # Shared ammunition (10B0F0/15C7F0): game mode, engine variant, team word.
    world.put(v.PVS + 8, rng.choice([2, 2, 1]))
    world.put(v.PVS + 0x180, rng.choice([7, 7, 0]))
    fp_put(v, world, v.PVS + 0x22c, rng.choice([0x20, 0, 0xff]), 1)
    # The engine variant table 0x56DFA0 is the base world's absolute block
    # (8 entries); the engine index (+C14) stays inside it.
    world.absolute[0x56dfa0][:] = struct.pack('<8I', *[rng.choice([0, 1, 1]) for _ in range(8)])
    fp_put(v, world, v.USER_BLOCK + 0xc14, rng.randrange(8))
    world.globals[0x51df9c] = TEAM_WORD
    fp_put(v, world, TEAM_WORD, rng.getrandbits(16), 2)
    put = world.put
    # The effect datums (17A570): usually none follow the weapon.
    world.put(0x4f720c, EFFECT_ARRAY)  # inside the deferred scene's camera block (0x4F655C)
    effects = rng.choice([0, 1, 2, 4])
    fp_put(v, world, EFFECT_ARRAY + 0x24, 0x190)
    fp_put(v, world, EFFECT_ARRAY + 0x38, effects)
    fp_put(v, world, EFFECT_ARRAY + 0x44, EFFECT_DATA)
    fp_put(v, world, EFFECT_ARRAY + 0x48, EFFECT_BITMAP)
    fp_put(v, world, EFFECT_BITMAP, rng.getrandbits(4))
    for e in range(4):
        datum = EFFECT_DATA + 0x190 * e
        fp_put(v, world, datum, rng.getrandbits(16), 2)
        # +2 bit 7 marks an effect that follows the weapon (bit 6 alone does not).
        fp_put(v, world, datum + 2, rng.choice([0x80, 0x40, 0xc0, 0, rng.getrandbits(8)]), 1)
        fp_put(v, world, datum + 0x48, weapon if rng.random() < .25 else rng.choice([NONE, unit]))
        fp_put(v, world, datum + 0x58, user if rng.random() < .15 else rng.choice([NONE, 2]))
    # The view: flags, unit, variant, fade, movement and aim values, hand.
    fp_put(v, world, view, rng.getrandbits(32))
    fp_put(v, world, view + 4, unit if rng.random() < .93 else NONE)
    fp_put(v, world, view + 8, variant)
    fp_put(v, world, view + 0x202c, rng.randrange(40), 1)
    fp_put(v, world, view + 0x202d, rng.choice([0, 1, 5, 10, 30]), 1)
    fp_put(v, world, view + 0x202e, rng.choice([0, 1, 2, 3, 4, 5]), 1)
    fp_put(v, world, view + 0x202f, rng.choice([0, 1, 0x80]), 1)
    # The aim channel's yaw (+2040) and pitch (+2054) stay finite: a NaN
    # pitch row reaches the CRT floor's error path (see SAMPLE_CUT).
    for o in (0x2030, 0x2034, 0x2040, 0x2044, 0x2054):
        aimed = o in (0x2040, 0x2054)
        fp_put(v, world, view + o, rng.choice([0, 0, f2u(rng.uniform(-1, 1)), f2u(rng.uniform(-1, 1)), f2u(-0.0),
                                                f2u(1e-40), f2u(rng.uniform(-3, 3))] +
                                               ([] if aimed else [rfloat(rng, special=.4, nan=.08, span=2), 0x7f800001])))
    fp_put(v, world, view + 0x2094, rng.choice([NONE, NONE, 0, 1, rng.randrange(max(count, 1))]))
    for o in range(13):
        fp_put(v, world, view + 0x2098 + 4 * o, rfloat(rng, special=.2, nan=.03, span=4))
    # The slot.
    fp_put(v, world, s, rng.getrandbits(32))
    fp_put(v, world, s + 4, weapon if rng.random() < .95 else NONE)
    manager = s + 8
    # 16A4DD reads the manager's graph unchecked (1D3FE0), and 169863's
    # 1D5320 too: a slot holds a weapon only once 16AD45 set its manager up.
    holding = fp_word(v, world, s + 4) != NONE
    fp_manager(v, world, rng, manager, GRAPH_TAGS[g] if rng.random() < .95 or holding or not rows_optional else NONE)
    fp_channel(v, world, rng, s + 0x98, .5)
    fp_channel(v, world, rng, s + 0xb8, .5)
    for o in (0xd8, 0xdc, 0xe0):
        fp_put(v, world, s + o, rng.choice([NONE, rng.randrange(8) << 16 | 0xffff, rng.randrange(8) << 16 | 0xffff]))
    fp_put(v, world, s + 0xe4, rng.choice([0, 0, f2u(rng.uniform(0, 1)), f2u(-0.5), NANS[0]]))
    fp_put(v, world, s + 0xe8, rfloat(rng))
    fp_put(v, world, s + 0xec, rng.getrandbits(16), 2)
    fp_put(v, world, s + 0xf0, rng.getrandbits(16), 2)
    # Node maps and their counts: matching the models (no rebuild) mostly.
    entry = FP_ENTRIES + 0x10 * (1 if entries > 1 and variant in (1, 3) else 0)
    fp_tag = fp_word(v, world, entry + 4)
    char_tag = fp_word(v, world, body_row + 4)
    nodes_of = {m[0]: m[1] for m in models}
    match = rng.random() < .75
    fp_put(v, world, s + 0xf4, nodes_of.get(fp_tag, 0) if match else rng.choice([0, 1, 9, NONE]))
    fp_put(v, world, s + 0xf8, nodes_of.get(char_tag, 0) if match else rng.choice([0, 3, NONE]))
    for n in range(64):
        for base in (0xfc, 0x1fc):
            fp_put(v, world, s + base + 4 * n, rng.choice([NONE, rng.randrange(max(count, 1)), rng.randrange(max(count, 1)),
                                                           0x10000 | rng.randrange(max(count, 1))]))
    mirrored = rng.random() < .3 and slot == 1
    fp_put(v, world, s + 0x2fc, rng.randrange(count) if mirrored else rng.choice([0xffff, 0xffff, 0]), 2)
    fp_put(v, world, s + 0x2fe, rng.randrange(count) if mirrored else rng.choice([0xffff, 0xffff, 1]), 2)
    # The node count (169B0A copies 32 bytes per node to the previous bank).
    fp_put(v, world, s + 0x300, rng.choice([0, 1, 5, count, count, 64]))
    fp_put(v, world, s + 0x1008, rng.choice([NONE, NONE, 0x10002]))  # a sound reaches 169863's 128E70 stop
    fp_put(v, world, s + 0x100c, rng.getrandbits(16), 2)
    # The movement overlay (+DC) samples only animations of 9 frames or more.
    for r in range(3):
        for a in range(8):
            if rng.random() < .4:
                put16_(world, GRAPH_ROWS + 0x400 * r + 0x6c * a + 0x14, rng.choice([8, 9, 9, 12, 30]))
    # The bank: random orientations (previous at +800).
    for n in range(0x1000 // 4):
        fp_put(v, world, FP_BANK + 4 * n, rfloat(rng, special=.1, nan=.01, span=2))
    return user, slot, view, s, unit, weapon


def first_person_case(v, world, rng):
    user, slot, *_ = fp_scene(v, world, rng, 'first-person', rows_optional=False)
    return [user, slot]


def first_person_rebuild_case(v, world, rng):
    user, slot, view, s, unit, weapon = fp_scene(v, world, rng, 'first-person-rebuild')
    return [user, slot]


def first_person_state_ties_case(v, world, rng):
    """169863 -> 169B0A where the fade seconds equal the manager fade's
    remaining time exactly (1E16E0: 8 or 4 ticks of a tick length that
    divides 0.267 or 0.1335 exactly)."""
    key, user, slot, stop = first_person_state_case(v, world, rng)
    s = FP_VIEW + 0xc + 0x1010 * slot
    seconds, ticks = rng.choice([(0x3e88b439, 8), (0x3e08b439, 4), (0x3e88b439, 4)])
    tick = struct.unpack('<f', struct.pack('<I', seconds))[0] / ticks
    struct.pack_into('<f', world.data2, v.TIME + 4 - v.DATA2, tick)
    fade = s + 8 + 0x64
    active = rng.random() < .5
    fp_put(v, world, fade, (2 * ticks - ticks) if active else ticks, 1)
    fp_put(v, world, fade + 1, 2 * ticks, 1)
    fp_put(v, world, fade + 3, 1 if active else 0, 1)
    fp_put(v, world, FP_VIEW, fp_word(v, world, FP_VIEW) | 2)
    key = rng.choice([0x6000005, 0x9000069, 0x8000063, 0xc000064, 0xb000065, 0x500000a, 0xe000607])
    return [key, user, slot, stop]


def first_person_state_case(v, world, rng):
    user, slot, view, s, unit, weapon = fp_scene(v, world, rng, 'first-person-state')
    key = fp_word(v, world, s + 0x84) if rng.random() < .3 else rng.choice(FP_STATE_KEYS)
    return [key, user, slot, rng.choice([0, 1, 1, 0x100])]


def first_person_ammunition_case(v, world, rng):
    """16A4DD's ammunition overlay (+E0) on a magazine weapon (+292 == 2) in
    a reload state (0xC000064/0xB000065) past the 0x2C-tick threshold: the
    rounds (105C10 -> 1059C0) of the weapon and of its owner's other
    weapons of the same tag, with or without shared ammunition (10B0F0 ->
    15C7F0)."""
    user, slot, view, s, unit, weapon = fp_scene(v, world, rng, 'first-person-ammunition', rows_optional=False)
    n = weapon & 0xffff
    wbody = BODIES + BODY * n
    k = world_word(v, world, wbody) & 3
    world.put(DEFINITIONS + DEFINITION * k + 0x292, (world_word(v, world, DEFINITIONS + DEFINITION * k + 0x292) & ~0xffff) |
              rng.choice([2, 2, 2, 1]))
    fp_put(v, world, s + 8 + 0x7c, rng.choice([0xc000064, 0xb000065, 0xc000064, 0x5000024]))
    fp_put(v, world, s + 0xe0, rng.randrange(8) << 16 | 0xffff)
    fp_put(v, world, wbody + 0x226, rng.choice([0, 3]), 2)
    fp_put(v, world, wbody + 0x228, rng.choice([0x2c, 0x2d, 0x40, 0x100, 0x7fff, 0x2b]), 2)
    fp_put(v, world, wbody + 0x22c, rng.choice([0, 3, 12, 40, 0xffff]), 2)
    fp_put(v, world, wbody + 0x22a, rng.choice([0, 5, 60, 0x7fff]), 2)
    fp_put(v, world, wbody + 0x12c, rng.choice([1, 1, 0, 3]), 1)
    world.put(wbody + 0x154, unit if rng.random() < .85 else NONE)
    ubody = BODIES + BODY * (unit & 0xffff)
    for w in range(4):
        other = rng.choice(world.handles[:v.ROWS - 1] + [weapon, NONE])
        world.put(ubody + 0x218 + 4 * w, other)
        if other != NONE:
            obody = BODIES + BODY * (other & 0xffff)
            if rng.random() < .6:  # the same tag as the weapon
                world.put(obody, world_word(v, world, wbody))
            fp_put(v, world, obody + 0x22a, rng.choice([0, 7, 30, 0x7fff]), 2)
    world.put(ubody + 0x13c, rng.choice([0x10003, 0x20005, NONE]))
    world.put(v.PVS + 8, rng.choice([2, 2, 2, 1]))
    world.put(v.PVS + 0x180, rng.choice([7, 7, 7, 3]))
    fp_put(v, world, v.PVS + 0x22c, rng.choice([0x20, 0x20, 0]), 1)
    fp_put(v, world, TEAM_WORD, rng.choice([0xffff, 0xffff, 0x8, 0x20, 0]), 2)
    if rng.random() < .3:  # shared ammunition across all four of the owner's weapons
        world.put(ubody + 0x13c, 0x10003)
        world.absolute[0x56dfa0][:] = struct.pack('<8I', *([1] * 8))
        world.put(v.PVS + 8, 2)
        world.put(v.PVS + 0x180, 7)
        fp_put(v, world, v.PVS + 0x22c, 0x20, 1)
        fp_put(v, world, TEAM_WORD, rng.choice([0xffff, 0xffff, 0x8, 0xfff7, 0]), 2)  # the player's bit 3
        fp_put(v, world, wbody + 0x12c, 1, 1)
        world.put(DEFINITIONS + DEFINITION * k + 0x2c0, rng.choice([1, 2]))
    return [user, slot]


composite('nodebuild_first_person', 0x16a4dd, 'void', FP_STOPS, 600, cut=FP_CUT, stack=[0, 1])
composite('nodebuild_first_person_ammunition', 0x16a4dd, 'void', FP_STOPS, 600, cut=FP_CUT, stack=[0, 1],
          native='nodebuild_first_person')
composite('nodebuild_first_person_rebuild', 0x16ad45, 'void', FP_STOPS, 500, cut=FP_CUT, stack=[0, 1])
composite('nodebuild_first_person_state', 0x169863, 'void', FP_STOPS, 500, cut=FP_CUT, regs={'eax': 0},
          stack=[1, 2, 3])
composite('nodebuild_first_person_state_ties', 0x169863, 'void', FP_STOPS, 300, cut=FP_CUT, regs={'eax': 0},
          stack=[1, 2, 3], native='nodebuild_first_person_state')
for _name in ('nodebuild_first_person', 'nodebuild_first_person_rebuild', 'nodebuild_first_person_state',
              'nodebuild_first_person_ammunition', 'nodebuild_first_person_state_ties'):
    COMPOSITE[_name]['through'] = FP_THROUGH
GENERATORS.update({'nodebuild_first_person': first_person_case, 'nodebuild_first_person_rebuild': first_person_rebuild_case,
                   'nodebuild_first_person_ammunition': first_person_ammunition_case,
                   'nodebuild_first_person_state_ties': first_person_state_ties_case,
                   'nodebuild_first_person_state': first_person_state_case})



# 169C46, the first-person update (168D55, per local user per tick). The
# inputs ([0x4F9B54] rows of 0x94), the idle globals ([[0x4ECC1C]+134]), the
# graphs' sound (+18) and effect (+20) reference rows and the animations'
# event tables (+4C/+50 frame, +54/+58 sound, +5C/+60 effect) live in the
# AI bank's free tail. 1D39C0's events call 169C21/169C33 (169BA2), whose
# sound event stops at 18BE50 (a residual owner).
FP_INPUTS, FP_IDLE, FP_ZERO_VECTOR = 0x1305000, 0x1305100, 0x1305180
FP_SOUND_REFS, FP_EFFECT_REFS, FP_EVENT_ROWS = 0x1305200, 0x1305400, 0x1305600
FP_UPDATE_STOPS = FP_STOPS | {0x18be50}
INDIRECT_TARGETS[0x1cf700] = [0x169c21, 0x169c33]
FP_UPDATE_KEYS = [0x400000c, 0x400000c, 0x600005f, 0xb000067, 0xb000067, 0x8000025, 0xc000064, 0x8000071, 0x6000006,
                  0x6000007, 0x500000a, 0x5000024, 0x8000063, 0x9000062, 0x900006a, 0x900006b, 0xa000066, 0xa0005bb,
                  0xb000065, 0xb00006d, 0xb0005b1, 0xb0005b2, 0x1000006e, 0x1000006c, 0xc0005ae, 0xc000073, 0xc000075,
                  0xc000077, 0xd000021, 0xe000607, 0xe00060a, 0x11000074, 0x11000076, 0x130005bc, 0x140005b0,
                  0x140005b3, 0x150005af, 0x9000069, 0xe000606, NONE, 0x80000001]


def fp_update_events(v, world, rng):
    """Event tables of every scene animation and the graphs' reference rows."""
    for g in range(3):
        data = GRAPHS_ + 0x400 * g
        world.put(data + 0x18, FP_SOUND_REFS + 0x60 * g)
        world.put(data + 0x20, FP_EFFECT_REFS + 0x60 * g)
        for k in range(8):
            for refs in (FP_SOUND_REFS, FP_EFFECT_REFS):
                row = refs + 0x60 * g + 12 * k
                fp_put(v, world, row, rng.getrandbits(32))
                fp_put(v, world, row + 4, rng.choice([(0x5800 + k) << 16 | (40 + k), NONE]) if rng.random() < .9 else 0x10002)
                fp_put(v, world, row + 8, rng.choice([0, 0, 0, 2, 4, 8, 8, 8, 0x10, 0x20, 0x30, 6, 0x18, 0x28]), 2)
        for a in range(8):
            animation = GRAPH_ROWS + 0x400 * g + 0x6c * a
            rows = FP_EVENT_ROWS + 0x80 * (8 * g + a)
            frames = struct.unpack_from('<H', world.track, animation + 0x14 - v.TRACK)[0]
            counts = [rng.choice([0, 0, 1, 2, 3]) for _ in range(3)]
            world.put(animation + 0x4c, counts[0])
            world.put(animation + 0x50, rows)
            world.put(animation + 0x54, counts[1])
            world.put(animation + 0x58, rows + 0x20)
            world.put(animation + 0x5c, counts[2])
            world.put(animation + 0x60, rows + 0x60)
            pick = lambda: rng.choice([0, 1, frames // 2, max(frames - 1, 0), frames, rng.randrange(max(frames, 1) + 2), 0xffff])
            for n in range(4):
                fp_put(v, world, rows + 4 * n, rng.randrange(40) | pick() << 16)
                fp_put(v, world, rows + 0x20 + 8 * n, rng.choice([0, 1, 2, 3, 7, 0xffff]) | pick() << 16)
                fp_put(v, world, rows + 0x24 + 8 * n, rng.getrandbits(32))
                fp_put(v, world, rows + 0x60 + 4 * n, rng.choice([0, 1, 5, 0xffff]) | pick() << 16)
            world.track[animation + 0x18 - v.TRACK] = rng.choice([0, 0, 2, 0x10, 0x12])
            struct.pack_into('<H', world.track, animation + 0x2e - v.TRACK, rng.choice([0, 0, 0, 1, max(frames - 1, 0)]))


def first_person_update_case(v, world, rng):
    user, slot, view, s, unit, weapon = fp_scene(v, world, rng, 'first-person-update', rows_optional=False)
    fp_update_events(v, world, rng)
    other = FP_VIEW + 0xc + 0x1010 * (1 - slot)
    fp_put(v, world, other + 4, NONE)
    for o in (0xf0, 0xf2):
        fp_put(v, world, other + o, rng.choice([0, 1, 2, 30, 0x8000]), 2)
    T = v.TRACK
    ubody = BODIES + BODY * (unit & 0xffff)
    wbody = BODIES + BODY * (weapon & 0xffff)
    k = world_word(v, world, wbody) & 3
    definition = DEFINITIONS + DEFINITION * k
    small = lambda: rng.choice([0, 0, f2u(rng.uniform(-1, 1)), f2u(rng.uniform(-3, 3)), f2u(-0.0), f2u(1e-40),
                                rfloat(rng, special=.3, nan=.03, span=2)])
    for o in range(3):  # the aim velocity (24570's length) and its spring targets
        world.put(ubody + 0x1b0 + 4 * o, rng.choice([0, small(), f2u(rng.uniform(-.3, .3)), f2u(rng.uniform(-2, 2))]))
    for o in (0x2e0, 0x2e4, 0x2e8, 0x2ec):
        world.put(definition + o, rng.choice([0, 0, f2u(rng.uniform(1, 100)), f2u(-0.0), rfloat(rng, special=.3, nan=.05)]))
    put8 = lambda address, value: fp_put(v, world, address, value, 1)
    put8(ubody + 0xaa, rng.choice([0, 0, 0, 1]))
    put8(ubody + 0x348, rng.choice([0, 0x10, 0x11, 1, 0x13]))
    put8(ubody + 0xc1, rng.choice([0, 0, 1]))
    put8(ubody + 0x399, rng.choice([0, 3, 5, 6, 10, 0x80]))
    put8(ubody + 0x3dc, rng.choice([1, 3, 0, 2]))
    world.put(ubody + 0x14, rng.choice([NONE, NONE, 0x10002]))
    struct.pack_into('<h', world.data2, v.TIME + 2 - v.DATA2, rng.choice([30, 30, 30, 60, 0, 1, -7, 29]))
    struct.pack_into('<I', world.data2, v.TIME + 4 - v.DATA2,
                     rng.choice([f2u(1 / 30), f2u(1 / 30), f2u(1 / 60), f2u(rng.uniform(.001, .2)), 0, f2u(-1 / 30),
                                 f2u(1e-40), f2u(rng.uniform(-.2, .2))]))  # finite: BF9A0/37DC0A take it as a dividend
    # The view's springs (+2030.. values, +2038.. rates), angles (+2050) and
    # angle changes (+2058); the input row's angles (+24), action (+42) and
    # magnitudes (+74/+78).
    for o in range(0x2030, 0x2060, 4):
        fp_put(v, world, view + o, rng.choice([0, 0, small(), small(), f2u(rng.uniform(-6, 6))]))
    for o in (0x2050, 0x2054):  # C1F90's previous angles: exact half-turn changes
        if rng.random() < .3:
            fp_put(v, world, view + o, 0)
    world.globals[0x4f9b54] = (FP_INPUTS - user * 0x94) & NONE
    for o in range(0, 0x94, 4):
        fp_put(v, world, FP_INPUTS + o, rng.getrandbits(32))
    for o in (0x24, 0x28):
        fp_put(v, world, FP_INPUTS + o, rng.choice([0, f2u(rng.uniform(-7, 7)), f2u(rng.uniform(-3.2, 3.2)), small(),
                                                    f2u(3.1415927), f2u(-3.1415927)]))
    fp_put(v, world, FP_INPUTS + 0x42, rng.choice([0xffff, 0xffff, 0xffff, 0, 3]), 2)
    for o in (0x74, 0x78):
        fp_put(v, world, FP_INPUTS + o, rng.choice([0, 0, 0, f2u(rng.uniform(0, 1)), f2u(-0.0), small()]))
    still = rng.random() < .5
    if still:  # 169C46's idle fidget needs every spring at rest
        for o in (0x2030, 0x2034, 0x2040, 0x2044, 0x2038, 0x203c, 0x2048, 0x204c, 0x2058, 0x205c):
            fp_put(v, world, view + o, 0)
        for o in (0x24, 0x28, 0x74, 0x78):
            fp_put(v, world, FP_INPUTS + o, 0)
        fp_put(v, world, view + 0x2050, 0)
        fp_put(v, world, view + 0x2054, 0)
        fp_put(v, world, FP_INPUTS + 0x42, rng.choice([0xffff] * 5 + [0, 3]), 2)
        if rng.random() < .3:  # 18A210 takes the larger magnitude
            fp_put(v, world, FP_INPUTS + rng.choice([0x74, 0x78]), rng.choice([f2u(-.5), f2u(.5), f2u(-1e-3)]))
        fp_put(v, world, s + 0xe4, rng.choice([0, 0, 0, f2u(.5)]))
        fp_put(v, world, s + 0xe8, rng.choice([0, 0, f2u(.1)]))
        for o in (0x1b0, 0x1b4, 0x1b8):
            world.put(ubody + o, rng.choice([0, 0, f2u(.05)]))
    # C1F90's half turn and turn ([0x5564C8], [0x5564C4]), 1D4510's zero
    # vector ([0x474D24]), 169BA2's sound vectors ([0x474D08], [0x474CE8])
    # and the blend policy byte [0x47C1B4].
    world.extra[0x5564c4] = bytearray(struct.pack('<ff', 6.2831855, 3.1415927) if rng.random() < .9 else rng.randbytes(8))
    world.extra[0x474d24] = bytearray(struct.pack('<I', FP_ZERO_VECTOR))
    for o in (0, 4):
        fp_put(v, world, FP_ZERO_VECTOR + o, rng.choice([0, 0, small()]))
    world.extra[0x474ce8] = bytearray(original_bytes(v, 0x474ce8, 4))
    world.extra[0x474d08] = bytearray(original_bytes(v, 0x474d08, 4))
    world.extra[0x47c1b4] = bytearray([rng.choice([1, 1, 0]), 1, 1, 0])
    # The idle globals (+94/+98 wait range, +9C chance) and the seed.
    globals_ = world.globals[0x4ecc1c]
    world.put(globals_ + 0x134, FP_IDLE)
    fp_put(v, world, FP_IDLE + 0x94, f2u(rng.choice([1.0, 2.0, 0.0, 0.1])))
    fp_put(v, world, FP_IDLE + 0x98, f2u(rng.choice([4.0, 2.0, 0.5, 1e-3])))
    fp_put(v, world, FP_IDLE + 0x9c, f2u(rng.choice([0.0, 0.5, 1.0, 0.9, 1.5])))
    world.put(v.SEED + 4, rng.getrandbits(32))
    fp_put(v, world, s + 0xec, rng.choice([0, 0, 1, 5, 30, 0x7fff, 0xffff]), 2)
    fp_put(v, world, s + 0xee, rng.choice([0, 0, 1, 2, 5, 40, 0xffff]), 2)
    fp_put(v, world, s + 0xf0, rng.choice([0, 1, 1, 2, 30, 0x8000]), 2)
    fp_put(v, world, s + 0xf2, rng.choice([0, 1, 2, 30, 0x8000]), 2)
    world.put(v.CINEMATIC + 4, rng.getrandbits(32))
    world.globals[0x51df50] = rng.choice([v.CINEMATIC, v.CINEMATIC, 0])
    # Directors: the first-person camera mostly (158290 kind 0).
    directors = world.absolute[0x4f5500]
    struct.pack_into('<I', directors, 0x14 + 0x140 * user + 0xc, rng.choice([0x245400] * 5 + [0x245ea0, 0x16f6a0, 0x246380, 0x12345]))
    struct.pack_into('<f', directors, 0x14 + 0x140 * user + 4, rng.choice([0.0] * 6 + [-0.0, 0.5, float('nan')]))
    world.extra[0x52d6a0] = bytearray([rng.choice([0, 1]), 0, 0, 0])
    # The slot's state, channel +98 (motion) and +B8 (overlay) keys, the
    # manager's channel at-end flags (16961F) and the transition channel.
    manager = s + 8
    fp_put(v, world, manager + 0x7c, rng.choice(FP_UPDATE_KEYS))
    fp_put(v, world, manager + 0x11, rng.choice([0, 0, 2, 8, 0xa, 1, 9]), 1)
    fp_put(v, world, s + 0xa0, rng.choice([0x60000d8, 0x6000060, NONE, rng.getrandbits(32)]))
    fp_put(v, world, manager + 0x51, rng.choice([0, 0, 2, 8, 0xa]), 1)
    fp_put(v, world, manager + 0x6c, rng.choice([0, 0, 0, 1, 0x20, 2]), 1)
    fp_put(v, world, manager + 0x80, rng.choice([f2u(rng.uniform(0, 1)), f2u(.45), f2u(.55), f2u(.5), 0, NANS[0]]))
    # The mode leaves' overlays also name the motion overlays 0x60000D8
    # (airborne, 1D4510) and 0x6000060 (moving); the definition's +292 kind
    # 3 continues 0x6000007 to 0xA000066 (16961F).
    for r in range(3):
        leaf = GRAPHS_ + 0x400 * r + 0x2c8
        rows = FP_OVERLAYS + 0x80 * r
        inherited = world_word(v, world, GRAPHS_ + 0x400 * r + 0x4c)
        names = {fp_word(v, world, rows + 8 * n): fp_word(v, world, rows + 8 * n + 4)
                 for n in range(world_word(v, world, leaf + 0xc))}
        for name in (0x60000d8, 0x6000060):
            if rng.random() < .75:
                names[name] = scene_handle(rng, inherited) if rng.random() < .9 else NONE
        world.put(leaf + 0xc, len(names))
        for n, name in enumerate(sorted(names)):
            fp_put(v, world, rows + 8 * n, name)
            fp_put(v, world, rows + 8 * n + 4, names[name])
    world.put(definition + 0x292, (world_word(v, world, definition + 0x292) & ~0xffff) | rng.choice([2, 2, 3, 0, 1]))
    follow = [0x400000c, 0xa000066, 0x8000071, 0xb0005b1, 0xb0005b2, 0x11000074, 0x11000076, 0x140005b3, 0x600005f]
    for r in range(3):
        leaf = GRAPHS_ + 0x400 * r + 0x2c8
        keys = sorted(set(rng.sample(follow, rng.choice([3, 5, 7])) +
                          rng.sample([k for k in FP_STATE_KEYS if k < 0x80000000], 3)))[:12]
        rows = FP_LEAF_STATES + 0x60 * r
        world.put(leaf + 4, len(keys))
        world.put(leaf + 8, rows)
        for n, key in enumerate(keys):
            fp_put(v, world, rows + 8 * n, key)
            fp_put(v, world, rows + 8 * n + 4, rng.randrange(8) << 16 | 0xffff if rng.random() < .9 else NONE)
    if still and rng.random() < .7:
        fp_put(v, world, manager + 0x7c, 0x400000c)
    elif not still and rng.random() < .2:  # the fidget ends (169863 with the 128E70 sound stop)
        fp_put(v, world, manager + 0x7c, 0x600005f)
        fp_put(v, world, s + 0x1008, rng.choice([NONE, 0x10002, 0x10002]))
    # 1D39C0 reads the primary channel's animation unchecked when the blend
    # channel (+20) is set under [0x47C1B4] (the original faults otherwise).
    if fp_word(v, world, manager + 0x20) != NONE and fp_word(v, world, manager + 0x26, 2) != 0xffff and \
            (fp_word(v, world, manager) == NONE or fp_word(v, world, manager + 6, 2) == 0xffff):
        fp_put(v, world, manager, fp_word(v, world, manager + 0x68))
        fp_put(v, world, manager + 4, rng.randrange(8) << 16 | 0xffff)
    # The manager's channels and the +98/+B8 records play the manager's
    # graph (1CE950's random variants, 1D3660, come from it).
    graph = fp_word(v, world, manager + 0x68)
    if graph != NONE:
        inherited = world_word(v, world, GRAPHS_ + 0x400 * GRAPH_TAGS.index(graph) + 0x4c)
        for c in (0, 0x20, 0x40, 0x90, 0xb0):
            if fp_word(v, world, manager + c) != NONE:
                fp_put(v, world, manager + c, graph)
                low = fp_word(v, world, manager + c + 4, 2)
                if low != 0xffff and low >= inherited:
                    fp_put(v, world, manager + c + 4, rng.choice([0xffff] + list(range(inherited))), 2)
    # Channel rates and frames within the animations mostly.
    for c in (0, 0x20, 0x40, 0x90, 0xb0):
        fp_put(v, world, manager + c + 0x10, rng.choice([0, 0, 0, 1, 3]), 1)
        fp_put(v, world, manager + c + 0x12, rng.choice([1, 3, 0x21, 0x23, 0x2b, 0x3003, 0x1003, 0x2003, 0x103, 0x283, 0, 0x81]), 2)
        fp_put(v, world, manager + c + 0x18, rng.choice([f2u(1.0), f2u(1.0), f2u(.5), f2u(2.0), 0, f2u(-1.0)]))
        fp_put(v, world, manager + c + 0x1c, rng.choice([0, f2u(rng.uniform(0, 5)), f2u(rng.uniform(0, 12)), f2u(30.0), f2u(-1.0)]))
    if rng.random() < .08:  # 16961F's 0xA000066 end: F0E00 false, 0x8000071 in every leaf
        fp_put(v, world, manager + 0x7c, 0xa000066)
        fp_put(v, world, manager + 0x11, 0xa, 1)
        fp_put(v, world, manager, fp_word(v, world, manager + 0x68))
        fp_put(v, world, manager + 4, rng.randrange(8) << 16 | 0xffff)
        put8(ubody + 0x212 + slot, 0xff)
        for r in range(3):
            leaf = GRAPHS_ + 0x400 * r + 0x2c8
            rows = FP_LEAF_STATES + 0x60 * r
            keys = [0x400000c, 0x8000071, 0xa000066]
            world.put(leaf + 4, len(keys))
            world.put(leaf + 8, rows)
            for n, key in enumerate(keys):
                fp_put(v, world, rows + 8 * n, key)
                fp_put(v, world, rows + 8 * n + 4, rng.randrange(8) << 16 | 0xffff)
    elif rng.random() < .1:
        fp_put(v, world, manager + 0x68, NONE)
        fp_put(v, world, manager, GRAPH_TAGS[0])  # a set channel meets the missing graph
        fp_put(v, world, manager + 4, rng.randrange(8) << 16 | 0xffff)
        fp_put(v, world, manager + 0x20, NONE)  # the blend channel's variants (1D3660) read the graph unchecked
        struct.pack_into('<I', directors, 0x14 + 0x140 * user + 0xc, 0x245ea0)
        put8(ubody + 0x348, 0)
        fp_put(v, world, manager + 0x7c, rng.choice([0x400000c, 0xc000064, 0x5000024]))
        fp_put(v, world, FP_INPUTS + 0x74, f2u(.5))
        fp_put(v, world, manager + 0x11, rng.choice([2, 8, 0xa]), 1)
    # The +B8 overlay's 37DC0A fmod: a NaN or infinite dividend reaches the
    # CRT error methods through 331ECB's jump table (outside the closure).
    if fp_word(v, world, manager + 0x7c) == 0xb000067:
        world.put(wbody + 0x188, rng.choice([0, f2u(1.0), f2u(rng.uniform(-2, 2)), f2u(-1.0), f2u(1e-40)]))
        dt = struct.unpack_from('<f', world.data2, v.TIME + 4 - v.DATA2)[0]
        if not math.isfinite(dt) or abs(dt) > 1e6:
            struct.pack_into('<f', world.data2, v.TIME + 4 - v.DATA2, 1 / 30)
    return [user]


composite('nodebuild_first_person_update', 0x169c46, 'void', FP_UPDATE_STOPS, 1500, cut=FP_CUT, stack=[0])
COMPOSITE['nodebuild_first_person_update']['through'] = FP_THROUGH
GENERATORS['nodebuild_first_person_update'] = first_person_update_case


# ---- The first-person owners on MCC caches (2026-11-15) ----
# MCC caches name the staged reload and melee states and the dual class in
# their own StringID namespace. The translated route joins the copied search
# keys inside 169863's 1D5320 call and 16AD45's four channel lookups; the
# observer applies those joins (the row's query_names, read from the
# translated route's source) and the natives search through the cache's
# names (weapon_animation::QueryNamespace). The graphs' class rows name the
# cache's dual class (sometimes TU1.5's), their leaves the cache's names of
# the first-person states (sometimes TU1.5's).
def query_names():
    from native_object_factory_cases_weapon import translated_query_names
    return translated_query_names()


FP_PENDING = 0x1304d80  # mcc_shape's load lists (one index per graph)


def mcc_shape(v, world, rng, keys=()):
    """Rename the scene graphs' classes and leaf states into the cache's
    namespace; `keys` are states the case enters (their cache names are put
    in some leaves)."""
    names = query_names()
    to_mcc = dict((tu15, mcc) for tu15, mcc in names['states'])
    dual, mcc_dual = names['dual']
    for r in range(3):
        data = GRAPHS_ + 0x400 * r
        world.put(data + 0x2b4, rng.choice([mcc_dual, mcc_dual, mcc_dual, dual, 0x30000d9]))
        leaf = data + 0x2c8
        rows = FP_LEAF_STATES + 0x60 * r
        count = world_word(v, world, leaf + 4)
        entries = {fp_word(v, world, rows + 8 * n): fp_word(v, world, rows + 8 * n + 4) for n in range(min(count, 12))}
        renamed = {}
        for key, handle in entries.items():
            renamed[to_mcc[key] if key in to_mcc and rng.random() < .85 else key] = handle
        for key in keys:
            if key in to_mcc and rng.random() < .7 and len(renamed) < 12:
                renamed[to_mcc[key]] = rng.randrange(8) << 16 | 0xffff
        world.put(leaf + 4, len(renamed))
        world.put(leaf + 8, rows)
        for n, key in enumerate(sorted(renamed)):
            fp_put(v, world, rows + 8 * n, key)
            fp_put(v, world, rows + 8 * n + 4, renamed[key])
        # Some leaves list a pending resource (+24/+28 load list; the row's
        # +10 byte set): a prefetch that finds the leaf reaches 126200, so
        # the keys 1E3420 and 1D46A0 prefetch with (the searched keys, which
        # stay in TU1.5's namespace) are compared too.
        if rng.random() < .3:
            index = rng.randrange(8)
            world.put(leaf + 0x24, 1)
            world.put(leaf + 0x28, FP_PENDING + 4 * r)
            world.put(FP_PENDING + 4 * r, index)
            world.track[data + 0x200 + 20 * index + 0x10 - v.TRACK] = 1


def first_person_state_mcc_case(v, world, rng):
    key, user, slot, stop = first_person_state_case(v, world, rng)
    if rng.random() < .6:
        key = rng.choice([tu15 for tu15, _ in query_names()['states']])
    mcc_shape(v, world, rng, [key])
    return [key, user, slot, stop]


def first_person_rebuild_mcc_case(v, world, rng):
    values = first_person_rebuild_case(v, world, rng)
    mcc_shape(v, world, rng)
    return values


# 16961F's follow-ups of these states (0xB0005B1, 0xB0005B2, 0x140005B3) are
# joined states.
FP_JOINED_ENDS = [0xc0005ae, 0x140005b0, 0x150005af, 0xa0005bb, 0x130005bc]


def first_person_update_mcc_case(v, world, rng):
    values = first_person_update_case(v, world, rng)
    for slot in (0, 1):
        manager = FP_VIEW + 0xc + 0x1010 * slot + 8
        if fp_word(v, world, FP_VIEW + 0xc + 0x1010 * slot + 4) != NONE and rng.random() < .5:
            fp_put(v, world, manager + 0x7c, rng.choice(FP_JOINED_ENDS))
    mcc_shape(v, world, rng, [0xb0005b1, 0xb0005b2, 0x140005b3])
    return values


composite('nodebuild_first_person_state_mcc', 0x169863, 'void', FP_STOPS, 500, cut=FP_CUT, regs={'eax': 0},
          stack=[1, 2, 3])
composite('nodebuild_first_person_rebuild_mcc', 0x16ad45, 'void', FP_STOPS, 500, cut=FP_CUT, stack=[0, 1])
composite('nodebuild_first_person_update_mcc', 0x169c46, 'void', FP_UPDATE_STOPS, 800, cut=FP_CUT, stack=[0])
for _name in ('nodebuild_first_person_state_mcc', 'nodebuild_first_person_rebuild_mcc',
              'nodebuild_first_person_update_mcc'):
    COMPOSITE[_name]['through'] = FP_THROUGH
    COMPOSITE[_name]['query_names'] = True
GENERATORS.update({'nodebuild_first_person_state_mcc': first_person_state_mcc_case,
                   'nodebuild_first_person_rebuild_mcc': first_person_rebuild_mcc_case,
                   'nodebuild_first_person_update_mcc': first_person_update_mcc_case})

def composite_main():
    import argparse
    import hashlib
    import json
    import os
    import random
    import subprocess
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import validate_native_campaign_script_builtins_tu15 as v
    register(v)
    parser = argparse.ArgumentParser(description='Composite nodebuild owners against the physical original')
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=0x5c41)
    parser.add_argument('--only', action='append', default=[])
    parser.add_argument('--coverage', default='', help='Comma-separated owner entries to report uncovered sites for')
    parser.add_argument('--gold-cache', type=Path, help='Reuse (or record) the original outcomes of identical cases '
                        '(mutation screens); the file belongs under HALO2_TEST_SCRATCH or another scratch directory')
    args = parser.parse_args()
    names = [n for n in COMPOSITE if not args.only or n in args.only]
    if not v.NEW_DEFAULT.is_file():
        print('SKIP missing owned TU1.5 executable')
        return 77
    if v.sha(v.NEW_DEFAULT) != v.NEW_PIN:
        raise ValueError('Wrong original executable')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image = v.Xbe(v.NEW_DEFAULT)
    rng = random.Random(args.seed)
    rows = []
    for name in names:
        spec = COMPOSITE[name]
        for repeat in range(spec['count']):
            world = v.World(rng)
            values = GENERATORS[name](v, world, rng)
            x87, mxcsr = rng.choice(v.FP_CONTROLS)
            gp = [0x5a010000 + len(rows) * 31 + k * 7 for k in range(8)]
            gp[4] = v.SP
            row = dict(id=len(rows), name=f'{name}-{repeat}', kernel=name, args=values, members=[], entry=spec['entry'],
                       stop_pc=v.STOP, stop_before_entries=sorted(spec['stops']), return_pc=v.STOP, gp=gp, matrix=0,
                       source=0, before_fp=[x87, 0, mxcsr], fp=[x87, mxcsr], live_x87=[])
            if spec.get('query_names'):
                row['query_names'] = query_names()
            # The ARG area holds 8 words; longer argument lists (2835B0, 283A90)
            # reach the original through registers and stack words only.
            banks = world.banks
            world.banks = lambda arguments, banks=banks: banks(arguments[:8])
            rows.append(v.routine_row(row, dict(routine=spec['routine']), world, values))
    key = hashlib.sha256(json.dumps([[r['args'], r['banks'], r['fp'], r['entry'], r['stop_before_entries']]
                                     for r in rows]).encode()).hexdigest()
    cached = json.loads(args.gold_cache.read_text()) if args.gold_cache and args.gold_cache.is_file() else None
    if cached and cached['key'] == key and not args.coverage:
        proof = cached['proof']
    else:
        stops = set().union(*(COMPOSITE[n]['stops'] | COMPOSITE[n]['cut'] for n in names))
        # An entry one selected kernel executes stays in the closure; the
        # other kernels' rows still stop before it.
        stops -= set().union(*(COMPOSITE[n].get('through', set()) for n in names))

        class Boundary:
            code = {}
        whole = v.complete_body
        v.complete_body = lambda code, address: Boundary() if address in stops else whole(code, address)
        v.UNREACHED_VIRTUAL_OWNERS.update(INDIRECT_TARGETS)
        entries = {n: COMPOSITE[n]['entry'] for n in names}
        try:
            # Only authenticate targets of indirect owners reached by this
            # selection. Adding every type hook to a codec-only run would
            # pull in unrelated CRT/placement closures without their cuts.
            while True:
                meta, code = v.metadata(image, entries)
                sites = {site['instruction'] for site in meta['sites']}
                targets = {target for owner, targets in INDIRECT_TARGETS.items() if owner in sites
                           for target in targets}
                missing = targets - set(entries.values())
                if not missing:
                    break
                entries.update({f'indirect-{target:x}': target for target in sorted(missing)})
        finally:
            v.complete_body = whole
        owned = v.prepare(image, meta, scratch)
        proof = v.physical(owned, rows)
        if args.gold_cache:
            args.gold_cache.write_text(json.dumps(dict(key=key, proof=dict(cases=[
                {k: c[k] for k in ('completed', 'writes', 'banks', 'stopped_at', 'boundary_gp', 'executed_sites',
                                   'query_joins') if k in c}
                for c in proof['cases']]))))
    golds = []
    outcomes = {}
    for row, original in zip(rows, proof['cases'], strict=True):
        if not original['completed']:
            raise ValueError('Original did not reach a boundary: ' + row['name'])
        writes = [[w['address'], w['written']] for w in original['writes']
                  if not v.STACK <= w['address'] < v.STACK + 0x10000 and not v.ARG <= w['address'] < v.ARG + 32]
        banks = {b['address']: b['after'] for b in original['banks'] if b['address'] != v.STACK}
        stopped = original.get('stopped_at')
        boundary = None if stopped == v.STOP else BOUNDARY_OWNERS.get(stopped, f'{stopped:X}')
        kind = COMPOSITE[row['kernel']]['kind']
        result = 0 if boundary or kind == 'void' else original['boundary_gp'][0] & (0xff if kind == 'boolean' else NONE)
        # 169BA2's 18BE50 residual: the sound (EBX) and the slot record (ESI)
        # the original holds at the stop.
        detail = None
        if boundary == '18BE50' and stopped == 0x18be50:
            gp = original['boundary_gp']
            detail = f'(EBX {gp[3] & NONE:08X}, ESI {gp[6] & NONE:08X})'
        golds.append(dict(boundary=boundary, writes=writes, banks=banks, result=result, detail=detail))
        key = (row['kernel'], boundary or 'returned')
        outcomes[key] = outcomes.get(key, 0) + 1
    total, stores = hashlib.sha256(), 0
    errors = scratch / 'composite-native.stderr'
    with errors.open('w+') as stderr:
        process = subprocess.Popen([str(args.native)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr, text=True)
        try:
            for row, gold in zip(rows, golds):
                process.stdin.write(json.dumps(dict(kernel=COMPOSITE[row['kernel']]['native'], args=row['args'], members=[], fp=row['fp'],
                    banks=[b for b in row['banks'] if b['address'] != v.STACK], publisher=False)) + '\n')
                process.stdin.flush()
                line = process.stdout.readline()
                if not line:
                    stderr.seek(0)
                    raise ValueError(row['name'] + ': native stopped: ' + stderr.read()[-3000:])
                actual = json.loads(line)
                native_banks = {b['address']: b['after'] for b in actual['banks']}
                problems = []
                if actual['writes'] != gold['writes']:
                    problems.append('stores differ')
                if native_banks != gold['banks']:
                    problems.append('final banks differ')
                if gold['boundary']:
                    expected = f"Native object factory: {gold['boundary']} "
                    if actual['completed'] or not actual.get('fault', '').startswith(expected):
                        problems.append(f"expected refusal at {gold['boundary']}, got {actual.get('fault', 'completion')}")
                    elif gold.get('detail') and gold['detail'] not in actual.get('fault', ''):
                        problems.append(f"expected {gold['detail']} at {gold['boundary']}, got {actual.get('fault')}")
                elif not actual['completed']:
                    problems.append('native refused: ' + actual.get('fault', ''))
                elif actual['result'] != gold['result']:
                    problems.append(f"result {actual['result']} != {gold['result']}")
                if problems:
                    raise ValueError(row['name'] + ': native differs (' + '; '.join(problems) + ')\nnative=' +
                                     json.dumps(dict(writes=actual['writes'], fault=actual.get('fault'), result=actual['result'])) +
                                     '\noriginal=' + json.dumps(dict(writes=gold['writes'], boundary=gold['boundary'],
                                                                     result=gold['result'])) + '\nargs=' + json.dumps(row['args']) +
                                     ' fp=' + json.dumps(row['fp']))
                total.update(json.dumps([actual['writes'], gold['boundary'], gold['result']]).encode())
                stores += len(gold['writes'])
            process.stdin.close()
            process.wait(timeout=30)
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=5)
    executed = set().union(*(set(c['executed_sites']) for c in proof['cases']))
    coverage = {}
    for entry in [int(x, 16) for x in args.coverage.split(',') if x]:
        body = whole(code, entry)
        coverage[f'{entry:x}'] = dict(sites=len(body.code), missing=[f'{pc:x}' for pc in sorted(body.code) if pc not in executed])
        print(f'{entry:x}', len(body.code), 'missing', coverage[f'{entry:x}']['missing'])
    # The translated route's MCC key joins the observer applied (query_names rows).
    joins = sum(len(c.get('query_joins', ())) for c in proof['cases'])
    report = dict(passed=True, physical_cases=len(rows), ordered_stores=stores, comparison_sha256=total.hexdigest(),
                  outcomes={f'{k[0]}:{k[1]}': n for k, n in sorted(outcomes.items())}, coverage=coverage,
                  original_xbe_sha256=v.NEW_PIN, instruction_patches=[], supplied_original_outcomes=False,
                  translated_route_query_joins=joins, retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} physical composite cases, {stores} ordered original stores')
    if joins:
        print(f'  {joins} translated-route MCC key joins applied (query_names cases)')
    for key, n in sorted(outcomes.items()):
        print(f'  {key[0]}: {key[1]} x{n}')
    return 0


if __name__ == '__main__':
    raise SystemExit(composite_main())
