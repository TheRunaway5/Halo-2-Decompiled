"""Oracle cases for factory track "weapon" (102AD0 weapon new, 10A980 weapon
state, 1D5320 animation state and their callees).

Each case declares a complete animation fixture in the track bank: animation
graphs (tag rows 16..19) with 0x6C-byte animations, alias/variant chains,
events, sorted three-level mode tables whose leaves hold state-key handles,
physical-query rows (never matching the 0xFF query byte, so the refused
1E4C39 path is not entered), key remaps and resource rows (never resident, so
the refused 126200 request is not entered). Managers (0x84 bytes) hold three
channels, the graph tag and the current keys. Weapons carry a definition
(tag 24: magazines +2C0/+2C4, barrels +2C8, triggers +2D0) and a manager at
body +300. Frame out-parameters live at the track bank's start.
"""
import os
from pathlib import Path
import random
import re
import struct

NONE = 0xffffffff
D9 = 0x30000d9
STATE_KEYS = [0x400000c, 0x6000006, 0x6000007, 0x9000004, 0x9000005, 0x8000002, 0x9000009, 0x5000024, 0x8000025,
              0x0a000521, 0x09000522, 0x0a000014, 0x09000015]
L1_KEYS = [0x6000086, D9, 0x2000010, 0x6100004]
L2_KEYS = [0x7000083, D9, 0x1000005, 0x7000090, 0x400054b]
L3_KEYS = [D9, 0x2000033, 0x5000044]
SELECTORS = [(0x0a000014, 0x0a000521), (0x09000015, 0x09000522), (0x09000016, 0x09000523), (0x0a000017, 0x0a000524)]


class Pools:
    """The key pools of a fixture's graphs and managers."""
    def __init__(self, l1, l2, l3, states, selectors):
        self.l1, self.l2, self.l3, self.states, self.selectors = l1, l2, l3, states, selectors


TU15_POOLS = Pools(L1_KEYS, L2_KEYS, L3_KEYS, STATE_KEYS, SELECTORS)


def translated_query_names():
    """The translated route's first-person query names (TU1.5 -> cache) and
    dual class, read from its source: the oracle's joins (the observer's
    query_names) follow the translated route, not the native's table."""
    text = (Path(__file__).resolve().parents[1] /
            'src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp').read_text()
    block = text[text.index('constexpr std::array symbols{'):]
    block = block[:block.index('};')]
    states = [[int(a, 16), int(b, 16)] for a, b in re.findall(r'Symbol\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"', block)]
    dual = re.search(r'equip_dual_class\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"dual"\}', text)
    if len(states) != 12 or not dual:
        raise ValueError('The translated first-person query names changed shape')
    return dict(states=states, dual=[int(dual.group(1), 16), int(dual.group(2), 16)])


# 169863's 1D5320 call (it returns to 169A56) in the shape the Elongation
# reload met: a first-person state key (staged reload, melee), the mode keys
# 0x7000101 (the manager's mode), 169432's dual or any class and 0x7000001,
# flags 0x82 (the 0x400000C retry). `weapon_manager_set_state_first_person`
# runs on an MCC-named graph with the translated route's joins
# (query_names); `..._tu15` on a TU1.5-named graph without them.
FIRST_PERSON_CALLER = 0x169a56
QUERY_NAMES = translated_query_names()
FP_KERNELS = {'weapon_manager_set_state_first_person': True, 'weapon_manager_set_state_first_person_tu15': False}
DUAL, MCC_DUAL = QUERY_NAMES['dual']


def first_person_pools(rng, mcc):
    """Three first-person names in the cache's namespace (mcc) or TU1.5's, a
    stray name of the other namespace, both dual classes and selectors that
    chain a name to itself (1D4F80's chained key equal to the state key)."""
    names = rng.sample(QUERY_NAMES['states'], 3)
    native = [m if mcc else t for t, m in names]
    stray = [t if mcc else m for t, m in rng.sample(names, 1)]
    states = native + native + stray + [0x400000c, 0x400000c, 0x6000006]
    selectors = [(names[0][0], names[0][0]), (names[1][0], names[2][0]), (0x400000c, names[0][0]),
                 (0x6000006, 0x6000007)]
    return Pools([0x6000086, 0x6000086, D9, 0x2000010], [0x7000083, D9, DUAL, MCC_DUAL, MCC_DUAL if mcc else DUAL],
                 [D9, D9, 0x2000033], states, selectors), names
GRAPH_TAGS, DEFINITION_TAG, MANAGER_OFFSET = (16, 17, 18, 19), 24, 0x300
# Owners inside the refused 126200/1D62C0/CC1E0 closures: their indirect
# calls are never reached by these fixtures.
UNREACHED = {0xe8e50, 0x13fe50, 0x140310, 0x2dbc84, 0x2dbf74, 0x2e02c4, 0x2e0342, 0x2e0362,
             0x2843f0, 0x2847d0, 0x2849f0, 0x284d30, 0x284ee0, 0x285370, 0x285630, 0x285a70, 0x285cf0, 0x2861a0,
             0x286460, 0x2868a0, 0x286b30, 0x287130, 0x287480, 0x287a20, 0x287d00, 0x2883c0, 0x2887f0, 0x288e70,
             0x289240, 0x289920, 0x289d60, 0x28a3f0, 0x28a7d0, 0x28ada0, 0x28b080, 0x28b5f0, 0x28b870, 0x28bf00,
             0x28c2c0, 0x28c8f0, 0x28cc70, 0x28d310, 0x28d6e0, 0x28dd20, 0x28e0b0, 0x28e730, 0x28eaf0, 0x28f110,
             0x28f460, 0x28fba0, 0x290030, 0x290710, 0x290b50, 0x2912a0, 0x291740, 0x291e30, 0x292280, 0x2928e0,
             0x292c90, 0x293290, 0x2935c0, 0x293cb0, 0x2940f0, 0x2947b0, 0x294b80, 0x295280, 0x2956d0, 0x295da0}
