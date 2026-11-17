#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native 26C3C0 and its movement owners.

26C3C0 is 1BBEE0's goal publication (CALL at 1BC054). Each case installs a
declared actor pool, the 26C600 actor/goal hash table, scenario and dynamic
goal definitions, object/frame banks (the path-search world), character tags
and a path workspace, then single-steps the authenticated English TU1.5
closure: 26C3C0, 1FC540, 1FC9B0, 200D30, 200960, 200A10, E9600, 27B0D0 and
the already-native callees 26C510, 26C600 (140E00/140DB0/140D50 with the
267A30/267A40 callbacks), 26C850/277D40, 16E940, 1ECC40/1ECD00 and 218D00
(218B20/218780/219030/145070). Callees without a native owner are declared
stop entries: the original halts right after their CALL pushed the return
word, which is where the native refuses by name. Direct cases enter at
1FC540, 1FC9B0 and 200D30. No instruction is patched and no callee outcome
is supplied. The ELF, observer and raw banks live only in HALO2_TEST_SCRATCH.
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
import oracle_native_campaign_ai_path_search_tu15 as search
import oracle_native_campaign_ai_route_tu15 as route

ENTRIES = (0x2013b0,0x275e90,0x1ed700,0x26c3c0, 0x1fc540, 0x1fc9b0, 0x200d30, 0x200960, 0x200a10, 0xe9600, 0x27b0d0,
           0x26c510, 0x26c600, 0x140e00, 0x140db0, 0x140d50, 0x267a30, 0x267a40, 0x26c850, 0x277d40,
           0x16e940, 0x1ecc40, 0x1ecd00, 0x218d00, 0x218b20, 0x218780, 0x219030, 0x145070,
           # 200D30 after 2013B0 (mid-entry component cases): 201500, the
           # 1D2D90/1D2E30 lease, 27B010 and the whole native 27B2B0 closure.
           0x201500, 0x1d2d90, 0x1d2e30, 0x149680, 0x27b010) + tuple(
               e for e in search.ENTRIES if e not in (0x218b20, 0x218780, 0x219030, 0x145070))
PREFIX_ENTRIES=tuple(dict.fromkeys(ENTRIES+route.PREFIX_ENTRIES))
ENTRIES=tuple(dict.fromkeys(ENTRIES+route.ENTRIES+(0x1ebbd0,0x1fb4f0,0x200a40,0x218eb0,0x218f60)))
OWNED = (0x26c3c0, 0x1fc540, 0x1fc9b0, 0x200d30, 0x200960, 0x200a10, 0xe9600, 0x27b0d0)
# First unported call on each branch (2013B0 belongs to another session).
STOP_ENTRIES = (0x218880,0x26c2a0,0x26c7a0, 0x201840, 0x275fe0, 0x201750, 0xbcfb0, 0x1d2cf0, 0x27a2d0,0x275ba0,0x275cb0)+route.STOP_ENTRIES
BASE = 0x1000000
POOL, ACTORS, TABLE, NODES, SCEN, ZONES, FIRING, DYN_POOL, DYN_ROWS, TAGS, CHAR, FLAGS, PROPS, PROP_ROWS = (
    BASE + n for n in (0, 0x1000, 0x3000, 0x3100, 0x3400, 0x3600, 0x3700, 0x3a00, 0x3b00, 0x4a00, 0x4c00, 0x4e00,
                       0x5000, 0x5100))
SIZE = 0x6000
W = BASE + 0x10000
W2 = BASE + 0x50000  # the second lease descriptor's workspace
WORLD = search.WORLD
UNIT = 0x12340000           # object 0 -> search.BODY
STOP, SP = reference.STOP, reference.SP
NONE = 0xffffffff
ACTOR, OWNER, THIRD = 0xe3610000, 0x7a2b0001, 0x11110002
GLOBAL_CONSTANTS = ((0x47d6c0, 4), (0x46a0c0, 0xd0), (0x46a560, 4), (0x44abe0, 0x100), (0x4500b4, 4),
                    (0x45695c, 4), (0x456918, 4), (0x46a460, 4), (0x46a4c0, 4),(0x46a1f8,4),(0x46a8c8,4),
                    (0x46a8b0,8),(0x46a6e4,4),(0x4502c4,4),(0x46a6a0,8),(0x46a698,8),
                    (0x46a430,8),(0x46a2c0,8),(0x46a838,4),(0x44923c,4))
GLOBAL_CONSTANTS=tuple((a,n) for a,n in GLOBAL_CONSTANTS if not 0x46a000<=a<0x46b000)+((0x46a000,0x1000),)
FP_PROFILES = ((0x27f, 0, 0x1f80), (0x37f, 0x0020, 0x1fa0), (0x07f, 0x3800, 0x9fc0), (0x67f, 0x1000, 0x7f80),
               (0x27f, 0x0820, 0x3f80), (0x47f, 0x0000, 0x5f80), (0x27f, 0, 0x1fc0))


def f(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]


def fv(v):
    return v if isinstance(v, int) else f(v)


