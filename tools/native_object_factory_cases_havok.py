"""Oracle cases for factory track "havok" (Havok allocator, array growth,
body/component activation, 1CC310).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py
as a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Indirect transfers the cases execute are declared as REGISTER_JUMPS so the
original's targets join the authenticated closure: the allocator [0x48C938]
methods 149DA0/149E00 (from 2E3250), the pool methods 233250/233420 (from
149DA0/149E00) and the default listener 2E4D60 (322660's world +4 and entity
+8 slots). Targets the natives refuse are declared with no admitted target
(executing one escapes the sites): the default heap's method after 149930,
233420's import free above 0x2000 bytes. 149930 (lazy default heap; its
closure has unbounded virtuals and is reachable only through 149DA0/149E00/
149C40/149D20) is a global closure stop.

Pools live in the image .rdata range (world.extra banks): P at 0x420000 and
P2 at 0x430000, each with its header (+0..+0x340), first page (+0x4340,
0x2000 bytes) and counters (+0xF0340). Everything else is in the TRACK bank.

SEH frames: 318550/318680, the arena arrays and the broadphase install an
exception registration through fs:[0]. register() gives the physical
original a TIB: the reference bridge points FS at a set_thread_area segment
based at STACK + 0x40, and FS-segment operands are authenticated at that
base. fs:[0] is thus a declared stack word; its stores, like every stack
store, are not compared (the natives keep no exception chain).
"""
import struct
import subprocess
from capstone.x86_const import X86_OP_MEM, X86_REG_FS

NONE = 0xffffffff
P, P2 = 0x420000, 0x430000
PAGE, COUNTERS = 0x4340, 0xf0340
# 2E3250 -> allocator, 149DA0/149E00 -> pool, 322660 -> listener.
REGISTER_JUMPS = {0x2e327b: [0x149da0], 0x2e32b0: [0x149e00],
                  0x149dbc: [0x233250], 0x149dcd: [0x233250], 0x149def: [0x233250], 0x149ddf: [],
                  0x149e32: [0x233420], 0x149e7a: [], 0x23347d: [],
                  0x32267f: [0x2e4d60], 0x3226f2: [0x2e4d60],
                  # 1D8470: body destructor slots (default 2E4D60) and the
                  # record release through [0x48C938] +14.
                  0x1d84bf: [0x2e4d60], 0x1d85d1: [0x2e4d60], 0x1d8655: [0x2e4d60], 0x1d8692: [0x2e4d60],
                  0x1d85f5: [0x149e00]}
# 1CD6A0 and 1D8EB0: concrete motion-kind getters, then the already native
# 1D9080/1D9100 motion setters. Declared owners execute unchanged in the oracle.
MOTION_KINDS = [0x31d520, 0x31e2f0, 0x2e8cf0, 0x31dd70, 0x31edf0, 0x31ede0]
VELOCITY_JUMPS = {0x1cd797: MOTION_KINDS, 0x1d8ec2: MOTION_KINDS,
                  0x1d90f3: [0x3239f0], 0x1d9181: [0x323a00]}
# 1D73D0's shape builders (unit, vehicle, machine, crate, creature) are
# refused at their entries: the original stops there (observed stop entry).
SHAPE_BUILDERS = [0x1dee30, 0x1dd9a0, 0x1ddbf0, 0x1dd950, 0x1def50]
# Broadphase (2EC4B0/2EC9B0), hkWorld phantoms (318550/318680) and 1CE340:
# reserve 2E31E0, arena slow paths 2E10A0/2E1110 (allocator +24/+28), the
# phantom +18 AABB (315170), broadphase +4/+C, the 37C960 filter (2E9A00)
# and phantom pair handlers (31FFC0/320040) with the hkAabbPhantom overlap
# owners (315300/315100, or 2E4D60), the listener slots (2E4D60) and the
# 315620 destructor chain. 315459 (a phantom shape destructor) is refused.
PHANTOM_JUMPS = {0x2e31fc: [0x149da0], 0x2e3235: [0x149e00], 0x2eb89d: [0x2e1110], 0x2eb8d7: [0x149e00],
                 0x2eba6e: [0x2e10a0], 0x2e10c5: [0x149da0], 0x2e114a: [0x149e00],
                 0x31782e: [0x2e10a0], 0x3178bd: [0x2e1110], 0x3178f7: [0x149e00],
                 0x31859a: [0x315170], 0x3185be: [0x2ec4b0], 0x3186d5: [0x2ec9b0], 0x318750: [0x315620],
                 0x32229e: [0x2e4d60], 0x32230e: [0x2e4d60],
                 0x37ca27: [0x2e9a00], 0x37ca7f: [0x2e9a00], 0x37ca42: [0x31ffc0], 0x37ca9a: [0x31ffc0],
                 0x37c9f8: [0x320040], 0x37cac4: [0x320040],
                 0x31fff2: [0x315300], 0x32001e: [0x315300], 0x320033: [0x315300],
                 0x32006c: [0x315100, 0x2e4d60], 0x32009a: [0x315100, 0x2e4d60],
                 0x314fc1: [0x2e4d60], 0x315012: [0x2e4d60],
                 0x31563f: [0x149e00], 0x3155c0: [0x149e00], 0x31fae3: [0x2e4d60], 0x31fb0b: [0x149e00],
                 0x31fb31: [0x149e00], 0x315459: [], 0x31547e: [0x149e00],
                 0x1ce40d: [0x149da0], 0x1ce4a1: [0x315620], 0x1ce4c3: [0x149e00]}
PHANTOM_STOPS = [0x2e12a0]  # B4150's flagged-allocation path (refused)
# Entities (3198A0/319370) and component membership (1D9510/1D97F0): motion
# kind/storage getters, island +10 (320130) and destructor (322100), shape
# bounds (sphere 2E8D90, capsule 2E8120), broadphase +4/+C, allocator and
# arena slots, default listeners, the record destructor 1D9C80, and 37C960's
# entity pair owners: the island handler (320EF0/320260) and the fixed-body
# collector 0x41D9A0 (317B70/317C40). Entity destructors are refused.
ENTITY_JUMPS = {0x1d9587: MOTION_KINDS, 0x1d988b: MOTION_KINDS, 0x1d9796: [0x149e00], 0x1d98f2: [0x149da0],
                0x1d9a34: [0x1d9c80], 0x1d9a5f: [0x1d9c80], 0x31e781: [0x2e8d90, 0x2e8120],
                0x318092: [0x2e8d90, 0x2e8120], 0x3221be: [0x2e4d60], 0x32222e: [0x2e4d60],
                0x31fdfd: [0x2e4d60], 0x31fe24: [0x2e4d60], 0x31fe54: [0x2e4d60],
                0x3180ab: [0x2ec4b0], 0x318237: [0x2ec9b0], 0x32167e: [0x149e00], 0x321693: [0x149da0],
                0x317b1c: [0x2e1110], 0x317b56: [0x149e00], 0x317a8f: [0x2e10a0], 0x317d63: [0x149e00],
                0x31699e: [0x149da0], 0x316a27: [0x149da0], 0x320e22: [0x323420], 0x32018e: [0x323420],
                0x320e28: [0x320130], 0x320196: [0x320130], 0x319954: [], 0x31842d: [], 0x3183d5: [0x322100],
                0x37ca42: [0x317b70, 0x320ef0], 0x37ca9a: [0x317b70, 0x320ef0],
                0x37c9f8: [0x317c40, 0x320260], 0x37cac4: [0x317c40, 0x320260],
                # 320260/320EF0's agent/manager owners are refused (no target).
                0x32034b: [], 0x320352: [], 0x3204b9: [], 0x3204c1: [],
                0x321092: [], 0x3210ab: [], 0x3210b4: [], 0x3210d3: [], 0x3210f9: [], 0x321111: [],
                0x321132: [], 0x321160: [],
                0x1d9cc0: [0x149e00], 0x32211f: [0x149e00], 0x3213f9: [0x149e00], 0x321425: [0x149e00],
                0x32144b: [0x149e00], 0x321471: [0x149e00], 0x321497: [0x149e00], 0x3214bd: [0x149e00],
                0x3214e3: [0x149e00]}
# MOVQ m64, mm stores of 2E9CF0/2E9DF0. Capstone reports this destination as
# a read (like the x87 and MOVHPS stores the validator already corrects), so
# they would go unlogged; only these sites, which only this track executes,
# are corrected here.
MMX_STORES = {0x2e9dcc, 0x2e9dd6, 0x2e9ecc, 0x2e9ed6}


def install_fs_tib(v):
    """Give the physical original a TIB (see the module docstring). Installed
    once on the validator module itself, also when borrowed."""
    real = vars(v).get('_v', v)
    if getattr(real, '_havok_fs_tib', None) is not None:
        return
    base = real.STACK + 0x40
    real._havok_fs_tib = base
    metadata, prepare = real.metadata, real.prepare

    def metadata_fs(image, evaluators):
        meta, code = metadata(image, evaluators)
        for site in meta['sites']:
            if site['instruction'] in MMX_STORES:
                site['operands'][0].update(read=False, write=True)
            instruction = code.at(site['instruction'])
            memory = [op for op in instruction.operands if op.type == X86_OP_MEM and instruction.mnemonic != 'lea']
            for operand, entry in zip(memory, site['operands']):
                if operand.mem.segment == X86_REG_FS and entry['displacement'] < 0x1000:
                    entry['displacement'] += base
        return meta, code

    def prepare_fs(image, meta, scratch):
        owned = prepare(image, meta, scratch)
        source = (owned / 'reference.s').read_text()
        bridge = '_start:\nfninit\nint3\njmp _start\n'
        if source.count(bridge) != 1:
            raise ValueError('Original reference bridge changed; the havok TIB cannot be installed')
        # set_thread_area(user_desc{-1, base, 0xFFFFF, 32-bit|pages|usable}), then FS.
        (owned / 'reference.s').write_text(source.replace(bridge, f'''_start:
push $0x51
push $0xfffff
push $0x{base:x}
push $-1
mov %esp, %ebx
mov $243, %eax
int $0x80
test %eax, %eax
jnz 2f
mov (%esp), %eax
shl $3, %eax
or $3, %eax
mov %ax, %fs
add $16, %esp
1:
fninit
int3
jmp 1b
2:
ud2
'''))
        for command in (['as', '--32', str(owned / 'reference.s'), '-o', str(owned / 'reference.o')],
                        ['ld', '-melf_i386', '-T', str(owned / 'link.ld'), str(owned / 'reference.o'), '-o',
                         str(owned / 'reference')]):
            result = subprocess.run(command, text=True, capture_output=True, check=False)
            if result.returncode:
                raise RuntimeError(' '.join(command) + '\n' + result.stderr[-3000:])
        return owned
    real.metadata, real.prepare = metadata_fs, prepare_fs