# 10A980's unchecked 11-state table, 1A3B70 (126200) and CC1E0 switches.
TABLES = {0x10ab20: 11, 0x1a3bfc: 4, 0xcc30c: 8, 0xcc32c: 8}
# Per-routine closure stops: 10A980's refused owner callees (CFB40 variant,
# CC1E0 notification) and the refused 126200 resource request, none of which
# a fixture reaches. CC1E0 reaches E8E50, whose start dispatch the
# unitactions track declares (REGISTER_JUMPS 0xE8E67), so without the stop
# that closure grows through every action callback.
OWNER_STOPS = {'weapon_set_state': [0x126200, 0xcc1e0, 0xcfb40], 'weapon_new': [0x126200, 0xcc1e0, 0xcfb40]}

ROUTINES = {
    'weapon_tag_block_search': (0x1e5810, {}, [0, 1, 2], 'long'),
    'weapon_graph_child': (0x1e3270, {'edx': 0, 'ecx': 1}, [], 'long'),
    'weapon_graph_animation': (0x1e3150, {'edx': 0}, [1], 'long'),
    'weapon_graph_require_resident': (0x1e5c80, {'edx': 0}, [1], 'void'),
    'weapon_channels_reset': (0x1d4e50, {'eax': 0}, [], 'void'),
    'weapon_resolve_keys': (0x1d45a0, {'eax': 0, 'ecx': 1, 'edx': 2, 'esi': 3}, [4, 5], 'void'),
    'weapon_mode_iterate': (0x2a0100, {'esi': 0, 'ebx': 1}, [2, 3], 'long'),
    'weapon_mode_entry': (0x1e33d0, {'eax': 0, 'ecx': 1}, [2, 3], 'long'),
    'weapon_mode_prefetch': (0x1e5e40, {'ebx': 0}, [1, 2, 3, 4, 5], 'void'),
    'weapon_mode_prefetch_fallbacks': (0x1e5f20, {'eax': 0, 'edi': 1, 'esi': 2}, [3, 4, 5], 'void'),
    'weapon_graph_prefetch': (0x1e5fb0, {'eax': 0, 'ebx': 1}, [2, 3, 4, 5], 'void'),
    'weapon_manager_prefetch': (0x1d6430, {'edi': 0, 'ecx': 1, 'edx': 2}, [3, 4, 5], 'void'),
    # At most eight argument words: wide routines repeat a word (out0 = out2,
    # the second and third keys, the query bytes).
    'weapon_animation_lookup': (0x1e3420, {'eax': 0}, [1, 2, 3, 4, 5, 6, 7, 6], 'long'),
    'weapon_choose_animation': (0x1d46a0, {'eax': 0, 'ecx': 1, 'edx': 2}, [3, 4, 4, 5, 6, 7], 'boolean'),
    'weapon_physical_query_find': (0x1e3320, {'ecx': 0}, [1, 2, 3, 3, 4, 5, 6, 6, 7], 'long'),
    'weapon_physical_query_select': (0x1e4b70, {'eax': 0}, [1, 2, 3, 4, 5, 6, 6, 3, 5, 6, 6, 7], 'long'),
    'weapon_physical_query_state': (0x1d5050, {'ebx': 0}, [1, 2, 3, 4], 'long'),
    'weapon_physical_query_selector': (0x1d4f80, {'ecx': 0, 'ebx': 1}, [2, 3, 4, 5, 6], 'long'),
    'weapon_channel_stopped': (0xe8d50, {'eax': 0}, [], 'boolean'),
    'weapon_channel_animation': (0xb8780, {'ecx': 0}, [], 'long'),
    'weapon_channel_valid': (0xb8760, {'eax': 0}, [], 'long'),
    'weapon_channel_finished': (0x1cf5b0, {'esi': 0}, [], 'boolean'),
    'weapon_channel_start': (0x1ce720, {'eax': 0, 'ecx': 1, 'edx': 2}, [3, 4, 5, 6, 7], 'boolean'),
    'weapon_channel_mark_events': (0x1cec60, {'esi': 0}, [], 'void'),
    'weapon_channel_set_frame': (0x1ce810, {'eax': 0}, [1], 'void'),
    'weapon_variant_alias': (0x1e5880, {'ebx': 0, 'edi': 1}, [2], 'long'),
    'weapon_random_variant': (0x1e58e0, {}, [0, 1, 2, 3], 'long'),
    'weapon_manager_variant': (0x1d3660, {'edi': 0, 'esi': 1}, [2], 'long'),
    'weapon_manager_start_channel': (0x1d36c0, {'eax': 0, 'ebx': 1}, [2, 3, 4, 5, 6, 7], 'boolean'),
    'weapon_manager_set_state': (0x1d5320, {'eax': 0, 'ecx': 1}, [2, 3, 4, 5, 6], 'boolean'),
    'weapon_manager_set_state_first_person': (0x1d5320, {'eax': 0, 'ecx': 1}, [2, 3, 4, 5, 6], 'boolean'),
    'weapon_manager_set_state_first_person_tu15': (0x1d5320, {'eax': 0, 'ecx': 1}, [2, 3, 4, 5, 6], 'boolean'),
    'weapon_set_state': (0x10a980, {}, [0, 1, 2], 'boolean'),
    'weapon_new': (0x102ad0, {}, [0, 1, 2], 'boolean'),
    'weapon_owner': (0x106ff0, {'eax': 0}, [], 'long'),
}
COUNTS = {'weapon_tag_block_search': 200, 'weapon_graph_child': 150, 'weapon_graph_animation': 200,
          'weapon_graph_require_resident': 200, 'weapon_channels_reset': 40, 'weapon_resolve_keys': 250,
          'weapon_mode_iterate': 300, 'weapon_mode_entry': 150, 'weapon_mode_prefetch': 120,
          'weapon_mode_prefetch_fallbacks': 100, 'weapon_graph_prefetch': 100, 'weapon_manager_prefetch': 100,
          'weapon_animation_lookup': 300, 'weapon_choose_animation': 400, 'weapon_physical_query_find': 250,
          'weapon_physical_query_select': 200, 'weapon_physical_query_state': 250,
          'weapon_physical_query_selector': 250, 'weapon_channel_stopped': 60, 'weapon_channel_animation': 80,
          'weapon_channel_valid': 60, 'weapon_channel_finished': 100, 'weapon_channel_start': 150,
          'weapon_channel_mark_events': 300, 'weapon_channel_set_frame': 400, 'weapon_variant_alias': 200,
          'weapon_random_variant': 400, 'weapon_manager_variant': 150, 'weapon_manager_start_channel': 300,
          'weapon_manager_set_state': 800, 'weapon_set_state': 400, 'weapon_new': 300, 'weapon_owner': 60,
          'weapon_manager_set_state_first_person': 600, 'weapon_manager_set_state_first_person_tu15': 400}