class Scene:
    def __init__(self):
        self.raw = bytearray(SIZE)
        self.world,_ = search.grid(3,2)
        self.world.put(search.DEVROOT+0xc4,1);self.world.put(search.DEVROOT+0xc8,search.NAV)
        self.ws = bytearray(search.Workspace(nodes=0,heap=0).raw)
        self.ws.extend(bytes(0x14190-len(self.ws)))
        self.ws[0x120b6:0x140b6]=b'\xff'*0x2000
        # A preexisting search with no admissible nearest node: the route's
        # fallback must not consume an uninitialized parent-chain row.
        self.ws[0x90:0x92]=b'\xff\xff'
        self.ws[0x94:0x98]=struct.pack('<I',0x7f7fffff)
        self.put(POOL + 0x24, 0x888)
        self.put(POOL + 0x38, 3)
        self.put(POOL + 0x44, ACTORS)
        for datum in (ACTOR, OWNER, THIRD):
            self.put(self.row(datum), datum >> 16, 2)
        # 26C600 table: 16 buckets, payload 4, the 267A30/267A40 callbacks,
        # eight free nodes.
        self.put(TABLE + 0x20, 16)
        self.put(TABLE + 0x28, 4)
        self.put(TABLE + 0x2c, 0x267a30)
        self.put(TABLE + 0x30, 0x267a40)
        self.put(TABLE + 0x38, NODES)
        for n in range(8):
            self.put(NODES + n * 16 + 8, NODES + (n + 1) * 16 if n < 7 else 0)
        self.put(SCEN + 0x16c, ZONES)
        self.put(DYN_POOL + 0x44, DYN_ROWS)
        self.put(PROPS + 0x44, PROP_ROWS)
        # Character tags: row 2 -> CHAR, row 3 -> CHAR+100 (the parent).
        for index, pointer in ((2, CHAR), (3, CHAR + 0x100)):
            self.put(TAGS + index * 16 + 8, pointer)
        self.put(CHAR + 8, NONE)
        self.put(CHAR + 0x108, NONE)
        self.character(behavior=0)
        # Unit object 0 (search.BODY): action block at +300, kind at +336.
        self.world.put(search.BODY + 0x346, 0x300, 2)
        self.actor(ACTOR)

    def put(self, address, value, size=4):
        if W <= address < W + len(self.ws):
            self.ws[address - W:address - W + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')
        elif WORLD <= address < WORLD + search.WORLD_SIZE:
            self.world.put(address, value, size)
        else:
            assert BASE <= address and address + size <= BASE + SIZE, hex(address)
            self.raw[address - BASE:address - BASE + size] = (value & ((1 << (8 * size)) - 1)).to_bytes(size, 'little')

    @staticmethod
    def row(datum):
        return ACTORS + (datum & 0xffff) * 0x888

    def actor(self, datum, **fields):
        r = self.row(datum)
        d = dict(active=1, goal=NONE, unit=UNIT, character=2, flying=0, disabled=0, resume=0, path=0,
                 action=4, point=(1.0, 2.0, 3.0, 0xffff), mode270=0, flag5d0=0, history=0, kind=0,
                 position=(4.0, 5.0, 6.0))
        d.update(fields)
        self.put(r + 0x18, d['unit'])
        self.put(r + 0x40, d['active'], 1)
        self.put(r + 0x54, d['character'])
        self.put(r + 0x229, d['flying'], 1)
        for k, v in enumerate(d['position']):
            self.put(r + 0x238 + k * 4, fv(v))
            self.put(r + 0x27c + k * 4, fv(v))
        self.put(r+0x278,1,1);self.put(r+0x288,0xffff);self.put(r+0x28c,0)
        self.put(r+0x26c,NONE);self.put(r+0x30,NONE);self.put(r+0x858,NONE)
        self.put(r + 0x267, d['disabled'], 1)
        self.put(r + 0x270, d['mode270'], 2)
        self.put(r + 0x3fe, d['history'], 2)
        for n in range(4):
            self.put(r + 0x400 + n * 6, 0x30 + n, 1)
            self.put(r + 0x402 + n * 6, (0x7000 + n) << 16 | n)
        self.put(r + 0x418, d['goal'])
        self.put(r + 0x480, d['resume'], 1)
        self.put(r + 0x4ac, d['action'], 2)
        for k, v in enumerate(d['point'][:3]):
            self.put(r + 0x4b8 + k * 4, fv(v))
        self.put(r + 0x4c4, d['point'][3])
        self.put(r + 0x504, d['path'], 2)
        self.put(r + 0x5d0, d['flag5d0'], 1)
        if 'kind' in fields:
            self.world.put(search.BODY + 0x336, d['kind'], 2)
        # Nonzero residue in every owned output field.
        for offset, size in ((0x3f0, 1), (0x3f1, 1), (0x4ae, 1), (0x4b0, 4), (0x4b4, 4), (0x4c8, 4), (0x4cc, 4),
                             (0x4d0, 4), (0x4d4, 1), (0x4d5, 1), (0x4e4, 4), (0x4e8, 1), (0x4ec, 4), (0x4f0, 4),
                             (0x4f4, 4), (0x4f8, 4), (0x4fc, 4), (0x500, 4), (0x506, 1), (0x508, 2), (0x50c, 1),
                             (0x5ac, 4), (0x5b0, 2), (0x5b4, 2), (0x5b6, 2), (0x5d2, 1), (0x5e8, 2), (0x656, 2)):
            if offset not in fields.get('keep', ()):
                self.put(r + offset, 0x5a5a5a5a & ((1 << (8 * size)) - 1), size)
        # The retry policy is consumed once the real query reaches 27B010.
        # Keep the explicitly varied retry count, with a valid empty policy
        # for its formerly unconsumed node/history fields.
        self.put(r+0x5b6,0,2)
        for offset in range(0x5b8,0x5d0,4):self.put(r+offset,0)
        # Mode 6 may keep its existing point before the newly bound query.
        # Give that input a real world-space frame, preserving upper residue.
        # Explicit framed-point cases below retain their authored overrides.
        if 0x4f8 not in fields.get('keep',()):self.put(r+0x4f8,0x5a5affff)
        for offset, value in fields.get('raw', {}).items():
            size = value[1] if isinstance(value, tuple) else 4
            self.put(r + offset, value[0] if isinstance(value, tuple) else value, size)

    def zone(self, zone, positions):
        """Scenario goal (zone<<16 | index) definitions: (xyz, frame, kind, word14)."""
        z = ZONES + zone * 0x38
        base = FIRING + zone * 0x100
        self.put(z + 0x28, len(positions))
        self.put(z + 0x2c, base)
        for k, (xyz, frame, kind, word14) in enumerate(positions):
            for axis, value in enumerate(xyz):
                self.put(base + k * 32 + axis * 4, fv(value))
            self.put(base + k * 32 + 0xc, frame)
            self.put(base + k * 32 + 0xe, kind, 2)
            self.put(base + k * 32 + 0x14, word14)

    def dynamic(self, owner, positions, mask=None):
        rec = DYN_ROWS + owner * 0x484
        self.put(rec + 8, len(positions), 2)
        self.put(rec + 0x40, (1 << len(positions)) - 1 if mask is None else mask)
        for k, (xyz, frame, kind, word14) in enumerate(positions):
            for axis, value in enumerate(xyz):
                self.put(rec + 0x44 + k * 32 + axis * 4, fv(value))
            self.put(rec + 0x44 + k * 32 + 0xc, frame)
            self.put(rec + 0x44 + k * 32 + 0xe, kind, 2)
            self.put(rec + 0x44 + k * 32 + 0x14, word14)

    def assign(self, goal, actor, node=None):
        """Pre-associate goal -> actor in the 26C600 table (from the free list)."""
        head = int.from_bytes(self.raw[TABLE + 0x38 - BASE:TABLE + 0x3c - BASE], 'little')
        nxt = int.from_bytes(self.raw[head + 8 - BASE:head + 12 - BASE], 'little')
        self.put(TABLE + 0x38, nxt)
        hashed = (goal & 0xff) * 4
        slot = TABLE + 0x3c + (hashed % 16) * 4
        self.put(head, goal)
        self.put(head + 4, hashed)
        self.put(head + 0xc, actor)
        self.put(head + 8, int.from_bytes(self.raw[slot - BASE:slot + 4 - BASE], 'little'))
        self.put(slot, head)

    def character(self, vision=None, behavior=None, parent=False):
        owner = CHAR + (0x100 if parent else 0)
        if parent:
            self.put(CHAR + 8, 3)
        for c in (CHAR, CHAR + 0x100):
            self.put(c + 0x34, 1 if vision is not None and c == owner else 0)
            self.put(c + 0x38, FLAGS + 4)
            self.put(c + 0x5c, 1 if behavior is not None and c == owner else 0)
            self.put(c + 0x60, FLAGS)
        self.put(FLAGS, behavior or 0)
        self.put(FLAGS + 4, vision or 0)

    def workspace(self, seven=(3, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66)):
        for n, v in enumerate(seven):
            self.put(W + 0x70 + n * 4, v)
        return W

    def banks(self, image, constants=None):
        # Real empty spatial cluster for the newly native route smoothing child.
        self.put(BASE+0x5700,NONE);self.put(BASE+0x5800+0x8c,0)
        self.world.put(search.DEVROOT+0xa0,BASE+0x5800)
        self.world.put(search.BODY+0x28,0);self.world.put(search.BODY+0x2c,0)
        self.world.put(search.BODY+0xaa,0,1)
        self.world.put(search.NAV+8,len(self.world.edges))
        banks = {BASE: bytes(self.raw), W: bytes(self.ws), WORLD: bytes(self.world.pack())}
        for address, size in GLOBAL_CONSTANTS:
            data = bytearray(image.read(address, size))
            for a, v in (constants or {}).items():
                if address <= a < address + size:
                    data[a - address:a - address + 4] = struct.pack('<I', v)
            banks[address] = bytes(data)
        for address, value in ((0x5020c8, POOL), (0x56aeb0, TABLE), (0x4ecc20, SCEN), (0x52d87c, DYN_POOL),
                               (0x4ecbd0, search.OBJHDR), (0x4f0414, TAGS), (0x505e78, search.FRAMES),
                               (0x50eef0, PROPS)):
            banks[address] = struct.pack('<I', value)
        for address,value in ((0x474cdc,0x44abec),(0x48d4f4,1)):banks[address]=struct.pack("<I",value)
        spatial=bytearray(0x100);spatial[0xb0:0xb4]=struct.pack('<I',BASE+0x5700);spatial[0xcc:0xd0]=struct.pack('<I',5)
        traversal=bytearray(0x100);traversal[0xe4:0xe8]=struct.pack('<I',8)
        banks[0x4eab00]=bytes(spatial);banks[0x4f3c00]=bytes(traversal)
        banks[W2]=bytes(self.ws)
        for address,data in ((0x52be54,struct.pack('<I',0)),(0x52be58,b'\x01'),(0x486070,struct.pack('<I',0)),
            (0x51df44,struct.pack('<I',0x1150000)),(0x51df48,b'\x33'),
            (0x50208c,struct.pack('<IIIIII',0,W,0x22974,0,W2,0x22974)),
            (0x474c24,struct.pack('<H',1)),(0x474d04,struct.pack('<IIII',0x44abec,0,0,0x44ac40)),
            (0x4ecc18,struct.pack('<II',search.DEVROOT,search.SCEN)),(0x4f9b50,struct.pack('<I',search.DEVBITS)),
            (0x52be68,struct.pack('<I',search.GRAVITY))):banks[address]=data
        return banks


def make(image, rows, name, entry, scene, args, *, fp=None, constants=None, max_steps=100000, stack_history=False):
    n = len(rows)
    banks = scene.banks(image, constants)
    sp = SP - (n % 4) * 4
    if stack_history:
        # 1FC540's local framed point [ESP-10, ESP): its frame dword's upper
        # word is never written and is copied whole into +4C4.
        banks[sp - 0x10] = bytes(((sp + k) * 0x9d + n * 0x3b + 0x5a) & 0xff for k in range(0x10))
    gp = [0x5a300000 + n * 41 + k * 11 for k in range(8)]
    gp[4] = sp
    profile = FP_PROFILES[n % len(FP_PROFILES)] if fp is None else fp
    rows.append(dict(id=n, name=name, entry=entry, gp=gp, matrix=args[0], source=args[1],
                     stack_arguments=list(args[2:]),
                     banks=[dict(address=a, before=d.hex()) for a, d in sorted(banks.items())],
                     before_fp=list(profile), live_x87=[],
                     xmm=[[0x6b300000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)],
                     stop_before_entries=list(STOP_ENTRIES), max_steps=max_steps, strict_writes=False))
    return rows[-1]


GOAL4 = (0 << 16) | 0      # zone 0 index 0: kind 0x40 -> mode 4
GOAL5 = (0 << 16) | 1      # kind 0 -> mode 5
GOAL6 = (0 << 16) | 2      # kind 0x20 -> mode 6
GOAL_MISSING = (0 << 16) | 9
GOAL_DYN = 0x80000000 | (1 << 16) | 0


def standard_zone(scene):
    scene.zone(0, [((7.0, 8.0, 9.0), 0xffff, 0x40, 0xffffffff), ((1.5, -2.5, 0.5), 0x0003ffff, 0x0, 0x1234),
                   ((-3.0, 4.0, 1.0), 0xffff, 0x20, 0x5678), ((0.0, 0.0, 0.0), 0xffff, 0x60, 0xffffffff)])
    scene.zone(1, [((2.0, 2.0, 2.0), 0xffff, 0x40, 7)])
    scene.dynamic(1, [((9.0, -9.0, 0.25), 0xffff, 0x40, 0x99), ((1.0, 1.0, 1.0), 0xffff, 0x0, 0xffffffff)])


def publication_cases(image, rows):
    """26C3C0 composed cases."""
    def case(name, actor=None, goal=GOAL4, owner=NONE, lease=0, found=0, begin=1, setup=None, **kw):
        s = Scene()
        standard_zone(s)
        s.actor(ACTOR, **(actor or {}))
        s.actor(OWNER, **(dict(goal=NONE)))
        ws = s.workspace()
        if setup:
            setup(s)
        make(image, rows, name, 0x26c3c0, s, (ACTOR, goal, owner, ws if lease else 0, found, begin), **kw)

    # NONE goal: 1FC540 reset variants, then 26C600(actor, NONE).
    for mode270, flag, flying, action, c8 in ((0, 0, 0, 4, NONE), (4, 0, 0, 1, NONE), (4, 1, 0, 4, NONE),
                                              (4, 1, 1, 4, NONE), (4, 1, 1, 2, NONE), (4, 1, 1, 2, 5)):
        case(f'none_goal_{mode270}_{flag}_{flying}_{action}_{c8 & 0xff:x}', goal=NONE,
             actor=dict(goal=GOAL5, mode270=mode270, flag5d0=flag, flying=flying, action=action,
                        raw={0x4c8: c8}, keep=(0x4c8,)),
             setup=lambda s: s.assign(GOAL5, ACTOR))
    # Current goal handling (26C510 history ring with flag 1), owners.
    for current in (NONE, GOAL4, GOAL5, 0x7001_0001):
        for history in (0, 3):
            case(f'current_{current:x}_{history}', actor=dict(goal=current, history=history),
                 setup=(lambda s, c=current: s.assign(c, ACTOR)) if current != NONE else None)
    for owner in (OWNER, OWNER | 0x10000, 0x7a2b0005, THIRD):
        case(f'owner_{owner:x}', owner=owner, setup=lambda s: s.assign(GOAL4, OWNER))
    case('owner_salt_zero', owner=0x00000001, setup=lambda s: s.put(Scene.row(OWNER), 0, 2))
    # Begin byte clear: no 1FC9B0.
    for found in (0, 1):
        case(f'no_begin_found{found}', found=found, begin=0)
    case('no_begin_same_goal', actor=dict(goal=GOAL4), begin=0, setup=lambda s: s.assign(GOAL4, ACTOR))
    # Definitions select modes 4/5/6 (or none); 1FC9B0 same/changed; 200D30.
    for goal in (GOAL4, GOAL5, GOAL6, GOAL_MISSING, GOAL_DYN, 0x80000000 | (1 << 16) | 1,
                 0x80000000 | (1 << 16) | 5, (1 << 16) | 0, (0 << 16) | 3):
        for found, lease in ((0, 0), (1, 1), (0, 1)):
            case(f'begin_{goal:x}_f{found}_l{lease}', goal=goal, found=found, lease=lease)
    # Same mode and goal: 1FC9B0 resumes through 200D30 (or returns 1).
    # GOAL4 has sector NONE (+14), so 200D30 fails its route gate; zone 1's
    # mode-4 position has sector 7 and reaches the route owners.
    for goal in (GOAL4, (1 << 16) | 0):
        for active, resumed, count in ((1, 0, 0), (0, 0, 0), (1, 1, 0), (1, 0, 3), (1, 0, 0xffff), (1, 0, 0x7fff)):
            for lease in (0, 1):
                case(f'same_{goal:x}_{active}_{resumed}_{count:x}_l{lease}', goal=goal, found=1, lease=lease,
                     actor=dict(goal=goal, active=active, action=4, raw={0x4b8: goal, 0x506: (resumed, 1), 0x5b4: (count, 2)},
                                keep=(0x4b8, 0x506, 0x5b4)),
                     setup=lambda s, g=goal: s.assign(g, ACTOR))
    # 200D30 gates reached through publication.
    for label, fields in (('disabled', dict(disabled=1)), ('resume', dict(resume=1)), ('path2', dict(path=2)),
                          ('flying', dict(flying=1)), ('kind5', dict(kind=5))):
        for lease in (0, 1):
            case(f'update_{label}_l{lease}', found=1, lease=lease, actor=fields)
    for vision, behavior, parent in ((0, 0x20, False), (2, 0x20, False), (0, 0, False), (None, 0x20, False),
                                     (0, None, False), (0, 0x20, True), (0x10, 0x30, False)):
        case(f'kind5_v{vision}_b{behavior}_p{int(parent)}', found=1, lease=1, actor=dict(kind=5),
             setup=lambda s, v=vision, b=behavior, p=parent: s.character(v, b, p))
    # Path mode 1 (mode 5): +26C NONE selects the unit at 2011A2.
    for target in (NONE, 0x55550001):
        for restriction in (NONE, 0x00030004):
            case(f'flying_target_{target:x}_{restriction:x}', goal=GOAL5, found=1, lease=1,
                 actor=dict(raw={0x26c: target, 0x4e4: restriction}, keep=(0x4e4,)))
    # Mode 6 behavior bits (1ECD00).
    for behavior, parent in ((0x10, False), (0x30, False), (0, False), (None, False), (0x10, True), (0x50, False)):
        for flying in (0, 1):
            case(f'mode6_b{behavior}_p{int(parent)}_fly{flying}', goal=GOAL6, found=1, lease=1,
                 actor=dict(flying=flying), setup=lambda s, b=behavior, p=parent: s.character(None, b, p))
    # Scalar gate at 2010C7: mode 6 without behavior bit 10 keeps the live
    # +500/+4FC (UCOMISS under the caller's MXCSR).
    for label, value in (('zero', 0.0), ('negzero', 0x80000000), ('one', 1.0), ('nan', 0x7fc00000),
                         ('snan', 0x7f800001), ('denormal', 1)):
        for frame in (NONE, 0x00040000):
            for mxcsr in (0x1f80, 0x1fc0, 0x9fc0):
                case(f'scalar_{label}_{frame & 0xffff:x}_{mxcsr:x}', goal=GOAL6, found=1, lease=1, fp=(0x27f, 0, mxcsr),
                     actor=dict(raw={0x500: fv(value), 0x4fc: frame}, keep=(0x500, 0x4fc)),
                     setup=lambda s: s.character(None, 0x40))
    for mxcsr in (0x1f80, 0x1fc0):
        for profile in range(3):
            case(f'fp_profile_{mxcsr:x}_{profile}', goal=GOAL4, found=profile & 1, lease=1,
                 fp=(FP_PROFILES[profile][0], FP_PROFILES[profile][1], mxcsr | (FP_PROFILES[profile][2] & 0x8040)))


def update_cases(image, rows):
    """Direct 200D30 (modes 2/3/7, out of range, the 2010C7 gate)."""
    def case(name, actor=None, lease=1, setup=None, **kw):
        s = Scene()
        standard_zone(s)
        s.actor(ACTOR, **(actor or {}))
        ws = s.workspace()
        if setup:
            setup(s)
        make(image, rows, name, 0x200d30, s, (ACTOR, ws if lease else 0), **kw)

    for action in (0, 1, 2, 3, 4, 5, 6, 7, 8, 0xffff, 0x8000):
        for lease in (0, 1):
            goal = {4: GOAL4, 5: GOAL5, 6: GOAL6, 7: 0x00020003}.get(action, f(1.0))
            case(f'direct_action{action:x}_l{lease}', lease=lease,
                 actor=dict(action=action, raw={0x4c8: 0x00050006, 0x4b8: goal}, keep=(0x4c8, 0x4b8)))
    for frame in (NONE, 0x0003ffff):
        for scalar in (0.0, 2.5, 0x7fc00000, 1):
            for mxcsr in (0x1f80, 0x1fc0):
                case(f'direct_mode2_{frame & 0xffff:x}_{scalar}_{mxcsr:x}', fp=(0x27f, 0, mxcsr),
                     actor=dict(action=2, raw={0x4c8: frame, 0x500: fv(scalar)}, keep=(0x4c8, 0x500)))
    for flying in (0, 1):
        case(f'direct_mode3_fly{flying}', actor=dict(action=3, flying=flying))
        case(f'direct_mode7_fly{flying}', actor=dict(action=7, flying=flying, raw={0x4b8: 0x00020003}, keep=(0x4b8,)))
    for path, resume, disabled in ((2, 0, 0), (2, 1, 0), (0, 1, 0), (2, 0, 1)):
        # Once the real resume/arrival child returns false, mode4 consumes
        # its goal ID. The old float point bits were an invalid zone index.
        raw={0x4b8:NONE} if (path,resume,disabled)==(2,0,0) else {}
        case(f'direct_gate_{path}_{resume}_{disabled}', actor=dict(path=path, resume=resume, disabled=disabled,raw=raw))


def begin_cases(image, rows):
    """Direct 1FC9B0 and 1FC540."""
    for mode in (4, 5, 6, 2, 0xffff):
        for same in (False, True):
            for lease in (0, 1):
                for flag in (0, 1, 0x80):
                    s = Scene()
                    standard_zone(s)
                    goal = {4: GOAL4, 5: GOAL5, 6: GOAL6}.get(mode, GOAL4)
                    s.actor(ACTOR, action=mode if same else 4 + (mode == 4),
                            raw={0x4b8: goal, 0x5b4: (0, 2)}, keep=(0x4b8, 0x5b4))
                    ws = s.workspace()
                    make(image, rows, f'begin_{mode:x}_{int(same)}_l{lease}_f{flag:x}', 0x1fc9b0, s,
                         (ACTOR, mode, goal, ws if lease else 0, flag))
    for mode270, flag, flying, action, c8, position, frame in (
            (0, 0, 0, 4, NONE, (4.0, 5.0, 6.0), 0xffff), (4, 0, 1, 2, NONE, (4.0, 5.0, 6.0), 0xffff),
            (4, 1, 0, 2, NONE, (4.0, 5.0, 6.0), 0xffff), (4, 1, 1, 3, NONE, (4.0, 5.0, 6.0), 0xffff),
            (4, 1, 1, 2, 7, (4.0, 5.0, 6.0), 0xffff), (4, 1, 1, 2, NONE, (1.0, 2.0, 3.0), 0xffff),
            (4, 1, 1, 2, NONE, (1.05, 2.0, 3.0), 0xffff), (4, 1, 1, 2, NONE, (1.0, 2.0, 3.1), 0xffff),
            (4, 1, 1, 2, NONE, (1.0, 2.0, 3.0), 0x0000), (4, 1, 1, 2, NONE, (0x7fc00000, 2.0, 3.0), 0xffff),
            (4, 1, 1, 2, NONE, (1.0, 2.0, 3.0), 0x00010000)):
        for resumed, resume, disabled in ((0, 0, 0), (1, 0, 0), (0, 1, 0), (0, 0, 1)):
            if (resume or disabled) and not flying:
                continue
            s = Scene()
            standard_zone(s)
            s.actor(ACTOR, mode270=mode270, flag5d0=flag, flying=flying, action=action, position=position,
                    point=(1.0, 2.0, 3.0, frame), resume=resume, disabled=disabled,
                    raw={0x4c8: c8, 0x506: (resumed, 1)}, keep=(0x4c8, 0x506))
            s.assign(GOAL4, ACTOR)
            s.put(Scene.row(ACTOR) + 0x418, GOAL4)
            make(image, rows, f'reset_{mode270}_{flag}_{flying}_{action}_{c8 & 0xff:x}_{position[0]}_{frame:x}_r{resumed}{resume}{disabled}',
                 0x1fc540, s, (ACTOR, 0x5eed0000 + len(rows)), stack_history=True)


LEASE_GLOBALS = ((0x52be54, 4), (0x52be58, 1), (0x486070, 4), (0x51df44, 4), (0x51df48, 1), (0x50208c, 0x18))
SEARCH_GLOBALS = ((0x474c24, 2), (0x474d04, 16), (0x4ecc18, 8), (0x4f9b50, 4), (0x52be68, 4))


def make_mid(image, rows, name, entry, scene, *, frame_words, regs, workspace=None, used=(0, 0), fp=None, second=None):
    """200D30 resumed at `entry` from a declared post-call state: ESP is its
    frame F ([F] saved EDI .. [F+C] saved EBX, locals, [F+90] return,
    [F+94] actor, [F+98] workspace argument), EBP the actor row."""
    n = len(rows)
    banks = scene.banks(image)
    if workspace is not None:
        banks[W] = bytes(workspace)
        if used[0]:
            banks[W2] = bytes(workspace)
    if second is not None:
        banks[W2] = second
    lease = bytes([used[0], 0, 0, 0]) + struct.pack('<II', W, 0x22974) + bytes([used[1], 0, 0, 0]) + struct.pack('<II', W2, 0x22974)
    for address, data in ((0x52be54, struct.pack('<I', 0)), (0x52be58, b'\x01'), (0x486070, struct.pack('<I', 0)),
                          (0x51df44, struct.pack('<I', 0x1150000)), (0x51df48, b'\x33'), (0x50208c, lease),
                          (0x474c24, struct.pack('<H', 1)), (0x474d04, struct.pack('<IIII', 0x44abec, 0, 0, 0x44ac40)),
                          (0x4ecc18, struct.pack('<II', search.DEVROOT, search.SCEN)), (0x4f9b50, struct.pack('<I', search.DEVBITS)),
                          (0x52be68, struct.pack('<I', search.GRAVITY))):
        banks[address] = data
    frame = SP - 0x100 - (n % 4) * 4
    words = [0x5f000000 + n * 64 + k for k in range(0x9c // 4)]
    for offset, value in frame_words.items():
        words[offset // 4] = value
    words[0x90 // 4] = STOP
    gp = [0x5a400000 + n * 43 + k * 13 for k in range(8)]
    gp[4] = frame
    gp[5] = Scene.row(ACTOR)
    for k, v in regs.items():
        gp[k] = v
    profile = FP_PROFILES[n % len(FP_PROFILES)] if fp is None else fp
    rows.append(dict(id=n, name=name, entry=entry, gp=gp, return_pc=words[0], matrix=words[1], source=words[2],
                     stack_arguments=words[3:],
                     banks=[dict(address=a, before=d.hex()) for a, d in sorted(banks.items())],
                     before_fp=list(profile), live_x87=[],
                     xmm=[[0x6b400000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)],
                     stop_before_entries=list(STOP_ENTRIES), max_steps=100000, strict_writes=False))


def search_scene(goal_sector, reachable=True, threshold=0.25):
    """A corridor navigation world (search.grid) and a goal for 27B0D0."""
    s = Scene()
    standard_zone(s)
    world, cell = search.grid(3, 1)
    s.world = world
    world.put(search.DEVROOT + 0xc4, 1)
    world.put(search.DEVROOT + 0xc8, search.NAV)
    if not reachable:
        lone = world.sector(0)
        goal_sector = lone
    return s, cell, goal_sector


def query_words(start=(2.0, 2.0, 0.0), sector=0, clearance=0.5):
    """The 20-dword main navigation record 2013B0 leaves at F+40."""
    words = {0x40 + k * 4: 0 for k in range(20)}
    words[0x40] = f(clearance)
    for k, v in enumerate(start):
        words[0x40 + 0x14 + k * 4] = f(v)
    words[0x40 + 0x20] = 0xffff
    words[0x40 + 0x24] = sector
    return words


def mid_cases(image, rows):
    # 20124A: 2013B0 has returned; the native 201500/1D2D90/27B010/27B0D0/
    # 27B2B0 composition runs, then 27A640 (found) or the not-found tail.
    for label, reachable, extra in (('found', True, {}), ('unreachable', False, {}),
                                    ('restriction', True, {0x4e4: 0x00020001}),
                                    ('perceived', True, {0x358: (1, 2), 0x35e: (0, 1)}),
                                    ('perceived_seen', False, {0x358: (1, 2), 0x35e: (1, 1)}),
                                    ('perceived_negative', False, {0x358: (0x8000, 2), 0x35e: (0, 1)})):
        for used in ((0, 0), (1, 0)):
            s, cell, goal_sector = search_scene(1, reachable)
            raw = {0x4ec: f(6.0), 0x4f0: f(2.0), 0x4f4: 0, 0x4f8: 0xffff, 0x4fc: goal_sector if reachable else 3,
                   0x500: f(0.25), 0x4e4: NONE, 0x5b4: (0, 2),
                   # 201500: no vehicle (+858) or squad (+30) restriction, +266 clear.
                   0x858: NONE, 0x30: NONE, 0x266: (0, 1)}
            raw.update(extra)
            s.actor(ACTOR, raw=raw, keep=tuple(raw))
            ws = bytearray(0x14190)
            for k in range(0, 0xb0, 4):
                ws[k:k + 4] = struct.pack('<I', 0x6c000000 + k)
            words = query_words()
            words[0x10] = 0x00000000
            words[0x14] = NONE
            words[0x94] = ACTOR
            words[0x98] = 0
            make_mid(image, rows, f'after_query_{label}_u{used[0]}', 0x20124a, s, frame_words=words,
                     regs={3: 0, 6: ACTOR, 7: NONE}, workspace=ws, used=used)
    # 2012DF: 27B2B0 returned AL for the lease in EDI.
    for found in (0, 1, 0x100):
        for used in ((1, 0), (0, 1), (1, 1)):
            s = Scene()
            standard_zone(s)
            s.actor(ACTOR, raw={0x5b4: (7, 2)}, keep=(0x5b4,))
            ws = s.workspace()
            lease = W if used != (0, 1) else W2
            authored,world,_=route.scene(length=3)
            s.world=world
            positive=bytearray(authored.raw);positive.extend(bytes(0x14190-len(positive)))
            if lease == W2:
                ws = W2
                extra = positive
            make_mid(image, rows, f'after_search_{found:x}_u{used[0]}{used[1]}', 0x2012df, s,
                     frame_words={0x10: 0, 0x14: lease, 0x94: ACTOR, 0x98: 0},
                     regs={0: 0x77777700 | found, 3: BASE + 0x24, 6: ACTOR, 7: lease}, used=used,workspace=positive,
                     second=bytes(extra) if lease == W2 else None)
    # 201342: the route owner's result byte at F+12.
    for result in (0, 1, 0x80):
        for failures in (0, 0xffff):
            s = Scene()
            standard_zone(s)
            s.actor(ACTOR, raw={0x508: (failures, 2)}, keep=(0x508,))
            make_mid(image, rows, f'conclude_{result:x}_{failures:x}', 0x201342, s,
                     frame_words={0x10: result << 16, 0x94: ACTOR, 0x98: 0}, regs={6: ACTOR})


def inputs(image):
    rows = []
    publication_cases(image, rows)
    update_cases(image, rows)
    begin_cases(image, rows)
    mid_cases(image, rows)
    return rows


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
    for site in meta['sites']:
        if site['bytes'].startswith('0f17'):site['operands'][0].update(read=False,write=True)
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
        if 0x2cabe0 in row['executed_sites']:row.update(compare_reads=False,semantic_child_stores=True)
    from generate_physics_live_support_tu15 import complete_body
    from oracle_smg_firing_1_1 import _Code
    code=_Code(image,'.text');prefix_sites=set().union(*(set(complete_body(code,e).code) for e in PREFIX_ENTRIES))
    originals={r['name']:r for r in rows};prefixes=[]
    for prefix in search.select_prefixes(rows):
        phase=prefix['fault']['phase'];original=originals[prefix['name'].rsplit('_'+phase+'_',1)[0]]
        if phase=='before_read' and not original.get('compare_reads',True):continue
        events=original['reads' if phase=='before_read' else 'writes']
        if events[prefix['fault']['ordinal']]['instruction'] in prefix_sites:prefixes.append(prefix)
    for n, prefix in enumerate(prefixes):
        prefix['id'] = len(rows) + n
    rows += search.physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(row['executed_sites']) for row in rows))
    from generate_physics_live_support_tu15 import complete_body
    from oracle_smg_firing_1_1 import _Code
    code = _Code(image, '.text')
    sites_by_owner, unreached = {}, {}
    for entry in ENTRIES:
        body = set(complete_body(code, entry).code)
        sites_by_owner[hex(entry)] = dict(reached=len(body & reached), total=len(body))
        if entry in OWNED:
            unreached[hex(entry)] = sorted(hex(s) for s in body - reached)
    stops = {}
    for row in rows:
        if 'fault' not in row and row.get('stopped_at', STOP) != STOP:
            stops[hex(row['stopped_at'])] = stops.get(hex(row['stopped_at']), 0) + 1
    # MOVS sites: the observer checks the destination operand before the
    # source, so a before-write boundary there records no source read.
    movs_sites = sorted(site['instruction'] for site in meta['sites'] if site['bytes'] in ('a4', 'a5', 'f3a4', 'f3a5'))
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=list(STOP_ENTRIES),
                   movs_sites=movs_sites,
                   bank_encoding='sparse-runs-v1', write_encoding='rep-runs-v1', cases=reference.compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original 26C3C0 repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    whole = [r for r in rows if 'fault' not in r]
    report = dict(passed=not args.select, selection=args.select, original_sha256=NEW_PIN, cases=len(rows),
                  completed=len(whole), prefixes=len(rows) - len(whole),
                  by_entry={hex(e): sum(1 for r in whole if r['entry'] == e)
                            for e in (0x26c3c0, 0x200d30, 0x1fc9b0, 0x1fc540, 0x20124a, 0x2012df, 0x201342)},
                  named_refusals=stops,
                  reads=sum(len(r['reads']) for r in rows), stores=sum(len(r['writes']) for r in rows),
                  coverage=sites_by_owner, unreached_owned_sites=unreached,
                  original_instructions=sum(row['original_instructions'] for row in rows),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=reference.sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(reference.ROOT)): reference.sha(p) for p in
                                 (Path(__file__), Path(reference.__file__), Path(search.__file__),
                                  reference.ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'coverage'}))


if __name__ == '__main__':
    main()
