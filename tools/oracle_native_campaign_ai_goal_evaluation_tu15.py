#!/usr/bin/env python3
"""Physical unchanged-i386 reference for the native 26A380 remainder.

26A380's only unported callee on the type-4 path is 2013B0 (the path query,
owned elsewhere), so the remainder is proven in isolation:

* 26A712 continuation cases enter the unchanged original right after 2013B0
  returned, with the actual 26A380 frame (saved registers, prologue locals,
  the query 2013B0 leaves, the eight arguments) installed as stack words.
  The original then runs 26A712..26AF82 with the real 201500, 27B010,
  27B2B0 (over a declared navigation graph), 27A460, 269D70/267A60/2689A0,
  140910/140BC0/269DF0, the 45687C evaluators, 269E70 and 140DB0.
* Direct cases enter 26C5A0, 1209C0, 269D70, 267A60, 2689A0, 269870,
  269DB0, 140910 and 269E70 at their own entries.

Unported callees (2013B0, 2694C0, 2A7D60, 120AF0 and the table callbacks of
other request types) are stop entries: the original halts right after their
CALL, which is where the native refuses by name. No instruction is patched
and no callee outcome is supplied. Scratch stays in HALO2_TEST_SCRATCH.
"""
import argparse
import copy
import gzip
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_perform_tu15 as reference
import oracle_native_campaign_ai_path_search_tu15 as search

OWNED = (0x26a380, 0x269d70, 0x267a60, 0x26c5a0, 0x2689a0, 0x269870, 0x269db0, 0x140910, 0x140bc0,
         0x269df0, 0x269e70, 0x1209c0)
ENTRIES = OWNED + (
    0x140db0, 0x267a30, 0x267a40, 0x0bbe00,
    # 201500 navigation filter and 27B010 workspace.
    0x201500, 0x1ecd00, 0x1ed700, 0x27c6c0, 0x27b010, 0x2013b0, 0x275e90, 0x1d2cf0,
    # 27A460 path result.
    0x27a460, 0x27c410, 0x27c450) + tuple(e for e in search.ENTRIES)
ENTRIES = tuple(dict.fromkeys(ENTRIES))
# Unported callees and the 26A380 branch PCs the native refuses at.
STOPS = (0x2013b0, 0x2694c0, 0x2a7d60, 0x120af0, 0x26a43e, 0x26a5b7, 0x26a62e, 0x26a846,
         0x268bb0, 0x268fa0, 0x268810, 0x268a70, 0x268880, 0x268510, 0x268490, 0x268ae0, 0x268140,
         0x269ca0, 0x269bf0, 0x269a60, 0x269940, 0x269b60)
BASE = reference.areas.BASE
W = search.W
G = BASE + 0x100000
G_SIZE = 0x8000
POOL, ROWS, REQ, CANDS, FIRE = G, G + 0x100, G + 0x1000, G + 0x2000, G + 0x4000
OUT, OWNER_OUT, FLAG, GLOBALS = G + 0x4800, G + 0x4880, G + 0x4890, G + 0x48a0
HASH, HNODES, LCG = G + 0x4900, G + 0x4a00, G + 0x4f00
REG, CHAR, NAVREC, PROPS, PROP_ROWS = G + 0x5000, G + 0x5100, G + 0x5200, G + 0x5400, G + 0x5500
UNIT_BODY, TARGET_BODY, ARRAY, VEC = G + 0x6000, G + 0x6200, G + 0x6400, G + 0x7000
WS_SIZE = 0x14190
NONE = 0xffffffff
ACTOR = 0x7a610000
UNIT, TARGET = 0x12350001, 0x12360002
CONSTANTS = ((0x4500b4, 4), (0x45695c, 4), (0x456910, 4), (0x456818, 0x98), (0x46a0c0, 0x100),
             (0x46a460, 4), (0x46a4c0, 4), (0x46a500, 0x300), (0x44abe0, 0x100), (0x47d6c0, 4))
FP = ((0x27f, 0, 0x1f80), (0x37f, 0x0020, 0x1fa0), (0x07f, 0x3800, 0x9fc0), (0x67f, 0x1000, 0x7f80),
      (0x27f, 0x0820, 0x3f80), (0x47f, 0x0000, 0x5f80))
SP = reference.SP
f = search.f


