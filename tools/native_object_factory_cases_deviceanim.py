"""Oracle cases for factory track "deviceanim": the device animation channel
owners (10D5E0 overlay track, 10D870/10D790 channels, 1D3770, 1E5360,
1E3F30).

The animation fixture is track "weapon"'s (graphs with mode tables, managers,
channels; resources never resident, physical queries never matching). This
module adds animation names (row +0) and the mode leaves' +C overlay tables,
and device bodies (rows 6, 7, 8 and 10 are machines, controls and light
fixtures) with a manager at body +300.
"""
import os
import random
import struct

import native_object_factory_cases_weapon as weapon

NONE = weapon.NONE
NAMES = [0x5000081, 0x8000080, 0x1110001, 0x2220002, 0x3330003, 0x4440004]
DEVICE_ROWS = [6, 7, 8, 10]
NODE_NAME = 0xd000533
FLOATS = [0.0, 0.5, 1.0, -1.0, 2.0, 1e-40, -0.0, float('nan'), 1e-6, 3.0e-5]
SCALE = float(os.environ.get('HALO2_DEVICEANIM_CASE_SCALE', '1'))

ROUTINES = {
    'deviceanim_find_named_animation': (0x1e5360, {'edx': 0}, [1, 2], 'long'),
    # Words: key3, graph, result, r0, r1, r2, p, q (out-pointers p, q, p).
    'deviceanim_overlay_lookup': (0x1e3f30, {'eax': 0}, [1, 2, 3, 4, 5, 6, 7, 6], 'long'),
    'deviceanim_start_named_animation': (0x1d3770, {'esi': 0, 'edi': 3}, [1, 2], 'boolean'),
    'deviceanim_overlay_channel': (0x10d870, {'eax': 0}, [1], 'boolean'),
    'deviceanim_position_channel': (0x10d790, {'eax': 0}, [1], 'boolean'),
    'deviceanim_set_overlay_track': (0x10d5e0, {'edi': 0}, [1], 'boolean'),
    'deviceanim_normalize3': (0x30bd0, {'eax': 0}, [], 'xmm0'),
    'deviceanim_basis_matrix': (0x144bf0, {'edx': 0, 'esi': 1, 'ecx': 2, 'eax': 3}, [], 'void'),
    'deviceanim_matrix_inverse': (0x144090, {'eax': 0, 'ecx': 1}, [], 'void'),
    'deviceanim_rotation_quaternion': (0x144a60, {'ebx': 0, 'ecx': 1}, [], 'void'),
    'deviceanim_matrix_orientation': (0x144e60, {'eax': 0, 'esi': 1}, [], 'void'),
    'deviceanim_quaternion_product': (0x1201e0, {'edx': 0, 'ecx': 1, 'eax': 2}, [], 'void'),
    'deviceanim_orientation_compose': (0x120570, {'edx': 0}, [1, 2], 'void'),
    'deviceanim_node_index': (0x1d4090, {'eax': 0, 'esi': 1}, [], 'long'),
    'deviceanim_codec_entry': (0x1d4100, {}, [0, 1], 'long'),
    'deviceanim_static_pose': (0x1e5ba0, {'ebx': 0}, [1, 2], 'boolean'),
    'deviceanim_channel_seconds': (0x1cf150, {'eax': 0}, [], 'xmm0'),
    'deviceanim_has_node_orientations': (0xc1600, {'eax': 0}, [], 'boolean'),
}
# Composite owners (own runner below): the original stops before a boundary
# entry, where the native must refuse after the identical store prefix.
COMPOSITE = {
    'deviceanim_sample_orientation': (0x1d4170, {'esi': 0}, [1, 2, 3], 'void', 400),
    'deviceanim_sample_root': (0x1d4200, {'ebx': 0}, [1, 2, 3, 4], 'void', 400),
    'deviceanim_sample_root_velocity': (0x1d4280, {'ebx': 0}, [1, 2, 3, 4, 5], 'void', 300),
    'deviceanim_set_position_track': (0x10cf80, {}, [0, 1, 2], 'boolean', 1200),
}
STOPS = {0x283a90: '283A90', 0xbf050: 'BF050', 0xb95d0: 'B95D0', 0x126200: '126200'}
COUNTS = {'deviceanim_find_named_animation': 300, 'deviceanim_overlay_lookup': 300,
          'deviceanim_start_named_animation': 300, 'deviceanim_overlay_channel': 300,
          'deviceanim_position_channel': 300, 'deviceanim_set_overlay_track': 400,
          'deviceanim_normalize3': 300, 'deviceanim_basis_matrix': 200, 'deviceanim_matrix_inverse': 300,
          'deviceanim_rotation_quaternion': 400, 'deviceanim_matrix_orientation': 200,
          'deviceanim_quaternion_product': 200, 'deviceanim_orientation_compose': 300, 'deviceanim_node_index': 150,
          'deviceanim_codec_entry': 200, 'deviceanim_static_pose': 300, 'deviceanim_channel_seconds': 150,
          'deviceanim_has_node_orientations': 60}


