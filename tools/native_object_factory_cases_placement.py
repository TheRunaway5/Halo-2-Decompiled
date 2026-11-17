"""Oracle cases for factory track "placement" (world placement owners).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Opaque callees: the placement roots statically reach the Havok world and the
other tracks' owners (the whole object_new closure). For each kernel,
OPAQUE[kernel] names direct callees whose bodies stay outside that kernel's
authenticated closure. The native refuses each of them by name, the fixtures
never reach them, and the observer rejects any case that enters one (its
first instruction is not an authenticated site).
"""
import math
import struct

TRACK_BASE = 0x1500000
# Track bank layout.
CBSP, PLANES, NODES, LEAVES = 0x0000, 0x0040, 0x0200, 0x0400
REFS, BSP2D, SURFACES, EDGES, VERTICES = 0x0500, 0x0700, 0x0b00, 0x0d00, 0x1600
SBSP, SBSP_LEAVES, CLUSTERS, PORTALS, PORTAL_DATA, MACHINE = 0x1a00, 0x1c40, 0x1e40, 0x2700, 0x2c00, 0x3e00
PARTITION_DATA, HEADS, LIGHT_PARTITION, LIGHT_POOL, LIGHT_ROWS = 0x4000, 0x5800, 0x6800, 0x9000, 0x9100
COMPONENTS, COMPONENT_ROWS, COUNTERS = 0xa000, 0xa080, 0xb000
HAVOK_MEMORY = 0x420000  # 149AD0 statistics: +6C..+A8 and +F0340..+F03C8 (image .bss blocks)
OUT, PAYLOAD = 0xf000, 0xf100
NONE = 0xffffffff

# Havok world entry points and other tracks' roots (see the track doc).
HAVOK = {0x1d97f0, 0x1d9510, 0x1d8470, 0x1d73d0, 0x1dc270, 0x1d7ca0, 0x1cd130, 0x1ccbe0, 0x1cb330,
         0x1cb220, 0x149680, 0x1498a0}
OTHER_TRACKS = {0xbf050, 0xbf0c0, 0xbff20, 0xb9b70, 0x179610, 0xbaf10, 0xbbac0, 0xcea70,
                0x1d3380, 0xc0270, 0xbc5c0, 0xc0910, 0xfa820, 0x102ad0}
# C4D30 is native for sphere and frustum lights; its census-unexecuted
# branches are refused. BACD0's FC510 item path is native (2026-11-15).
# C2870 (the frame ray march) and C39B0 (scenario light shapes) are native
# since 2026-11-15; 16D740 (the collision sphere fit) since 2026-11-16, for
# the structure BSP and (continuation) its instanced geometry (16CF00): its
# object query (16D0E0) is refused, and no 16D740 caller sets flags bit 3.
LIGHT_STOPS = {0x16d0e0}
UNPORTED = {0xba570, 0xe3e20} | LIGHT_STOPS
OPAQUE = {}
# Indirect-call targets a kernel's cases reach (10DDE0's machine callback);
# their bodies join that kernel's authenticated sites.
INDIRECT = {}


def _opaque_metadata(v):
    """Wrap v.metadata: kernels listed in OPAQUE get closures that stop at
    their opaque callees. Other kernels keep the original closure."""
    original = v.metadata

    class Empty:
        code = ()

    def metadata(image, evaluators):
        plain = {n: e for n, e in evaluators.items() if n not in OPAQUE}
        meta, code = original(image, plain)
        sites = {s['instruction']: s for s in meta['sites']}
        for name in sorted(set(evaluators) - set(plain)):
            stop, complete = OPAQUE[name], v.complete_body
            v.complete_body = lambda c, entry: Empty if entry in stop else complete(c, entry)
            try:
                part, _ = original(image, {name: evaluators[name]})
            finally:
                v.complete_body = complete
            for site in part['sites']:
                sites.setdefault(site['instruction'], site)
            meta['owners'].update(part['owners'])
        for name in sorted(set(evaluators) & set(INDIRECT)):
            for target in INDIRECT[name]:
                part, _ = original(image, {f'{name}@{target:x}': target})
                for site in part['sites']:
                    sites.setdefault(site['instruction'], site)
        meta['sites'] = list(sites.values())
        return meta, code
    v.metadata = metadata


def check_literals(v):
    """The native .rdata literals must equal the authenticated XBE."""
    import re
    from pathlib import Path
    source = (Path(__file__).resolve().parents[1] / 'src/gameplay/objects/native_object_factory_placement_tu15.cpp').read_text()
    block = source[source.index('// BEGIN TU1.5 .rdata literals'):source.index('// END TU1.5 .rdata literals')]
    image = v.Xbe(v.NEW_DEFAULT)
    rows = [int(x) for x in re.findall(r'(\d+)', block[block.index('projection_rows{'):block.index(';')])]
    if list(struct.unpack('<18h', image.read(0x44ac8c, 36))) != rows:
        raise ValueError('Native 0x44AC8C projection rows differ from the authenticated XBE')
    for value, address in re.findall(r'=0x([0-9a-f]{8})u;\s*// 0x([0-9A-F]+)', block):
        if struct.unpack('<I', image.read(int(address, 16), 4))[0] != int(value, 16):
            raise ValueError(f'Native .rdata literal 0x{address} differs from the authenticated XBE')


