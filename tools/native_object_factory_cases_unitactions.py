"""Oracle cases for factory track "unitactions" (unit action system).

register(v) receives tools/validate_native_campaign_script_builtins_tu15.py as
a module (see tools/native_object_factory_cases_nodes.py for the protocol).

Fixture layout (TRACK bank offsets): unit definitions (tag rows 40..43) at
0x1000 * k (+BC flags, +1C8 seat count, +1CC seat rows, +2C0 trigger count),
seat rows (0xB0) at 0x8000 + 0x800 * k, CAF20/2199C0 output at 0xC000 and the
count word at 0xC400, requests at 0xC800, seat mapping rows ([[0x4ECC20]+22C])
at 0xD000, player rows ([0x4F54F4]) at 0xE000 and their local view histories
([0x52BE64] + 0x1B0 * local + 0x150) at 0xF000. Object bodies form a forest
(+10 first child, +C next sibling); the action block sits at body +360..+390,
the animation manager at body +280 (word +12A) and the aim block at body
+270 (word +33E; its +A4/+C4 keys at body +314/+334). Animation graphs (tag
rows 16..19) come from the weapon track's fixture builder, placed at
0x4000..0x8000 without dense mode tables (resources never resident).

Refused callees are STOPS: the original's run ends at their entry and the
native refuses them by name (the harness kernel reports the original's EAX
there). Their subtrees are closure_stops.
"""
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import native_object_factory_cases_weapon as weapon_cases  # noqa: E402  (graph fixture builder)

NONE = 0xffffffff
ROW0 = 40
SEATS, OUT, COUNT, REQUEST, MAPPING, PLAYERS, VIEWS = 0x8000, 0xc000, 0xc400, 0xc800, 0xd000, 0xe000, 0xf000
TABLE = 0x473d28
GRAPHS, GRAPHS_END = 0x4000, 0x8000
MANAGER, AIM = 0x280, 0x270
# Callbacks the native dispatchers bind; every other table callback refuses.
NATIVE_CALLBACKS = {0xeaf00, 0xed110, 0xe9870, 0xe9910, 0xec750, 0x8fdb0, 0xea970, 0xeb190, 0xeb360}
# Refused callees of the native owners (stop entries).
REFUSED = {0x106670, 0xd0520, 0x1c4600, 0x1c49d0, 0xbbac0, 0x105200, 0xea9b0, 0xcabc0, 0xf0d50, 0x116570,
           0xeab70, 0x112420, 0xd2db0, 0x105010, 0x106810}
# Callees past a refused one (never reached; kept out of the closure): ECE70
# and CECF0's AI hooks, E9E50's tail past BBAC0, and 126200 (the fixtures'
# animation resources are never resident).
UNREACHED = {0xece70, 0x111b10, 0xbbc90, 0xb2d40, 0xd37f0, 0x126200,
             0x259b0, 0x30bd0, 0xb18b0, 0xb1a70, 0xb33f0, 0xb95d0, 0xba570, 0xbad60, 0xbbe00, 0xbe200, 0xcd4a0,
             0xd33a0, 0xff820, 0x102560, 0x10eeb0, 0x207620, 0xd0e10, 0x197f2c}
# 113190's compare trees and C9590's state keys (with near misses).
STATE_OVERLAY = [0x400004a, 0x400076c, 0x400076d, 0x50000c3, 0x5000768, 0x5000769, 0x500076a, 0x500076b, 0x600005f,
                 0x600008c, 0x60000cc, 0x60000cd, 0x60006ac, 0x700002c, 0x7000543, 0x700076e, 0x80000c4, 0x800076f,
                 0x9000011, 0x9000012, 0xa000010, 0xa000013, 0xa00002d, 0xa00003e, 0xa00022d, 0xa000767, 0xb00002e,
                 0xb00022e, 0xb0005b8, 0xc000043, 0xc0005b7, 0xd00002b, 0xd000042, 0xd00022c, 0xe00002a, 0xe00003b,
                 0xe0000c3, 0xf00003a, 0x1000003c]
STATE_UNLOADED = [0x500000a, 0x5000024, 0xa000040, 0xa0005b9, 0xb0005b2, 0xc000073, 0xc000075, 0xc000077, 0xc0006b3,
                  0xc0006cd, 0xd000021, 0xe000038, 0xe00067d, 0x1000006c, 0x11000074, 0x11000076, 0x140005b3]
NEAR = sorted({(k + d) & NONE for k in STATE_OVERLAY + STATE_UNLOADED for d in (-1, 1)} - set(STATE_OVERLAY + STATE_UNLOADED))
SELECT_KEYS = [0x8000025, 0xd000539]


class GraphFixture(weapon_cases.Fixture):
    """The weapon track's graph builder over a zeroed window of the track
    bank (no dense mode tables, no mixed table)."""

    def __init__(self, v, world, rng, start, end):
        self.v, self.world, self.rng = v, world, rng
        self.cursor, self.end, self.out = start, end, None
        # Like the weapon track's zeroed bank: resource rows stay unmarked.
        world.track[start - v.TRACK:end - v.TRACK] = bytes(end - start)
        self.blocks, self.l3_rows, self.queries = [], [], []
        self.dense = False
        policy = bytearray(0x24)
        policy[0], policy[1] = rng.choice([0, 1, 1, 1]), rng.choice([0, 1, 1, 1])
        for n, (key, chained) in enumerate(weapon_cases.SELECTORS):
            struct.pack_into('<II', policy, 4 + 8 * n, key, rng.choice([chained, chained, chained, NONE]))
        world.extra[0x47c1b4] = policy
        world.extra[0x51df20] = bytearray([rng.choice([0, 1, 1]), rng.choice([0, 1, 1]), 0, 0])
        tags = weapon_cases.GRAPH_TAGS
        children = [self.graph(tags[1], rng.randrange(2, 6)), self.graph(tags[2], rng.randrange(1, 5))]
        self.main = self.graph(tags[0], rng.randrange(3, 8), children[:rng.choice([0, 1, 2, 2])])
        self.other = self.graph(tags[3], rng.randrange(1, 4))
        self.graphs = [self.main, self.other] + children

    def alloc(self, size):
        at = self.cursor
        self.cursor += (size + 15) & ~15
        if self.cursor > self.end:
            raise OverflowError('unitactions graph fixture overflowed its window')
        return at


