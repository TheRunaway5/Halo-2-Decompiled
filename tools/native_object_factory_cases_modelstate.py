"""Oracle cases for factory track "modelstate" (change colors, regions).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Whole owners whose static closure reaches a refused callee or a CRT error
method (C0270 through 37DC0A, BC5C0 through 17E060/1CE0E0) need the
validator's closure stops: CLOSURE_STOPS (functions never admitted, so
executing one escapes the authenticated sites) and REGISTER_JUMPS (declared
targets of a register-based indirect JMP, 331ECB's [EBX] dispatch). Those
kernels register only when the validator provides both tables.
"""
import struct

from native_object_factory_track_fixtures import borrow

NONE = 0xffffffff
# 37DC0A's two admitted methods (FPREM, FLDZ); 3334F0 is the CRT error path.
FMOD_METHODS = (0x37dc14, 0x331f68)
# Refused subtrees of this track's owners. C0CD0's creators (C2260 light,
# 18BD80 looping sound, 17E940 contrail) and C0910's BCB70/134730 are wired
# to the attachments track's natives: their closures are proven here, with
# that track's own stops (COMPOSED). 179540 (effects) still refuses at 178E30.
CLOSURE_STOPS = {0x3334f0, 0x17b1f0, 0x1cc310, 0x126200, 0x1e3090, 0x1d62c0,
                 0x118480, 0x119060, 0xc5b00, 0x179540}
# Routines whose closures reach the attachments track's owners.
COMPOSED = ('object_function_colors', 'object_attachments_create', 'object_type_finish', 'object_widgets_create')


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def register(v):
    """Registers this track and scopes its closure stops to its own routines."""
    attachments, _ = borrow(v, 'attachments')
    before = set(v.ROUTINES)
    _register(v, attachments.FIXTURES)
    for name in set(v.ROUTINES) - before:
        stops = set(v.ROUTINES[name].get('closure_stops', ())) | CLOSURE_STOPS
        if name in COMPOSED:
            stops |= set(attachments.STOPS)
        v.ROUTINES[name]['closure_stops'] = sorted(stops)
    # BEC50 (10DE40's CALL ECX target, under the attachments track's stops)
    # joins the authenticated sites for the composed routines too.
    attachments.ROUTINE_NAMES.extend(name for name in COMPOSED if name in v.ROUTINES)
    attachments._indirect_metadata(v)


