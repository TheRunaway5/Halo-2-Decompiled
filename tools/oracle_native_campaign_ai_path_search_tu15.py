#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native 27B2B0 / 27B340 / 27BD30.

27B2B0 is the path search 26A380 calls at 26A831 (EAX = path workspace).
Each case installs a declared workspace, a synthetic navigation graph
(sectors, boundary edges, vertices, links), frame/object/body banks for the
218780/218960 frame resolution, the 202970 device tables, the 278E60/FF1A0
jump scenario and the actual .rdata constants, then single-steps the
authenticated English TU1.5 closure: 27B2B0, 27B100, 27B340, 27BD30, the
heap (27BB60/27BC00/27BCE0), 202970/186E30, 218C40/218B20/218780/219030/
145070, 218EB0/218880/145140, 27C520/218960/145200/218FE0, 278E60/FF1A0/
FF580/30BD0 and 32CC0C. Nothing is a stop entry: the whole closure runs.
Direct 27B340 cases pre-populate search state (node/heap limits, hash
probing); direct 27BD30 cases declare their row array. Optional stack banks
expose 27B340's frame (candidate point, goal distance, 218EB0 difference,
neighbor rows) with nonzero stack history, so the never-initialized row
+A/+B and candidate-frame upper words are compared too.
No instruction is patched and no callee outcome is supplied. The ELF,
observer and raw banks live only in HALO2_TEST_SCRATCH.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
import oracle_native_campaign_ai_area_perform_tu15 as reference

ENTRIES = (0x27b2b0, 0x27b100, 0x27b340, 0x27bd30, 0x27bb60, 0x27bc00, 0x27bce0, 0x202970, 0x186e30,
           0x218c40, 0x218b20, 0x218780, 0x219030, 0x145070, 0x218eb0, 0x218880, 0x145140,
           0x27c520, 0x218960, 0x145200, 0x218fe0, 0x278e60, 0xff1a0, 0xff580, 0x30bd0, 0x32cc0c)
OWNED = (0x27b2b0, 0x27b340, 0x27bd30, 0x202970, 0x186e30)
BASE = reference.areas.BASE
W = BASE + 0x10000
WORLD, WORLD_SIZE = BASE + 0x30000, 0x6000
NAV, SECTORS, EDGES, VERTS, LINKS = (WORLD + n for n in (0, 0x100, 0x1100, 0x1500, 0x1b00))
FRAMES, OBJHDR, OBJROWS, BODY = (WORLD + n for n in (0x2200, 0x2600, 0x2700, 0x2800))
DEVROOT, DEVA, DEVTABLE, DEVBITS = (WORLD + n for n in (0x3000, 0x3040, 0x3100, 0x3400))
SCEN, BLOCK, GRAVITY = (WORLD + n for n in (0x4000, 0x4100, 0x4200))
ROWS = WORLD + 0x4400
SP, STOP = reference.SP, reference.STOP
NONE = 0xffffffff
CONSTANTS = ((0x4500b4, 4), (0x45695c, 4), (0x46a0c0, 0xd0), (0x46a460, 4), (0x46a4c0, 4), (0x44abe0, 0x100))
FP_PROFILES = ((0x27f, 0, 0x1f80), (0x37f, 0x0020, 0x1fa0), (0x07f, 0x3800, 0x9fc0), (0x67f, 0x1000, 0x7f80),
               (0x27f, 0x0820, 0x3f80), (0x47f, 0x0000, 0x5f80))
REP_SITE = 0x27b2dd


def f(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]