# Track-bank room kept for the allocations that follow the mode tables (the
# remaining graphs' headers, animations, events and children, base poses,
# node names, overlay tables and case arguments).
RESERVE = 0x2800
# Worst-case bytes below one row of a mode table: level 1 (graph +34) rows
# hold up to five level-2 rows, level-2 rows up to four leaves, and a leaf
# (0x34 bytes) up to four state keys, two resource lists and two query rows.
LEAF_WORST = 0x20 + 0x10 + 0x10 + 0x30 + 2 * 0x30
LEVEL2_WORST = 0xd0 + 4 * LEAF_WORST
LEVEL1_WORST = 0x70 + 5 * LEVEL2_WORST


class Fixture(weapon.Fixture):
    # Dense mode tables only in the child graphs (dense tables everywhere,
    # with this track's poses and overlay tables, overflow the 64 KiB track
    # bank); the children carry the fallback prefetch rows. The weapon
    # fixture's draw for the choice is kept.
    @property
    def dense(self):
        return self._dense and getattr(self, '_child', False)

    @dense.setter
    def dense(self, value):
        self._dense = value

    def __init__(self, v, world, rng):
        super().__init__(v, world, rng)
        for graph in self.graphs:
            rows = struct.unpack_from('<I', world.track, graph['base'] + 0x30 - v.TRACK)[0]
            for n in range(graph['count']):
                self.p32(rows + 0x6c * n, rng.choice(NAMES + [rng.getrandbits(32)]))
        for graph in self.graphs:
            self.graph_extras(graph)
        # [0x474D38]: the identity orientation that root samples start from.
        identity = self.alloc(0x20)
        world.extra[0x474d38] = bytearray(4)
        self.p32(0x474d38, identity)
        for k, value in enumerate([0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0] if rng.random() < .8 else
                                  [rng.uniform(-1, 1) for _ in range(8)]):
            self.pf(identity + 4 * k, value)
        # Overlay tables (+C/+10) of the main graph's mode leaves.
        for row in self.main['leaves']:
            keys = rng.sample(NAMES + weapon.STATE_KEYS[:4], rng.randrange(0, 5))
            self.table(row + 0xc, keys, 8, lambda entry, key: self.p32(entry + 4, self.handle(self.main)))

    def graph_extras(self, graph):
        """Animation count (+2C), base poses (+B8 rows of 24 bytes; animation
        +2C indexes them) and node names (+C/+10 rows of 0x20)."""
        rng, v = self.rng, self.v
        base = graph['base']
        self.p32(base + 0x2c, graph['count'] if rng.random() < .9 else rng.randrange(graph['count'] + 1))
        poses = rng.randrange(1, 4)
        table = self.alloc(poses * 24)
        self.p32(base + 0xb8, table)
        for n in range(poses):
            row = table + 24 * n
            for k in range(4):
                self.p16(row + 2 * k, rng.choice([0, 0, 1, -1, 0x7fff, -0x8000, 0x4000, rng.randrange(-0x8000, 0x8000)]))
            for k in range(3):
                self.pf(row + 8 + 4 * k, rng.choice([0.0, 1.0, -2.5, rng.uniform(-20, 20)]))
            self.pf(row + 0x14, rng.choice([1.0, 1.0, 0.5, 2.0]))
        rows = struct.unpack_from('<I', self.world.track, base + 0x30 - v.TRACK)[0]
        for n in range(graph['count']):
            self.p16(rows + 0x6c * n + 0x2c, rng.randrange(poses) if rng.random() < .5 else 0xffff)
        # Child rows' codec data (+8, +C, +10, +14: 1E32A0 returns the row
        # when all are set). A local generator keeps every other case unchanged.
        local = random.Random(base)
        children = struct.unpack_from('<I', self.world.track, base + 0x50 - v.TRACK)[0]
        for n in range(len(graph['children'])):
            for offset in (8, 0xc, 0x10, 0x14):
                self.p32(children + 0x20 * n + offset, local.choice([1, 1, 2, 0x40, 0, -1]))
        nodes = rng.choice([0, 1, 2, 3])
        table = self.alloc(max(nodes, 1) * 0x20)
        self.p32(base + 0xc, nodes)
        self.p32(base + 0x10, table)
        for n in range(nodes):
            self.p32(table + 0x20 * n, rng.choice([NODE_NAME, 0x1000001, 0x2000002]))

    def alloc(self, size):
        at = super().alloc(size)
        if getattr(self, '_graph_pending', False):
            self._graph_base, self._graph_pending = at, False
        return at

    def table(self, address, keys, size, fill):
        """The weapon fixture's table, shortened (deterministically, without
        drawing) so the worst case of its subtree always fits the track bank."""
        if size == 0x14:
            worst = LEVEL1_WORST if address == getattr(self, '_graph_base', None) + 0x34 else LEVEL2_WORST
        else:
            worst = LEAF_WORST if size == 0x34 else 0
        room = self.v.TRACK + self.v.TRACK_SIZE - self.cursor - RESERVE
        keys = list(keys)
        while keys and ((max(len(keys), 1) * size + 15) & ~15) + len(keys) * worst > room:
            keys.pop()
        return super().table(address, keys, size, fill)

    def graph(self, tag_index, count, children=()):
        start = len(self.l3_rows)
        self._child = tag_index in weapon.GRAPH_TAGS[1:3]
        self._graph_pending = True
        graph = super().graph(tag_index, count, children)
        self._child = False
        graph['leaves'] = self.l3_rows[start:]
        return graph

    def device(self, rows=DEVICE_ROWS):
        """A device object (or another row) with a manager at body +300."""
        rng, world = self.rng, self.world
        n = rng.choice(rows)
        body = world.body(n)
        self.p16(body + 0x12a, weapon.MANAGER_OFFSET if rng.random() < .9 else 0xffff)
        flags = rng.getrandbits(32)
        # A device on a position track (+12C bit 3) has an animation graph.
        self.manager(body + weapon.MANAGER_OFFSET, attached=flags & 8 or rng.random() < .85)
        for channel in (0x18c, 0x1ac):
            for offset in range(0, 0x20, 4):
                self.p32(body + channel + offset, rng.getrandbits(32))
        self.p32(body + 0x12c, flags)
        # 10CF80/BC380 layout: orientation blocks +1D0 (current) and +210
        # (previous), node matrices +250 (3 nodes), manager +300.
        self.p16(body + 0x112, 0x1d0 if rng.random() < .8 else 0xffff)
        self.p16(body + 0x10e, 0x210)
        self.p16(body + 0x110, 0x20 * rng.choice([1, 2]))
        self.p16(body + 0x116, 0x250)
        for at in range(0x1d0, 0x250, 4):
            self.pf(body + at, rng.choice([0.0, 1.0, 0.5, rng.uniform(-2, 2)]))
        for node in range(3):
            self.matrix(body + 0x250 + 0x34 * node)
        self.channel(body + 0x18c, self.main)
        # 10CF80 samples the position channel's handle unchecked.
        self.p32(body + 0x18c, self.main['tag'])
        self.p32(body + 0x190, self.handle(self.main, missing=False))
        self.pf(body + 0x1a8, rng.choice([0.0, 0.0, 0.0, 1e-6, 1.0, 15.0, float('nan')]))
        if rng.random() < .6:
            self.p32(body + 0x14, NONE)
        for at, vector in ((0x64, self.point), (0x70, self.unit), (0x7c, self.unit)):
            vector(body + at)
        return world.handles[n]

    def point(self, at):
        for k in range(3):
            self.pf(at + 4 * k, self.rng.choice([0.0, 1.0, self.rng.uniform(-50, 50)]))

    def unit(self, at):
        rng = self.rng
        x = [rng.uniform(-1, 1) for _ in range(3)]
        n = sum(c * c for c in x) ** .5 or 1.0
        for k in range(3):
            self.pf(at + 4 * k, x[k] / n if rng.random() < .9 else rng.choice(FLOATS))

    def rotation(self, at):
        """A 3x3 rotation (from a random quaternion), or arbitrary values."""
        rng = self.rng
        if rng.random() < .75:
            q = [rng.uniform(-1, 1) for _ in range(4)]
            n = sum(c * c for c in q) ** .5 or 1.0
            x, y, z, w = (c / n for c in q)
            if rng.random() < .2:
                x, y, z, w = rng.choice([(1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1)])
            m = [1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y),
                 2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x),
                 2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)]
        else:
            m = [rng.choice(FLOATS + [rng.uniform(-2, 2)]) for _ in range(9)]
        for k in range(9):
            self.pf(at + 4 * k, m[k])

    def matrix(self, at):
        rng = self.rng
        self.pf(at, rng.choice([1.0, 1.0, 1.0, 2.0, 0.5, 0.0, 1e-40, float('nan')]))
        self.rotation(at + 4)
        self.point(at + 0x28)

    def orientation(self, at):
        rng = self.rng
        q = [rng.uniform(-1, 1) for _ in range(4)]
        n = sum(c * c for c in q) ** .5 or 1.0
        for k in range(4):
            self.pf(at + 4 * k, q[k] / n if rng.random() < .9 else rng.choice(FLOATS))
        self.point(at + 0x10)
        self.pf(at + 0x1c, rng.choice([1.0, 1.0, 2.0, 0.5, 0.0]))