def _register(v, attached):
    TRACK, DATA, DATA2, DATA3 = v.TRACK, v.DATA, v.DATA2, v.DATA3
    OBJECT_TAGS, MODEL_TAGS, ROWS = v.OBJECT_TAGS, v.MODEL_TAGS, v.ROWS
    staged = hasattr(v, 'CLOSURE_STOPS') and hasattr(v, 'REGISTER_JUMPS')
    v.JUMP_TABLES[0x134610] = 6          # 134510's six hue sectors
    v.JUMP_TABLES[0xc0e78] = 4           # C0CD0's four attachment kinds
    # D6E40's widget creator call (CALL ESI) is never reached: every fixture
    # widget of a known type has a NONE tag (its creators are refused).
    v.UNREACHED_VIRTUAL_OWNERS.add(0xd6e40)
    if staged:
        # Stops apply to this track's routines only (added below via
        # register()), so other tracks porting these owners still see them.
        v.REGISTER_JUMPS[0x331f55] = FMOD_METHODS

    # Track bank layout.
    RGB, HSV, OUT = TRACK, TRACK + 0x20, TRACK + 0x40
    SLOTS = TRACK + 0x80
    DEFAULT_COLOR, REQUEST = TRACK + 0x100, TRACK + 0x140
    RENDER_POOL, RENDER_ROWS = TRACK + 0x200, TRACK + 0x400       # [515F14], 0x100 rows
    COLOR_ROWS, PERMUTATIONS = TRACK + 0x800, TRACK + 0x900       # definition +B0 rows; 0x20 rows
    FUNCTIONS = TRACK + 0x1400                                    # 0x28 function entries
    VARIANTS = TRACK + 0x1800                                     # model +54 rows (0x38)
    REGIONS, REGION_PERMS = TRACK + 0x2000, TRACK + 0x2800        # model +74 rows (16), 8-byte perms
    VARIANT_REGIONS, VARIANT_PERMS, VARIANT_ENTRIES = TRACK + 0x3000, TRACK + 0x3400, TRACK + 0x3c00
    STATES, REGION_OUT = TRACK + 0x4800, TRACK + 0x4900
    EFFECT_POOL, EFFECT_ROWS = TRACK + 0x5000, TRACK + 0x5100
    HAVOK_POOL, HAVOK_ROWS = TRACK + 0x6000, TRACK + 0x6100
    PREVIOUS = TRACK + 0x7000
    VARIANT_NAMES = [0x1000101, 0x2000202, 0x3000303]

    def track(world, rng):
        if world.track is None:
            world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))

    def byte(world, address, value):
        for base, bank in ((TRACK, world.track), (DATA, world.data), (DATA2, world.data2), (DATA3, world.data3)):
            if bank is not None and base <= address < base + len(bank):
                bank[address - base] = value & 0xff
                return
        raise ValueError(f'byte outside the declared banks {address:x}')

    def half(world, address, value):
        byte(world, address, value)
        byte(world, address + 1, value >> 8)

    def unit(rng):
        special = [0.0, -0.0, 1.0, 0.5, 0.25, 1.5, -0.25, 1e-40, -1e-40, 0.999999, 1.0000001, 2.0, float('nan')]
        return fbits(rng.choice(special)) if rng.random() < .25 else fbits(rng.uniform(0, 1))

    def color(rng):
        style = rng.random()
        if style < .15:
            value = unit(rng)
            return [value, value, value]
        if style < .3:
            a, b = unit(rng), unit(rng)
            return rng.choice([[a, a, b], [a, b, a], [b, a, a]])
        return [unit(rng) for _ in range(3)]

    def put_rgb(world, address, rgb):
        for n, value in enumerate(rgb):
            world.put(address + 4 * n, value)

    def definition(k):
        return OBJECT_TAGS + 0x200 * k

    def model(k):
        return MODEL_TAGS + 0x80 * k

    def colors_layout(world, rng, n, count):
        body = world.body(n)
        half(world, body + 0x124, count * 0x18)
        half(world, body + 0x126, 0x300)
        for slot in range(2 * count):
            put_rgb(world, body + 0x300 + 12 * slot, color(rng))

    def render_rows(world, rng):
        world.globals[0x515f14] = RENDER_POOL
        world.put(RENDER_POOL + 0x44, RENDER_ROWS)
        for n in range(ROWS - 1):
            handle = rng.choice([NONE, NONE, 0, 1, 2, 3])
            if handle != NONE:
                handle |= rng.getrandbits(16) << 16
            world.put(world.body(n) + 0xcc, handle)

    def change_color_rows(world, rng):
        for k in range(4):
            world.put(definition(k) + 0xb0, COLOR_ROWS + 0x40 * k)
            byte(world, definition(k) + 0x1c, rng.choice([0, 1, 0xfe, 0xff]))
        for k in range(4):
            world.put(model(k) + 0x54, VARIANTS + 0x100 * k)
            for row in range(4):
                world.put(VARIANTS + 0x100 * k + 0x38 * row, rng.choice(VARIANT_NAMES + [0x4444]))
        for r in range(16):
            row = COLOR_ROWS + 0x10 * r
            world.put(row, rng.choice([0, 0, 1, 2, 3, 4, NONE]))
            world.put(row + 4, PERMUTATIONS + 0x80 * (r % 8))
        for p in range(32):
            at = PERMUTATIONS + 0x20 * p
            world.put(at, rng.choice([fbits(rng.uniform(0, 2)), fbits(1.0), 0, fbits(-0.5), fbits(1e-40), fbits(float('nan'))])
                      if rng.random() < .3 else fbits(rng.uniform(0.05, 3)))
            # No denormal channels: a tiny negative hue wraps to exactly 1.0
            # under upward rounding, whose sector 6 jumps past 134510's table.
            put_rgb(world, at + 4, [0 if (c & 0x7f800000) == 0 else c for c in color(rng)])
            put_rgb(world, at + 0x10, [0 if (c & 0x7f800000) == 0 else c for c in color(rng)])
            world.put(at + 0x1c, rng.choice([0, 0, 0] + VARIANT_NAMES))

    def object_index(rng):
        return rng.randrange(ROWS - 1)

    def position(world, rng, n):
        body = world.body(n)
        for offset in (0x64, 0x68, 0x6c):
            # A zero (or denormal) y makes color 0's blend weight 1 on the
            # first hue, which a wrap can leave at exactly 1.0: hue sector 6
            # jumps past 134510's table in the original.
            special = [1.5, -3.25, 12345.678, -0.001] if offset == 0x68 else [0.0, -0.0, 1.5, -3.25, 1e-40, 12345.678, -0.001]
            value = rng.choice(special) if rng.random() < .3 else rng.uniform(-500, 500)
            world.put(body + offset, fbits(value))
        byte(world, body + 0xb1, rng.choice([0xff, 0, 1, 2, 3]))

    # --- 1343E0 / 134510 / 1349E0 ------------------------------------------
    def rgb_case(name, world, rng):
        track(world, rng)
        put_rgb(world, RGB, color(rng))
        return [RGB, HSV]

    def hsv_case(name, world, rng):
        track(world, rng)
        style = rng.random()
        if style < .7:
            hue = fbits(rng.choice([0.0, 1 / 6, 2 / 6, 0.5, 4 / 6, 5 / 6, 0.9999, 1e-40, rng.uniform(0, 0.9999)])
                        if rng.random() < .4 else rng.uniform(0, 0.9999))
        else:
            hue = fbits(rng.choice([float('nan'), 1e10, 3e9, float('inf')]))
        world.put(HSV, hue)
        world.put(HSV + 4, rng.choice([0, fbits(-0.0), fbits(1e-40), unit(rng), unit(rng), unit(rng)]))
        world.put(HSV + 8, unit(rng))
        return [OUT, HSV]

    def pack_case(name, world, rng):
        track(world, rng)
        rgb = [rng.choice([fbits(rng.uniform(0, 1)), unit(rng), fbits(rng.choice([-1.0, 300.0, 1e12, -1e12, 0.5 / 255, 1.5 / 255]))])
               for _ in range(3)]
        put_rgb(world, RGB, rgb)
        return [RGB]

    # --- BCD80 / 3DD00 / C0270 / C0910 --------------------------------------
    def channel_case(name, world, rng):
        track(world, rng)
        n = object_index(rng)
        colors_layout(world, rng, n, rng.choice([0, 1, 2, 3, 4]))
        return [world.handles[n], rng.choice([0, 1, 2, 3, 4, NONE, 0x80000000]), OUT]

    def publish_case(name, world, rng):
        track(world, rng)
        render_rows(world, rng)
        n = object_index(rng)
        colors_layout(world, rng, n, rng.choice([0, 1, 2, 3, 4, 5]))
        for k in range(4):
            byte(world, definition(k) + 0x1c, rng.choice([0, 1, 0xfe, 0xff]))
        return [world.handles[n], rng.choice([0, 1, 0x100, 0xff])]

    def change_colors_case(name, world, rng):
        track(world, rng)
        render_rows(world, rng)
        change_color_rows(world, rng)
        world.globals[0x474c70] = DEFAULT_COLOR
        put_rgb(world, DEFAULT_COLOR, color(rng))
        n = object_index(rng)
        count = rng.choice([0, 1, 2, 3, 4])
        colors_layout(world, rng, n, count)
        position(world, rng, n)
        for slot in range(4):
            put_rgb(world, REQUEST + 12 * slot, color(rng))
        mask = rng.choice([0, 0, 0, 1, 2, 5, 0xf, rng.getrandbits(32)])
        if rng.random() < .15:
            # Threshold tie: a zero position hashes color 0 to 0, so a zero
            # first weight meets the threshold (JAE) before the last pick.
            for offset in (0x64, 0x68, 0x6c):
                world.put(world.body(n) + offset, 0)
            k = struct.unpack_from('<I', world.data, world.body(n) - DATA)[0] & 3
            row = COLOR_ROWS + 0x40 * k
            world.put(row, rng.choice([2, 3]))
            perms = struct.unpack_from('<I', world.track, row + 4 - TRACK)[0]
            world.put(perms, 0)
            world.put(perms + 0x20, fbits(1.0))
            # Mid-range hues: no wrap can carry the blend to exactly 1.0 (a
            # hue sector of 6 jumps past 134510's table in the original).
            for at in (perms + 4, perms + 0x10):
                put_rgb(world, at, [fbits(rng.uniform(0.1, 0.3)), fbits(rng.uniform(0.6, 0.9)), fbits(rng.uniform(0.35, 0.5))])
            world.put(perms + 0x1c, 0)
            world.put(perms + 0x3c, 0)
            mask &= ~1
            if count == 0:
                colors_layout(world, rng, n, 1)
        return [world.handles[n], mask, REQUEST]

    def function_colors_case(name, world, rng):
        """C0910 over the attachments track's function world (objects,
        definition functions, body values): each entry's function (+24) and
        scale (+20) names reach BCB70; 134730 blends with flags bit 0 clear
        (its HSV branch is refused)."""
        attached['function_world'](world, rng)
        n = object_index(rng)
        colors_layout(world, rng, n, rng.choice([0, 1, 2, 3, 4]))
        for k in range(4):
            world.put(definition(k) + 0xb0, COLOR_ROWS + 0x40 * k)
            byte(world, definition(k) + 0x1c, rng.choice([0, 1, 1, 0xfe, 0xff]))
        for r in range(16):
            row = COLOR_ROWS + 0x10 * r
            world.put(row + 8, rng.choice([0, 0, 1, 2, 2, NONE]))
            world.put(row + 0xc, FUNCTIONS + 0x50 * (r % 8))
        for e in range(16):
            at = FUNCTIONS + 0x28 * e
            world.put(at + 4, rng.choice([0, 0, 2, 0xfffffffe, 0x10]))
            put_rgb(world, at + 8, color(rng))
            put_rgb(world, at + 0x14, color(rng))
            for offset in (0x20, 0x24):
                world.put(at + offset, attached['name_choice'](rng) if rng.random() < .7 else 0)
        return [world.handles[n]]

    # --- BC6C0 / 1700E0 / 17E060 / 1CE0E0 / BC5C0 ---------------------------
    def region_layout(world, rng, n, count):
        body = world.body(n)
        half(world, body + 0x118, count * 10 + rng.choice([0, 0, 0, 3, 9]))
        half(world, body + 0x11a, rng.choice([0x200, 0x240]))

    def bank_case(name, world, rng):
        track(world, rng)
        n = object_index(rng)
        body = world.body(n)
        half(world, body + 0x118, rng.choice([0, 10, 19, 60, 160, 0x7ff0, 0x8000, 0xfff6, rng.getrandbits(16)]))
        half(world, body + 0x11a, rng.choice([0x200, 0x8000, rng.getrandbits(16)]))
        count, active, cached, rows = SLOTS, SLOTS + 4, SLOTS + 8, SLOTS + 0xc
        choice = rng.random()
        if choice < .2:
            active = count                     # the count slot is re-read after [active]
        elif choice < .3:
            cached = count
        slots = [rng.choice([active, active, 0]), rng.choice([cached, cached, 0]), rng.choice([rows, rows, 0])]
        return [world.handles[n], count] + slots

    def model_regions(world, rng, k, regions, physics, zero_first=False):
        m = model(k)
        world.put(m + 0x70, regions)
        world.put(m + 0x74, REGIONS + 0x80 * k)
        world.put(m + 0x54, VARIANTS + 0x100 * k)
        for r in range(8):
            region = REGIONS + 0x80 * k + 0x10 * r
            world.put(region + 8, rng.choice([0, 1, 1, 2, 3, 4, 5, 9]))
            world.put(region + 0xc, REGION_PERMS + 0x80 * (r % 8))
        for p in range(128):
            byte(world, REGION_PERMS + 8 * p + 4, rng.choice([0, 0, 0, 1, 2, 0xfe]))
        # Physics bytes (+6) for every signed state byte of every region's
        # window: all 0xFF keeps 1CE0E0 away from the refused 1CC310.
        for p in range(-128, 256):
            byte(world, REGION_PERMS + 8 * p + 6, 0xff if physics else rng.choice([0xff, 0, 1]))
        for row in range(4):
            at = VARIANTS + 0x100 * k + 0x38 * row
            world.put(at, rng.choice(VARIANT_NAMES + [0x4444]))
            for r in range(8):
                byte(world, at + 4 + r, rng.choice([0xff, 0xff, 0, 1, 2, 3]))
            world.put(at + 0x18, VARIANT_REGIONS + 0x14 * 4 * row)   # four 0x14 region rows each
        for vr in range(4 * 12):
            at = VARIANT_REGIONS + 0x14 * vr
            world.put(at + 8, rng.choice([0, 1, 1, 2, 3]))
            world.put(at + 0xc, VARIANT_PERMS + 0x80 * (vr % 12))
        for vp in range(48):
            at = VARIANT_PERMS + 0x20 * vp
            byte(world, at + 4, rng.choice([0, 1, 2, 0xff, 0x80]))
            world.put(at + 8, fbits(rng.uniform(0, 2)) if rng.random() < .8 else rng.choice([0, fbits(-1.0), fbits(float('nan')), fbits(1e-40)]))
            world.put(at + 0xc, rng.choice([0, 0, 1, 2, 3, NONE]))
            world.put(at + 0x10, VARIANT_ENTRIES + 0x48 * (vp % 24))
        for e in range(72):
            at = VARIANT_ENTRIES + 0x18 * e
            byte(world, at + 4, rng.choice([0, 1, 2, 3, 0xff]))
            byte(world, at + 5, rng.choice([0, 1, 0x80]))
            half(world, at + 6, rng.choice([0, 1, 0x1234, 0xffff]))
            world.put(at + 0x14, fbits(rng.uniform(0, 1)) if rng.random() < .85 else rng.choice([0, fbits(-0.5), fbits(float('nan'))]))
            if zero_first and e % 3 == 0:
                world.put(at + 0x14, 0)

    def seed(world, rng):
        world.put(v.SEED, rng.getrandbits(32))

    def permutations_case(name, world, rng):
        track(world, rng)
        seed(world, rng)
        k = rng.randrange(4)
        tie = rng.random() < .2
        if tie:
            # The next LCG state has a zero high word: the draw is 0, which a
            # zero first entry weight ties (170930 picks on JA only).
            world.put(v.SEED, ((rng.randrange(0x10000) - 0x3c6ef35f) * pow(0x19660d, -1, 1 << 32)) & 0xffffffff)
        model_regions(world, rng, k, rng.choice([0, 1, 2, 3, 5, 8]), False, tie)
        tag = (0x5000 + 4 + k) << 16 | (4 + k)
        variant = rng.choice([NONE, NONE, 0, 1, 2, 3])
        mask = rng.choice([0, 0, 0, 1, 4, 0xff, rng.getrandbits(32)])
        return [tag, variant, STATES, REGION_OUT, mask]

    def attachments(world, rng, n):
        body = world.body(n)
        entries = rng.choice([0, 1, 2, 4])
        half(world, body + 0x11c, entries * 8 + rng.choice([0, 0, 3]))
        half(world, body + 0x11e, 0x2c0)
        world.globals[0x4f720c] = EFFECT_POOL
        world.put(EFFECT_POOL + 0x24, 400)
        world.put(EFFECT_POOL + 0x38, rng.choice([0, 2, 4]))
        world.put(EFFECT_POOL + 0x44, EFFECT_ROWS)
        for r in range(4):
            half(world, EFFECT_ROWS + 400 * r, rng.choice([0, 0x1111 + r, 0x8001 + r]))
        for e in range(entries + 1):
            at = body + 0x2c0 + 8 * e
            byte(world, at, rng.choice([2, 2, 0, 1, 0xff]))
            index = rng.choice([0, 1, 2, 3, 5])
            salt = rng.choice([0, 0x2222, 0x9999])     # never a live salt: 17B1F0 is refused
            world.put(at + 4, rng.choice([NONE, salt << 16 | index]))

    def effect_case(name, world, rng):
        track(world, rng)
        n = object_index(rng)
        attachments(world, rng, n)
        return [world.handles[n]]

    def havok(world, rng, n):
        world.globals[0x52be5c] = HAVOK_POOL
        world.put(HAVOK_POOL + 0x44, HAVOK_ROWS)
        for r in range(4):
            world.put(HAVOK_ROWS + 0xa0 * r + 4, rng.choice([0, 0x800, 0xfffff7ff, NONE]))
        world.put(world.body(n) + 0xb4, rng.choice([NONE, 0, 1, 2, 3 | 0x40000]))

    def physics_case(name, world, rng):
        track(world, rng)
        n = object_index(rng)
        body = world.body(n)
        k = struct.unpack_from('<I', world.data, body - DATA)[0] & 3
        regions = rng.choice([0, 1, 2, 3, 6])
        model_regions(world, rng, k, regions, True)
        world.put(definition(k) + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
        region_layout(world, rng, n, regions)
        havok(world, rng, n)
        base = body + struct.unpack_from('<h', world.data, body + 0x11a - DATA)[0]
        for r in range(8):
            byte(world, base + r, rng.choice([0xff, 0, 1, 2]))
            byte(world, PREVIOUS + r, rng.choice([0xff, 0, 1, 2]))
        return [PREVIOUS, world.handles[n]]

    def regions_case(name, world, rng):
        track(world, rng)
        seed(world, rng)
        n = object_index(rng)
        body = world.body(n)
        k = struct.unpack_from('<I', world.data, body - DATA)[0] & 3
        regions = rng.choice([0, 1, 2, 3, 5, 8])
        model_regions(world, rng, k, regions, True)
        if rng.random() < .25:
            world.put(definition(k) + 0x38, NONE)
        else:
            world.put(definition(k) + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
        region_layout(world, rng, n, regions)
        base = body + struct.unpack_from('<h', world.data, body + 0x11a - DATA)[0]
        for r in range(8):
            byte(world, base + r, rng.choice([0xff, 0, 1, 2]))
        byte(world, body + 0xb1, rng.choice([0xff, 0, 1, 2, 3]))
        attachments(world, rng, n)
        havok(world, rng, n)
        if struct.unpack_from('<I', world.data3, definition(k) + 0x38 - DATA3)[0] == NONE:
            world.put(body + 0xb4, NONE)       # Havok components imply a model (1CE0E0 reads it unchecked)
        mask = rng.choice([0, 0, 1, 5, 0xff, rng.getrandbits(32)])
        return [world.handles[n], mask]

    kernels = [
        ('color_rgb_to_hsv', 'ms_rgb', 'long', dict(entry=0x1343e0, regs={'ecx': 0, 'edx': 1}), rgb_case, 600, False),
        ('color_hsv_to_rgb', 'ms_hsv', 'long', dict(entry=0x134510, regs={'ecx': 0, 'edx': 1}), hsv_case, 600, False),
        ('color_pack', 'ms_pack', 'long', dict(entry=0x1349e0, stack=[0]), pack_case, 400, False),
        ('object_color_channel', 'ms_channel', 'boolean', dict(entry=0xbcd80, regs={'eax': 0, 'edi': 1, 'esi': 2}),
         channel_case, 300, False),
        ('object_color_publish', 'ms_publish', 'void', dict(entry=0x3dd00, stack=[0, 1]), publish_case, 400, False),
        ('object_change_colors_initialize', 'ms_change_colors', 'void', dict(entry=0xc0270, stack=[0, 1, 2]),
         change_colors_case, 1200, True),
        ('object_function_colors', 'ms_function_colors', 'boolean', dict(entry=0xc0910, stack=[0]),
         function_colors_case, 800, True),
        ('object_region_bank', 'ms_bank', 'void', dict(entry=0xbc6c0, regs={'eax': 0, 'esi': 1, 'edi': 2, 'ebx': 3}, stack=[4]),
         bank_case, 300, False),
        ('model_region_permutations_select', 'ms_permutations', 'void', dict(entry=0x1700e0, stack=[0, 1, 2, 3, 4]),
         permutations_case, 1200, False),
        ('object_region_effect_restart', 'ms_effect', 'void', dict(entry=0x17e060, regs={'eax': 0}), effect_case, 400, True),
        ('object_region_physics_check', 'ms_physics', 'void', dict(entry=0x1ce0e0, regs={'eax': 0}, stack=[1]),
         physics_case, 400, True),
        ('object_regions_initialize', 'ms_regions', 'void', dict(entry=0xbc5c0, stack=[0, 1]), regions_case, 800, True),
    ]
    # --- 1D3380 leaves: graphs (tag rows 20..23) in the track bank -----------
    GRAPHS, BLOCKS, CHANNELS = TRACK + 0x8000, TRACK + 0xa000, TRACK + 0xc000
    RESOURCE_LIST, STATE_ROWS, TRANSITION_ROWS = TRACK + 0xf000, TRACK + 0xf100, TRACK + 0xf200
    LABELS = [0x6000086, 0x7000083, 0x30000d9, 0x400000c, 0x400054b, 0x1000011, 0x2000022, 0x5000033, 0x7000001, 0x7000101]

    def graph_tag(g):
        return (0x7700 + g) << 16 | (20 + g)

    def sorted_block(world, rng, block, rows, stride, keys):
        chosen = sorted(rng.sample(keys, min(len(keys), 6)), key=lambda x: struct.unpack('<i', struct.pack('<I', x))[0])
        # Never more rows than written: unwritten rows hold wild pointers.
        count = rng.choice([0, 1, 2, 3, 5, len(chosen)]) if rng.random() < .95 else NONE
        world.put(block, count)
        world.put(block + 4, rows)
        if rng.random() < .1:
            rng.shuffle(chosen)
        for n, key in enumerate(chosen):
            world.put(rows + stride * n, key)
        return chosen

    def graphs(world, rng):
        track(world, rng)
        world.globals[0x51df20] = rng.choice([0, 1, 0x100, 0x101, 0x101, 0xffff])
        cursor = BLOCKS
        for g in range(4):
            graph = GRAPHS + 0x800 * g
            world.put(v.TAG_TABLE + 16 * (20 + g) + 8, graph)
            anims = graph + 0x100
            world.put(graph + 0x30, anims)
            for a in range(8):
                row = anims + 0x6c * a
                flagged = rng.random() < .4
                byte(world, row + 0x16, rng.choice([0x10, 0x30, 0xff]) if flagged else rng.choice([0, 0xef, 0x20]))
                byte(world, row + 0x18, rng.choice([0, 1, 0xfe, 0xff]))
                world.put(row + 0x1c, fbits(rng.choice([1.0, 1.0, 0.5, 0.25, 1.5, 0.0, rng.uniform(0, 1), rng.uniform(0, 1), 1e-40])))
                half(world, row + 0x14, rng.choice([0, 1, 2, 10, 30, 31, 0xffff]))
                world.put(row + 0x4c, rng.choice([0, 1, 2, 3, 5, 8]))
                world.put(row + 0x50, graph + 0x500 + 0x20 * a)
                for e in range(8):
                    byte(world, graph + 0x500 + 0x20 * a + 4 * e, rng.choice([0, 1, 3, 7, 15, 16, 20]))
                    half(world, graph + 0x500 + 0x20 * a + 4 * e + 2, rng.choice([0, 1, 2, 5, 29, 30, 31, 0xfffe]))
                world.put(row + 0x24, rng.choice([0, 1]) if flagged else rng.choice([0, 1, 2, 3]))
                half(world, row + 0x30, rng.choice([0xffff, 0xffff, rng.randrange(8)]))
                half(world, row + 0x32, rng.choice([0xffff, rng.randrange(a + 1, 8)]) if a < 7 else 0xffff)
            world.put(graph + 0xb0, graph + 0x80)
            for r in range(4):
                byte(world, graph + 0x80 + 20 * r + 0x10, 0 if r < 2 else rng.choice([1, 0x80, 0xff]))
            world.put(graph + 0x4c, rng.choice([0, 1, 2, 4, NONE]))
            world.put(graph + 0x50, graph + 0x40)
            for i in range(2):
                world.put(graph + 0x40 + 0x20 * i + 4, rng.choice([NONE, graph_tag(rng.randrange(4))]))
            # The +54 weapon-type -> class rows (8 bytes).
            sorted_block(world, rng, graph + 0x54, cursor, 8, LABELS)
            for n in range(6):
                world.put(cursor + 8 * n + 4, rng.choice(LABELS + [NONE]))
            cursor += 0x40
        return cursor

    def entry_graph(world, rng):
        """One graph with a fully linked mode/class/type chain."""
        track(world, rng)
        graph = GRAPHS
        cursor = BLOCKS
        modes = sorted_block(world, rng, graph + 0x34, cursor, 0x14, LABELS)
        mode_rows = cursor; cursor += 0x100
        for m in range(len(modes)):
            classes = sorted_block(world, rng, mode_rows + 0x14 * m + 4, cursor, 0x14, LABELS)
            class_rows = cursor; cursor += 0x100
            for c in range(len(classes)):
                sorted_block(world, rng, class_rows + 0x14 * c + 4, cursor, 0x34, LABELS)
                # Type rows: resource lists (+24/+28, +2C/+30) naming only the
                # non-resident resources 0 and 1 (126200 is refused).
                for n in range(7):
                    world.put(cursor + 0x34 * n + 0x24, rng.choice([0, 1, 2, NONE]))
                    world.put(cursor + 0x34 * n + 0x28, RESOURCE_LIST)
                    world.put(cursor + 0x34 * n + 0x2c, rng.choice([0, 1, 2, NONE]))
                    world.put(cursor + 0x34 * n + 0x30, RESOURCE_LIST + 8)
                    world.put(cursor + 0x34 * n + 4, rng.choice([0, 1, 2, 3, 4, 4, NONE]))
                    world.put(cursor + 0x34 * n + 8, STATE_ROWS)
                    world.put(cursor + 0x34 * n + 0x1c, rng.choice([0, 1, 2, 3]))
                    world.put(cursor + 0x34 * n + 0x20, TRANSITION_ROWS)
                cursor += 0x180
        for n in range(4):
            world.put(RESOURCE_LIST + 4 * n, rng.choice([0, 1]))
        # Transitions (0x14 rows: key, bytes +A/+B, +C/+10 sub-rows) whose
        # sub-rows never name the requested mode (1E4B70's hit is refused).
        for n in range(3):
            entry = TRANSITION_ROWS + 0x14 * n
            world.put(entry + 4, rng.choice([0x400000c, 0x1000011, 0x7777]))
            byte(world, entry + 0xa, rng.choice([0xff, 0xff, 0]))
            byte(world, entry + 0xb, rng.choice([0xff, 0xff, 1]))
            world.put(entry + 0xc, rng.choice([0, 1, 2]))
            world.put(entry + 0x10, TRANSITION_ROWS + 0x40)
        for n in range(2):
            world.put(TRANSITION_ROWS + 0x40 + 0x14 * n + 4, 0x9999)
        # (state, packed) rows sorted by state; local animations only.
        for n, state in enumerate([0x1000011, 0x2000022, 0x400000c, 0x5000033]):
            world.put(STATE_ROWS + 8 * n, state)
            world.put(STATE_ROWS + 8 * n + 4, rng.choice([0xffff, rng.randrange(8)]) << 16 | 0xffff)
        return graph

    def valid_packed(world, rng, graph):
        """A packed animation whose import (if any) exists."""
        index = rng.choice([0xffff, rng.randrange(8), rng.randrange(8), rng.randrange(8)])
        imports = struct.unpack('<i', struct.pack('<I', struct.unpack_from('<I', world.track, graph + 0x4c - TRACK)[0]))[0]
        live = [i for i in range(min(imports, 2))
                if struct.unpack_from('<I', world.track, graph + 0x40 + 0x20 * i + 4 - TRACK)[0] != NONE]
        return index << 16 | rng.choice([0xffff, 0xffff] + live)

    # --- 1D3380's node-count guard (its setup path needs 1D5320: refused) ---
    def manager_world(world, rng):
        """Graph 0 with labels, state rows, transitions and animations; the
        tick, RNG, secondary-mode globals and a manager record."""
        graphs(world, rng)
        request_graph(world, rng)
        world.put(v.SEED, rng.getrandbits(32))
        world.put(v.SEED + 4, rng.getrandbits(32))
        struct.pack_into('<f', world.data2, v.TIME + 4 - DATA2, rng.choice([1 / 30, 1 / 60, 0.0, 1.0]))
        world.globals[0x47c1b4] = rng.choice([0x0101, 0x0101, 0x0100, 0x0001, 0])
        world.extra[0x47c1b8] = bytearray(DEFAULT_SECONDARY)

    def random_manager(world, rng, manager):
        for channel in (0, 0x20, 0x40):
            g = rng.randrange(4)
            tag = rng.choice([NONE, graph_tag(g), graph_tag(g)])
            world.put(manager + channel, tag)
            world.put(manager + channel + 4, valid_packed(world, rng, GRAPHS + 0x800 * g) if tag != NONE else NONE)
            world.put(manager + channel + 0x18, fbits(rng.choice([1.0, 0.5, 0.0])))
            world.put(manager + channel + 0x1c, fbits(rng.choice([0.0, 2.0, 10.5, 40.0, -1.0])))
            byte(world, manager + channel + 0x11, rng.choice([0, 1, 2, 9, 0xff]))
            byte(world, manager + channel + 0x12, rng.choice([0, 1, 0xff]))
        world.put(manager + 0x68, graph_tag(0))
        byte(world, manager + 0x6c, rng.choice([0, 1, 2, 0x10, 0x13, 0xff]))
        byte(world, manager + 0x6e, rng.choice([0, 1, 0xff]))
        for offset in (0x70, 0x74, 0x78, 0x7c):
            world.put(manager + offset, rng.choice(LABELS + STATES_ + [NONE]))
        world.put(manager + 0x80, fbits(rng.choice([0.0, 0.3, 0.6, 1.0])))

    def setup_case(name, world, rng):
        manager_world(world, rng)
        record = CHANNELS + 0x100
        random_manager(world, rng, record)
        k = rng.randrange(4)
        nodes = struct.unpack_from('<I', world.data2, model(k) + 0x78 - DATA2)[0]
        g = 0                                  # only graph 0 carries mode tables
        match = rng.random() < .7
        half(world, GRAPHS + 0x800 * g + 0xc, (nodes + (0 if match else rng.choice([1, 2, -1, 0x8000]))) & 0xffff)
        graph = rng.choice([graph_tag(g)] * 5 + [NONE])
        model_tag = rng.choice([(0x5000 + 4 + k) << 16 | (4 + k)] * 4 + [NONE])
        if model_tag == NONE:
            graph = graph_tag(g)              # 1D3380 dereferences the graph without a model
        return [record, graph, model_tag, rng.choice([0, 1, 0x100])]

    kernels.append(('animation_manager_setup', 'ms_setup', 'boolean',
                    dict(entry=0x1d3380, regs={'esi': 0, 'edi': 1, 'edx': 2}, stack=[3]), setup_case, 600, True))
    # --- 1D3380 subtree, group A (2A0100, 1E3320, 1E5880, 1E58E0, 1D3660, 1CE720)
    ITER, SLOT3, TRANSITIONS = TRACK + 0xc400, TRACK + 0xc440, TRACK + 0xc500

    # --- 1D3380 subtree, group B (1CEC60, 1CE810, 1D36C0, 1E5E40/1E5F20/1E5FB0/1D6430)
    def request_graph(world, rng):
        graph = entry_graph(world, rng)
        world.put(v.TAG_TABLE + 16 * 20 + 8, graph)
        world.put(graph + 0xb0, graph + 0x80)
        for r in range(4):
            byte(world, graph + 0x80 + 20 * r + 0x10, 0 if r < 2 else 1)
        world.put(graph + 0x4c, rng.choice([0, 0, 1, 2]))
        world.put(graph + 0x50, graph + 0x40)
        for i in range(2):
            world.put(graph + 0x40 + 0x20 * i + 4, graph_tag(0))
        sorted_block(world, rng, graph + 0x54, graph + 0x600, 8, LABELS)
        for n in range(10):
            world.put(graph + 0x600 + 8 * n + 4, rng.choice(LABELS + [NONE]))
        return graph

    # --- 1D3380 subtree, group C (1E3420, 1D46A0, 1D4F80) -------------------
    STATES_ = [0x400000c, 0x1000011, 0x2000022, 0x5000033]
    LABELS_OUT = TRACK + 0xc800

    # --- 1D5050 (with the executed 1E4B70 search) ---------------------------
    DEFAULT_SECONDARY = bytes.fromhex('1400000a2105000a150000092205000916000009230500091700000a2405000a')

    # --- C06C0: D6E40 widgets and C0CD0 attachments --------------------------
    # The attachments track's world (borrowed): function rows, the looping
    # sound [0x4F9B5C], contrail [0x4F7214] and light [0x4ECBDC] arrays and
    # their definitions, so C0CD0's creators run on real rows. Widget and
    # attachment rows sit in a TRACK range that world leaves free.
    WIDGETS, ATTACHMENTS = TRACK + 0x5800, TRACK + 0x5000
    WIDGET_GROUPS = [0x616e7421, 0x636c7764, 0x7464746c, 0x12345678]
    LIGHT, SOUND, CONTRAIL, EFFECT = 0x6c696768, 0x6c736e64, 0x636f6e74, 0x65666665
    ATTACH_GROUPS = [0x6c656e73, 0x4d475332, 0x7464746c, CONTRAIL, EFFECT, LIGHT, SOUND, 0x55555555]
    # Contrail primary scales BCB70 leaves inactive on the fixture's objects:
    # an active one reaches 17EF70 (contrail reset, refused).
    INACTIVE_SCALES = [0x40005a7, 0x77770007, 0xd000558, 0x11110001]
    UNKNOWN_NAMES = [0x77770007, 0x0b000123, 0x80000001, 0x08000000, 0x17000000, 0x17000700]
    EXPORT_NAMES = [0x11110001, 0x22220002, 0x33330003, 0x44440004, 0x55550005, 0x66660006]

    def attachment_tag(rng, group):
        if group == LIGHT:
            return NONE if rng.random() < .15 else attached['light_tag'](rng)
        if group == SOUND:
            return rng.choice([NONE, rng.getrandbits(32), (0x7300 << 16) | 3, (0x7301 << 16) | 2])
        if group == CONTRAIL:
            c = rng.randrange(4)
            return NONE if rng.random() < .15 else (0x7100 + c) << 16 | (attached['contrail_row'] + c)
        if group == EFFECT:
            return NONE                                 # 179540 refuses at 178E30
        return NONE if rng.random() < .3 else rng.getrandbits(32)

    def finish_world(world, rng):
        attached['function_world'](world, rng, rng.random() < .5)
        attached['data_array'](world, rng, *attached['sounds'], 0x18, 0x4f9b5c)
        attached['data_array'](world, rng, *attached['contrails'], 0x48, 0x4f7214)
        attached['contrail_definitions'](world, rng)
        attached['data_array'](world, rng, *attached['lights'], 0x110, 0x4ecbdc)
        attached['light_definitions'](world, rng)
        for k in range(4):
            world.put(definition(k) + 0x9c, rng.choice([0, 0, 1, 2, 3, NONE]))
            world.put(definition(k) + 0xa0, WIDGETS + 0x40 * k)
            world.put(definition(k) + 0x94, rng.choice([0, 1, 2, 3, 4, 4, NONE]))
            world.put(definition(k) + 0x98, ATTACHMENTS + 0x100 * k)
            for w in range(4):
                group = rng.choice(WIDGET_GROUPS)
                world.put(WIDGETS + 0x40 * k + 8 * w, group)
                # Known widget types keep a NONE tag: their creators are refused.
                world.put(WIDGETS + 0x40 * k + 8 * w + 4, NONE if group != 0x12345678 or rng.random() < .3 else rng.getrandbits(32))
            for a in range(4):
                row = ATTACHMENTS + 0x100 * k + 0x18 * a
                group = rng.choice(ATTACH_GROUPS)
                world.put(row, group)
                world.put(row + 4, attachment_tag(rng, group))
                world.put(row + 8, rng.getrandbits(32))
                world.put(row + 0xc, rng.choice([0, 1, 2, 3, 0xffff, 0x7fff, rng.getrandbits(16)]) | (rng.getrandbits(16) << 16))
                if group == CONTRAIL:
                    scale = rng.choice(INACTIVE_SCALES)
                else:
                    scale = rng.choice(attached['safe_names'] + EXPORT_NAMES + UNKNOWN_NAMES + [0, 0])
                world.put(row + 0x10, scale)
                world.put(row + 0x14, rng.choice([0, NONE, attached['special'](rng)]))
        for n in range(ROWS - 1):
            half(world, world.body(n) + 0x11e, 0x2c0)

    def contrail_safe(world, rng, n):
        """An object with a live contrail row keeps every contrail scale
        inactive (as the attachments track's 17E940 cases): no function
        variable override (+C0 bit 9), no parent (+4 bit 26), no definition
        functions (+64) and a non-positive 0xD000558 value (+EC)."""
        body = world.body(n)
        k = struct.unpack_from('<I', world.data, body - DATA)[0] & 3
        count = struct.unpack_from('<i', world.data3, definition(k) + 0x94 - DATA3)[0]
        rows = [ATTACHMENTS + 0x100 * k + 0x18 * a for a in range(max(0, min(count, 4)))]
        word = lambda at: struct.unpack_from('<I', world.track, at - TRACK)[0]
        if not any(word(row) == CONTRAIL and word(row + 4) != NONE for row in rows):
            return
        half(world, body + 0xc0, 0)
        world.put(body + 4, struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~(1 << 26))
        world.put(definition(k) + 0x64, 0)
        world.put(body + 0xec, fbits(rng.choice([0.0, -1.0, -0.0, -1e-40, -2.5])))

    def finish_case(name, world, rng):
        finish_world(world, rng)
        n = object_index(rng)
        contrail_safe(world, rng, n)
        if rng.random() < .4:
            byte(world, v.RECORDS + 12 * n + 3, 5)        # a projectile header (C06C0 skips D6E40)
        return [world.handles[n]]

    kernels += [
        ('object_widgets_create', 'ms_widgets', 'void', dict(entry=0xd6e40, stack=[0]), finish_case, 300, True),
        ('object_attachments_create', 'ms_attachments', 'void', dict(entry=0xc0cd0, stack=[0]), finish_case, 800, True),
        ('object_type_finish', 'ms_type_finish', 'void', dict(entry=0xc06c0, regs={'esi': 0}), finish_case, 600, True),
    ]
    for name, argument, kind, routine, case, count, needs_stops in kernels:
        if needs_stops and not staged:
            continue
        v.KERNELS[name] = ([argument], kind)
        v.ROUTINES[name] = routine
        v.TRACK_ARGUMENTS[argument] = case
        v.TRACK_COUNTS[name] = count