class World:
    """A synthetic navigation graph and its retained support data."""

    def __init__(self):
        self.raw = bytearray(WORLD_SIZE)
        self.sectors, self.edges, self.verts, self.links = [], [], [], []
        self.put(NAV + 4, SECTORS)
        self.put(NAV + 0xc, EDGES)
        self.put(NAV + 0x2c, VERTS)
        self.put(NAV + 0x3c, LINKS)
        # One valid object frame (0) with scale 2 and a translation, one
        # missing object (1) and one out-of-range node (2).
        for n, (obj, node, axis, variant) in enumerate(((0x12340000, 0, 0, 0), (NONE, 0, 0, 0), (0x12340000, 7, 1, 1),
                                                        (0x12340000, 1, 2, 1))):
            self.put(FRAMES + n * 16, obj)
            self.put(FRAMES + n * 16 + 6, node, 2)
            self.put(FRAMES + n * 16 + 0xa, axis, 1)
            self.put(FRAMES + n * 16 + 0xb, variant, 1)
        self.put(OBJHDR + 0x44, OBJROWS)
        self.put(OBJROWS + 8, BODY)
        self.put(BODY + 0x114, 2 * 52, 2)
        self.put(BODY + 0x116, 0x40, 2)
        for node, (scale, basis, origin) in enumerate(((2.0, (1, 0, 0, 0, 1, 0, 0, 0, 1), (3.0, -2.0, 0.5)),
                                                       (1.0, (0, 1, 0, -1, 0, 0, 0, 0, 1), (-1.0, 4.0, 2.0)))):
            m = BODY + 0x40 + node * 0x34
            self.put(m, f(scale))
            for k, v in enumerate(basis):
                self.put(m + 4 + k * 4, f(v))
            for k, v in enumerate(origin):
                self.put(m + 0x28 + k * 4, f(v))
        self.put(DEVROOT + 0x18, DEVA)
        self.put(DEVA + 0x2c, DEVTABLE)
        self.put(SCEN + 0xc8, 1)
        self.put(SCEN + 0xcc, BLOCK)
        for k in range(6):
            self.put(BLOCK + 0x80 + k * 4, f(6.0 + k))
            self.put(BLOCK + 0x98 + k * 4, f(3.0))
        self.put(GRAVITY, f(1.0))

    def put(self, address, value, size=4):
        self.raw[address - WORLD:address - WORLD + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')

    def putf(self, address, value):
        self.put(address, f(value))

    def vertex(self, x, y, z):
        self.verts.append((x, y, z))
        return len(self.verts) - 1

    def sector(self, flags=0, link=0xffff, ring=NONE):
        self.sectors.append([flags, link, ring])
        return len(self.sectors) - 1

    def edge(self, a, b, side_a, side_b, portal=0x80, next_a=0, next_b=0):
        self.edges.append([a, b, portal, next_a, next_b, side_a, side_b])
        return len(self.edges) - 1

    def link(self, sector, kind, words=(0, 0, 0, 0), flags=0, mask=0, target=0xffff, frames=(0xffff, 0xffff), raw4=None, raw8=None):
        """Prepend a link to `sector`'s linked list."""
        self.links.append(dict(kind=kind, next=self.sectors[sector][1], words=words, flags=flags, mask=mask,
                               target=target, frames=frames, raw4=raw4, raw8=raw8))
        self.sectors[sector][1] = len(self.links) - 1
        return len(self.links) - 1

    def portal(self, a, b, va, vb, flag=0x80):
        """A shared edge with kind-0 links from both sides."""
        e = self.edge(va, vb, a, b, flag)
        self.link(a, 0, raw4=e)
        self.link(b, 0, raw4=e)
        return e

    def ring(self, sector, edges):
        """Close sector's boundary ring through each edge's matching side."""
        for n, e in enumerate(edges):
            nxt = edges[(n + 1) % len(edges)]
            rec = self.edges[e]
            if rec[6] == sector:
                rec[4] = nxt
            else:
                rec[3] = nxt
        self.sectors[sector][2] = edges[0]

    def pack(self, link_count=None):
        self.put(NAV, len(self.sectors))
        self.put(NAV + 0x38, len(self.links) if link_count is None else link_count)
        for n, (flags, link, ring) in enumerate(self.sectors):
            self.put(SECTORS + n * 8, flags, 2)
            self.put(SECTORS + n * 8 + 2, link, 2)
            self.put(SECTORS + n * 8 + 4, ring)
        for n, (a, b, portal, na, nb, sa, sb) in enumerate(self.edges):
            e = EDGES + n * 16
            self.put(e, a, 2)
            self.put(e + 2, b, 2)
            self.put(e + 4, portal, 1)
            self.put(e + 8, na, 2)
            self.put(e + 0xa, nb, 2)
            self.put(e + 0xc, sa, 2)
            self.put(e + 0xe, sb, 2)
        for n, v in enumerate(self.verts):
            for k in range(3):
                self.putf(VERTS + n * 12 + k * 4, v[k])
        for n, l in enumerate(self.links):
            r = LINKS + n * 20
            self.put(r, l['kind'], 2)
            self.put(r + 2, l['next'], 2)
            for k, w in enumerate(l['words']):
                self.put(r + 4 + k * 2, w, 2)
            if l['raw4'] is not None:
                self.put(r + 4, l['raw4'])
            if l['raw8'] is not None:
                self.put(r + 8, l['raw8'])
            self.put(r + 0xc, l['flags'], 1)
            self.put(r + 0xd, l['mask'], 1)
            self.put(r + 0xe, l['target'], 2)
            self.put(r + 0x10, l['frames'][0], 2)
            self.put(r + 0x12, l['frames'][1], 2)
        return self.raw


def grid(width, height, size=4.0, flags=1, portal=0x80, links=False):
    """Rectangular sectors with closed boundary-edge rings (flag bit 0 makes
    27BD30 walk the ring and 27B340 accept the sector as a target).
    Optional kind-0 links duplicate the interior portals."""
    w = World()
    corner = {}
    for y in range(height + 1):
        for x in range(width + 1):
            corner[x, y] = w.vertex(x * size, y * size, 0.0)
    cell = {}
    for y in range(height):
        for x in range(width):
            cell[x, y] = w.sector(flags)
    side = lambda x, y: cell.get((x, y), 0xffff)
    horizontal, vertical = {}, {}
    for y in range(height + 1):
        for x in range(width):
            horizontal[x, y] = w.edge(corner[x, y], corner[x + 1, y], side(x, y - 1), side(x, y), portal)
    for y in range(height):
        for x in range(width + 1):
            vertical[x, y] = w.edge(corner[x, y], corner[x, y + 1], side(x - 1, y), side(x, y), portal)
    for (x, y), c in cell.items():
        w.ring(c, [horizontal[x, y], vertical[x + 1, y], horizontal[x, y + 1], vertical[x, y]])
        if links:
            if x + 1 < width:
                w.link(c, 0, raw4=vertical[x + 1, y])
            if y + 1 < height:
                w.link(c, 0, raw4=horizontal[x, y + 1])
    return w, cell


class Workspace:
    def __init__(self, nodes=48, heap=64, full=False):
        self.full = full
        self.nodes, self.heap = nodes, heap
        self.raw = bytearray(0x140c8)
        self.put(0x8c, NAV)
        self.putf(0x0, 0.5)
        self.put(0x140b8, 0x7f)
        self.put(0x140c0, 0x3f)
        self.point(0x14, (2.0, 2.0, 0.0), 0xffff)
        self.put(0x24, 0)

    def put(self, offset, value, size=4):
        self.raw[offset:offset + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')

    def putf(self, offset, value):
        self.put(offset, f(value))

    def point(self, offset, xyz, frame, upper=0):
        for k in range(3):
            self.putf(offset + k * 4, xyz[k])
        self.put(offset + 0xc, frame | (upper << 16))

    def goal(self, sector, xyz, frame=0xffff, threshold=0.25):
        self.put(0x54, 1, 1)
        self.point(0x58, xyz, frame)
        self.put(0x68, sector)
        self.putf(0x6c, threshold)

    def banks(self):
        if self.full:
            return [(W, self.raw)]
        head = 0xb0 + self.nodes * 0x44
        heap = 6 + 4 * (self.heap + 1)
        return [(W, self.raw[:head]), (W + 0x110b0, self.raw[0x110b0:0x110b0 + heap]),
                (W + 0x120b6, self.raw[0x120b6:])]


def history(address, size, seed):
    return bytes(((address + n) * 0x9d + seed * 0x3b + 0x5a) & 0xff for n in range(size))


def make_case(image, rows, name, entry, workspace, world, *, fp=None, gp_seed=None, stack_history=False,
              arguments=None, extra=(), constants=(), globals_=None, max_steps=100000, link_count=None, **fields):
    n = len(rows)
    world_raw = world.pack(link_count)
    banks = {WORLD: bytearray(world_raw)}
    for address, data in workspace.banks():
        banks[address] = bytearray(data)
    for address, size in CONSTANTS:
        banks[address] = bytearray(image.read(address, size))
    for address, value in constants:
        for a in banks:
            if a <= address < a + len(banks[a]):
                banks[a][address - a:address - a + 4] = value.to_bytes(4, 'little')
    g = dict(group=1, up=0x44abec, gravity_vector=0x44ac40)
    g.update(globals_ or {})
    banks[0x474c24] = bytearray(struct.pack('<H', g['group'] & 0xffff))
    banks[0x474d04] = bytearray(struct.pack('<III', g['up'], 0, 0) + struct.pack('<I', g['gravity_vector']))
    banks[0x4ecbd0] = bytearray(struct.pack('<I', OBJHDR))
    banks[0x4ecc18] = bytearray(struct.pack('<II', DEVROOT, g.get('scenario', SCEN)))
    banks[0x4f9b50] = bytearray(struct.pack('<I', DEVBITS))
    banks[0x505e78] = bytearray(struct.pack('<I', FRAMES))
    banks[0x52be68] = bytearray(struct.pack('<I', GRAVITY))
    for address, data in extra:
        banks[address] = bytearray(data)
    sp = SP - (n % 4) * 4
    if stack_history:
        # 27B340's frame: candidate point + goal distance, the 218EB0
        # difference and the rows. Under 27B2B0 the top rows also hold the
        # earlier 27B100/218C40/218B20/218780/32CC0C frames (to about
        # entry-0x80), whose leftovers native owners keep in host storage;
        # that band stays undeclared.
        frame = (sp - 0x14 if entry == 0x27b2b0 else sp) - 0xc54
        top = 0xc54 - (0xec if entry == 0x27b2b0 else 0)
        for start, end in ((0x1c, 0x30), (0x48, 0x54), (0x54, top)):
            banks[frame + start] = bytearray(history(frame + start, end - start, n))
    seed = n if gp_seed is None else gp_seed
    gp = [0x5a200000 + seed * 37 + k * 13 for k in range(8)]
    gp[4] = sp
    if entry == 0x27b2b0:
        gp[0] = W
    matrix, source, stack_arguments = 0, 0, []
    if entry == 0x27b340:
        matrix = W
    if entry == 0x27bd30:
        matrix, source, stack_arguments = arguments
    profile = FP_PROFILES[n % len(FP_PROFILES)] if fp is None else fp
    rows.append(dict(id=n, name=name, entry=entry, gp=gp, matrix=matrix, source=source,
                     stack_arguments=stack_arguments,
                     banks=[dict(address=a, before=bytes(d).hex()) for a, d in sorted(banks.items())],
                     before_fp=list(profile), live_x87=[],
                     xmm=[[0x6b100000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)],
                     stop_before_entries=[], max_steps=max_steps, strict_writes=False, **fields))


def search_cases(image, rows):
    """27B2B0 whole searches over synthetic graphs."""
    def run(name, world, ws, **kw):
        make_case(image, rows, name, 0x27b2b0, ws, world, **kw)

    # Corridors with and without goals; the retry counter.
    for length in (1, 2, 4):
        for goal in (False, True):
            for counter in (0, 3, 0x20, 0x7fff, 0x8000):
                w, cell = grid(length, 1, links=bool(counter & 1))
                ws = Workspace()
                ws.put(0x70, counter, 2)
                if goal:
                    ws.goal(cell[length - 1, 0], (length * 4.0 - 2.0, 2.0, 0.0))
                run(f'corridor_{length}_{int(goal)}_{counter:x}', w, ws)
    # Unreachable goal and threshold outcomes.
    for threshold in (0.0, 0.25, 100.0, -1.0):
        w, cell = grid(3, 1)
        lone = w.sector(0)
        ws = Workspace()
        ws.goal(lone, (30.0, 2.0, 0.0), threshold=threshold)
        run(f'unreachable_{threshold}', w, ws)
    # Grids: several routes, decrease-key and hash collisions.
    for size, goal_cell in (((3, 3), (2, 2)), ((4, 3), (3, 0)), ((5, 2), (4, 1))):
        for history_bank in (False, True):
            w, cell = grid(*size)
            ws = Workspace(nodes=64, heap=64)
            gx, gy = goal_cell
            ws.goal(cell[gx, gy], (gx * 4.0 + 1.0, gy * 4.0 + 3.0, 0.0))
            run(f'grid_{size[0]}x{size[1]}_{int(history_bank)}', w, ws, stack_history=history_bank)
    # No goal: exhaustive expansion of a grid (result is the frame byte 1).
    for history_bank in (False, True):
        w, cell = grid(3, 2)
        run(f'grid_exhaustive_{int(history_bank)}', w, Workspace(nodes=64), stack_history=history_bank)
    # 27B100 refusals.
    w, cell = grid(2, 1)
    for label, edit in (('no_navigation', lambda s: s.put(0x8c, 0)), ('negative_sector', lambda s: s.put(0x24, NONE)),
                        ('sector_range', lambda s: s.put(0x24, 2)), ('low_start', lambda s: s.putf(0x1c, -1000.0)),
                        ('nan_start', lambda s: s.put(0x1c, 0x7fc00000))):
        for counter in (0, 0x1f, 0x20, 0x7fff, 0xffff):
            ws = Workspace()
            edit(ws)
            ws.put(0x70, counter, 2)
            run(f'query_start_{label}_{counter:x}', w, ws)
    # Far goal: the quantized start priority reaches 0x7FFF.
    w, cell = grid(2, 1)
    ws = Workspace()
    ws.goal(cell[1, 0], (4000.0, 2.0, 0.0))
    run('far_goal', w, ws)
    # Goal reached at the first pop (best becomes 0: threshold 0 ties it).
    for threshold in (0.0, -0.0):
        w, cell = grid(2, 1)
        ws = Workspace()
        ws.goal(cell[0, 0], (1.0, 1.0, 0.0), threshold=threshold)
        run(f'goal_first_tie_{threshold}', w, ws)
    for frame in (0xffff, 0):
        w, cell = grid(2, 1)
        ws = Workspace()
        ws.goal(cell[0, 0], (1.0, 1.0, 0.0), frame=frame)
        run(f'goal_first_{frame:x}', w, ws)
    # Exclusions (+70 mask over the +74 list) and the +4 special check.
    for mask, count in ((1, 1), (3, 2), (2, 2), (1, 0), (0x8001, 1)):
        w, cell = grid(3, 1)
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        ws.put(0x70, mask, 2)
        ws.put(0x72, count, 2)
        ws.put(0x74, 0xffff, 2)
        ws.put(0x78, 0)
        ws.put(0x7c, 0xffff, 2)
        ws.put(0x80, 2)
        run(f'exclusion_{mask:x}_{count}', w, ws)
    # Devices on kind-7 links of special (flag 2) sectors.
    for label, flags, bit_set, gate, argument in (('blocked', 8, False, 3, NONE), ('open', 8, True, 3, NONE),
                                                  ('ungated', 0, False, 3, NONE), ('indexed', 8, True, 37, 5),
                                                  ('indexed_clear', 8, False, 37, 5), ('disabled', 8, False, 3, NONE)):
        w, cell = grid(3, 1)
        w.sectors[cell[1, 0]][0] = 3
        w.link(cell[1, 0], 3)
        w.link(cell[1, 0], 7, raw4=2, raw8=argument)
        w.put(DEVTABLE + 2 * 8 + 4, flags, 1)
        w.put(DEVTABLE + 2 * 8 + 5, gate, 1)
        group = 1
        field = DEVBITS + (1 + group * 32 if argument == NONE else group * 1024 + argument + 0x201)
        if bit_set:
            w.put(field + (gate >> 5) * 4, 1 << (gate & 31))
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        if label == 'disabled':
            ws.put(4, 1, 1)
        run(f'device_{label}', w, ws)
    # Kind-7 after another link; a list without kind 7.
    for label, kinds in (('second', (7, 3)), ('absent', (3, 4))):
        w, cell = grid(3, 1)
        w.sectors[cell[1, 0]][0] = 3
        for kind in kinds:
            w.link(cell[1, 0], kind, raw4=2, raw8=NONE)
        w.put(DEVTABLE + 2 * 8 + 4, 8, 1)
        w.put(DEVTABLE + 2 * 8 + 5, 3, 1)
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        run(f'device_list_{label}', w, ws)
    # A sector with no kind-7 link and a special flag; an empty link list.
    w, cell = grid(3, 1)
    w.sectors[cell[1, 0]][0] = 3
    ws = Workspace()
    ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
    run('device_none', w, ws)
    # Influence, penalty, bound and the node-bound exit.
    for radius, weight in ((0.0, 1.0), (3.0, 0.5), (8.0, 2.0), (100.0, -0.5)):
        w, cell = grid(3, 2)
        ws = Workspace(nodes=64)
        ws.goal(cell[2, 1], (10.0, 6.0, 0.0))
        ws.put(0x28, 1, 1)
        ws.point(0x2c, (6.0, 4.0, 0.0), 0xffff)
        ws.putf(0x3c, radius)
        ws.putf(0x40, weight)
        run(f'influence_{radius}_{weight}', w, ws)
    for penalty in (0.0, 1.5, -2.0):
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        v0, v1 = w.vertex(1.0, 1.0, 0.0), w.vertex(9.0, 1.0, 0.0)
        w.link(a, 2, words=(b, v1, 0, 0))
        w.link(a, 5, words=(b, 0, v1, 0), flags=1)
        ws = Workspace()
        ws.goal(b, (10.0, 2.0, 0.0))
        ws.putf(0x4c, penalty)
        ws.put(0x140bc, 0x400)
        run(f'penalty_{penalty}', w, ws)
    for bound in (1.0, 6.0, 1000.0):
        w, cell = grid(4, 1)
        ws = Workspace()
        ws.goal(cell[3, 0], (14.0, 2.0, 0.0))
        ws.put(0x45, 1, 1)
        ws.putf(0x48, bound)
        run(f'bound_{bound}', w, ws)
    # Wide/narrow portals: the clearance gate and the goal slide (len^2 > 16).
    for clamp in (0.1, 0.5, 3.0, 6.0):
        for flag in (0x80, 0):
            w, cell = grid(3, 2, size=8.0, portal=flag)
            ws = Workspace(nodes=64)
            ws.putf(0, clamp)
            ws.point(0x14, (4.0, 4.0, 0.0), 0xffff)
            ws.goal(cell[2, 1], (22.0, 13.0, 0.0))
            run(f'portal_{clamp}_{flag:x}', w, ws)
    # Framed goal/start points: 218C40/218EB0 projections.
    for start_frame, goal_frame in ((0, 0xffff), (0xffff, 0), (0, 0), (1, 0xffff), (2, 0), (3, 3)):
        w, cell = grid(3, 1, size=8.0)
        ws = Workspace()
        ws.point(0x14, (-1.0, 3.0, 0.0), start_frame)
        ws.goal(cell[2, 0], (8.0, 1.0, 0.0), frame=goal_frame)
        run(f'framed_{start_frame:x}_{goal_frame:x}', w, ws)
    # Framed influence: 218960 resolves the center and candidate into the
    # node frame (valid object, missing object, node out of range, node 1).
    for frame in (0, 1, 2, 3):
        w, cell = grid(3, 2)
        ws = Workspace(nodes=64)
        ws.point(0x14, (2.0, 2.0, 0.0), frame)
        ws.goal(cell[2, 1], (10.0, 6.0, 0.0))
        ws.put(0x28, 1, 1)
        ws.point(0x2c, (6.0, 4.0, 0.0), 0 if frame == 3 else frame)
        ws.putf(0x3c, 8.0)
        ws.putf(0x40, 1.0)
        run(f'influence_framed_{frame}', w, ws)
    # Framed slides: the equal-frame 218EB0 resolves its direction (218880).
    for frame in (0, 1, 2, 3):
        w, cell = grid(3, 2, size=8.0)
        ws = Workspace(nodes=64)
        ws.point(0x14, (4.0, 4.0, 0.0), frame)
        ws.goal(cell[2, 1], (22.0, 13.0, 0.0), frame=frame)
        run(f'portal_framed_{frame}', w, ws, stack_history=frame == 0)
    # Gravity and speed sweep for the composed 278E60/FF1A0 (FF580 fallback).
    for gravity, speed in ((0.0, 6.0), (-1.0, 6.0), (1e-30, 6.0), (100.0, 1.0), (1.0, 0.0), (float('inf'), 6.0)):
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        va, vc, vd = w.vertex(2.0, 2.0, 0.0), w.vertex(9.0, 2.0, 1.0), w.vertex(9.0, 3.0, 1.0)
        w.link(a, 1, words=(va, 0, vc, vd), mask=1, target=b, frames=(0xffff, 0))
        w.putf(GRAVITY, gravity)
        for k in range(6):
            w.putf(BLOCK + 0x80 + k * 4, speed)
        ws = Workspace()
        ws.goal(b, (10.0, 2.0, 1.0))
        run(f'jump_gravity_{gravity}_{speed}', w, ws)
    # Ring sectors (flag 1|ring) and sectors without links.
    for variant in range(3):
        w = World()
        c = [w.vertex(x, y, 0.0) for x, y in ((0, 0), (4, 0), (8, 0), (8, 4), (4, 4), (0, 4))]
        a, b = w.sector(1 if variant == 1 else 0), w.sector(1)
        e0 = w.edge(c[1], c[4], a, b, 0x80)
        e1 = w.edge(c[4], c[5], a, 0xffff)
        e2 = w.edge(c[5], c[0], a, 0xffff)
        e3 = w.edge(c[0], c[1], a, 0xffff)
        w.ring(a, [e0, e1, e2, e3])
        e4 = w.edge(c[1], c[2], b, 0xffff)
        e5 = w.edge(c[2], c[3], b, 0xffff, 0)
        e6 = w.edge(c[3], c[4], b, 0xffff)
        w.ring(b, [e4, e5, e6, e0])
        if variant == 2:
            w.link(a, 0, raw4=e0)
        ws = Workspace()
        ws.goal(b, (6.0, 2.0, 0.0))
        run(f'ring_{variant}', w, ws)
    # Jump links (kinds 1 and 6) with frames, masks and policies.
    for label, kind, flags, mask, frames, policy, allow, rise in (
            ('free', 1, 1, 0, (0xffff, 0xffff), 0x3f, 0, 0.0), ('same_frame', 1, 0, 1, (0, 0), 0x3f, 0, 0.0),
            ('arc', 1, 0, 1, (0xffff, 0), 0x3f, 0, 0.0), ('arc6', 6, 0, 2, (0, 0xffff), 0x3f, 0, 1.0),
            ('masked', 1, 0, 4, (0xffff, 0), 0x03, 0, 0.0), ('allowed', 1, 0, 0, (0xffff, 0), 0x3f, 1, 0.0),
            ('high', 1, 0, 1, (0xffff, 0), 0x3f, 0, 30.0), ('steep', 6, 0, 0x81, (0xffff, 0), 0x01, 0, 0.0),
            ('exhausted', 1, 0, 0, (0xffff, 0), 0x3f, 1, -30.0), ('exhausted_sparse', 6, 0, 0x40, (0, 0xffff), 0x6a, 0, -30.0)):
        # 278E60 applies the +98 rise limit when the first stack point (A)
        # is above the ECX point (C): a negative rise here.
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        va = w.vertex(2.0, 2.0, -rise if rise < 0 else 0.0)
        vc, vd = w.vertex(9.0, 2.0, max(rise, 0.0)), w.vertex(9.0, 3.0, max(rise, 0.0))
        w.link(a, kind, words=(va, 0, vc, vd), flags=flags, mask=mask, target=b, frames=frames)
        ws = Workspace()
        ws.goal(b, (10.0, 2.0, rise))
        ws.put(0x140c0, policy)
        ws.put(0x140c4, allow, 1)
        run(f'jump_{label}', w, ws)
    # Only kind 5 is enabled (+140C4 admits the link without a mask).
    w, cell = grid(3, 1)
    a, b = cell[0, 0], cell[2, 0]
    va, vc, vd = w.vertex(2.0, 2.0, 0.0), w.vertex(9.0, 2.0, 0.0), w.vertex(9.0, 3.0, 0.0)
    w.link(a, 1, words=(va, 0, vc, vd), target=b, frames=(0xffff, 0))
    ws = Workspace()
    ws.goal(b, (10.0, 2.0, 0.0))
    ws.put(0x140c0, 0x20)
    ws.put(0x140c4, 1, 1)
    run('jump_only_kind5', w, ws)
    # Jump, kind-2 and kind-5 targets whose sector lacks flag bit 0: only
    # kinds 1/6/2 force it into the row flags.
    for kind in (1, 6, 2, 5):
        w, cell = grid(2, 1)
        a = cell[0, 0]
        lone = w.sector(0x40)
        v = w.vertex(12.0, 2.0, 0.0)
        if kind in (1, 6):
            w.link(a, kind, words=(v, 0, v, v), flags=1, mask=1, target=lone)
        elif kind == 2:
            w.link(a, 2, words=(lone, v, 0, 0))
        else:
            w.link(a, 5, words=(lone, 0, v, 0), flags=1)
        ws = Workspace()
        ws.goal(lone, (12.0, 2.0, 0.0))
        ws.put(0x140bc, 0x400)
        run(f'unflagged_target_{kind}', w, ws)
    # Kind 2 modes and kind 5 policies, kinds 3/4/7 and masked kinds.
    for mode in (0, 1, 2, 3):
        for near in (False, True):
            w, cell = grid(3, 1)
            a, b = cell[0, 0], cell[2, 0]
            v = w.vertex(2.1 if near else 3.5, 2.0, 0.0)
            w.link(a, 2, words=(b, v, 0, mode))
            ws = Workspace()
            ws.goal(b, (10.0, 2.0, 0.0))
            run(f'kind2_{mode}_{int(near)}', w, ws)
    for policy, flags in ((0x400, 1), (0x400, 2), (0x800, 2), (0x1000, 4), (0x1000, 3), (0, 7), (0x1c00, 0)):
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        v = w.vertex(9.0, 2.0, 0.0)
        w.link(a, 5, words=(b, 0, v, 0), flags=flags)
        w.link(a, 3, words=(b, 0, v, 0))
        w.link(a, 4, words=(b, 0, v, 0))
        w.link(a, 0xffff)
        ws = Workspace()
        ws.goal(b, (10.0, 2.0, 0.0))
        ws.put(0x140bc, policy)
        run(f'kind5_{policy:x}_{flags}', w, ws)
    for kinds in (0x01, 0x7e, 0x00):
        w, cell = grid(3, 1)
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        ws.put(0x140b8, kinds)
        run(f'kind_mask_{kinds:x}', w, ws)
    # Sector flags: untraversable targets, 0x1000 with node +C, 0x3C0.
    for flags in (0, 0x1001, 0x0041, 0x1000 | 0x3c1):
        w, cell = grid(3, 1)
        w.sectors[cell[1, 0]][0] = flags
        w.sectors[cell[0, 0]][0] = 0x1001
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        run(f'sector_flags_{flags:x}', w, ws)
    # Targets with bit 12/11 or 0x3C0 bits from an unrestricted node.
    for flags in (0x1001, 0x0801, 0x0201, 0x0041):
        w, cell = grid(3, 1)
        w.sectors[cell[1, 0]][0] = flags
        ws = Workspace()
        ws.goal(cell[2, 0], (10.0, 2.0, 0.0))
        run(f'target_flags_{flags:x}', w, ws)
    # The +45 bound compares the accumulated distance, not the penalized cost.
    for bound in (7.5, 8.0, 9.0):
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        v = w.vertex(9.0, 2.0, 0.0)
        w.link(a, 2, words=(b, v, 0, 0))
        ws = Workspace()
        ws.goal(b, (10.0, 2.0, 0.0))
        ws.putf(0x4c, 1.5)
        ws.put(0x45, 1, 1)
        ws.putf(0x48, bound)
        run(f'bound_penalty_{bound}', w, ws)
    # A NaN vertex: CVTTSS2SI's indefinite integer passes the 0x7FFF gate.
    w, cell = grid(2, 1)
    w.verts[1] = (float('nan'), 0.0, 0.0)
    ws = Workspace()
    ws.goal(cell[1, 0], (6.0, 2.0, 0.0))
    run('nan_vertex', w, ws)


def expand_cases(image, rows):
    """Direct 27B340 entries over pre-populated search state."""
    def prepared(name, world, ws, heap_nodes, **kw):
        """heap_nodes: [(index, priority)] already published and queued."""
        ws.put(0x110b0, len(heap_nodes) + 1, 2)
        for slot, (index, priority) in enumerate(heap_nodes, 1):
            ws.put(0x110b0 + 2 + slot * 4, index, 2)
            ws.put(0x110b0 + 4 + slot * 4, priority, 2)
        make_case(image, rows, name, 0x27b340, ws, world, **kw)

    def node(ws, index, sector, xyz, priority=0, back=0xffff, cost=0.0, special=0, frame=0xffff, upper=0):
        rec = 0xb0 + index * 0x44
        ws.put(rec + 2, 0xffff, 2)
        ws.put(rec + 4, NONE)
        ws.put(rec + 8, sector)
        ws.put(rec + 0xe, special, 1)
        ws.point(rec + 0x18, xyz, frame, upper)
        ws.putf(rec + 0x2c, 3.4e38)
        ws.putf(rec + 0x34, cost)
        ws.putf(rec + 0x38, cost)
        ws.put(rec + 0x3c, priority, 2)
        ws.put(rec + 0x40, back, 2)

    def hash_put(ws, key, index, probe=0):
        slot = (((key & 0x1ff) << 3) + probe) & 0xfff
        ws.put(0x120b6 + slot * 2, index, 2)

    def blank_hash(ws):
        ws.raw[0x120b6:0x140b6] = b'\xff' * 0x2000

    # The node table and the heap at their 0x400 limits. Queued NONE
    # entries end the search at the next pop.
    for nodes, heap in ((0x3ff, 8), (0x400, 8), (0x401, 8), (3, 0x3fe), (3, 0x3ff), (3, 0x400)):
        w, cell = grid(3, 1)
        ws = Workspace(full=True)
        blank_hash(ws)
        node(ws, 0, cell[0, 0], (2.0, 2.0, 0.0), back=1)
        hash_put(ws, cell[0, 0], 0)
        ws.put(0xae, nodes, 2)
        prepared(f'limits_{nodes:x}_{heap:x}', w, ws, [(0, 0)] + [(0xffff, 0x7000)] * (heap - 1))
    # Hash probing: collisions, found better/worse/closed, special points.
    for label, edit in (('collide', 0), ('found_worse', 2), ('found_better', 3), ('found_closed', 4),
                        ('special_far', 5), ('special_near', 6)):
        w, cell = grid(2, 1)
        a, b = cell[0, 0], cell[1, 0]
        ws = Workspace(full=True)
        blank_hash(ws)
        node(ws, 0, a, (2.0, 2.0, 0.0), back=1)
        hash_put(ws, a, 0)
        ws.put(0xae, 3, 2)
        queue = [(0, 0)]
        if edit == 0:
            node(ws, 1, b + 0x200, (6.0, 2.0, 0.0), priority=5)
            hash_put(ws, b, 1)
            node(ws, 2, b + 0x400, (6.0, 2.0, 0.0), priority=5)
            hash_put(ws, b, 2, 1)
        elif edit in (2, 3, 4):
            node(ws, 1, b, (4.0, 3.0, 0.0), priority=(0 if edit == 2 else 0x7000), back=(0xffff if edit == 4 else 2))
            hash_put(ws, b, 1)
            if edit != 4:
                queue.append((1, 0x7000))
        else:
            node(ws, 1, b, (4.0, 3.0 if edit == 5 else 0.1, 0.0), priority=0x7000, back=2, special=1)
            hash_put(ws, b, 1)
            queue.append((1, 0x7000))
        prepared(f'hash_{label}', w, ws, queue)
    # Wrap: a sector whose (key&0x1FF)<<3 is 0xFF8 probes 0xFF8..0xFFF, 0, 1.
    w = World()
    v = w.vertex(4.0, 2.0, 0.0)
    sectors = [w.sector(0) for _ in range(0x200)]
    w.link(sectors[0], 2, words=(0x1ff, v, 0, 0))
    ws = Workspace(full=True)
    blank_hash(ws)
    node(ws, 0, 0, (2.0, 2.0, 0.0), back=1)
    for probe in range(8):
        node(ws, 1 + probe, 0x1ff + 0x200 * (probe + 1), (6.0, 2.0, 0.0))
        ws.put(0x120b6 + (0xff8 + probe) * 2, 1 + probe, 2)
    node(ws, 9, 0x1ff + 0x2000, (6.0, 2.0, 0.0))
    ws.put(0x120b6, 9, 2)
    ws.put(0xae, 10, 2)
    prepared('hash_wrap_table_end', w, ws, [(0, 0)])
    # 64 kind-2 rows from one node, each publishing a node: every row's
    # never-written +A/+B word reaches node +12 from stack history.
    for history_bank, fp in ((True, None), (True, FP_PROFILES[2]), (False, None)):
        w = World()
        root = w.sector(0)
        targets = [w.sector(0) for _ in range(64)]
        for k, target in enumerate(targets):
            v = w.vertex(2.0 + (k % 8), 2.0 + (k // 8), 0.25 * (k % 3))
            w.link(root, 2, words=(target, v, 0, 0))
        ws = Workspace(full=True)
        blank_hash(ws)
        node(ws, 0, root, (1.0, 1.0, 0.0), back=1)
        hash_put(ws, root, 0)
        ws.put(0xae, 1, 2)
        prepared(f'wide_{int(history_bank)}_{0 if fp is None else 2}', w, ws, [(0, 0)], stack_history=history_bank, fp=fp)
    # Node bound exit: node +38 above max(5, best) * 10 + [+98].
    for best, node_cost, offset in ((1.0, 40.0, 0.0), (1.0, 60.0, 0.0), (9.0, 80.0, 0.0), (9.0, 100.0, 0.0),
                                    (1.0, 60.0, 15.0), (1.0, 40.0, -15.0), (6.0, 70.0, 5.0)):
        w, cell = grid(2, 1)
        ws = Workspace(full=True)
        blank_hash(ws)
        ws.goal(cell[1, 0], (6.0, 2.0, 0.0))
        ws.putf(0x94, best)
        ws.putf(0x98, offset)
        node(ws, 0, cell[0, 0], (2.0, 2.0, 0.0), back=1, cost=node_cost)
        hash_put(ws, cell[0, 0], 0)
        ws.put(0xae, 1, 2)
        prepared(f'node_bound_{best}_{node_cost}_{offset}', w, ws, [(0, 0)])
    # Priority at the 0x7FFF gate: 0.75 to the portal midpoint (4, 2) from
    # cost 3276.0 gives 32767.5 -> 0x7FFF (rejected); 3275.9 is accepted.
    for base in (3276.0, 3275.9, 3276.05):
        w, cell = grid(2, 1)
        ws = Workspace(full=True)
        blank_hash(ws)
        node(ws, 0, cell[0, 0], (3.25, 2.0, 0.0), back=1, cost=base)
        hash_put(ws, cell[0, 0], 0)
        ws.put(0xae, 1, 2)
        prepared(f'priority_gate_{base}', w, ws, [(0, 0)])
    # An empty heap entry (-1) and an empty queue.
    w, cell = grid(2, 1)
    ws = Workspace(full=True)
    blank_hash(ws)
    prepared('pop_none', w, ws, [(0xffff, 0)])
    prepared('queue_empty', w, Workspace(full=True), [])


def neighbor_cases(image, rows):
    """Direct 27BD30 entries with a declared 66-row array."""
    def run(name, world, ws, node_fields, overrun=False, **kw):
        nraw = bytearray(0x44)
        for offset, value, size in node_fields:
            nraw[offset:offset + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
        ws.raw[0xb0:0xb0 + 0x44] = nraw
        rows_bank = history(ROWS, 66 * 0x30, len(rows))
        world.pack()
        world.raw[ROWS - WORLD:ROWS - WORLD + len(rows_bank)] = rows_bank
        make_case(image, rows, name, 0x27bd30, ws, world, arguments=(NAV, W + 0xb0, [ROWS, W]),
                  overrun_control=overrun, **kw)

    def node(sector, xyz=(2.0, 2.0, 0.0), frame=0xffff, parent=0xffff, special=0):
        fields = [(2, parent, 2), (8, sector, 4), (0xd, special, 1), (0x24, frame, 2)]
        for k, v in enumerate(xyz):
            fields.append((0x18 + k * 4, f(v), 4))
        return fields

    # Every kind from one sector.
    w, cell = grid(3, 1)
    a, b = cell[0, 0], cell[2, 0]
    v = [w.vertex(*p) for p in ((2.0, 2.0, 0.0), (9.0, 2.0, 1.0), (9.0, 3.0, 1.0), (2.1, 2.0, 0.0))]
    w.link(a, 1, words=(v[0], 0, v[1], v[2]), flags=1, target=b)
    w.link(a, 6, words=(v[0], 0, v[1], v[2]), flags=0, mask=1, target=b, frames=(0xffff, 0))
    w.link(a, 2, words=(b, v[3], 0, 1))
    w.link(a, 2, words=(b, v[1], 0, 3))
    w.link(a, 5, words=(b, 0, v[1], 0), flags=4)
    w.link(a, 3)
    w.link(a, 4)
    w.link(a, 0xfffe)
    for special in (0, 1):
        for frame in (0xffff, 0):
            ws = Workspace()
            ws.put(0x140bc, 0x1000)
            run(f'neighbors_all_{special}_{frame:x}', w, ws, node(a, frame=frame, special=special))
    # Jump admission: allow byte, masks with the sign bit, NONE targets/verts.
    for label, allow, policy, mask, target in (('allow', 1, 0, 0, None), ('mask_sign', 0, 0x80000000, 0x80, None),
                                               ('mask_zero', 0, 0x3f, 0, None), ('target_none', 0, 0x3f, 1, 0xffff)):
        w, cell = grid(3, 1)
        a, b = cell[0, 0], cell[2, 0]
        v = [w.vertex(*p) for p in ((2.0, 2.0, 0.0), (9.0, 2.0, 0.0), (9.0, 3.0, 0.0))]
        w.link(a, 1, words=(v[0], 0, v[1], v[2]), flags=1, mask=mask, target=b if target is None else target)
        ws = Workspace()
        ws.put(0x140c0, policy)
        ws.put(0x140c4, allow, 1)
        run(f'neighbors_jump_{label}', w, ws, node(a))
    # Proximity rejection of kind 1 at a special node.
    w, cell = grid(3, 1)
    a, b = cell[0, 0], cell[2, 0]
    v = [w.vertex(*p) for p in ((5.0, 2.0, 0.0), (9.0, 2.0, 0.0), (9.0, 3.0, 0.0))]
    w.link(a, 1, words=(v[0], 0, v[1], v[2]), flags=1, target=b)
    run('neighbors_jump_far', w, Workspace(), node(a, special=1))
    # 64 linked rows stop the linked loop; the ring then publishes row 64.
    for ring in (False, True):
        w = World()
        c = [w.vertex(x, y, 0.0) for x, y in ((0, 0), (4, 0), (4, 4), (0, 4))]
        a = w.sector(1 if ring else 0)
        b = w.sector(1)
        e0 = w.edge(c[1], c[2], a, b)
        e1 = w.edge(c[2], c[3], a, b)
        e2 = w.edge(c[3], c[0], a, 0xffff)
        w.ring(a, [e0, e1, e2])
        for k in range(70):
            w.link(a, 0, raw4=e0 if k % 2 else e1)
        run(f'neighbors_limit_{int(ring)}', w, Workspace(), node(a), overrun=ring)
    # 63 linked rows, then the ring stops exactly at 64.
    w = World()
    c = [w.vertex(x, y, 0.0) for x, y in ((0, 0), (4, 0), (4, 4), (0, 4))]
    a, b = w.sector(1), w.sector(1)
    e0 = w.edge(c[1], c[2], a, b)
    e1 = w.edge(c[2], c[3], a, b)
    e2 = w.edge(c[3], c[0], a, b)
    e3 = w.edge(c[0], c[1], a, 0xffff)
    w.ring(a, [e0, e1, e2, e3])
    for k in range(63):
        w.link(a, 0, raw4=e0)
    run('neighbors_limit_ring_64', w, Workspace(), node(a))
    # Ring for a root node without links; a non-root node without links.
    for parent in (0xffff, 0):
        w = World()
        c = [w.vertex(x, y, 0.0) for x, y in ((0, 0), (4, 0), (4, 4))]
        a, b = w.sector(0), w.sector(1)
        e0 = w.edge(c[0], c[1], b, a, 0)
        e1 = w.edge(c[1], c[2], a, 0xffff)
        e2 = w.edge(c[2], c[0], a, b)
        w.ring(a, [e0, e1, e2])
        run(f'neighbors_ring_root_{parent:x}', w, Workspace(), node(a, parent=parent))
    # Non-positive link count skips the linked list.
    w, cell = grid(2, 1)
    run('neighbors_no_link_count', w, Workspace(), node(cell[0, 0]), link_count=0x80000000)


def inputs(image):
    rows = []
    search_cases(image, rows)
    expand_cases(image, rows)
    neighbor_cases(image, rows)
    return rows


def compact_writes(rows):
    """Collapse 27B2B0's REP STOSD iterations into counted runs."""
    for row in rows:
        out = []
        for w in row.get('writes', []):
            last = out[-1] if out else None
            if (w['instruction'] == REP_SITE and last is not None and last['instruction'] == REP_SITE
                    and last['before'] == w['before'] and last['written'] == w['written']
                    and w['address'] == last['address'] + 4 * last.get('repeat', 1)):
                last['repeat'] = last.get('repeat', 1) + 1
            else:
                out.append(dict(w))
        if 'writes' in row:
            row['writes'] = out
    return rows


def select_prefixes(rows):
    """One before-read prefix per read site and one before/after-write
    prefix per store site, taken from the cheapest case reaching it."""
    seen, prefixes = set(), []
    order = sorted(rows, key=lambda r: (len(r['writes']), len(r['reads'])))
    for row in order:
        for phase, events in (('before_read', row['reads']), ('before_write', row['writes']),
                              ('after_write', row['writes'])):
            for ordinal, event in enumerate(events):
                key = (phase, event['instruction'])
                if key in seen:
                    continue
                seen.add(key)
                prefix = copy.deepcopy(row)
                for k in ('reads', 'writes', 'executed_sites', 'calls', 'boundary', 'completed'):
                    prefix.pop(k, None)
                for bank in prefix['banks']:
                    bank.pop('after', None)
                prefix['name'] = f"{row['name']}_{phase}_{ordinal}"
                prefix['fault'] = dict(phase=phase, ordinal=ordinal)
                prefix.pop('overrun_control', None)
                prefixes.append(prefix)
    return prefixes


def physical(owned, rows, suffix, batch=40):
    out = []
    for n in range(0, len(rows), batch):
        out += reference.physical(owned, rows[n:n + batch], f'{suffix}-{n}')
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--report', required=True, type=Path)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--select', help='Diagnostic only: regular expression over case names')
    args = parser.parse_args()
    assert not args.output.exists()
    assert reference.sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = reference.metadata(image, ENTRIES)
    owned = reference.prepare(image, meta, scratch)
    selected = inputs(image)
    if args.select:
        import re
        selected = [r for r in selected if re.search(args.select, r['name'])]
        for n, r in enumerate(selected):
            r['id'] = n
    rows = physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'], row['name']
    prefixes = select_prefixes(rows)
    for n, prefix in enumerate(prefixes):
        prefix['id'] = len(rows) + n
    rows += physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    owned_sites = {}
    sites_by_owner = {}
    from generate_physics_live_support_tu15 import complete_body
    from oracle_smg_firing_1_1 import _Code
    code = _Code(image, '.text')
    for entry in ENTRIES:
        body = set(complete_body(code, entry).code)
        sites_by_owner[hex(entry)] = dict(reached=len(body & reached), total=len(body))
        if entry in OWNED:
            owned_sites[hex(entry)] = sorted(hex(s) for s in body - reached)
    missing = {k: v for k, v in owned_sites.items() if v}
    compact_writes(rows)
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=[],
                   bank_encoding='sparse-runs-v1', write_encoding='rep-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original 27B2B0 repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    whole = [r for r in rows if 'fault' not in r]
    report = dict(passed=not missing and not args.select, selection=args.select, original_sha256=NEW_PIN, cases=len(rows), completed=len(whole),
                  prefixes=len(rows) - len(whole),
                  by_entry={hex(e): sum(1 for r in whole if r['entry'] == e) for e in (0x27b2b0, 0x27b340, 0x27bd30)},
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  coverage=sites_by_owner, unreached_owned_sites=missing,
                  original_instructions=sum(row['original_instructions'] for row in rows),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__),
                                  reference.ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'coverage'}))


if __name__ == '__main__':
    main()