def register(v):
    v.UNREACHED_VIRTUAL_OWNERS |= weapon.UNREACHED
    v.JUMP_TABLES.update(weapon.TABLES)
    for name, (entry, regs, stack, kind) in ROUTINES.items():
        v.KERNELS[name] = (['deviceanim_case'], kind)
        v.ROUTINES[name] = dict(entry=entry, regs=regs, stack=stack)
        v.TRACK_COUNTS[name] = max(1, int(COUNTS[name] * SCALE))

    def name(rng):
        return rng.choice(NAMES + weapon.STATE_KEYS[:4] + [rng.getrandbits(32)])

    def case(kernel, world, rng):
        fx = Fixture(v, world, rng)
        values = arguments(fx, kernel, world, rng)
        fx.finish()
        return values

    def arguments(fx, kernel, world, rng):
        out = fx.out
        if kernel == 'deviceanim_find_named_animation':
            graph = rng.choice(fx.graphs) if rng.random() < .9 else None
            if graph and rng.random() < .3:
                # A NONE child row ends the search.
                count = len(graph['children'])
                rows = struct.unpack_from('<I', world.track, graph['base'] + 0x50 - v.TRACK)[0]
                fx.p32(rows + 0x20 * count + 4, NONE)
                fx.p32(graph['base'] + 0x4c, count + 1)
            return [graph['base'] if graph else 0, out + 0x60, name(rng)]
        if kernel == 'deviceanim_overlay_lookup':
            key = lambda pool: rng.choice(pool + [NONE, weapon.D9] if rng.random() < .2 else pool)
            outs = [rng.choice([out + 0x40 + 4 * n, out + 0x40 + 4 * n, 0]) for n in range(2)]
            return [name(rng), fx.main['base'], out + 0x60, key(weapon.L1_KEYS), key(weapon.L2_KEYS),
                    key(weapon.L3_KEYS)] + outs
        if kernel == 'deviceanim_start_named_animation':
            manager = fx.manager(attached=rng.random() < .85)
            record = rng.choice([manager, manager + 0x20, fx.alloc(0x20)])
            return [manager, record, rng.choice([0x40, 0x40, 0x14, 0x3f, 0x8004]), name(rng)]
        if kernel in ('deviceanim_overlay_channel', 'deviceanim_position_channel'):
            device = fx.device(DEVICE_ROWS + [0, 3])
            return [name(rng), rng.choice([device, device, device, NONE])]
        if kernel == 'deviceanim_set_overlay_track':
            device = fx.device(DEVICE_ROWS + [1, 4])
            if rng.random() < .1:
                device = rng.choice([NONE, (device & 0xffff) | 0x7ffe0000, world.handles[11]])
            return [device, name(rng)]
        return helper_arguments(fx, kernel, world, rng)

    v.TRACK_ARGUMENTS['deviceanim_case'] = case