class Env:
    """Declared banks of one case: the G area, optionally the navigation
    world and the path workspace, the globals and the original constants."""

    def __init__(self, image, world=None):
        self.image = image
        self.g = bytearray(G_SIZE)
        self.world = world
        self.ws = bytearray(WS_SIZE) if world is not None else None
        self.extra = {}
        put = self.put
        put(POOL + 0x44, ROWS)
        put(HASH + 0x20, 16)
        put(HASH + 0x28, 4)
        put(HASH + 0x2c, 0x267a30)
        put(HASH + 0x30, 0x267a40)
        put(LCG, 0x12345678)
        put(REG + 2 * 16 + 8, CHAR)
        put(CHAR + 8, NONE)
        put(CHAR + 0x20, NONE)
        put(CHAR + 0x5c, 1)
        put(CHAR + 0x60, NAVREC)
        put(NAVREC + 0x1a, 7, 2)
        for offset, value in ((0x10, 0x8234), (0x12, 0xffff), (0x14, 0x7123), (0x16, 0x4567)):
            put(NAVREC + offset, value, 2)
        put(PROPS + 0x44, PROP_ROWS)
        self.actor()

    def put(self, address, value, size=4):
        if isinstance(value, float):
            value = f(value)
        raw = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
        if G <= address and address + size <= G + G_SIZE:
            self.g[address - G:address - G + size] = raw
        elif self.ws is not None and W <= address and address + size <= W + WS_SIZE:
            self.ws[address - W:address - W + size] = raw
        elif self.world is not None and search.WORLD <= address and address + size <= search.WORLD + search.WORLD_SIZE:
            self.world.put(address, value, size)
        else:
            raise ValueError(hex(address))

    def actor(self, row=ROWS):
        put = self.put
        put(row + 0x18, UNIT)
        put(row + 0x26c, NONE)
        put(row + 0x30, NONE)
        put(row + 0x54, 0xeeff0002)
        put(row + 0x858, NONE)
        put(row + 0x28c, NONE)
        put(row + 0x300, 0x5150000)
        for k in range(4):
            put(row + 0x400 + k * 6, 0x30 + k, 1)
            put(row + 0x402 + k * 6, (0x7000 + k) << 16 | k)
        for axis, value in enumerate((2.0, 2.0, 0.0)):
            put(row + 0x238 + axis * 4, value)

    def candidate(self, k, xyz, goal, valid=1, score=0.0, cost=None, sector=0, flags=(), kind=0x40, word10=0):
        c = CANDS + k * 0x78
        record = FIRE + k * 0x20
        put = self.put
        put(c, record)
        put(c + 4, goal)
        for axis, value in enumerate(xyz):
            put(c + 0xc + axis * 4, value)
            put(record + axis * 4, value)
        put(record + 0xc, 0xffff, 2)
        put(record + 0xe, kind, 2)
        put(record + 0x10, word10, 2)
        put(record + 0x14, sector)
        put(c + 0x18, cost if cost is not None else 3.0 + k)
        put(c + 0x4c, valid, 1)
        put(c + 0x54, score)
        for offset in flags:
            put(c + offset, 1, 1)
        return c

    def owner(self, goal, value, slot=0):
        node = HNODES + slot * 0x20
        hashed = (goal & 0xff) << 2
        put = self.put
        put(node, goal)
        put(node + 4, hashed)
        put(node + 0xc, value)
        bucket = HASH + 0x3c + (hashed % 16) * 4
        put(node + 8, struct.unpack_from('<I', self.g, bucket - G)[0])
        put(bucket, node)

    def banks(self, frame=None):
        banks = {G: bytes(self.g)}
        if self.world is not None:
            banks[search.WORLD] = bytes(self.world.pack())
            banks[W] = bytes(self.ws)
            banks[0x474c24] = struct.pack('<H', 1)
            banks[0x474d04] = struct.pack('<IIII', 0x44abec, 0, 0, 0x44ac40)
            banks[0x4ecbd0] = struct.pack('<I', search.OBJHDR)
            banks[0x4ecc18] = struct.pack('<II', search.DEVROOT, search.SCEN)
            banks[0x4f9b50] = struct.pack('<I', search.DEVBITS)
            banks[0x505e78] = struct.pack('<I', search.FRAMES)
            banks[0x52be68] = struct.pack('<I', search.GRAVITY)
        for address, size in CONSTANTS:
            banks[address] = self.image.read(address, size)
        for address, value in ((0x5020c8, POOL), (0x56aeb0, HASH), (0x4f3cd8, LCG), (0x4f0414, REG),
                               (0x50eef0, PROPS)):
            banks[address] = struct.pack('<I', value)
        banks[0x52d874] = struct.pack('<HHI', 0x5a5a, 0x1111, 0xdeadbeef)
        banks.update(self.extra)
        return [dict(address=a, before=d.hex()) for a, d in sorted(banks.items())]


def world_env(image, width=3, height=2, links=False):
    """Env with a grid navigation world whose object table also holds the
    actor's unit (row 1) and target (row 2)."""
    world, cell = search.grid(width, height, links=links)
    env = Env(image, world)
    env.cell = cell
    world.put(search.DEVROOT + 0xc4, 1)
    world.put(search.DEVROOT + 0xc8, search.NAV)
    world.put(search.OBJROWS + 12 + 8, UNIT_BODY)
    world.put(search.OBJROWS + 24 + 8, TARGET_BODY)
    for body, origin in ((UNIT_BODY, (2.5, 1.5, 0.25)), (TARGET_BODY, (9.0, 3.0, 0.0))):
        env.put(body + 0x14, NONE)
        for axis, value in enumerate(origin):
            env.put(body + 0x64 + axis * 4, value)
        for axis, value in enumerate((1.0, 0.0, 0.0)):
            env.put(body + 0x70 + axis * 4, value)
    env.ws[0x8c:0x90] = struct.pack('<I', search.NAV)
    return env