def register(v):
    T = v.TRACK
    ALLOC, AVT, PVT, LVT, BVT, LISTENERS = T, T + 0x10, T + 0x40, T + 0x80, T + 0xa0, T + 0xc0
    WORLD, ACTIVE, INACTIVE, WORLD_LISTENERS, DEFERRED = T + 0x100, T + 0x300, T + 0x380, T + 0x400, T + 0x440
    ISLANDS, ISLAND_BODIES, BODIES, BODY_LISTENERS = T + 0x500, T + 0x900, T + 0xb00, T + 0x1800
    COMPONENT, COMPONENT_ROWS = T + 0x2000, T + 0x2100
    FREE, SMALL, GROW, GROW_DATA = T + 0x7000, T + 0x8000, T + 0x5000, T + 0x5100
    # Free-list chains: distinct 0x100-byte blocks per pool class (P: 2,
    # P2: 4), never shared, so a block handed out (and overwritten) is never
    # popped again. FREE holds data the cases release.
    CHAINS = {'primary': (T + 0xa000, 2), 'secondary': (T + 0xc000, 4)}
    v.REGISTER_JUMPS.update(REGISTER_JUMPS)
    for site, targets in list(PHANTOM_JUMPS.items()) + list(VELOCITY_JUMPS.items()) + list(ENTITY_JUMPS.items()):
        v.REGISTER_JUMPS[site] = sorted(set(v.REGISTER_JUMPS.get(site, [])) | set(targets))
    v.CLOSURE_STOPS.add(0x149930)
    install_fs_tib(v)
    image = v.Xbe(v.NEW_DEFAULT)

    def kernel(name, kind, entry, **routine):
        v.KERNELS[name] = ([name], kind)
        v.ROUTINES[name] = dict(entry=entry, **routine)

    # ---- shared fixture: allocator, pools, globals ----
    def pool(world, rng, base, tag):
        """A 233250 pool at `base`: class tables, free heads (TRACK blocks or
        empty), small region, counters and page 0."""
        header = bytearray(rng.getrandbits(8) for _ in range(0x340))
        world.extra[base] = header
        world.extra[base + PAGE] = bytearray(rng.getrandbits(8) for _ in range(0x2000))
        world.extra[base + COUNTERS] = bytearray(rng.getrandbits(8) for _ in range(0x90))
        put = world.put
        put(base, PVT)
        sizes = [16 << k for k in range(10)] + [0x400 * (k + 1) for k in range(1, 7)]  # 16..0x200, 0x800..0x1c00
        sizes[15] = 0x2000
        for k, size in enumerate(sizes):
            put(base + 0x68 + 4 * k, size)
        for size in range(0x201):
            header[0xac + size] = next(k for k in range(10) if sizes[k] >= size)
        for k in range(8):
            put(base + 0x2b0 + 4 * k, min(15, 10 + k))
        chains, length = CHAINS[tag]
        for k in range(16):
            # Free heads: this class's own chain, or empty (P carves).
            blocks = [chains + 0x100 * (length * k + j) for j in range(rng.randrange(1, length + 1))]
            for block, following in zip(blocks, blocks[1:] + [0]):
                put(block, following)
            put(base + 0x24 + 4 * k, blocks[0] if tag == 'secondary' or rng.random() < .6 else 0)
            for counter in (0x2e8, COUNTERS + 4, COUNTERS + 0x48):  # in use, total, free
                put(base + counter + 4 * k, rng.choice([0, 1, 7, 0x7fffffff, rng.getrandbits(32)]))
        put(base + 0x2d0, rng.getrandbits(32))
        # Small region: a TRACK window (carving from the cursor) or exhausted.
        start = SMALL + 0x1000 * (tag == 'secondary')
        put(base + 0x18, start)
        put(base + 0x1c, start + rng.choice([0x40, 0x200, 0xfc0]))
        put(base + 0x20, start + rng.choice([0, 0x10, 0x100, 0x200, 0xfc0]))
        # Pages: 0 (a new page at +0x4340) or exhausted (0x76+).
        put(base + COUNTERS, rng.choice([0, 0, 0, 0x76, 0x80]) if tag == 'primary' else 0)
        return sizes

    def fixture(world, rng, mode=None, exhaustible=False):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        put = world.put
        put(ALLOC, AVT)
        put(AVT + 0x10, 0x149da0)
        put(AVT + 0x14, 0x149e00)
        put(PVT + 0x10, 0x233250)
        put(PVT + 0x14, 0x233420)
        put(LVT + 4, 0x2e4d60)
        put(LVT + 8, 0x2e4d60)
        for k in range(8):
            put(LISTENERS + 4 * k, LVT)
        pool(world, rng, P, 'primary')
        pool(world, rng, P2, 'secondary')
        mode = rng.choice([0, 0, 0, 1, 2]) if mode is None else mode
        fixed = rng.choice([P, P2])
        world.globals.update({0x48c938: ALLOC, 0x486064: mode, 0x486068: fixed, 0x48606c: P, 0x486070: P2})
        if not exhaustible:
            # P serves every allocation (a null block would be copied to by
            # 2E3250, which has no check). P2 only takes releases: a P2 page
            # carve would rewrite P2 +18/+1C, 149E00's secondary range.
            world.put(P + COUNTERS, 0)
            world.globals[0x486068] = P
        # Secondary range ([P2+18], size [P2+1C]) covering TRACK releases.
        world.put(P2 + 0x18, T)
        world.put(P2 + 0x1c, 0x10000)
        # 283450 reservation globals (bodies here are below 0x80061000).
        world.extra[0x556588] = bytearray([rng.choice([0, 1, 0xff])])
        world.extra[0x5565ac] = bytearray(rng.getrandbits(8) for _ in range(4))

    # ---- [0x48C938] +10 (149DA0) and +14 (149E00) ----
    kernel('havok_array_allocate', 'long', 0x149da0, stack=[0, 1])

    def allocate_case(name, world, rng):
        fixture(world, rng, exhaustible=True)
        size = rng.choice([0, 1, 4, 15, 16, 17, 0x40, 0x7f, 0x100, 0x1ff, 0x200, 0x201, 0x400, 0x401, 0x800,
                           0xfff, 0x1000, 0x1c00, 0x1fff, 0x2000, rng.randrange(1, 0x2001)])
        if world.globals[0x486064] == 1 and rng.random() < .3:
            size = rng.choice([0x2001, 0x4000, 0x7fffffff])  # the fixed tail returns 233250's null
        return [size, rng.choice([0x12, 0x2c, 0, rng.getrandbits(32)])]

    # ---- 283450 reserved-body predicate (cdecl body; compares only) ----
    kernel('havok_reserved_body', 'boolean', 0x283450, stack=[0])

    def reserved_case(name, world, rng):
        world.extra[0x556588] = bytearray([rng.choice([0, 1, 0x80, 0xff])])
        world.extra[0x5565ac] = bytearray(struct.pack('<I', rng.choice([0, 0x10, 0x1000, 0x7fffffff, 0x80000000,
                                                                          rng.getrandbits(32)])))
        size = struct.unpack('<I', world.extra[0x5565ac])[0] if world.extra[0x556588][0] else 0
        edge = (0x80061000 + size) & 0xffffffff
        return [rng.choice([0, 0x1500b00, 0x7fffffff, 0x80000000, 0x80060fff, 0x80061000, 0x80061001,
                            edge, (edge - 1) & 0xffffffff, (edge + 1) & 0xffffffff, 0xffffffff, rng.getrandbits(32)])]

    kernel('havok_array_release', 'void', 0x149e00, stack=[0, 1, 2])

    def release_case(name, world, rng):
        fixture(world, rng)
        size = rng.choice([0, 1, 16, 0x40, 0x200, 0x201, 0x800, 0x1000, 0x2000, rng.randrange(1, 0x2001)])
        data = rng.choice([0,
                           P + PAGE + 4 * rng.randrange(0x7f0),             # primary range
                           P + 0x330, P2 + 0x330,                            # fixed range (fixed = P or P2)
                           FREE + 4 * rng.randrange(0x400), T + 0x200])      # secondary range (TRACK)
        if data in (P + 0x330, P2 + 0x330):
            world.globals[0x486068] = data - 0x330
        return [data, size, rng.choice([0x12, 0x2c, rng.getrandbits(32)])]

    # ---- 2E3250 array growth (cdecl header, stride) ----
    kernel('havok_grow_array', 'void', 0x2e3250, stack=[0, 1])

    def grow_case(name, world, rng):
        fixture(world, rng)
        count = rng.choice([0, 0, 1, 2, 3, 5, 8])
        stride = rng.choice([1, 2, 3, 4, 4, 5, 8, 12])
        data = rng.choice([GROW_DATA, GROW_DATA + 1, GROW_DATA + 2] + ([0] if count == 0 else []))
        world.put(GROW, data)
        world.put(GROW + 4, count)
        world.put(GROW + 8, rng.choice([count, count + 1, 16]) | rng.choice([0, 0, 0x80000000]))
        return [GROW, stride]

    # ---- 315CB0 body activation and 1D9F10 component activation ----
    def physics_world(world, rng):
        fixture(world, rng)
        put = world.put
        islands = list(range(7))
        rng.shuffle(islands)
        split = rng.randrange(0, 4)
        active, inactive, detached = islands[:split], islands[3:6], islands[split:3] + islands[6:]
        for position, i in enumerate(active):
            put(ACTIVE + 4 * position, ISLANDS + 0x80 * i)
            put(ISLANDS + 0x80 * i + 0x24, position)
            world.track[ISLANDS + 0x80 * i + 0x29 - T] = rng.choice([1, 1, 0x80, 2])
        for position, i in enumerate(inactive):
            put(INACTIVE + 4 * position, ISLANDS + 0x80 * i)
            put(ISLANDS + 0x80 * i + 0x24, position)
            world.track[ISLANDS + 0x80 * i + 0x29 - T] = 0
        for i in detached:
            put(ISLANDS + 0x80 * i + 0x24, NONE)
            world.track[ISLANDS + 0x80 * i + 0x29 - T] = rng.choice([0, 0, 1])
        put(WORLD + 8, ACTIVE)
        put(WORLD + 0xc, len(active))
        put(WORLD + 0x10, rng.choice([len(active), len(active), len(active) + 1, 16]) | rng.choice([0, 0x80000000]))
        put(WORLD + 0x14, INACTIVE)
        put(WORLD + 0x18, len(inactive))
        # World listeners: default-listener objects or null (compacted).
        listeners = rng.randrange(0, 5)
        put(WORLD + 0x78, WORLD_LISTENERS)
        put(WORLD + 0x7c, listeners)
        for k in range(listeners):
            put(WORLD_LISTENERS + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8), 0, LISTENERS]))
        bodies = [BODIES + 0x100 * b for b in range(12)]
        for i in range(7):
            island = ISLANDS + 0x80 * i
            members = rng.sample(bodies, rng.randrange(0, 4))
            put(island + 0x3c, ISLAND_BODIES + 0x40 * i)
            put(island + 0x40, len(members))
            for k, body in enumerate(members):
                put(ISLAND_BODIES + 0x40 * i + 4 * k, body)
        for b, body in enumerate(bodies):
            put(body, BVT)
            put(body + 8, WORLD if rng.random() < .9 else 0)
            put(body + 0x44, ISLANDS + 0x80 * rng.randrange(7))
            count = rng.randrange(0, 4)
            put(body + 0x70, BODY_LISTENERS + 0x40 * b)
            put(body + 0x74, count)
            for k in range(count):
                put(BODY_LISTENERS + 0x40 * b + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8), 0]))
        # Deferred references: each body's count stays nonzero (no destructor).
        deferred = [rng.choice(bodies) for _ in range(rng.randrange(0, 6))]
        put(WORLD + 0x108, DEFERRED)
        put(WORLD + 0x10c, len(deferred))
        for k, body in enumerate(deferred):
            put(DEFERRED + 4 * k, body)
        for body in bodies:
            uses = deferred.count(body)
            struct.pack_into('<H', world.track, body + 6 - T, rng.choice([0, uses + 1, uses + 2, 0x8000 + uses]))
        return bodies

    kernel('havok_activate_body', 'void', 0x315cb0, regs={'ecx': 0})

    def activate_case(name, world, rng):
        return [rng.choice(physics_world(world, rng))]

    kernel('havok_component_activate', 'void', 0x1d9f10, regs={'esi': 0})

    def component_case(name, world, rng):
        bodies = physics_world(world, rng)
        put = world.put
        put(COMPONENT + 4, rng.getrandbits(32) | 0x20 if rng.random() < .85 else rng.getrandbits(32) & ~0x20)
        count = rng.choice([0, 1, 2, 3, 4])
        put(COMPONENT + 0x70, COMPONENT_ROWS)
        put(COMPONENT + 0x74, count if rng.random() < .9 else rng.choice([NONE, 0x80000000]))
        for k in range(4):
            put(COMPONENT_ROWS + 0x60 * k + 0x40, rng.choice(bodies))
        return [COMPONENT]

    # ---- 1CD6A0: object component velocities, including force=0/1 ----
    kernel('havok_object_component_velocities', 'void', 0x1cd6a0,
           regs={'eax': 0, 'ebx': 1}, stack=[2, 3])
    COMPONENT_POOL, MOTIONS, MOTION_VTABLES, INPUTS = T + 0x2400, T + 0x2600, T + 0x3300, T + 0x3900

    def component_velocities_case(name, world, rng):
        bodies = physics_world(world, rng)
        put = world.put
        world.globals[0x52be5c] = COMPONENT_POOL
        put(COMPONENT_POOL + 0x44, COMPONENT)
        count = rng.choice([0, 1, 2, 3, 4, 4, NONE, 0x80000000])
        put(COMPONENT + 4, rng.getrandbits(32))
        put(COMPONENT + 0x70, COMPONENT_ROWS)
        put(COMPONENT + 0x74, count)
        world.track[COMPONENT + 0x18 - T] = rng.choice([0, 0, 1, 2, 3, 4, 0x7f, 0x80, 0xff])
        for k in range(4):
            put(COMPONENT_ROWS + 0x60 * k + 0x40, rng.choice(bodies))
        for k, body in enumerate(bodies):
            motion, vtable = MOTIONS + k * 0x100, MOTION_VTABLES + k * 0x80
            put(body + 0x3c, motion)
            world.track[body + 0x40 - T] = rng.choice([0, 0, 0, 1, 0x80])
            if rng.random() < .2:
                put(body + 0x44, 0)
                put(body + 8, 0)  # a detached body has neither island nor world
            put(motion, vtable)
            put(vtable + 0x18, rng.choice(MOTION_KINDS))
            put(vtable + 0x50, 0x3239f0)
            put(vtable + 0x54, 0x323a00)
        n = rng.randrange(v.ROWS - 1)
        datum = rng.choice([0, 0, 0x12340000, 0xffff0000, NONE])
        put(world.body(n) + 0xb4, datum)
        # Original type shifts use the low five bits; no datum salt check.
        world.data[v.RECORDS + 12 * n + 3 - v.DATA] = rng.choice([0, 1, 1, 2, 6, 7, 0x21, 0xe1, 0xff])
        linear = rng.choice([0, INPUTS, INPUTS + 1, MOTIONS + 0x40, MOTIONS + 0x50])
        angular = rng.choice([0, INPUTS + 0x10, INPUTS + 3, linear, MOTIONS + 0x40, MOTIONS + 0x50])
        # Raw words exercise NaNs, denormals and signed zero without converting
        # them in the fixture. The owner copies, never evaluates, these lanes.
        words = [0, 0x80000000, 0x7fc12345, 0x7f800001, 1, 0x80000001,
                 0x3f800000, 0xbf800000, 0x7f800000, 0xff800000]
        for at in (INPUTS, INPUTS + 0x10):
            for lane in range(3):
                put(at + 4 * lane, rng.choice(words + [rng.getrandbits(32)]))
        force = rng.choice([0, 0, 1, 2, 0x80, 0x100, 0xffffff00, NONE])
        return [(rng.getrandbits(16) << 16) | n, linear, angular, force]

    # ---- 31C3B0 phantom info (ECX) and 3154E0 phantom constructor (ECX, info) ----
    kernel('havok_aabb_phantom_info', 'long', 0x31c3b0, regs={'ecx': 0})

    def phantom_info_case(name, world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        return [T + 0x100 + 0x10 * rng.randrange(64)]

    kernel('havok_aabb_phantom_construct', 'long', 0x3154e0, regs={'ecx': 0}, stack=[1])

    def phantom_construct_case(name, world, rng):
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        phantom = T + 0x1000 + 0x10 * rng.randrange(128)
        # Aliased info: its reads follow the constructor's earlier stores.
        info = rng.choice([T + 0x10 * rng.randrange(0xc0), phantom, phantom + 0x30, phantom + 0x40])
        return [phantom, info]

    # ---- 1D8470 component reset (stack component) ----
    kernel('havok_component_reset', 'void', 0x1d8470, stack=[0])
    RECORDS7C, ROWS70, PROPERTIES = T + 0x2200, T + 0x2300, T + 0x6000

    def reset_case(name, world, rng):
        fixture(world, rng)
        put = world.put
        put(BVT, 0x2e4d60)
        bodies = [BODIES + 0x100 * b for b in range(12)]
        keys = [0x2001, 0x2002, 0x2009, 0x12340001, 0]
        for b, body in enumerate(bodies):
            put(body, BVT)
            struct.pack_into('<H', world.track, body + 6 - T, rng.choice([0, 1, 1, 2, 3, 0xffff]))
            count = rng.randrange(0, 5)
            put(body + 0x30, PROPERTIES + 0x80 * b)
            put(body + 0x34, count)
            put(body + 0x38, rng.choice([count, count, count + 1, 8]) | rng.choice([0, 0x80000000]))
            for k, key in enumerate(rng.sample(keys, count)):
                put(PROPERTIES + 0x80 * b + 8 * k, key)
        put(COMPONENT + 0x7c, RECORDS7C)
        put(COMPONENT + 0x80, rng.choice([0, 0, 1, 2, 3, NONE]))
        for k in range(4):
            put(RECORDS7C + 12 * k + 8, rng.choice(bodies))
        put(COMPONENT + 0x70, ROWS70)
        put(COMPONENT + 0x74, rng.choice([0, 1, 2, 3, NONE]))
        for k in range(3):
            row = ROWS70 + 0x60 * k
            put(row + 0x40, rng.choice(bodies))
            put(row + 0x48, rng.choice([0, FREE + 0x100 * k, FREE + 0x100 * k + 4]))
            put(row + 0x50, rng.choice([0, 1, 4, 0x10, 0x200, 0x80000000, NONE]))
        put(COMPONENT + 0x98, rng.choice([0, 0] + bodies))
        put(COMPONENT + 0x9c, rng.choice([0, 0] + bodies))
        return [COMPONENT]

    # ---- 1CC310 (component state flags): the no-component path ----
    # A global 0x1CC310 closure stop (any track) would empty this routine.
    # (Registered with the entity kernels below: its component paths use them.)

    # ---- hk3AxisSweep broadphase, hkWorld phantoms, 1CE340 ----
    # Broadphase BP (+10/+20 offsets, +30 scale, +40 nodes, +4C/+58/+64 axes,
    # +70/+74/+78 markers), the arena ([0x48C938] +8/+C/+14 over ARENA, slow
    # path backing BIG or a new P page), the world's phantom fields (+54
    # listeners, +C4, +D0 filter, +120 phantoms, +130 ids, +138 dispatcher,
    # +234 pair reserve), up to 4 hkAabbPhantoms (0xA0 apart) and their arrays.
    BP, BPVT, NODES = T + 0x2400, T + 0x2480, T + 0x2500
    AXES, CELLS, CELL_LISTS = (T + 0x2600, T + 0x2680, T + 0x2700), T + 0x2780, T + 0x2800
    PAIRS, PAIR_DATA, AABB, HANDLES = T + 0x2900, T + 0x2910, T + 0x2980, T + 0x2a00
    PHANTOMS, PHANTOM_VT, SHAPE = T + 0x2b00, T + 0x2d80, T + 0x2dc0
    ARENA, BIG, BIG2 = T + 0x3000, T + 0x3800, T + 0x6800
    # The large broadphase variant (33..45 nodes: two bitset words).
    # (Node capacity 64: no growth, whose 0xC00-class block would take P's page 0.)
    NODES_L, AXES_L, HANDLES_L, PAIR_DATA_L = T + 0x5300, (T + 0x5700, T + 0x5900, T + 0x5b00), T + 0x5d00, T + 0x6000
    DISPATCH, PHANDLER, PHANDLER_VT, FILTER_PARENT, FVT = T + 0x4900, T + 0x4a00, T + 0x4a10, T + 0x4a40, T + 0x4b00
    WORLD_PHANTOM_LISTENERS, WORLD_PHANTOMS, OVERLAPS, PHANTOM_LISTS = T + 0x4b40, T + 0x4b80, T + 0x4c00, T + 0x4d00
    AABB_PHANTOM_VT = 0x41d87c
    images = {address: bytearray(image.read(address, size)) for address, size in
              ((AABB_PHANTOM_VT, 0x40), (0x41c680, 0x30))}

    def floats(world, address, values):
        for k, value in enumerate(values):
            struct.pack_into('<f', world.track, address + 4 * k - T, value)

    def arena(world, rng):
        put = world.put
        put(ALLOC + 8, ARENA + 0x10 * rng.randrange(8))
        put(ALLOC + 0xc, rng.choice([0x700, 0x700, 0x700, 0x100, 0x40, 0x10, 0]))
        put(ALLOC + 0x14, rng.choice([BIG + 0x10, T + 0x37f0]))  # never a fast block
        put(AVT + 0x24, 0x2e10a0)
        put(AVT + 0x28, 0x2e1110)
        # 2E10A0's 0x1010-byte backing is class 14, served by BIG then BIG2
        # (1CE340 can need one per add and remove; a new P page could follow
        # a small carve's page 0, outside the bank). Released ones go to P2.
        put(P + 0x24 + 4 * 14, BIG)
        put(BIG, BIG2)
        put(BIG2, 0)
        world.extra.update({address: bytearray(data) for address, data in images.items()})

    def sap(world, rng, handles, nodes_max=14, large=False, cluster=None, clustered=(), detached=True):
        """A consistent hk3AxisSweep: node 0 is the sentinel (both axis ends),
        marker nodes cover the x buckets of +74 bits, objects own `handles`
        (node +C, [handle] = index). Sometimes node 1 is detached: on no axis,
        its position words have bit 15 set (PCMPGTW is signed). Returns the
        node specs."""
        put = world.put
        nodes_at, axes = (NODES_L, AXES_L) if large else (NODES, AXES)
        bits = rng.choice([0, 0, 1, 2]) if len(handles) + 4 <= nodes_max else 0
        shift = 16 - bits
        markers = [dict(code=((b - 1) * 16) | 1, marker=b, x=(b << shift, ((b + 1) << shift) - 1),
                        y=(0, 0xffff), z=(0, 0xffff)) for b in range(1, 1 << bits)]
        used = [{m[a][e] for m in markers for e in (0, 1)} for a in 'xyz']
        objects = []
        for handle in handles:
            spec = dict(code=handle)
            for axis, name in enumerate('xyz'):
                while True:
                    center, width = rng.randrange(0x40, 0x7fc0), rng.choice([2, 0x40, 0x400, 0x1000, 0x2000])
                    if cluster and handle in clustered:
                        # Entity kernels: overlapping neighbours around one point.
                        center = cluster[axis] + rng.randrange(-0x300, 0x300)
                    k0, k1 = max(1, center - rng.randrange(1, width)), min(0x7ffe, center + rng.randrange(1, width))
                    low, high = 2 * k0, 2 * k1 + 1
                    if low < high and not {low, high} & used[axis]:
                        break
                used[axis] |= {low, high}
                spec[name] = (low, high)
            objects.append(spec)
        nodes = markers + objects
        rng.shuffle(nodes)
        nodes = [dict(code=HANDLES + 0xf0, x=(0, 0xffff), y=(0, 0xffff), z=(0, 0xffff))] + nodes
        if detached and rng.random() < .3:
            nodes.insert(1, dict(code=HANDLES + 0xd0, detached=[rng.choice([rng.getrandbits(16), 0x8000 | rng.getrandbits(15)])
                                                                for _ in range(6)]))
        count = len(nodes)
        for axis, name in enumerate('xyz'):
            ends = []
            for index, node in enumerate(nodes):
                if 'detached' in node:
                    continue
                low, high = node[name]
                # Equal values: a later minimum follows, a later maximum precedes.
                ends.append(((low, 0, index), index, 0))
                ends.append(((high, 1, -index), index, 1))
            ends.sort()
            # A capacity at most half the count makes 2EAFB0 reserve count + 2.
            entries = len(ends)
            capacity = 128 if large else rng.choice([entries, entries, entries + 2, 32, entries // 2])
            capacity |= rng.choice([0, 0, 0x80000000])
            put(BP + 0x4c + 12 * axis, axes[axis])
            put(BP + 0x50 + 12 * axis, entries)
            put(BP + 0x54 + 12 * axis, capacity)
            for position, (key, index, end) in enumerate(ends):
                struct.pack_into('<HH', world.track, axes[axis] + 4 * position - T, key[0], index)
                nodes[index][name + 'p' + str(end)] = position
        for index, node in enumerate(nodes):
            words = node.get('detached') or [node['yp0'], node['zp0'], node['yp1'], node['zp1'], node['xp0'], node['xp1']]
            struct.pack_into('<HHHHHH', world.track, nodes_at + 16 * index - T, *words)
            put(nodes_at + 16 * index + 12, node['code'])
            if not node['code'] & 1:
                put(node['code'], index)
        put(BP, BPVT)
        put(BPVT + 4, 0x2ec4b0)
        put(BPVT + 0xc, 0x2ec9b0)
        put(BP + 0x40, nodes_at)
        put(BP + 0x44, count)
        put(BP + 0x48, (64 if large else count if count <= 8 and rng.random() < .4 else 16) | rng.choice([0, 0x80000000]))
        put(BP + 0x70, rng.choice([1, 0x100]) if bits else 0)
        put(BP + 0x74, bits)
        put(BP + 0x78, CELLS)
        scale = rng.choice([1.0, 1.0, 1.0, 0.5, 2.0, 0.37])
        floats(world, BP + 0x10, [rng.choice([0.0, 0.0, 3.5, -100.25]) for _ in range(4)])
        floats(world, BP + 0x20, [rng.choice([0.0, 0.0, 0.75, 7.0]) for _ in range(4)])
        floats(world, BP + 0x30, [scale] * 3 + [rng.choice([1.0, 0.0])])
        for node in nodes:
            if 'marker' not in node:
                continue
            k = node['marker'] - 1
            low, high = node['x']
            members = [i for i, other in enumerate(nodes)
                       if 'marker' not in other and 'x' in other and i and other['x'][0] <= high and other['x'][1] >= low]
            # Perturbed lists (a missing or a foreign member) stay single-shot:
            # entity worlds remove several bodies in sequence (detached=False).
            if detached and members and rng.random() < .2:
                members.pop(rng.randrange(len(members)))
            if detached and rng.random() < .15:
                members.append(rng.randrange(1, count))
            members = members[:12]
            put(CELLS + 16 * k, nodes.index(node))
            put(CELLS + 16 * k + 4, CELL_LISTS + 0x20 * k)
            put(CELLS + 16 * k + 8, len(members))
            put(CELLS + 16 * k + 12, rng.choice([len(members), len(members) + 2, 16]) | rng.choice([0, 0x80000000]))
            for n, member in enumerate(members):
                struct.pack_into('<H', world.track, CELL_LISTS + 0x20 * k + 2 * n - T, member)
        return nodes

    def aabb(world, rng, address, nodes):
        """AABB floats near existing objects (fractions exercise rounding),
        with clamps and non-finite values."""
        scale = struct.unpack_from('<f', world.track, BP + 0x30 - T)[0]
        offsets = [struct.unpack_from('<4f', world.track, BP + o - T) for o in (0x10, 0x20)]
        target = rng.choice([node for node in nodes[1:] if 'x' in node])
        low, high = [], []
        for axis, name in enumerate('xyz'):
            a, b = target[name] if rng.random() < .8 else sorted(rng.sample(range(0, 0x10000), 2))
            a, b = (a + rng.randrange(-0x800, 0x400)) & 0xffff, (b + rng.randrange(-0x400, 0x800)) & 0xffff
            a, b = min(a, b), max(a, b)
            fraction = rng.choice([0.0, 0.25, 0.5, 0.75, 0.999, rng.random()])
            low.append((a + fraction) / scale - offsets[0][axis] if scale else a)
            high.append((b + fraction) / scale - offsets[1][axis] if scale else b)
        for values in (low, high):
            if rng.random() < .1:
                values[rng.randrange(3)] = rng.choice([float('nan'), float('inf'), -float('inf'), 1e-40, -1e-40,
                                                       -5000.0, 1e9, 65532.5, 65531.996])
            values.append(rng.choice([0.0, 1.0, float('nan')]))
        floats(world, address, low)
        floats(world, address + 16, high)

    def pairs_header(world, rng, address, data):
        count = rng.choice([0, 0, 1, 3])
        world.put(address, data)
        world.put(address + 4, count)
        world.put(address + 8, rng.choice([count, count + 1, 12]) | rng.choice([0, 0x80000000]))

    kernel('havok_broadphase_add', 'void', 0x2ec4b0, regs={'ecx': 0}, stack=[1, 2, 3])

    def broadphase(world, rng):
        """Small (1..12 objects) or large (32..44: two bitset words, the
        8-node skips) broadphase with its pair header."""
        fixture(world, rng)
        arena(world, rng)
        large = rng.random() < .15
        if large:
            handles = [HANDLES_L + 0x10 * k for k in range(rng.randrange(32, 45))]
            nodes = sap(world, rng, handles, 48, large=True)
            world.put(PAIRS, PAIR_DATA_L)
            world.put(PAIRS + 4, rng.choice([0, 3]))
            world.put(PAIRS + 8, 64 | rng.choice([0, 0x80000000]))
        else:
            handles = [HANDLES + 0x10 * k for k in range(rng.randrange(1, 13))]
            nodes = sap(world, rng, handles)
            pairs_header(world, rng, PAIRS, PAIR_DATA)
        return handles, nodes

    def broadphase_add_case(name, world, rng):
        handles, nodes = broadphase(world, rng)
        aabb(world, rng, AABB, nodes)
        return [BP, HANDLES_L + 0x2f0, AABB, PAIRS]

    kernel('havok_broadphase_remove', 'void', 0x2ec9b0, regs={'ecx': 0}, stack=[1, 2])

    def broadphase_remove_case(name, world, rng):
        handles, nodes = broadphase(world, rng)
        count = struct.unpack_from('<I', world.track, BP + 0x44 - T)[0]
        if count <= 6 and rng.random() < .15:
            world.put(BP + 0x48, count - 2)  # a short capacity: the shrink reserves (2E31E0)
        return [BP, rng.choice(handles), PAIRS]

    def phantom(world, rng, address, index, vtable=AABB_PHANTOM_VT):
        """An hkAabbPhantom (3154E0 layout) with its arrays."""
        put = world.put
        put(address, vtable)
        struct.pack_into('<HH', world.track, address + 4 - T, rng.choice([0x90, 0x90, 0xa0, 0x100, 0x40]),
                         rng.choice([1, 1, 2, 3]))
        put(address + 8, WORLD)
        put(address + 0xc, SHAPE if rng.random() < .15 else 0)
        put(address + 0x1c, 0)
        put(address + 0x20, rng.getrandbits(32))
        put(address + 0x24, 2)
        put(address + 0x28, rng.choice([0, 0, 3, 0x10003, rng.getrandbits(32)]))
        put(address + 0x2c, address)
        put(address + 0x30, 0)
        put(address + 0x34, 0)
        put(address + 0x38, rng.choice([0x80000000, 0x80000000, 0]))
        lists = PHANTOM_LISTS + 0x40 * index
        for offset, data in ((0x3c, lists), (0x48, lists + 0x20)):
            count = rng.choice([0, 0, 1, 2])
            put(address + offset, data)
            put(address + offset + 4, count)
            put(address + offset + 8, rng.choice([count, count + 2, 8]) | rng.choice([0, 0x80000000]))
            for k in range(count):
                put(data + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8), 0]))
        floats(world, address + 0x60, [rng.uniform(0, 30000) for _ in range(4)])
        floats(world, address + 0x70, [rng.uniform(30000, 65000) for _ in range(4)])
        put(address + 0x80, OVERLAPS + 0x40 * index)
        put(address + 0x84, 0)
        put(address + 0x88, rng.choice([8, 0x80000008, 0]))

    def phantom_world(world, rng, cluster=None, entity_count=None, detached=True):
        """physics_world plus the broadphase, the world's phantom fields, entity
        handles (body +1C: index, +20 id, +24 type 1, +28 filter info, +2C
        owner, +40 byte) and 1..3 world phantoms. Returns (bodies, phantoms)."""
        bodies = physics_world(world, rng)
        arena(world, rng)
        put = world.put
        put(LVT, 0x2e4d60)
        world.globals[0x52be48] = WORLD
        count = rng.randrange(0, 4)
        put(WORLD + 0x54, WORLD_PHANTOM_LISTENERS)
        put(WORLD + 0x58, count)
        for k in range(count):
            put(WORLD_PHANTOM_LISTENERS + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8), 0]))
        put(WORLD + 0xc4, BP)
        put(WORLD + 0xd0, FILTER_PARENT)
        put(WORLD + 0x130, rng.getrandbits(32))
        put(WORLD + 0x138, DISPATCH)
        put(WORLD + 0x234, rng.choice([0, 2, 4, 8, 16]))
        for a in range(8):
            for b in range(8):
                put(DISPATCH + 4 * (8 * a + b), PHANDLER if {a, b} <= {1, 2} and 2 in (a, b) else 0)
        put(PHANDLER, PHANDLER_VT)
        put(PHANDLER_VT + 4, 0x31ffc0)
        put(PHANDLER_VT + 8, 0x320040)
        put(FILTER_PARENT + 8, FVT)
        put(FVT, 0x2e9a00)
        for group in range(32):
            put(FILTER_PARENT + 8 + 0x14 + 4 * group, rng.choice([0xffffffff, 0xffffffff, rng.getrandbits(32)]))
        # A phantom vtable whose overlap removal (+20) is the default 2E4D60.
        for k in range(16):
            put(PHANTOM_VT + 4 * k, struct.unpack_from('<I', images[AABB_PHANTOM_VT], 4 * k)[0])
        put(PHANTOM_VT + 0x20, 0x2e4d60)
        put(SHAPE, 0)
        struct.pack_into('<H', world.track, SHAPE + 6 - T, rng.choice([2, 3, 0x10]))
        entities = rng.sample(bodies, entity_count or rng.randrange(1, 7))
        for body in entities:
            put(body + 0x1c, 0)
            put(body + 0x20, rng.getrandbits(32))
            put(body + 0x24, 1)
            put(body + 0x28, rng.choice([0, 0, 3, 0x10003, rng.getrandbits(32)]))
            put(body + 0x2c, body)
            world.track[body + 0x40 - T] = rng.choice([0, 0, 1])
        phantoms = [PHANTOMS + 0xa0 * k for k in range(rng.randrange(1, 4))]
        for k, address in enumerate(phantoms):
            phantom(world, rng, address, k, rng.choice([AABB_PHANTOM_VT] * 3 + [PHANTOM_VT]))
        handles = [body + 0x1c for body in entities] + [address + 0x1c for address in phantoms]
        nodes = sap(world, rng, handles, 12, cluster=cluster, clustered={body + 0x1c for body in entities},
                    detached=detached)
        # Each phantom overlaps a few of the others' collidables.
        collidables = [h - 0x10 for h in handles]
        for k, address in enumerate(phantoms):
            members = [c for c in rng.sample(collidables, rng.randrange(0, min(4, len(collidables)) + 1))
                       if c != address + 0xc]
            put(address + 0x84, len(members))
            for n, c in enumerate(members):
                put(OVERLAPS + 0x40 * k + 4 * n, c)
        inside = [a for a in phantoms if rng.random() < .9]
        put(WORLD + 0x120, WORLD_PHANTOMS)
        put(WORLD + 0x124, len(inside))
        put(WORLD + 0x128, rng.choice([len(inside), len(inside) + 1, 8]) | rng.choice([0, 0x80000000]))
        for k, address in enumerate(inside):
            put(WORLD_PHANTOMS + 4 * k, address)
        return entities, phantoms, nodes

    kernel('havok_add_phantom', 'long', 0x318550, regs={'ecx': 0}, stack=[1])

    def add_phantom_case(name, world, rng):
        entities, phantoms, nodes = phantom_world(world, rng)
        fresh = PHANTOMS + 0xa0 * 3
        phantom(world, rng, fresh, 3)
        world.put(fresh + 8, 0)
        world.put(fresh + 0xc, 0)
        world.put(fresh + 0x1c, rng.getrandbits(32))
        struct.pack_into('<H', world.track, fresh + 6 - T, rng.choice([0, 1]))
        aabb(world, rng, fresh + 0x60, nodes)
        return [WORLD, fresh]

    kernel('havok_remove_phantom', 'void', 0x318680, regs={'ecx': 0}, stack=[1], closure_stops=PHANTOM_STOPS)

    def remove_phantom_case(name, world, rng):
        entities, phantoms, nodes = phantom_world(world, rng)
        if rng.random() < .1:
            world.put(WORLD + 0xd0, 0)  # no filter: 37C960 only deletes here
        return [WORLD, rng.choice(phantoms)]

    kernel('havok_phantom_wake', 'void', 0x1ce340, regs={'eax': 0}, closure_stops=PHANTOM_STOPS)

    def phantom_wake_case(name, world, rng):
        entities, phantoms, nodes = phantom_world(world, rng)
        world.globals[0x48b874] = rng.choice([0x3d06594b, 0x3d06594b, 0x3d088889, 0x3f800000])
        world.globals[0x46a11c] = struct.unpack('<I', image.read(0x46a11c, 4))[0]
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        scale = struct.unpack_from('<f', world.track, BP + 0x30 - T)[0] or 1.0
        target = rng.choice([node for node in nodes[1:] if 'x' in node])
        for axis, name in enumerate('xyz'):
            low, high = target[name]
            center = (low + high) / 2 / scale + rng.uniform(-300, 300)
            struct.pack_into('<f', world.data, body + 0x30 + 4 * axis - v.DATA, center + rng.choice([0, 0.37, 0.5]))
        struct.pack_into('<f', world.data, body + 0x3c - v.DATA,
                         rng.choice([0.0, 0.5, 40.0, 900.0, 3000.25, rng.uniform(0, 8000)]))
        return [world.handles[n]]

    v.TRACK_ARGUMENTS.update(havok_array_allocate=allocate_case, havok_array_release=release_case,
                             havok_grow_array=grow_case, havok_activate_body=activate_case,
                             havok_component_activate=component_case,
                             havok_object_component_velocities=component_velocities_case,
                             havok_reserved_body=reserved_case,
                             havok_aabb_phantom_info=phantom_info_case,
                             havok_aabb_phantom_construct=phantom_construct_case,
                             havok_component_reset=reset_case, havok_broadphase_add=broadphase_add_case,
                             havok_broadphase_remove=broadphase_remove_case, havok_add_phantom=add_phantom_case,
                             havok_remove_phantom=remove_phantom_case, havok_phantom_wake=phantom_wake_case)
    v.TRACK_COUNTS.update(havok_array_allocate=500, havok_array_release=400, havok_grow_array=400,
                          havok_activate_body=600, havok_component_activate=500,
                          havok_object_component_velocities=1600,
                          havok_reserved_body=300, havok_aabb_phantom_info=100, havok_aabb_phantom_construct=200,
                          havok_component_reset=600, havok_broadphase_add=600, havok_broadphase_remove=600,
                          havok_add_phantom=500, havok_remove_phantom=500, havok_phantom_wake=500)

    # ---- hkWorld entities (3198A0/319370) and component membership
    # (1D9510/1D97F0). phantom_world's broadphase, phantoms, filter and
    # dispatcher, plus full islands (TRACK +6200, 0xC0 apart: 322100
    # releases them into P2's secondary range), the fixed island, shapes,
    # motions, listener/reference arrays and the component in the FACTORY
    # bank. Old body records (released by 1D9C80) sit at TRACK +4E00.
    F = v.FACTORY
    E_ISLANDS, OLD_RECORDS = T + 0x6200, T + 0x4e00
    E_BODY_ARRAYS, E_RECORDS, E_PENDING = F, F + 0x200, F + 0xa00  # pending: +74, +80, +8C lists
    E_MOTIONS, E_MOTION_VT, E_SHAPES, E_SHAPE_VT = F + 0x1000, F + 0x2700, F + 0x1700, F + 0x1880
    E_PROPERTIES, E_ACTIVATION, E_REFERENCES = F + 0x1900, F + 0x1b00, F + 0x1c00
    E_COMPONENT, E_ROWS = F + 0x1e80, F + 0x1f40
    E_DEFERRED_REFS, E_DEFERRED_REMOVALS, E_ENTITY_LISTENERS = F + 0x2100, F + 0x2140, F + 0x2200
    E_SCALE, E_INFO, E_CONSTRAINTS, E_RESULT, E_COMPONENT_RECORDS = F + 0x2300, F + 0x2340, F + 0x2400, F + 0x2600, T + 0x7a00
    ENTITY_STOPS = [0x1d7ca0, 0x16e8a0, 0xba570, 0xba8c0, 0x1496f0, 0x1d73d0, 0x316aa0, 0x317350, 0x22d0a0,
                    0xbbbc0, 0x1cdd60, 0x319750, 0x31e430, 0x31e560, 0x31e590, 0x317200, 0x2e12a0]
    entity_images = {address: bytearray(image.read(address, size)) for address, size in
                     ((0x41f074, 0x30), (0x41d9a0, 0x10), (0x461970, 0x8), (0x46e400, 0x40), (0x46a0c0, 0x20))}

    def entity_world(world, rng, adding=False, admit=True, spread=False):
        """phantom_world plus consistent islands/entities. Returns (entities,
        free bodies, phantoms, nodes, islands, fixed island)."""
        # Most worlds cluster the entities so that they overlap (pair work).
        # `spread`: six clustered entities, the first fixed and each other one
        # in its own island (a fixed body then collects five islands).
        cluster = [rng.randrange(0x1000, 0x7000) for _ in range(3)] if spread or rng.random() < .7 else None
        # No detached SAP node: a marker could pair it, and its handle owns no
        # body (320260 would read its island).
        entities, phantoms, nodes = phantom_world(world, rng, cluster, 6 if spread else None, detached=False)
        world.entity_cluster = cluster
        # Arena slow paths and large blocks can take P's pages 0..2 (a
        # small carve may then need the next page): three pages are banked.
        world.extra[P + PAGE] = bytearray(rng.getrandbits(8) for _ in range(0x6000))
        world.factory = bytearray(rng.getrandbits(8) for _ in range(v.FACTORY_SIZE))
        world.extra.update({address: bytearray(data) for address, data in entity_images.items()})
        put = world.put
        put(LVT + 0xc, 0x2e4d60)
        bodies = [BODIES + 0x100 * b for b in range(12)]
        free = [b for b in bodies if b not in entities]
        # Motion vtables (0x60 apart): +14 323420 (storage 0x80), +18 the
        # kind getter, +50/+54 1D9080/1D9100's velocity setters.
        for k, getter in enumerate(MOTION_KINDS):
            put(E_MOTION_VT + 0x60 * k + 0x14, 0x323420)
            put(E_MOTION_VT + 0x60 * k + 0x18, getter)
            put(E_MOTION_VT + 0x60 * k + 0x50, 0x3239f0)
            put(E_MOTION_VT + 0x60 * k + 0x54, 0x323a00)
        put(E_SHAPE_VT + 0x18, 0x2e8d90)          # sphere (radius +C, center: transform +30)
        put(E_SHAPE_VT + 0x20 + 0x18, 0x2e8120)   # capsule (radius +C, endpoints +10/+20)
        # 1D9510/1D97F0's kind gate: an active component's dynamic bodies.
        # Admitted entity pairs must defer (320DA0): only a new island (+23C
        # set) or the fixed island differs from every other entity's island.
        new_islands = rng.choice([1, 1, 1, 0])
        accept = adding and (spread or admit and new_islands and rng.random() < .5)
        group = 0x10000 * rng.choice([1, 2])
        for b, body in enumerate(bodies):
            motion, shape = E_MOTIONS + 0x80 * b, E_SHAPES + 0x20 * b
            put(motion, E_MOTION_VT + 0x60 * rng.randrange(len(MOTION_KINDS)))
            vals = [rng.choice([1.0, 0.0, -1.0, 0.5, 0.70710677]) for _ in range(12)]
            center = [rng.uniform(100, 30000) for _ in range(3)] + [rng.choice([0.0, 1.0])]
            for k, x in enumerate(vals + center):
                struct.pack_into('<f', world.factory, motion + 0x20 + 4 * k - F, x)
            capsule = rng.random() < .3
            put(shape, E_SHAPE_VT + (0x20 if capsule else 0))
            struct.pack_into('<f', world.factory, shape + 0xc - F, rng.choice([0.0, 0.25, 1.5, 40.0, rng.uniform(0, 600)]))
            for k in range(8):
                struct.pack_into('<f', world.factory, shape + 0x10 + 4 * k - F, rng.uniform(-300, 300))
            put(body + 0xc, shape)
            put(body + 0x14, motion)
            put(body + 0x3c, motion)
            # Properties (0x2002 feeds the record's +64).
            count = rng.randrange(0, 4)
            put(body + 0x30, E_PROPERTIES + 0x20 * b)
            put(body + 0x34, count)
            put(body + 0x38, 4 | 0x80000000)
            for k, key in enumerate(rng.sample([0x2001, 0x2002, 0x2009], count)):
                put(E_PROPERTIES + 0x20 * b + 8 * k, key)
            # Activation (+60) listeners, the +70 entity listeners' capacity.
            count = rng.randrange(0, 3)
            put(body + 0x60, E_ACTIVATION + 0x10 * b)
            put(body + 0x64, count)
            put(body + 0x68, rng.choice([count, count + 1, 4]) | 0x80000000)
            for k in range(count):
                put(E_ACTIVATION + 0x10 * b + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8) + 4, 0]))
            count = struct.unpack_from('<I', world.track, body + 0x74 - T)[0]
            put(body + 0x78, rng.choice([count, count + 1, 8]) | rng.choice([0, 0x80000000]))
            # Removal (+7C) and the two +8 subobject reference arrays.
            for n, offset in enumerate((0x7c, 0x88, 0x94)):
                data, count = E_REFERENCES + 0x30 * b + 0x10 * n, rng.randrange(0, 3)
                put(body + offset, data)
                put(body + offset + 4, count)
                # A capacity below the count makes 3174A0 reserve (2E31E0).
                put(body + offset + 8, rng.choice([4, 4, 4, max(count - 1, 0)]) | 0x80000000)
                for k in range(count):
                    put(data + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8) - (8 if n else 0), 0]))
            struct.pack_into('<H', world.track, body + 0x5c - T, rng.getrandbits(16))
            put(body + 0x54, 0)
            # Every body's references stay above the removals (no destructor).
            struct.pack_into('<H', world.track, body + 6 - T, rng.choice([8, 9, 0x40]))
            # Entity filter info: one nonzero system (entity pairs rejected)
            # unless this add case admits them (all islands defer, +30 0).
            put(body + 0x28, (0 if accept else group) | rng.randrange(32))
        for k in range(32):
            put(FILTER_PARENT + 8 + 0x14 + 4 * k, rng.choice([0xffffffff, 0xffffffff, rng.getrandbits(32)]))
        # Islands: k < active listed active, then inactive; the last is fixed.
        count = 5 if spread else rng.randrange(2, 7)
        islands = [E_ISLANDS + 0xc0 * k for k in range(count + 1)]
        fixed_island = islands[-1]
        active = rng.randrange(0, count + 1)
        members = {island: [] for island in islands}
        if spread:
            for n, body in enumerate(entities):
                world.track[body + 0x40 - T] = 0 if n else 1
        for n, body in enumerate(entities):
            island = fixed_island if world.track[body + 0x40 - T] else islands[n - 1] if spread else rng.choice(islands[:-1])
            members[island].append(body)
        # Entity pairs (lower id first) are recorded in one of their islands,
        # deferred (+74) in one, or neither.
        ident = lambda h: struct.unpack_from('<I', world.track, h + 4 - T)[0]
        handles = sorted((b + 0x1c for b in entities), key=ident)
        home = {b + 0x1c: island for island, bodies_in in members.items() for b in bodies_in}
        recorded = {island: [] for island in islands}
        pending = {island: [] for island in islands}
        for i, a in enumerate(handles):
            for b in handles[i + 1:]:
                where = rng.choice([home[a], home[b]])
                roll = rng.random()
                if roll < .45:
                    recorded[where].append((a, b))
                elif roll < .6:
                    pending[where].append((a, b))
        put(WORLD + 0xc, active)
        put(WORLD + 0x10, rng.choice([active, active + 1, 16]) | rng.choice([0, 0x80000000]))
        put(WORLD + 0x18, count - active)
        put(WORLD + 0x2c, fixed_island)
        for k, island in enumerate(islands):
            put(island, 0x41f080)
            struct.pack_into('<HH', world.track, island + 4 - T, 0xb0, rng.choice([1, 2]))
            put(island + 0x14, rng.getrandbits(32))
            put(island + 0x1c, 0x41f074)
            put(island + 0x20, WORLD)
            is_active = island != fixed_island and k < active
            if island == fixed_island:
                put(island + 0x24, NONE)
            else:
                position = k if is_active else k - active
                put((ACTIVE if is_active else INACTIVE) + 4 * position, island)
                put(island + 0x24, position)
            world.track[island + 0x28 - T] = rng.choice([0, 1])
            world.track[island + 0x29 - T] = 1 if is_active else 0
            world.track[island + 0x30 - T] = 0
            bodies_here = members[island]
            put(island + 0x3c, E_BODY_ARRAYS + 0x40 * k)
            put(island + 0x40, len(bodies_here))
            put(island + 0x44, rng.choice([len(bodies_here), len(bodies_here) + 1, 16]) | 0x80000000)
            for n, body in enumerate(bodies_here):
                put(E_BODY_ARRAYS + 0x40 * k + 4 * n, body)
                put(body + 0x44, island)
                put(body + 0x58, n)
                put(body + 8, WORLD)
            # Removal worlds own some arrays in TRACK (322100 releases them).
            for n, offset in enumerate((0x4c, 0x58)):
                owned = not adding and rng.random() < .5
                put(island + offset, T + 0x6740 + 0x10 * k + 8 * n if owned else F + 0x3f00)
                put(island + offset + 4, 0)
                put(island + offset + 8, 2 if owned else 0x80000000)
            # Pair records (+68), sorted, without agents.
            records = recorded[island][:15]
            put(island + 0x68, E_RECORDS + 0x100 * k)
            put(island + 0x6c, len(records))
            put(island + 0x70, rng.choice([len(records), len(records) + 1, 16]) | 0x80000000)
            for n, (a, b) in enumerate(records):
                put(E_RECORDS + 0x100 * k + 16 * n, a)
                put(E_RECORDS + 0x100 * k + 16 * n + 4, b)
                put(E_RECORDS + 0x100 * k + 16 * n + 8, 0)
                put(E_RECORDS + 0x100 * k + 16 * n + 12, rng.choice([0, NONE]))
            # Pending pair lists: deferred (+74, a few real pairs), added
            # (+80) and removed (+8C) empty.
            for n, offset in enumerate((0x74, 0x80, 0x8c)):
                data = E_PENDING + 0x200 * n + 0x40 * k
                entries = [] if n else pending[island][:3]
                owned = n and not adding and rng.random() < .5
                put(island + offset, T + 0x4e00 + 0x20 * k + 0x10 * (n - 1) if owned else data)
                put(island + offset + 4, len(entries))
                put(island + offset + 8, 2 if owned else rng.choice([len(entries), len(entries) + 1, 8]) | 0x80000000)
                for e, (a, b) in enumerate(entries):
                    put(data + 8 * e, a)
                    put(data + 8 * e + 4, b)
            put(island + 0xa8, NONE)
            world.track[island + 0xae - T] = 0
        # World: entity listeners, tolerance, deferred lists, lock, ids.
        count = rng.randrange(0, 4)
        put(WORLD + 0x48, E_ENTITY_LISTENERS)
        put(WORLD + 0x4c, count)
        for k in range(count):
            put(E_ENTITY_LISTENERS + 4 * k, rng.choice([LISTENERS + 4 * rng.randrange(8), 0]))
        put(WORLD + 0xcc, E_INFO)
        struct.pack_into('<f', world.factory, E_INFO + 8 - F,
                         rng.choice([0.1, 0.05, 0.0, 1.0, 0.0333, 1e-40, 2.5, 7.75]))
        put(WORLD + 0x20, E_DEFERRED_REMOVALS)
        put(WORLD + 0x24, 0)
        count = rng.randrange(0, 3)
        put(WORLD + 0xf0, E_DEFERRED_REMOVALS)
        put(WORLD + 0xf4, count)
        put(WORLD + 0xf8, rng.choice([count, count + 1, 8]) | 0x80000000)
        deferred = struct.unpack_from('<I', world.track, WORLD + 0x10c - T)[0]
        put(WORLD + 0x110, rng.choice([deferred, deferred + 1, 8]) | rng.choice([0, 0x80000000]))
        references = rng.sample(entities, min(len(entities), rng.randrange(0, 3)))
        put(WORLD + 0x114, E_DEFERRED_REFS)
        put(WORLD + 0x118, len(references))
        for k, body in enumerate(references):
            put(E_DEFERRED_REFS + 4 * k, body)
        world.track[WORLD + 0x12c - T] = 0
        put(WORLD + 0x134, rng.getrandbits(32))
        world.track[WORLD + 0x23c - T] = new_islands
        put(DISPATCH + 0x24, DISPATCH + 0x100)
        # Globals: the gameplay listener, the entity count, 282C10's flags.
        world.globals[0x52d880] = rng.choice([LISTENERS + 4 * rng.randrange(8), LISTENERS + 4 * rng.randrange(8), 0])
        world.globals[0x48b868] = rng.choice([0, 1, 0x80, 0x351, rng.randrange(0x352)])
        world.globals[0x52d690] = E_CONSTRAINTS
        put(E_CONSTRAINTS + 0x44, E_CONSTRAINTS + 0x80)
        world.extra[0x56dfcc] = bytearray([0])
        world.extra[0x48b873] = bytearray([rng.choice([0, 1])])
        return entities, free, phantoms, nodes, islands, fixed_island

    def fresh_entity(world, rng, body):
        """A body outside the world (as after 1D9510), with an old record."""
        put = world.put
        put(body + 8, 0)
        put(body + 0x44, 0)
        put(body + 0x58, NONE)
        put(body + 0x24, 1)
        put(body + 0x2c, body)
        world.track[body + 0x40 - T] = rng.choice([0, 0, 0, 1])

    def place_near(world, rng, body):
        """Put a fresh sphere body's AABB around the entity cluster (or drop
        its shape: no broadphase, 31E760's -1 extent)."""
        if rng.random() < .1:
            world.put(body + 0xc, 0)
            return
        cluster = world.entity_cluster
        if not cluster:
            return
        shape, motion = struct.unpack_from('<II', world.track, body + 0xc - T)[0], \
            struct.unpack_from('<I', world.track, body + 0x3c - T)[0]
        world.put(shape, E_SHAPE_VT)
        scale = struct.unpack_from('<f', world.track, BP + 0x30 - T)[0] or 1.0
        offsets = struct.unpack_from('<3f', world.track, BP + 0x10 - T)
        struct.pack_into('<f', world.factory, shape + 0xc - F, rng.choice([0x40, 0x200, 0x800]) / scale)
        for axis in range(3):
            q = 2 * cluster[axis] + rng.randrange(-0x300, 0x300)
            struct.pack_into('<f', world.factory, motion + 0x50 + 4 * axis - F, q / scale - offsets[axis])

    def old_record(world, rng, body, k):
        record = OLD_RECORDS + 0x80 * k
        world.put(record, 0x461970)
        struct.pack_into('<HH', world.track, record + 4 - T, 0x70, rng.choice([1, 1, 2]))
        world.put(body + 0x54, record if rng.random() < .8 else 0)

    def entity_listener_member(world, rng, body):
        """Sometimes the gameplay listener is in the body's +70 (and +60) list."""
        listener = world.globals[0x52d880]
        data70 = struct.unpack_from('<I', world.track, body + 0x70 - T)[0]
        count70 = struct.unpack_from('<I', world.track, body + 0x74 - T)[0]
        if count70 and rng.random() < .8:
            world.put(data70 + 4 * rng.randrange(count70), listener)
        data60 = struct.unpack_from('<I', world.track, body + 0x60 - T)[0]
        count60 = struct.unpack_from('<I', world.track, body + 0x64 - T)[0]
        if count60 and rng.random() < .8:
            world.put(data60 + 4 * rng.randrange(count60), listener + 4 if listener else 0)

    def component(world, rng, bodies, adding):
        put = world.put
        n = rng.randrange(v.ROWS - 1)
        flags = rng.getrandbits(32) & ~0x800 if rng.random() < .6 else rng.getrandbits(32)
        put(E_COMPONENT + 4, flags)
        put(E_COMPONENT + 8, world.handles[n])
        put(E_COMPONENT + 0x20, NONE if rng.random() < .8 else rng.randrange(4))
        for k in range(4):
            put(E_CONSTRAINTS + 0x80 + 0x40 * k + 2, 0)
        put(E_COMPONENT + 0x70, E_ROWS)
        put(E_COMPONENT + 0x74, len(bodies))
        put(E_COMPONENT + 0x80, 0)
        records = rng.choice([0, 0, 1, 2])
        put(E_COMPONENT + 0x88, E_COMPONENT_RECORDS)
        put(E_COMPONENT + 0x8c, records)
        put(E_COMPONENT + 0x90, rng.choice([0x80000000, records, 4]))
        put(E_COMPONENT + 0x94, 0)
        put(E_COMPONENT + 0x98, 0)
        put(E_COMPONENT + 0x9c, 0)
        for k, body in enumerate(bodies):
            put(E_ROWS + 0x60 * k + 0x40, body)
        # The object: +C1 bit 0, velocities (+88/+94), and a unit model
        # scale (tag +38 -> model +24 -> +8/+C) for record bit 11.
        object_body = world.body(n)
        world.data[object_body + 0xc1 - v.DATA] = rng.getrandbits(8)
        for k in range(6):
            struct.pack_into('<f', world.data, object_body + 0x88 + 4 * k - v.DATA,
                             rng.choice([0.0, 1.0, -3.5, 1e-40, rng.uniform(-50, 50)]))
        tag = n % 4
        world.put(v.DATA3 + 0x2c80 + 0x200 * tag + 0x38, (0x5000 + 4 + tag) << 16 | (4 + tag))
        world.put(v.DATA2 + 0x2c80 + 0x80 * tag + 0x24, 16)
        world.put(v.DATA3 + 0x1000 + 16 * 16 + 8, E_SCALE)
        struct.pack_into('<ff', world.factory, E_SCALE + 8 - F, 1.0, 1.0)
        world.extra[0x46a0c0] = bytearray(image.read(0x46a0c0, 0x20))
        return n

    kernel('havok_remove_entity', 'void', 0x3198a0, regs={'ecx': 0}, stack=[1, 2], closure_stops=ENTITY_STOPS)

    def remove_entity_case(name, world, rng):
        spread = rng.random() < .15
        entities, free, phantoms, nodes, islands, fixed_island = entity_world(world, rng, spread=spread)
        body = entities[0] if spread else rng.choice(entities)
        entity_listener_member(world, rng, body)
        if rng.random() < .1:
            world.put(WORLD + 0xd0, 0)  # no filter: removals only delete
        if rng.random() < .1:
            world.track[WORLD + 0x12c - T] = 1  # locked: deferred removal
        return [WORLD, E_RESULT, body, v.SP]

    kernel('havok_add_entity', 'long', 0x319370, regs={'ecx': 0}, stack=[1], closure_stops=ENTITY_STOPS)

    def add_entity_case(name, world, rng):
        spread = rng.random() < .15
        entities, free, phantoms, nodes, islands, fixed_island = entity_world(world, rng, adding=True, spread=spread)
        body = rng.choice(free)
        fresh_entity(world, rng, body)
        place_near(world, rng, body)
        if spread:
            world.track[body + 0x40 - T] = 1
        world.track[WORLD + 0x12c - T] = rng.choice([0, 0, 0, 1, 2])  # 319370 restores it
        return [WORLD, body, v.SP]

    kernel('havok_component_remove', 'void', 0x1d9510, regs={'ecx': 0}, closure_stops=ENTITY_STOPS)

    def component_remove_case(name, world, rng):
        entities, free, phantoms, nodes, islands, fixed_island = entity_world(world, rng)
        rows = rng.sample(entities, rng.randrange(1, min(3, len(entities)) + 1))
        for body in rows:
            entity_listener_member(world, rng, body)
        component(world, rng, rows, False)
        if phantoms and rng.random() < .2:
            inside = [p for p in phantoms if p in [struct.unpack_from('<I', world.track, WORLD_PHANTOMS + 4 * k - T)[0]
                                                   for k in range(struct.unpack_from('<I', world.track, WORLD + 0x124 - T)[0])]]
            if inside:
                world.put(E_COMPONENT + rng.choice([0x98, 0x9c]), rng.choice(inside))
        return [E_COMPONENT, v.SP]

    kernel('havok_component_add', 'void', 0x1d97f0, regs={'ecx': 0}, closure_stops=ENTITY_STOPS)

    def component_add_case(name, world, rng):
        # Bodies added earlier sit in new islands (+30 set): a later row's
        # admitted pair with one would merge (317200), so only one row admits.
        count = rng.randrange(1, 4)
        entities, free, phantoms, nodes, islands, fixed_island = entity_world(world, rng, adding=True, admit=count == 1)
        rows = rng.sample(free, count)
        for k, body in enumerate(rows):
            fresh_entity(world, rng, body)
            place_near(world, rng, body)
            old_record(world, rng, body, k)
        component(world, rng, rows, True)
        world.globals[0x48b868] = rng.choice([0, 1, 0x80, 0x34f, rng.randrange(0x34f)])
        world.track[WORLD + 0x12c - T] = rng.choice([0, 0, 0, 1, 2])
        if rng.random() < .2:
            fresh = PHANTOMS + 0xa0 * 3
            phantom(world, rng, fresh, 3)
            world.put(fresh + 8, 0)
            world.put(fresh + 0xc, 0)
            world.put(fresh + 0x1c, rng.getrandbits(32))
            struct.pack_into('<H', world.track, fresh + 6 - T, rng.choice([0, 1]))
            aabb(world, rng, fresh + 0x60, nodes)
            world.put(E_COMPONENT + rng.choice([0x98, 0x9c]), fresh)
        return [E_COMPONENT, v.SP]

    # ---- 1D73D0 (EAX datum) and 1CC310 (object, flags), both ending at the
    # selected shape builder's entry (or 1CC310's no-component return).
    REFRESH_STOPS = sorted(set(ENTITY_STOPS + SHAPE_BUILDERS) - {0x1d73d0})
    v.JUMP_TABLES[0x1d74a4] = 5
    kernel('havok_component_refresh', 'void', 0x1d73d0, regs={'eax': 0}, closure_stops=REFRESH_STOPS)
    # 1CC310 stops at 1D9510 (active rows) or 1D73D0 (see the track doc).
    kernel('havok_component_state_flags', 'void', 0x1cc310, stack=[0, 1], closure_stops=ENTITY_STOPS)
    E_POOL = F + 0x2a00

    def component_datum(world, rng, n):
        """The object's +B4 component datum (rows at [0x52BE5C] +44) and a
        valid shape-builder type (definition +AA)."""
        index = rng.choice([0, 1])
        world.globals[0x52be5c] = E_POOL
        world.put(E_POOL + 0x44, E_COMPONENT - 0xa0 * index)
        datum = (rng.getrandbits(16) << 16) | index
        world.put(world.body(n) + 0xb4, datum)
        world.data[world.body(n) + 0xaa - v.DATA] = rng.choice([0, 1, 7, 11, 12])
        world.data[v.RECORDS + 12 * n + 2 - v.DATA] = rng.getrandbits(8)
        return datum

    def refresh_case(name, world, rng):
        entity_world(world, rng)
        n = component(world, rng, [], False)
        return [component_datum(world, rng, n)]

    def state_flags_case(name, world, rng):
        if rng.random() < .3:
            for n in range(v.ROWS - 1):
                world.put(world.body(n) + 0xb4, NONE)
            return [rng.choice(world.handles[:v.ROWS - 1] + [world.handles[0] | 0xffff0000]), rng.getrandbits(32), v.SP]
        entities, free, phantoms, nodes, islands, fixed_island = entity_world(world, rng)
        active = rng.random() < .6
        rows = rng.sample(entities if active else entities + free, rng.randrange(1, min(3, len(entities)) + 1))
        for body in rows:
            entity_listener_member(world, rng, body)
        n = component(world, rng, rows, False)
        flags = struct.unpack_from('<I', world.factory, E_COMPONENT + 4 - F)[0]
        world.put(E_COMPONENT + 4, flags | 0x20 if active else flags & ~0x20)
        world.put(E_COMPONENT + 0x7c, E_COMPONENT_RECORDS)
        for k in range(len(rows)):
            world.put(E_ROWS + 0x60 * k + 0x48, T + 0x7b00 + 0x40 * k)
            world.put(E_ROWS + 0x60 * k + 0x50, rng.choice([0x80000000, 0x80000000, 0x40]))
        component_datum(world, rng, n)
        return [world.handles[n], rng.getrandbits(32), v.SP]

    original_cases = v.cases

    def stopped_cases(seed, evaluators, tails):
        rows = original_cases(seed, evaluators, tails)
        for row in rows:
            stops = {'havok_component_refresh': SHAPE_BUILDERS,
                     'havok_component_state_flags': [0x1d9510, 0x1d73d0]}.get(row['kernel'])
            if stops:
                row['stop_before_entries'] = sorted(set(stops) | {v.STOP})
                row['observe_stop_entry'] = True
        return rows

    v.cases = stopped_cases
    v.TRACK_ARGUMENTS.update(havok_component_refresh=refresh_case, havok_component_state_flags=state_flags_case)
    v.TRACK_COUNTS.update(havok_component_refresh=200, havok_component_state_flags=400)

    v.TRACK_ARGUMENTS.update(havok_remove_entity=remove_entity_case, havok_add_entity=add_entity_case,
                             havok_component_remove=component_remove_case, havok_component_add=component_add_case)
    v.TRACK_COUNTS.update(havok_remove_entity=300, havok_add_entity=300, havok_component_remove=300,
                          havok_component_add=300)