def seconds(rng):
    return weapon.f32(rng.choice([0.0, 0.0, 1e-6, -1e-6, 0.0333, 0.5, 1.0, -0.5, 1e-40, float('nan')]))


def helper_arguments(fx, kernel, world, rng):
    out = fx.alloc(0x40)
    if kernel == 'deviceanim_normalize3':
        at = fx.alloc(12)
        for k in range(3):
            fx.pf(at + 4 * k, rng.choice(FLOATS + [rng.uniform(-10, 10), 1e-5, 3e-5]))
        return [at]
    if kernel == 'deviceanim_basis_matrix':
        position, forward, up = fx.alloc(12), fx.alloc(12), fx.alloc(12)
        fx.point(position), fx.unit(forward), fx.unit(up)
        return [out, position, forward, up]
    if kernel in ('deviceanim_matrix_inverse', 'deviceanim_matrix_orientation'):
        matrix = fx.alloc(0x34)
        fx.matrix(matrix)
        return [out, matrix if rng.random() < .9 else out]
    if kernel == 'deviceanim_rotation_quaternion':
        rotation = fx.alloc(0x24)
        fx.rotation(rotation)
        return [out, rotation]
    if kernel in ('deviceanim_quaternion_product', 'deviceanim_orientation_compose'):
        a, b = fx.alloc(0x20), fx.alloc(0x20)
        fx.orientation(a), fx.orientation(b)
        fx.orientation(out)
        choice = rng.random()
        if choice < .15:
            a = out
        elif choice < .3:
            b = out
        elif choice < .35:
            a = b = out
        return [out, a, b]
    if kernel == 'deviceanim_node_index':
        manager = fx.manager(graph=rng.choice(fx.graphs))
        return [manager, rng.choice([NODE_NAME, NODE_NAME, 0x1000001, 0x5555])]
    if kernel == 'deviceanim_codec_entry':
        return [fx.manager(), fx.handle(fx.main)]
    if kernel == 'deviceanim_static_pose':
        graph = rng.choice(fx.graphs)
        return [graph['base'], fx.handle(graph, missing=False), out]
    if kernel == 'deviceanim_channel_seconds':
        record = fx.alloc(0x20)
        fx.channel(record, fx.main)
        return [record]
    if kernel == 'deviceanim_has_node_orientations':
        return [fx.device()]
    raise ValueError(kernel)


