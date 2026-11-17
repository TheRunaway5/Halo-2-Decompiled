"""Oracle cases for factory track "attachments" (C0CD0's attachment creators).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Owners: 18BD80 (looping sound), 17E940/17EE10 (contrail) and the object
function value subtree BCB70 (10FB70, 10DE40 with the object callback BEC50,
BC8F0, 13E670 over the existing native 13DE70).

Closures: every kernel declares closure_stops for its refused callees (the
type callbacks other than BEC50, BEC50's unported name callees, 17EF70, the
13DE70 power/control-point owners and the CRT error method). 10DE40's CALL ECX
is admitted (UNREACHED_VIRTUAL_OWNERS) and BEC50's closure joins the
authenticated sites through a metadata wrapper (INDIRECT), like the placement
track's 10DDE0 machine callback.
"""
import struct

NONE = 0xffffffff
# Refused callees (named refusals in the native) and unported subtrees.
# C9070 (unit), E0900 (biped) and 1046C0 (weapon) are native (2026-11-14);
# 1655F0's 24B430 (mode 9) and 1E5AF0's 126200 (resource admission, reached
# through 1CF0E0's 1E3150) are refused.
# F9270 (vehicle) too; its F9E40 kind-2 limit stays in its closure (cases
# avoid those names), FA5D0 and 11FF40 are refused.
# 100E70 (projectile), 10BE20 (device), 11E490 (machine/crate, with 1DB800),
# 24B430, FA5D0, 11FF40 and F9E40's kind 2 are native since 2026-11-15.
TYPE_CALLBACKS = {0x126200}
# BEC50's callees are native since 2026-11-16 (C3910, then C1F90, BEAA0,
# 15B2D0, BE9A0 and the console clock's BEBE0/BEB00/BEB40/BEB90). The clock
# reads GetLocalTime (13BB10 -> 33BA6B, kernel imports the observer cannot
# run): the clock names stay out of BEC50's cases, and the clock kernels
# enter each owner after its 13BB10 call (CLOCK_KERNELS).
BASE_CALLEES = {0x13bb10, 0x13bb70}
FUNCTION_STOPS = {0x37cde8, 0x13e9a0, 0x3334f0}     # 13DE70 power, control points; CRT error
# Light owners: C4D30 (placement's light_reconnect) and C5250 (light detach)
# are native but need a light partition the cases do not build; 13E6E0 (light
# color function) and 134730's HSV callees are refused.
LIGHT_STOPS = {0xc4d30, 0xc5250, 0x1343e0, 0x134510}   # 13E6E0 (light colour function) native since 2026-11-16
STOPS = sorted(TYPE_CALLBACKS | BASE_CALLEES | FUNCTION_STOPS | LIGHT_STOPS | {0x17ef70})
INDIRECT_TARGETS = [0xbec50, 0xc9070, 0xe0900, 0x1046c0, 0xf9270, 0x100e70, 0x10be20, 0x11e490]
FMOD_METHODS = (0x37dc14, 0x331f68)
# Names BEC50 answers natively; RAW ones are published without the clamp.
BASE_NAMES = [0x500055b, 0x30005a6, 0xa0006b9, 0x100006b4, 0x40005a7, 0x700055c, 0x700055d, 0xd000558, 0xf000559,
              0x15000557, 0x13000556, 0x1800055a, 0x1a00071a, 0x1a00071b, 0x1a00071c, 0x1a00071d, 0xf0005aa, 0xf0005ac,
              0x130006bb]
# BEC50's names over BEAA0 ([0x4EABC4] +14) and BE9A0 (the object's +24..+26
# bytes); BEC50's own and 10DE40's cases draw them (their fixtures declare
# those inputs).
CALLEE_NAMES = [0x90006b5, 0xd0005ff, 0x170005fa, 0x170005fb, 0x180005fc, 0x1d0005fd, 0x1d0005fe]
EXPORTS = [0x11110001, 0x22220002, 0x33330003, 0x44440004, 0x55550005, 0x66660006]
# Unknown names, including ones inside each BEC50 comparison range.
UNKNOWN = [0x77770007, 0x0b000123, 0x80000001, 0x08000000, 0x17000000, 0x17000700]
SIMPLE_TYPES = [3, 4, 6, 10, 12]     # chains without a type-level +4C callback
LITERALS = {0x46a0e8: 0x00000000, 0x46a160: 0x37800080, 0x46a0f8: 0x3f7eb852, 0x46a0d0: 0x3f800000,
            0x46a0ec: 0x38d1b717, 0x46a11c: 0x40000000, 0x46a120: 0x40800000, 0x46a0c4: 0x3b808081,
            0x46a178: 0x3dcccccd, 0x46a164: 0x3c8efa35, 0x46a180: 0x3a83126f, 0x46a0cc: 0x3f000000,
            0x44a8a0: 0x3e19999a, 0x46a16c: 0x3e4ccccd, 0x46a194: 0x3f666666,
            0x46a614: 0x3e22f983, 0x46a0dc: 0xbf800000, 0x46a0e4: 0x40400000, 0x46a130: 0x3e800000,
            0x46a13c: 0x3f400000, 0x46a108: 0x41700000, 0x46a118: 0x41a00000, 0x46a200: 0x3daaaaab,
            0x46a1fc: 0x3c888889, 0x46a384: 0x3c800000, 0x46a388: 0x3d000000}
# Jump tables of BEBE0 (holidays 1..4) and 15B2D0 (the player row's +C1
# index), against the native switches.
SWITCHES = {0xbec3c: [0xbebfb, 0xbec0a, 0xbec19, 0xbec28], 0x15b39c: [0x15b397, 0x15b377, 0x15b383, 0x15b38f]}
# Image blocks the native 13DE70/17F790/17F8A0 read: literals and curve tables.
IMAGE_BLOCKS = [(0x46a0c0, 0x4e0), (0x4752a8, 0x5000)]


def fbits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def check_literals(v, image):
    from pathlib import Path
    source = (Path(__file__).resolve().parents[1] / 'src/gameplay/objects/native_object_factory_attachments_tu15.cpp').read_text()
    for address, value in LITERALS.items():
        if struct.unpack('<I', image.read(address, 4))[0] != value:
            raise ValueError(f'TU1.5 literal 0x{address:X} changed')
        if address != 0x46a0d0 and f'=0x{value:08x}u;' not in source:
            raise ValueError(f'Native literal for 0x{address:X} is missing')
    for table, targets in SWITCHES.items():
        if list(struct.unpack(f'<{len(targets)}I', image.read(table, 4 * len(targets)))) != targets:
            raise ValueError(f'TU1.5 jump table 0x{table:X} changed')


def _indirect_metadata(v):
    """Admit BEC50's closure (10DE40's CALL ECX target) for this track's
    kernels, with this track's stops."""
    original = v.metadata

    def metadata(image, evaluators):
        meta, code = original(image, evaluators)
        if not set(evaluators) & set(ROUTINE_NAMES):
            return meta, code
        sites = {s['instruction']: s for s in meta['sites']}
        for target in INDIRECT_TARGETS:
            key = f'attachments@{target:x}'
            v.ROUTINES[key] = dict(entry=target, closure_stops=STOPS)
            try:
                part, _ = original(image, {key: target})
            finally:
                del v.ROUTINES[key]
            for site in part['sites']:
                sites.setdefault(site['instruction'], site)
        meta['sites'] = list(sites.values())
        return meta, code
    v.metadata = metadata


ROUTINE_NAMES = []
# Fixture builders and layout published by register() for composed kernels
# of other tracks (modelstate's C0CD0/C06C0/C0910), which borrow them through
# tools/native_object_factory_track_fixtures.py.
FIXTURES = {}


