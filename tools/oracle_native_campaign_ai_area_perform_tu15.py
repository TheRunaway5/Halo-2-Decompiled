#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native 1BBEE0, 1BC0A0 and the 26A2E0 root.

1BBEE0 is behavior row 109's "perform" callback (descriptor 48A9D8 +40) and
1BC0A0 its +48 slot. Each case installs declared actor, behavior, workspace,
authored-area and path banks plus the actual caller frame, then single-steps
the authenticated English TU1.5 code. The natively owned callees (200C00,
26C510, 1BBBD0, 1D2D90, 1D2E30 and the 26A2E0 root with its 32A660 probe)
execute physically, as do 26B220 (type 4) and the complete 26BA30 candidate
collection closure, which also has direct-entry cases over its own declared
squad/order/zone/firing-position/owner-table/cluster banks. Unported callees
(26A380, 26C3C0, BBF20) are declared stop entries: the original stops right
after their CALL pushed the return word, which is where the native closure
refuses by name. No instruction is patched and no callee outcome is supplied.
The original ELF, observer and raw banks live only in HALO2_TEST_SCRATCH.
"""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import CS_AC_READ, CS_AC_WRITE
from capstone.x86_const import X86_OP_MEM
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
import oracle_native_campaign_ai_area_tu15 as areas
import oracle_native_campaign_ai_area_choice_tu15 as choice

ENTRIES = (0x1bbee0, 0x1bc0a0, 0x26a2e0, 0x32a660, 0x200c00, 0x218780, 0x145070, 0x219030,
           0x26c510, 0x1bbbd0, 0x20d180, 0x20d2d0, 0x1d2d90, 0x1d2e30, 0x149680,
           0x26b220, 0x1d2cf0, 0x1ea1d0, 0x1ed530, 0x1ecd00, 0x1ecc40,
           0xbad60, 0xbacd0, 0x0fc510, 0x144910, 0x145560, 0x1e11b0, 0x1e1200, 0x1ecd40,
           # 26BA30 candidate collection closure.
           0x26ba30, 0x26bbd0, 0x26be90, 0x26c0b0, 0x26c1e0, 0x26c740, 0x140db0, 0x267a30, 0x267a40,
           0x218b20, 0x3ea20, 0x2a78a0, 0x202ab0, 0x26c850, 0x277d40,
           # 26A380 goal evaluation (native prefix).
           0x26a380, 0x2013b0, 0x275e90, 0x1ed700)
COLLECTION = (0x26ba30, 0x26bbd0, 0x26be90, 0x26c0b0, 0x26c1e0, 0x26c740, 0x140db0, 0x267a30, 0x267a40,
              0x218b20, 0x3ea20, 0x2a78a0, 0x202ab0)
# 26A380's first unported points: the 2013B0 path query and the branch PCs
# of request shapes that 1BBEE0's type-4 request never builds.
STOP_ENTRIES = (0x2013b0, 0x26a43e, 0x26a5b7, 0x26a62e, 0x26a846, 0x26c3c0, 0xbbf20)
AI_GLOBALS = areas.BASE + 0x26800
# Composed 1BBEE0 cases: 26BA30 globals and their structures in the seed bank.
FIRE_BSP, FIRE_DYN, FIRE_DYN_ROWS, FIRE_HASH, FIRE_NODES, FIRE_ORIGIN, FIRING = (
    areas.BASE + n for n in (0x26000, 0x26100, 0x26200, 0x27000, 0x27100, 0x27200, 0x27400))
FIRE_GLOBALS = ((0x474c24, 2), (0x474d04, 4), (0x45695c, 4), (0x52d87c, 4), (0x56aeb0, 4), (0x4ecc18, 4),
                (0x5020a8, 4))
# 26B220 type-4 inputs: object definitions, character tags, props, squads.
TAGS, DEF_UNIT, CHAR, CHAR_ENTRIES, FLAGS = (areas.BASE + n for n in (0x24000, 0x24100, 0x24200, 0x24400, 0x24800))
BODY2, PROP_POOL, PROP_ROWS, SQUAD_POOL, SQUAD_ROWS = (areas.BASE + n for n in (0x25000, 0x25800, 0x25900, 0x25a00, 0x25b00))
UNIT_CONSTANTS = ((0x46a870, 4), (0x46a20c, 4), (0x46a108, 4), (0x46a0e8, 4), (0x46a0ec, 4), (0x46a0e4, 4))
STACK, STACK_SIZE, STOP = 0x2000000, 0x40000, 0x3000000
SP = STACK + 0x3e000
DATA_END = 0x574000
ACTOR = areas.ACTORS  # datum 0xe3610000 -> row 0
STATE = choice.OUTPUT - 0xc  # behavior state; +C is the 1BBBD0 selection
NONE = 0xffffffff
REGISTERS = {name: n for n, name in enumerate(('eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi'))}
WORKSPACE_GLOBALS = ((0x52be54, 4), (0x52be58, 1), (0x486070, 4), (0x51df44, 4), (0x51df48, 1), (0x50208c, 0x18))
CONSTANTS = ((0x456914, 4), (0x46a1a4, 4))


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def frame_base(sp):
    return ((sp - 4) & ~7) - 0x76c - 12


def metadata(image, entries=ENTRIES):
    code = _Code(image, '.text')
    sites, owners = [], {}
    for entry in entries:
        body = complete_body(code, entry)
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            listing.append(f'{pc:x} {instruction.bytes.hex()} {instruction.mnemonic} {instruction.op_str}\n')
            operands = []
            for operand in instruction.operands:
                if operand.type != X86_OP_MEM or instruction.mnemonic == 'lea':
                    continue
                base, index = instruction.reg_name(operand.mem.base), instruction.reg_name(operand.mem.index)
                read, write = bool(operand.access & CS_AC_READ), bool(operand.access & CS_AC_WRITE)
                if instruction.mnemonic in ('cmp', 'test'):
                    # Capstone omits the read flag on some compare/test operands.
                    read, write = True, False
                if instruction.mnemonic.startswith(('fst', 'fist')):
                    read, write = False, True
                operands.append(dict(base=REGISTERS[base] if base else -1, index=REGISTERS[index] if index else -1,
                                     scale=operand.mem.scale, displacement=operand.mem.disp & 0xffffffff,
                                     bytes=operand.size, read=read, write=write))
            site = dict(instruction=pc, bytes=instruction.bytes.hex(), operands=operands)
            if {0xf2, 0xf3} & set(instruction.prefix) and instruction.mnemonic.startswith(('rep', 'stos', 'movs')):
                site['repeat_ecx'] = True
            sites.append(site)
        owners[hex(entry)] = dict(sites=len(body.code), listing_sha256=hashlib.sha256(''.join(listing).encode()).hexdigest())
    return dict(sites=sites, stop=STOP, owners=owners)


def prepare(image, meta, scratch):
    p = scratch / 'physical-original'
    p.mkdir()
    assembly, sections = ['.code32'], []
    for n, name in enumerate(('.text', '.rdata', '.data')):
        s = image.section(name)
        data = image.section_bytes(s)
        f = p / f'section{n}.bin'
        f.write_bytes(data)
        assembly += [f'.section .original{n},"' + ('ax"' if n == 0 else 'aw"'), f'.incbin "{f}"']
        if name == '.data':
            assembly += [f'.space {DATA_END - s.va - len(data)}']
        sections += [f'.original{n} 0x{s.va:x}:{{*(.original{n})}}']
    assembly += ['.section .bank,"aw"', '.space 8388608', '.section .stack,"aw"', f'.space {STACK_SIZE}',
                 '.section .stop,"ax"', 'int3', '.section .bridge,"ax"', '.globl _start', '_start:', 'fninit', 'int3', 'jmp _start']
    sections += [f'.bank 0x{areas.BASE:x}:{{*(.bank)}}', f'.stack 0x{STACK:x}:{{*(.stack)}}',
                 f'.stop 0x{STOP:x}:{{*(.stop)}}', '.bridge 0x70000000:{*(.bridge)}']
    (p / 'reference.s').write_text('\n'.join(assembly) + '\n')
    (p / 'link.ld').write_text('ENTRY(_start)\nSECTIONS{\n' + '\n'.join(sections) + '\n}\n')
    (p / 'metadata.json').write_text(json.dumps(meta))
    for command in (['as', '--32', str(p / 'reference.s'), '-o', str(p / 'reference.o')],
                    ['ld', '-melf_i386', '-T', str(p / 'link.ld'), str(p / 'reference.o'), '-o', str(p / 'reference')],
                    ['c++', '-std=c++20', '-O2', str(ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp'), '-o', str(p / 'observer')]):
        r = subprocess.run(command, text=True, capture_output=True)
        assert not r.returncode, r.stderr
    return p


def physical(p, rows, suffix):
    data = p / (suffix + '-inputs.json')
    data.write_text(json.dumps(rows, separators=(',', ':')))
    result = p / (suffix + '-observed.json')
    r = subprocess.run([str(p / 'observer'), str(p / 'reference'), str(p / 'metadata.json'), str(data), str(result)],
                       text=True, capture_output=True, timeout=1800)
    assert r.returncode == 0, r.stdout + r.stderr
    print(r.stdout.strip(), flush=True)
    observed = json.loads(result.read_text())
    assert not observed['instruction_patches'] and observed['supplied_original_outcomes'] is False
    byid = {x['id']: x for x in observed['cases']}
    for row in rows:
        actual = byid[row['id']]
        for old, new in zip(row['banks'], actual.pop('banks')):
            assert old['address'] == new['address']
            old['after'] = new['after']
        row.update(actual, physical_verified=True)
    return rows


def runs(data, base=None, gap=16):
    """Sparse [offset, hex] runs: nonzero bytes, or bytes that differ from base."""
    out, n = [], 0
    differs = (lambda k: data[k] != 0) if base is None else (lambda k: data[k] != base[k])
    while n < len(data):
        if not differs(n):
            n += 1
            continue
        start = end = n
        while n < len(data) and n - end <= gap:
            if differs(n):
                end = n
            n += 1
        out.append([start, data[start:end + 1].hex()])
    return out


def compact(rows):
    """Store each bank as its size plus sparse before/after runs."""
    for row in rows:
        for bank in row['banks']:
            before, after = bytes.fromhex(bank.pop('before')), bytes.fromhex(bank.pop('after'))
            bank.update(size=len(before), before_runs=runs(before), after_runs=runs(after, before))
    return rows


def seeds(image):
    """Area-choice rows for 1BBBD0 supply real authored-area/actor banks."""
    rows = [r for r in choice.inputs(image) if r['entry'] == 0x1bbbd0 and not r['input_timeline']]
    picked = {}
    for r in rows:
        key = r['name'].split('_')[0]
        if r['name'] in ('default_0_0x0', 'default_1_0x1', 'count_0', 'count_1', 'count_3', 'count_40',
                         'frame_0_valid', 'reset_(0, 2)', 'order_0_0x3', 'pairs_1_0xffffffff'):
            picked[r['name']] = r
    return picked


def case_rows(image):
    base_rows = seeds(image)
    rows = []

    def make(name, seed='default_0_0x0', entry=0x1bbee0, actor=None, globals_=None, stack_extra=None,
             fp=None, depth=0, xmm_exact=False, path=None, unit=None, constants=None, ecx=None, property_b4=None):
        row = copy.deepcopy(base_rows[seed])
        raw = bytearray.fromhex(next(b['before'] for b in row['banks'] if b['address'] == areas.BASE))

        def put(address, value, width=4):
            assert areas.BASE <= address and address + width <= areas.BASE + len(raw), hex(address)
            raw[address - areas.BASE:address - areas.BASE + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'little')
        fields = dict(active=1, goal=NONE, mode=0, klass=0, excluded=0, pinned=0, failures=0, history=0,
                      reference=None)
        fields.update(actor or {})
        put(ACTOR + 0x40, fields['active'], 1)
        put(ACTOR + 0x418, fields['goal'])
        put(ACTOR + 0x504, fields['mode'], 2)
        put(ACTOR + 0x328, fields['klass'], 2)
        put(ACTOR + 7, fields['excluded'], 1)
        put(ACTOR + 0x220, fields['pinned'], 1)
        put(ACTOR + 0x5b4, fields['failures'], 2)
        put(ACTOR + 0x3fe, fields['history'], 2)
        put(ACTOR + 0x3f2, 0x5a, 1)
        if fields['reference'] is not None:
            put(ACTOR + 0x26c, fields['reference'])
        for n in range(4):
            put(ACTOR + 0x400 + n * 6, 0x30 + n, 1)
            put(ACTOR + 0x402 + n * 6, (0x7000 + n) << 16 | n)
        put(ACTOR + 0x50c, 0, 1)
        if path:
            put(ACTOR + 0x50c, 1, 1)
            origin, points = path
            for axis, value in enumerate(origin):
                put(ACTOR + 0x238 + axis * 4, bits(value) if isinstance(value, float) else value)
            put(ACTOR + 0x53a, 0, 1)
            put(ACTOR + 0x539, len(points), 1)
            for k, point in enumerate(points):
                for axis, value in enumerate(point):
                    put(ACTOR + 0x548 + k * 0x1c + axis * 4, bits(value) if isinstance(value, float) else value)
                put(ACTOR + 0x548 + k * 0x1c + 0xc, 0xffff, 2)
        # 26B220 type-4 actor, object and character state.
        u = dict(mode270=0, alert=0, target=None, unit=0, mounted=0, seat=NONE, character=2, perceived=0, prop=0,
                 prop_count=0, seen=0., now=0., squad_size=0, squad=NONE, squad_count=0, object_type=0, parent=NONE,
                 physics=NONE, weapon_index=0xff, weapon=1, weapon_definition=0x5555, entries=(), behavior=None,
                 vision=None, kind=0, character_parent=NONE, flags4=0, node_position=None)
        u.update(unit or {})
        if name.startswith('compose_'):
            # These composed cases now execute the real query. Its character
            # radius and already-cached framed point are independent inputs.
            if u['behavior'] is None:u['behavior']=0
            put(ACTOR+0x278,1,1)
            put(ACTOR+0x288,0xffff);put(ACTOR+0x28c,NONE)
            for axis,value in enumerate((1.0,2.0,3.0)):put(ACTOR+0x27c+axis*4,bits(value))
        # 26BA30: no dynamic firing positions unless a case declares them.
        put(ACTOR + 0x3f4, fields.get('dynamic', NONE))
        if 'squad' in fields:
            put(ACTOR + 0x30, fields['squad'])
        put(FIRE_BSP + 0xc4, 0)
        put(FIRE_DYN + 0x44, FIRE_DYN_ROWS)
        put(FIRE_HASH + 0x20, 16)
        put(FIRE_HASH + 0x28, 4)
        put(FIRE_HASH + 0x2c, 0x267a30)
        put(FIRE_HASH + 0x30, 0x267a40)
        for axis, value in enumerate((1.5, -2.5, 3.25)):
            put(FIRE_ORIGIN + axis * 4, bits(value))
        firing = fields.get('firing', 0)
        if firing:
            # Zone 0 / area 0 (the seeds' CANDIDATES records) gets firing
            # positions [0, firing) in the zone's 32-byte table.
            put(areas.ZONES + 0x24, 0xffff, 2)
            put(areas.ZONES + 0x28, firing)
            put(areas.ZONES + 0x2c, FIRING)
            put(choice.CANDIDATES + 0x38, 0, 2)
            put(choice.CANDIDATES + 0x3a, firing, 2)
            for k in range(firing):
                for axis, value in enumerate((k + 1.0, 0.25 * k, 0.5)):
                    put(FIRING + k * 0x20 + axis * 4, bits(value))
                put(FIRING + k * 0x20 + 0xc, 0xffff, 2)
                put(FIRING + k * 0x20 + 0xe, 0x40 if k != 2 else 0x60, 1)
                put(FIRING + k * 0x20 + 0x14, 0)
        dynamic_count = fields.get('dynamic_count', 0)
        put(FIRE_DYN_ROWS + 2, 1, 2)
        put(FIRE_DYN_ROWS + 8, dynamic_count, 2)
        put(FIRE_DYN_ROWS + 0x40, (1 << dynamic_count) - 1 if dynamic_count else 0)
        for k in range(dynamic_count):
            for axis, value in enumerate((-1.0 - k, 2.0, 0.)):
                put(FIRE_DYN_ROWS + 0x44 + k * 0x20 + axis * 4, bits(value))
            put(FIRE_DYN_ROWS + 0x44 + k * 0x20 + 0xc, 0xffff, 2)
            put(FIRE_DYN_ROWS + 0x44 + k * 0x20 + 0xe, 0x40, 1)
            put(FIRE_DYN_ROWS + 0x444 + k * 2, 0x300 + k, 2)
        put(ACTOR + 0x270, u['mode270'], 2)
        put(ACTOR + 0x229, u['alert'], 1)
        if u['target'] is not None:
            put(ACTOR + 0x26c, u['target'])
        put(ACTOR + 0x18, u['unit'])
        put(ACTOR + 0x268, u['mounted'], 1)
        put(ACTOR + 0x274, u['seat'])
        put(ACTOR + 0x54, u['character'])
        put(ACTOR + 0x358, u['perceived'], 2)
        put(ACTOR + 0x368, u['prop'])
        put(ACTOR + 0x39c, bits(u['seen']) if isinstance(u['seen'], float) else u['seen'])
        put(ACTOR + 0x394, bits(u['now']) if isinstance(u['now'], float) else u['now'])
        put(ACTOR + 0x86, u['squad_size'], 2)
        put(ACTOR + 0x7c, u['squad'])
        # Unit fields +212/+218 overlap the seed's node matrices at +200:
        # move those 13 floats to +400 and retarget the +116 node offset.
        matrices = bytes(raw[choice.BODY + 0x200 - areas.BASE:choice.BODY + 0x234 - areas.BASE])
        raw[choice.BODY + 0x400 - areas.BASE:choice.BODY + 0x434 - areas.BASE] = matrices
        put(choice.BODY + 0x116, 0x400, 2)
        if (unit or {}).get('node_position'):
            for axis, value in enumerate(unit['node_position']):
                put(choice.BODY + 0x428 + axis * 4, bits(value))
        put(choice.BODY, 1)
        put(choice.BODY + 4, u['flags4'])
        put(choice.BODY + 0x14, u['parent'])
        put(choice.BODY + 0xaa, u['object_type'], 1)
        put(choice.BODY + 0xb4, u['physics'])
        for axis, value in enumerate((3.5, -2.25, 0.75)):
            put(choice.BODY + 0x30 + axis * 4, bits(value))
        put(choice.BODY + 0x212, u['weapon_index'], 1)
        put(choice.BODY + 0x218, u['weapon'])
        put(choice.BODY + 0x21c, NONE)
        put(choice.BODY + 0x346, 0x300, 2)
        put(choice.BODY + 0x336, u['kind'], 2)
        put(choice.OBJECT_ROWS + 12 + 8, BODY2)
        put(BODY2, u['weapon_definition'])
        put(BODY2 + 0x14, NONE)
        for row_index, pointer in ((1, DEF_UNIT), (2, CHAR), (3, CHAR + 0x100)):
            put(TAGS + row_index * 16 + 8, pointer)
        put(DEF_UNIT + 2, 0, 1)
        put(DEF_UNIT + 0x38, NONE)
        owner = CHAR + (0x100 if u['character_parent'] != NONE else 0)
        for character, parent in ((CHAR, u['character_parent']), (CHAR + 0x100, NONE)):
            put(character + 8, parent)
            put(character + 0x34, 1 if u['vision'] is not None and character == CHAR else 0)
            put(character + 0x38, FLAGS + 4)
            put(character + 0x5c, 1 if u['behavior'] is not None and character == owner else 0)
            put(character + 0x60, FLAGS)
            put(character + 0xcc, len(u['entries']) if character == owner else 0)
            put(character + 0xd0, CHAR_ENTRIES)
        for character, (count, pointer) in zip((CHAR, CHAR + 0x100), property_b4 or ((0, 0), (0, 0))):
            put(character + 0xb4, count)
            put(character + 0xb8, pointer)
        for k, (definition, vector) in enumerate(u['entries']):
            put(CHAR_ENTRIES + k * 0xcc + 8, definition)
            for axis, value in enumerate(vector):
                put(CHAR_ENTRIES + k * 0xcc + 0x60 + axis * 4, bits(value) if isinstance(value, float) else value)
        put(FLAGS, u['behavior'] or 0)
        put(FLAGS + 4, u['vision'] or 0)
        put(PROP_POOL + 0x44, PROP_ROWS)
        put(PROP_ROWS + 0x24, u['prop_count'], 2)
        put(SQUAD_POOL + 0x44, SQUAD_ROWS)
        put(SQUAD_ROWS + 0x10, u['squad_count'], 2)
        for address in (ACTOR + 0x4ae, ACTOR + 0x484):
            put(address, 0x77, 1)
        put(ACTOR + 0x4b4, 0x12345678)
        n = len(rows)
        gp = [0x5a010000 + n * 31 + k * 7 for k in range(8)]
        gp[0] = entry
        gp[4] = SP
        if ecx is not None:
            gp[1] = ecx
        stack = bytearray(0x800)
        # Declared stack bank [frame, SP+0x20). The observer installs the
        # return/argument words at SP after these bytes.
        low = frame_base(SP)
        for k in range(0, len(stack), 4):
            stack[k:k + 4] = struct.pack('<I', 0xc0de0000 + n * 0x1000 + k)
        args = [STOP, 0xe3610000, STATE]
        for k, value in enumerate(args):
            stack[SP - low + k * 4:SP - low + k * 4 + 4] = struct.pack('<I', value)
        workspace = dict(initialized=0, owner=0, backing=0x1150000, used0=0, used1=0, ptr0=0x1160000, ptr1=0x1180000,
                         restore=0)
        workspace.update(globals_ or {})
        globals_bytes = {
            0x52be54: struct.pack('<I', workspace['restore']),
            0x52be58: bytes([workspace['initialized']]),
            0x486070: struct.pack('<I', workspace['owner']),
            0x51df44: struct.pack('<I', workspace['backing']),
            0x51df48: bytes([0x33]),
            0x50208c: bytes([workspace['used0'], 0, 0, 0]) + struct.pack('<II', workspace['ptr0'], 0x22974) +
            bytes([workspace['used1'], 0, 0, 0]) + struct.pack('<II', workspace['ptr1'], 0x22974),
        }
        banks = [b for b in row['banks'] if b['address'] not in (areas.BASE,)]
        banks = [b for b in banks if not any(a <= b['address'] < a + w for a, w in
                                             WORKSPACE_GLOBALS + CONSTANTS + UNIT_CONSTANTS + FIRE_GLOBALS)]
        banks.append(dict(address=areas.BASE, before=raw.hex()))
        banks.append(dict(address=low, before=bytes(stack[:SP - low + 0x20]).hex()))
        if name.startswith('compose_'):
            # Actual 1BBEE0 -> 26A2E0 candidate frame -> 26A380 query.
            banks.append(dict(address=low-0xf8a8,before=bytes(0x50).hex()))
            for address in (0x46a0d0,0x46a100):
                banks.append(dict(address=address,before=image.read(address,4).hex()))
        for address, data in globals_bytes.items():
            banks.append(dict(address=address, before=data.hex()))
        for address, size in CONSTANTS + UNIT_CONSTANTS:
            value = (constants or {}).get(address)
            data = image.read(address, size) if value is None else struct.pack('<I', value)
            banks.append(dict(address=address, before=data.hex()))
        for address, value in ((0x4f0414, TAGS), (0x50eef0, PROP_POOL), (0x50eef8, SQUAD_POOL),
                               (0x474d04, FIRE_ORIGIN), (0x52d87c, FIRE_DYN), (0x56aeb0, FIRE_HASH),
                               (0x4ecc18, FIRE_BSP), (0x5020a8, AI_GLOBALS)):
            banks.append(dict(address=address, before=struct.pack('<I', value).hex()))
        for address, size in ((0x474c24, 2), (0x45695c, 4)):
            banks.append(dict(address=address, before=image.read(address, size).hex()))
        before_fp = fp or [0x27f, 0, 0x1f80]
        live = [struct.pack('<QH', 0x8000000000000000 + (k << 58), 0x3fff + k % 3 | (0x8000 if k % 2 else 0)).hex()
                for k in range(depth)]
        xmm = [[0x6b000000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)]
        rows.append(dict(id=n, name=name, entry=entry, gp=gp, matrix=0xe3610000, source=STATE,
                         banks=sorted(banks, key=lambda b: b['address']), before_fp=before_fp, live_x87=live,
                         xmm=xmm, stop_before_entries=list(STOP_ENTRIES), max_steps=60000,
                         strict_writes=False, xmm_exact=xmm_exact, seed=seed))
        if name.startswith('compose_'):
            rows[-1]['stop_before_entries']=[e for e in STOP_ENTRIES if e!=0x2013b0]+[0x26a712]
        return rows[-1]

    # Inactive actor: immediate AL=1, only the frame stores.
    for goal in (NONE, 0x00010002):
        make(f'inactive_goal_{goal:x}', actor=dict(active=0, goal=goal), xmm_exact=True)
    # No current goal: lease, request, 26A2E0 root.
    for excluded in (1, 0x80):
        for klass in (0, 2, 3, 0xffff, 0x8000):
            make(f'no_goal_excluded{excluded:x}_class{klass:x}', actor=dict(excluded=excluded, klass=klass), xmm_exact=True)
    for state in (dict(initialized=1), dict(initialized=1, used0=1), dict(initialized=1, used0=1, used1=1),
                  dict(initialized=0, restore=0), dict(initialized=1, used1=1)):
        make('workspace_' + '_'.join(f'{k}{v:x}' for k, v in state.items()), actor=dict(excluded=1), globals_=state,
             xmm_exact=True)
    for klass in (0, 3):
        for pinned in (0, 1):
            make(f'no_goal_select_class{klass}_pinned{pinned}', actor=dict(klass=klass, pinned=pinned), xmm_exact=True)
    # A current goal with path mode 2 chooses an area first (26C510 + 1BBBD0).
    for seed in ('default_0_0x0', 'default_1_0x1', 'count_0', 'count_1', 'count_3', 'frame_0_valid',
                 'order_0_0x3', 'pairs_1_0xffffffff', 'reset_(0, 2)'):
        for excluded in (1, 0):
            make(f'area_{seed}_excluded{excluded}', seed=seed, actor=dict(goal=0x00010002, mode=2, excluded=excluded))
    for history in (0, 1, 3):
        make(f'area_history{history}', actor=dict(goal=0x00010002, mode=2, excluded=1, history=history))
    make('area_known_goal', actor=dict(goal=(0x7001 << 16) | 1, mode=2, excluded=1))
    # A current goal with another mode measures 200C00's remaining distance.
    for mode in (0, 3, 0xffff):
        make(f'distance_zero_mode{mode:x}', actor=dict(goal=0x00010002, mode=mode, excluded=1))
    for label, points in (('near', [(0.5, 0., 0.)]), ('far', [(10., 0., 0.)]), ('edge', [(0.6, 0., 0.)]),
                          ('two', [(0.25, 0., 0.), (0.25, 0.3, 0.)]), ('nan', [(0x7fc00000, 0., 0.)]),
                          ('denormal', [(0x00000001, 0., 0.)])):
        for fp in ([0x27f, 0, 0x1f80], [0x27f, 0, 0x1fc0], [0x7f, 0x3800, 0x9fc0]):
            for depth in (0, 3):
                make(f'distance_{label}_{fp[2]:x}_{depth}', actor=dict(goal=0x00010002, mode=1, excluded=1),
                     path=((0., 0., 0.), points), fp=fp, depth=depth)
    make('distance_far_select', actor=dict(goal=0x00010002, mode=1), path=((0., 0., 0.), [(10., 0., 0.)]))
    # 26B220 type-4 candidate preparation (stops at 26BA30 candidate collection).
    sel = dict(goal=NONE)
    for mode270 in (0, 1, 4, 0xffff):
        for alert in (0, 1):
            make(f'prep_mode{mode270:x}_alert{alert}', actor=sel, unit=dict(mode270=mode270, alert=alert))
    for label, consts in (('unequal', {0x46a0e8: bits(-2.)}), ('nan', {0x46a1a4: 0x7fc00000}),
                          ('snan', {0x46a1a4: 0x7f800001}), ('denormal', {0x46a1a4: 1, 0x46a0e8: 1})):
        for mxcsr in (0x1f80, 0x1fc0):
            make(f'prep_previous_{label}_{mxcsr:x}', actor=sel, constants=consts, fp=[0x27f, 0, mxcsr])
    for object_type in (1, 12, 2, 3):
        for parent, physics in ((NONE, NONE), (5, NONE), (5, 7)):
            make(f'prep_object{object_type}_{parent:x}_{physics:x}', actor=sel,
                 unit=dict(object_type=object_type, parent=parent, physics=physics))
    for position in ((5.5, -6.25, 7.125), (-0., 1e-30, 3e30)):
        for flags4 in (0, 0x400):
            make(f'prep_marker_{position[0]}_{flags4:x}', actor=sel, unit=dict(node_position=position, flags4=flags4))
    make('prep_bbf20', actor=sel, unit=dict(object_type=2, parent=NONE, physics=3))
    make('prep_target_object', actor=dict(goal=NONE, reference=0), unit=dict(object_type=2, parent=5))
    make('prep_unit_mirror', actor=sel, unit=dict(flags4=0x400))
    for mounted, seat in ((1, NONE), (1, 0), (0, 0)):
        for index in (0xff, 0, 1):
            make(f'prep_weapon_m{mounted}_s{seat:x}_i{index:x}', actor=sel,
                 unit=dict(mounted=mounted, seat=seat, weapon_index=index, entries=((0x5555, (1., 0., 0.)),)))
    for label, vector in (('large', (1., 2., 3.)), ('small', (1e-3, 0., 0.)), ('zero', (0., 0., 0.)),
                          ('nan', (0x7fc00000, 0., 0.)), ('denormal', (1, 1, 1)), ('huge', (3e38, 3e38, 0.))):
        for mxcsr in (0x1f80, 0x9fc0):
            make(f'prep_facing_{label}_{mxcsr:x}', actor=sel, fp=[0x27f, 0, mxcsr],
                 unit=dict(weapon_index=0, entries=((0x1111, (9., 9., 9.)), (0x5555, vector))))
    for label, vector, consts in (('zonly', (0., 0., 1.), None), ('equal', (2., 0., 0.), {0x46a0ec: bits(4.)}),
                                  ('equal_z', (0., 0., 2.), {0x46a0ec: bits(4.)}), ('below', (1., 1., 1.), {0x46a0ec: bits(3.5)})):
        make(f'prep_facing_{label}', actor=sel, constants=consts,
             unit=dict(weapon_index=0, entries=((0x5555, vector),)))
    make('prep_prop_equal', actor=sel, constants={0x46a0e4: bits(1.)},
         unit=dict(perceived=1, prop_count=1, seen=1., now=2.))
    make('prep_prop_just_above', actor=sel, constants={0x46a0e4: bits(1.)},
         unit=dict(perceived=1, prop_count=1, seen=1., now=1.9999999))
    make('prep_facing_parent', actor=sel, unit=dict(weapon_index=0, character_parent=3,
         entries=((0x5555, (1., 1., 1.)),)))
    make('prep_facing_missing', actor=sel, unit=dict(weapon_index=0, entries=((0x1111, (1., 1., 1.)),)))
    for perceived, count, seen, now in ((0, 1, 0., 0.), (1, 0, 0., 0.), (1, 1, 0., 0.), (1, 1, 5., 0.5),
                                        (1, 1, 0., 100.), (0xffff, 1, 5., 0.), (1, 2, float('nan'), 0.)):
        make(f'prep_prop_{perceived:x}_{count}_{seen}_{now}', actor=sel,
             unit=dict(perceived=perceived, prop_count=count, seen=seen, now=now))
    for behavior in (0, 0x20, 0x30, 0x10, 0xffffffff):
        for kind in (0, 5):
            for vision in (None, 0, 2):
                make(f'prep_behavior_{behavior:x}_{kind}_{vision}', actor=sel,
                     unit=dict(behavior=behavior, kind=kind, vision=vision))
    make('prep_behavior_parent', actor=sel, unit=dict(behavior=0x30, character_parent=3))
    make('prep_character_none', actor=sel, unit=dict(character=NONE))
    for size, squad, count in ((5, NONE, 2), (4, 0, 2), (5, 0, 1), (5, 0, 2), (0x8000, 0, 2)):
        make(f'prep_squad_{size:x}_{squad:x}_{count}', actor=sel, unit=dict(squad_size=size, squad=squad, squad_count=count))
    for seed in ('count_1', 'default_1_0x1'):
        make(f'prep_after_area_{seed}', seed=seed, actor=dict(goal=0x00010002, mode=2))
    # 1ECD40 (ECX character datum; count +B4, value +B8).
    for label, datum, parent, counts in (
            ('none', NONE, NONE, ((1, 0x1234), (0, 0))), ('first', 2, NONE, ((1, 0x1234), (0, 0))),
            ('negative', 2, NONE, ((0xffffffff, 0x1234), (0, 0))), ('zero_end', 2, NONE, ((0, 0x1234), (0, 0))),
            ('parent', 2, 3, ((0, 0x1234), (7, 0x5678))), ('parent_empty', 2, 3, ((0, 0x1234), (0x80000000, 0x5678))),
            ('tagged', 0x7fff0002, NONE, ((2, 0xabcd), (0, 0))), ('tagged_parent', 0xe3610002, 0x55550003, ((0, 0), (1, 0x9999)))):
        make(f'property_{label}', entry=0x1ecd40, ecx=datum, xmm_exact=True,
             unit=dict(character_parent=parent), property_b4=counts)
    # 1BC0A0.
    for reference in (0xffffffff, 0, 0x12345678):
        make(f'release_reference{reference:x}', entry=0x1bc0a0, actor=dict(reference=reference), xmm_exact=True)
    # Composed 1BBEE0 -> 26A2E0 -> 26BA30 with real candidates; stops at 26A380.
    for seed in ('default_0_0x0', 'order_0_0x3', 'pairs_1_0xffffffff', 'count_1'):
        for firing in (1, 3):
            make(f'compose_firing{firing}_{seed}', seed=seed, actor=dict(goal=NONE, firing=firing))
    make('compose_firing3_alert', actor=dict(goal=NONE, firing=3), unit=dict(alert=1))
    make('compose_firing3_targeted', actor=dict(goal=NONE, firing=3, reference=0))
    make('compose_dynamic2', actor=dict(goal=NONE, dynamic=0, dynamic_count=2))
    make('compose_dynamic3_firing2', actor=dict(goal=NONE, dynamic=0, dynamic_count=3, firing=2))
    make('compose_squad_none_dynamic2', actor=dict(goal=NONE, squad=NONE, dynamic=0, dynamic_count=2))
    for mxcsr in (0x1fc0, 0x9fc0):
        make(f'compose_firing3_{mxcsr:x}', actor=dict(goal=NONE, firing=3), fp=[0x27f, 0, mxcsr], depth=3)
    collection_rows(image, rows)
    return rows


# Direct 26BA30 layout (fresh bank at areas.BASE).
D = areas.BASE
(D_ACTOR_POOL, D_ACTORS, D_SQUAD_POOL, D_SQUADS, D_SCENARIO, D_DEFS, D_ORDERS, D_PRIMARY, D_SECONDARY,
 D_ZONES, D_AREAS, D_FIRING, D_DYN_POOL, D_DYN_ROWS, D_HASH, D_NODES, D_FRAMES, D_OBJECT_POOL,
 D_OBJECT_ROWS, D_BODY, D_BSP, D_RESOURCE, D_GEOMETRY, D_GEO_ENTRIES, D_REFS, D_ORIGIN, D_REQUEST,
 D_CANDIDATES) = (D + n for n in (0x100, 0x1000, 0x3000, 0x3100, 0x3800, 0x3c00, 0x4000, 0x4400, 0x4600,
                                  0x4800, 0x5000, 0x7000, 0x8000, 0x8100, 0x9000, 0x9100, 0xa000, 0xa100,
                                  0xa200, 0xa400, 0xb000, 0xb100, 0xb200, 0xb300, 0xb800, 0xbf00, 0xc000,
                                  0xd000))
D_SIZE = 0x10000
D_FLAG, D_AI_GLOBALS, D_CANDIDATE_OUT, D_OWNER_OUT = D + 0xbf80, D + 0xbfa0, D + 0xbe00, D + 0xbfc0
DATUMS = (0xe3610000, 0xe3620001, 0xe3630002)


def area_at(zone, area):
    return D_AREAS + zone * 0x800 + area * 0x88


def firing_at(zone, index):
    return D_FIRING + zone * 0x400 + index * 0x20


def collection_rows(image, rows):
    """Direct 26BA30 entries: [esp+4] actor, +8 candidates, +C capacity, +10 request."""
    def collect(name, changes=(), capacity=0x20, request=D_REQUEST, actor=0, fp=None, depth=0, nodes=(),
                constants=None, evaluate=None):
        raw = bytearray(D_SIZE)

        def put(address, value, width=4):
            assert D <= address and address + width <= D + D_SIZE, hex(address)
            if isinstance(value, float):
                value = bits(value)
            raw[address - D:address - D + width] = (value & ((1 << (8 * width)) - 1)).to_bytes(width, 'little')
        put(D_ACTOR_POOL + 0x44, D_ACTORS)
        for r, position in enumerate(((0., 0., 0.), (10., 0., 0.), (3., 0., 0.))):
            row = D_ACTORS + r * 0x888
            put(row + 0x24, 7, 2)
            put(row + 0x26c, NONE)
            put(row + 0x30, 0 if r == 0 else NONE)
            put(row + 0x3f4, NONE)
            for axis, value in enumerate(position):
                put(row + 0x238 + axis * 4, value)
        put(D_SQUAD_POOL + 0x44, D_SQUADS)
        for k, (indicator, order) in enumerate(((NONE, 0xffff), (0, 0xffff), (NONE, 0), (NONE, 1))):
            row = D_SQUADS + k * 0x98
            put(row + 0x80, indicator)
            put(row + 0x2a, order, 2)
            put(D_DEFS + k * 0x74 + 0x38, (0, 1, 0, 0xffff)[k], 2)
        for n, (zone, area) in enumerate(((0, 1), (1, 0), (0, 2))):
            put(D_SQUADS + 0x98 + 0x32 + n * 4, zone, 2)
            put(D_SQUADS + 0x98 + 0x34 + n * 4, area, 2)
        put(D_SQUADS + 0x98 + 0x30, 3, 1)
        put(D_SCENARIO + 0x164, D_DEFS)
        put(D_SCENARIO + 0x168, 4)
        put(D_SCENARIO + 0x16c, D_ZONES)
        put(D_SCENARIO + 0x244, D_ORDERS)
        lists = {0: ([(0, 0, 0), (2, 0, 1), (1, 1, 2)], [(2, 1, 0), (0, 0, 2)]), 2: ([(0, 1, 1)], [])}
        for k, (flags, parent) in enumerate(((0, 0xffff), (0x20, 2), (0x10, 0xffff), (0x20, 0xffff))):
            row = D_ORDERS + k * 0x7c
            put(row + 0x24, flags, 1)
            put(row + 0x4e, parent, 2)
            primary, secondary = lists.get(k, ([(0, 0, 0)], [(1, 0, 1)]))
            for base, entries, count_at, pointer_at in ((D_PRIMARY + k * 0x80, primary, 0x54, 0x58),
                                                         (D_SECONDARY + k * 0x80, secondary, 0x5c, 0x60)):
                put(row + count_at, len(entries))
                put(row + pointer_at, base)
                for n, (kind, zone, area) in enumerate(entries):
                    put(base + n * 8, kind, 2)
                    put(base + n * 8 + 4, zone, 2)
                    put(base + n * 8 + 6, area, 2)
        for zone in range(4):
            row = D_ZONES + zone * 0x38
            put(row + 0x24, 0xffff if zone != 3 else 0, 2)
            put(row + 0x28, 8)
            put(row + 0x2c, firing_at(zone, 0))
            put(row + 0x30, 3)
            put(row + 0x34, area_at(zone, 0))
            for area in range(3):
                at = area_at(zone, area)
                put(at + 0x38, area * 2, 2)
                put(at + 0x3a, 3, 2)
                put(at + 0x80, 2)
                put(at + 0x84, D_REFS + (zone * 8 + area) * 0x10)
                put(D_REFS + (zone * 8 + area) * 0x10, (zone + area) % 8, 2)
                put(D_REFS + (zone * 8 + area) * 0x10 + 4, (zone + area + 1) % 8, 2)
            for index in range(8):
                at = firing_at(zone, index)
                for axis, value in enumerate((index + 1.0, float(zone), 0.5)):
                    put(at + axis * 4, value)
                put(at + 0xc, 0xffff, 2)
                put(at + 0xe, (0x40, 0x40, 0x40, 0x60, 0x40, 0x20, 0x00, 0xc0)[index], 1)
                put(at + 0x14, NONE if index == 6 else index)
        put(D_DYN_POOL + 0x44, D_DYN_ROWS)
        for k, (flag, count, mask) in enumerate(((1, 4, 0b1011), (0, 2, 0b11))):
            row = D_DYN_ROWS + k * 0x484
            put(row + 2, flag, 2)
            put(row + 8, count, 2)
            put(row + 0x40, mask)
            for index in range(count):
                for axis, value in enumerate((float(index), 5.0 + k, 0.)):
                    put(row + 0x44 + index * 0x20 + axis * 4, value)
                put(row + 0x44 + index * 0x20 + 0xc, 0xffff, 2)
                put(row + 0x44 + index * 0x20 + 0xe, 0x40, 1)
                put(row + 0x444 + index * 2, 0x100 + index + k * 0x10, 2)
        put(D_HASH + 0x20, 16)
        put(D_HASH + 0x28, 4)
        put(D_HASH + 0x2c, 0x267a30)
        put(D_HASH + 0x30, 0x267a40)
        # nodes: (goal, value[, bucket hash override]) chained per bucket in order.
        heads = {}
        for n, node in enumerate(nodes):
            goal, value = node[0], node[1]
            hashed = node[2] if len(node) > 2 else (goal & 0xff) << 2
            at = D_NODES + n * 0x10
            put(at, goal)
            put(at + 4, hashed)
            put(at + 0xc, value)
            bucket = hashed % 16
            if bucket in heads:
                put(heads[bucket] + 8, at)
            else:
                put(D_HASH + 0x3c + bucket * 4, at)
            heads[bucket] = at
        put(D_FRAMES, 0x198b0000)
        put(D_FRAMES + 0xa, 2, 1)
        put(D_FRAMES + 0xb, 1, 1)
        put(D_FRAMES + 0x10, NONE)
        put(D_FRAMES + 0x20, 0x198b0000)
        put(D_FRAMES + 0x26, 0xffff, 2)
        put(D_FRAMES + 0x30, 0x198b0000)
        put(D_FRAMES + 0x36, 1, 2)
        put(D_OBJECT_POOL + 0x44, D_OBJECT_ROWS)
        put(D_OBJECT_ROWS + 8, D_BODY)
        put(D_BODY + 0x114, 52, 2)
        put(D_BODY + 0x116, 0x200, 2)
        for offset, value in ((0, 1.), (4, 0.), (8, 1.), (0x10, -1.), (0x14, 0.), (0x24, 1.),
                              (0x28, 100.), (0x2c, 200.), (0x30, 300.)):
            put(D_BODY + 0x200 + offset, value)
        put(D_BSP + 0xc4, 1)
        put(D_BSP + 0xc8, D_RESOURCE)
        put(D_RESOURCE + 0x6c, 1)
        put(D_RESOURCE + 0x70, D_GEOMETRY)
        put(D_GEOMETRY + 0x40, 8)
        put(D_GEOMETRY + 0x44, D_GEO_ENTRIES)
        for cluster in range(8):
            put(D_GEO_ENTRIES + cluster * 8, cluster % 3)
        for axis, value in enumerate((1.5, -2.5, 3.25)):
            put(D_ORIGIN + axis * 4, value)
        put(D_REQUEST, 4, 2)
        for change in changes:
            address, value, width = (tuple(change) + (4,))[:3]
            put(address, value, width)
        n = len(rows)
        entry = 0x26ba30 if evaluate is None else 0x26a380
        gp = [0x5a020000 + n * 29 + k * 11 for k in range(8)]
        gp[0] = entry
        gp[4] = SP
        banks = [dict(address=D, before=raw.hex())]
        for address, value in ((0x5020c8, D_ACTOR_POOL), (0x52be7c, D_SQUAD_POOL), (0x4ecc20, D_SCENARIO),
                               (0x52d87c, D_DYN_POOL), (0x56aeb0, D_HASH), (0x4ecc18, D_BSP),
                               (0x505e78, D_FRAMES), (0x4ecbd0, D_OBJECT_POOL), (0x474d04, D_ORIGIN),
                               (0x5020a8, D_AI_GLOBALS)):
            banks.append(dict(address=address, before=struct.pack('<I', value).hex()))
        for address, size in ((0x474c24, 2), (0x45695c, 4), (0x47d6c0, 4), (0x46a0d0, 4), (0x44acb0, 36)):
            value = (constants or {}).get(address)
            data = image.read(address, size) if value is None else value.to_bytes(size, 'little')
            banks.append(dict(address=address, before=data.hex()))
        live = [struct.pack('<QH', 0x8000000000000000 + (k << 58), 0x3fff + k % 3 | (0x8000 if k % 2 else 0)).hex()
                for k in range(depth)]
        xmm = [[0x6c000000 + n * 37 + r * 4 + k for k in range(4)] for r in range(8)]
        # 26BA30(actor, candidates, capacity, request) or
        # 26A380(actor, request, candidates, count, out, owner out, workspace, flag out).
        source, arguments = ((D_CANDIDATES, [capacity, request]) if evaluate is None else
                             (request, [D_CANDIDATES, evaluate, D_CANDIDATE_OUT, D_OWNER_OUT, 0, D_FLAG]))
        rows.append(dict(id=n, name=name, entry=entry, gp=gp, matrix=DATUMS[actor], source=source,
                         stack_arguments=arguments, banks=sorted(banks, key=lambda b: b['address']),
                         before_fp=fp or [0x27f, 0, 0x1f80], live_x87=live, xmm=xmm,
                         stop_before_entries=list(STOP_ENTRIES), max_steps=100000, strict_writes=False,
                         xmm_exact=False, seed='collection'))
        return rows[-1]

    def actor_field(offset, value, width=4, r=0):
        return (D_ACTORS + r * 0x888 + offset, value, width)

    def req(offset, value, width=1):
        return (D_REQUEST + offset, value, width)

    collect('collect_default')
    # Squad paths: definition zone, explicit pairs, order lists, redirect,
    # held order, upper datum bits, none.
    for squad in (NONE, 0, 1, 2, 3, 0x7fff0002):
        collect(f'collect_squad_{squad:x}', changes=(actor_field(0x30, squad),))
        collect(f'collect_squad_{squad:x}_no_request', changes=(actor_field(0x30, squad),), request=0)
    for pairs in (0xff, 0, 1, 0x80):
        collect(f'collect_pairs_{pairs:x}', changes=(actor_field(0x30, 1), (D_SQUADS + 0x98 + 0x30, pairs, 1)))
    collect('collect_definition_zone_none', changes=(actor_field(0x30, 3), (D_SQUADS + 3 * 0x98 + 0x2a, 0xffff, 2)))
    collect('collect_definition_zone_empty', changes=((D_ZONES + 0x30, 0),))
    for pinned in (0, 1):
        for near in (0, 1):
            for kind2 in (True, False):
                changes = [actor_field(0x30, 2), actor_field(0x3f2, pinned, 1), req(0x69c, near)]
                if not kind2:
                    changes.append((D_PRIMARY + 8, 0, 2))
                collect(f'collect_mask_p{pinned}_n{near}_k{int(kind2)}', changes=changes)
    collect('collect_secondary_list', changes=(actor_field(0x30, 2), (D_SQUADS + 2 * 0x98 + 0x60, 1, 1), req(0x69c, 1)))
    collect('collect_list_empty', changes=(actor_field(0x30, 2), (D_ORDERS + 0x54, 0)))
    collect('collect_list_negative', changes=(actor_field(0x30, 2), (D_ORDERS + 0x54, NONE)))
    collect('collect_list_kind31', changes=(actor_field(0x30, 2), (D_PRIMARY, 0x21, 2), (D_PRIMARY + 8, 0x22, 2)))
    collect('collect_list_kind16', changes=(actor_field(0x30, 2), (D_PRIMARY, 0x10, 2), (D_PRIMARY + 8, 0x12, 2),
                                            (D_PRIMARY + 16, 0x30, 2)))
    collect('collect_redirect_chain', changes=(actor_field(0x30, 3), (D_ORDERS + 2 * 0x7c + 0x24, 0x20, 1),
                                               (D_ORDERS + 2 * 0x7c + 0x4e, 1, 2), (D_SQUADS + 0x98 + 0x2a, 2, 2)))
    collect('collect_redirect_parent_none', changes=(actor_field(0x30, 2), (D_ORDERS + 0x24, 0x20, 1)))
    # 26BE90 gates.
    for zone, area in ((0, 0), (0, 1), (1, 2), (0xffff, 0)):
        collect(f'collect_request_area_{zone:x}_{area:x}', changes=(actor_field(0x30, 2), req(0x68c, 1),
                                                                     req(0x68e, zone, 2), req(0x690, area, 2)))
    collect('collect_zone_bsp_mismatch', changes=((D_ZONES + 0x24, 0, 2),))
    collect('collect_zone_bsp_global', constants={0x474c24: 3}, changes=((D_ZONES + 0x24, 3, 2),))
    for flags in (0, 1, 2, 3, 0xfffffffe):
        for targeted in (False, True):
            changes = [(area_at(0, a) + 0x20, flags, 4) for a in range(3)]
            if targeted:
                changes.append(actor_field(0x26c, 0x12340000))
            collect(f'collect_area_flags_{flags:x}_t{int(targeted)}', changes=changes)
    for b57, b59 in ((1, 0), (0, 1), (1, 1)):
        collect(f'collect_request_57_{b57}_59_{b59}', changes=(req(0x57, b57), req(0x59, b59),
                                                              (area_at(0, 0) + 0x20, 2, 4)))
    for capacity in (0, 1, 2, 5, NONE, 0x7fffffff):
        collect(f'collect_capacity_{capacity:x}', capacity=capacity)
        collect(f'collect_capacity_{capacity:x}_order', capacity=capacity, changes=(actor_field(0x30, 2), req(0x69c, 1)))
    collect('collect_span_negative', changes=((area_at(0, 0) + 0x3a, 0xfffe, 2),))
    collect('collect_first_negative', changes=((area_at(0, 0) + 0x38, 0xfffe, 2), (area_at(0, 0) + 0x3a, 4, 2)))
    collect('collect_firing_out_of_range', changes=((area_at(0, 2) + 0x38, 7, 2), (area_at(0, 2) + 0x3a, 4, 2)))
    # 26C0B0 admission flags.
    for b56, b58 in ((1, 0), (0, 1)):
        collect(f'collect_admit_56_{b56}_58_{b58}', changes=(req(0x56, b56), req(0x58, b58),
                                                            (D_BSP + 0xc4, 0)))
    collect('collect_admit_facing', changes=(req(0x57, 1),))
    collect('collect_admit_any', changes=(req(0x59, 1),))
    # Owner table: goals held by other actors.
    goal = lambda zone, index: (zone << 16) | index
    for label, nodes in (('self', ((goal(0, 0), DATUMS[0]),)),
                         ('none', ((goal(0, 0), NONE),)),
                         ('owner', ((goal(0, 0), DATUMS[1]), (goal(0, 1), DATUMS[1]), (goal(0, 2), DATUMS[2]))),
                         ('collision', ((goal(0, 0) + 0x100, DATUMS[1]), (goal(0, 0), DATUMS[1]))),
                         ('hash_mismatch', ((goal(0, 0), DATUMS[1], 0x40),)),
                         ('other_key', ((goal(1, 0), DATUMS[1]),))):
        collect(f'collect_owner_{label}', nodes=nodes)
    for label, owner_position in (('near', (0.5, 0., 0.)), ('edge', (1., 0., 0.)), ('far', (10., 0., 0.)),
                                  ('double', (2., 0., 0.)), ('nan', (0x7fc00000, 0., 0.)),
                                  ('inf', (0x7f800000, 0., 0.)), ('denormal', (1, 1, 1))):
        changes = [actor_field(0x238 + axis * 4, value, 4, 1) for axis, value in enumerate(owner_position)]
        collect(f'collect_owner_{label}', nodes=((goal(0, 0), DATUMS[1]), (goal(0, 1), DATUMS[1])), changes=changes)
    # Exact x87 boundaries: d_owner == 1.0 (JA not taken) and 2*d_self ==
    # (float)d_owner (JBE taken). Firing position 0 is at (1, 0, 0.5).
    collect('collect_owner_exact_one', nodes=((goal(0, 0), DATUMS[1]),),
            changes=[actor_field(0x238 + axis * 4, value, 4, 1) for axis, value in enumerate((1., 0., 1.5))] +
                    [actor_field(0x238 + axis * 4, value) for axis, value in enumerate((1., 0., 0.75))])
    collect('collect_self_double_exact', nodes=((goal(0, 0), DATUMS[1]),),
            changes=[actor_field(0x238 + axis * 4, value, 4, 1) for axis, value in enumerate((1., 0., 3.5))] +
                    [actor_field(0x238 + axis * 4, value) for axis, value in enumerate((1., 0., 2.))])
    collect('collect_owner_other_group', nodes=((goal(0, 0), DATUMS[1]),), changes=(actor_field(0x24, 8, 2, 1),))
    for label, self_position in (('far', (100., 0., 0.)), ('nan', (0x7fc00000, 0., 0.)), ('exact', (1.5, 0., 0.))):
        changes = [actor_field(0x238 + axis * 4, value) for axis, value in enumerate(self_position)]
        collect(f'collect_self_{label}', nodes=((goal(0, 0), DATUMS[1]), (goal(0, 1), DATUMS[1])), changes=changes)
    # Partial REP MOVSB copies over the NONE-initialized local (size 1 would
    # select actor 0xFF01 outside the declared pool).
    for size in (0, 2, 3):
        collect(f'collect_owner_size{size}', nodes=((goal(0, 0), DATUMS[1]),), changes=((D_HASH + 0x28, size),))
    collect('collect_owner_one_large', nodes=((goal(0, 0), DATUMS[1]),), constants={0x46a0d0: bits(9.)})
    collect('collect_owner_hash_refused', nodes=((goal(0, 0), DATUMS[1]),), changes=((D_HASH + 0x2c, 0xbbf20),))
    collect('collect_owner_equal_refused', nodes=((goal(0, 0), DATUMS[1]),), changes=((D_HASH + 0x30, 0xbbf20),))
    # Firing-position frames: transform, missing object, negative/beyond node.
    for frame in (0, 1, 2, 3):
        collect(f'collect_frame_{frame}', nodes=((goal(0, 0), DATUMS[1]),),
                changes=((firing_at(0, 0) + 0xc, frame, 2), (firing_at(0, 1) + 0xc, frame, 2)))
    collect('collect_limit_origin', constants={0x45695c: 0x7f800001},
            changes=((D_ORIGIN, 0xffc00001), (D_ORIGIN + 8, 0x80000000)))
    # Dynamic (52D87C) firing positions.
    for dynamic in (0, 1):
        for squad in (NONE, 2):
            collect(f'collect_dynamic{dynamic}_squad{squad:x}', changes=(actor_field(0x3f4, dynamic), actor_field(0x30, squad)))
    collect('collect_dynamic_no_request', request=0, changes=(actor_field(0x3f4, 0),))
    collect('collect_dynamic_capacity', capacity=3, changes=(actor_field(0x3f4, 0),))
    collect('collect_dynamic_count_negative', changes=(actor_field(0x3f4, 0), (D_DYN_ROWS + 8, 0xffff, 2)))
    collect('collect_dynamic_mask_high', changes=(actor_field(0x3f4, 0), (D_DYN_ROWS + 8, 34, 2)))
    for flags, parent, indicator, b3c, kind in ((0x10, 0xffff, 0, 0, 4), (0x20, 1, 0, 0, 4), (0x20, 0xffff, 0, 0, 4),
                                                (0x10, 0xffff, NONE, 0, 4), (0x10, 0xffff, 0, 1, 4),
                                                (0x10, 0xffff, 0, 0, 3), (0, 0xffff, 0, 0, 4)):
        collect(f'collect_held_{flags:x}_{parent:x}_{indicator:x}_{b3c}_{kind}',
                changes=(actor_field(0x3f4, 0), actor_field(0x30, 2), actor_field(0x3c, b3c, 1), req(0x69c, 1),
                         (D_ORDERS + 0x24, flags, 4), (D_ORDERS + 0x4e, parent, 2),
                         (D_SQUADS + 2 * 0x98 + 0x80, indicator), (D_REQUEST, kind, 2),
                         (D_SQUADS + 2 * 0x98 + 0x30, 1, 1), (D_SQUADS + 2 * 0x98 + 0x34, 1, 2)))
    collect('collect_held_empty_squad', changes=(actor_field(0x3f4, 0), actor_field(0x30, 2), (D_ORDERS + 0x24, 0x10, 4),
                                                 (D_SQUADS + 2 * 0x98 + 0x80, 0), (D_ORDERS + 0x54, 0)))
    collect('collect_dynamic_owner', changes=(actor_field(0x3f4, 0),),
            nodes=(((0x8000 << 16) | 1, DATUMS[1]),))
    # 2A78A0 cluster list (request+0x56).
    clusters = lambda *extra: (req(0x56, 1),) + tuple(extra)
    collect('collect_clusters', changes=clusters())
    collect('collect_clusters_order', changes=clusters(actor_field(0x30, 2), req(0x69c, 1)))
    for count in (0x1d, 0x1e, 0x1c, 0xffff, 0xfff0):
        collect(f'collect_clusters_count_{count:x}', changes=clusters(req(0x754, count, 2)))
    collect('collect_clusters_duplicate', changes=clusters(req(0x6a0, 0, 2), req(0x754, 1, 2)))
    collect('collect_clusters_fill', changes=clusters(*[(D_GEO_ENTRIES + c * 8, 9) for c in range(8)]))
    collect('collect_clusters_no_bsp', changes=clusters((D_BSP + 0xc4, 0)))
    collect('collect_clusters_no_resource', changes=clusters((D_RESOURCE + 0x6c, 0)))
    collect('collect_clusters_small_geometry', changes=clusters((D_GEOMETRY + 0x40, 1)))
    collect('collect_clusters_zone_bound', changes=clusters((D_SCENARIO + 0x168, 0)))
    collect('collect_clusters_area_bound', changes=clusters((D_ZONES + 0x30, 1)))
    collect('collect_clusters_refs_empty', changes=clusters((area_at(0, 0) + 0x80, 0)))
    # Negative clusters index the unbounded mask below it: the area record,
    # geometry and counter locals, then the saved EDI and EBX words.
    for cluster in (0xffe0, 0xffc0, 0xffa0, 0xff80, 0xff60, 63, 32):
        refs = D_REFS
        collect(f'collect_clusters_ref_{cluster:x}', changes=clusters((refs, cluster, 2), (refs + 4, 1, 2),
                                                                      (refs + 8, 2, 2), (area_at(0, 0) + 0x80, 3)))
    # FP environment around the owner/self x87 distances and SIMD projection.
    owned = ((goal(0, 0), DATUMS[1]), (goal(0, 1), DATUMS[1]), (goal(0, 2), DATUMS[1]))
    for cw in (0x27f, 0x7f, 0x37f, 0xe7f, 0x67f):
        for mxcsr in (0x1f80, 0x9fc0):
            for depth in (0, 4):
                collect(f'collect_fp_{cw:x}_{mxcsr:x}_{depth}', nodes=owned, fp=[cw, 0x3800 if depth else 0, mxcsr],
                        depth=depth, changes=(actor_field(0x238, 0.1), actor_field(0x23c, 1e-30),
                                              actor_field(0x238, 7.3, 4, 1), (firing_at(0, 1) + 0xc, 0, 2)))
    collect('collect_fp_status_sticky', nodes=owned, fp=[0x27f, 0x003f, 0x1fbf])
    # Direct 26A380: no candidates (count low word 0) completes; otherwise
    # the first unported point for each request shape.
    for count in (0, 0x10000, 0xffff0000):
        for actor in (0, 2):
            collect(f'evaluate_count_{count:x}_actor{actor}', evaluate=count, actor=actor,
                    changes=((D_FLAG, 0x5a, 1), (D_AI_GLOBALS + 3, 0x77, 1)))
    for label, offset in (('618', 0x618), ('54', 0x54), ('55', 0x55), ('56', 0x56), ('none', None)):
        for count in (1, 0x8000):
            changes = [(D_FLAG, 0x5a, 1)] + ([req(offset, 1)] if offset is not None else [])
            collect(f'evaluate_{label}_{count:x}', evaluate=count, changes=changes)
    collect('evaluate_618_and_56', evaluate=3, changes=(req(0x618, 1), req(0x56, 1)))
    collect('evaluate_55_and_56', evaluate=3, changes=(req(0x55, 1), req(0x56, 1)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--verify', type=Path)
    parser.add_argument('--no-prefixes', action='store_true')
    args = parser.parse_args()
    assert not args.output.exists()
    assert sha(NEW_DEFAULT) == NEW_PIN
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    image = Xbe(NEW_DEFAULT)
    meta = metadata(image)
    owned = prepare(image, meta, scratch)
    rows = physical(owned, case_rows(image), 'complete')
    # The previous composed qualification ended at the 2013B0 call. Extend
    # it through the unchanged query and stop on the first real read of its
    # consumer, 26A712. This is a read-fault prefix, never a callee outcome.
    continued=[]
    for row in rows:
        if row.get('stopped_at')!=0x26a712:continue
        ordinal=len(row['reads'])
        for key in ('reads','writes','executed_sites','calls','boundary','completed','stopped_at'):
            row.pop(key,None)
        for bank in row['banks']:bank.pop('after',None)
        row['fault']=dict(phase='before_read',ordinal=ordinal)
        row['stop_before_entries'].remove(0x26a712)
        continued.append(row)
    physical(owned,continued,'query-consumer-prefix')
    prefixes = []
    if not args.no_prefixes:
        for seed in [r for r in rows if r['name'] in ('no_goal_excluded1_class3', 'area_count_1_excluded0',
                                                       'distance_near_1f80_0', 'release_reference0',
                                                       'prep_facing_large_1f80', 'prep_behavior_30_5_0',
                                                       'property_parent', 'compose_firing3_order_0_0x3',
                                                       'collect_owner_far', 'collect_clusters_order',
                                                       'collect_held_20_ffff_0_0_4', 'collect_frame_0',
                                                       'collect_mask_p1_n1_k1', 'evaluate_count_0_actor2',
                                                       'compose_firing3_default_0_0x0',
                                                       'no_goal_select_class3_pinned1')]:
            for phase, events in (('before_read', seed['reads']), ('before_write', seed['writes']),
                                  ('after_write', seed['writes'])):
                for ordinal in range(0, len(events), max(1, len(events) // 12)):
                    row = copy.deepcopy(seed)
                    for key in ('reads', 'writes', 'executed_sites', 'calls', 'boundary', 'completed'):
                        row.pop(key, None)
                    for bank in row['banks']:
                        bank.pop('after', None)
                    row.update(id=len(rows) + len(prefixes), name=f"{seed['name']}_{phase}_{ordinal}",
                               fault=dict(phase=phase, ordinal=ordinal))
                    prefixes.append(row)
        rows += physical(owned, prefixes, 'prefix')
    reached = set().union(*(set(r['executed_sites']) for r in rows))
    roots = {s['instruction'] for s in meta['sites'] if 0x1bbee0 <= s['instruction'] < 0x1bc0e4 or
             0x26a2e0 <= s['instruction'] < 0x26a378 or 0x26b220 <= s['instruction'] < 0x26ba1c}
    code = _Code(image, '.text')
    collection_coverage = {}
    for entry in COLLECTION + (0x26a380,):
        body = set(complete_body(code, entry).code)
        collection_coverage[hex(entry)] = dict(sites=len(body), reached=len(body & reached),
                                               unreached=[hex(p) for p in sorted(body - reached)])
    fixture = dict(image_sha256=NEW_PIN, backend='physical-unchanged-i386-offline', owners=meta['owners'],
                   instruction_patches=[], supplied_original_outcomes=False, stop_entries=list(STOP_ENTRIES),
                   bank_encoding='sparse-runs-v1', cases=compact(rows))
    data = gzip.compress(json.dumps(fixture, separators=(',', ':')).encode(), mtime=0)
    if args.verify:
        assert data == args.verify.read_bytes(), 'Original 1BBEE0 repeat differs'
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    stops = {}
    for r in rows:
        if 'fault' not in r and r.get('stopped_at') and r['stopped_at'] != STOP:
            stops[hex(r['stopped_at'])] = stops.get(hex(r['stopped_at']), 0) + 1
    report = dict(passed=True, original_sha256=NEW_PIN, cases=len(rows), prefixes=sum('fault' in r for r in rows),
                  completed=sum(1 for r in rows if r.get('completed') and r.get('stopped_at', STOP) == STOP),
                  stopped_at_required_owner=stops, root_sites=len(roots), reached_root_sites=len(roots & reached),
                  unreached_root_sites=[hex(p) for p in sorted(roots - reached)],
                  reached_closure_sites=len(reached), original_instructions=sum(r['original_instructions'] for r in rows),
                  collection_coverage=collection_coverage,
                  collection_cases=sum(1 for r in rows if r['entry'] == 0x26ba30),
                  fixture=str(args.output), fixture_bytes=len(data), fixture_sha256=sha(args.output),
                  independent_repeat_identical=True if args.verify else None,
                  source_sha256={str(p.relative_to(ROOT)): sha(p) for p in
                                 (Path(__file__), ROOT / 'tools/observe_native_campaign_ai_vector_math_tu15.cpp')},
                  owned_scratch=str(scratch), retained_large_artifacts=[])
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: (v['reached'], v['sites'], v['unreached']) for k, v in collection_coverage.items()}))
    print(json.dumps({k: report[k] for k in ('cases', 'prefixes', 'completed', 'stopped_at_required_owner',
                                             'root_sites', 'reached_root_sites', 'unreached_root_sites')}))


if __name__ == '__main__':
    main()