def composite_arguments(fx, kernel, world, rng):
    if kernel == 'deviceanim_sample_orientation':
        return [fx.manager(), fx.handle(fx.main, missing=False), seconds(rng), fx.alloc(0x20)]
    if kernel == 'deviceanim_sample_root':
        return [fx.alloc(0x40), fx.manager(), fx.handle(fx.main, missing=False), seconds(rng), rng.getrandbits(32)]
    if kernel == 'deviceanim_sample_root_velocity':
        rate = weapon.f32(rng.choice([1.0, 0.5, 2.0, 0.0, -1.0]))
        return [fx.alloc(0x10), fx.manager(), fx.handle(fx.main, missing=False), seconds(rng), rate,
                rng.getrandbits(32)]
    if kernel == 'deviceanim_set_position_track':
        device = fx.device(DEVICE_ROWS + [1])
        if rng.random() < .05:
            device = rng.choice([NONE, (device & 0xffff) | 0x7ffe0000])
        name = rng.choice(NAMES + weapon.STATE_KEYS[:4] + [rng.getrandbits(32)])
        return [device, name, seconds(rng)]
    raise ValueError(kernel)


def composite_main():
    """Composite owners against the physical original (stops at STOPS)."""
    import argparse
    import hashlib
    import json
    import random
    import subprocess
    import sys
    from pathlib import Path
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import validate_native_campaign_script_builtins_tu15 as v
    parser = argparse.ArgumentParser(description=composite_main.__doc__)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=0x5c41)
    parser.add_argument('--only', action='append', default=[])
    parser.add_argument('--coverage', default='', help='Comma-separated owner entries to report uncovered sites for')
    args = parser.parse_args()
    names = [n for n in COMPOSITE if not args.only or n in args.only]
    if not v.NEW_DEFAULT.is_file():
        print('SKIP missing owned TU1.5 executable')
        return 77
    if v.sha(v.NEW_DEFAULT) != v.NEW_PIN:
        raise ValueError('Wrong original executable')
    v.UNREACHED_VIRTUAL_OWNERS |= weapon.UNREACHED
    v.JUMP_TABLES.update(weapon.TABLES)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    image = v.Xbe(v.NEW_DEFAULT)

    class Boundary:
        code = {}
    whole = v.complete_body
    v.complete_body = lambda code, address: Boundary() if address in STOPS else whole(code, address)
    try:
        meta, code = v.metadata(image, {n: COMPOSITE[n][0] for n in names})
    finally:
        v.complete_body = whole
    owned = v.prepare(image, meta, scratch)
    rng = random.Random(args.seed)
    rows = []
    for name in names:
        entry, regs, stack, kind, count = COMPOSITE[name]
        for repeat in range(max(1, int(count * SCALE))):
            world = v.World(rng)
            fx = Fixture(v, world, rng)
            values = composite_arguments(fx, name, world, rng)
            fx.finish()
            x87, mxcsr = rng.choice(v.FP_CONTROLS)
            gp = [0x5a010000 + len(rows) * 31 + k * 7 for k in range(8)]
            gp[4] = v.SP
            row = dict(id=len(rows), name=f'{name}-{repeat}', kernel=name, args=values, members=[], entry=entry,
                       stop_pc=v.STOP, stop_before_entries=sorted(STOPS), return_pc=v.STOP, gp=gp, matrix=0,
                       source=0, before_fp=[x87, 0, mxcsr], fp=[x87, mxcsr], live_x87=[])
            rows.append(v.routine_row(row, dict(routine=dict(entry=entry, regs=regs, stack=stack)), world, values))
    proof = v.physical(owned, rows)
    golds, outcomes = [], {}
    for row, original in zip(rows, proof['cases'], strict=True):
        if not original['completed']:
            raise ValueError('Original did not reach a boundary: ' + row['name'])
        writes = [[w['address'], w['written']] for w in original['writes']
                  if not v.STACK <= w['address'] < v.STACK + 0x10000 and not v.ARG <= w['address'] < v.ARG + 32]
        banks = {b['address']: b['after'] for b in original['banks'] if b['address'] != v.STACK}
        stopped = original.get('stopped_at')
        boundary = None if stopped == v.STOP else STOPS.get(stopped, f'{stopped:X}')
        kind = COMPOSITE[row['kernel']][3]
        result = 0 if boundary or kind == 'void' else original['boundary_gp'][0] & 0xff
        golds.append(dict(boundary=boundary, writes=writes, banks=banks, result=result))
        key = (row['kernel'], boundary or 'returned')
        outcomes[key] = outcomes.get(key, 0) + 1
    total, stores = hashlib.sha256(), 0
    errors = scratch / 'composite-native.stderr'
    with errors.open('w+') as stderr:
        process = subprocess.Popen([str(args.native)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=stderr,
                                   text=True)
        try:
            for row, gold in zip(rows, golds):
                process.stdin.write(json.dumps(dict(kernel=row['kernel'], args=row['args'], members=[], fp=row['fp'],
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
                elif not actual['completed']:
                    problems.append('native refused: ' + actual.get('fault', ''))
                elif (actual['result'] & 0xff if COMPOSITE[row['kernel']][3] == 'boolean' else actual['result']) != gold['result']:
                    problems.append(f"result {actual['result']} != {gold['result']}")
                if problems:
                    raise ValueError(row['name'] + ': native differs (' + '; '.join(problems) + ')\nnative=' +
                                     json.dumps(dict(writes=actual['writes'], fault=actual.get('fault'),
                                                     result=actual['result'])) +
                                     '\noriginal=' + json.dumps(dict(writes=gold['writes'], boundary=gold['boundary'],
                                                                     result=gold['result'])) +
                                     '\nargs=' + json.dumps(row['args']) + ' fp=' + json.dumps(row['fp']))
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
    report = dict(passed=True, physical_cases=len(rows), ordered_stores=stores, comparison_sha256=total.hexdigest(),
                  outcomes={f'{k[0]}:{k[1]}': n for k, n in sorted(outcomes.items())}, coverage=coverage,
                  original_xbe_sha256=v.NEW_PIN, instruction_patches=[], supplied_original_outcomes=False,
                  retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} physical composite cases, {stores} ordered original stores')
    for key, n in sorted(outcomes.items()):
        print(f'  {key[0]}: {key[1]} x{n}')
    return 0


if __name__ == '__main__':
    raise SystemExit(composite_main())