def register(v):
    image = v.Xbe(v.NEW_DEFAULT)
    check_literals(v, image)
    blocks = {address: bytes(image.read(address, size)) for address, size in IMAGE_BLOCKS}
    TRACK, DATA, DATA2, DATA3 = v.TRACK, v.DATA, v.DATA2, v.DATA3
    v.UNREACHED_VIRTUAL_OWNERS.add(0x10de40)   # type callbacks: only BEC50, authenticated via INDIRECT
    v.JUMP_TABLES[0x13e5f4] = 11               # 13DE70's function kinds
    v.JUMP_TABLES[0x13e948] = 5                # 13E6E0's colour kinds
    v.JUMP_TABLES[0xf9f9c] = 9                 # F9E40's limit kinds (F9270 passes 0 and 2)
    v.JUMP_TABLES[0xbec3c] = 4                 # BEBE0's holidays 1..4
    v.JUMP_TABLES[0x15b39c] = 4                # 15B2D0's player row +C1 index
    v.REGISTER_JUMPS[0x331f55] = FMOD_METHODS  # 37DC0A's FPREM/FLDZ methods (as modelstate)
    _indirect_metadata(v)

    # Track bank layout.
    SOUNDS, SOUND_BITS, SOUND_ROWS = TRACK, TRACK + 0x100, TRACK + 0x200            # 0x18 rows
    CONTRAILS, CONTRAIL_BITS, CONTRAIL_ROWS = TRACK + 0x400, TRACK + 0x480, TRACK + 0x500  # 0x48 rows
    OUT = TRACK + 0x1000
    FUNCTION_ROWS = TRACK + 0x2000      # definition +68 rows (0x20), 0x100 per definition
    BLOBS = TRACK + 0x2800              # 13DE70 function data, 0x100 each
    REFERENCES = TRACK + 0x3800         # 8-byte function references
    CONTRAIL_DEFS, PROFILES, PROFILE_ROWS = TRACK + 0x4000, TRACK + 0x4400, TRACK + 0x4800
    ATTACHMENTS = TRACK + 0x5000        # definition +98 rows (0x18)
    GLOBALS_BLOCK = TRACK + 0x7000      # [0x4EABC4]
    MATRICES = TRACK + 0x7400
    LIGHT_STATE = TRACK + 0xe800        # [0x51DAE8]
    PLAYER_ROWS = TRACK + 0x9400        # [0x4F54F4] +44 (0x21C rows; 15B2D0)
    PLAYER_BITS = TRACK + 0x9880        # [0x51DF9C]
    CONTRAIL_ROW, PROFILE_ROW = 40, 44  # tag rows

    def byte(world, address, value):
        for base, bank in ((TRACK, world.track), (DATA, world.data), (DATA2, world.data2), (DATA3, world.data3)):
            if bank is not None and base <= address < base + len(bank):
                bank[address - base] = value & 0xff
                return
        raise ValueError(f'byte outside the declared banks {address:x}')

    def half(world, address, value):
        byte(world, address, value)
        byte(world, address + 1, value >> 8)

    def fresh(world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        for address, data in blocks.items():
            world.extra[address] = bytearray(data)
        # C3910 (BEC50 0x130006BB): [0x51DAE8] +0 lights on, +4 the
        # transition's start tick, around the game tick.
        world.globals[0x51dae8] = LIGHT_STATE
        tick = struct.unpack_from('<I', world.data2, v.TIME + 8 - DATA2)[0]
        world.track[LIGHT_STATE - TRACK] = rng.choice([0, 1, 1, 0x80])
        world.put(LIGHT_STATE + 4, (tick - rng.choice([0, 1, 5, 15, 29, 30, 31, 45, 60, 1000, -3])) & NONE)
        # BEC50's callees: C1F90's pi and 2 pi (set at startup), the
        # scenario's +10 word (BE9A0) and north angle (+1C), 15B2D0's game
        # globals (+180 engine, +1C4 team mode, +22C flags), player bits and
        # player rows (+C1 index).
        world.absolute[0x5564c4] = bytearray(struct.pack('<2f', *rng.choice(
            [(2 * 3.14159265, 3.14159265), (6.2831855, 3.1415927), (rng.uniform(1, 8), rng.uniform(0.5, 4))])))
        half(world, v.SCENARIO + 0x10, rng.choice([0, 0, 1, 0x100]))
        world.put(v.SCENARIO + 0x1c, fbits(rng.choice([0.0, 1.5707964, -3.1, rng.uniform(-4, 4)])))
        world.put(v.PVS + 0x180, rng.choice([7, 7, 0, 3]))
        world.put(v.PVS + 0x1c4, rng.choice([0, 1, 2, 3, 0xffffffff]))
        byte(world, v.PVS + 0x22c, rng.choice([0, 2, 0xfd, 0xff]))
        world.globals[0x51df9c] = PLAYER_BITS
        half(world, PLAYER_BITS, rng.choice([0, 1, 2, 3, 0xffff, rng.getrandbits(16)]))
        world.put(v.DATA3 + 0x2400 + 0x44, PLAYER_ROWS)
        for player in range(2):
            byte(world, PLAYER_ROWS + 0x21c * player + 0xc1, rng.choice([0, 1, 2, 3, 4, 0xff, 0x80]))
        world.put(GLOBALS_BLOCK + 0x14, rng.choice([0, 0, (tick - rng.choice([1, 15, 29, 30, 31, 600])) & NONE,
                                                    (tick + 5) & NONE, tick]))

    def tag_row(world, row, data, salt):
        world.put(v.TAG_TABLE + 16 * row + 8, data)
        return (salt << 16) | row

    def special(rng, finite=True):
        values = [0.0, -0.0, 1.0, 0.5, 0.25, 2.0, -0.5, 1e-40, -1e-40, 0.999, 1.0000001, 3.5, -3.0]
        if not finite:
            values += [float('nan'), float('inf'), float('-inf')]
        return fbits(rng.choice(values)) if rng.random() < .4 else fbits(rng.uniform(-1.5, 1.5))

    def data_array(world, rng, header, bits, rows, size, globals_address):
        world.globals[globals_address] = header
        maximum = rng.choice([4, 8, 16, 16])
        cursor = rng.randrange(0, maximum + 1)
        world.put(header + 0x20, rng.choice([maximum, maximum, 0]))
        world.put(header + 0x24, size)
        world.put(header + 0x34, cursor)
        world.put(header + 0x38, rng.choice([cursor, rng.randrange(cursor, maximum + 1), maximum]))
        world.put(header + 0x3c, rng.randrange(0, maximum + 1))
        half(world, header + 0x40, rng.choice([0x8000, 0x8123, 0xfffe, 0xffff]))
        world.put(header + 0x44, rows)
        world.put(header + 0x48, bits)
        world.put(bits, rng.choice([0, 0xffff, 0x7fff, 0xfffe, rng.getrandbits(32)]))

    # --- objects: definitions, models, types ------------------------------
    def objects(world, rng, simple):
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body, (rng.getrandbits(16) << 16) | rng.randrange(4))
            types = SIMPLE_TYPES if simple else SIMPLE_TYPES + list(range(13))
            byte(world, body + 0xaa, rng.choice(types))
            half(world, body + 0xc0, rng.choice([0, 0x200, 0xfdff, 0xffff]))
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0]
            world.put(body + 4, flags ^ ((1 << 26) if rng.random() < .5 else 0))
        for k in range(4):
            definition = v.OBJECT_TAGS + 0x200 * k
            world.put(definition + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
            world.put(v.MODEL_TAGS + 0x80 * k + 0x50, rng.choice([0, 1, 3, 8, NONE]))

    def body_values(world, rng, finite):
        world.put(0x4eabc4, GLOBALS_BLOCK)
        for k in range(4):
            world.put(GLOBALS_BLOCK + 0x70 + 4 * k, special(rng, finite))
        world.put(v.PVS + 8, rng.choice([2, 2, 0, 1]))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            byte(world, body + 0x10a, rng.choice([0, 4, 0xfb, 0xff]))
            byte(world, body + 0xb1, rng.choice([0, 1, 2, 7, 0x7f, 0x80, 0xfe, 0xff]))
            for offset in (0xec, 0xf0, 0xf4, 0xf8):
                world.put(body + offset, special(rng, finite))
            world.put(body + 0x13c, rng.choice([NONE, NONE, 0, 1, world.handles[0]]))
            # Node 0 for 0x700055C: its forward axis (+4, +8 and +C, the
            # upright test at 0.995).
            half(world, body + 0x116, 0x380)
            world.put(body + 0x380 + 0xc, fbits(rng.choice([1.0, -1.0, 0.995, -0.9951, 2.5, float('nan'), -7.0,
                                                             0.0, 0.9949, -0.5, 0.3, 0.0])))
            for offset in (4, 8):
                world.put(body + 0x380 + offset, fbits(rng.choice([0.0, -0.0, 1.0, -1.0, rng.uniform(-1, 1),
                                                                   rng.uniform(-1, 1), 1e-40])))
            # BE9A0's bytes: +24, +25 and the +26 flags.
            byte(world, body + 0x24, rng.getrandbits(8))
            byte(world, body + 0x25, rng.getrandbits(8))
            byte(world, body + 0x26, rng.choice([0, 1, 2, 4, 8, 9, 0xf, 0xf7, rng.getrandbits(8)]))

    def safe_type(world, n):
        """Objects reached with a 0xF0005AC name must keep 15B2D0 unreached."""
        body = world.body(n)
        mode = struct.unpack_from('<I', world.data2, v.PVS + 8 - DATA2)[0]
        kind = world.data[body + 0xaa - DATA] & 31
        handle = struct.unpack_from('<I', world.data, body + 0x13c - DATA)[0]
        return not (mode == 2 and kind in (0, 1) and handle != NONE)

    def fix_type(world, n):
        if not safe_type(world, n):
            world.put(world.body(n) + 0x13c, NONE)

    # --- 13DE70 functions --------------------------------------------------
    def function_blob(world, rng, at, kinds=(0, 0, 1, 1, 2, 2, 3, 3, 4, 5, 6, 7, 8)):
        for offset in range(0, 0x100, 4):
            world.put(at + offset, special(rng))
        kind = rng.choice(kinds)
        byte(world, at, kind)
        byte(world, at + 1, rng.choice([0, 1, 0x10, 0x11, 0xf0, 0x21, 0x80, 0x81]))
        byte(world, at + 2, rng.randrange(8) if kind == 2 else rng.randrange(12) if kind == 3 else rng.getrandbits(8))
        byte(world, at + 3, rng.randrange(8) if kind == 2 else rng.randrange(12) if kind == 3 else rng.getrandbits(8))
        return kind

    def reference(world, rng, at, blob, kinds=None):
        kind = function_blob(world, rng, blob, kinds) if kinds else function_blob(world, rng, blob)
        world.put(at, rng.choice([1, 1, 1, 0x40, 0, NONE]))
        world.put(at + 4, rng.choice([blob] * 6 + [0]))
        return kind

    # --- definition functions (+64/+68) -----------------------------------
    def function_rows(world, rng, simple_names):
        """Each definition exports EXPORTS[i] from rows whose scale-by and
        turn-off names only refer to later exports (no recursion cycle)."""
        names = BASE_NAMES if not simple_names else [0x40005a7, 0x30005a6, 0xd000558, 0xf0005aa, 0x700055d]
        for k in range(4):
            definition = v.OBJECT_TAGS + 0x200 * k
            count = rng.choice([0, 1, 2, 3, 5, 8, NONE])
            world.put(definition + 0x64, count)
            world.put(definition + 0x68, FUNCTION_ROWS + 0x100 * k)
            for r in range(8):
                row = FUNCTION_ROWS + 0x100 * k + 0x20 * r
                export = rng.randrange(len(EXPORTS))
                world.put(row, rng.choice([0, 1, 2, 3, 4, 5, 8, 0xa, 0xf, rng.getrandbits(32)]))
                world.put(row + 4, rng.choice(names + EXPORTS[export + 1:] + UNKNOWN + [0]))
                world.put(row + 8, EXPORTS[export])
                world.put(row + 0xc, rng.choice([0, 0, 0] + names + EXPORTS[export + 1:] + UNKNOWN))
                world.put(row + 0x10, fbits(rng.choice([0.0, 0.0, 0.5, -1.0, 0.25, 1e-40, 1.0])))
                blob = BLOBS + 0x100 * ((k * 8 + r) % 16)
                world.put(row + 0x14, rng.choice([1, 1, 0, NONE]))
                world.put(row + 0x18, blob)
                function_blob(world, rng, blob)
                world.put(row + 0x1c, rng.choice([0, 0, 0] + names + EXPORTS[export + 1:] + UNKNOWN))

    def function_world(world, rng, simple_names=False):
        fresh(world, rng)
        objects(world, rng, True)
        body_values(world, rng, True)
        function_rows(world, rng, simple_names)
        for n in range(v.ROWS - 1):
            fix_type(world, n)

    def name_choice(rng):
        return rng.choice(BASE_NAMES + EXPORTS + EXPORTS + UNKNOWN + [0, 0])

    # --- 18BD80 -------------------------------------------------------------
    def sound_case(name, world, rng):
        fresh(world, rng)
        data_array(world, rng, SOUNDS, SOUND_BITS, SOUND_ROWS, 0x18, 0x4f9b5c)
        tag = rng.choice([NONE, rng.getrandbits(32), (0x7300 << 16) | 3])
        target = rng.choice(world.handles[:v.ROWS - 1] + [NONE, NONE])
        return [tag, target, rng.getrandbits(32), rng.choice([0, NONE, rng.getrandbits(32)])]

    # --- 10FB70 -------------------------------------------------------------
    def variable_case(name, world, rng):
        fresh(world, rng)
        target = rng.choice(world.handles[:3] + [NONE, world.handles[5]])
        return [target, rng.choice([0x1000a, 0x20003, 0, 0x1000a, 0x5555]), OUT, OUT + 4]

    # --- BEC50 / 10DE40 -----------------------------------------------------
    def base_case(name, world, rng):
        fresh(world, rng)
        objects(world, rng, False)
        body_values(world, rng, False)
        n = rng.randrange(v.ROWS - 1)
        label = rng.choice(BASE_NAMES + BASE_NAMES + CALLEE_NAMES + CALLEE_NAMES + UNKNOWN + [0])
        if label == 0xf0005ac:
            if rng.random() < .3:
                fix_type(world, n)
            elif rng.random() < .8:
                # A player's unit or biped in game mode 2: 15B2D0's scale.
                # Players 16 and 17 test the mask's high bits (a word); a
                # zero scale (team mode 1) against an +F0 of zero.
                world.put(v.PVS + 8, 2)
                byte(world, world.body(n) + 0xaa, rng.choice([0, 1, 0x20, 0x21]))
                world.put(world.body(n) + 0x13c, rng.choice([0, 1, 0x10000, 0xffff0001, 16, 17, 0x10011]))
                world.put(PLAYER_BITS, rng.getrandbits(32))
                if rng.random() < .4:
                    world.put(v.PVS + 0x1c4, 1)
                    world.put(world.body(n) + 0xf0, fbits(rng.choice([0.0, -0.0])))
            if rng.random() < .5:
                # A denormal is zero only under MXCSR.DAZ (UCOMISS).
                world.put(world.body(n) + 0xf0, fbits(rng.choice([1e-40, -1e-40, 1e-45])))
        if label in CALLEE_NAMES and rng.random() < .2:
            world.globals[0x4ecc20] = 0      # BE9A0 without a scenario
        if label == 0x700055c and rng.random() < .35:
            # A heading of exactly +-pi (atan2(+-0, -1)) against a zero north
            # and exact pi: C1F90's two wraps at their ties.
            matrix = world.body(n) + 0x380
            world.put(matrix + 4, fbits(rng.choice([0.0, -0.0])))
            world.put(matrix + 8, fbits(-1.0))
            world.put(matrix + 0xc, fbits(rng.choice([0.0, 0.5])))
            world.put(v.SCENARIO + 0x1c, 0)
            world.absolute[0x5564c4] = bytearray(struct.pack('<2f', 6.2831855, 3.1415927))
        return [world.handles[n], label, OUT, OUT + 4]

    def type_case(name, world, rng):
        fresh(world, rng)
        objects(world, rng, False)
        body_values(world, rng, False)
        n = rng.randrange(v.ROWS - 1)
        label = rng.choice(BASE_NAMES + CALLEE_NAMES + UNKNOWN + [0])
        if label not in BASE_NAMES + CALLEE_NAMES:
            # Unknown names reach the type-level callbacks: simple types only.
            byte(world, world.body(n) + 0xaa, rng.choice(SIMPLE_TYPES))
        if label == 0xf0005ac:
            fix_type(world, n)
        return [world.handles[n], label, OUT, OUT + 4]

    # --- 13E670 -------------------------------------------------------------
    def reference_case(name, world, rng):
        fresh(world, rng)
        kind = reference(world, rng, REFERENCES, BLOBS)
        if kind in (2, 3):
            x = fbits(rng.choice([0.0, 0.5, 1.0, -0.25, 2.0, 1e-40, 0.999, 123.5, -7.25]) if rng.random() < .4
                      else rng.uniform(-2, 2))
        else:
            x = special(rng, False)
        return [REFERENCES, x, rng.choice([fbits(1.0), special(rng, False)])]

    # --- BC8F0 / BCB70 ------------------------------------------------------
    def evaluate_case(name, world, rng):
        function_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        k = struct.unpack_from('<I', world.data, world.body(n) - DATA)[0] & 3
        row = FUNCTION_ROWS + 0x100 * k + 0x20 * rng.randrange(8)
        if rng.random() < .35:
            # A periodic function with an active-or-denormal import: the
            # periodic path runs regardless of the import's active byte
            # (a DAZ denormal is inactive), with the optional handle draw.
            world.put(row, rng.choice([0x6, 0xe, 0xe, 0x4]))
            world.put(row + 4, 0xd000558)
            world.put(row + 0xc, 0)
            world.put(row + 0x1c, 0)
            world.put(row + 0x14, 1)
            half(world, world.body(n) + 0xc0, 0)
            world.put(world.body(n) + 0xec, fbits(rng.choice([1e-40, 1e-40, 0.5, 0.25])))
            blob = struct.unpack_from('<I', world.track, row + 0x18 - TRACK)[0]
            function_blob(world, rng, blob, (3,))
            byte(world, blob + 2, rng.randrange(1, 12))
            world.put(blob + 0x14, fbits(rng.uniform(0.5, 2)))
        return [row, world.handles[n], OUT]

    def value_case(name, world, rng):
        function_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        return [world.handles[n], name_choice(rng), OUT]

    # --- 17EE10 / 17E940 ----------------------------------------------------
    def contrail_definitions(world, rng):
        for c in range(4):
            definition = CONTRAIL_DEFS + 0x40 * c
            tag_row(world, CONTRAIL_ROW + c, definition, 0x7100 + c)
            profiles = PROFILES + 0x40 * (c & 1)
            world.put(definition + 0x34, rng.choice([NONE, (0x7200 + c) << 16 | (PROFILE_ROW + (c & 1))]))
            half(world, definition + 0x38, rng.choice([0, 1, 3, 0x7fff, 0xfffe, rng.getrandbits(16)]))
            half(world, definition + 0x3a, rng.choice([0, 1, 4, 0x8000, 0xffff, rng.getrandbits(16)]))
        for p in range(2):
            profiles = PROFILES + 0x40 * p
            tag_row(world, PROFILE_ROW + p, profiles, 0x7300 + p)
            world.put(profiles + 0x3c, rng.choice([0, 1, 3, 6, NONE]))
            world.put(profiles + 0x40, PROFILE_ROWS + 0x200 * p)
            for r in range(6):
                half(world, PROFILE_ROWS + 0x200 * p + 0x3c * r + 0x22, rng.choice([0, 1, 2, 5, 0x8000, 0x7fff]))

    def profile_case(name, world, rng):
        fresh(world, rng)
        contrail_definitions(world, rng)
        row = CONTRAIL_ROWS + 0x48 * rng.randrange(4)
        world.put(row + 4, (0x7100 << 16) | (CONTRAIL_ROW + rng.randrange(4)))
        half(world, row + 0x18, rng.choice([0, 1, 2, 5, 0xffff, 0x8000, 0x7fff]))
        half(world, row + 0x1a, rng.choice([0, 1, 2, 4, 0xffff, 0x7fff, 0x8000]))
        return [row]

    def contrail_case(name, world, rng):
        function_world(world, rng, True)
        data_array(world, rng, CONTRAILS, CONTRAIL_BITS, CONTRAIL_ROWS, 0x48, 0x4f7214)
        contrail_definitions(world, rng)
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        k = struct.unpack_from('<I', world.data, body - DATA)[0] & 3
        world.put(v.OBJECT_TAGS + 0x200 * k + 0x98, ATTACHMENTS)
        # The primary scale must stay inactive: an active one reaches 17EF70.
        half(world, body + 0xc0, 0)
        world.put(world.body(n) + 4, struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~(1 << 26))
        world.put(v.OBJECT_TAGS + 0x200 * k + 0x64, 0)
        index = rng.randrange(4)
        for a in range(4):
            world.put(ATTACHMENTS + 0x18 * a + 0x10, rng.choice([0x40005a7, 0x77770007, 0xd000558, 0x11110001]))
        for offset in (0xec,):
            world.put(body + offset, fbits(rng.choice([0.0, -1.0, -0.0, -1e-40, -2.5])))
        tag = rng.choice([NONE] + [(0x7100 + c) << 16 | (CONTRAIL_ROW + c) for c in range(4)] * 3)
        return [tag, world.handles[n], index | (rng.getrandbits(16) << 16 if rng.random() < .3 else 0)]

    stops = dict(closure_stops=STOPS)
    kernels = [
        ('object_looping_sound_attachment_create', 'at_sound', 'long',
         dict(entry=0x18bd80, regs={'esi': 0, 'edi': 1}, stack=[2, 3]), sound_case, 600),
        ('object_function_variable', 'at_variable', 'boolean',
         dict(entry=0x10fb70, regs={'ebx': 0}, stack=[1, 2, 3]), variable_case, 600),
        ('object_base_function_value', 'at_base', 'boolean',
         dict(entry=0xbec50, stack=[0, 1, 2, 3], **stops), base_case, 1200),
        ('object_type_function_value', 'at_type', 'boolean',
         dict(entry=0x10de40, regs={'ebx': 2, 'edi': 3}, stack=[0, 1], **stops), type_case, 1000),
        ('function_reference_value', 'at_reference', 'xmm0',
         dict(entry=0x13e670, regs={'eax': 0}, stack=[1, 2], **stops), reference_case, 1200),
        ('object_function_evaluate', 'at_evaluate', 'boolean',
         dict(entry=0xbc8f0, regs={'eax': 0}, stack=[1, 2], **stops), evaluate_case, 1500),
        ('object_function_value', 'at_value', 'boolean',
         dict(entry=0xbcb70, stack=[0, 1, 2], **stops), value_case, 1500),
        ('contrail_profile_advance', 'at_profile', 'void',
         dict(entry=0x17ee10, regs={'eax': 0}), profile_case, 500),
        ('object_contrail_attachment_create', 'at_contrail', 'long',
         dict(entry=0x17e940, regs={'ebx': 1, 'edi': 2}, stack=[0], **stops), contrail_case, 800),
    ]

    # --- Lights: C2260, C2CB0, C2B30, 134870 ---------------------------------
    LIGHTS, LIGHT_BITS, LIGHT_ROWS = TRACK + 0x8000, TRACK + 0x8080, TRACK + 0x8100   # 0x110 rows
    LIGHT_DEFS, LIGHT_REFS, DEFAULT_COLOR = TRACK + 0xa000, TRACK + 0xa400, TRACK + 0xa800   # rows end at 0x9200
    BLEND = TRACK + 0xa900
    LIGHT_ROW = 48
    SAFE_NAMES = [n for n in BASE_NAMES if n != 0x700055c] + [0x700055c]

    def light_definitions(world, rng):
        for d in range(4):
            definition = LIGHT_DEFS + 0x100 * d
            tag_row(world, LIGHT_ROW + d, definition, 0x7400 + d)
            for offset in range(0, 0xc0, 4):
                world.put(definition + offset, special(rng))
            byte(world, definition, rng.choice([0, 1, 0x80, 0x81, 0x7e]))
            world.put(definition + 0x34, rng.choice([0, 2, 0xfffffffe, 4]))     # bit 0 (HSV) is refused
            world.put(definition + 0x94, rng.choice([NONE, 0, 0x12340005]))
            half(world, definition + 0xa4, rng.choice([0, 1, 2, 2, 3]))
            world.put(definition + 0xb0, fbits(rng.choice([1.0, 0.5, 2.0, -1.5, 0.0333])))
            half(world, definition + 0xb6, rng.randrange(8))
            # +C0 function block: flags, scale (count, references), color
            # (count: 13E6E0 is refused), pair (count, references).
            functions = definition + 0xc0
            world.put(functions, rng.choice([0, 1, 2, 3]))
            world.put(functions + 4, rng.choice([0, 1, 1, NONE]))
            world.put(functions + 8, LIGHT_REFS + 0x40 * d)
            world.put(functions + 0xc, rng.choice([0, NONE, 1, 1]))
            # +10: the colour function's reference: a function blob whose +1
            # high nibble is the colour kind (0 grey .. 4 four stops, 5+
            # none) with its colours at +4.
            color_ref = LIGHT_REFS + 0x40 * d + 0x20
            world.put(functions + 0x10, color_ref)
            world.put(color_ref, rng.choice([1, 1, 1, 0, NONE]))
            color_blob = BLOBS + 0x100 * rng.randrange(16)
            world.put(color_ref + 4, rng.choice([color_blob, color_blob, color_blob, 0]))
            function_blob(world, rng, color_blob)
            byte(world, color_blob + 1, rng.choice([0x00, 0x01, 0x10, 0x11, 0x20, 0x21, 0x30, 0x31, 0x40, 0x41,
                                                    0x50, 0xf1]))
            world.put(functions + 0x14, rng.choice([0, 1, 2, NONE]))
            world.put(functions + 0x18, LIGHT_REFS + 0x40 * d + 0x10)
            for r in range(3):
                at = LIGHT_REFS + 0x40 * d + 0x10 * (r and 1) + 8 * max(r - 1, 0)
                world.put(at, rng.choice([1, 1, 0, NONE]))
                blob = BLOBS + 0x100 * rng.randrange(16)
                world.put(at + 4, blob)
                function_blob(world, rng, blob)
        world.globals[0x474c70] = DEFAULT_COLOR
        for k in range(3):
            world.put(DEFAULT_COLOR + 4 * k, special(rng))
        world.globals[0x4ecbd8] = rng.getrandbits(32)
        world.put(GLOBALS_BLOCK, rng.getrandbits(32) & ~0xff)   # [[0x4EABC4]] 0: C4D30 stays unreached
        half(world, v.TIME + 2, rng.choice([30, 60, -7, 29, 1, 0x7fff]))
        world.put(v.PVS + 8, rng.choice([0, 1, 2, 4, 5, 5]))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            half(world, body + 0x124, 24 * rng.randrange(4))
            half(world, body + 0x126, 0x300)
            for slot in range(6):
                world.put(body + 0x300 + 4 * slot, special(rng))
            world.put(body + 0x2b0, fbits(rng.choice([0.0, 0.0, 0.5, 1.5, -1.0, 1e-40])))
            fix_type(world, n)

    def light_tag(rng):
        d = rng.randrange(4)
        return (0x7400 + d) << 16 | (LIGHT_ROW + d)

    def light_create_case(name, world, rng):
        function_world(world, rng)
        data_array(world, rng, LIGHTS, LIGHT_BITS, LIGHT_ROWS, 0x110, 0x4ecbdc)
        light_definitions(world, rng)
        n = rng.randrange(v.ROWS - 1)
        label = rng.choice(SAFE_NAMES + EXPORTS + UNKNOWN + [0, 0])
        marker = rng.choice([0, 1, 2, 3, 0xffff, 0x7fff, rng.getrandbits(16)]) | (rng.getrandbits(16) << 16)
        return [light_tag(rng), world.handles[n], rng.randrange(4), label, marker]

    def light_rows(world, rng):
        """Existing light rows: owners, timed fades and flags (bit 3, which
        reaches C5250, stays clear)."""
        handles = []
        for r in range(8):
            row = LIGHT_ROWS + 0x110 * r
            world.put(row + 4, light_tag(rng))
            owner = rng.choice(world.handles[:v.ROWS - 1] + [NONE, NONE])
            parent = NONE if owner != NONE or rng.random() < .5 else rng.choice(world.handles[:v.ROWS - 1])
            world.put(row + 0x4c, owner)
            world.put(row + 0x58, parent)
            attached = owner != NONE or parent != NONE
            half(world, row + 0x54, rng.choice([0, 1, 3, 0xffff]) if attached else 0xffff)
            world.put(row + 0x50, rng.choice(SAFE_NAMES + [0]))
            half(world, row + 0x56, rng.choice([0, 1, 2, 0xffff, 5]))
            byte(world, row + 2, rng.choice([0, 1, 2, 3, 0x10, 0x11, 0x12, 0x13, 0x33, 0x81]))
            world.put(row + 0x14, rng.choice([0, 100, rng.getrandbits(32)]))
            for offset in range(0xc4, 0xf8, 4):
                world.put(row + offset, special(rng))
            handles.append(((0x9100 + r) << 16) | r)
        return handles

    def light_update_case(name, world, rng):
        fresh(world, rng)
        objects(world, rng, False)
        body_values(world, rng, True)
        function_rows(world, rng, False)
        data_array(world, rng, LIGHTS, LIGHT_BITS, LIGHT_ROWS, 0x110, 0x4ecbdc)
        light_definitions(world, rng)
        handles = light_rows(world, rng)
        handle = rng.choice(handles)
        if rng.random() < .3:
            # An unattached, untimed light (definition copies) whose colors are
            # all zero but one denormal: visible unless MXCSR.DAZ.
            row = LIGHT_ROWS + 0x110 * (handle & 0xffff)
            world.put(row + 4, (0x7400 << 16) | LIGHT_ROW)
            world.put(row + 0x4c, NONE)
            world.put(row + 0x58, NONE)
            half(world, row + 0x54, 0xffff)
            byte(world, row + 2, rng.choice([1, 1, 3, 0x21]))
            world.put(row + 0xcc, fbits(1.0))
            definition = LIGHT_DEFS
            slots = [0x4c, 0x50, 0x54, 0x64, 0x68, 0x6c]
            for offset in slots:
                world.put(definition + offset, 0)
            if rng.random() < .8:
                world.put(definition + rng.choice(slots), fbits(rng.choice([1e-40, 3e-39, 1e-45])))
            world.put(definition + 8, fbits(1.0))
            world.put(definition + 0xc, fbits(1.0))
        return [handle]

    def light_scales_case(name, world, rng):
        fresh(world, rng)
        light_definitions(world, rng)
        functions = LIGHT_DEFS + 0x100 * rng.randrange(4) + 0xc0
        for k in range(4):
            world.put(OUT + 4 * k, special(rng))
        return [rng.getrandbits(32), special(rng), BLEND, functions, OUT, OUT + 8]

    def color_function_case(name, world, rng):
        """13E6E0 directly: every colour kind, stops with extreme channels,
        inputs inside, at and outside [0, 1] (the clamp, the stops'
        boundaries, the CVTPS2PI rounding of the fraction)."""
        fresh(world, rng)
        reference, blob = OUT + 0x40, BLOBS
        world.put(reference, rng.choice([1, 1, 1, 0, NONE]))
        world.put(reference + 4, rng.choice([blob] * 8 + [0]))
        byte(world, blob, rng.choice([0, 1, 1, 2, 3, 8]))
        byte(world, blob + 1, rng.choice([0x00, 0x01, 0x10, 0x11, 0x20, 0x21, 0x30, 0x31, 0x40, 0x41, 0x50, 0xf0]))
        for k in range(5):
            world.put(blob + 4 + 4 * k, rng.choice([0xff000000, 0x00ffffff, 0xffffffff, 0, 0x80808080, 0x7f01fe00,
                                                    rng.getrandbits(32)]))
        x = rng.choice([rng.random(), rng.random(), rng.random(), 0.0, -0.0, 1.0, 0.5, 1 / 3, 2 / 3, 0.25,
                        -0.25, 1.5, 1e-40, float('nan'), 0.99999994, 0.50001, 0.4999, 3.0517578e-05])
        return [reference, fbits(x)]

    def blend_case(name, world, rng):
        fresh(world, rng)
        for k in range(12):
            world.put(BLEND + 4 * k, special(rng))
        for at in (BLEND, BLEND + 0x10):
            if rng.random() < .5:
                world.put(at, fbits(rng.choice([1.0, 0.0, 1e-4, 1.0001e-4, 9.99e-5, -1.0])))
        color = rng.choice([BLEND + 0x20, BLEND + 0x20, 0])
        return [OUT, BLEND, BLEND + 0x10, color, rng.choice([0, 2, 0xfffffffe]), special(rng)]

    def interpolate_case(name, world, rng):
        fresh(world, rng)
        for k in range(12):
            world.put(BLEND + 4 * k, special(rng, rng.random() < .3))
        # The output may alias an operand (C0910 never does; the native reads per channel like the original).
        out = rng.choice([OUT, OUT, BLEND + 4, BLEND + 0x14])
        return [rng.choice([0, 2, 0xfffffffe]), BLEND, BLEND + 0x10, out, special(rng, False)]

    kernels += [
        ('color_interpolate', 'at_interpolate', 'void',
         dict(entry=0x134730, regs={'eax': 0, 'ecx': 1, 'esi': 2, 'edi': 3}, xmm={5: 4}, **stops), interpolate_case, 500),
        ('light_color_blend', 'at_blend', 'void',
         dict(entry=0x134870, regs={'ecx': 0, 'ebx': 3, 'eax': 4}, xmm={7: 5}, stack=[1, 2], **stops), blend_case, 600),
        ('light_function_scales', 'at_light_scales', 'void',
         dict(entry=0xc2b30, regs={'eax': 2, 'ecx': 3}, stack=[0, 1, 4, 5], **stops), light_scales_case, 800),
        ('color_function_value', 'at_color_function', 'long',
         dict(entry=0x13e6e0, regs={'ecx': 0}, stack=[1], **stops), color_function_case, 800),
        ('light_update', 'at_light_update', 'void',
         dict(entry=0xc2cb0, stack=[0], **stops), light_update_case, 1500),
        ('object_light_attachment_create', 'at_light', 'long',
         dict(entry=0xc2260, stack=[0, 1, 2, 3, 4], **stops), light_create_case, 1200),
    ]
    # --- Type-level callbacks: C9070 (unit), E0900 (biped), 1046C0 (weapon) --
    # Their objects use TRACK type definitions (tag rows 56..59, 0x400 bytes:
    # the unit, biped and weapon definition fields reach +2DC) with the
    # object definitions' +38 model, so BEC50 answers its names first.
    TYPE_DEFS, TYPE_ROW = TRACK + 0xb000, 56
    GRAPHS, ANIMATIONS, GRAPH_ROW = TRACK + 0xc000, TRACK + 0xc200, 60     # jmad graphs (0x80), 0x6C rows
    INHERITANCE = TRACK + 0xc100
    MAGAZINES, TRIGGER_ROWS = TRACK + 0xc800, TRACK + 0xcc00              # 0x5C and 0x40 rows, 0x100 per definition
    MANAGER = 0x280                                                        # body +12A
    REQUEST_FLAGS = 0x51df20                                               # 1E5C80: two zero bytes, no request

    def type_definitions(world, rng):
        mode9 = struct.unpack_from('<I', world.data2, v.PVS + 0x180 - DATA2)[0] == 9
        for k in range(4):
            definition = TYPE_DEFS + 0x400 * k
            tag_row(world, TYPE_ROW + k, definition, 0x7500 + k)
            world.put(definition + 0x38, (0x5000 + 4 + k) << 16 | (4 + k))
            world.put(definition + 0x64, 0)
            # Large limits too, so speeds and ratios land inside (0, 1).
            for offset in (0x1d4, 0x1dc, 0x2dc, 0x164):
                world.put(definition + offset, fbits(rng.choice([4.0, 10.0, 37.5])) if rng.random() < .3
                          else special(rng, rng.random() < .8))
            # Vehicle limits (F9270): zeros, tiny values around F9E10's 0.001 guard.
            # Signaling NaNs: the x87 FLD quiets them (F9E40, FABS paths).
            for offset in (0x1f4, 0x1f8, 0x204, 0x208, 0x20c, 0x22c, 0x230, 0x264):
                world.put(definition + offset, rng.choice([0x7f800001, 0xff812345]) if rng.random() < .05 else
                          fbits(rng.choice([0.0, -0.0, 0.0005, -0.002, 0.001, 0.0010001, 1e-40, 4.0, -10.0, 25.0]))
                          if rng.random() < .4 else special(rng, rng.random() < .8))
            world.put(definition + 0x150, fbits(1.0) if rng.random() < .25 else special(rng, rng.random() < .8))
            # 165590: +290 == 1 under mode 9 would reach 1655F0's 24B430.
            half(world, definition + 0x290, rng.choice([0, 2] if mode9 else [0, 1, 1, 2]))
            world.put(definition + 0x2c0, rng.choice([NONE, 0, 1, 2, 2]))
            world.put(definition + 0x2c4, MAGAZINES + 0x100 * k)
            world.put(definition + 0x2c8, rng.choice([NONE, 0, 1, 2, 3, 3]))
            world.put(definition + 0x2cc, TRIGGER_ROWS + 0x100 * k)
            world.put(definition + 0x2d0, rng.choice([NONE, 0, 1, 2, 4, 4]))
            for m in range(2):
                half(world, MAGAZINES + 0x100 * k + 0x5c * m + 0xa, rng.choice([0, 1, 7, 12, 0x7fff, 0xffff, 0x8000]))
            for t in range(3):
                row = TRIGGER_ROWS + 0x100 * k + 0x40 * t
                world.put(row + 0x1c, special(rng, rng.random() < .8))
                world.put(row + 0x28, special(rng, rng.random() < .8))

    def graphs(world, rng):
        world.extra[REQUEST_FLAGS] = bytearray(4)
        for g in range(2):
            graph = GRAPHS + 0x80 * g
            tag_row(world, GRAPH_ROW + g, graph, 0x7600 + g)
            world.put(graph + 0x30, ANIMATIONS + 0x200 * g)
            world.put(graph + 0x4c, rng.choice([0, 1, 1]))
            world.put(graph + 0x50, INHERITANCE + 0x20 * g)
            world.put(INHERITANCE + 0x20 * g + 4, (0x7600 + 1 - g) << 16 | (GRAPH_ROW + 1 - g))
            for a in range(4):
                half(world, ANIMATIONS + 0x200 * g + 0x6c * a + 0x14, rng.choice([0, 1, 10, 30, 0xfffb, 0x7fff, 2]))

    def manager(world, rng, body):
        at = body + MANAGER
        half(world, body + 0x12a, MANAGER)
        world.put(at + 0x7c, rng.choice([0x700005c, 0x700005d, 0x700005c, 0x700005d, 0x700005e, 0x600005c]))
        graph = rng.choice([NONE, (0x7600 << 16) | GRAPH_ROW, (0x7601 << 16) | (GRAPH_ROW + 1)])
        world.put(at, graph)
        inherited = rng.random() < .3 and graph != NONE
        count = struct.unpack_from('<I', world.track, GRAPHS + 0x80 * (graph & 1) + 0x4c - TRACK)[0] if graph != NONE else 0
        low = 0 if inherited and count else 0xffff
        half(world, at + 4, low)
        half(world, at + 6, rng.choice([0, 1, 2, 3, 0xffff]))
        world.put(at + 0x1c, fbits(rng.choice([0.0, 1.0, 5.5, 9.0, 29.0, 31.0, -2.0, 1e-40]) if rng.random() < .5
                                   else rng.uniform(-3, 40)))

    def units_world(world, rng):
        fresh(world, rng)
        objects(world, rng, False)
        body_values(world, rng, rng.random() < .7)
        world.put(v.PVS + 0x180, rng.choice([0, 9, 4, 8]))
        byte(world, GLOBALS_BLOCK + 0x82, rng.choice([0, 0, 1, 0x80]))
        type_definitions(world, rng)
        graphs(world, rng)
        world.put(v.TIME + 4, fbits(rng.choice([1 / 30, 1 / 60, 0.5, 1e-40])) if rng.random() < .8 else special(rng))
        tick = rng.getrandbits(31)
        world.put(v.TIME + 8, tick)
        # A unit's weapon slots (+218) and a weapon's triggers (+20C) overlap:
        # each body plays one role, units carry weapon-role handles.
        roles = [rng.choice(('unit', 'weapon')) for _ in range(v.ROWS - 1)]
        roles[0], roles[1] = 'unit', 'weapon'
        world.roles = roles
        weapons = [world.handles[n] for n in range(v.ROWS - 1) if roles[n] == 'weapon']
        for n in range(v.ROWS - 1):
            body = world.body(n)
            k = rng.randrange(4)
            world.put(body, ((0x7500 + k) << 16) | (TYPE_ROW + k))
            # Unit: C9070 fields, D3750 (+334/+338), the weapon slot (+212/+218).
            byte(world, body + 0x10a, rng.choice([0, 4, 0xfb, 0xff]))
            world.put(body + 0x134, rng.choice([0, 2, 1 << 18, (1 << 18) | 2, rng.getrandbits(32)]))
            for offset in (0x1b8, 0x2c4, 0x208, 0x2b0, 0x25c, 0x260, 0x264):
                world.put(body + offset, special(rng, rng.random() < .8))
            byte(world, body + 0x243, rng.getrandbits(8))
            byte(world, body + 0x1f8, rng.getrandbits(8))
            world.put(body + 0x248, rng.choice([NONE, tick, tick - 1, tick - 2, tick + 3, rng.getrandbits(32)]))
            world.put(body + 0x24c, rng.choice([NONE, 0, rng.getrandbits(32)]))
            world.put(body + 0x334, special(rng, rng.random() < .8))
            world.put(body + 0x338, rng.choice([0, 0, 0x80000000, 1]) if rng.random() < .4 else special(rng))
            manager(world, rng, body)
            # Vehicle: F9270 (+70/+7C axes, +148, +348.., +354..+378, +384, +38B, +3FC).
            for offset in (0x70, 0x74, 0x78, 0x7c, 0x80, 0x84):
                world.put(body + offset, special(rng, rng.random() < .8))
            world.put(body + 0x148, rng.choice([0, 2, 0x1000, 0x1002, rng.getrandbits(32)]))
            for offset in (0x348, 0x34f, 0x384):
                byte(world, body + offset, rng.choice([0, 0, 1, 0x20, 15, 16, 0xff]))
            byte(world, body + 0x38b, rng.choice([0xfe, 0xff, 0, 1, 3]))
            for offset in (0x354, 0x358, 0x35c, 0x360, 0x368, 0x36c, 0x370, 0x374, 0x378, 0x3fc):
                world.put(body + offset, special(rng, rng.random() < .8))
            # Biped: E0900 (+3DC, +39D, the +88 velocity).
            byte(world, body + 0x3dc, rng.choice([0, 1, 2, 2, 3]))
            byte(world, body + 0x39d, rng.choice([0, 1, 0x80, 0xff]))
            for offset in (0x88, 0x8c, 0x90):
                world.put(body + offset, special(rng, rng.random() < .8))
            # Weapon: 1046C0 (age, heat, barrels +1B4.., triggers +20C, magazines +22C).
            byte(world, body + 0x175, rng.getrandbits(8))
            for offset in (0x180, 0x184, 0x188, 0x18c):
                world.put(body + offset, special(rng, rng.random() < .8))
            byte(world, body + 0x16c, rng.getrandbits(8))
            byte(world, body + 0x16e, rng.getrandbits(8))
            byte(world, body + 0x12c, rng.choice([0, 1, 2, 3, 0xfd]))
            half(world, body + 0x17e, rng.choice([0xffff, 0, 3]))
            for b in range(4):
                for offset in (0x1b4, 0x1b8, 0x1bc):
                    world.put(body + offset + 0x34 * b, special(rng, rng.random() < .8))
            for t in range(3):
                byte(world, body + 0x20c + 12 * t, rng.choice([0, 1, 1, 2, 2, 3, 0xff]))
                half(world, body + 0x20e + 12 * t, rng.choice([0, 1, 5, 30, 0x7fff, 0xfff0]))
            for m in range(2):
                half(world, body + 0x22c + 16 * m, rng.choice([0, 1, 9, 10, 37, 123, 0xffff, 0xff85, 0x8000, 0x7fff]))
            if roles[n] == 'unit':
                for slot in range(4):
                    world.put(body + 0x218 + 4 * slot, rng.choice(weapons + [NONE]))
                byte(world, body + 0x212, rng.choice([0xff, 0, 1, 2, 3]))
            fix_type(world, n)

    known_unit = [0xf0006bf, 0x90006aa, 0xb0006ab, 0x5000581, 0x6000087, 0xe00059f, 0x11000088, 0x1100059b,
                  0x1100059c, 0x160005a0, 0xd00059d, 0x1800059e, 0x90005a1, 0x140005a2, 0x140005a3, 0xb0005a9,
                  0x130005a8]
    known_biped = [0x6000542, 0x70000c9, 0xc0005a4]
    known_weapon = [0x1d0007ba, 0xc000569, 0x1500056e, 0x1700056f, 0x14000570, 0x16000571, 0x10000573, 0xe000572,
                    0xf000574, 0x11000575, 0x1200056a, 0x1400056d, 0x1700056b, 0x1700056c, 0xb0005a9, 0x5000081,
                    0x3000577, 0x4000567, 0x5000024, 0x9000568, 0xa000066, 0x14000578, 0x16000579, 0xe0006e6,
                    0xd0006e7, 0xf0006e5, 0x120006e4]
    known_vehicle = [0x5000581, 0x4000580, 0x800061e, 0x5000596, 0x6000595, 0xd00059a, 0x900000e, 0xa00000f,
                     0xa00001a, 0xb00001b, 0xa000598, 0x13000588, 0x14000589, 0x1700058e, 0x1800058c, 0x1800058f,
                     0x1900058d, 0xe00057a, 0xe00057d, 0xd00057f, 0xb000713, 0x1300057e, 0xe000583, 0xf000584,
                     0xc000582, 0x10000585, 0xe00057c, 0xd00057b, 0x14000599]
    # F9E40 kind 2, FA5D0 and 11FF40 users (native since 2026-11-15).
    known_vehicle += [0xb000587, 0xd000586, 0x1300058a, 0x1400058b, 0x17000592, 0x18000590, 0x18000593,
                      0x19000591, 0xb000597, 0xc0005a5, 0x10000594]
    refused_vehicle = set()
    known_projectile = [0xe000562, 0x6000564, 0x11000566, 0xf000563, 0x12000565]
    known_device = [0x5000561, 0x5000081, 0x8000080, 0x40005ab, 0x6000560, 0x1200055f, 0xf00055e]
    known_component = [0xa000693, 0xd000691, 0xd000692]
    refused_base = {0x70006ba, 0x90006b5, 0xa0006b6, 0xc0006b7, 0xc0006b8, 0x130006bb, 0xd0005ff, 0x170005fa,
                    0x170005fb, 0x180005fc, 0x1d0005fd, 0x1d0005fe, 0x700055c, 0xf0005ac}
    every = (set(known_unit) | set(known_biped) | set(known_weapon) | set(known_vehicle) | set(known_projectile)
             | set(known_device) | set(known_component))
    # Unknown names on both sides of every comparison of the four trees.
    near = sorted({n + d for n in every | refused_vehicle for d in (-1, 1, 8)} - every - refused_base - refused_vehicle)

    def type_name(rng, known):
        return rng.choice(known * 3 + near + UNKNOWN)

    def role_handle(world, rng, role):
        return world.handles[rng.choice([n for n in range(v.ROWS - 1) if world.roles[n] == role])]

    def unit_case(name, world, rng):
        units_world(world, rng)
        known = {'unit_function_value': known_unit, 'biped_function_value': known_biped,
                 'weapon_function_value': known_weapon, 'vehicle_function_value': known_vehicle}[name]
        role = 'weapon' if name == 'weapon_function_value' else 'unit'
        handle, label = role_handle(world, rng, role), type_name(rng, known)
        if name == 'vehicle_function_value':
            vehicle_fields(world, rng, handle)
            if rng.random() < .15:
                # The engine names on a definition +1F0 mode other than 0/1,
                # with +348 bit 7 against bit 6 and +34C weights inside (0.5, 1).
                label = rng.choice([0xb000597, 0xc0005a5])
                body = world.body(handle & 0xffff)
                definition = TYPE_DEFS + 0x400 * (struct.unpack_from('<I', world.data, body - DATA)[0] & 3)
                half(world, definition + 0x1f0, rng.choice([2, 3, 0xffff]))
                byte(world, body + 0x348, rng.choice([0x80, 0x81, 0x7f, 0x40, 0]))
                byte(world, body + 0x34c, rng.choice([1, 2, 3, 4]))
                for offset in (0x354, 0x3dc, 0x78, 0x74, 0x70, 0x88, 0x8c, 0x90, 0x3f0, 0x3f4, 0x3f8):
                    world.put(body + offset, fbits(rng.uniform(-2, 2)))
                for offset in (0x1f4, 0x1f8):
                    world.put(definition + offset, fbits(rng.uniform(1, 10)))
                world.put(body + 0x25c, fbits(rng.choice([1.0, 0.5, 0.0, -1.0])))
            elif rng.random() < .12:
                # wingtip_contrail with a lateral speed under 0.3 (an unsaturated
                # square) or NaN velocity/forward lanes of distinct payloads.
                label = 0x10000594
                body = world.body(handle & 0xffff)
                for offset in (0x70, 0x74, 0x78, 0x88, 0x8c, 0x90):
                    world.put(body + offset, fbits(rng.uniform(-0.15, 0.15)))
                if rng.random() < .4:
                    # The forward lane's payload is the larger: the x87 length
                    # keeps the larger NaN significand, so the MULSS operand
                    # order of v1 * f1 stays observable.
                    world.put(body + 0x74, rng.choice([0x7fc00009, 0xffc0000a]))
                    world.put(body + 0x8c, rng.choice([0x7fc00003, 0xffc00004]))
        if name == 'weapon_function_value':
            mode9_world(world, rng)
            if rng.random() < .2:
                # The bomb names on a live mode-9 player (1655F0 -> 24B430):
                # values 0, (0, 1) and 1 for the unclamped compares.
                label = rng.choice([0xf0006e5, 0xe0006e6, 0xd0006e7, 0x120006e4])
                body = world.body(handle & 0xffff)
                definition = TYPE_DEFS + 0x400 * (struct.unpack_from('<I', world.data, body - DATA)[0] & 3)
                world.put(v.PVS + 0x180, 9)
                world.put(v.PVS + 8, 2)
                half(world, definition + 0x290, 1)
                player = rng.randrange(4)
                half(world, body + 0x17e, player)
                byte(world, MODE9 + 0x170 + player, rng.choice([0, 0, 8]))
                half(world, MODE9 + 0x1a0 + 2 * player, rng.choice([0, 0xfff0, 1, 2, 15, 30, 31]))
                half(world, v.PVS + 0x244, rng.choice([0, 1, 30]))
                world.put(v.TIME + 4, fbits(rng.choice([1 / 30, 1 / 60, 0.5])))
        if name == 'vehicle_function_value' and rng.random() < .1:
            # 0x1300057E's maximum of two ratios with a NaN on one side: the
            # JA keeps the non-NaN second ratio only in the original order.
            label = 0x1300057e
            world.put(world.body(handle & 0xffff) + rng.choice([0x354, 0x358]), rng.choice([0x7fc00000, 0xffc00000]))
        return [handle, label, OUT, OUT + 4]

    # --- 2026-11-15: F9270's limit, engine and contrail names, 24B430, and
    # the projectile (100E70), device (10BE20) and machine/crate (11E490,
    # 1DB800) levels.
    COMPONENT_POOL, COMPONENT_ROWS, COMPONENTS = TRACK + 0xd000, TRACK + 0xd100, 4   # [0x52BE5C], 0xA0 rows
    MODE9 = TRACK + 0xd800                                                          # [0x52D710]

    def edge_float(rng, finite=False):
        return fbits(rng.choice([0.0, -0.0, 1.0, 0.9, 0.90000004, 1e-4, 9.99e-5, -1e-4, 0.0001001, 1e-40, -1e-40,
                                 0.5, 2.0, -3.0])) if rng.random() < .5 else (
            rng.choice([0x7f800001, 0xff812345, 0x7fc00000]) if not finite and rng.random() < .1 else special(rng, finite))

    def vehicle_fields(world, rng, handle):
        body = world.body(handle & 0xffff)
        definition = TYPE_DEFS + 0x400 * (struct.unpack_from('<I', world.data, body - DATA)[0] & 3)
        half(world, definition + 0x1f0, rng.choice([0, 1, 2, 3, 0xffff, 0x8000]))
        byte(world, body + 0x348, rng.choice([0, 0x80, 0x81, 0xff, 1, 0x7f]))
        for offset in (0x3f0, 0x3f4, 0x3f8, 0x3dc, 0x25c):
            world.put(body + offset, edge_float(rng, rng.random() < .8))
        byte(world, body + 0x34c, rng.choice([0, 1, 2, 5, 6, 0xff, rng.getrandbits(8)]))

    def mode9_world(world, rng):
        world.globals[0x52d710] = MODE9
        mode = rng.choice([9, 9, 0])
        world.put(v.PVS + 0x180, mode)
        world.put(v.PVS + 8, rng.choice([2, 2, 2, 1]))
        half(world, v.PVS + 0x244, rng.choice([0, 1, 3, 0xffff, 0x7fff, 0x8000]))
        for k in range(4):
            half(world, TYPE_DEFS + 0x400 * k + 0x290, rng.choice([0, 1, 1, 2]))
        for p in range(4):
            byte(world, MODE9 + 0x170 + p, rng.choice([0, 8, 0xf7, 0xff, rng.getrandbits(8)]))
            half(world, MODE9 + 0x1a0 + 2 * p, rng.choice([0, 1, 30, 0x7fff, 0x8000, 0xffff, rng.getrandbits(16)]))
        for n in range(v.ROWS - 1):
            half(world, world.body(n) + 0x17e, rng.choice([0xffff, 0, 1, 2, 3]))

    def device_world(world, rng):
        """Projectile, device and machine fields over units_world."""
        world.globals[0x52be5c] = COMPONENT_POOL
        world.put(COMPONENT_POOL + 0x44, COMPONENT_ROWS)
        for c in range(COMPONENTS):
            row = COMPONENT_ROWS + 0xa0 * c
            world.put(row + 4, rng.choice([0, 0x2000, 0xffffdfff, rng.getrandbits(32)]))
            byte(world, row + 0x1a, rng.choice([0, 1, 0x80, 0xff, rng.getrandbits(8)]))
            byte(world, row + 0x1b, rng.choice([0, 1, 0x80, 0xff, rng.getrandbits(8)]))
        for k in range(4):
            definition = TYPE_DEFS + 0x400 * k
            for offset in (0xe0, 0x17c, 0x180, 0x18c, 0x194, 0xc0, 0xc8, 0x10c):
                world.put(definition + offset, edge_float(rng, rng.random() < .7))
        for n in range(v.ROWS - 1):
            body = world.body(n)
            for offset in (0x134, 0x138, 0x140, 0x144, 0x158, 0x164, 0x168):
                world.put(body + offset, edge_float(rng, rng.random() < .7))
            world.put(body + 0x12c, rng.choice([0, 2, 8, 0x28, 0x100, 0x10a, rng.getrandbits(32)]))
            half(world, body + 0x148, rng.choice([0, 1, 30, 0x7fff, 0x8000, 0xffff]))
            world.put(body + 0x13c, rng.choice(world.groups + [NONE, NONE]))
            world.put(body + 0x1cc, rng.choice([0, 1, 4, 8, 0x10, 0x11, 0xd, rng.getrandbits(32)]))
            world.put(body + 0xb4, rng.choice([NONE, NONE] + [((0x6100 + c) << 16) | c for c in range(COMPONENTS)]))

    def device_case(name, world, rng):
        units_world(world, rng)
        device_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        known = {'projectile_function_value': known_projectile, 'device_function_value': known_device,
                 'machine_function_value': known_component}[name]
        body = world.body(n)
        definition = TYPE_DEFS + 0x400 * (struct.unpack_from('<I', world.data, body - DATA)[0] & 3)
        if name == 'projectile_function_value' and rng.random() < .15:
            # acceleration_range with one zero limit: +17C, then +180.
            world.put(body + 0x12c, rng.choice([0, 2, 0x8a]))
            world.put(definition + 0x17c, fbits(rng.choice([0.0, 2.0, 0.5])))
            world.put(definition + 0x180, fbits(rng.choice([0.0, -0.0, 1e-40, 3.0])))
            world.put(body + 0x168, fbits(rng.uniform(0, 3)))
            world.put(definition + 0x18c, fbits(rng.uniform(0, 1)))
            world.put(definition + 0x194, fbits(rng.uniform(0.1, 1)))
            return [world.handles[n], 0x12000565, OUT, OUT + 4]
        if name == 'device_function_value' and rng.random() < .15:
            # delay inside (0, 1): (tick seconds / +10C) * word +148.
            world.put(definition + 0x10c, fbits(rng.uniform(0.5, 5)))
            half(world, body + 0x148, rng.randrange(2, 20))
            world.put(v.TIME + 4, fbits(rng.choice([1 / 30, 1 / 60, 0.0333])))
            return [world.handles[n], 0x5000561, OUT, OUT + 4]
        if name == 'device_function_value':
            byte(world, world.body(n) + 0xaa, rng.choice([7, 7, 8, 9, 0]))
            if rng.random() < .15:
                # change_in_position under +12C bit 3 with a speed at the 1e-4 floor.
                body = world.body(n)
                world.put(body + 0x12c, rng.choice([8, 9, 0x88]))
                world.put(body + 0x164, fbits(rng.choice([9.99e-5, -9.99e-5, 1e-4, 0.0, 1e-40, 0.0001001])))
                world.put(body + 0x144, fbits(rng.choice([0.5, -2.0, 1e-5])))
                return [world.handles[n], 0x1200055f, OUT, OUT + 4]
        return [world.handles[n], own_name(rng, known), OUT, OUT + 4]

    def own_name(rng, known):
        """Mostly the callback's own names, with their neighbours and unknowns."""
        return rng.choice(known * 8 + [n + d for n in known for d in (-1, 1, 8)] + UNKNOWN)

    def component_case(name, world, rng):
        units_world(world, rng)
        device_world(world, rng)
        return [COMPONENT_ROWS + 0xa0 * rng.randrange(COMPONENTS), own_name(rng, known_component), OUT, OUT + 4]

    def mode9_case(name, world, rng):
        units_world(world, rng)
        mode9_world(world, rng)
        return [role_handle(world, rng, 'weapon')]

    def device_type_case(name, world, rng):
        """10DE40 over projectiles, devices, machines and crates."""
        units_world(world, rng)
        device_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        byte(world, world.body(n) + 0xaa, rng.choice([5, 5, 7, 7, 8, 9, 11, 11]))
        fix_type(world, n)
        label = rng.choice(known_projectile + known_device + known_component + near + BASE_NAMES + UNKNOWN)
        if label in (0xf0005ac, 0x700055c):
            label = 0xd000558
        return [world.handles[n], label, OUT, OUT + 4]

    def unit_type_case(name, world, rng):
        """10DE40 over bipeds and weapons: BEC50, C9070 and E0900, or 1046C0."""
        units_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        byte(world, world.body(n) + 0xaa, rng.choice({'unit': [0, 0, 1, 1], 'weapon': [2] * 4}[world.roles[n]] + SIMPLE_TYPES))
        fix_type(world, n)
        label = rng.choice(known_unit + known_biped + known_weapon + known_vehicle + near + BASE_NAMES + UNKNOWN)
        if label in (0xf0005ac, 0x700055c):
            label = 0xd000558
        return [world.handles[n], label, OUT, OUT + 4]

    def unit_xmm_case(name, world, rng):
        units_world(world, rng)
        n = rng.choice([k for k in range(v.ROWS - 1) if world.roles[k] ==
                        ('weapon' if name == 'weapon_trigger_fraction' else 'unit')])
        if name == 'animation_channel_fraction':
            # 1CF0E0 itself does not test the graph (D3950 does): keep it valid.
            channel = world.body(n) + MANAGER
            if struct.unpack_from('<I', world.data, channel - DATA)[0] == NONE:
                world.put(channel, (0x7600 << 16) | GRAPH_ROW)
                half(world, channel + 4, 0xffff)
            return [channel]
        if name == 'unit_animation_fraction':
            return [rng.choice([world.handles[n]] * 6 + [NONE])]
        if name == 'weapon_trigger_fraction':
            return [world.handles[n], rng.choice([0, 1, 2, 3, 0xffff, 0x10001, 0xfffe])]
        return [world.handles[n]]

    kernels += [
        ('unit_function_value', 'at_unit_value', 'boolean', dict(entry=0xc9070, stack=[0, 1, 2, 3], **stops),
         unit_case, 1500),
        ('biped_function_value', 'at_biped_value', 'boolean', dict(entry=0xe0900, stack=[0, 1, 2, 3], **stops),
         unit_case, 800),
        ('weapon_function_value', 'at_weapon_value', 'boolean', dict(entry=0x1046c0, stack=[0, 1, 2, 3], **stops),
         unit_case, 2000),
        ('vehicle_function_value', 'at_vehicle_value', 'boolean', dict(entry=0xf9270, stack=[0, 1, 2, 3], **stops),
         unit_case, 2000),
        ('unit_function_ratio', 'at_unit_ratio', 'xmm0', dict(entry=0xd3750, regs={'eax': 0}, **stops),
         unit_xmm_case, 500),
        ('unit_animation_fraction', 'at_unit_fraction', 'xmm0', dict(entry=0xd3950, regs={'eax': 0}, **stops),
         unit_xmm_case, 600),
        ('animation_channel_fraction', 'at_channel_fraction', 'xmm0', dict(entry=0x1cf0e0, regs={'esi': 0}, **stops),
         unit_xmm_case, 600),
        ('weapon_trigger_fraction', 'at_trigger_fraction', 'xmm0', dict(entry=0x107140, regs={'eax': 0}, stack=[1],
         **stops), unit_xmm_case, 600),
        ('unit_type_function_value', 'at_unit_type', 'boolean',
         dict(entry=0x10de40, regs={'ebx': 2, 'edi': 3}, stack=[0, 1], **stops), unit_type_case, 1500),
        ('projectile_function_value', 'at_projectile_value', 'boolean', dict(entry=0x100e70, stack=[0, 1, 2, 3], **stops),
         device_case, 800),
        ('device_function_value', 'at_device_value', 'boolean', dict(entry=0x10be20, stack=[0, 1, 2, 3], **stops),
         device_case, 1500),
        ('machine_function_value', 'at_machine_value', 'boolean', dict(entry=0x11e490, stack=[0, 1, 2, 3], **stops),
         device_case, 600),
        ('physics_component_function_value', 'at_component_value', 'boolean',
         dict(entry=0x1db800, regs={'esi': 0, 'ecx': 1, 'edx': 2}, stack=[3], **stops), component_case, 400),
        ('weapon_mode9_value', 'at_mode9_value', 'xmm0', dict(entry=0x24b430, regs={'eax': 0}, **stops),
         mode9_case, 600),
        ('device_type_function_value', 'at_device_type', 'boolean',
         dict(entry=0x10de40, regs={'ebx': 2, 'edi': 3}, stack=[0, 1], **stops), device_type_case, 1500),
    ]

    # --- The console clock: BEB00/BEB40/BEB90 and 13BB70 enter after their
    # 13BB10 call (the observer cannot run GetLocalTime's kernel imports),
    # with 13BB10's unpacked time (year, month, day, hour, minute, second
    # dwords) as their frame and STOP as their return; BEBE0 after its 13BB70
    # call with the holiday in EAX. BEBE0's default (its local 0.0 at
    # [ESP]) is the observer's return slot here, so its cases are holidays
    # 1..4 only.
    def clock_case(name, world, rng):
        month = rng.choice([12, 12, 1, 10, 7, 7, rng.randrange(1, 13), 0, 0xffff, rng.getrandbits(32)])
        day = rng.choice([25, 12, 1, 31, 7, 4, rng.randrange(1, 32), 0, 0xffff])

        def field(top):
            return rng.choice([0, 1, top - 1, top, top + 1, rng.randrange(0, top + 1), rng.randrange(0, top + 1),
                               0xffff, 0x7fffffff, 0x80000000, 0xffffffff, rng.getrandbits(32)])
        time = [rng.choice([2004, 2026, 0xffff, rng.getrandbits(32)]), month, day, field(24), field(60), field(60)]
        if name == 'local_holiday':
            return [0] + time + [v.STOP]       # 13BB70's saved EDI/ESI, its frame, its return
        if name == 'holiday_function_value':
            return [rng.choice([1, 2, 3, 4]), v.STOP]
        return time + [v.STOP]

    clock = [0, 1, 2, 3, 4, 5, 6]
    kernels += [
        ('clock_hour_value', 'at_clock', 'xmm0', dict(entry=0xbeb0d, stack=clock), clock_case, 400),
        ('clock_minute_value', 'at_clock', 'xmm0', dict(entry=0xbeb4d, stack=clock), clock_case, 400),
        ('clock_second_value', 'at_clock', 'xmm0', dict(entry=0xbeb9d, stack=clock), clock_case, 400),
        ('local_holiday', 'at_clock', 'long', dict(entry=0x13bb80, regs={'edi': 0}, stack=clock + [7]),
         clock_case, 600),
        ('holiday_function_value', 'at_clock', 'xmm0', dict(entry=0xbebee, regs={'eax': 0}, stack=[1]),
         clock_case, 200),
    ]
    FIXTURES.update(fresh=fresh, function_world=function_world, data_array=data_array,
                    contrail_definitions=contrail_definitions, light_definitions=light_definitions,
                    special=special, name_choice=name_choice, light_tag=light_tag, fix_type=fix_type,
                    sounds=(SOUNDS, SOUND_BITS, SOUND_ROWS), contrails=(CONTRAILS, CONTRAIL_BITS, CONTRAIL_ROWS),
                    lights=(LIGHTS, LIGHT_BITS, LIGHT_ROWS), contrail_row=CONTRAIL_ROW, safe_names=SAFE_NAMES)
    # HALO2_ATTACHMENTS_CASE_SCALE (mutation screens only) scales every count.
    import os
    scale = float(os.environ.get('HALO2_ATTACHMENTS_CASE_SCALE', '1'))
    for name, argument, kind, routine, case, count in kernels:
        ROUTINE_NAMES.append(name)
        v.KERNELS[name] = ([argument], kind)
        v.ROUTINES[name] = routine
        v.TRACK_ARGUMENTS[argument] = case
        v.TRACK_COUNTS[name] = count if scale == 1 else max(50, int(count * scale))