def register(v):
    # 11E060's bounded switch (CMP EDX,3 / JA): four first-person model cases.
    v.JUMP_TABLES[0x11e09c] = 4
    check_literals(v)
    _opaque_metadata(v)
    v.UNREACHED_VIRTUAL_OWNERS.add(0x10dde0)  # type callbacks: only 118120 (machines), authenticated via INDIRECT
    DATA = v.DATA

    def put16(world, address, value):
        bank, base = bank_of(world, address)
        struct.pack_into('<H', bank, address - base, value & 0xffff)

    def putf(world, address, value):
        world.put(address, struct.unpack('<I', struct.pack('<f', value))[0])

    def bank_of(world, address):
        for base, bank in ((v.DATA, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (v.AI, world.ai),
                           (TRACK_BASE, world.track), *world.extra.items(), *world.absolute.items()):
            if bank is not None and base <= address < base + len(bank):
                return bank, base
        raise ValueError(f'Undeclared world address {address:x}')

    def fbits(value):
        return struct.unpack('<I', struct.pack('<f', value))[0]

    def track(world, rng):
        if world.track is None:
            world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))

    def scalar(rng, scale=8.0):
        roll = rng.random()
        if roll < .02:
            return rng.choice([float('nan'), 1e-40, -1e-41, float('inf'), -0.0, 0.0])
        return rng.uniform(-scale, scale)

    def collision_bsp(world, rng):
        """A small collision BSP at TRACK+CBSP: 3-D nodes over random planes,
        leaves with BSP2D references, BSP2D nodes, triangle surfaces with
        consistent edge rings and vertices."""
        T = TRACK_BASE
        planes = rng.randrange(3, 9)
        for p in range(planes):
            for k in range(3):
                putf(world, T + PLANES + 16 * p + 4 * k, scalar(rng, 1.0))
            putf(world, T + PLANES + 16 * p + 12, scalar(rng, 3.0))
        leaves, nodes = [], []

        def leaf():
            if rng.random() < .08:
                return 0xffffff
            leaves.append(len(leaves))
            return 0x800000 | (len(leaves) - 1)

        def build(depth):
            if depth > rng.choice([2, 3, 4, 5]) or len(nodes) >= 40:
                return leaf()
            index = len(nodes)
            nodes.append(None)
            nodes[index] = (rng.randrange(planes), build(depth + 1), build(depth + 1))
            return index
        if build(0) & 0x800000:
            nodes.append((rng.randrange(planes), leaf(), leaf()))  # node 0 always exists
            nodes[0], nodes[-1] = nodes[-1], nodes[0]
        for n, (plane, a, b) in enumerate(nodes):
            at = T + NODES + 8 * n
            put16(world, at, plane)
            bank, base = bank_of(world, at)
            bank[at + 2 - base:at + 5 - base] = a.to_bytes(3, 'little')
            bank[at + 5 - base:at + 8 - base] = b.to_bytes(3, 'little')
        surfaces = rng.randrange(1, 12)
        verts = 3 * surfaces
        for k in range(verts):
            for c in range(3):
                putf(world, T + VERTICES + 16 * k + 4 * c, rng.uniform(-6, 6))
            put16(world, T + VERTICES + 16 * k + 0xc, k)   # its first edge (24E710 reads its surface)
        for s in range(surfaces):
            put16(world, T + SURFACES + 8 * s, rng.randrange(planes))
            put16(world, T + SURFACES + 8 * s + 2, 3 * s)
            right_side = rng.random() < .3
            for k in range(3):
                e = T + EDGES + 12 * (3 * s + k)
                nxt = 3 * s + (k + 1) % 3
                other = rng.choice([0xffff, (s + 1) % surfaces if surfaces > 1 else 0xffff])
                put16(world, e, 3 * s + k)
                put16(world, e + 2, 3 * s + (k + 1) % 3)
                put16(world, e + 4, rng.randrange(3 * surfaces) if right_side else nxt)
                put16(world, e + 6, nxt if right_side else rng.randrange(3 * surfaces))
                put16(world, e + 8, other if right_side else s)
                put16(world, e + 10, s if right_side else other)
        bsp2d = []

        def build2d(depth):
            if depth > rng.choice([0, 1, 2]) or len(bsp2d) >= 50:
                return 0x8000 | rng.randrange(surfaces)
            index = len(bsp2d)
            bsp2d.append(None)
            bsp2d[index] = (build2d(depth + 1), build2d(depth + 1))
            return index
        refs = []
        for n in range(len(leaves)):
            count = rng.choice([0, 0, 1, 2, 3])
            world.track[LEAVES + 4 * n] = rng.getrandbits(8)
            world.track[LEAVES + 4 * n + 1] = count
            put16(world, T + LEAVES + 4 * n + 2, len(refs))
            for _ in range(count):
                plane = rng.randrange(planes) | (0x8000 if rng.random() < .5 else 0)
                refs.append((plane, build2d(0)))
        for n, (plane, node) in enumerate(refs):
            put16(world, T + REFS + 4 * n, plane)
            put16(world, T + REFS + 4 * n + 2, node)
        for n, (a, b) in enumerate(bsp2d):
            at = T + BSP2D + 16 * n
            for k in range(3):
                putf(world, at + 4 * k, scalar(rng, 2.0))
            put16(world, at + 12, a)
            put16(world, at + 14, b)
        world.cbsp_planes = planes
        # An edge side without a surface (0xFFFF) names the row before the
        # surfaces; 24E850 (16D740's edge features) reads its plane word.
        put16(world, T + SURFACES - 8, 0)
        for offset, address in ((4, NODES), (0xc, PLANES), (0x14, LEAVES), (0x1c, REFS), (0x24, BSP2D),
                                (0x2c, SURFACES), (0x34, EDGES), (0x3c, VERTICES)):
            world.put(T + CBSP + offset, T + address)
        return len(leaves)

    def structure_bsp(world, rng, leaves, clusters=12):
        T = TRACK_BASE
        for n in range(leaves):
            put16(world, T + SBSP_LEAVES + 8 * n, rng.choice(list(range(clusters)) + [0xffff]))
        world.put(T + SBSP + 0x18, T + CBSP)
        world.put(T + SBSP + 0x30, T + SBSP_LEAVES)
        world.put(T + SBSP + 0x9c, clusters)
        world.put(T + SBSP + 0xa0, T + CLUSTERS)
        world.put(T + SBSP + 0x60, T + PORTALS)
        world.put(T + SBSP + 0x220, T + MACHINE)
        world.put(T + MACHINE + 0xc, T + MACHINE + 0x40)
        portals = rng.randrange(1, 20)
        data = T + PORTAL_DATA
        touching = {c: [] for c in range(clusters)}
        for p in range(portals):
            at = T + PORTALS + 0x24 * p
            a = rng.randrange(clusters)
            b = rng.choice([c for c in range(clusters) if c != a])
            put16(world, at, a)
            put16(world, at + 2, b)
            touching[a].append(p)
            touching[b].append(p)
            world.put(at + 4, rng.randrange(world.cbsp_planes))
            center = [rng.uniform(-6, 6) for _ in range(3)]
            for k in range(3):
                putf(world, at + 8 + 4 * k, center[k])
            putf(world, at + 0x14, rng.choice([rng.uniform(0, 8), rng.uniform(0, 2), 0.0, float('nan')]))
            count = rng.choice([0, 3, 4, 4, 5, 6])
            world.put(at + 0x1c, count)
            world.put(at + 0x20, data)
            for n in range(count):
                angle = 2 * math.pi * n / max(count, 1) + rng.uniform(-.3, .3)
                radius = rng.uniform(1, 9)
                offsets = [radius * math.cos(angle), radius * math.sin(angle), rng.uniform(-radius, radius)]
                rng.shuffle(offsets)
                for k in range(3):
                    putf(world, data + 12 * n + 4 * k, center[k] + offsets[k])
            data += 12 * count
        for c in range(clusters):
            at = T + CLUSTERS + 0xb0 * c
            indices = touching[c][:]
            if rng.random() < .2:
                indices.append(rng.randrange(portals))
            rng.shuffle(indices)
            world.put(at + 0x8c, len(indices))
            world.put(at + 0x90, data)
            for n, index in enumerate(indices):
                put16(world, data + 2 * n, index)
            data += 2 * len(indices)
        for n in range(16):
            put16(world, T + MACHINE + 0x40 + 2 * n, rng.randrange(portals))
        world.portal_count = portals

    def map_world(world, rng):
        track(world, rng)
        leaves = collision_bsp(world, rng)
        structure_bsp(world, rng, leaves)
        world.globals[0x4ecc0c] = TRACK_BASE + CBSP
        world.globals[0x4ecc10] = TRACK_BASE + CBSP
        world.globals[0x4ecc18] = TRACK_BASE + SBSP
        index = rng.choice([0, 1, 3, 0xffff]) if rng.random() < .15 else rng.choice([0, 1, 2])
        world.absolute[0x474c24][0:2] = struct.pack('<H', index)
        # 16D2A0's breakable-surface bits: 32 bytes per structure BSP.
        world.globals[0x4f9b50] = TRACK_BASE + 0xc000
        return leaves

    def point(world, rng, at):
        for k in range(3):
            putf(world, at + 4 * k, scalar(rng, 6.0))

    def root_plane_x(world):
        """Root node plane = (1, 0, 0, 0): exact-boundary distances."""
        plane = struct.unpack_from('<h', world.track, NODES)[0]
        for k, value in enumerate((1.0, 0.0, 0.0, 0.0)):
            putf(world, TRACK_BASE + PLANES + 16 * plane + 4 * k, value)

    # 14CDB0 / 11E600 / 11E5C0 / C1540.
    v.KERNELS['bsp3d_leaf_from_point'] = (['placement_leaf_point'], 'long')
    v.ROUTINES['bsp3d_leaf_from_point'] = dict(entry=0x14cdb0, regs={'ecx': 0, 'eax': 1, 'edx': 2})
    v.KERNELS['location_from_point'] = (['placement_location_point'], 'void')
    v.ROUTINES['location_from_point'] = dict(entry=0x11e600, regs={'esi': 0, 'edx': 1})
    v.KERNELS['location_from_leaf'] = (['placement_location_leaf'], 'void')
    v.ROUTINES['location_from_leaf'] = dict(entry=0x11e5c0, regs={'eax': 0, 'ecx': 1})
    v.KERNELS['object_get_location'] = (['placement_object_location'], 'boolean')
    v.ROUTINES['object_get_location'] = dict(entry=0xc1540, regs={'edi': 1}, stack=[0])

    def leaf_point(name, world, rng):
        map_world(world, rng)
        point(world, rng, TRACK_BASE + OUT)
        if rng.random() < .25:
            root_plane_x(world)
            putf(world, TRACK_BASE + OUT, rng.choice([0.0, -0.0]))
        return [TRACK_BASE + CBSP, 0, TRACK_BASE + OUT]

    def location_point(name, world, rng):
        map_world(world, rng)
        point(world, rng, TRACK_BASE + OUT + 0x20)
        return [TRACK_BASE + OUT, TRACK_BASE + OUT + 0x20]

    def location_leaf(name, world, rng):
        leaves = map_world(world, rng)
        return [TRACK_BASE + OUT, rng.choice([NONE] + list(range(max(leaves, 1))))]

    def straddle_walk_faults(chain, last):
        """1E6150's control flow over a chain of straddled nodes (each
        (other child, next first)): True when a push reaches slot 131 (the
        original overwrites 1E60C0's return address). Slot 128 marked for
        the first child sets the stop byte; the plain plane clears it; a set
        stop byte ends the walk once a leaf is recorded."""
        state = dict(count=0, stop=False)

        def visit(child, depth):
            if child < 0x800000:
                return node(child, depth)
            if child != 0xffffff:
                state['count'] += 1
            return False

        def node(k, depth):
            if depth >= 131:
                return True
            other, next_first = chain[k]
            following = k + 1 if k + 1 < len(chain) else last
            first, second = (following, other) if next_first else (other, following)
            if depth == 128:
                state['stop'] = True
            if visit(first, depth + 1):
                return True
            if state['stop'] and state['count']:
                return False
            if depth == 128:
                state['stop'] = False
            if visit(second, depth + 1):
                return True
            return False
        return node(0, 0)

    def deep_sphere_chain(world, rng, center, leaves):
        """C1540's sphere walk over a chain of nodes the sphere straddles
        (planes 9..12 through its centre): the walk stacks past 128 planes
        (see straddle_walk_faults); chains whose walk would fault are cut."""
        T = TRACK_BASE
        nodes = T + 0xd000
        for p in range(9, 13):
            n = unit(rng)
            for k, value in enumerate(n + [sum(x * c for x, c in zip(n, center))]):
                putf(world, T + PLANES + 16 * p + 4 * k, value)
        leaf = lambda: 0xffffff if not leaves or rng.random() < .5 else 0x800000 | rng.randrange(leaves)
        chain = [(leaf(), rng.random() < .5) for _ in range(rng.choice([126, 128, 129, 130, 131, 133, 136, 140]))]
        last = leaf()
        while straddle_walk_faults(chain, last):
            chain.pop()
        for k, (other, next_first) in enumerate(chain):
            following = k + 1 if k + 1 < len(chain) else last
            write_node(world, nodes + 8 * k, rng.randrange(9, 13), [following, other] if next_first else [other, following])
        world.put(T + CBSP + 4, nodes)

    def object_location(name, world, rng):
        leaves = map_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        point(world, rng, body + 0x40)
        point(world, rng, body + 0x64)
        if rng.random() < .3:
            # Centre and origin far outside every leaf's cluster range.
            for at in (body + 0x40, body + 0x64):
                putf(world, at, rng.choice([40.0, -40.0]))
        putf(world, body + 0x4c, rng.choice([rng.uniform(0, 4), rng.uniform(0, 12), 0.0, -1.0, 1e-40,
                                            float('nan'), rng.uniform(0, 0.5)]))
        if rng.random() < .2:
            # Sphere touching the root plane exactly (d == -r or d == r);
            # no structure BSP, so C1540 reaches the sphere leaf walk.
            root_plane_x(world)
            radius = rng.choice([2.0, 0.5])
            putf(world, body + 0x4c, radius)
            putf(world, body + 0x40, rng.choice([-radius, radius]))
            world.absolute[0x474c24][0:2] = struct.pack('<H', 0xffff)
        elif rng.random() < .2:
            # No structure BSP: the sphere walk over a deep straddled chain.
            putf(world, body + 0x4c, rng.uniform(0.5, 4))
            center = list(struct.unpack_from('<3f', world.data, body + 0x40 - DATA))
            deep_sphere_chain(world, rng, center, leaves)
            world.absolute[0x474c24][0:2] = struct.pack('<H', 0xffff)
        return [world.handles[n], TRACK_BASE + OUT]

    v.TRACK_ARGUMENTS['placement_leaf_point'] = leaf_point
    v.TRACK_ARGUMENTS['placement_location_point'] = location_point
    v.TRACK_ARGUMENTS['placement_location_leaf'] = location_leaf
    v.TRACK_ARGUMENTS['placement_object_location'] = object_location
    v.TRACK_COUNTS.update(bsp3d_leaf_from_point=300, location_from_point=300, location_from_leaf=100,
                          object_get_location=600)

    # Cluster partitions (0x4EABA4 / 0x4EABB0 in the 0x4EABA0 absolute block),
    # the light pool (0x4ECBDC) and the light partition (0x4ECBE0).
    def data_array(world, header, rows, bitmap, size, maximum, used):
        world.put(header + 0x20, maximum)
        world.put(header + 0x24, size)
        world.put(header + 0x44, rows)
        world.put(header + 0x48, bitmap)
        high = max(used) + 1 if used else 0
        world.put(header + 0x34, min(used) if used and world.rng.random() < .3 else high)
        world.put(header + 0x38, high)
        world.put(header + 0x3c, len(used))
        put16(world, header + 0x40, world.rng.choice([0x8000, 0x9abc, 0xfffe]))
        for word in range((maximum + 31) // 32):
            bits = 0
            for n in used:
                if n // 32 == word:
                    bits |= 1 << (n % 32)
            world.put(bitmap + 4 * word, bits)
        for n in used:
            put16(world, rows + size * n, 0x8000 | world.rng.getrandbits(14))

    def handle(world, rows, size, n):
        bank, base = bank_of(world, rows + size * n)
        return (struct.unpack_from('<H', bank, rows + size * n - base)[0] << 16) | n

    class Partition:
        """A partition at `root` (heads, entries, refs) laid out in TRACK."""

        def __init__(self, world, rng, root, at, heads, clusters=12, entries=48, refs=48):
            T = TRACK_BASE + at
            self.world, self.root = world, root
            # 512 cluster heads (0x800 bytes): forced vectors reach every cluster.
            self.heads, self.entries, self.refs = TRACK_BASE + heads, T + 0x100, T + 0x180
            self.entry_rows, self.ref_rows = T + 0x200, T + 0x800
            world.put(root, self.heads)
            world.put(root + 4, self.entries)
            world.put(root + 8, self.refs)
            for c in range(clusters):
                world.put(self.heads + 4 * c, NONE)
            self.clusters = clusters
            self.entry_used, self.ref_used = [], []
            self.entry_max, self.ref_max = entries, refs
            self.chains = {}

        def finish(self, rng):
            data_array(self.world, self.entries, self.entry_rows, self.entries + 0x60, 32, self.entry_max,
                       self.entry_used)
            data_array(self.world, self.refs, self.ref_rows, self.refs + 0x60, 12, self.ref_max, self.ref_used)

        def free_index(self, used, maximum, rng):
            choices = [n for n in range(maximum) if n not in used]
            n = rng.choice(choices)
            used.append(n)
            return n


    v.KERNELS['object_disconnect_from_map'] = (['placement_disconnect'], 'void')
    v.ROUTINES['object_disconnect_from_map'] = dict(entry=0xba7e0, stack=[0])
    OPAQUE['object_disconnect_from_map'] = UNPORTED

    def object_tree_flags(world, rng, chosen):
        # No owned lights (C3750) and no attachment block unless a case
        # declares one, anywhere C0F60 may walk.
        for n in range(v.ROWS - 1):
            body = world.body(n)
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~0x50
            if rng.random() < .2:
                flags |= 1
            world.put(body + 4, flags)
            for at in (0xc, 0x10):
                world.put(body + at, NONE)

    def lights(world, rng, partition, count=6):
        pool = TRACK_BASE + LIGHT_POOL
        world.globals[0x4ecbdc] = pool
        used = rng.sample(range(8), count)
        data_array(world, pool, TRACK_BASE + LIGHT_ROWS, pool + 0x60, 0x110, 8, used)
        handles = []
        for n in used:
            row = TRACK_BASE + LIGHT_ROWS + 0x110 * n
            bank, base = bank_of(world, row)
            bank[row + 2 - base] = rng.choice([0, 2, 0x0a, 0xff, 8])
            world.put(row + 0x10, NONE)
            handles.append(handle(world, TRACK_BASE + LIGHT_ROWS, 0x110, n))
        return handles

    def build_links(world, rng, part, owners):
        """owners: [(owner handle, head address)]. Every owner gets references
        into random clusters; each cluster list holds entries of several owners
        in random order (prepended)."""
        cluster_lists = {c: [] for c in range(part.clusters)}
        owner_refs = []
        for owner, head in owners:
            clusters = rng.sample(range(part.clusters), rng.choice([0, 1, 1, 2, 3]))
            refs = []
            for c in clusters:
                r = part.free_index(part.ref_used, part.ref_max, rng)
                e = part.free_index(part.entry_used, part.entry_max, rng)
                refs.append((r, c))
                if rng.random() < .9:  # else a stale reference (1D30F0 walks the whole list)
                    cluster_lists[c].append((e, owner))
            owner_refs.append((head, refs))
        part.finish(rng)
        for head, refs in owner_refs:
            first = NONE
            for r, c in reversed(refs):
                row = part.ref_rows + 12 * r
                world.put(row + 4, c if rng.random() < .9 else (c | 0xffff0000 if c else c))
                world.put(row + 8, first)
                first = handle(world, part.ref_rows, 12, r)
            world.put(head, first)
        for c, entries in cluster_lists.items():
            rng.shuffle(entries)
            first = NONE
            for e, owner in entries:
                row = part.entry_rows + 32 * e
                world.put(row + 4, owner)
                world.put(row + 8, first)
                first = handle(world, part.entry_rows, 32, e)
            world.put(part.heads + 4 * c, first)

    def owned_lights(world, rng, members, light_handles, marker=False):
        """C3750: +4 bit 6 owners and the lights naming them at +58 (with a
        marker owner for C4D30 when `marker`: a +58 light without one takes
        the refused 145070 path)."""
        for k in members:
            if rng.random() < .5:
                continue
            body = world.body(k)
            world.put(body + 4, struct.unpack_from('<I', world.data, body + 4 - DATA)[0] | 0x40)
            for h in rng.sample(light_handles, min(len(light_handles), rng.randrange(0, 4))):
                row = TRACK_BASE + LIGHT_ROWS + 0x110 * (h & 0xffff)
                world.put(row + 0x58, world.handles[k])
                if marker:
                    bank, base = bank_of(world, row)
                    if struct.unpack_from('<I', bank, row + 0x4c - base)[0] == NONE:
                        world.put(row + 0x4c, rng.choice(world.handles[:v.ROWS - 1]))
                    put16(world, row + 0x54, rng.choice([0, 1, 2]))

    def disconnect(name, world, rng):
        track(world, rng)
        n = rng.randrange(v.ROWS - 1)
        object_tree_flags(world, rng, n)
        world.absolute[0x4eaba0][:] = bytes(rng.getrandbits(8) for _ in range(0x80))
        parts = [Partition(world, rng, 0x4eaba4, PARTITION_DATA, HEADS), Partition(world, rng, 0x4eabb0, PARTITION_DATA + 0xc00, HEADS + 0x800)]
        owners = [[], []]
        for k in range(v.ROWS - 1):
            body = world.body(k)
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0]
            owners[(flags >> 9) & 1].append((world.handles[k], body + 0x60))
        for part, group in zip(parts, owners):
            build_links(world, rng, part, group)
        # Lights attached through +11C/+11E descriptors; children and siblings.
        light_root = TRACK_BASE + LIGHT_PARTITION
        world.globals[0x4ecbe0] = light_root
        world.globals[0x4ecbe4] = light_root + 0x100
        world.globals[0x4ecbe8] = light_root + 0x180
        lp = Partition.__new__(Partition)
        lp.world, lp.root = world, 0x4ecbe0
        lp.heads, lp.entries, lp.refs = light_root, light_root + 0x100, light_root + 0x180
        lp.entry_rows, lp.ref_rows = light_root + 0x200, light_root + 0x600
        lp.clusters, lp.entry_used, lp.ref_used, lp.entry_max, lp.ref_max = 12, [], [], 24, 24
        for c in range(12):
            world.put(light_root + 4 * c, NONE)
        light_handles = lights(world, rng, lp)
        light_owners = [(h, TRACK_BASE + LIGHT_ROWS + 0x110 * (h & 0xffff) + 0x10) for h in light_handles]
        build_links(world, rng, lp, light_owners)
        tree = [n] + [k for k in rng.sample(range(v.ROWS - 1), 4) if k != n]
        if rng.random() < .6 and len(tree) > 1:
            world.put(world.body(n) + 0x10, world.handles[tree[1]])
            for a, b in zip(tree[1:], tree[2:]):
                world.put(world.body(a) + (0xc if rng.random() < .6 else 0x10), world.handles[b])
        owned_lights(world, rng, tree, light_handles)
        for k in tree:
            body = world.body(k)
            if rng.random() < .6:
                flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0] | 0x10
                world.put(body + 4, flags)
                count = rng.randrange(0, 4)
                offset = 0x300
                struct.pack_into('<hh', world.data, body + 0x11c - DATA, 8 * count, offset)
                for d in range(count):
                    world.data[body + offset + 8 * d - DATA] = rng.choice([0, 0, 1])
                    world.put(body + offset + 8 * d + 4, rng.choice(light_handles + [NONE]))
        return [world.handles[n]]

    v.TRACK_ARGUMENTS['placement_disconnect'] = disconnect
    v.TRACK_COUNTS['object_disconnect_from_map'] = 400

    v.KERNELS['object_defer_physics'] = (['placement_defer'], 'void')
    v.ROUTINES['object_defer_physics'] = dict(entry=0xba8c0, stack=[0])
    OPAQUE['object_defer_physics'] = HAVOK

    def defer(name, world, rng):
        n = rng.randrange(v.ROWS - 1)
        world.put(world.body(n) + 0xb4, NONE)
        return [world.handles[n]]

    v.TRACK_ARGUMENTS['placement_defer'] = defer
    v.TRACK_COUNTS['object_defer_physics'] = 60

    # ---- Shared placement world: map, clusters, partitions, activation globals ----
    PVS = v.PVS

    def placement_world(world, rng, objects=True):
        map_world(world, rng)
        T = TRACK_BASE
        stamp = rng.getrandbits(32)
        world.absolute[0x4f3ce0][1] = rng.getrandbits(8)
        struct.pack_into('<I', world.absolute[0x4f3ce0], 4, stamp)
        world.extra[0x4f3d10] = bytearray(rng.getrandbits(8) for _ in range(0x40))
        for c in range(26):
            world.put(0x4f3ce8 + 4 * c, rng.choice([stamp + 1, stamp + 1, stamp, rng.getrandbits(32)]) & NONE)
        world.absolute[0x4eaba0][:] = bytes(rng.getrandbits(8) for _ in range(0x80))
        world.parts = [Partition(world, rng, 0x4eaba4, PARTITION_DATA, HEADS), Partition(world, rng, 0x4eabb0, PARTITION_DATA + 0xc00, HEADS + 0x800)]
        world.put(0x4eabc4, T + COUNTERS)
        world.globals[0x52be44] = T + COUNTERS + 0x10
        world.globals[0x52be5c] = T + COMPONENTS
        world.put(T + COMPONENTS + 0x44, T + COMPONENT_ROWS)
        for n in range(8):
            world.put(T + COMPONENT_ROWS + 160 * n + 4, rng.getrandbits(32))
        world.globals[0x48b870] = (rng.choice([0, 0, 0, 1]) << 24) | rng.choice([0, 0, 0x100])
        world.globals[0x56dfcc] = 0
        world.globals[0x486074] = 0
        world.globals[0x486070] = 0
        world.globals[0x48606c] = 0
        world.globals[0x486058] = rng.getrandbits(32)
        world.data2[PVS + 0xc - v.DATA2] = rng.choice([0, 1, 4, 4])
        types = []
        for k in range(v.ROWS - 1):
            body = world.body(k)
            row = v.RECORDS + 12 * k
            kind = world.data[row + 3 - DATA] & 0x1f
            types.append(kind)
            world.data[body + 0xaa - DATA] = kind
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~0x50
            world.put(body + 4, flags)
            for at in (0xc, 0x10):
                world.put(body + at, NONE)
            mask_type = (1 << kind) & 0x1883
            world.put(body + 0xb4, NONE if mask_type or rng.random() < .5 else rng.randrange(8))
            world.put(body + 0xd4, rng.choice([NONE, 5]))
            world.data[body + 0x3dc - DATA] = 0
            put16(world, body + 0x2c, rng.choice([0xffff, rng.randrange(12)]))  # previous location cluster
            put16(world, body + 0x1dc, rng.choice([0, 0, 1, 3, 0xffff]))
            put16(world, body + 0x1da, rng.randrange(12))
        world.types = types
        owners = [[], []]
        for k in range(v.ROWS - 1):
            body = world.body(k)
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0]
            owners[(flags >> 9) & 1].append((world.handles[k], body + 0x60))
        for part, group in zip(world.parts, owners):
            build_links(world, rng, part, group)

    def tree(world, rng, n, chain=4):
        """Children of n over +10 (first child) and +C (siblings)."""
        members = [k for k in rng.sample(range(v.ROWS - 1), chain) if k != n]
        if members and rng.random() < .6:
            world.put(world.body(n) + 0x10, world.handles[members[0]])
            for a, b in zip(members, members[1:]):
                world.put(world.body(a) + (0xc if rng.random() < .6 else 0x10), world.handles[b])
        return members

    # 123210, 243450, 14CEA0, 14D0E0, 1D2F10.
    v.KERNELS['dominant_axis'] = (['placement_axis'], 'short')
    v.ROUTINES['dominant_axis'] = dict(entry=0x123210, regs={'eax': 0})
    v.KERNELS['circle_inside_portal'] = (['placement_circle'], 'boolean')
    v.ROUTINES['circle_inside_portal'] = dict(entry=0x243450, regs={'ecx': 0, 'edi': 1}, xmm={0: 2}, stack=[3])
    v.KERNELS['portal_intersects_sphere'] = (['placement_portal'], 'boolean')
    v.ROUTINES['portal_intersects_sphere'] = dict(entry=0x14cea0, regs={'eax': 0, 'ecx': 1, 'edx': 2}, stack=[3])
    v.KERNELS['cluster_portal_flood'] = (['placement_flood'], 'short')
    v.ROUTINES['cluster_portal_flood'] = dict(entry=0x14d0e0, stack=[0, 1, 2, 3, 4])
    v.KERNELS['cluster_partition_reconnect'] = (['placement_reconnect_clusters'], 'void')
    # Eight argument words: the payload is the owner's centre block (+40..+53)
    # and the overflow byte is the location's first byte (read before it).
    # The "bare" form passes a NULL payload and no forced vector.
    v.ROUTINES['cluster_partition_reconnect'] = dict(entry=0x1d2f10, regs={'eax': 0}, stack=[1, 2, 3, 4, 5, 6, 7, 4, 6])
    v.KERNELS['cluster_partition_reconnect_bare'] = (['placement_reconnect_clusters'], 'void')
    v.ROUTINES['cluster_partition_reconnect_bare'] = dict(entry=0x1d2f10, regs={'eax': 0}, stack=[1, 2, 3, 4, 5, 6, 7, 0, 6])

    def special(rng):
        return rng.choice([0.0, -0.0, 1e-40, -1e-40, float('nan'), float('inf'), 1.0, -1.0, 2.5, 3.4e38])

    def axis(name, world, rng):
        track(world, rng)
        for k in range(3):
            putf(world, TRACK_BASE + OUT + 4 * k, special(rng) if rng.random() < .3 else rng.uniform(-2, 2))
        if rng.random() < .2:
            world.put(TRACK_BASE + OUT + 4 * rng.randrange(3), rng.choice([0x7f800001, 0xff800001, 0x00000001, 0x80000001]))
        if rng.random() < .2:
            world.put(TRACK_BASE + OUT + 4, struct.unpack_from('<I', world.track, OUT + 8)[0] ^ rng.choice([0, 0x80000000]))
        return [TRACK_BASE + OUT]

    def circle(name, world, rng):
        track(world, rng)
        count = rng.choice([0, 1, 3, 4, 5, 8, 0xffff])
        center = [rng.uniform(-3, 3), rng.uniform(-3, 3)]
        for n in range(min(count, 8)):
            angle = 2 * math.pi * n / 8 + rng.uniform(-.2, .2)
            r = rng.uniform(1, 6)
            point = [center[0] + r * math.cos(angle), center[1] + r * math.sin(angle)]
            if rng.random() < .1:
                point = [rng.choice([special(rng), point[0]]), point[1]]
            if n and rng.random() < .1:
                point = list(struct.unpack('<2f', world.track[OUT + 8 * (n - 1):OUT + 8 * n]))
            elif n and rng.random() < .15:
                previous = struct.unpack('<2f', world.track[OUT + 8 * (n - 1):OUT + 8 * n])
                point = [previous[0] + rng.choice([1e-20, -3e-20]), previous[1] + rng.choice([2e-20, 0.0])]
            putf(world, TRACK_BASE + OUT + 8 * n, point[0])
            putf(world, TRACK_BASE + OUT + 8 * n + 4, point[1])
        if 2 <= count <= 8 and rng.random() < .3:
            # A tiny edge at the origin: its squared length is denormal, or
            # zero once a DAZ ADDSS consumes the denormal square.
            n = rng.randrange(count - 1)
            for k, value in enumerate((1e-20, rng.choice([0.0, 1e-20]), rng.choice([2e-20, -1e-20]), 3e-20)):
                putf(world, TRACK_BASE + OUT + 8 * n + 4 * k, value)
            center = [rng.choice([-6.0, 6.0]), rng.choice([-6.0, 6.0])]
        putf(world, TRACK_BASE + OUT + 0x80, center[0] + rng.uniform(-2, 2))
        putf(world, TRACK_BASE + OUT + 0x84, center[1] + rng.uniform(-2, 2))
        radius = rng.choice([rng.uniform(0, 3), rng.uniform(0, 0.5), special(rng)])
        if rng.random() < .2:
            # The point is inside a square whose last two corners are joined
            # through a tiny reversed edge at the origin. Under DAZ its length
            # is zero and the edge is skipped; otherwise it rejects the point.
            square = [(-10.0, -10.0), (10.0, -10.0), (10.0, 10.0), (-10.0, 10.0), (1e-20, 0.0), (1e-20, 4e-20)]
            for n, (x, y) in enumerate(square):
                putf(world, TRACK_BASE + OUT + 8 * n, x)
                putf(world, TRACK_BASE + OUT + 8 * n + 4, y)
            putf(world, TRACK_BASE + OUT + 0x80, 5.0)
            putf(world, TRACK_BASE + OUT + 0x84, -5.0)
            return [TRACK_BASE + OUT, TRACK_BASE + OUT + 0x80, fbits(0.5), 6]
        return [TRACK_BASE + OUT, TRACK_BASE + OUT + 0x80, fbits(radius), count]

    def sphere(world, rng, at):
        for k in range(3):
            putf(world, at + 4 * k, rng.uniform(-6, 6) if rng.random() < .95 else special(rng))
        return fbits(rng.choice([rng.uniform(0, 10), rng.uniform(0, 3), 0.0, special(rng)]))

    def portal(name, world, rng):
        placement_world(world, rng, objects=False)
        radius = sphere(world, rng, TRACK_BASE + OUT)
        index = rng.randrange(world.portal_count)
        if rng.random() < .2:
            T, at = TRACK_BASE, TRACK_BASE + PORTALS + 0x24 * index
            center = struct.unpack_from('<3f', world.track, PORTALS + 0x24 * index + 8)
            putf(world, at + 0x14, 2.0)
            for k, value in enumerate((center[0] + 3.0, center[1], center[2])):
                putf(world, T + OUT + 4 * k, value)
            plane = struct.unpack_from('<I', world.track, PORTALS + 0x24 * index + 4)[0]
            for k, value in enumerate((0.0, 0.0, 1.0, center[2])):
                putf(world, T + PLANES + 16 * plane + 4 * k, value)
            radius = fbits(1.0)
        return [index, TRACK_BASE + OUT, TRACK_BASE + SBSP, radius]

    def flood(name, world, rng):
        placement_world(world, rng, objects=False)
        radius = sphere(world, rng, TRACK_BASE + OUT)
        maximum = rng.choice([0x100, 0x100, 1, 2, 3, 0, 0x10000, 0x18000])
        return [rng.randrange(12), TRACK_BASE + OUT, radius, maximum, TRACK_BASE + OUT + 0x40]

    def reconnect_clusters(name, world, rng):
        placement_world(world, rng)
        T = TRACK_BASE
        part = rng.choice([0x4eaba4, 0x4eabb0])
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        radius = sphere(world, rng, body + 0x40)
        location = T + OUT + 0x20
        put16(world, location + 4, rng.choice([rng.randrange(12), rng.randrange(12), 0xffff]))
        forced = 0
        if name == 'cluster_partition_reconnect' and rng.random() < .3:
            forced = T + OUT + 0x40
            for k in range(16):
                world.put(forced + 4 * k, rng.choice([0, NONE, rng.getrandbits(32)]))
            world.put(T + SBSP + 0x9c, rng.choice([0, 12, 32, 257, 300, 511]))
        size = rng.choice([0x14, 0x14, 0, 7, 0x10, 0x13])
        return [forced, part, world.handles[n], body + 0x60, body + 0x40, radius, location, size]

    v.TRACK_ARGUMENTS['placement_axis'] = axis
    v.TRACK_ARGUMENTS['placement_circle'] = circle
    v.TRACK_ARGUMENTS['placement_portal'] = portal
    v.TRACK_ARGUMENTS['placement_flood'] = flood
    v.TRACK_ARGUMENTS['placement_reconnect_clusters'] = reconnect_clusters
    v.TRACK_COUNTS.update(dominant_axis=300, circle_inside_portal=400, portal_intersects_sphere=600,
                          cluster_portal_flood=400, cluster_partition_reconnect=500,
                          cluster_partition_reconnect_bare=150)

    # B8D90 (+10DDE0/118120), B92C0, B9330, B1870, 149AD0, 1496F0, 1CC190, 1CC290, 282C10, BA630.
    v.KERNELS['object_in_active_clusters'] = (['placement_active'], 'boolean')
    v.ROUTINES['object_in_active_clusters'] = dict(entry=0xb8d90, regs={'eax': 0}, stack=[1])
    INDIRECT['object_in_active_clusters'] = [0x118120]
    v.KERNELS['object_activate'] = (['placement_activation'], 'void')
    v.ROUTINES['object_activate'] = dict(entry=0xb92c0, regs={'edi': 0})
    OPAQUE['object_activate'] = HAVOK
    v.KERNELS['object_deactivate'] = (['placement_activation'], 'void')
    v.ROUTINES['object_deactivate'] = dict(entry=0xb9330, regs={'edx': 0})
    OPAQUE['object_deactivate'] = HAVOK | UNPORTED
    v.KERNELS['object_delete_protected'] = (['placement_protected'], 'boolean')
    v.ROUTINES['object_delete_protected'] = dict(entry=0xb1870, regs={'eax': 0})
    v.KERNELS['object_create_physics'] = (['placement_physics'], 'void')
    v.ROUTINES['object_create_physics'] = dict(entry=0x1cc190, stack=[0])
    OPAQUE['object_create_physics'] = HAVOK
    v.KERNELS['object_create_physics_tree'] = (['placement_physics_tree'], 'void')
    v.ROUTINES['object_create_physics_tree'] = dict(entry=0x1cc290, stack=[0, 1])
    OPAQUE['object_create_physics_tree'] = HAVOK | UNPORTED
    v.KERNELS['process_dirty_islands'] = (['placement_islands'], 'void')
    v.ROUTINES['process_dirty_islands'] = dict(entry=0x282c10)
    OPAQUE['process_dirty_islands'] = HAVOK | UNPORTED
    v.KERNELS['object_reconnect_to_map'] = (['placement_reconnect'], 'void')
    v.ROUTINES['object_reconnect_to_map'] = dict(entry=0xba630, stack=[0, 1])
    OPAQUE['object_reconnect_to_map'] = HAVOK | UNPORTED | OTHER_TRACKS
    INDIRECT['object_reconnect_to_map'] = [0x118120]
    # (light attachments join object_reconnect_to_map below, after light_world)

    def bits(world, rng):
        for k in range(16):
            world.put(PVS + 0x11b8 + 4 * k, rng.choice([0, NONE, rng.getrandbits(32)]))
        return PVS + 0x11b8

    def active(name, world, rng):
        placement_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        if rng.random() < .4:
            world.data[v.RECORDS + 12 * n + 3 - DATA] = 7
            world.data[world.body(n) + 0xaa - DATA] = 7
            world.put(world.body(n) + 0xb4, NONE)
        put16(world, v.RECORDS + 12 * n + 4, rng.choice([0xffff, rng.randrange(12), rng.randrange(512)]))
        return [world.handles[n], bits(world, rng)]

    def activation(name, world, rng):
        placement_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        row = v.RECORDS + 12 * n
        put16(world, row + 4, rng.choice([0xffff, rng.randrange(12)]))
        body = world.body(n)
        if rng.random() < .5:
            world.put(body + 0x14, NONE)
        kind = world.types[n]
        if (1 << kind) & 0x1883 and rng.random() < .5:
            component = rng.randrange(8)
            world.put(body + 0xb4, component)
            flags = struct.unpack_from('<I', world.track, COMPONENT_ROWS + 160 * component + 4)[0] & ~0x20000
            if name == 'object_activate':
                flags |= 0x20000   # 1CC3F0 skips; 1CC440 calls 1CC310 only when set
            world.put(TRACK_BASE + COMPONENT_ROWS + 160 * component + 4, flags)
        if kind == 0 and name == 'object_deactivate':
            world.data[body + 0x34b - DATA] = rng.choice([0, 1, 1])
            world.data[body + 0x3dc - DATA] = rng.choice([0, 2])
        return [world.handles[n]]

    def protected(name, world, rng):
        placement_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        return [world.handles[n]]

    def physics(name, world, rng):
        placement_world(world, rng)
        world.globals[0x48b870] &= ~0xff  # Havok creation is refused
        n = rng.randrange(v.ROWS - 1)
        put16(world, v.RECORDS + 12 * n + 4, rng.choice([0xffff, rng.randrange(12)]))
        for k in range(v.ROWS - 1):
            if rng.random() < .5:
                put16(world, v.RECORDS + 12 * k + 4, rng.choice([0xffff, rng.randrange(12)]))
        return [world.handles[n]]

    def physics_tree(name, world, rng):
        args = physics(name, world, rng)
        n = world.handles.index(args[0])
        members = tree(world, rng, n)
        root = rng.choice([args[0], args[0], world.handles[members[-1]] if members else args[0]])
        return [root, args[0]]

    def islands(name, world, rng):
        placement_world(world, rng)
        if rng.random() < .5:
            world.globals[0x56dfcc] = rng.choice([1, 0x80])
            world.globals[0x48b870] |= 1 << 24   # already processing: the second early exit
        return []

    def reconnect(name, world, rng):
        placement_world(world, rng)
        world.globals[0x48b870] &= ~0xff
        T = TRACK_BASE
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        row = v.RECORDS + 12 * n
        put16(world, row + 4, rng.choice([0xffff, 0xffff, rng.randrange(12)]))
        sphere(world, rng, body + 0x40)
        putf(world, body + 0x4c, rng.choice([rng.uniform(0, 8), rng.uniform(0, 2), 0.0, special(rng)]))
        point(world, rng, body + 0x64)
        flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0]
        flags &= ~(0x200000 | 0x20000 | 0x2)
        flags |= rng.choice([0, 0, 0x200000]) | rng.choice([0, 0x20000]) | rng.choice([0, 0, 0x2])
        world.put(body + 4, flags)
        if flags & 0x20000:
            world.data2[PVS + 0xc - v.DATA2] = 4   # B1870 protects: BA570 is refused
            world.put(body + 0xd4, 5)
        if rng.random() < .3:
            world.data[row + 3 - DATA] = 7
            world.data[body + 0xaa - DATA] = 7
            world.put(body + 0xb4, NONE)   # machines activate Havok components (1CC310)
        bits(world, rng)
        tree(world, rng, n)
        location = 0
        if rng.random() < .4:
            location = T + OUT + 0x20
            world.put(location, rng.getrandbits(32))
            put16(world, location + 4, rng.choice([0xffff, rng.randrange(12)]))
            put16(world, location + 6, rng.randrange(3))
        return [world.handles[n], location]

    v.TRACK_ARGUMENTS['placement_active'] = active
    v.TRACK_ARGUMENTS['placement_activation'] = activation
    v.TRACK_ARGUMENTS['placement_protected'] = protected
    v.TRACK_ARGUMENTS['placement_physics'] = physics
    v.TRACK_ARGUMENTS['placement_physics_tree'] = physics_tree
    v.TRACK_ARGUMENTS['placement_islands'] = islands
    v.TRACK_ARGUMENTS['placement_reconnect'] = reconnect
    v.TRACK_COUNTS.update(object_in_active_clusters=500, object_activate=300, object_deactivate=300,
                          object_delete_protected=100,
                          object_create_physics=300, object_create_physics_tree=300, process_dirty_islands=40,
                          object_reconnect_to_map=800)

    # B7E50, 1CD620, E24F0, 10E340, B9460.
    NETWORK, TAG_ROW, DEFINITION, DEFAULTS = 0xc000, 60, 0xe800, 0xe400
    POSITION, FORWARD, UP = 0xf400, 0xf500, 0xf600

    def network(world, rng):
        T = TRACK_BASE
        world.globals[0x4dc04c] = T + NETWORK
        world.put(T + NETWORK + 8, rng.choice([4, 4, 4, 5, 3, 0, NONE]))
        state = T + NETWORK + 0x100
        world.put(T + NETWORK + 4, state)
        world.put(state + 0x20a4, T + NETWORK + 0x10)
        for n in range(16):
            world.track[NETWORK + 0x10 + 0x44 + 8 * n] = rng.choice([0, 4, 0xff, 0xfb])

    def entity(rng):
        return (rng.getrandbits(22) << 10) | rng.randrange(16)

    def biped_world(world, rng, n):
        """Tag definition +264 bit 3 and the default forward/up vectors."""
        T = TRACK_BASE
        body = world.body(n)
        world.put(body, (rng.getrandbits(16) << 16) | TAG_ROW)
        world.put(v.DATA3 + 0x1000 + 16 * TAG_ROW + 8, T + DEFINITION)
        world.put(T + DEFINITION + 0x264, rng.choice([0, 8, 0x10, 0xfffffff7]))
        world.data[body + 0x3dc - DATA] = rng.choice([0, 3, 1])
        world.globals[0x474d10] = T + DEFAULTS
        world.globals[0x474d08] = T + DEFAULTS + 0x10
        for k in range(3):
            putf(world, T + DEFAULTS + 4 * k, rng.choice([0.0, 1.0, 0.0]))
            putf(world, T + DEFAULTS + 0x10 + 4 * k, rng.choice([1.0, 0.0, 0.0]))

    def vector(world, rng, at, z_choices=None):
        for k in range(3):
            putf(world, at + 4 * k, rng.uniform(-2, 2) if rng.random() < .85 else special(rng))
        if z_choices is not None and rng.random() < .5:
            world.put(at + 8, rng.choice(z_choices))

    v.KERNELS['simulation_entity_dirty'] = (['placement_dirty'], 'void')
    v.ROUTINES['simulation_entity_dirty'] = dict(entry=0xb7e50, stack=[0, 1])
    v.KERNELS['object_update_physics_position'] = (['placement_physics_position'], 'void')
    v.ROUTINES['object_update_physics_position'] = dict(entry=0x1cd620, regs={'eax': 0}, stack=[1, 2])
    OPAQUE['object_update_physics_position'] = HAVOK
    v.KERNELS['biped_relocation_filter'] = (['placement_biped'], 'void')
    v.ROUTINES['biped_relocation_filter'] = dict(entry=0xe24f0, stack=[0, 1, 2, 3])
    v.KERNELS['object_type_relocation'] = (['placement_biped'], 'void')
    v.ROUTINES['object_type_relocation'] = dict(entry=0x10e340, regs={'edi': 0, 'ebx': 3}, stack=[1, 2])
    INDIRECT['object_type_relocation'] = [0xe24f0]
    v.UNREACHED_VIRTUAL_OWNERS.add(0x10e340)  # type relocation callbacks: only E24F0 (bipeds)
    v.KERNELS['object_relocate'] = (['placement_relocate'], 'boolean')
    v.ROUTINES['object_relocate'] = dict(entry=0xb9460, stack=[0, 1, 2, 3, 4, 1, 5, 6, 7])
    OPAQUE['object_relocate'] = HAVOK | UNPORTED | OTHER_TRACKS
    INDIRECT['object_relocate'] = [0x118120]

    def dirty(name, world, rng):
        placement_world(world, rng)
        network(world, rng)
        return [entity(rng), rng.choice([2, 4, 0x800000, NONE])]

    def physics_position(name, world, rng):
        placement_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        component = rng.randrange(8)
        world.put(world.body(n) + 0xb4, rng.choice([NONE, component, component]))
        absent, force = rng.choice([0, 1, 0x100]), rng.choice([0, 1, 0x100])
        flags = rng.getrandbits(32)
        # 1DC270 (Havok) is refused: every case returns before it.
        choice = rng.randrange(4)
        if choice == 0:
            flags |= 0x100
        elif choice == 1:
            flags = (flags | 2) & ~0x180
            absent |= 1
        else:
            flags = (flags | 0x80) & ~0x102
            force &= ~0xff
        world.put(TRACK_BASE + COMPONENT_ROWS + 160 * component + 4, flags)
        return [world.handles[n], absent, force]

    def biped(name, world, rng):
        placement_world(world, rng)
        n = rng.randrange(v.ROWS - 1)
        biped_world(world, rng, n)
        body = world.body(n)
        if rng.random() < .7:
            world.put(body + 0x14, NONE)
        if name == 'object_type_relocation':
            world.data[body + 0xaa - DATA] = rng.choice([0, 0, 1, 2, 6, 7])
        T = TRACK_BASE
        vector(world, rng, T + FORWARD, [0, 0x80000000, 1, 0x7fc00000])
        vector(world, rng, T + UP, [0, 0x3f800000])
        if rng.random() < .3:
            world.put(T + UP + 8, struct.unpack_from('<I', world.track, DEFAULTS + 8)[0])
        if rng.random() < .15:
            for k in range(3):
                world.put(T + FORWARD + 4 * k, rng.choice([0, 0x80000000, 1, 0x00800000]))
            world.put(T + FORWARD + 8, rng.choice([1, 0x80000001]))
        if rng.random() < .1:
            k = rng.randrange(2)
            for j, value in enumerate((0, 0, 0x3f800000)):
                world.put(T + FORWARD + 4 * j, value)
            world.put(T + FORWARD + 4 * k, 0x38d1b717)
        position = rng.choice([0, T + POSITION])
        forward = rng.choice([0, T + FORWARD, T + FORWARD])
        up = rng.choice([0, T + UP, T + UP])
        return [world.handles[n], position, forward, up]

    def relocate(name, world, rng):
        placement_world(world, rng)
        world.globals[0x48b870] &= ~0xff
        network(world, rng)
        T = TRACK_BASE
        n = rng.randrange(v.ROWS - 1)
        body = world.body(n)
        row = v.RECORDS + 12 * n
        put16(world, row + 4, rng.choice([0xffff, rng.randrange(12)]))
        sphere(world, rng, body + 0x40)
        point(world, rng, body + 0x64)
        flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~(0x200000 | 0x20000)
        flags |= rng.choice([0, 0x100, 0x100]) | rng.choice([0, 0, 0x200000])
        world.put(body + 4, flags)
        if rng.random() < .5:
            world.put(body + 0x14, NONE)
        component = rng.randrange(8)
        if not (1 << world.types[n]) & 0x1883 and rng.random() < .5:
            world.put(body + 0xb4, component)
            world.put(T + COMPONENT_ROWS + 160 * component + 4, rng.getrandbits(32) | 0x100)  # 1CD620 returns
        world.put(body + 0xd4, rng.choice([NONE, entity(rng)]))
        current = struct.unpack_from('<I', world.data, body + 0xb4 - DATA)[0]
        if current != NONE:   # 1CD620 returns before Havok (1DC270)
            at = T + COMPONENT_ROWS + 160 * current + 4
            world.put(at, struct.unpack_from('<I', world.track, at - T)[0] | 0x100)
        bits(world, rng)
        tree(world, rng, n)
        for k in range(3):
            putf(world, T + POSITION + 4 * k, rng.choice([rng.uniform(-50, 50), rng.uniform(-50, 50),
                                                         rng.choice([32768.0, -32768.0, 32769.0, -40000.0, 1e9]),
                                                         special(rng)]))
        vector(world, rng, T + FORWARD)
        vector(world, rng, T + UP)
        location = 0
        if rng.random() < .3:
            location = T + OUT + 0x20
            world.put(location, rng.getrandbits(32))
            put16(world, location + 4, rng.choice([0xffff, rng.randrange(12)]))
            put16(world, location + 6, rng.randrange(3))
        forward = rng.choice([0, T + FORWARD])
        return [world.handles[n], rng.choice([0, T + POSITION, T + POSITION]), forward, T + UP if forward else 0,
                location, rng.choice([0, 1, 0x100]), rng.choice([0, 1]), rng.choice([0, 0, 1, 0x100])]

    v.TRACK_ARGUMENTS['placement_dirty'] = dirty
    v.TRACK_ARGUMENTS['placement_physics_position'] = physics_position
    v.TRACK_ARGUMENTS['placement_biped'] = biped
    v.TRACK_ARGUMENTS['placement_relocate'] = relocate
    v.TRACK_COUNTS.update(simulation_entity_dirty=200, object_update_physics_position=200,
                          biped_relocation_filter=500, object_type_relocation=300, object_relocate=800)

    # B9460 with BF050 (track nodebuild) and B95D0. Bodies take the
    # model-less BF0C0 path (definition +38 NONE: node 0 from the placement)
    # and unconnected children, so BFF20 never re-registers them.
    NODE_ROW, NODE_DEFINITION, NODE_OFFSET = 61, 0xe900, 0x200
    NODEBUILD_STOPS = {0x1dbbd0, 0xb3ae0, 0xb3b70, 0x283a90, 0x1d63c0, 0x284090, 0x126200, 0x114750, 0xf34b0,
                       0x10c0b0, 0x1d36c0, 0xbcb70, 0x1ce810, 0x37dc0a, 0x1dc7a0}
    RELOCATION_STOPS = HAVOK | UNPORTED | NODEBUILD_STOPS | (OTHER_TRACKS - {0xbf050, 0xbf0c0, 0xbff20})
    v.UNREACHED_VIRTUAL_OWNERS.update({0x10e180, 0x10e100})  # node and orientation type hooks: never reached here
    v.JUMP_TABLES.setdefault(0x1e16cc, 4)  # 1E1620 interpolation switch (as track nodebuild declares)
    v.JUMP_TABLES.setdefault(0x1a3bfc, 4)

    def nodes_world(world, rng):
        T = TRACK_BASE
        world.put(v.DATA3 + 0x1000 + 16 * NODE_ROW + 8, T + NODE_DEFINITION)
        definition = T + NODE_DEFINITION
        world.track[NODE_DEFINITION] = rng.choice([0, 1, 2, 6, 10])
        put16(world, definition + 2, rng.choice([0, 0x1000, 0xffff]))
        putf(world, definition + 4, rng.uniform(0.1, 3))
        for k in range(3):
            putf(world, definition + 8 + 4 * k, rng.uniform(-1, 1))
            putf(world, definition + 0x24 + 4 * k, rng.uniform(-1, 1))
        putf(world, definition + 0x20, rng.choice([0.0, -1.0, rng.uniform(0.1, 4)]))
        world.put(definition + 0x38, NONE)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body, (rng.getrandbits(16) << 16) | NODE_ROW)
            count = rng.randrange(1, 4)
            put16(world, body + 0x114, 0x34 * count)
            put16(world, body + 0x116, NODE_OFFSET)
            put16(world, body + 0x112, rng.choice([0xffff, 0x2d0]))
            world.data[body + 0x18 - DATA] = 0
            world.data[body + 0xc0 - DATA] &= ~0x40
            world.data[body + 0x34b - DATA] = rng.choice([0, 2])
            putf(world, body + 0xa0, rng.choice([1.0, rng.uniform(0.5, 2)]))
            for k in range(13 * count):
                putf(world, body + NODE_OFFSET + 4 * k, rng.choice([1.0, rng.uniform(-2, 2)]) if k % 13 == 0
                     else rng.uniform(-2, 2))

    def unconnect_children(world, n):
        pending = [n]
        while pending:
            body = world.body(pending.pop())
            child = struct.unpack_from('<I', world.data, body + 0x10 - DATA)[0]
            while child != NONE:
                k = child & 0xffff
                cbody = world.body(k)
                world.put(cbody + 4, struct.unpack_from('<I', world.data, cbody + 4 - DATA)[0] & ~0x100)
                pending.append(k)
                child = struct.unpack_from('<I', world.data, cbody + 0xc - DATA)[0]

    v.KERNELS['object_relocate_nodes'] = (['placement_relocate_nodes'], 'boolean')
    v.ROUTINES['object_relocate_nodes'] = dict(entry=0xb9460, stack=[0, 1, 2, 3, 4, 5, 5, 6, 7])
    OPAQUE['object_relocate_nodes'] = RELOCATION_STOPS
    INDIRECT['object_relocate_nodes'] = [0x118120]
    v.KERNELS['object_set_position'] = (['placement_set_position'], 'void')
    v.ROUTINES['object_set_position'] = dict(entry=0xb95d0, regs={'eax': 1, 'ecx': 2, 'edx': 3}, stack=[0, 4, 5])
    OPAQUE['object_set_position'] = RELOCATION_STOPS
    INDIRECT['object_set_position'] = [0x118120, 0xe24f0]

    def relocate_nodes(name, world, rng):
        values = relocate(name, world, rng)
        n = values[0] & 0xffff
        nodes_world(world, rng)
        unconnect_children(world, n)
        values[5] = rng.choice([1, 0x101])          # update_physics and rebuild_nodes
        if struct.unpack_from('<I', world.data, world.body(n) + 4 - DATA)[0] & 0x100:
            values[7] = 0                            # connected objects are disconnected first
        return values

    def set_position(name, world, rng):
        values = relocate_nodes(name, world, rng)
        n = values[0] & 0xffff
        body = world.body(n)
        biped_world(world, rng, n)
        world.put(body, (rng.getrandbits(16) << 16) | NODE_ROW)   # BF0C0 reads the same definition
        if values[1] and struct.unpack_from('<I', world.data, body + 0x14 - DATA)[0] == NONE:
            # Out-of-bounds roots are deleted (BA570) unless protected.
            world.data2[PVS + 0xc - v.DATA2] = 4
            world.put(body + 0xd4, entity(rng))
        return [values[0], values[1], values[2], values[3], values[4], values[6]]

    v.TRACK_ARGUMENTS['placement_relocate_nodes'] = relocate_nodes
    v.TRACK_ARGUMENTS['placement_set_position'] = set_position
    v.TRACK_COUNTS.update(object_relocate_nodes=600, object_set_position=600)

    # Object markers: BAC70, BC330, BACD0, 1E11B0, 144910, 1E1200, BAD60.
    MARK_DEF_ROW, MARK_MODEL_ROW, MARK_RENDER_ROW = 62, 63, 64
    MARK_DEF, MARK_ATTACH, MARK_MODEL, MARK_RENDER, MARK_GROUPS, MARK_ROWS, MARK_OUT = \
        0x7000, 0x7080, 0x7100, 0x7180, 0x7200, 0x7300, 0x7800
    MARKER_NAMES = [0x1000001, 0x2000002, 0x3000003, 0x4000004]

    def tag_row(world, row, data, salt):
        world.put(v.DATA3 + 0x1000 + 16 * row + 8, data)
        return (salt << 16) | row

    def marker_world(world, rng):
        track(world, rng)
        T = TRACK_BASE
        definition = tag_row(world, MARK_DEF_ROW, T + MARK_DEF, 0x7701)
        model = tag_row(world, MARK_MODEL_ROW, T + MARK_MODEL, 0x7702)
        render = tag_row(world, MARK_RENDER_ROW, T + MARK_RENDER, 0x7703)
        render_handle[0] = render
        world.track[MARK_DEF + 2] = rng.choice([0, 0x10, 0xef, 0xff])
        world.put(T + MARK_DEF + 0x38, rng.choice([model, model, model, NONE]))
        world.put(T + MARK_DEF + 0x94, rng.choice([0, 1, 3, NONE]))
        world.put(T + MARK_DEF + 0x98, T + MARK_ATTACH)
        for k in range(3):
            world.put(T + MARK_ATTACH + 24 * k + 8, rng.choice(MARKER_NAMES + [NONE, 0xa0000b6]))
        world.put(T + MARK_MODEL + 4, rng.choice([render, render, render, NONE]))
        groups = rng.choice([0, 1, 2, 4, 4])
        world.put(T + MARK_RENDER + 0x58, groups)
        world.put(T + MARK_RENDER + 0x5c, T + MARK_GROUPS)
        names = rng.sample(MARKER_NAMES, 4)
        if rng.random() < .2:
            names[rng.randrange(4)] = 0   # a group named 0: 1E11B0 never matches name 0
        if rng.random() < .3:
            names[rng.randrange(4)] = 0xa0000b6   # 16918F's hands marker
        for g in range(4):
            at = T + MARK_GROUPS + 12 * g
            world.put(at, names[g])
            count = rng.choice([0, 1, 2, 3, 5])
            world.put(at + 4, count)
            rows = T + MARK_ROWS + 0x100 * g
            world.put(at + 8, rows)
            for n in range(count):
                marker = rows + 0x24 * n
                world.track[marker - T] = rng.choice([0xff, 0xff, 0, 1, 3, 0x80, 0xfe])   # signed region index
                world.track[marker - T + 1] = rng.choice([0, 1, 2])
                world.track[marker - T + 2] = rng.randrange(3)
                for k in range(3):
                    putf(world, marker + 4 + 4 * k, rng.uniform(-2, 2))
                quaternion(world, rng, marker + 0x10)
        for n in range(v.ROWS - 1):
            body = world.body(n)
            world.put(body, definition)
            put16(world, body + 0x114, 3 * 0x34)
            put16(world, body + 0x116, 0x200)
            put16(world, body + 0x11a, 0x2e0)
            for k in range(4):
                world.data[body + 0x2e0 + k - DATA] = rng.choice([0, 1, 2])
            flags = struct.unpack_from('<I', world.data, body + 4 - DATA)[0] & ~1
            world.put(body + 4, flags | rng.choice([0, 0x400]))
            # BACD0's FC510 path: an item (+AA types 2..4, +4 bit 0) whose
            # +12C bits 0/1 select parent mode carries its markers on +154.
            if rng.random() < .3:
                world.put(body + 4, flags | 1 | rng.choice([0, 0x400]))
                world.data[body + 0xaa - DATA] = rng.choice([2, 3, 4, 4, 0, 5])
                world.data[body + 0x12c - DATA] = rng.choice([1, 1, 3, 0, 2, 0xfd])
                world.put(body + 0x154, rng.choice(world.handles[:v.ROWS - 1] + [NONE]))
            for k in range(39):
                putf(world, body + 0x200 + 4 * k, rng.uniform(-2, 2))
        return render

    def quaternion(world, rng, at):
        values = [rng.uniform(-1, 1) for _ in range(4)]
        if rng.random() < .1:
            # Norm exactly the 0x46A0EC epsilon (0x3C23D70A squared).
            values = [0.0, 0.0, 0.0, 0.0]
            values[rng.randrange(4)] = struct.unpack('<f', struct.pack('<I', 0x3c23d70a))[0]
        elif rng.random() < .2:
            values = [0.0, 0.0, 0.0, rng.choice([0.0, 1e-3, 1.0])]
        elif rng.random() < .1:
            values[rng.randrange(4)] = special(rng)
        for k in range(4):
            putf(world, at + 4 * k, values[k])

    v.KERNELS['object_attachment_marker'] = (['placement_attachment'], 'long')
    v.ROUTINES['object_attachment_marker'] = dict(entry=0xbac70, regs={'ecx': 0, 'eax': 1})
    v.KERNELS['object_root_location'] = (['placement_root_location'], 'void')
    v.ROUTINES['object_root_location'] = dict(entry=0xbc330, regs={'ecx': 0, 'eax': 1})
    v.KERNELS['object_marker_owner'] = (['placement_marker_owner'], 'long')
    v.ROUTINES['object_marker_owner'] = dict(entry=0xbacd0, stack=[0])
    v.KERNELS['model_marker_index'] = (['placement_marker_index'], 'long')
    v.ROUTINES['model_marker_index'] = dict(entry=0x1e11b0, regs={'ecx': 0, 'edi': 1})
    v.KERNELS['quaternion_rotation'] = (['placement_quaternion'], 'void')
    v.ROUTINES['quaternion_rotation'] = dict(entry=0x144910, regs={'ecx': 0, 'eax': 1})
    v.KERNELS['model_marker_matrices'] = (['placement_marker_matrices'], 'long')
    v.ROUTINES['model_marker_matrices'] = dict(entry=0x1e1200, regs={'eax': 0, 'ecx': 1}, stack=[2, 7, 3, 4, 5, 6])
    v.KERNELS['object_marker_matrices'] = (['placement_object_markers'], 'short')
    v.ROUTINES['object_marker_matrices'] = dict(entry=0xbad60, regs={'ecx': 3, 'ebx': 4}, stack=[0, 1, 2])

    def attachment(name, world, rng):
        marker_world(world, rng)
        return [world.handles[rng.randrange(v.ROWS - 1)], rng.choice([0, 1, 2, 0xffff, 0x8000, 0x10001])]

    def root_location(name, world, rng):
        track(world, rng)
        return [world.handles[rng.randrange(v.ROWS - 1)], TRACK_BASE + MARK_OUT]

    def marker_owner(name, world, rng):
        marker_world(world, rng)
        return [world.handles[rng.randrange(v.ROWS - 1)]]

    def marker_index(name, world, rng):
        render = marker_world(world, rng)
        return [rng.choice([render, render, NONE]), rng.choice(MARKER_NAMES + [0, NONE, 0x5000005])]

    def quaternion_case(name, world, rng):
        track(world, rng)
        quaternion(world, rng, TRACK_BASE + MARK_OUT + 0x100)
        return [TRACK_BASE + MARK_OUT + 0x100, TRACK_BASE + MARK_OUT]

    def marker_matrices(name, world, rng):
        render = marker_world(world, rng)
        body = world.body(rng.randrange(v.ROWS - 1))
        # Arg 7: a first-person node map (dwords; the low word is the node).
        node_map = 0
        if rng.random() < .4:
            node_map = body + 0x300
            for k in range(4):
                world.put(node_map + 4 * k, rng.choice([0, 1, 2, 0x10001, 0xffff0002]))
        return [rng.choice([0, 1, 2, 3, NONE]), render, rng.choice([body + 0x2e0, 0]), body + 0x200,
                rng.choice([0, 1, 0x100]), TRACK_BASE + MARK_OUT, rng.choice([0, 1, 2, 6, NONE]), node_map]

    def object_markers(name, world, rng):
        marker_world(world, rng)
        return [world.handles[rng.randrange(v.ROWS - 1)], rng.choice(MARKER_NAMES + [0, NONE, 0x5000005]),
                rng.choice([1, 2, 6, 0, 0xffff]), rng.choice([0, 0, 1, 0x100]), TRACK_BASE + MARK_OUT]

    v.TRACK_ARGUMENTS['placement_attachment'] = attachment
    v.TRACK_ARGUMENTS['placement_root_location'] = root_location
    v.TRACK_ARGUMENTS['placement_marker_owner'] = marker_owner
    v.TRACK_ARGUMENTS['placement_marker_index'] = marker_index
    v.TRACK_ARGUMENTS['placement_quaternion'] = quaternion_case
    v.TRACK_ARGUMENTS['placement_marker_matrices'] = marker_matrices
    v.TRACK_ARGUMENTS['placement_object_markers'] = object_markers
    v.TRACK_COUNTS.update(object_attachment_marker=200, object_root_location=100, object_marker_owner=200,
                          model_marker_index=200, quaternion_rotation=600, model_marker_matrices=500,
                          object_marker_matrices=600)

    # C4D30 (+C48E0/C4C80 sphere path, BAD60, BC330, 11E600, 1D2F10).
    LIGHT_DEF_ROW, LIGHT_DEF = 65, 0x7b00

    FIRST_PERSON = 0x560000   # an image .bss block: four 0x20CC users, players, globals
    FP_PLAYERS, FP_GLOBALS, FP_HANDS = FIRST_PERSON + 0x8400, FIRST_PERSON + 0x8d00, FIRST_PERSON + 0x8e00
    FP_ENTRIES = FIRST_PERSON + 0x9200
    render_handle = [None]

    def first_person_world(world, rng, render):
        """1692E0/16918F: users at [0x4F6498] (+8 variant; slots of 0x1010 at
        +C: flags, weapon, node maps +FC/+1FC, node matrices +308; view
        matrix +2060), players [0x4F54F4] (+28 user), directors 0x4F5514
        (+4, +C mode), [0x4ECC1C] +13C hands rows, the weapon definition's
        +2A8/+2AC first-person models, unit +13C players."""
        T = TRACK_BASE
        bank = bytearray(rng.getrandbits(8) for _ in range(0x9600))
        world.extra[FIRST_PERSON] = bank
        put = world.put
        world.globals[0x4f6498] = FIRST_PERSON
        world.globals[0x4f54f4] = FP_PLAYERS
        weapon = world.handles[5]
        candidates = [weapon, weapon, weapon, world.handles[rng.randrange(v.ROWS - 1)], NONE]
        for u in range(4):
            user = FIRST_PERSON + 0x20cc * u
            put(user + 8, rng.choice([0, 1, 2, 3, 4, 0x80000001]))
            for slot in range(2):
                record = user + 0xc + 0x1010 * slot
                put(record, rng.choice([1, 3, 7, 6, 2, 4, 0]) | rng.getrandbits(8) << 8 & 0xf800)
                put(record + 4, rng.choice(candidates))
                for table in (0xfc, 0x1fc):
                    for k in range(4):
                        put(record + table + 4 * k, rng.choice([0, 1, 2, 3, 0x10002]))
                for n in range(4):
                    for k in range(13):
                        putf(world, record + 0x308 + 0x34 * n + 4 * k, rng.choice([1.0, rng.uniform(-2, 2)]))
            for k in range(13):
                putf(world, user + 0x2060 + 4 * k, rng.choice([1.0, 0.0, rng.uniform(-2, 2)]))
        put(FP_PLAYERS + 0x44, FP_PLAYERS + 0x80)
        for n in range(4):
            struct.pack_into('<h', bank, FP_PLAYERS + 0x80 + 0x21c * n + 0x28 - FIRST_PERSON, rng.choice([0, 1, 2, 3, -1]))
        directors = world.absolute[0x4f5500]   # 0x4F5514: the users' directors (in the absolute block)
        for u in range(4):
            struct.pack_into('<I', directors, 0x14 + 0x140 * u + 0xc,
                             rng.choice([0x245400, 0x245400, 0x245400, 0x245ea0, 0x16f6a0, 0x246380, 0x12345]))
            struct.pack_into('<f', directors, 0x14 + 0x140 * u + 4, rng.choice([0.0, 0.0, -0.0, 0.5, float('nan')]))
        world.extra[0x52d6a0] = bytearray([rng.choice([0, 1]), 0, 0, 0])
        put(v.GAME_GLOBALS + 0x13c, FP_HANDS)
        for n in range(5):
            put(FP_HANDS + 0xbc * n + 4, rng.choice([render, render, NONE]))
        # The weapon definition (MARK_DEF, shared): +2A8 count, +2AC rows of
        # 0x10 (+4 model, +C a nonzero tag).
        put(T + MARK_DEF + 0x2a8, rng.choice([0, 1, 2, 2, 3]))
        put(T + MARK_DEF + 0x2ac, FP_ENTRIES)
        for n in range(3):
            put(FP_ENTRIES + 16 * n + 4, rng.choice([render, render, NONE]))
            put(FP_ENTRIES + 16 * n + 0xc, rng.choice([0x12345678, 0x12345678, NONE]))
        for n in range(v.ROWS - 1):  # unit players
            put(world.body(n) + 0x13c, rng.choice([n & 3, (0xe000 + n) << 16 | (n & 3), NONE]))

    def light_world(world, rng, count=6):
        """Sphere lights (definition +4 = 0) with marker owners, in a light
        partition; returns the light handles."""
        T = TRACK_BASE
        marker_world(world, rng)
        definition = tag_row(world, LIGHT_DEF_ROW, T + LIGHT_DEF, 0x7704)
        # +0 bit 15: C4D30 fits the light's sphere into the collision BSP (16D740).
        world.put(T + LIGHT_DEF, rng.choice([0, 2, 8, 0xa, 0x100, 0x10a, 0x20, 0x22, 0x2a,
                                             0x8000, 0x8002, 0x8008, 0x8020, 0x810a]))
        first_person_world(world, rng, render_handle[0])
        # Shape: 0 sphere, 1 orthogonal frustum, 2/3/4 frustum.
        put16(world, T + LIGHT_DEF + 4, rng.choice([0, 0, 1, 2, 2, 3, 4]))
        for offset, value in ((0x20, rng.choice([0.0, rng.uniform(0.1, 2)])), (0x24, rng.uniform(0.3, 2)),
                              (0x28, rng.uniform(0.2, 2.5)), (0x2c, rng.uniform(0, 1)), (0x30, rng.uniform(0.5, 8))):
            putf(world, T + LIGHT_DEF + offset, value)
        if rng.random() < .15:
            # Zero angle and near width: C3820's spread gate fails (C4C80 default).
            putf(world, T + LIGHT_DEF + 0x28, 0.0)
            putf(world, T + LIGHT_DEF + 0x20, 0.0)
        world.put(T + LIGHT_DEF + 0x7c, NONE)
        if rng.random() < .4:
            bitmap = tag_row(world, 66, T + LIGHT_DEF + 0x200, 0x7705)
            world.put(T + LIGHT_DEF + 0x7c, bitmap)
            world.put(T + LIGHT_DEF + 0x200 + 0x48, T + LIGHT_DEF + 0x280)
            put16(world, T + LIGHT_DEF + 0x284, rng.choice([64, 128, 256, 7]))
            put16(world, T + LIGHT_DEF + 0x286, rng.choice([64, 32, 256, 300, -16 & 0xffff]))
        # C3820 view gate, the global origin and the [0x51DAE8] switch.
        world.globals[0x474ce8] = T + LIGHT_DEF + 0x300
        for k in range(3):
            putf(world, T + LIGHT_DEF + 0x300 + 4 * k, rng.choice([0.0, 0.0, rng.uniform(-1, 1)]))
        # C3AB0's orthogonal slopes: the global zero 2-D vector [0x474CD4]
        # (random here, so that only its two words can match).
        world.globals[0x474cd4] = T + LIGHT_DEF + 0x310
        for k in range(2):
            putf(world, T + LIGHT_DEF + 0x310 + 4 * k, rng.choice([0.0, 0.0, -0.0, rng.uniform(-1, 1)]))
        world.globals[0x51dae8] = T + LIGHT_DEF + 0x320
        world.track[LIGHT_DEF + 0x328] = rng.choice([1, 1, 1, 0])
        world.put(v.USER_BLOCK + 0xc14, rng.randrange(8))
        world.put(v.USER_BLOCK, rng.getrandbits(32))
        sizes = rng.choice([(rng.uniform(0, 4), rng.uniform(0, 4)), (0.0, -0.0), (-0.0, 0.0),
                            (float('nan'), 1.0), (1.0, float('nan'))])
        for offset, value in ((0xc, rng.uniform(0.5, 2)), (0x18, sizes[0]), (0x1c, sizes[1]),
                              (0x98, rng.choice([0.0, 1.0, 3.0, -1.0]))):
            putf(world, T + LIGHT_DEF + offset, value)
        if rng.random() < .15:
            # Zero scale against a -0.0 minimum radius (C48E0's FCOMIP tie).
            putf(world, T + LIGHT_DEF + 0xc, 0.0)
            putf(world, T + LIGHT_DEF + 0x98, -0.0)
        light_root = T + LIGHT_PARTITION
        world.globals[0x4ecbe0] = light_root
        world.globals[0x4ecbe4] = light_root + 0x100
        world.globals[0x4ecbe8] = light_root + 0x180
        lp = Partition.__new__(Partition)
        lp.world, lp.root = world, 0x4ecbe0
        lp.heads, lp.entries, lp.refs = light_root, light_root + 0x100, light_root + 0x180
        lp.entry_rows, lp.ref_rows = light_root + 0x200, light_root + 0x600
        lp.clusters, lp.entry_used, lp.ref_used, lp.entry_max, lp.ref_max = 12, [], [], 24, 24
        for c in range(12):
            world.put(light_root + 4 * c, NONE)
        handles = lights(world, rng, lp, count)
        kind = struct.unpack_from('<H', world.track, LIGHT_DEF + 4)[0]
        for h in handles:
            row = T + LIGHT_ROWS + 0x110 * (h & 0xffff)
            bank, base = bank_of(world, row)
            # +2 bit 2: C2870's ray march from +84 toward +90 (+A0, +9C).
            bank[row + 2 - base] = rng.choice([2, 3, 0xa, 0xb, 0, 0xf3, 6, 7, 0xe, 0xf7])
            world.put(row + 4, definition)
            # +8: a scenario light (rows 0x6C at [[0x4ECC20] + EC]); +3E bit 0 selects C39B0.
            world.put(row + 8, rng.choice([NONE, NONE, 0, 1, 2, 3]))
            point(world, rng, row + 0x18)
            putf(world, row + 0x24, rng.uniform(0, 4))
            world.put(row + 0x44, rng.getrandbits(32))
            put16(world, row + 0x48, rng.choice([0xffff, rng.randrange(12)]))
            put16(world, row + 0x4a, rng.randrange(3))
            owner = rng.choice(world.handles[:v.ROWS - 1] + [NONE])
            world.put(row + 0x4c, owner)
            put16(world, row + 0x54, rng.choice([0, 1, 2, 0xffff]) if owner != NONE else 0xffff)
            # A parent-node frame (+58 object, +5C node; 145070) for lights
            # without a marker owner.
            world.put(row + 0x58, rng.choice(world.handles[:v.ROWS - 1]) if owner == NONE and rng.random() < .6 else NONE)
            put16(world, row + 0x5c, rng.choice([0, 1, 2]))
            for k in range(9):
                putf(world, row + 0x60 + 4 * k, rng.choice([rng.uniform(-2, 2), rng.uniform(-2, 2), 0.0, float('nan')]))
            for offset in (0x84, 0xa0):
                point(world, rng, row + offset)
            # Light frame: forward, up, and a target ahead along forward.
            forward = [rng.uniform(-1, 1) for _ in range(3)]
            forward[rng.randrange(3)] += rng.choice([1.5, -1.5])
            for k in range(3):
                putf(world, row + 0xac + 4 * k, forward[k])
                putf(world, row + 0xb8 + 4 * k, rng.uniform(-1, 1))
            position = struct.unpack('<3f', bank[row + 0x84 - base:row + 0x90 - base])
            reach = rng.uniform(0.5, 4)
            if kind == 0 and rng.random() < .15:
                reach = rng.choice([0.0, 1e-6, 5e-5])   # C2870's 1e-4 length floor
            elif kind != 0 and rng.random() < .12:
                # A frustum target on or behind the light (C3AB0's fallback).
                reach = rng.choice([0.0, -0.5, -2.0])
            for k in range(3):
                putf(world, row + 0x90 + 4 * k, position[k] + forward[k] * reach)
            putf(world, row + 0xc4, rng.choice([1.0, rng.uniform(0.2, 3)]))
            putf(world, row + 0x9c, rng.choice([0.0, 0.0, -1.0, rng.uniform(0, 2)]))
        scenario_lights(world, rng)
        if struct.unpack_from('<I', world.track, LIGHT_DEF)[0] & 0x8000:
            # 16D740: a small collision scene around the first light's frame
            # position for the structure's sphere queries ([0x4ECC10]).
            row = T + LIGHT_ROWS + 0x110 * (handles[0] & 0xffff)
            bank, base = bank_of(world, row + 0x84)
            position = struct.unpack_from('<3f', bank, row + 0x84 - base)
            fit_scene(world, rng, base=T + 0xe000, center=position, largest=2, own_globals=False)
        build_links(world, rng, lp, [(h, T + LIGHT_ROWS + 0x110 * (h & 0xffff) + 0x10) for h in handles])
        return handles

    SCENARIO_LIGHTS = TRACK_BASE + 0xd800

    def scenario_lights(world, rng):
        world.put(v.SCENARIO + 0xec, SCENARIO_LIGHTS)
        # Frustum entries only where the definition is a frustum: the light
        # frames are built for its shape (degenerate targets stay unreached).
        kind = struct.unpack_from('<H', world.track, LIGHT_DEF + 4)[0]
        for k in range(4):
            entry = SCENARIO_LIGHTS + 0x6c * k
            world.track[entry + 0x3e - TRACK_BASE] = rng.choice([0, 1, 1, 0xfe, 0xff])
            put16(world, entry + 0x3c, rng.choice([kind, 1, 2, 3, 4]) if kind else 0)
            for offset in range(0x58, 0x6c, 4):
                putf(world, entry + offset, rng.choice([rng.uniform(0.2, 3), rng.uniform(0.2, 3), 0.0, 1e-40]))
        if rng.random() < .2:
            # C39B0's +1C / +18 scale with an ordered-zero, denormal or NaN +1C.
            putf(world, TRACK_BASE + LIGHT_DEF + 0x1c, rng.choice([0.0, -0.0, 1e-40, float('nan')]))

    v.KERNELS['light_reconnect'] = (['placement_light'], 'void')
    v.ROUTINES['light_reconnect'] = dict(entry=0xc4d30, stack=[0])
    OPAQUE['light_reconnect'] = LIGHT_STOPS

    def light(name, world, rng):
        placement_world(world, rng)
        handles = light_world(world, rng)
        return [rng.choice(handles)]

    v.TRACK_ARGUMENTS['placement_light'] = light
    v.TRACK_COUNTS['light_reconnect'] = 600

    # BA630 with attached sphere lights (C0F60 -> C4D30).
    v.KERNELS['object_reconnect_lights'] = (['placement_reconnect_lights'], 'void')
    v.ROUTINES['object_reconnect_lights'] = dict(entry=0xba630, stack=[0, 1])
    OPAQUE['object_reconnect_lights'] = HAVOK | UNPORTED | OTHER_TRACKS
    INDIRECT['object_reconnect_lights'] = [0x118120]

    def reconnect_lights(name, world, rng):
        values = reconnect(name, world, rng)
        handles = light_world(world, rng)
        n = values[0] & 0xffff
        members = [n]
        child = struct.unpack_from('<I', world.data, world.body(n) + 0x10 - DATA)[0]
        if child != NONE:
            members.append(child & 0xffff)
        for k in members:
            body = world.body(k)
            world.put(body + 4, struct.unpack_from('<I', world.data, body + 4 - DATA)[0] | 0x10)
            count = rng.randrange(1, 4)
            struct.pack_into('<hh', world.data, body + 0x11c - DATA, 8 * count, 0x300)
            for d in range(count):
                world.data[body + 0x300 + 8 * d - DATA] = rng.choice([0, 0, 1])
                world.put(body + 0x300 + 8 * d + 4, rng.choice(handles + [NONE]))
        owned_lights(world, rng, members, handles, marker=True)
        return values

    v.TRACK_ARGUMENTS['placement_reconnect_lights'] = reconnect_lights
    v.TRACK_COUNTS['object_reconnect_lights'] = 600

    v.KERNELS['points_bounding_sphere'] = (['placement_points'], 'void')
    v.ROUTINES['points_bounding_sphere'] = dict(entry=0x1e7330, stack=[0, 1, 2, 3])

    def points(name, world, rng):
        track(world, rng)
        T = TRACK_BASE
        count = rng.choice([0, 1, 2, 3, 5, 8, 8])
        for n in range(count):
            if n and rng.random() < .15:
                world.track[OUT + 12 * n:OUT + 12 * n + 12] = world.track[OUT + 12 * (n - 1):OUT + 12 * n]
            else:
                for k in range(3):
                    putf(world, T + OUT + 12 * n + 4 * k, rng.uniform(-5, 5) if rng.random() < .97 else special(rng))
        world.globals[0x474ce8] = T + OUT + 0x200
        return [T + OUT, count, T + OUT + 0x100, T + OUT + 0x10c]

    v.TRACK_ARGUMENTS['placement_points'] = points
    v.TRACK_COUNTS['points_bounding_sphere'] = 500

    # 16D740 (16D2A0, 1E60C0..1E6570, 120FA0, 24EDB0..24E580, 24FCC0..24F9C0,
    # 30BD0): the collision sphere fit over a scene of convex polygons.
    # A compact layout (0xF80 bytes from its base): at most 6 surfaces, 64
    # edges and vertices, 60 BSP2D lines.
    FIT = dict(cbsp=0x0, planes=0x40, nodes=0xc0, leaves=0x100, refs=0x140, bsp2d=0x260,
               surfaces=0x640, edges=0x690, vertices=0x9a0, breakable=0xdc0, io=0xe80)
    v.KERNELS['collision_sphere_fit'] = (['placement_sphere_fit'], 'boolean')
    v.ROUTINES['collision_sphere_fit'] = dict(entry=0x16d740, stack=[0, 1, 2, 3, 4, 5, 6],
                                              closure_stops=sorted(LIGHT_STOPS))

    def unit(rng):
        while True:
            u = [rng.uniform(-1, 1) for _ in range(3)]
            n = math.sqrt(sum(x * x for x in u))
            if n > 0.2:
                return [x / n for x in u]

    def cross(a, b):
        return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]

    FIT_INFO = {}   # the last fit_scene's layout: F, count, leaves, refs

    def fit_scene(world, rng, base=TRACK_BASE, center=None, largest=6, own_globals=True, least=1, size=None):
        """Convex polygons (3..8 vertices) around a sphere, each on its own
        plane (stored negated with surface bit 15 sometimes); a BSP chain
        with one node per surface plane and a leaf on the other side of each
        node; leaves referencing the surfaces (direct BSP2D leaves or a
        split line); shared and dangling edge sides; breakable surfaces."""
        track(world, rng)
        F = {k: base + o for k, o in FIT.items()}
        center = list(center) if center is not None else [rng.uniform(-2, 2) for _ in range(3)]
        radius = rng.choice([0.25, 0.5, 1.0, 1.5, 2.5, 4.0, rng.uniform(0.1, 3)])
        if size is not None:
            radius = size   # an instance definition's scene: the query's scale
        # Exact ties (binary fractions): the walk radius r + 1/16 is 0.5, a
        # diamond on the z plane through the centre has its vertices 0.5
        # away, and BSP2D lines sit 0.5 from the projected centre.
        tie = largest > 2 and rng.random() < .1
        if tie:
            center, radius = [1.25, -0.75, 0.5], 0.4375
        # Small scenes: every ray tests every feature, and the observer stops
        # an original case after 100000 instructions.
        count = rng.choice([c for c in (1, 1, 2, 2, 3, 3, 4, 5, 6) if least <= c <= largest])
        polygons, planes = [], []
        for s in range(count):
            if rng.random() < .35:
                n = [0.0, 0.0, 0.0]
                n[rng.randrange(3)] = rng.choice([1.0, -1.0])
            elif rng.random() < .12:
                # Equal magnitudes on two or three axes (the dominant-axis ties).
                h = rng.choice([0.5, 0.7071067690849304])
                n = rng.choice([[0.0, h, h], [h, 0.0, -h], [-h, h, 0.0], [h, h, h], [0.0, -h, h]])
            else:
                n = unit(rng)
            height = rng.choice([rng.uniform(-1.2, 1.2) * radius, rng.uniform(-1.2, 1.2) * radius, 0.0, 0.004,
                                 -0.004, radius, -radius, 0.5 * radius])
            foot = [c - x * height for c, x in zip(center, n)]
            a = unit(rng)
            u = cross(n, a)
            lu = math.sqrt(sum(x * x for x in u)) or 1.0
            u = [x / lu for x in u]
            w = cross(n, u)
            shift = rng.choice([0.0, 0.0, 0.5, 1.0, 2.0]) * radius
            ang = rng.uniform(0, 2 * math.pi)
            base = [f + shift * (math.cos(ang) * x + math.sin(ang) * y) for f, x, y in zip(foot, u, w)]
            k = rng.choice([3, 3, 4, 4, 5, 6, 7, 8] if count <= 2 and largest > 2 else [3, 3, 4, 4, 5] if largest > 2 else [3, 4])
            size = rng.uniform(0.2, 2.5) * radius
            start = rng.uniform(0, 2 * math.pi)
            ring = [[b + size * (math.cos(start + 2 * math.pi * i / k) * x + math.sin(start + 2 * math.pi * i / k) * y)
                     for b, x, y in zip(base, u, w)] for i in range(k)]
            if rng.random() < .25:
                ring.reverse()
            scale = rng.choice([1.0] * 18 + [1e-5, 2.0, 0.0])   # 0: a degenerate surface (24E580's sign test)
            n = [x * scale for x in n]
            d = sum(x * c for x, c in zip(n, foot))
            if tie and s == 0:
                n, d = [0.0, 0.0, 1.0], center[2]
                ring = [[center[0] + 0.5, center[1], center[2]], [center[0], center[1] + 0.5, center[2]],
                        [center[0] - 0.5, center[1], center[2]], [center[0], center[1] - 0.5, center[2]]]
            polygons.append(ring)
            planes.append((n, d))
        if count >= 2 and rng.random() < .2:
            # The centre on polygon 0 (its centroid) and an edge of polygon 1
            # through the end of the ray along polygon 0's normal (24F670's
            # exit clamp at the ray's end).
            ring = polygons[0]
            center[:] = [sum(p[k] for p in ring) / len(ring) for k in range(3)]
            n0 = planes[0][0]
            reach = radius * rng.choice([1.0, 1.0, 0.999999, 1.000001, 0.5])
            end = [c + x * reach for c, x in zip(center, n0)]
            a = cross(n0, unit(rng))
            la = math.sqrt(sum(x * x for x in a)) or 1.0
            a = [x / la * rng.uniform(0.3, 2) for x in a]
            b = [x * rng.uniform(0.3, 2) for x in unit(rng)]
            ring1 = [[e - x for e, x in zip(end, a)], [e + x for e, x in zip(end, a)],
                     [e + x + y for e, x, y in zip(end, a, b)]]
            roll = rng.random()
            if roll < .25:
                ring1[0] = list(end)   # a vertex at the ray's end
            elif roll < .5:
                # The edge's line through the ray's end, the segment short of it.
                ring1[0] = [e - 2 * x for e, x in zip(end, a)]
                ring1[1] = [e - x for e, x in zip(end, a)]
            n1 = cross([q - p for p, q in zip(ring1[0], ring1[1])], [q - p for p, q in zip(ring1[0], ring1[2])])
            l1 = math.sqrt(sum(x * x for x in n1)) or 1.0
            n1 = [x / l1 for x in n1]
            polygons[1] = ring1
            planes[1] = (n1, sum(x * c for x, c in zip(n1, ring1[0])))
        coplanar = count >= 2 and rng.random() < .12
        if coplanar:
            # Surface 1 on surface 0's plane (another plane index): edges
            # between them have a zero convexity.
            planes[1] = planes[0]
        if not tie and rng.random() < .1:
            # The centre just outside an edge of polygon 0, in its plane
            # (an offset between 1e-4 and 0.01).
            ring = polygons[0]
            i = rng.randrange(len(ring))
            a, b = ring[i], ring[(i + 1) % len(ring)]
            mid = [(x + y) / 2 for x, y in zip(a, b)]
            centroid = [sum(p[k] for p in ring) / len(ring) for k in range(3)]
            out = [m - c for m, c in zip(mid, centroid)]
            lo = math.sqrt(sum(x * x for x in out)) or 1.0
            step = rng.choice([0.002, 0.005, 0.009])
            center[:] = [m + x / lo * step for m, x in zip(mid, out)]
        if not tie and rng.random() < .15:
            # The centre on a polygon vertex or edge midpoint.
            ring = rng.choice(polygons)
            i = rng.randrange(len(ring))
            point = ring[i] if rng.random() < .5 else [(x + y) / 2 for x, y in zip(ring[i], ring[(i + 1) % len(ring)])]
            center[:] = point
        # Planes, surfaces, edges, vertices.
        for k in range(8):
            world.put(F['cbsp'] + 4 + 8 * k, 0)
        for offset, key in ((4, 'nodes'), (0xc, 'planes'), (0x14, 'leaves'), (0x1c, 'refs'), (0x24, 'bsp2d'),
                            (0x2c, 'surfaces'), (0x34, 'edges'), (0x3c, 'vertices')):
            world.put(F['cbsp'] + offset, F[key])
        put16(world, F['surfaces'] - 8, 0)   # the row an edge side without a surface (0xFFFF) names
        flips = []
        for s, (n, d) in enumerate(planes):
            flip = rng.random() < .3
            flips.append(flip)
            stored = [-x for x in n] + [-d] if flip else n + [d]
            for k in range(4):
                putf(world, F['planes'] + 16 * s + 4 * k, stored[k])
        vertex, edge = 0, 0
        first_edges = []
        for s, ring in enumerate(polygons):
            first = edge
            first_edges.append(first)
            ids = []
            for i, p in enumerate(ring):
                for k in range(3):
                    putf(world, F['vertices'] + 16 * vertex + 4 * k, p[k])
                put16(world, F['vertices'] + 16 * vertex + 0xc, first + i)   # the vertex's first edge
                ids.append(vertex)
                vertex += 1
            for i in range(len(ring)):
                e = F['edges'] + 12 * (first + i)
                side = rng.randrange(2)
                # The other side: no surface, or another one (never this one:
                # its ring walk would take the other side's next edge).
                other = rng.choice([0xffff] + [t for t in range(count) if t != s])
                if coplanar and s == 0:
                    other = 1
                nxt = first + (i + 1) % len(ring)
                put16(world, e + 2 * side, ids[i])
                put16(world, e + 2 * (1 - side), ids[(i + 1) % len(ring)])
                put16(world, e + 4 + 2 * side, nxt)
                put16(world, e + 4 + 2 * (1 - side), rng.randrange(first + len(ring)))
                put16(world, e + 8 + 2 * side, s)
                put16(world, e + 8 + 2 * (1 - side), other)
            edge += len(ring)
            row = F['surfaces'] + 8 * s
            put16(world, row, s | (0x8000 if flips[s] else 0))
            put16(world, row + 2, first)
            world.track[F['surfaces'] - TRACK_BASE + 8 * s + 4] = rng.choice([0, 0, 0, 8, 9, 0x18, 0xf7])
            world.track[F['surfaces'] - TRACK_BASE + 8 * s + 5] = rng.randrange(256)
        # The BSP chain: node s splits on plane s; one child continues, the
        # other is a leaf (or NONE).
        leaves = []
        for s in range(count):
            node = F['nodes'] + 8 * s
            put16(world, node, s)
            nxt = s + 1 if s + 1 < count else 0x800000 | len(leaves)
            if s + 1 >= count:
                leaves.append(s)
            leaf = 0x800000 | len(leaves)
            leaves.append(s)
            if rng.random() < .06:
                leaf = 0xffffff
            children = [nxt, leaf] if rng.random() < .5 else [leaf, nxt]
            bank, b0 = bank_of(world, node)
            bank[node + 2 - b0:node + 5 - b0] = children[0].to_bytes(3, 'little')
            bank[node + 5 - b0:node + 8 - b0] = children[1].to_bytes(3, 'little')
        # Leaves: references to the surfaces (bit 15 random: one side's
        # straddle matches), direct surface leaves or one split line.
        refs, lines = 0, 0
        for n in range(len(leaves)):
            chosen = rng.sample(range(count), rng.randrange(0, count + 1))
            world.track[F['leaves'] - TRACK_BASE + 4 * n + 1] = len(chosen)
            put16(world, F['leaves'] + 4 * n + 2, refs)
            for s in chosen:
                at = F['refs'] + 4 * refs
                put16(world, at, s | (0x8000 if rng.random() < .5 else 0))
                if rng.random() < .3 and lines < 60:
                    line = F['bsp2d'] + 16 * lines
                    coefficients = (rng.uniform(-1, 1), rng.uniform(-1, 1), rng.uniform(-2, 2))
                    if tie and rng.random() < .5:
                        coefficients = (1.0, 1.0, center[0] + center[1] - 0.5)   # 0.5 from the projected centre
                    for k, value in enumerate(coefficients):
                        putf(world, line + 4 * k, value)
                    put16(world, line + 0xc, 0x8000 | s)
                    put16(world, line + 0xe, 0x8000 | rng.choice([s, rng.randrange(count)]))
                    put16(world, at + 2, lines)
                    lines += 1
                else:
                    put16(world, at + 2, 0x8000 | s)
                refs += 1
        world.globals[0x4ecc10] = F['cbsp']
        world.globals[0x4f9b50] = F['breakable']
        FIT_INFO.update(F=F, count=count, leaves=len(leaves), refs=refs)
        if own_globals:
            world.absolute[0x474c24][0:2] = struct.pack('<H', rng.choice([0, 0, 1, 3, 0xffff]))
            world.globals[0x474ce8] = F['io'] + 0x80
            for k in range(3):
                putf(world, F['io'] + 0x80 + 4 * k, rng.choice([0.0, 0.0, 0.0, -0.0, 0.25]))
        return center, radius

    def read16(world, address):
        bank, base = bank_of(world, address)
        return struct.unpack_from('<H', bank, address - base)[0]

    def write_node(world, at, plane, children):
        put16(world, at, plane)
        bank, b0 = bank_of(world, at)
        bank[at + 2 - b0:at + 5 - b0] = children[0].to_bytes(3, 'little')
        bank[at + 5 - b0:at + 8 - b0] = children[1].to_bytes(3, 'little')

    def deep_chain(world, rng, center, radius):
        """1E6150's unbounded plane stack: a chain of 129..131 nodes the sphere
        straddles (planes through its centre) replaces fit_scene's BSP, so
        the walk stacks up to 131 planes and its slots 128..130 run over the
        walk record's projection state (the stop byte, the projected point).

        Nodes 0..127 walk their next node first, so no leaf is recorded
        before slot 128. Node 128 marks its plane for a next node walked
        first (the stop byte: the deeper leaves are recorded without their
        references, then the walk ends) or walks a NONE leaf first and then
        stacks its plain plane (the stop byte clear). Nodes 128..130 stack
        planes no other node uses, and the deep leaves reference them (and
        planes 0..2, which an overrun slot 128 can hold as its axis word)
        several times, so a projection that overwrites slots 128..130
        changes which later references find their plane. The nodes, leaves
        and references move to 0x2000.. of the bank."""
        F, count = FIT_INFO['F'], FIT_INFO['count']
        nodes, leaves_at, refs_at = TRACK_BASE + 0x2000, TRACK_BASE + 0x2800, TRACK_BASE + 0x3000
        length = rng.choice([129, 130, 131, 131])
        deep = {128: 4, 129: 5, 130: 6}          # the overrun slots' unique planes
        for p in range(count, 8):
            n = unit(rng) if rng.random() < .6 else [1.0 if k == p % 3 else 0.0 for k in range(3)]
            stored = n + [sum(x * c for x, c in zip(n, center))]
            for k in range(4):
                putf(world, F['planes'] + 16 * p + 4 * k, stored[k])
        state = dict(leaves=0, refs=0)

        def leaf(references):
            index = state['leaves']
            world.track[leaves_at - TRACK_BASE + 4 * index + 1] = len(references)
            put16(world, leaves_at + 4 * index + 2, state['refs'])
            first = rng.randrange(count)
            for n, plane in enumerate(references):
                # Direct surfaces, alternating: a reference whose plane a
                # projection removed from the stack loses its own surface.
                at = refs_at + 4 * state['refs']
                put16(world, at, plane)
                put16(world, at + 2, 0x8000 | (first + n) % count)
                state['refs'] += 1
            state['leaves'] += 1
            return 0x800000 | index

        def deep_references():
            pool = [4, 5, 6, 4 | 0x8000, 5 | 0x8000, 6 | 0x8000, 0, 1, 2, 7]
            return [rng.choice(pool) for _ in range(rng.choice([2, 3, 4]))]
        marked = rng.random() < .4
        for k in range(length):
            nxt = k + 1 if k + 1 < length else leaf(deep_references())
            if k < 128:
                # A NONE leaf may come first (the stop byte is then checked
                # after the second child too).
                other = 0xffffff if k < 120 or rng.random() < .5 else leaf([])
                children = [other, nxt] if other == 0xffffff and rng.random() < .3 else [nxt, other]
            elif k == 128:
                children = [nxt, leaf(deep_references())] if marked else [0xffffff, nxt]
            else:
                other = leaf(deep_references())
                children = [nxt, other] if rng.random() < .5 else [other, nxt]
            write_node(world, nodes + 8 * k, deep.get(k, 7), children)
        for offset, at in ((4, nodes), (0x14, leaves_at), (0x1c, refs_at)):
            world.put(F['cbsp'] + offset, at)

    def sphere_fit_case(name, world, rng):
        deep = rng.random() < .25
        # A centre on a coordinate plane (a projected coordinate of +0.0
        # matches plane 0 in an overrun stack slot).
        start = [0.0 if k == rng.randrange(3) else rng.uniform(-2, 2) for k in range(3)] if deep else None
        center, radius = fit_scene(world, rng, center=start, largest=2 if deep else 6, least=2 if deep else 1)
        if deep:
            deep_chain(world, rng, center, radius)
        io = TRACK_BASE + FIT['io']
        for k in range(3):
            putf(world, io + 4 * k, center[k])
        radius = rng.choice([radius] * 12 + [0.0, -0.0, -1.0, 1e-30, 1e-40, float('nan')])
        out = rng.choice([io, io + 0x20])   # C4D30 fits the light's centre in place
        flags = rng.choice([0x0d800001, 0x0d800001, 0x0d800001, 1, 0x11, 0x0d800000, 0])
        return [flags, io, fbits(radius), rng.choice([NONE, 0x12340005]), NONE, out, io + 0x40]

    v.TRACK_ARGUMENTS['placement_sphere_fit'] = sphere_fit_case
    # HALO2_SPHERE_FIT_CASES (mutation screens only) lowers the case count.
    import os
    v.TRACK_COUNTS['collision_sphere_fit'] = int(os.environ.get('HALO2_SPHERE_FIT_CASES', '1500'))

    # 16D2A0's cluster loops (2026-11-16, continuation): 16CF00, the
    # instanced-geometry query (16B810, 144090, 186E30, 1E60C0 in the
    # instance's space, and 24EDB0's transform paths in 24E710, 24E850 and
    # 24EB60), and the object chain walk. 16D0E0 stays refused: every
    # chained object is already visited, so the original never calls it.
    # A structure BSP view at IG maps the walk's leaves to clusters; each
    # cluster lists instances; each instance names a definition whose
    # collision BSP is a small fit_scene in the instance's space, placed so
    # that the query centre lands on the definition's scene centre (or near
    # it). The visit blocks 0x4F3CE0 (clusters), 0x4F44E8 (instances, an
    # image .bss block) and 0x4EABC8 (objects, in the 0x4EABA0 block) hold
    # random, current and already-visited stamps.
    IG = TRACK_BASE + 0x4000
    IGL = dict(leaves=0x200, clusters=0x400, lists=0xa00, pathfinding=0xb00, refs=0xb80, instances=0xc00,
               definitions=0xe80, heads=0x1200, entries=0x1280, entry_rows=0x1300, scenes=0x2000)
    v.KERNELS['collision_sphere_fit_instances'] = (['placement_sphere_fit_instances'], 'boolean')
    v.ROUTINES['collision_sphere_fit_instances'] = dict(entry=0x16d740, stack=[0, 1, 2, 3, 4, 5, 6],
                                                        closure_stops=sorted(LIGHT_STOPS))

    def f32(x):
        return struct.unpack('<f', struct.pack('<f', x))[0]

    def copy_bytes(world, dst, src, n):
        sb, s0 = bank_of(world, src)
        db, d0 = bank_of(world, dst)
        db[dst - d0:dst - d0 + n] = sb[src - s0:src - s0 + n]

    def rotation(rng):
        """The forward, left and up axes: the identity, a signed axis
        permutation, or a random rotation."""
        roll = rng.random()
        if roll < .3:
            return [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
        if roll < .55:
            out = []
            for a in rng.sample(range(3), 3):
                axis = [0.0, 0.0, 0.0]
                axis[a] = rng.choice([1.0, -1.0])
                out.append(axis)
            return out
        q = [rng.gauss(0, 1) for _ in range(4)]
        n = math.sqrt(sum(x * x for x in q)) or 1.0
        w, x, y, z = [c / n for c in q]
        return [[1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)],
                [2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)],
                [2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)]]

    def f32_step(x, k):
        """The float k representable values from x (x positive)."""
        bits = struct.unpack('<I', struct.pack('<f', x))[0] + k
        return struct.unpack('<f', struct.pack('<I', bits & 0x7fffffff))[0]

    def bounding_tie(rng, center, query):
        """A bounding sphere at an exact tie of 16CF00's test: (r + query)^2
        equals d^2 summed as the original sums it, (z + x) + y. Most offsets
        have all three components nonzero, so another summation order
        rounds d^2 differently. None when no tie is found."""
        c = [f32(x) for x in center]
        sq = lambda v: f32(v * v)
        for _ in range(64):
            b = [f32(ck + rng.choice([rng.uniform(-3, 3)] * 4 + [4.0, 0.0])) for ck in c]
            dx, dy, dz = (f32(b[k] - c[k]) for k in range(3))
            d2 = f32(f32(sq(dz) + sq(dx)) + sq(dy))
            if not d2 > 0:
                continue
            root = f32(math.sqrt(d2))
            for reach in (f32_step(root, k) for k in (0, -1, 1, -2, 2)):
                if sq(reach) != d2:
                    continue
                base = f32(reach - query)
                for r in (base, f32_step(base, 1), f32_step(base, -1)) if base > 0 else (base,):
                    if f32(r + query) == reach:
                        return b, r
        return None

    def instance_world(world, rng, center, radius, flags):
        T = TRACK_BASE
        query = f32(f32(radius) + 0.0625)
        clusters = rng.choice([1, 2, 3, 4, 6, 8])
        definitions = rng.choice([1, 1, 2, 3])
        instances = rng.choice([1, 2, 2, 3, 4, 6])
        world.put(IG + 0x30, IG + IGL['leaves'])
        world.put(IG + 0xa0, IG + IGL['clusters'])
        world.put(IG + 0x13c, IG + IGL['definitions'])
        world.put(IG + 0x144, IG + IGL['instances'])
        world.globals[0x4ecc18] = IG
        for n in range(64):
            put16(world, IG + IGL['leaves'] + 8 * n, rng.choice(list(range(clusters)) * 3 + [0xffff]))
        for c in range(clusters):
            row = IG + IGL['clusters'] + 0xb0 * c
            listed = rng.choice([0, 1, 1, 2, 2, 3, 4])
            world.put(row + 0x98, listed)
            world.put(row + 0x9c, IG + IGL['lists'] + 0x10 * c)
            for k in range(listed):
                put16(world, IG + IGL['lists'] + 0x10 * c + 2 * k,
                      rng.choice(list(range(instances)) * 4 + [0xffff]))
        # Definitions: a small scene in each one's space, its collision BSP
        # header copied to +70; some without surfaces or with bit 4 on the
        # last surface (16B810), breakable indices below 8 (186E30's bits).
        saved = (world.globals[0x4ecc10], world.globals[0x4f9b50], dict(FIT_INFO))
        local_centers = []
        for d in range(definitions):
            base = IG + IGL['scenes'] + 0x1000 * d
            # A third of the scenes lie around the definition's origin, where a
            # zero transform's inverse puts the query centre (144090).
            start = [rng.uniform(-2, 2) for _ in range(3)] if rng.random() < .67 else \
                [rng.choice([0.0, rng.uniform(-.05, .05)]) for _ in range(3)]
            local, _ = fit_scene(world, rng, base=base, center=start, largest=2, own_globals=False,
                                 size=rng.choice([radius, radius, None]))
            local_centers.append(local)
            row = IG + IGL['definitions'] + 0xc8 * d
            copy_bytes(world, row + 0x70, base, 0x40)
            count = FIT_INFO['count']
            world.put(row + 0x70 + 0x28, rng.choice([count] * 14 + [0, NONE]))
            surfaces = base + FIT['surfaces'] - T
            for s in range(count):
                flag = world.track[surfaces + 8 * s + 4] & ~0x10
                if s == count - 1 and rng.random() < .1:
                    flag |= 0x10
                world.track[surfaces + 8 * s + 4] = flag
                world.track[surfaces + 8 * s + 5] = rng.randrange(10)
        world.globals[0x4ecc10], world.globals[0x4f9b50] = saved[0], saved[1]
        FIT_INFO.clear()
        FIT_INFO.update(saved[2])
        # Instances: a transform that takes the definition's scene centre to
        # the query centre (or near it), and a bounding sphere.
        for i in range(instances):
            row = IG + IGL['instances'] + 0x58 * i
            d = rng.randrange(definitions)
            put16(world, row + 0x34, d)
            axes = rotation(rng)
            scale = rng.choice([1.0] * 8 + [2.0, 0.5, 0.75, 1.5, rng.uniform(0.3, 3.0), -1.0, 0.0, 1e-40,
                                            float('nan')])
            target = [c + rng.choice([0.0, 0.0, 0.0, rng.uniform(-1, 1) * radius]) for c in center]
            local = local_centers[d]
            if math.isfinite(scale) and scale != 0.0:
                origin = [t - scale * sum(axes[a][k] * local[a] for a in range(3)) for k, t in enumerate(target)]
            else:
                # A zero (or under DAZ denormal) scale takes every feature
                # to the position: near the query, it meets the rays.
                origin = [t + rng.uniform(-.5, .5) * radius for t in target] if rng.random() < .7 else \
                    [rng.uniform(-3, 3) for _ in range(3)]
            putf(world, row, scale)
            for a in range(3):
                for k in range(3):
                    putf(world, row + 4 + 12 * a + 4 * k, axes[a][k])
            for k in range(3):
                putf(world, row + 0x28 + 4 * k, origin[k])
            roll = rng.random()
            sphere = None
            if roll < .1:
                sphere = bounding_tie(rng, center, query)
            if sphere is None:
                if roll < .16:
                    sphere = ([c + 40.0 for c in center], rng.choice([1.0, 0.0]))     # out of reach
                elif roll < .18:
                    sphere = (list(target), float('nan'))
                else:
                    sphere = ([t + rng.uniform(-.5, .5) for t in target], rng.choice([10.0, 4.0, 2.0 * radius, 1.0]))
            for k in range(3):
                putf(world, row + 0x3c + 4 * k, sphere[0][k])
            putf(world, row + 0x48, sphere[1])
        # The pathfinding data's instance references (flags bit 18).
        world.put(IG + 0xc4, rng.choice([0, 1, 1, 2, NONE]))
        world.put(IG + 0xc8, rng.choice([IG + IGL['pathfinding']] * 4 + [0]))
        world.put(IG + IGL['pathfinding'] + 0x40, rng.choice([instances, instances, max(instances - 1, 0), 0, NONE]))
        world.put(IG + IGL['pathfinding'] + 0x44, IG + IGL['refs'])
        for i in range(8):
            put16(world, IG + IGL['refs'] + 4 * i, rng.choice([0xffff, 0xffff, i, 3]))
        # Visit stamps: clusters (8 at most: the 0x4F3CE0 block), instances.
        counter = rng.choice([rng.getrandbits(32)] * 9 + [NONE])
        world.absolute[0x4f3ce0][1] = rng.getrandbits(8)
        struct.pack_into('<I', world.absolute[0x4f3ce0], 4, counter)
        for c in range(clusters):
            struct.pack_into('<I', world.absolute[0x4f3ce0], 8 + 4 * c,
                             rng.choice([(counter + 1) & NONE] + [counter, rng.getrandbits(32)] * 2))
        counter = rng.choice([rng.getrandbits(32)] * 9 + [NONE])
        block = bytearray(rng.getrandbits(8) for _ in range(8 + 4 * 8))
        struct.pack_into('<I', block, 4, counter)
        for i in range(instances):
            struct.pack_into('<I', block, 8 + 4 * i,
                             rng.choice([(counter + 1) & NONE] + [counter, rng.getrandbits(32)] * 2))
        world.extra[0x4f44e8] = block
        # Objects (bit 3): the cluster partition's chains ([0x4EABB0] heads,
        # [0x4EABB4] entries), every object the walk reaches already
        # visited. An entry whose object is NONE ends its chain: the objects
        # after it are left unvisited.
        if flags & 8:
            block = world.absolute[0x4eaba0]
            heads, entries, rows = IG + IGL['heads'], IG + IGL['entries'], IG + IGL['entry_rows']
            struct.pack_into('<II', block, 0x10, heads, entries)
            counter = rng.getrandbits(32)
            block[0x28] = rng.getrandbits(8)
            struct.pack_into('<I', block, 0x2c, counter)
            for k in range(20):
                struct.pack_into('<I', block, 0x30 + 4 * k, rng.choice([counter, rng.getrandbits(32)]))
            size = rng.choice([0x10, 0x20, 0xc])
            world.put(entries + 0x24, size)
            world.put(entries + 0x44, rows)
            used = 0
            for c in range(clusters):
                chain = list(range(used, used + rng.choice([0, 0, 1, 2, 3])))
                used += len(chain)
                world.put(heads + 4 * c, (rng.getrandbits(16) << 16 | chain[0]) if chain else NONE)
                ended = False
                for n, e in enumerate(chain):
                    index = rng.randrange(20)
                    thing = NONE if rng.random() < .15 else (rng.getrandbits(16) << 16) | index
                    ended = ended or thing == NONE
                    if thing != NONE and not ended:
                        struct.pack_into('<I', block, 0x30 + 4 * index, (counter + 1) & NONE)
                    world.put(rows + size * e + 4, thing)
                    world.put(rows + size * e + 8, (rng.getrandbits(16) << 16 | chain[n + 1]) if n + 1 < len(chain) else NONE)

    def sphere_fit_instances_case(name, world, rng):
        deep = rng.random() < .1
        start = [0.0 if k == rng.randrange(3) else rng.uniform(-2, 2) for k in range(3)] if deep else None
        center, radius = fit_scene(world, rng, center=start, largest=2, least=2 if deep else 1)
        if deep:
            deep_chain(world, rng, center, radius)
        elif rng.random() < .08:
            # Only NONE leaves: the walk records none, so only an object query
            # (bit 3) takes the visit counters.
            F = FIT_INFO['F']
            for n in range(FIT_INFO['count']):
                node = F['nodes'] + 8 * n
                bank, b0 = bank_of(world, node)
                for at in (2, 5):
                    if bank[node + at + 2 - b0] & 0x80:
                        bank[node + at - b0:node + at + 3 - b0] = b'\xff\xff\xff'
        io = TRACK_BASE + FIT['io']
        for k in range(3):
            putf(world, io + 4 * k, center[k])
        flags = rng.choice([0x0d800005] * 6 + [0x0d800004, 5, 4, 0x40005, 0x0d840005, 0x40004, 0xc, 0x0d80000d, 8, 9,
                                               0x0d800001])
        instance_world(world, rng, center, radius, flags)
        radius = rng.choice([radius] * 14 + [0.0, -1.0, float('nan')])
        out = rng.choice([io, io + 0x20])
        return [flags, io, fbits(radius), rng.choice([NONE, 0x12340005]), NONE, out, io + 0x40]

    v.TRACK_ARGUMENTS['placement_sphere_fit_instances'] = sphere_fit_instances_case
    v.TRACK_COUNTS['collision_sphere_fit_instances'] = int(os.environ.get('HALO2_SPHERE_FIT_INSTANCE_CASES', '1500'))