# Diagnostic runs (mutation screens) may shrink the case counts.
SCALE = float(os.environ.get('HALO2_WEAPON_CASE_SCALE', '1'))


def f32(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


FLOATS = [0.0, 0.5, 1.0, 2.5, 7.0, 29.5, 30.0, 100.0, -1.0, -0.0, 1e-40, -1e-40, 0.0333, float('nan'), 3.4e38]
WEIGHTS = [0.0, 0.1, 0.25, 0.5, 0.75, 0.999, 1.0, 1.5, 1e-40, float('nan')]


class Fixture:
    # Subclasses with their own __init__ (unitactions' GraphFixture) keep
    # the TU1.5 pools.
    pools = TU15_POOLS

    def __init__(self, v, world, rng, pools=TU15_POOLS):
        self.v, self.world, self.rng, self.pools = v, world, rng, pools
        world.track = bytearray(v.TRACK_SIZE)
        world.track[:0x200] = bytes(rng.getrandbits(8) for _ in range(0x200))
        self.cursor = v.TRACK + 0x200
        self.out = v.TRACK
        self.blocks = []  # (block, row size, keys present)
        self.l3_rows, self.queries = [], []
        # Dense fixtures put every pool key in every mode table, so the
        # fallback combinations also hit in child graphs.
        self.dense = rng.random() < .35
        policy = bytearray(0x24)
        policy[0], policy[1] = rng.choice([0, 1, 1, 1]), rng.choice([0, 1, 1, 1])
        for n, (key, chained) in enumerate(pools.selectors):
            struct.pack_into('<II', policy, 4 + 8 * n, key, rng.choice([chained, chained, chained, NONE]))
        world.extra[0x47c1b4] = policy
        world.extra[0x51df20] = bytearray([rng.choice([0, 1, 1]), rng.choice([0, 1, 1]), 0, 0])
        children = [self.graph(GRAPH_TAGS[1], rng.randrange(2, 6)), self.graph(GRAPH_TAGS[2], rng.randrange(1, 5))]
        self.main = self.graph(GRAPH_TAGS[0], rng.randrange(3, 8), children[:rng.choice([0, 1, 2, 2])])
        self.other = self.graph(GRAPH_TAGS[3], rng.randrange(1, 4))
        self.graphs = [self.main, self.other] + children
        # A mixed-sign {count, rows} table: 1E5810 orders and compares keys signed.
        keys = rng.sample([0x80000000, 0x80000001, 0xc0000000, 0xfffffffb, NONE, 0, 3, 0x400000c, 0x7fffffff],
                          rng.randrange(1, 8))
        self.mixed = self.alloc(8)
        self.table(self.mixed, keys, 12, lambda row, key: self.p32(row + 4, rng.getrandbits(32)))
        self.mixed_keys = keys

    # Bank access by address (any declared bank).
    def bank(self, address, size):
        world = self.world
        regions = [(self.v.DATA, world.data), (self.v.DATA2, world.data2), (self.v.DATA3, world.data3),
                   (self.v.AI, world.ai), (self.v.TRACK, world.track)] + list(world.extra.items())
        for base, bank in regions:
            if base <= address and address + size <= base + len(bank):
                return bank, address - base
        raise ValueError(f'Undeclared fixture address {address:x}')

    def p32(self, address, value):
        bank, at = self.bank(address, 4)
        struct.pack_into('<I', bank, at, value & 0xffffffff)

    def p16(self, address, value):
        bank, at = self.bank(address, 2)
        struct.pack_into('<H', bank, at, value & 0xffff)

    def p8(self, address, value):
        bank, at = self.bank(address, 1)
        bank[at] = value & 0xff

    def track_byte(self, address):
        return self.world.track[address - self.v.TRACK]

    def pf(self, address, value):
        self.p32(address, f32(value))

    def alloc(self, size):
        at = self.cursor
        self.cursor += (size + 15) & ~15
        if self.cursor > self.v.TRACK + self.v.TRACK_SIZE:
            raise ValueError('Weapon fixture overflowed the track bank')
        return at

    def finish(self):
        used = (self.cursor - self.v.TRACK + 0xff) & ~0xff
        self.world.track = self.world.track[:used]

    def handle(self, graph, missing=True):
        rng = self.rng
        roll = rng.random()
        if missing and roll < .06:
            return NONE
        if missing and roll < .1:
            return 0xffff0000 | rng.choice([0xffff, 0, 1])
        owners = [(0xffff, graph)] + list(enumerate(graph['children']))
        low, owner = rng.choice(owners)
        return (rng.randrange(owner['count']) << 16) | low

    def table(self, address, keys, size, fill):
        """A sorted {count, rows} block at `address` with one row per key."""
        rows = self.alloc(max(len(keys), 1) * size)
        count = len(keys) if self.rng.random() < .97 else self.rng.choice([0, -1])
        self.p32(address, count)
        self.p32(address + 4, rows)
        for n, key in enumerate(sorted(keys, key=lambda k: k - (1 << 32) if k & 0x80000000 else k)):
            self.p32(rows + size * n, key)
            fill(rows + size * n, key)
        self.blocks.append((address, size, list(keys)))
        return rows

    def graph(self, tag_index, count, children=()):
        rng = self.rng
        base = self.alloc(0xc0)
        tag = (rng.choice([0xe000, 0x1234, 0x0, 0x7fff]) << 16) | tag_index
        self.p32(self.v.TAG_TABLE + 16 * tag_index + 8, base)
        graph = dict(base=base, tag=tag, count=count, children=list(children))
        resources = self.alloc(8 * 20)
        self.p32(base + 0xb0, resources)
        animations = self.alloc(count * 0x6c)
        self.p32(base + 0x30, animations)
        for n in range(count):
            row = animations + 0x6c * n
            self.p16(row + 0x14, rng.choice([0, 1, 2, 5, 10, 30, 31, 60, -1, -3, 0x7fff]))
            self.p8(row + 0x16, rng.choice([0, 0, 0x10, 0x30, 0xef]))
            self.p8(row + 0x18, rng.choice([0, 1, 2, 3]))
            self.pf(row + 0x1c, rng.choice(WEIGHTS))
            self.p32(row + 0x24, rng.randrange(8))
            self.p16(row + 0x30, rng.randrange(count) if rng.random() < .3 else 0xffff)
            self.p16(row + 0x32, rng.randrange(n + 1, count) if n + 1 < count and rng.random() < .5 else 0xffff)
            events = rng.choice([0, 0, 1, 2, 3, 4, 5, 6])
            table = self.alloc(max(events, 1) * 4)
            self.p32(row + 0x4c, events)
            self.p32(row + 0x50, table)
            for e in range(events):
                self.p8(table + 4 * e, rng.choice([0, 1, 3, 7, 15, 16, 20, 31, 40]))
                self.p16(table + 4 * e + 2, rng.choice([0, 1, 2, 5, 10, 29, 30, 31, -1, 60]))
        # Children, preceded by a row whose tag is NONE (index -1).
        rows = self.alloc((len(graph['children']) + 2) * 0x20) + 0x20
        self.p32(rows - 0x20 + 4, NONE)
        self.p32(base + 0x4c, len(graph['children']))
        self.p32(base + 0x50, rows)
        for n, child in enumerate(graph['children']):
            self.p32(rows + 0x20 * n + 4, child['tag'])
        # Key remaps (+54).
        pools = self.pools
        remaps = rng.sample(pools.l3 + [0x7000101], rng.randrange(0, 4))
        self.table(base + 0x54, remaps, 8,
                   lambda row, key: self.p32(row + 4, rng.choice(pools.l2 + [NONE, D9, 0x6600001])))
        # Mode tables (+34): three levels, then state-key handles.
        def leaf(row, key):
            entries = rng.sample(pools.states, rng.randrange(0, 5))
            self.table(row + 4, entries, 8, lambda entry, k: self.p32(entry + 4, self.handle(graph)))
            for offset in (0x24, 0x2c):
                indices = [rng.randrange(8) for _ in range(rng.choice([0, 0, 1, 2]))]
                at = self.alloc(max(len(indices), 1) * 4)
                self.p32(row + offset, len(indices))
                self.p32(row + offset + 4, at)
                for n, index in enumerate(indices):
                    self.p32(at + 4 * n, index)
            self.query_rows(row)
            self.l3_rows.append(row)
        def keys(pool):
            return list(pool) if self.dense else rng.sample(pool, rng.randrange(0, min(4, len(pool)) + 1))
        def middle(row, key):
            self.table(row + 4, keys(pools.l3 + [0x1100007]), 0x34, leaf)
        def top(row, key):
            self.table(row + 4, keys(pools.l2), 0x14, middle)
        self.table(base + 0x34, keys(pools.l1), 0x14, top)
        return graph

    def query_rows(self, row):
        """Physical-query rows (+1C/+20) of a mode leaf; +E bytes are never 0xFF."""
        rng = self.rng
        count = rng.choice([0, 0, 1, 2])
        rows = self.alloc(max(count, 1) * 0x14)
        self.p32(row + 0x1c, count)
        self.p32(row + 0x20, rows)
        for n in range(count):
            entry = rows + 0x14 * n
            self.p32(entry + 4, rng.choice(self.pools.states + [0x1234]))
            self.p8(entry + 0xa, rng.choice([0xff, 0xff, 0]))
            self.p8(entry + 0xb, rng.choice([0xff, 0xff, 1]) if rng.random() < .5 else self.track_byte(entry + 0xa))
            subcount = rng.choice([0, 1, 2])
            subrows = self.alloc(max(subcount, 1) * 0x14)
            self.p32(entry + 0xc, subcount)
            self.p32(entry + 0x10, subrows)
            for k in range(subcount):
                sub = subrows + 0x14 * k
                self.p32(sub + 4, rng.choice(self.pools.l1 + [0x6000086]))
                self.p32(sub + 8, rng.choice(self.pools.states))
                self.p8(sub + 0xc, rng.choice([0, 1, 2, 3, 4]))
                self.p8(sub + 0xe, rng.choice([0, 1, 0xfe]))
                self.p8(sub + 0xf, rng.choice([0xff, 0xff, 0]) if rng.random() < .5 else self.track_byte(sub + 0xe))
                self.p32(sub + 0x10, rng.choice([NONE, 0x30001, 0xffff0002, rng.getrandbits(32)]))
                self.queries.append((row, entry, sub))

    def channel(self, at, graph, valid=True):
        rng = self.rng
        tag = graph['tag'] if valid or rng.random() < .5 else rng.choice([NONE, 0x12345678])
        handle = self.handle(graph) if valid else rng.choice([NONE, self.handle(graph), 0xffff0003])
        if valid and rng.random() < .1:
            tag, handle = NONE, rng.choice([NONE, 0xffff0000])  # a cleared channel
        self.p32(at, tag)
        self.p32(at + 4, handle)
        self.p32(at + 8, rng.choice(self.pools.states + [NONE]))
        for offset in (0xc, 0xd, 0x10, 0x11):
            self.p8(at + offset, rng.choice([0, 1, 2, 8, 9, 0xff, rng.getrandbits(8)]))
        self.p16(at + 0xe, rng.getrandbits(16))
        self.p16(at + 0x12, rng.choice([0, 1, 0x3f, 0x8001, rng.getrandbits(16)]))
        self.p16(at + 0x14, rng.getrandbits(16))
        self.p16(at + 0x16, rng.getrandbits(16))
        self.pf(at + 0x18, rng.choice([1.0, 1.0, 0.5, 2.0, 0.0, -1.0, 1e-40, float('nan')]))
        self.pf(at + 0x1c, rng.choice(FLOATS + [rng.uniform(0, 40)]))

    def manager(self, at=None, graph=None, attached=True):
        rng = self.rng
        at = at or self.alloc(0x90)
        graph = graph or self.main
        self.channel(at, graph)
        for channel in (0x20, 0x40):
            if rng.random() < .5:
                self.channel(at + channel, graph, valid=False)
            else:
                self.p32(at + channel, NONE)
                self.p32(at + channel + 4, NONE)
                for offset in range(8, 0x20, 4):
                    self.p32(at + channel + offset, rng.getrandbits(32))
        for offset in range(0x60, 0x68, 4):
            self.p32(at + offset, rng.getrandbits(32))
        self.p32(at + 0x68, graph['tag'] if attached else NONE)
        self.p8(at + 0x6c, rng.choice([0, 1, 2, 3, 0x10, 0xff]))
        self.p8(at + 0x6d, rng.getrandbits(8))
        self.p8(at + 0x6e, rng.choice([0, 1, 0xff]))
        self.p8(at + 0x6f, rng.getrandbits(8))
        pools = self.pools
        self.p32(at + 0x70, rng.choice(pools.l1 + [NONE, 0x7000001, 0x1abcdef]))
        self.p32(at + 0x74, rng.choice(pools.l2 + [NONE, 0x7000001]))
        self.p32(at + 0x78, rng.choice(pools.l3 + [NONE, 0x7000001]))
        self.p32(at + 0x7c, rng.choice(pools.states + [NONE, 0x7000001]))
        self.pf(at + 0x80, rng.choice([0.0, 0.3, 0.6, 0.9999, 1.0, -0.5, 1e-40, float('nan')]))
        return at

    def weapon(self):
        """A weapon object row with definition tag 24 and a manager at +300."""
        rng, v = self.rng, self.v
        n = rng.randrange(v.ROWS - 1)
        body = self.world.body(n)
        definition = self.alloc(0x2e0)
        self.p32(v.TAG_TABLE + 16 * DEFINITION_TAG + 8, definition)
        self.p32(body, (rng.choice([0xe024, 0x1111]) << 16) | DEFINITION_TAG)
        magazines = rng.choice([0, 1, 1, 2, 3, -1])
        rows = self.alloc(max(magazines, 1) * 0x5c)
        self.p32(definition + 0x2c0, magazines)
        self.p32(definition + 0x2c4, rows)
        for k in range(max(magazines, 0)):
            for offset in (6, 0xa, 0xc):
                self.p16(rows + 0x5c * k + offset, rng.choice([0, 1, 4, 12, 36, 60, 200, -1, -5, 0x7fff, 0x8000]))
        self.p32(definition + 0x2c8, rng.choice([0, 1, 2, 3, -2]))
        self.p32(definition + 0x2d0, rng.choice([0, 1, 2, -1]))
        attached = rng.random() < .9
        self.p16(body + 0x12a, MANAGER_OFFSET if attached else 0xffff)
        self.manager(body + MANAGER_OFFSET)
        self.p32(body + 0x178, rng.choice([0, 0, 1, 2, 3, -1]))
        owned = rng.random() < .2
        self.p8(body + 0x12c, rng.choice([1, 3]) if owned else rng.choice([0, 2, 0xfe]))
        self.p32(body + 0x154, NONE if owned or rng.random() < .5 else self.world.handles[0])
        return self.world.handles[n]


def flags(rng):
    if rng.random() < .3:
        return rng.choice([0x96, 0x82])
    value = 0
    for bit in (2, 4, 8, 0x10, 0x20, 0x40, 0x100, 0x200, 0x400, 0x80):
        if rng.random() < (.15 if bit in (8, 0x100, 0x200) else .5):
            value |= bit
    return value


def start_flags(rng):
    return rng.choice([0x3f, 0x3f, 0x14, 0x4, 0x8, 0x10, 0, 0xc000 | 0x14, rng.getrandbits(16)])


def register(v):
    v.UNREACHED_VIRTUAL_OWNERS |= UNREACHED
    v.JUMP_TABLES.update(TABLES)
    for name, (entry, regs, stack, kind) in ROUTINES.items():
        v.KERNELS[name] = (['weapon_case'], kind)
        v.ROUTINES[name] = dict(entry=entry, regs=regs, stack=stack)
        if name in OWNER_STOPS:
            v.ROUTINES[name]['closure_stops'] = OWNER_STOPS[name]
        v.TRACK_COUNTS[name] = max(1, int(COUNTS[name] * SCALE))

    def case(name, world, rng):
        if name in FP_KERNELS:
            pools, names = first_person_pools(rng, FP_KERNELS[name])
            fx = Fixture(v, world, rng, pools)
            values = first_person_arguments(fx, names, rng)
        else:
            fx = Fixture(v, world, rng)
            values = arguments(fx, name, world, rng)
        fx.finish()
        return values

    def first_person_arguments(fx, names, rng):
        # key3, manager, a0, a1, a2, flags, start flags.
        roll = rng.random()
        name = rng.choice(names)
        key3 = (name[0] if roll < .7 else name[1] if roll < .8 else 0x400000c if roll < .9 else
                rng.choice(STATE_KEYS + [t for t, _ in QUERY_NAMES['states']]))
        state = 0x82 if rng.random() < .6 else flags(rng)
        return [key3, fx.manager(), rng.choice([0x7000101] * 3 + [0x6000086, D9]),
                rng.choice([DUAL, DUAL, D9, 0x7000101, 0x7000083]), rng.choice([0x7000001] * 3 + [D9, 0x7000101]),
                state, rng.choice([0x3f, 0x103f, 0x203f, start_flags(rng)])]

    def key(rng, pool):
        return rng.choice(pool + [NONE, D9, rng.getrandbits(32)] if rng.random() < .2 else pool)

    def arguments(fx, name, world, rng):
        graph = rng.choice(fx.graphs) if rng.random() < .3 else fx.main
        out = fx.out
        if name == 'weapon_tag_block_search':
            block, size, keys = rng.choice(fx.blocks)
            if rng.random() < .4:
                block, size, keys = fx.mixed, 12, fx.mixed_keys
            return [block, rng.choice(keys + [0, NONE, D9, rng.getrandbits(32)]) if keys else rng.getrandbits(32), size]
        if name == 'weapon_graph_child':
            count = len(graph['children'])
            return [graph['base'], rng.choice(list(range(count + 2)) + [NONE, 0x80000000, 0x7fffffff])]
        if name in ('weapon_graph_animation', 'weapon_graph_require_resident'):
            return [graph['base'], fx.handle(graph)]
        if name == 'weapon_channels_reset':
            return [fx.manager()]
        if name == 'weapon_resolve_keys':
            manager = fx.manager()
            pick = lambda pool: rng.choice(pool + [0x7000101, 0x7000101, 0x7000001, NONE])
            return [pick(L1_KEYS), pick(L2_KEYS), pick(L3_KEYS), out + 0x40, manager, pick(STATE_KEYS)]
        if name == 'weapon_mode_iterate':
            iterator = out + 0x80
            values = [graph['base'], key(rng, L1_KEYS), key(rng, L2_KEYS), key(rng, L3_KEYS),
                      rng.choice([0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]), 0, 0]
            for n, value in enumerate(values):
                fx.p32(iterator + 4 * n, value)
            rows = [b for b in fx.blocks if b[1] == 0x14 and b[0] != graph['base'] + 0x34]
            if rows and rng.random() < .3:
                # Resume from a level-1 row (and maybe a level-2 row).
                block = rng.choice([b for b in fx.blocks if b[1] == 0x34] or [None])
                if block:
                    fx.p32(iterator + 0x18, block[0] - 4)
                    fx.p32(iterator + 0x14, rng.choice([b[0] - 4 for b in fx.blocks if b[1] == 0x14 and
                                                        b[0] != graph['base'] + 0x34] or [0]))
            return [iterator, out + 0xa8, out + 0xa0, out + 0xa4]
        if name == 'weapon_mode_entry':
            return [key(rng, L1_KEYS), graph['base'], key(rng, L2_KEYS), key(rng, L3_KEYS)]
        load = lambda: rng.choice([0, 1, 1, 0x100, 0xff])
        if name == 'weapon_mode_prefetch':
            return [load(), graph['base'], key(rng, L1_KEYS), key(rng, L2_KEYS), key(rng, L3_KEYS), load()]
        if name == 'weapon_mode_prefetch_fallbacks':
            return [load(), key(rng, L1_KEYS), graph['base'], key(rng, L2_KEYS), key(rng, L3_KEYS), load()]
        if name == 'weapon_graph_prefetch':
            return [key(rng, L1_KEYS), graph['base'], key(rng, L2_KEYS), key(rng, L3_KEYS), load(), load()]
        if name == 'weapon_manager_prefetch':
            manager = fx.manager(attached=rng.random() < .9)
            pick = lambda pool: rng.choice(pool + [0x7000101, 0x7000001])
            return [manager, pick(L2_KEYS), pick(L3_KEYS), pick(L1_KEYS), load(), load()]
        if name == 'weapon_animation_lookup':
            outs = [rng.choice([out + 0x40 + 4 * n, out + 0x40 + 4 * n, 0]) for n in range(2)]
            return [key(rng, STATE_KEYS), fx.main['base'], out + 0x60, key(rng, L1_KEYS), key(rng, L2_KEYS),
                    key(rng, L3_KEYS)] + outs
        if name == 'weapon_choose_animation':
            manager = fx.manager()
            pick = lambda pool: rng.choice(pool + [0x7000101, 0x7000101, 0x7000001])
            return [manager, out + 0x40, out + 0x50, pick(L1_KEYS), pick(L2_KEYS + L3_KEYS), pick(STATE_KEYS),
                    rng.choice([0, 2, 4, 6, 6, rng.getrandbits(8)]), out + 0x60]
        if name == 'weapon_physical_query_find':
            if fx.queries and rng.random() < .7:
                row, entry, sub = rng.choice(fx.queries)
                word = lambda a: struct.unpack_from('<I', world.track, a - v.TRACK)[0]
                byte = lambda a: world.track[a - v.TRACK]
                # The +A/+B and +E/+F byte pairs share one argument word each.
                exact = rng.random() < .4
                if exact:
                    # An exact sub-row, then at most the +C byte differs.
                    fx.p8(entry + 0xb, byte(entry + 0xa))
                    fx.p8(sub + 0xf, byte(sub + 0xe))
                values = [row, out + 0x60, word(entry + 4), byte(entry + rng.choice([0xa, 0xb])), word(sub + 4),
                          word(sub + 8), byte(sub + rng.choice([0xe, 0xf])), byte(sub + 0xc)]
                if exact:
                    if rng.random() < .5:
                        values[7] = rng.choice([NONE, values[7] ^ 1, values[7] + 1])
                elif rng.random() < .4:
                    k = rng.randrange(2, 8)
                    values[k] = rng.choice([NONE, 0, values[k] ^ 1])
                return values
            row = rng.choice(fx.l3_rows) if fx.l3_rows else fx.alloc(0x40)
            return [row, out + 0x60, rng.choice(STATE_KEYS), 0xff, rng.choice(L1_KEYS), rng.choice(STATE_KEYS),
                    rng.getrandbits(8), rng.randrange(5)]
        if name == 'weapon_physical_query_select':
            # Query bytes share one word whose low byte 0xFF never matches +E.
            return [NONE if rng.random() < .15 else key(rng, L2_KEYS), fx.main['base'], out + 0x60, key(rng, L1_KEYS),
                    key(rng, L3_KEYS), rng.choice(STATE_KEYS), rng.choice([0xff, 0x1ff, NONE, 0x123456ff]),
                    rng.randrange(5)]
        if name == 'weapon_physical_query_state':
            return [fx.manager(), out + 0x60, rng.choice(L1_KEYS + [0x6000086]), rng.choice(STATE_KEYS), out + 0x64]
        if name == 'weapon_physical_query_selector':
            manager = fx.manager()
            selector = rng.choice([s[0] for s in SELECTORS] + [rng.choice(STATE_KEYS)])
            return [selector, out + 0x60, manager, key(rng, L1_KEYS), key(rng, L2_KEYS), key(rng, L3_KEYS), out + 0x64]
        if name in ('weapon_channel_stopped', 'weapon_channel_valid'):
            record = fx.alloc(0x20)
            fx.channel(record, graph, valid=rng.random() < .5)
            return [record]
        if name in ('weapon_channel_animation', 'weapon_channel_finished', 'weapon_channel_mark_events'):
            record = fx.alloc(0x20)
            fx.channel(record, graph)
            return [record]
        if name == 'weapon_channel_set_frame':
            record = fx.alloc(0x20)
            fx.channel(record, graph)
            frame = f32(rng.choice(FLOATS + [rng.uniform(-5, 70)]))
            handle = struct.unpack_from('<I', world.track, record + 4 - v.TRACK)[0]
            if handle >> 16 == 0xffff:
                # No animation: only the negative-frame, not-playing path is defined.
                frame = f32(rng.choice([-1.0, -0.5, -100.0]))
                fx.p8(record + 0x12, 0)
            elif struct.unpack_from('<I', world.track, record - v.TRACK)[0] == graph['tag']:
                # Within a tick of the end, the 30 Hz look-ahead decides +11 bit 1.
                # A local generator keeps every other case unchanged.
                local = random.Random(record ^ frame)
                if local.random() < .4:
                    owner = graph if handle & 0xffff == 0xffff else graph['children'][handle & 0xffff]
                    rows = struct.unpack_from('<I', world.track, owner['base'] + 0x30 - v.TRACK)[0]
                    count = struct.unpack_from('<h', world.track, rows + 0x6c * (handle >> 16) + 0x14 - v.TRACK)[0]
                    frame = f32(count - local.choice([0.1, 0.5, 0.9, 0.97, 1.2]))
                    fx.p8(record + 0x12, 1)
                    fx.p8(record + 0x11, local.choice([0, 2]))
                    fx.pf(record + 0x18, local.choice([1.0, 1.0, 2.0, 0.5]))
                    fx.pf(v.TIME + 4, local.choice([1 / 30, 1 / 60]))
            return [record, frame]
        if name == 'weapon_channel_start':
            return [fx.alloc(0x20), rng.choice([graph['tag'], graph['tag'], NONE]), start_flags(rng),
                    fx.handle(graph), rng.choice(STATE_KEYS + [NONE]), rng.getrandbits(8), rng.getrandbits(8),
                    rng.choice([0, 0x7f, 0x80, 0xff])]
        if name == 'weapon_variant_alias':
            return [graph['base'], out + 0x60, fx.handle(graph)]
        if name == 'weapon_random_variant':
            flag = rng.choice([0, 1, 0x100, 0xff])
            if rng.random() < .4:
                main = fx.main
                n = rng.randrange(main['count'] - 1)
                row = struct.unpack_from('<I', world.track, main['base'] + 0x30 - v.TRACK)[0] + 0x6c * n
                seed = struct.unpack_from('<I', world.data2, v.SEED + (0 if flag & 0xff else 4) - v.DATA2)[0]
                draw = (((seed * 0x19660d + 0x3c6ef35f) & 0xffffffff) >> 16) * struct.unpack('<f', struct.pack('<I', 0x37800080))[0]
                fx.p16(row + 0x30, 0xffff)
                fx.p16(row + 0x32, n + 1)
                fx.pf(row + 0x1c, draw)
                return [main['base'], out + 0x60, (n << 16) | 0xffff, flag]
            return [graph['base'], out + 0x60, fx.handle(graph), flag]
        if name == 'weapon_manager_variant':
            return [fx.manager(), out + 0x60, fx.handle(fx.main)]
        if name == 'weapon_manager_start_channel':
            manager = fx.manager(attached=rng.random() < .9)
            record = rng.choice([manager, manager + 0x20, fx.alloc(0x20)])
            return [manager, start_flags(rng), record, fx.handle(fx.main), rng.choice(STATE_KEYS + [NONE]),
                    rng.getrandbits(8), rng.getrandbits(8), rng.choice([0, 0x80, 0xff])]
        if name == 'weapon_manager_set_state':
            manager = fx.manager()
            pick = lambda pool: rng.choice(pool + [0x7000101, 0x7000101, 0x7000001])
            state = flags(rng)
            if rng.random() < .3:
                # Keep channel 0 (no reset) and time the blend from its frame.
                state = (state & ~0x18) | rng.choice([0x20, 0x40, 0x60])
                fx.pf(manager + 0x1c, rng.choice([50.0, 1000.0, 3e4, 0.75, 4.0, -1.0, -0.5]))
            return [pick(STATE_KEYS), manager, pick(L1_KEYS), pick(L2_KEYS), pick(L3_KEYS), state, start_flags(rng)]
        if name == 'weapon_set_state':
            return [fx.weapon(), rng.choice([0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10]), rng.choice([0, 1, 1, 0x100])]
        if name == 'weapon_new':
            return [fx.weapon(), rng.getrandbits(32), rng.getrandbits(32)]
        if name == 'weapon_owner':
            return [fx.weapon()]
        raise ValueError(name)

    v.TRACK_ARGUMENTS['weapon_case'] = case

    # The first-person kernels return to 169863's call site (their query
    # scope's caller); the MCC one carries the translated route's joins.
    original_cases = v.cases

    def cases(seed, evaluators, tails):
        rows = original_cases(seed, evaluators, tails)
        for row in rows:
            if row['kernel'] in FP_KERNELS:
                row['return_pc'] = row['stop_pc'] = FIRST_PERSON_CALLER
                if FP_KERNELS[row['kernel']]:
                    row['query_names'] = QUERY_NAMES
        return rows
    v.cases = cases