def graph_fixture(v, world, rng):
    for _ in range(8):
        try:
            return GraphFixture(v, world, rng, v.TRACK + GRAPHS, v.TRACK + GRAPHS_END)
        except OverflowError:
            continue
    raise ValueError('unitactions graph fixture does not fit')


def action_table(v):
    image = v.Xbe(v.NEW_DEFAULT)
    word = lambda a: struct.unpack('<I', image.read(a, 4))[0]
    rows = []
    for t in range(60):
        d = word(TABLE + 4 * t)
        rows.append([d] + [word(d + 4 * k) for k in range(4)])
    return rows


def check_action_table(v, rows):
    """The native 0x473D28 literal must equal the authenticated XBE."""
    source = (Path(__file__).resolve().parents[1] / 'src/gameplay/objects/native_object_factory_unitactions_tu15.cpp').read_text()
    block = source[source.index('// BEGIN 0x473D28'):source.index('// END 0x473D28')]
    literal = [[int(a, 16) for a in re.findall(r'0x([0-9a-f]+)u', row)] for row in re.findall(r'\{(0x[^{}]*)\}', block)]
    if literal != rows:
        raise ValueError('Native 0x473D28 unit action table differs from the authenticated XBE')


def register(v):
    rows = action_table(v)
    check_action_table(v, rows)
    T, D = v.TRACK, v.DATA
    callbacks = {slot: sorted({r[1 + slot] for r in rows if r[1 + slot]}) for slot in range(4)}
    refused_callbacks = {c for slot in callbacks.values() for c in slot} - NATIVE_CALLBACKS
    stops = REFUSED | refused_callbacks
    v.REGISTER_JUMPS[0xe8e67] = callbacks[0]   # E8E50 start
    v.REGISTER_JUMPS[0xe8dde] = callbacks[1]   # E8D80 update
    v.REGISTER_JUMPS[0xe8ee1] = callbacks[2]   # E8EB0 stop
    v.REGISTER_JUMPS[0xe8f41] = callbacks[3]   # E8F10 interrupt
    v.JUMP_TABLES[0xe98ec] = 2                 # E9870 trigger of actions 0/1/A/B
    v.JUMP_TABLES[0x219af0] = 4                # 2199C0 seat flag filters 0..3
    null = {slot: [t for t in range(60) if not rows[t][1 + slot]] for slot in range(4)}
    native_update = [t for t in range(60) if rows[t][2] in NATIVE_CALLBACKS]
    refused_update = [t for t in range(60) if rows[t][2] and rows[t][2] not in NATIVE_CALLBACKS]
    native_interrupt = [t for t in range(60) if rows[t][4] in NATIVE_CALLBACKS]
    refused_interrupt = [t for t in range(60) if rows[t][4] and rows[t][4] not in NATIVE_CALLBACKS]
    refused_stop = [t for t in range(60) if rows[t][3]]

    STOP_KERNELS = set()

    def kernel(name, types, kind, entry, count, **routine):
        v.KERNELS[name] = (types, kind)
        v.ROUTINES[name] = dict(entry=entry, closure_stops=sorted(stops | UNREACHED), **routine)
        v.TRACK_COUNTS[name] = count
        STOP_KERNELS.add(name)

    banks = lambda world: [(D, world.data), (v.DATA2, world.data2), (v.DATA3, world.data3), (v.AI, world.ai),
                           (T, world.track), *world.extra.items(), *world.absolute.items()]

    def put(world, address, value, size=4):
        for base, bank in banks(world):
            if bank is not None and base <= address and address + size <= base + len(bank):
                bank[address - base:address - base + size] = (value & ((1 << 8 * size) - 1)).to_bytes(size, 'little')
                return
        raise ValueError(f'Undeclared world address {address:x}')

    def tag(row):
        return ((0x7700 + row) << 16) | row

    def chance(rng, p):
        return rng.random() < p

    live = lambda world: world.handles[:v.ROWS - 1]

    def world_setup(world, rng, actions=None):
        """Definitions, seats, the object forest, weapons, players and the
        mapping rows. `actions(n)` returns the three action words of body n."""
        world.track = bytearray(rng.getrandbits(8) for _ in range(v.TRACK_SIZE))
        for k in range(4):
            definition = T + 0x1000 * k
            put(world, v.TAG_TABLE + 16 * (ROW0 + k) + 8, definition)
            put(world, definition + 0xbc, rng.choice([0, 1 << 25, 1 << 26, 3 << 25, rng.getrandbits(32)]))
            seats = rng.choice([0, 1, 2, 3, 5, 8, 8] + ([0x30] if k == 0 and chance(rng, .2) else []))
            put(world, definition + 0x1c8, seats if chance(rng, .97) else 0xfffffffe)
            put(world, definition + 0x1cc, T + SEATS + 0x800 * k)
            put(world, definition + 0x2c0, rng.choice([0, 1, 2, 3, NONE]))
            # Weapon definition classes (CFB80/EAC70) and the +12C bit 29.
            put(world, definition + 0x288, rng.choice(weapon_cases.L2_KEYS + [0x7000101, rng.getrandbits(32)]))
            put(world, definition + 0x28c, rng.choice(weapon_cases.L3_KEYS + [0x7000101, rng.getrandbits(32)]))
            put(world, definition + 0x12c, rng.getrandbits(32) | (1 << 29) if chance(rng, .2) else rng.getrandbits(32) & ~(1 << 29))
            for s in range(11):
                row = T + SEATS + 0x800 * k + 0xb0 * s
                flags = sum(1 << b for b in (2, 3, 5, 11, 13, 18) if chance(rng, .4)) | (rng.getrandbits(32) & 0x8000_0003)
                if chance(rng, .4):
                    flags |= 1 << 5
                put(world, row, flags)
                put(world, row + 4, rng.choice(weapon_cases.L1_KEYS + [0x7000101, 0x7000101, NONE]))
                put(world, row + 0x3e, rng.choice([0xffff, 0xffff, 0, 1, 2, 4]), 2)
        graphs = graph_fixture(v, world, rng)
        for n in range(5):
            put(world, 0x473990 + 4 * n, rng.choice(STATE_OVERLAY + STATE_UNLOADED + [rng.getrandbits(32)] * 3))
        n_live = v.ROWS - 1
        parent = [None] * n_live
        for n in range(1, n_live):
            if chance(rng, .7):
                parent[n] = rng.randrange(n)
        children = {n: [c for c in range(n_live) if parent[c] == n] for n in range(n_live)}
        for n in range(n_live):
            rng.shuffle(children[n])
        for n in range(n_live):
            body = world.body(n)
            put(world, body, tag(ROW0 + rng.randrange(4)))
            kids = children[n]
            put(world, body + 0x10, world.handles[kids[0]] if kids else NONE)
            p = parent[n]
            siblings = children[p] if p is not None else []
            following = siblings[siblings.index(n) + 1:] if p is not None else []
            put(world, body + 0xc, world.handles[following[0]] if following else NONE)
            up = world.handles[p] if p is not None else NONE
            if chance(rng, .1):
                # Inconsistent but acyclic parent links (B9390 walks +14).
                up = rng.choice(world.handles[:n] + [NONE])
            put(world, body + 0x14, up)
            header_type = rng.choice([0, 0, 1, 1, 0x20, 0x21, 2, 3, 6, 0x41])
            world.data[v.RECORDS + 12 * n + 3 - D] = header_type
            world.data[body + 0xaa - D] = header_type & 0x1f if chance(rng, .85) else rng.choice([0, 1, 2, 7, 0x40])
            world.data[body + 0x10a - D] = rng.choice([0, 0, 4, 0xfb, 0xff])
            put(world, body + 4, rng.getrandbits(32) | (1 << 26) if chance(rng, .6) else rng.getrandbits(32) & ~(1 << 26))
            put(world, body + 0x12c, NONE if chance(rng, .8) else rng.getrandbits(32))
            put(world, body + 0x134, rng.choice([0, 2, 1 << 25, rng.getrandbits(32)]))
            put(world, body + 0x13c, NONE if chance(rng, .65) else ((0x9100 + (i := rng.randrange(2))) << 16) | i)
            put(world, body + 0x1fc, rng.choice([0xffff, 0xffff, 0, 1, 2, 3]), 2)
            for at in (0x212, 0x213, 0x216, 0x217):
                world.data[body + at - D] = rng.choice([0xff, 0xff, 0, 1, 2])
            if chance(rng, .4):
                world.data[body + 0x216 - D] = world.data[body + 0x212 - D]
            if chance(rng, .4):
                world.data[body + 0x217 - D] = world.data[body + 0x213 - D]
            for w in range(3):
                put(world, body + 0x218 + 4 * w, rng.choice(live(world) + [NONE, NONE]))
            put(world, body + 0x248, rng.choice(live(world) + [NONE] * 6))
            put(world, body + 0x24c, rng.choice(live(world) + [NONE] * 4))
            block = rng.choice([0x360, 0x370, 0x380, 0x390])
            put(world, body + 0x346, block, 2)
            words = actions(n) if actions else [rng.getrandbits(32) for _ in range(3)]
            for w, value in enumerate(words):
                put(world, body + block + 4 * w, value)
            put(world, body + block + 0x2c, rng.getrandbits(32))
            # E9E50's held object (action block +10/+14).
            put(world, body + block + 0x10, NONE if chance(rng, .7) else rng.choice(live(world)))
            put(world, body + block + 0x14, rng.getrandbits(16), 2)
            # Animation manager (+12A) and aim block (+33E) keys.
            put(world, body + 0x12a, MANAGER, 2)
            put(world, body + 0x33e, AIM, 2)
            graphs.manager(body + MANAGER, rng.choice(graphs.graphs) if chance(rng, .2) else graphs.main)
            if chance(rng, .7):
                put(world, body + MANAGER + 0x7c, rng.choice(STATE_OVERLAY + STATE_UNLOADED + NEAR + SELECT_KEYS +
                                                           weapon_cases.STATE_KEYS + [NONE]))
            world.data[body + MANAGER + 0x14 - D] = rng.getrandbits(8)
            for at in (0xa4, 0xc4):
                put(world, body + AIM + at, rng.choice(STATE_OVERLAY + STATE_UNLOADED + NEAR + SELECT_KEYS * 12 +
                                                       weapon_cases.STATE_KEYS * 4 + [NONE] * 4))
            # Weapon words: +16C flags (F0E00), +17C rounds (113190).
            put(world, body + 0x16c, rng.choice([0, 2, 4, 6, 0xfffd, rng.getrandbits(16)]), 2)
            put(world, body + 0x17c, rng.choice([0, 0, 1, 5, 0xffff, 0x8000, 0x7fff]), 2)
        world.globals[0x4f54f4] = T + PLAYERS
        put(world, T + PLAYERS + 0x44, T + PLAYERS + 0x100)
        for i in range(2):
            put(world, T + PLAYERS + 0x100 + 0x21c * i + 0x28, rng.choice([0xffff, 0, 1]), 2)
        world.globals[0x52be64] = T + VIEWS
        for local in range(2):
            for slot in range(4):
                put(world, T + VIEWS + 0x1b0 * local + 0x150 + 8 * slot,
                    rng.choice([0, 5, 100, 0x7fffffff, 0x80000000, rng.getrandbits(32)]))
        put(world, v.SCENARIO + 0x22c, T + MAPPING)
        for r in range(16):
            put(world, T + MAPPING + 8 * r, tag(ROW0 + rng.randrange(4)) if chance(rng, .8) else rng.getrandbits(32))
            put(world, T + MAPPING + 8 * r + 4, rng.choice([0, NONE, 1, 2, 5, rng.getrandbits(32)]))

    def mapping(rng):
        return rng.choice([NONE, NONE, (rng.randrange(5) << 16) | rng.randrange(12), rng.randrange(1, 4) << 16])

    def positive_ready_query(world):
        """One exact ready animation, with no mode fallback or remapping.

        Sparse random graphs often miss under both the correct and a wrong
        mode key. This graph separates those queries without supplying a
        callee outcome: the original still traverses every lookup table.
        """
        unit, weapon = live(world)[:2]
        body = world.body(unit & 0xffff)
        mode, class0, class1 = 0x6100004, 0x7000083, 0x2000033
        graph, top, middle, leaf, state = [T + x for x in (0x4000, 0x4100, 0x4120, 0x4140, 0x4180)]
        world.track[GRAPHS:GRAPHS_END] = bytes(GRAPHS_END - GRAPHS)
        put(world, v.TAG_TABLE + 16 * 16 + 8, graph)
        for address, key, child in ((graph + 0x30, 0, top), (top, mode, middle),
                                    (middle, class0, leaf), (leaf, class1, state)):
            put(world, address, key)
            put(world, address + 4, 1)
            put(world, address + 8, child)
        put(world, state, 0x400000c)
        put(world, state + 4, 0x0000ffff)  # animation 0 in this graph
        # No streaming request, children, mode resources or key remaps.
        world.extra[0x51df20] = bytearray(4)
        put(world, body + 0x12a, MANAGER, 2)
        put(world, body + MANAGER + 0x68, 16)
        put(world, body + MANAGER + 0x70, mode)
        put(world, body + 0x14, NONE)
        put(world, body + 0x1fc, 0xffff, 2)
        put(world, body + 0xaa, 0, 1)  # no vehicle shortcut
        put(world, body + 0x13c, NONE)  # no player/weapon-bit-29 veto
        definition = T + 0x3000
        put(world, world.body(weapon & 0xffff), tag(ROW0 + 3))
        put(world, definition + 0x288, class0)
        put(world, definition + 0x28c, class1)
        return unit, weapon, body, mode

    def weapon_triggers(world, rng):
        """Weapon bodies' trigger words +224 + 16 * n (n < 3)."""
        for n in range(v.ROWS - 1):
            body = world.body(n)
            for i in range(3):
                put(world, body + 0x224 + 16 * i, rng.choice([0xffff, 0, 1, 3, 6, 7, 0x8000]), 2)

    def request_block(world, rng, types, flag4=None):
        address = T + REQUEST
        put(world, address, rng.choice(types))
        world.track[REQUEST + 4] = flag4 if flag4 is not None else rng.choice([0, 0, 0, 1, 0x80])
        world.track[REQUEST + 5] = rng.choice([0, 1, 0xff, rng.getrandbits(8)])
        return address

    def action_words(rng, pool, p=.5):
        words = [0, 0, 0]
        if chance(rng, .15):
            return [rng.getrandbits(32) for _ in range(3)]
        for t in pool:
            if chance(rng, p):
                words[t >> 5] |= 1 << (t & 31)
        return words

    # ---- Dispatchers ----
    kernel('unitactions_request', ['ua_request'], 'boolean', 0xe8e50, 600, regs={'eax': 0, 'edi': 1})
    kernel('unitactions_request_any', ['ua_request_any'], 'void', 0xe8e50, 300, regs={'eax': 0, 'edi': 1})
    kernel('unitactions_request_type', ['ua_request_type'], 'boolean', 0xe8e10, 400, regs={'ecx': 0}, stack=[1])
    kernel('unitactions_update', ['ua_update'], 'boolean', 0xe8d80, 600, stack=[0])
    kernel('unitactions_stop', ['ua_stop'], 'void', 0xe8eb0, 300, regs={'ecx': 0, 'edi': 1})
    kernel('unitactions_interrupt', ['ua_interrupt'], 'void', 0xe8f10, 400, regs={'ecx': 0, 'edi': 1})
    kernel('unitactions_active', ['ua_active'], 'boolean', 0xe9530, 200, regs={'eax': 0, 'ecx': 1})

    def exit_bits(rng):
        words = [rng.getrandbits(32) for _ in range(3)]
        for bit, w in ((29, 1), (31, 1), (0, 2)):
            if chance(rng, .7):
                words[w] &= ~(1 << bit)
        return words

    def request_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        weapon_triggers(world, rng)
        address = request_block(world, rng, [8, 0x12, 0x1d])
        return [rng.choice(live(world)), address]

    def request_any_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        address = request_block(world, rng, list(range(60)))
        return [rng.choice(live(world)), address]

    def request_type_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        return [rng.choice([8, 0x12, 0x1d]), rng.choice(live(world))]

    def update_case(name, world, rng):
        pool = native_update * 3 + null[1] + (rng.sample(refused_update, 1) if chance(rng, .2) else [])
        world_setup(world, rng, actions=lambda n: action_words(rng, pool, .25))
        weapon_triggers(world, rng)
        return [rng.choice(live(world))]

    def stop_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), rng.choice(null[2] + (refused_stop if chance(rng, .3) else []))]

    def interrupt_case(name, world, rng):
        world_setup(world, rng)
        weapon_triggers(world, rng)
        pool = native_interrupt * 4 + null[3] + (refused_interrupt if chance(rng, .2) else [])
        return [rng.choice(live(world)), rng.choice(pool)]

    def active_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), rng.choice(list(range(60)) + [0x16, 0x1d, 0x20, 0x3f, 0x5f])]

    # ---- Callbacks ----
    kernel('unitactions_weapon_ready', ['ua_weapon_ready'], 'boolean', 0xeaf00, 600, stack=[0, 1])
    kernel('unitactions_ready_interrupt', ['ua_ready_interrupt'], 'void', 0xeb190, 500, stack=[0, 1])
    kernel('unitactions_put_away', ['ua_put_away'], 'boolean', 0xeb360, 600, stack=[0, 1])
    kernel('unitactions_channels_clear', ['ua_channels_clear'], 'void', 0x116920, 300, regs={'eax': 0})
    kernel('unitactions_seat_actions_reset', ['ua_seat_actions_reset'], 'void', 0xd0520, 500, regs={'esi': 0})
    v.ROUTINES['unitactions_seat_actions_reset']['closure_stops'].remove(0xd0520)
    kernel('unitactions_trigger_update', ['ua_trigger_update'], 'boolean', 0xe9870, 400, stack=[0, 1])
    kernel('unitactions_trigger_ready', ['ua_trigger_ready'], 'boolean', 0x105950, 400, regs={'eax': 0, 'ebx': 1})
    kernel('unitactions_trigger_interrupt', ['ua_trigger_interrupt'], 'void', 0xe9910, 300, stack=[0, 1])
    kernel('unitactions_target_clear', ['ua_target_clear'], 'void', 0xec750, 100, stack=[0, 1])
    kernel('unitactions_vehicle_exit', ['ua_vehicle_exit'], 'boolean', 0xed110, 500, stack=[0, 1])
    kernel('unitactions_seat_release', ['ua_seat_release'], 'boolean', 0xed060, 300, regs={'eax': 0})

    def ready_path(world, rng, unit):
        """Clear the unit's held weapons and held object so EAF00 reaches
        EAA60 and EAC70 (E9910 and E9E50 would refuse first)."""
        body = world.body(unit & 0xffff)
        if chance(rng, .6):
            world.data[body + 0x212 - D] = 0xff
            world.data[body + 0x213 - D] = 0xff
        if chance(rng, .6):
            block = body + struct.unpack_from('<h', world.data, body + 0x346 - D)[0]
            put(world, block + 0x10, NONE)

    def weapon_ready_case(name, world, rng):
        world_setup(world, rng)
        weapon_triggers(world, rng)
        address = request_block(world, rng, [8, 0x12], flag4=rng.choice([0, 0, 1, 0x80]))
        if chance(rng, .2):
            unit, weapon, body, _ = positive_ready_query(world)
            put(world, address, 8)
            put(world, address + 4, 1, 1)  # bypass change-blocked gating
            for offset, value in ((0x212, 0xff), (0x213, 0xff), (0x216, 0), (0x217, 0xff)):
                put(world, body + offset, value, 1)
            put(world, body + 0x218, weapon)
            block = body + struct.unpack_from('<h', world.data, body + 0x346 - D)[0]
            put(world, block + 0x10, NONE)
            # CFB80 succeeds; interrupts and EAA60 see empty hands. EAC70's
            # current-mode placeholder reaches 112420, while literal NONE
            # misses this graph and reaches EAB70 for the unseated unit.
            return [unit, address]
        unit = rng.choice(live(world))
        ready_path(world, rng, unit)
        return [unit, address]

    def trigger_update_case(name, world, rng):
        world_setup(world, rng)
        weapon_triggers(world, rng)
        return [rng.choice(live(world)), rng.choice([0, 1, 0xa, 0xb])]

    def trigger_ready_case(name, world, rng):
        world_setup(world, rng)
        weapon_triggers(world, rng)
        return [rng.choice(live(world)), rng.choice([NONE, NONE, 0, 1, 2, 5])]

    def trigger_interrupt_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), rng.choice([0, 1, 0xa, 0xb])]

    def target_clear_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), 0x1b]

    def vehicle_exit_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        return [rng.choice(live(world)), request_block(world, rng, [0x1d])]

    def seat_release_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world))]

    # ---- CECF0 ----
    kernel('unitactions_seat_drivers', ['ua_seat_drivers'], 'void', 0xcecf0, 600, stack=[0])

    def seat_drivers_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        for n in range(v.ROWS - 1):
            if chance(rng, .8):
                put(world, world.body(n) + 0x248, NONE)
            if chance(rng, .4):
                # Child exiting through 0x1F (pass 1 of alternate seats).
                body = world.body(n)
                block = body + struct.unpack_from('<h', world.data, body + 0x346 - D)[0]
                put(world, block + 4, struct.unpack_from('<I', world.data, block + 4 - D)[0] | 1 << 31)
        for k in range(4):
            for s in range(11):
                row = T + SEATS + 0x800 * k + 0xb0 * s
                if chance(rng, .35):
                    # Alternate seats (bits 11, 13) whose +3E seat drives.
                    flags = struct.unpack_from('<I', world.track, row - T)[0] | 1 << 11 | 1 << 13
                    put(world, row, flags & ~(1 << 2) if chance(rng, .5) else flags)
                    put(world, row + 0x3e, rng.choice([0, 1, 2]), 2)
        return [rng.choice(live(world)[:4] + live(world))]

    # ---- Seat queries ----
    kernel('unitactions_seat_occupant', ['ua_occupant'], 'long', 0xcb440, 300, regs={'eax': 0}, stack=[1])
    kernel('unitactions_mapping_admits', ['ua_admits'], 'boolean', 0x219b00, 400, regs={'ecx': 0, 'edx': 1}, stack=[2])
    kernel('unitactions_object_seats', ['ua_object_seats'], 'void', 0xcaf20, 500, stack=[0, 1, 2, 3])
    kernel('unitactions_mapped_seats', ['ua_mapped_seats'], 'long', 0x2199c0, 600, regs={'edx': 5}, stack=[0, 1, 2, 3, 4])

    def occupant_case(name, world, rng):
        world_setup(world, rng)
        for n in range(v.ROWS - 1):
            if chance(rng, .1):
                put(world, world.body(n) + 0x1fc, 0x8001, 2)
        return [rng.choice(live(world)), rng.choice([0, 1, 2, 3, 0xffff, 0x8001]) | (rng.getrandbits(16) << 16)]

    def admits_case(name, world, rng):
        world_setup(world, rng)
        return [mapping(rng), rng.choice(live(world)), rng.choice([0, 1, 2, 5, 31, 32, 33, -1 & NONE, 0x8001])]

    def object_seats_case(name, world, rng):
        world_setup(world, rng)
        maximum = rng.choice([0x40, 0x40, 0, 1, 3, 0x7fff, 0x10000 | 5])
        put(world, T + COUNT, rng.choice([0, 0, 0, 1, 3, 0x3f, 0x40]) | (rng.getrandbits(16) << 16))
        return [rng.choice(live(world)), T + OUT, T + COUNT, maximum]

    def mapped_seats_case(name, world, rng):
        world_setup(world, rng)
        return [mapping(rng), rng.choice([0, 1, 2, 3, 4, 7]), rng.choice([0, 1, 2, 3]), T + OUT,
                rng.choice([0x40, 0x40, 0, 1, 2, 5]), rng.choice(live(world))]

    # ---- Script cores ----
    kernel('unitactions_vehicle_unload', ['ua_vehicle_unload'], 'short', 0x11d990, 600, regs={'ecx': 0, 'edx': 1})
    kernel('unitactions_exit_vehicle', ['ua_exit_vehicle'], 'void', 0x11d670, 500, regs={'eax': 0, 'edx': 1})
    kernel('unitactions_ai_vehicle_exit', ['ua_ai_vehicle_exit'], 'void', 0x27eab0, 400, regs={'ecx': 0, 'ebx': 1})

    def unseat_some(world, rng):
        """Riders mostly without a parent link, so ED060 returns before D0520."""
        for n in range(v.ROWS - 1):
            if chance(rng, .6):
                put(world, world.body(n) + 0x14, NONE)

    def vehicle_unload_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        unseat_some(world, rng)
        return [mapping(rng), rng.choice(live(world)[:5] + [NONE])]

    def exit_vehicle_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        unseat_some(world, rng)
        return [rng.choice(live(world) + [NONE]), rng.choice([0, 0, 1, 2, 3, 3, 4, 0xffff]) | (rng.getrandbits(16) << 16)]

    def ai_vehicle_exit_case(name, world, rng):
        world_setup(world, rng, actions=lambda n: exit_bits(rng))
        unseat_some(world, rng)
        seats = mapping(rng)
        for n in range(v.ACTORS):
            put(world, v.ACTOR_ROWS + 0x888 * n + 0x26c, rng.choice([NONE, NONE, 0, 7, rng.getrandbits(32)]))
            if seats == NONE and chance(rng, .3):
                # Without a mapping, 27EAB0 tests the actor's unit for NONE.
                put(world, v.ACTOR_ROWS + 0x888 * n + 0x18, NONE)
        return [rng.randrange(v.SQUADS), seats]

    # ---- Weapon readiness owners ----
    kernel('unitactions_animation_keys', ['ua_animation_keys'], 'boolean', 0x111d10, 300,
           regs={'eax': 0, 'esi': 1, 'edx': 2})
    kernel('unitactions_state_active', ['ua_state_active'], 'boolean', 0x116720, 400, regs={'eax': 0, 'edi': 1})
    kernel('unitactions_mode_locked', ['ua_mode_locked'], 'boolean', 0xc9590, 200, regs={'eax': 0})
    kernel('unitactions_change_blocked', ['ua_change_blocked'], 'boolean', 0x113190, 800, stack=[0])
    kernel('unitactions_animation_available', ['ua_animation_available'], 'boolean', 0x1123b0, 400, regs={'eax': 0},
           stack=[1, 2, 3])
    kernel('unitactions_ready_allowed', ['ua_ready_allowed'], 'boolean', 0xcfb80, 500, regs={'edx': 2}, stack=[0, 1])
    kernel('unitactions_slot_flagged', ['ua_slot_flagged'], 'boolean', 0xf0e00, 200, regs={'ecx': 0}, stack=[1])
    kernel('unitactions_pickup_interrupt', ['ua_pickup_interrupt'], 'void', 0xea970, 300, stack=[0, 1])
    kernel('unitactions_weapon_select', ['ua_weapon_select'], 'boolean', 0xeaa60, 400, regs={'ebx': 3},
           stack=[0, 1, 2])
    kernel('unitactions_ready_animation', ['ua_ready_animation'], 'boolean', 0xeac70, 600, regs={'eax': 0},
           stack=[1, 2, 3, 4])

    def unsettle_managers(world, rng):
        """111D10's three manager tests: graph +68, channel graph +0, handle word +6."""
        for n in range(v.ROWS - 1):
            manager = world.body(n) + MANAGER
            roll = rng.random()
            if roll < .1:
                put(world, manager + 0x68, NONE)
            elif roll < .2:
                put(world, manager, NONE)
            elif roll < .3:
                put(world, manager + 6, 0xffff, 2)

    def animation_keys_case(name, world, rng):
        world_setup(world, rng)
        unsettle_managers(world, rng)
        return [rng.choice(live(world)), T + OUT, T + OUT + 0x10]

    def state_active_case(name, world, rng):
        world_setup(world, rng)
        unsettle_managers(world, rng)
        unit = rng.choice(live(world))
        body = world.body(unit & 0xffff)
        word = lambda a: struct.unpack_from('<I', world.data, a - D)[0]
        key = rng.choice([word(body + AIM + 0xa4), word(body + AIM + 0xc4), word(body + MANAGER + 0x7c)] +
                         SELECT_KEYS + [NONE, rng.getrandbits(32)])
        return [unit, key]

    def mode_locked_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        if chance(rng, .5):
            # C9550 must not mask C9590's own three-key comparison.
            body = world.body(unit & 0xffff)
            for n in range(5):
                put(world, 0x473990 + 4 * n, 0x12345678)
            put(world, body + MANAGER + 0x7c, rng.choice([0x80000c4, 0x400004a, 0x50000c3]))
        return [unit]

    def change_blocked_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        if chance(rng, .5):
            # Isolate the two original compare trees. Random unrelated aim
            # keys and C9550's lock list otherwise mask a missing state key.
            body = world.body(unit & 0xffff)
            put(world, body + 0x13c, NONE)
            for n in range(5):
                put(world, 0x473990 + 4 * n, 0x12345678)
            if chance(rng, .5):
                put(world, body + MANAGER + 0x7c, rng.choice(STATE_OVERLAY + STATE_UNLOADED + NEAR))
                put(world, body + AIM + 0xa4, NONE)
            else:
                put(world, body + MANAGER + 0x7c, NONE)
                put(world, body + AIM + 0xa4, rng.choice([0xa00022d, 0xb00022e, 0xd00022c] + STATE_UNLOADED))
        elif chance(rng, .6):
            # A player armed in both hands, rounds (+17C) around 0 and an
            # "unloaded" key: 113190's two rounds tests decide.
            body = world.body(unit & 0xffff)
            put(world, body + 0x13c, 0x91000000)
            for n in range(5):
                put(world, 0x473990 + 4 * n, 0x12345678)
            for hand, slot in ((0x212, 0), (0x213, 1)):
                world.data[body + hand - D] = slot
                weapon = rng.choice(live(world))
                put(world, body + 0x218 + 4 * slot, weapon)
                put(world, world.body(weapon & 0xffff) + 0x17c, rng.choice([0, 0, 1, 0xffff]), 2)
            if chance(rng, .5):
                put(world, body + MANAGER + 0x7c, rng.choice(STATE_UNLOADED))
                put(world, body + AIM + 0xa4, NONE)
            else:
                put(world, body + MANAGER + 0x7c, NONE)
                put(world, body + AIM + 0xa4, rng.choice(STATE_UNLOADED[:1] + STATE_UNLOADED[2:]))
        return [unit]

    def animation_available_case(name, world, rng):
        world_setup(world, rng)
        if chance(rng, .3):
            # The one exact animation, its handle possibly a missing one
            # (high word 0xFFFF): 1123B0 tests only the high word.
            unit, _, _, mode = positive_ready_query(world)
            put(world, T + 0x4184, rng.choice([0x0000ffff, 0xffff0000, 0xffff0001, 0x0001ffff, NONE]))
            return [unit, rng.choice([mode, 0x7000101]), 0x7000083, 0x2000033]
        pick = lambda pool: rng.choice(pool + [0x7000101, 0x7000101, NONE])
        return [rng.choice(live(world)), pick(weapon_cases.L1_KEYS), pick(weapon_cases.L2_KEYS),
                pick(weapon_cases.L3_KEYS)]

    def ready_allowed_case(name, world, rng):
        world_setup(world, rng)
        if chance(rng, .2):
            unit, weapon, body, mode = positive_ready_query(world)
            parent = live(world)[2]
            put(world, world.body(parent & 0xffff), tag(ROW0 + 2))
            put(world, body + 0x14, parent)
            put(world, body + 0x1fc, 0, 2)
            seat = T + SEATS + 0x800 * 2
            put(world, T + 0x2000 + 0x1cc, seat)
            put(world, seat, 1 << 5)
            put(world, seat + 4, mode)
            put(world, seat + 8, 0x2000010)
            put(world, body + MANAGER + 0x70, 0x2000010)
            # The seat supplies the only positive mode. Keeping the current
            # mode and the adjacent seat word absent distinguishes both
            # placeholder substitution and the source's +4 seat-key read.
            return [unit, rng.choice([NONE, 0x7000101]), weapon]
        key = rng.choice([NONE, NONE, 0x7000101, 0x7000101] + weapon_cases.L1_KEYS)
        return [rng.choice(live(world)), key, rng.choice(live(world))]

    def slot_flagged_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), rng.choice([0, 1])]

    def pickup_interrupt_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), 0x16]

    def weapon_select_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        body = world.body(unit & 0xffff)
        if chance(rng, .4):
            put(world, body + AIM + rng.choice([0xa4, 0xc4]), rng.choice(SELECT_KEYS))
        if chance(rng, .3):
            # An armed hand whose matching animation is already active must
            # return before the unported lowering/holster owners. Exercise
            # both hands without an empty-slot early return hiding the key.
            primary = rng.choice([0, 1])
            put(world, body + (0x212 if primary else 0x213), 0, 1)
            put(world, body + 0x218, live(world)[((unit & 0xffff) + 1) % len(live(world))])
            put(world, body + AIM + 0xa4, 0x8000025 if primary else 0xd000539)
            put(world, body + AIM + 0xc4, NONE)
            put(world, body + MANAGER + 0x7c, NONE)
            return [unit, 0, rng.choice([0, 1]), primary]
        flag = lambda: rng.choice([0, 0, 1, 0x80, 0x100, 0x1ff])
        return [unit, flag(), flag(), rng.choice([0, 1])]

    def ready_animation_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        primary = rng.choice([0, 1])
        body = world.body(unit & 0xffff)
        if chance(rng, .6):
            # A desired slot no hand holds, with a weapon.
            slot = rng.randrange(3)
            world.data[body + 0x216 + (1 - primary) - D] = slot
            for hand in (0x212, 0x213):
                if world.data[body + hand - D] == slot:
                    world.data[body + hand - D] = rng.choice([0xff, (slot + 1) % 3])
            put(world, body + 0x218 + 4 * slot, rng.choice(live(world) + [NONE]))
        key = rng.choice([0x7000101, 0x7000101, NONE] + weapon_cases.L1_KEYS)
        return [primary, unit, key, rng.choice([0, 1, 0xff]), rng.choice([0, 1, 0xff])]

    def ready_interrupt_case(name, world, rng):
        world_setup(world, rng)
        return [rng.choice(live(world)), rng.choice([8, 0x12])]

    def put_away_case(name, world, rng):
        world_setup(world, rng)
        address = request_block(world, rng, [9, 0x13])
        return [rng.choice(live(world)), address]

    def channels_clear_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        body = world.body(unit & 0xffff)
        # Exercise signed aim offsets and preserve randomized surrounding bytes.
        # The first body's negative offset would overlap the harness's HS
        # argument staging block, whose original stores are intentionally
        # excluded. Other rows exercise negative offsets without that alias.
        offsets = [AIM, 0x80, 0x100] + ([-0x100] if body >= v.BODIES + v.BODY_SIZE else [])
        put(world, body + 0x33e, rng.choice(offsets), 2)
        return [unit]

    def seat_actions_reset_case(name, world, rng):
        world_setup(world, rng)
        unit = rng.choice(live(world))
        body = world.body(unit & 0xffff)
        if chance(rng, .6):
            # Fully admitted unarmed path, including player action history.
            block = body + struct.unpack_from('<h', world.data, body + 0x346 - D)[0]
            put(world, block + 4, rng.getrandbits(32) & ~((1 << 8) | (1 << 0x12)))
            put(world, body + 0x212, 0xff, 1)
            put(world, body + 0x213, 0xff, 1)
        return [unit]

    v.TRACK_ARGUMENTS.update(
        ua_animation_keys=animation_keys_case, ua_state_active=state_active_case, ua_mode_locked=mode_locked_case,
        ua_change_blocked=change_blocked_case, ua_animation_available=animation_available_case,
        ua_ready_allowed=ready_allowed_case, ua_slot_flagged=slot_flagged_case,
        ua_pickup_interrupt=pickup_interrupt_case, ua_weapon_select=weapon_select_case,
        ua_ready_animation=ready_animation_case)
    v.TRACK_ARGUMENTS.update(ua_ready_interrupt=ready_interrupt_case, ua_put_away=put_away_case,
                            ua_channels_clear=channels_clear_case, ua_seat_actions_reset=seat_actions_reset_case)
    v.TRACK_ARGUMENTS.update(
        ua_request=request_case, ua_request_any=request_any_case, ua_request_type=request_type_case,
        ua_update=update_case, ua_stop=stop_case, ua_interrupt=interrupt_case, ua_active=active_case,
        ua_weapon_ready=weapon_ready_case, ua_trigger_update=trigger_update_case, ua_trigger_ready=trigger_ready_case,
        ua_trigger_interrupt=trigger_interrupt_case, ua_target_clear=target_clear_case,
        ua_vehicle_exit=vehicle_exit_case, ua_seat_release=seat_release_case, ua_seat_drivers=seat_drivers_case,
        ua_occupant=occupant_case, ua_admits=admits_case, ua_object_seats=object_seats_case,
        ua_mapped_seats=mapped_seats_case, ua_vehicle_unload=vehicle_unload_case, ua_exit_vehicle=exit_vehicle_case,
        ua_ai_vehicle_exit=ai_vehicle_exit_case)

    original_cases = v.cases

    def cases(seed, evaluators, tails):
        rows_ = original_cases(seed, evaluators, tails)
        for row in rows_:
            if row['kernel'] in STOP_KERNELS:
                row['stop_before_entries'] = sorted((stops - ({0xd0520} if row['kernel'] == 'unitactions_seat_actions_reset' else set())) | {v.STOP})
                row['observe_stop_entry'] = True
        return rows_

    v.cases = cases