def resume_frame(env, request=REQ, count=1, candidate_out=OUT, owner_out=OWNER_OUT, flag=FLAG, actor=ACTOR,
                 start=(2.0, 2.0, 0.0), sector=0, query_extra=(), locals_=None):
    """26A380's frame at 26A712 (ESP = entry - 0x8B0), as stack words."""
    words = [0x5a7d0000 + 4 * n for n in range(0x8b0 // 4)]
    row = ROWS + (actor & 0xffff) * 0x888
    words[0:4] = [0x11110007, 0x22220003, 0x33330006, 0x44440005]
    words[5] = 0x00006b6b
    values = dict(goal=NONE, best=NONE, row=row, offset=(actor & 0xffff) * 0x888, score=0)
    values.update(locals_ or {})
    words[6], words[7], words[8], words[9], words[10] = (values['goal'], values['best'], values['row'],
                                                         values['offset'], values['score'])
    query = [0] * 20
    query[0] = f(0.5)
    for axis, value in enumerate(start):
        query[5 + axis] = f(value)
    query[8] = 0xffff
    query[9] = sector
    for index, value in query_extra:
        query[index] = value
    words[0x44 // 4:0x44 // 4 + 20] = query
    words.append(reference.STOP)
    words += [actor, request, CANDS, count, candidate_out, owner_out, W, flag]
    return words


def request_fields(env, **fields):
    env.put(REQ, fields.pop('type', 4), 2)
    for offset in (0x15, 0x5b, 0x59):
        env.put(REQ + offset, 1, 1)
    env.put(REQ + 0x1c, fields.pop('reach', 6.0))
    env.put(REQ + 0x18, fields.pop('radius', 8.0))
    for name, value in fields.items():
        offset, width = {'flag14': (0x14, 1), 'random': (0x15, 1), 'wide': (0x46, 1), 'range': (0x4c, 4),
                         'spread': (0x48, 4), 'exact': (0x50, 1), 'steer': (0x51, 1), 'alert': (0x56, 1),
                         'r58': (0x58, 1), 'r59': (0x59, 1), 'r5a': (0x5a, 1), 'facing': (0x5b, 1),
                         'front': (0x5c, 1), 'weight': (0x60, 4), 'points': (0x70, 4), 'avoid': (0x618, 1),
                         'target': (0x664, 4), 'kinds': (0x68c, 1), 'kind_a': (0x68e, 2), 'kind_b': (0x690, 2),
                         'prefer': (0x692, 1), 'preferred': (0x694, 4), 'mask': (0x698, 1), 'mask_bits': (0x69a, 2),
                         'type_byte': (0, 1)}[name]
        env.put(REQ + offset, value, width)


class Cases:
    def __init__(self, image):
        self.image, self.rows = image, []

    def add(self, name, env, entry, gp=None, matrix=0, source=0, arguments=(), return_pc=None, fp=None,
            max_steps=100000, depth=0):
        n = len(self.rows)
        regs = [0x5a300000 + n * 41 + k * 17 for k in range(8)]
        regs[4] = SP - (n % 4) * 4
        for k, v in (gp or {}).items():
            regs[k] = v
        profile = FP[n % len(FP)] if fp is None else fp
        row = dict(id=n, name=name, entry=entry, gp=regs, matrix=matrix, source=source,
                   stack_arguments=list(arguments), banks=env.banks(), before_fp=list(profile),
                   live_x87=[struct.pack('<QH', 0x8000000000000000 + (k << 58), 0x3fff + k).hex() for k in range(depth)],
                   xmm=[[0x6b300000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)],
                   stop_before_entries=list(STOPS), max_steps=max_steps, strict_writes=False)
        if return_pc is not None:
            row['return_pc'] = return_pc
        self.rows.append(row)
        return row

    def resume(self, name, env, frame, **kw):
        self.add(name, env, 0x26a712, gp={5: REQ, 4: SP - 0x8b0 - 0x400}, return_pc=frame[0], matrix=frame[1],
                 source=frame[2], arguments=frame[3:], **kw)


def resume_cases(cases, image):
    """26A712 continuation over a 3x2 grid: candidates in the six sectors."""
    def base(n=3, positions=None, **request):
        env = world_env(image)
        request_fields(env, **request)
        cells = sorted(env.cell.values())
        for k in range(n):
            xyz = positions[k] if positions else (2.0 + 4.0 * (k % 3), 2.0 + 4.0 * (k // 3 % 2), 0.0)
            env.candidate(k, xyz, (0x6100 + k) << 16 | k, sector=cells[k % len(cells)], score=0.25 * k)
        return env

    def run(name, env, n=3, **kw):
        cases.resume(name, env, resume_frame(env, count=n, **{k: v for k, v in kw.items() if k in (
            'candidate_out', 'owner_out', 'actor', 'start', 'sector', 'query_extra', 'locals_')}),
            **{k: v for k, v in kw.items() if k in ('fp', 'depth')})

    # Sorted evaluation (the 01a shape) with 1..6 candidates.
    for n in (1, 2, 3, 6):
        run(f'sorted_{n}', base(n), n)
    for profile in FP:
        run(f'sorted_fp_{profile[2]:x}', base(3), 3, fp=profile)
    run('sorted_x87_depth', base(3), 3, depth=2)
    # Random pick: nothing within the request reach.
    for n in (1, 3, 6):
        for state in (0, 0x12345678, 0xfffffff0):
            env = base(n, reach=0.5)
            env.put(LCG, state)
            run(f'random_{n}_{state:x}', env, n)
    env = base(3, reach=0.5)
    env.put(env.cell[0, 0] * 0 + CANDS + 4, 0x61000000)
    run('random_unsorted_goal', env, 3)
    env = base(3, reach=0.5, alert=0)
    env.put(FIRE + 0x14, 0x7fff)
    env.put(FIRE + 0x20 + 0x14, NONE)
    env.put(FIRE + 0x40 + 0x14, 0x7fff)
    run('random_invalid_clusters', env, 3)
    # Path search finds nothing: no navigation start sector in the graph.
    run('search_none', base(3), 3, sector=0x40)
    # Query fields from the actor's perceived target (+358 > 0).
    for perceived in (1, 0x7fff, 0x8000):
        env = base(3)
        env.put(ROWS + 0x358, perceived, 2)
        env.put(ROWS + 0x36c, 1.5)
        for axis, value in enumerate((8.0, 6.0, 0.0)):
            env.put(ROWS + 0x370 + axis * 4, value)
        env.put(ROWS + 0x360, env.cell[2, 1])
        run(f'perceived_{perceived:x}', env, 3)
    # Wide requests read +618: the avoid branch distances and its refusals.
    for target in (NONE, 0x22220001):
        env = base(3, wide=1, avoid=1, target=target, range=2.0, spread=1.0, exact=1)
        env.put(PROP_ROWS + 0x3c + 0x20, 0x5)
        for axis, value in enumerate((6.0, 2.0, 0.0)):
            env.put(REQ + 0x620 + axis * 4, value)
        run(f'avoid_{target:x}', env, 3)
    env = base(3, wide=1)
    run('wide_without_avoid', env, 3)
    # Direct straight-line distances (+58/+5A with candidate flags, +56).
    for request, flags in ((dict(r5a=1), (0x58,)), (dict(r58=1), (0x59,)), (dict(r58=1, steer=1), (0x59,)),
                           (dict(r5a=1, steer=1), (0x58,))):
        for tx in (2.0, 6.0, 30.0):
            env = base(3, **request)
            for axis, value in enumerate((tx, 2.0, 0.0)):
                env.put(REQ + 0x620 + axis * 4, value)
            for k in range(3):
                for offset in flags:
                    env.put(CANDS + k * 0x78 + offset, 1, 1)
            run(f'direct_{"_".join(request)}_{tx}', env, 3)
    env = base(3, r5a=1)
    env.put(CANDS + 0x58, 1, 1)
    env.put(CANDS + 0xc, 2.0)
    env.put(CANDS + 0x10, 2.0)
    run('direct_zero_length', env, 3)
    # 27A460 directions (+51) and the 267A60 target sweep refusal path.
    run('steer', base(3, steer=1), 3)
    # The +5E8 failure count: unit row flags, flag_out and node counts.
    for name, changes in (('target_unset', ((ROWS + 0x28c, 5, 4),)), ('busy', ((ROWS + 0x264, 1, 1),)),
                          ('target_far', ((ROWS + 0x28c, 5, 4), (REQ + 0x1c, 0.5, 4)))):
        env = base(3)
        for address, value, width in changes:
            env.put(address, value, width)
        run(f'failures_{name}', env, 3)
    # Outputs: no candidate/owner outputs, owner hash hits and misses.
    run('no_outputs', base(3), 3, candidate_out=0, owner_out=0)
    for owner in (0x5a5a0001, ACTOR):
        for mixed in (False, True):
            env = base(3)
            for k in range(3):
                env.owner((0x6100 + k) << 16 | k, owner if k != 1 or not mixed else 0x4b4b0000 + k, slot=k)
            run(f'owner_{owner:x}_{int(mixed)}', env, 3)
    # Nothing near and nothing beyond 45695C: candidates whose sectors the
    # search never reaches keep the 27A460 "not found" cost. The +5E8 count.
    for width, height in ((3, 2), (4, 3), (4, 2), (2, 2), (3, 3)):
        for target in (NONE, 5):
            for busy in (0, 1):
                env = world_env(image, width, height)
                request_fields(env, random=0)
                lone = [env.world.sector(1) for _ in range(3)]
                for k in range(3):
                    env.candidate(k, (2.0 + k, 2.5, 0.0), (0x6100 + k) << 16 | k, sector=lone[k], score=0.25 * k, cost=7.0)
                env.put(ROWS + 0x28c, target)
                env.put(ROWS + 0x264, busy, 1)
                run(f'unreached_{width}x{height}_{target:x}_{busy}', env, 3)
    # The +5E8 limit of 8 search nodes (ws+AE): the reach bounds the search.
    for reach in (7.0, 8.0, 9.0, 10.0, 11.0, 12.0, 14.0):
        env = world_env(image, 4, 3)
        request_fields(env, random=0, reach=reach)
        lone = [env.world.sector(1) for _ in range(2)]
        for k in range(2):
            env.candidate(k, (2.0 + k, 2.5, 0.0), (0x6100 + k) << 16 | k, sector=lone[k], cost=7.0)
        env.put(ROWS + 0x28c, 5)
        run(f'unreached_nodes_{reach}', env, 2)
    # Random picks that the scoring admits (no +5B facing filter).
    for n in (1, 3):
        for state in (0, 0x9e3779b9):
            env = base(n, positions=[(3.5 + 4.0 * k, 2.5, 0.0) for k in range(n)], reach=0.5, facing=0)
            env.put(LCG, state)
            run(f'random_admitted_{n}_{state:x}', env, n)
    for size in (0, 2, 7, 9):
        env = base(3)
        env.put(HASH + 0x28, size)
        for k in range(3):
            env.owner((0x6100 + k) << 16 | k, 0x5a5a0000 + k, slot=k)
        run(f'owner_size_{size}', env, 3)
    # Pinned firing-position flag (+3F2) and the chosen candidate's +5B.
    for pinned in (0, 1):
        for flag5b in (0, 1):
            env = base(3)
            env.put(ROWS + 0x3f2, 1, 1)
            env.put(ROWS + 0x220, pinned, 1)
            for k in range(3):
                env.put(CANDS + k * 0x78 + 0x5b, flag5b, 1)
            run(f'pinned_{pinned}_{flag5b}', env, 3)
    # Evaluator bookkeeping: preferred goal (+692) and candidates marked
    # invalid by the goal history; equal scores in the sort.
    env = base(6, prefer=1, preferred=(0x6102 << 16) | 2)
    env.put(ROWS + 0x402, (0x6101 << 16) | 1)
    run('history_and_preference', env, 6)
    env = base(6)
    for k in range(6):
        env.put(CANDS + k * 0x78 + 0x54, 1.0)
    run('equal_scores', env, 6)
    # Straight-line directions with distinct y/z deltas (+51 outputs).
    for request in (dict(r58=1, steer=1), dict(r5a=1, steer=1)):
        env = base(3, positions=[(3.0, 2.5, 0.75), (2.5, 4.0, -1.25), (1.0, 3.0, 2.0)], **request)
        for axis, value in enumerate((5.0, 3.0, 1.5)):
            env.put(REQ + 0x620 + axis * 4, value)
        for k in range(3):
            env.put(CANDS + k * 0x78 + (0x59 if 'r58' in request else 0x58), 1, 1)
        run(f'direct_3d_{"_".join(request)}', env, 3)
    # Exact boundaries: d2 == reach^2, and an initial cost == reach.
    env = base(2, positions=[(4.0, 2.0, 0.0), (2.0, 5.0, 0.0)], reach=2.0)
    run('reach_boundary_radius', env, 2)
    env = base(2, positions=[(12.0, 9.0, 0.0), (11.0, 9.0, 0.0)])
    env.put(CANDS + 0x18, 6.0)
    env.put(CANDS + 0x78 + 0x18, 6.0)
    run('reach_boundary_cost', env, 2)
    # Twin candidates: equal totals (break) and, with the +58 request
    # penalty in +680, equal evaluated scores (no update).
    for penalty in (0, 1):
        env = base(2, positions=[(3.0, 2.5, 0.0), (3.0, 2.5, 0.0)], r58=penalty)
        for k in range(2):
            env.put(FIRE + k * 0x20 + 0x14, env.cell[0, 0])
            env.put(CANDS + k * 0x78 + 0x54, 1.0)
        run(f'twins_{penalty}', env, 2)
    # The +56 path at the tail requires 2013B0 again.
    env = base(3, alert=1)
    run('tail_query', env, 3)


def direct_cases(cases, image):
    """Callees at their own entries over the G area (no navigation)."""
    def env_with(n, **request):
        env = Env(image)
        env.extra[0x4ecbd0] = struct.pack('<I', G + 0x6800)
        env.put(G + 0x6800 + 0x44, G + 0x6900)
        env.put(G + 0x6900 + 12 + 8, UNIT_BODY)
        env.put(G + 0x6900 + 24 + 8, TARGET_BODY)
        env.put(G + 0x6900 + 8, TARGET_BODY)
        for body, origin, parent in ((UNIT_BODY, (2.5, 1.5, 0.25), NONE), (TARGET_BODY, (9.0, 3.0, 0.0), NONE)):
            env.put(body + 0x14, parent)
            for axis, value in enumerate(origin):
                env.put(body + 0x64 + axis * 4, value)
            for axis, value in enumerate((0.8, 0.6, 0.0)):
                env.put(body + 0x70 + axis * 4, value)
        env.extra[0x4ecc18] = struct.pack('<I', G + 0x6c00)
        env.put(G + 0x6c00 + 0xc4, 1)
        env.put(G + 0x6c00 + 0xc8, G + 0x6d00)
        env.put(G + 0x6d00, 4)
        request_fields(env, **request)
        for k in range(n):
            env.candidate(k, (2.0 + 1.5 * k, 1.0 - 0.5 * k, 0.25 * (k % 2)), (0x6100 + k) << 16 | k,
                          sector=k % 6, score=0.5 * (k % 3), cost=1.0 + 2.0 * k, kind=(0x40, 0x60, 0x8040, 0x20)[k % 4],
                          word10=k % 3)
        return env

    # 26C5A0.
    for goal in (NONE, (0x7001 << 16) | 1, (0x7003 << 16) | 3, 5):
        env = env_with(0)
        cases.add(f'history_{goal:x}', env, 0x26c5a0, gp={0: ACTOR}, matrix=goal)
    # 1209C0 over retained vectors.
    for k, (vector, origin, point) in enumerate((((1., 0., 0.), (0., 0., 0.), (0.5, 1., 0.)),
                                                 ((1., 0., 0.), (0., 0., 0.), (-2., 1., 0.)),
                                                 ((1., 0., 0.), (0., 0., 0.), (3., 1., 2.)),
                                                 ((0., 0., 0.), (1., 2., 3.), (4., 6., 3.)),
                                                 ((1e-20, 0., 0.), (1., 2., 3.), (4., 6., 3.)),
                                                 ((2., -1., 0.5), (1., 1., 1.), (2., 0.5, 1.)),
                                                 ((float('nan'), 0., 0.), (0., 0., 0.), (1., 1., 1.)))):
        for profile in (FP[0], FP[2]):
            env = env_with(0)
            for axis in range(3):
                env.put(VEC + axis * 4, vector[axis])
                env.put(VEC + 0x10 + axis * 4, origin[axis])
                env.put(VEC + 0x20 + axis * 4, point[axis])
            cases.add(f'segment_{k}_{profile[2]:x}', env, 0x1209c0, gp={0: VEC, 1: VEC + 0x10, 2: VEC + 0x20}, fp=profile)
    # 2689A0: cost falloff and the +68C kind restriction.
    for radius in (8.0, 0.0, -1.0, float('nan')):
        for kinds in (0, 1):
            for flag14 in (0, 1):
                env = env_with(5, radius=radius, kinds=kinds, kind_a=0x6101, kind_b=1, flag14=flag14)
                env.put(CANDS + 3 * 0x78 + 0x4c, 0, 1)
                cases.add(f'distance_{radius}_{kinds}_{flag14}', env, 0x2689a0, matrix=ACTOR, source=REQ,
                          arguments=(5, CANDS))
    for kinds in ((0x8101 << 16) | 1, (0x6180 << 16) | 1):
        env = env_with(3, kinds=1, kind_a=kinds >> 16, kind_b=0)
        env.put(CANDS + 0x78 + 4, kinds)
        cases.add(f'distance_goal_kind_{kinds >> 16:x}', env, 0x2689a0, matrix=ACTOR, source=REQ, arguments=(3, CANDS))
    for count in (0, 0xffff, 0x10005):
        env = env_with(5, kinds=1, kind_a=0x6100)
        cases.add(f'distance_count_{count:x}', env, 0x2689a0, matrix=ACTOR, source=REQ, arguments=(count, CANDS))
    # 267A60: history, +51 sweep, +70 avoid points, +698/+5A, +5B facing, +692.
    def history(name, n=5, row=None, **request):
        env = env_with(n, **request)
        env.put(ROWS + 0x402 + 6, (0x6101 << 16) | 1)
        for offset, value, width in (row or ()):
            env.put(offset, value, width)
        cases.add(f'scoring_{name}', env, 0x267a60, matrix=ACTOR, source=REQ, arguments=(n, CANDS))
        return env
    history('default')
    history('flag14', flag14=1)
    history('no_facing', facing=0)
    for front in (0, 1):
        for weight in (0.0, 2.0, -1.0):
            history(f'facing_{front}_{weight}', front=front, weight=weight)
    history('facing_target', row=((ROWS + 0x26c, TARGET, 4),))
    history('facing_moving', row=((TARGET_BODY + 0x88, 3.0, 4), (UNIT_BODY + 0x88, 3.0, 4)))
    history('untargeted_5a', row=((CANDS + 0x5a, 1, 1), (CANDS + 0x78 + 0x5a, 1, 1)))
    history('targeted_5a', row=((CANDS + 0x5a, 1, 1), (ROWS + 0x26c, TARGET, 4)))
    history('mask', mask=1, mask_bits=0x60)
    history('mask_all', mask=1, mask_bits=0xffff)
    history('prefer', prefer=1, preferred=(0x6102 << 16) | 2)
    for points in (1, 3):
        env = env_with(5, points=points)
        for k in range(points):
            record = REQ + k * 16
            env.put(record + 0x74, (0.5, 4.0, 1e-10)[k % 3])
            for axis, value in enumerate((3.0 + k, 1.0, 0.0)):
                env.put(record + 0x78 + axis * 4, value)
        cases.add(f'scoring_avoid_{points}', env, 0x267a60, matrix=ACTOR, source=REQ, arguments=(5, CANDS))
    for radius in (-1.0, 0.0):
        env = env_with(3, points=2)
        env.put(REQ + 0x74, radius)
        for axis, value in enumerate((2.0, 1.0, 0.0)):
            env.put(REQ + 0x78 + axis * 4, value)
        env.put(REQ + 0x84, 2.0)
        for axis, value in enumerate((3.5, 0.5, 0.25)):
            env.put(REQ + 0x88 + axis * 4, value)
        cases.add(f'scoring_avoid_radius_{radius}', env, 0x267a60, matrix=ACTOR, source=REQ, arguments=(3, CANDS))
    # +51 target sweep with the actor's target fields.
    for name, fields in (('near', dict(seen=0.0, now=0.0, radius=0.5, start=(3.0, 1.0, 0.0), end=(3.5, 1.0, 0.0))),
                         ('far', dict(seen=0.0, now=0.0, radius=0.5, start=(30.0, 1.0, 0.0), end=(31.0, 1.0, 0.0))),
                         ('short', dict(seen=0.0, now=0.0, radius=0.25, start=(2.0, 1.0, 0.0), end=(2.0, 1.0, 0.0))),
                         ('inside', dict(seen=0.0, now=0.0, radius=5.0, start=(2.0, 1.0, 0.0), end=(4.0, 1.0, 0.0))),
                         ('ring', dict(seen=0.0, now=0.0, radius=0.75, start=(2.0, 1.5, 0.0), end=(6.0, 1.5, 0.0))),
                         ('moving', dict(seen=1.0, now=3.0, radius=0.5, start=(9.0, 9.0, 0.0), end=(10.0, 9.0, 0.0))),
                         ('moving_still', dict(seen=1.0, now=3.0, radius=0.5, start=(9.0, 9.0, 0.0), end=(10.0, 9.0, 0.0),
                                               still=True))):
        for flag14 in (0, 1):
            env = env_with(4, steer=1, flag14=flag14)
            env.put(ROWS + 0x36c, fields['radius'])
            env.put(ROWS + 0x394, fields['now'])
            env.put(ROWS + 0x398, 0.5)
            env.put(ROWS + 0x39c, fields['seen'])
            for axis in range(3):
                env.put(ROWS + 0x370 + axis * 4, fields['start'][axis])
                env.put(ROWS + 0x388 + axis * 4, fields['end'][axis])
                env.put(ROWS + 0x3a0 + axis * 4, (2.5, 1.0, 0.0)[axis])
            for k in range(4):
                for axis, value in enumerate((0.0, 0.0, 0.0) if fields.get('still') else (1.0, 0.5, 0.0)):
                    env.put(CANDS + k * 0x78 + 0x1c + axis * 4, value)
            cases.add(f'scoring_sweep_{name}_{flag14}', env, 0x267a60, matrix=ACTOR, source=REQ, arguments=(4, CANDS))
    for profile in FP:
        history(f'fp_{profile[2]:x}')
    # 269870 visibility evaluator.
    for request in (dict(), dict(alert=1), dict(r5a=1), dict(r58=1), dict(r5a=1, r58=1)):
        for candidate_flags in ((), (0x58,), (0x59,), (0x58, 0x59)):
            for valid in (0, 1):
                for with_candidate in (False, True):
                    if not with_candidate and (candidate_flags or not valid):
                        continue
                    env = env_with(1, **request)
                    env.put(REQ + 0x680, 0.75)
                    env.put(CANDS + 0x4c, valid, 1)
                    for offset in candidate_flags:
                        env.put(CANDS + offset, 1, 1)
                    tag = '_'.join(request) or 'none'
                    cases.add(f'visible_{tag}_{"".join(f"{o:x}" for o in candidate_flags)}_{valid}_{int(with_candidate)}',
                              env, 0x269870, matrix=ACTOR, source=REQ, arguments=(CANDS if with_candidate else 0,))
    # 269DB0 / 269D70 table walks by request type.
    for kind in (4, 0, 1, 3, 5, 7, 8, 0x24, 0x14, 0x30):
        env = env_with(3, type=kind)
        cases.add(f'evaluators_type_{kind:x}', env, 0x269db0, gp={7: REQ, 3: CANDS}, matrix=ACTOR)
        env = env_with(3, type=kind)
        cases.add(f'score_table_type_{kind:x}', env, 0x269d70, gp={7: REQ, 3: CANDS}, matrix=ACTOR, source=3)
    # 140910 with 269DF0 over the candidates at [52D878].
    for n, seed in ((2, 1), (3, 2), (8, 3), (9, 4), (12, 5), (20, 6), (40, 7), (64, 8), (40, 9)):
        env = env_with(0)
        state = seed * 2654435761 & 0xffffffff
        for k in range(n):
            state = (state * 1103515245 + 12345) & 0xffffffff
            c = CANDS + k * 0x78
            env.put(c + 0x4c, (state >> 8) % 4 != 0, 1)
            env.put(c + 0x4d, (state >> 12) % 5 == 0, 1)
            env.put(c + 0x54, [0.0, 1.0, 0.5, -2.0, float('nan'), 1.0, 3.25][(state >> 16) % 7] if seed != 9 else 1.0)
            env.put(ARRAY + k * 4, k if seed % 2 else n - 1 - k)
        env.extra[0x52d874] = struct.pack('<HHI', n, 0, CANDS)
        cases.add(f'sort_{n}_{seed}', env, 0x140910, gp={0: n, 1: ARRAY}, matrix=G + 0x10, source=0x269df0,
                  arguments=(0,))
    for count in (0, 1):
        env = env_with(0)
        env.extra[0x52d874] = struct.pack('<HHI', count, 0, CANDS)
        cases.add(f'sort_trivial_{count}', env, 0x140910, gp={0: count, 1: ARRAY}, matrix=G + 0x10, source=0x269df0,
                  arguments=(0,))
    # 269E70 random-pick admission.
    for name, request, record in (('valid', dict(), 2), ('negative', dict(), NONE), ('beyond', dict(), 4),
                                  ('alert', dict(alert=1), 7), ('avoid', dict(avoid=1), 1),
                                  ('visible', dict(r58=1), 1), ('flag14', dict(flag14=1, prefer=1, preferred=0x61000000), 1)):
        env = env_with(1, **request)
        env.put(FIRE + 0x14, record)
        env.put(CANDS + 0x59, 1, 1)
        cases.add(f'admit_{name}', env, 0x269e70, gp={0: CANDS, 1: REQ, 6: ACTOR})
    env = env_with(1)
    env.extra[0x4ecc18] = struct.pack('<I', G + 0x6c00)
    env.put(G + 0x6c00 + 0xc4, 0)
    cases.add('admit_no_clusters', env, 0x269e70, gp={0: CANDS, 1: REQ, 6: ACTOR})


def inputs(image):
    cases = Cases(image)
    resume_cases(cases, image)
    direct_cases(cases, image)
    # Whole 26A380 entries: 2013B0 constructs the real query consumed by
    # 201500/27B010/27B2B0, rather than installing a continuation result.
    for name, count, fields, flying in (
            ('cached_random', 3, {}, 0), ('cached_scored', 3, dict(random=0), 0),
            ('cached_single', 1, {}, 0), ('flying', 3, {}, 1)):
        for fp in FP:
            env=world_env(image)
            request_fields(env, **fields)
            env.put(ROWS+0x278, 1, 1);env.put(ROWS+0x229, flying, 1)
            env.put(ROWS+0x86, 3, 2);env.put(ROWS+0x328, 3, 2)
            env.put(NAVREC+4, 0.5)
            for axis,value in enumerate((2.0,2.0,0.0)):
                env.put(ROWS+0x27c+axis*4, value)
            env.put(ROWS+0x288, 0xffff);env.put(ROWS+0x28c, 0)
            env.extra[0x5020a8]=struct.pack('<I',GLOBALS)
            for k in range(count):
                env.candidate(k,(2.0+4*k,2.0,0.0),(0x6100+k)<<16|k,
                              sector=sorted(env.cell.values())[k],score=0.25*k)
            row=cases.add(f'whole_query_{name}_{fp[0]:x}_{fp[2]:x}',env,0x26a380,
                          matrix=ACTOR,source=REQ,arguments=(CANDS,count,OUT,OWNER_OUT,W,FLAG),fp=fp)
            row['stop_before_entries']=[e for e in STOPS if e!=0x2013b0]+[0x275cb0,0x275ba0,0xbbf20]
            frame=row['gp'][4]-0x8b0
            # Only externally consumed frame fields are retained: query,
            # navigation filter and sorting indices; private locals remain
            # the existing owner's host values.
            for address,size in ((frame+0x44,0x50),(frame+0x94,0x1c),(frame+0xb0,count*4)):
                row['banks'].append(dict(address=address,before=bytes(size).hex()))
            row['banks'].sort(key=lambda b:b['address'])
    return cases.rows


def select_prefixes(rows, owned_sites):
    """One before-read prefix per owned read site and one before/after-write
    prefix per owned store site, from the cheapest case reaching it."""
    seen, prefixes = set(), []
    for row in sorted(rows, key=lambda r: (len(r['writes']), len(r['reads']))):
        for phase, events in (('before_read', row['reads']), ('before_write', row['writes']),
                              ('after_write', row['writes'])):
            for ordinal, event in enumerate(events):
                key = (phase, event['instruction'])
                if event['instruction'] not in owned_sites or key in seen:
                    continue
                seen.add(key)
                prefix = copy.deepcopy(row)
                for k in ('reads', 'writes', 'executed_sites', 'calls', 'boundary', 'completed', 'stopped_at'):
                    prefix.pop(k, None)
                for bank in prefix['banks']:
                    bank.pop('after', None)
                prefix['name'] = f"{row['name']}_{phase}_{ordinal}"
                prefix['fault'] = dict(phase=phase, ordinal=ordinal)
                prefixes.append(prefix)
    return prefixes


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
    rows = search.physical(owned, selected, 'complete')
    for row in rows:
        assert row['completed'], row['name']
    code = _Code(image, '.text')
    bodies = {entry: set(complete_body(code, entry).code) for entry in ENTRIES}
    owned_sites = set().union(*(bodies[e] for e in OWNED))
    prefixes = select_prefixes(rows, owned_sites)
    for n, prefix in enumerate(prefixes):
        prefix['id'] = len(rows) + n
    rows += search.physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    coverage = {hex(e): dict(reached=len(bodies[e] & reached), total=len(bodies[e])) for e in ENTRIES}
    unreached = {hex(e): sorted(hex(s) for s in bodies[e] - reached) for e in OWNED}
    search.compact_writes(rows)
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=list(STOPS),
                   bank_encoding='sparse-runs-v1', write_encoding='rep-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original 26A380 remainder repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    whole = [r for r in rows if 'fault' not in r]
    stops = {}
    for r in whole:
        if r.get('stopped_at', reference.STOP) != reference.STOP:
            stops[hex(r['stopped_at'])] = stops.get(hex(r['stopped_at']), 0) + 1
    report = dict(passed=not args.select, selection=args.select, original_sha256=NEW_PIN, cases=len(rows),
                  whole=len(whole), prefixes=len(rows) - len(whole), stops=stops,
                  by_entry={hex(e): sum(1 for r in whole if r['entry'] == e) for e in sorted({r['entry'] for r in whole})},
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  coverage=coverage, unreached_owned_sites=unreached,
                  original_instructions=sum(row['original_instructions'] for row in rows),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__), Path(search.__file__),
                                  reference.ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k not in ('coverage', 'unreached_owned_sites')}))
    print(json.dumps({k: f"{v['reached']}/{v['total']}" for k, v in coverage.items() if int(k, 16) in OWNED}))


if __name__ == '__main__':
    main()
