#!/usr/bin/env python3
"""Compare declared TU1.5 multiplayer callback ABIs with unchanged Xbox code.

The original CPU computes every result; no native implementation is imported.
Inputs are explicit synthetic engine/player banks and original immutable code
operands. No callee result or objective birth is supplied. Missing services stop
at their actual original entry. These are callback tests, not full matches.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import struct
from capstone import x86_const as FLAGS

from oracle_ctf_activation_tu15 import (
    ROOT, SOURCE, Original, base, sha, encoded,
    E, G, S, ROWS, OP, OR, OB, NS, DEF, PP, PR, PB, T, K, STACK, LU, RT)
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from generate_ctf_activation_tu15 import ENTRIES as CTF_ENTRIES
from generate_ctf_configuration_tu15 import ENTRIES as CONFIG_ENTRIES
from generate_ctf_activation_test_support import ENTRIES as SUPPORT_ENTRIES
from oracle_game_variants import Source as VariantSource, MENU_PIN
from inspect_multiplayer import Cache
ITERATOR_ENTRIES = (0x1a8f10,)

VARIANT_SPEC = dict(path=SOURCE['u5']['path'], pin=SOURCE['u5']['pin'],
                    globals=0x4ecc1c, init=0x1a6d40, setter=0x23ddae,
                    getter=0x23dee0, normalize=0x1a7170)

REGISTRATION = ((1, 0x47d174), (2, 0x4822a0), (3, 0x4822a4),
                (4, 0x4822a8), (7, 0x47a2f8), (8, 0x4822ac), (9, 0x47d174))


def defined_flags_mask(code, trace):
    # Follow instruction-declared arithmetic flag definitions. IMUL/IDIV
    # and bit tests do not define all six. This is conservative for variable
    # shift counts: Capstone does not distinguish their runtime count here.
    mask = 0x8d5
    for pc in trace:
        instruction = code.at(pc)
        if instruction.mnemonic in ('mul', 'imul'):
            # Xbox-era Intel SDM Vol.2A IMUL/MUL: SF/ZF/AF/PF are
            # undefined; only CF/OF are defined. This Capstone build instead
            # labels IMUL's SF as modified, so its metadata is insufficient.
            mask = (mask & ~0xd4) | 0x801
            continue
        effects = instruction.eflags
        for flag, bit in (('CF', 1), ('PF', 4), ('AF', 16), ('ZF', 64), ('SF', 128), ('OF', 2048)):
            if effects & getattr(FLAGS, 'X86_EFLAGS_UNDEFINED_' + flag, 0):
                mask &= ~bit
            elif effects & sum(getattr(FLAGS, 'X86_EFLAGS_' + effect + '_' + flag, 0)
                               for effect in ('MODIFY', 'RESET', 'SET')):
                mask |= bit
    return mask


def registry(image):
    engines = []
    for source_type, cell in REGISTRATION:
        obj, = struct.unpack('<I', image.read(cell, 4))
        vtable, = struct.unpack('<I', image.read(obj, 4))
        methods = struct.unpack('<51I', image.read(vtable, 204))
        engines.append((source_type, obj, vtable, methods))
    return engines


def callback_banks(image, engines, immutable):
    banks = base(image)
    for _, obj, vtable, _ in engines:
        for address, size in ((obj, 4), (vtable, 204)):
            if address not in banks.rows:
                banks.add(address, image.read(address, size))
    # Explicit synthetic ABI inputs. Actual original initializers still do all
    # initialization/publication stores and select the original registered mode.
    for address in (0x51df9c, 0x52d898, 0x52d89c, 0x52d8a0, 0x52d8a4):
        banks.add(address, encoded(K))
    banks.add(0x515dc8, bytes(2))
    banks.add(0x515dd0, bytes(16))
    banks.add(0x4f3cd8, encoded(0x4049000))
    banks.add(0x4049000, encoded(0x12345678))
    # These are the existing native world's exact loader-zeroed activation
    # records (native_tu15_world_memory.cpp), not service responses. Whole
    # shared15EBA0 updates the view counters and consumes the queue count.
    banks.add(0x4f63c0, bytes(50 * 4))
    banks.add(0x50ee28, bytes(40 * 4))
    # Existing final-startup owner: the time-extension announcement latch
    # written by whole164EF0 during the shared host frame.
    banks.add(0x56e224, bytes(1))
    # Whole King reset2C7720 copies the original three-float constant
    # through this file-backed pointer; the pointed record is readonly.
    banks.add(0x474c70, image.read(0x474c70, 4))
    immutable = set(immutable)
    immutable.add((struct.unpack('<I', image.read(0x474c70, 4))[0], 12))
    banks.w(S + 0x118, 0)
    banks.w(PP + 0x24, 0x21c)
    banks.w(PP + 0x38, 0)
    banks.w(PB, 0)
    banks.w(PR + 0x2c, -1)
    banks.w(PR + 0x2, 0, 2)
    banks.w(T + 2, 30, 2)
    # Actual configure_session multiplayer kind and packed host/tick-rate
    # inputs. The variants below occupy the original options+13C contract.
    banks.w(G + 8, 2)
    banks.w(G + 0xc, 1 | (30 << 16))
    banks.w(G + 0x18, 0x12345678)
    banks.raw(K, bytes(0x208))
    # Merge only authenticated readonly operand bytes, never mutable globals
    # or an undeclared service's storage. Existing explicit records win.
    pending = {}
    for address, size in immutable:
        for offset, value in enumerate(image.read(address, size)):
            at = address + offset
            if not any(p <= at < p + len(raw) for p, raw in banks.rows.items()):
                pending[at] = value
    while pending:
        start = min(pending)
        end = start
        raw = bytearray()
        while end in pending:
            raw.append(pending.pop(end))
            end += 1
        banks.add(start, raw)
    return banks


def authored_variants():
    source = VariantSource(VARIANT_SPEC)
    menu = ROOT / 'assets/disc/maps/mainmenu.map'
    assert sha(menu.read_bytes()) == MENU_PIN
    catalogue = Cache(menu).inspect_variants()['variants']
    result = []
    for row in catalogue:
        source.init(row['engine'])
        for parameter, value in row['overrides'].items():
            source.set(parameter, value)
        raw = source.raw()
        from unicorn.x86_const import UC_X86_REG_EBX
        from oracle_game_variants import VARIANT
        valid = source.call(VARIANT_SPEC['normalize'], registers=((UC_X86_REG_EBX, VARIANT),))
        assert valid & 255 and source.raw() == raw, row['name']
        result.append(dict(name=row['name'].replace('\ue008', '').strip(),
            frontend_engine=row['engine'], raw=bytes.fromhex(raw),
            writes=list(row['overrides'].items()),
            source_type=struct.unpack_from('<I', bytes.fromhex(raw), 0x44)[0]))
    assert len(result) == 39
    return result


def occupied_players(banks, *, teams, inside_mask=0, inside_ticks=30, score=0, alive=False):
    banks.w(PP + 0x20, 16)
    banks.w(PP + 0x38, 2)
    banks.w(PB, 3)
    banks.w(LU, 2)
    for slot in range(2):
        address = PR + slot * 0x21c
        banks.raw(address, bytes(0x21c))
        banks.w(address, 0x101 + slot, 2)
        for offset in (0x1a, 0x1c, 0x28):
            banks.w(address + offset, -1, 2)
        for offset in (0x20, 0x24, 0x30):
            banks.w(address + offset, -1)
        banks.w(address + 0x2c, 0x1010000 + slot * 0x10001 if alive else -1)
        banks.w(address + 0xc0, slot if teams else -1, 1)
        banks.w(address + 0x1b8, inside_ticks if inside_mask & (1 << slot) else 0, 2)
        banks.w(E + 0x304 + slot * 0x1c + 4, score if slot == 0 else 0, 2)
        banks.w(E + 0x304 + 0x1c4 + slot * 0x12, score if teams and slot == 0 else 0, 2)


def carrier_objects(banks, *, item_type, active, scalar):
    # Explicit caller-owned unit and equipped objective records. Whole
    # 1A8FD0 resolves the item through the real unit slots, object headers and
    # definition subtype. No equipment query or dirty callback is supplied.
    occupied_players(banks, teams=False, alive=True)
    for slot in range(2):
        address = OB + slot * 0x500
        banks.w(OR + slot * 12 + 8, address)
        banks.w(address + 0x14, -1)
        banks.w(address + 0xd4, -1)  # Original nonreplicated entity contract.
        banks.w(address + 0x134, 8 if active else 0)
        banks.w(address + 0x1fc, -1, 2)
        banks.w(address + 0x212, 0, 1)
        for item in range(4):
            banks.w(address + 0x218 + item * 4, 0x1030002 if item == 0 else -1)
        banks.raw(address + 0x2b0, struct.pack('<f', scalar))
    banks.w(OR + 2 * 12 + 8, OB + 0xa00)
    banks.w(OB + 0xa00, 0xa0010001)
    banks.w(OB + 0xa00 + 0x17e, 1, 2)
    banks.w(DEF + 0x290, item_type, 2)


def mcc_objective_markers():
    path = ROOT / 'tests/fixtures/ctf_initialization_original.json'
    row = next(m for m in json.loads(path.read_text())['maps'] if m['name'] == 'mcc-lockout')
    raw = bytearray(bytes.fromhex(row['fill_row_hex']) * row['count'])
    for patch in row['patches']:
        raw[patch['index'] * 32:(patch['index'] + 1) * 32] = bytes.fromhex(patch['row_hex'])
    assert sha(raw) == row['mcc_source']['rows_sha256']
    return bytes(raw)


def placed_bomb_banks(banks, variant, *, seconds, subticks=30, ready=True, client=False):
    """Explicit placed-item caller state, without a home/effect/damage tag.

    This tests the countdown/expiry/score owner with real goal geometry. It
    does not substitute a placement, object factory, effect or movement result.
    The missing-home branch is intentional and cannot certify item return.
    """
    carrier_objects(banks, item_type=1, active=True, scalar=0.75)
    occupied_players(banks, teams=True, alive=False)
    markers = mcc_objective_markers()
    banks.w(S + 0x118, len(markers) // 32)
    banks.raw(ROWS, markers)
    for slot in range(9):
        for offset in (0x2, 0x14, 0x26):
            banks.w(K + offset + slot * 2, -1, 2)
    banks.w(K + 0x38, -1)
    for team in range(2):
        marker = next(i for i in range(len(markers) // 32)
                      if struct.unpack_from('<hhh', markers, i * 32 + 0x10) == (3, team, 0))
        banks.w(K + 0x14 + team * 2, marker, 2)
        if team == 1:
            banks.raw(OB + 0xa00 + 0x64, markers[marker * 32:marker * 32 + 12])
    # Runtime globals supplies the real optional-tag contract consumed by
    # 24B950. NONE tags select the absent presentation/damage branches.
    banks.rows[RT].extend(bytes(0x538 - len(banks.rows[RT])))
    constants = 0x404a000
    banks.w(RT + 0x534, constants)
    banks.add(constants, bytes(0x100))
    banks.raw(constants + 0xd8, struct.pack('<f', 2.0))
    for offset in (0xe0, 0xe8, 0xf0):
        banks.w(constants + offset, -1)
    index = 8 if struct.unpack_from('<I', variant['raw'], 0x104)[0] == 2 else 0
    banks.w(OB + 0xa00 + 0x14, -1)
    banks.w(OB + 0xa00 + 4, 0x100 if ready else 0)
    banks.w(OB + 0xa00 + 0x17e, index, 2)
    banks.w(OB + 0xa00 + 0xd4, -1)
    banks.w(E + 0xc18, 1)
    banks.raw(E + 0xc1c, struct.pack('<IHHII', 0x1030002, index, 0, 0xffffffff, 0xffffffff))
    banks.w(K + 0x170 + index, 11, 1)
    banks.w(K + 0x114 + index * 2, seconds, 2)
    banks.w(K + 0x126 + index * 2, subticks, 2)
    banks.w(K + 0x17c + index * 4, 0x1010000)
    banks.w(G + 0xc, 4 if client else 1, 1)


def cases(image, engines, immutable, variants):
    output = []
    def add(source_type, methods, slot, label, *, args=(), regs=None, playing=False,
            setup=None, variant=None, entry=None, observe_scores=False, observe_objects=False,
            ticks=1, objective_index=None, move_at_tick=None, position=None, initialize=None):
        banks = callback_banks(image, engines, immutable)
        banks.w(G + 0x180, source_type)
        if variant:
            assert variant['source_type'] == source_type
            banks.raw(G + 0x13c, variant['raw'])
        banks.w(E + 0xc14, source_type)
        banks.w(E + 0x6c, int(playing), 2)
        banks.w(E + 0xc04, int(playing))
        if setup:
            setup(banks)
        obj = next(obj for kind, obj, _, _ in engines if kind == source_type)
        gp = [0, obj, 0, 0, STACK, 0, 0, ROWS + 0x800]
        if regs:
            for index, value in regs.items():
                gp[index] = value & 0xffffffff
        registration_gp = [*gp]
        registration_gp[4] = STACK
        for i in range(16):
            banks.w(STACK + 4 + i * 4, 0)
        for i, value in enumerate(args):
            banks.w(STACK + 4 + i * 4, value)
        output.append(dict(name=f'{source_type}-slot{slot}-{label}',
            source_type=source_type, slot=slot, entry=methods[slot] if entry is None else entry,
            banks=banks, gp=gp, registration_gp=registration_gp,
            initialize=(playing and source_type in (1, 9)) if initialize is None else initialize,
            variant_name=variant['name'] if variant else None,
            ticks=ticks, tick_clock_address=T + 8, objective_index=objective_index,
            move_at_tick=move_at_tick, position_address=OB + 0x64, position_hex=position,
            observe_scores=observe_scores, observe_objects=observe_objects,
            observe_banks=((E, PR, G) if observe_scores else ()) + ((OB,) if observe_objects else ())
                          + ((LU,) if entry == 0x14e73c else ())))
    for source_type, _, _, methods in engines:
        def fresh_inputs(b, left=False, lives=None, teams=False):
            # Declared allocated input, not a supplied16E7F0 response. Native
            # world acceptance separately executes the actual DatumPool owner.
            occupied_players(b, teams=teams)
            b.w(PP + 0x38, 1);b.w(PB, 1);b.w(LU, 0)
            b.w(PR + 0x1ac, 0x4321, 2)
            b.w(E + 0x304 + 4, 0x12345678)
            for slot in range(16):b.w(E + 0x2c + slot * 4, -1)
            b.w(T + 8, 0)
            # Whole19AE70's original32A660 stack probe touches its real
            # 0x4568-byte caller stack allocation before the offline guard.
            b.add(0x3002000, bytes(0x5000))
            b.add(0x4dc040, bytes(1))
            # File-backed policy words read by whole15ABC0's team census.
            b.add(0x47a388, image.read(0x47a388, 4))
            creation=bytearray(0xac)
            creation[1]=int(left)
            creation[2:8]=bytes([255])*6
            creation[8:14]=bytes.fromhex('102030405060')
            creation[14:26]=bytes.fromhex('0102030405060708090a0b0c')
            creation[0x1c + 0x7c]=0 if teams else 255
            b.raw(ROWS + 0x800, creation)
            b.w(STACK - 4, 0x13579bdf)
            if lives is not None:b.w(G + 0x1b8, lives)
        fresh_regs={0:0x1010000,3:ROWS + 0x800,4:STACK - 4,6:0x13579bdf}
        for lives in (0, 1, 3):
            add(source_type,methods,-1,f'fresh-player-lives-{lives}',entry=0x14e73c,
                regs=fresh_regs,setup=lambda b,lives=lives:fresh_inputs(b,lives=lives),observe_scores=True)
        add(source_type,methods,-1,'fresh-player-left',entry=0x14e73c,
            regs=fresh_regs,setup=lambda b:fresh_inputs(b,left=True,lives=3),observe_scores=True)
        add(source_type,methods,-1,'fresh-player-allocation-failed',entry=0x14e73c,
            regs={**fresh_regs,0:0xffffffff},setup=fresh_inputs,observe_scores=True)
        for variant in (v for v in variants if v['source_type']==source_type):
            teams=bool(variant['raw'][0x48]&1)
            add(source_type,methods,-1,'fresh-player-'+variant['name'],entry=0x14e73c,
                regs=fresh_regs,variant=variant,
                setup=lambda b,teams=teams:fresh_inputs(b,teams=teams),observe_scores=True)
        for slot in (0, 1, 3, 4, 9, 18):
            add(source_type, methods, slot, 'cold')
        add(source_type, methods, 18, 'playing-empty-roster', playing=True)
        def frame_inputs(b, mode=4):
            b.w(G + 0xc, mode, 1)
            b.w(OP + 0x38, 0)
            b.w(E, 0)
            b.w(E + 0xc0c, 0)
            b.w(E + 0x70, 0)
            b.w(E + 0xbfc, 0)
            b.w(T + 8, 1)
            b.raw(T + 4, struct.pack('<f', 1 / 30))
            if mode != 4:
                # Whole15D890 obtains this converted globals reference before
                # testing the empty authored netgame-equipment block.
                b.rows[RT].extend(bytes(0x538 - len(b.rows[RT])))
                b.w(RT + 0x534, 0x404a000)
                b.add(0x404a000, bytes(0x100))
        add(source_type, methods, -1, 'shared-frame-client-empty', entry=0x15eba0,
            playing=True, setup=frame_inputs, observe_scores=True)
        if source_type in (2, 4):
            add(source_type, methods, -1, 'shared-frame-host-empty', entry=0x15eba0,
                playing=True, setup=lambda b:frame_inputs(b, 1), observe_scores=True)
        # Original13AAC0's later phase consumes pending states after objects.
        # Declare phase/timer inputs explicitly; every transition executes its
        # unchanged real callback and service owners, with empty object pools.
        def phase_inputs(b, phase=1, requested=1, timer=3, mode=1, round_index=0):
            frame_inputs(b, mode)
            b.w(E + 0x6c, phase, 2)
            b.w(E + 0x6e, round_index, 2)
            b.w(E + 0xc04, requested)
            b.w(E + 0xc00, timer)
            b.w(E + 0x24, -1)
            b.w(E + 0x28, -1)
            b.w(G + 0x1128, 0, 1)
            b.w(0x51dfa0, 0, 1)
        for phase in (0, 1, 2, 3):
            for mode in (1, 4):
                add(source_type, methods, -1, f'post-phase-stable-{phase}-{mode}',
                    entry=0x15f450, setup=lambda b,phase=phase,mode=mode:phase_inputs(
                        b,phase=phase,requested=phase,mode=mode), observe_scores=True)
        for requested in (2, 3):
            add(source_type, methods, -1, f'post-phase-request-{requested}',
                entry=0x15f450, setup=lambda b,requested=requested:phase_inputs(
                    b,requested=requested), observe_scores=True)
        for round_index in (0, 1, 30, 31):
            add(source_type, methods, -1, f'post-phase-intermission-expiry-{round_index}',
                entry=0x15f450, setup=lambda b,round_index=round_index:phase_inputs(
                    b,phase=2,requested=2,timer=1,round_index=round_index), observe_scores=True)
        # Host objective restarts reach the real item factory even with no
        # markers. They require the native world's real tag/object services;
        # the isolated composition supplies no successful factory result.
        if source_type in (2, 4):
            add(source_type, methods, -1, 'post-phase-next-round-expiry',
                entry=0x15f450, setup=lambda b:phase_inputs(
                    b,phase=3,requested=3,timer=1,round_index=1), observe_scores=True)
            add(source_type, methods, -1, 'post-phase-complete-round-delay',
                entry=0x15f450, setup=lambda b:phase_inputs(b,requested=2),
                ticks=181, observe_scores=True)
        add(source_type, methods, -1, 'phase-consumer-client-next-round',
            entry=0x15efa0, setup=lambda b:phase_inputs(
                b,phase=3,requested=1,timer=0,round_index=1,mode=4), observe_scores=True)
        for phase in (1, 2, 3):
            def finish_inputs(b, phase=phase):
                phase_inputs(b,phase=phase,requested=phase)
                b.w(G + 0x1128, 1, 1)
                b.w(E + 0xc08, 0, 1)
            add(source_type, methods, -1, f'post-phase-finalize-once-{phase}',
                entry=0x15f450, setup=finish_inputs, ticks=2, observe_scores=True)
        for mode in (1, 4):
            def late_inputs(b, mode=mode, occupied=False, cooldown=0):
                phase_inputs(b,mode=mode)
                b.w(LU + 0xa8, -1)
                if occupied:
                    occupied_players(b,teams=False,alive=False)
                    for slot in range(2):
                        player=PR + slot * 0x21c
                        b.w(player + 0x16c, cooldown)
                        b.w(player + 0x3f, cooldown, 1)
                        b.w(player + 0x40, cooldown, 1)
                        b.w(player + 0x180, cooldown, 2)
                        b.w(player + 3, 8, 1)
                        b.w(E + 0x2c + slot * 4, -1)
            add(source_type, methods, -1, f'late-player-empty-{mode}',
                entry=0x1503d0, setup=late_inputs, observe_scores=True)
            for cooldown in (0, 1, 3):
                add(source_type, methods, -1, f'late-player-absent-units-{mode}-{cooldown}',
                    entry=0x1503d0, setup=lambda b,mode=mode,cooldown=cooldown:late_inputs(
                        b,mode=mode,occupied=True,cooldown=cooldown), observe_scores=True)
        for limit in (3, 4, 16):
            for teams in (False, True):
                def limit_inputs(b,limit=limit,teams=teams):
                    occupied_players(b,teams=teams,alive=False)
                    b.w(PP + 0x38,4);b.w(PB,15);b.w(LU,4)
                    b.w(G + 0x1b4,limit);b.w(G + 0x1b8,0)
                    b.w(G + 0x184,int(teams),1)
                    for slot in range(4):
                        player=PR + slot * 0x21c;b.raw(player,bytes(0x21c))
                        b.w(player,0x101+slot,2);b.w(player+2,1,2)
                        b.w(player+0xc0,slot%2 if teams else slot,1)
                        # Explicit caller current-unit identities. This owner
                        # tests their availability and never reads object data.
                        b.w(player+0x2c,0x1010000+slot if slot<3 else -1)
                add(source_type,methods,-1,f'active-player-limit-{limit}-{int(teams)}',
                    entry=0x1601b0,regs={0:0x1040003},setup=limit_inputs,observe_scores=True)
        for mode in (1, 4):
            def setup(b, mode=mode):
                b.w(G + 0xc, mode, 1)
                b.w(G + 0x184, 1, 1)  # Preserve-world later round.
                b.w(OP + 0x38, 0)
            add(source_type, methods, -1, f'round-preserve-world-empty-{mode}',
                entry=0x15c840, setup=setup)
        for mode in (1, 4):
            for entry in (0x13b340, 0x13b360):
                add(source_type, methods, -1, f'authority-{entry:x}-{mode}', entry=entry,
                    setup=lambda b, mode=mode:b.w(G + 0xc, mode, 1))
        for trait in range(16):
            add(source_type, methods, 35, f'trait-{trait}', args=(0, trait))
        for left, right in ((0, 0), (0, 1), (1, 0), (7, 7)):
            add(source_type, methods, 27, f'teams-{left}-{right}', args=(left, right))
        # Actual engine entity-bank callbacks: complete initialization,
        # difference-mask publication and selected-bit application. Their
        # common prefix and any dirty callback execute as original code.
        add(source_type, methods, 44, 'initialize-entity-bank', args=(0, ROWS + 0x800))
        add(source_type, methods, 45, 'difference-entity-bank',
            args=(ROWS + 0xf00, 0, ROWS + 0x800))
        for mask in (0, 1, 0x1f, 0x3f, 0xffffffff):
            add(source_type, methods, 46, f'apply-entity-bank-{mask:x}',
                args=(mask, 0, ROWS + 0x800))
        if source_type == 7:
            for role_mask in (0, 1, 2, 0x8000, 0xffff):
                for left, right in ((0, 0), (0, 1), (1, 0), (7, 15)):
                    add(source_type, methods, 27, f'role-mask-{role_mask:x}-{left}-{right}',
                        args=(left, right), setup=lambda b,m=role_mask:b.w(K + 2, m, 2))
    for variant in variants:
        source_type = variant['source_type']
        methods = next(methods for kind, _, _, methods in engines if kind == source_type)
        for slot in (1, 35):
            add(source_type, methods, slot, 'authored-' + variant['name'],
                args=(0, 3) if slot == 35 else (), variant=variant,
                setup=lambda b:occupied_players(b, teams=bool(variant['raw'][0x48] & 1)) if slot == 35 else None)
        if source_type == 3:
            for mode in (1, 4):
                for enabled in (False, True):
                    for active in (False, True):
                        for scalar in (0.25, 0.75):
                            def setup(b, mode=mode, enabled=enabled, active=active, scalar=scalar):
                                carrier_objects(b, item_type=2, active=active, scalar=scalar)
                                b.w(G + 0xc, mode, 1)
                                flags = struct.unpack_from('<I', variant['raw'], 0xf0)[0]
                                b.w(G + 0x22c, (flags & ~2) | (2 if enabled else 0))
                            add(3, methods, 15, f'carrier-{variant["name"]}-{mode}-{int(enabled)}-{int(active)}-{scalar}',
                                args=(0x1010000,), variant=variant, setup=setup, observe_objects=True)
        if source_type in (1, 9):
            for mode in (1, 4):
                for enabled in (False, True):
                    def setup(b, mode=mode, enabled=enabled):
                        carrier_objects(b, item_type=1, active=True, scalar=0.75)
                        for slot in range(2):
                            b.w(PR + slot * 0x21c + 0xc0, slot, 1)
                        b.w(G + 0xc, mode, 1)
                        kind = struct.unpack_from('<I', variant['raw'], 0x104)[0]
                        b.w(OB + 0xa00 + 0x17e, 8 if source_type == 9 and kind == 2 else
                            (0 if source_type == 9 else 1), 2)
                        # CTF/Assault's carrier trait1 is bit7; Oddball's
                        # corresponding selector above consumes bit1.
                        flags = struct.unpack_from('<I', variant['raw'], 0xf0)[0]
                        b.w(G + 0x22c, (flags & ~0x80) | (0x80 if enabled else 0))
                    add(source_type, methods, 15, f'carrier-{variant["name"]}-{mode}-{int(enabled)}',
                        args=(0x1010000,), variant=variant, setup=setup, playing=True, observe_objects=True)
            # Original15E1E0/15E070 compare the score against options+18C,
            # i.e. the variant's DWORD+50; +54 is the time limit instead.
            limit = struct.unpack_from('<I', variant['raw'], 0x50)[0]
            for mode in (1, 4):
                for score in sorted({0, max(0, limit - 1)}):
                    def setup(b, mode=mode, score=score):
                        occupied_players(b, teams=bool(variant['raw'][0x48] & 1), score=score)
                        b.w(G + 0xc, mode, 1)
                    add(source_type, methods, -1, f'capture-score-{variant["name"]}-{mode}-{score}',
                        args=(1,), regs={0:0x1010000}, variant=variant, playing=True,
                        entry=0x24c0f0, setup=setup, observe_scores=True)
        if source_type == 9:
            fuse = struct.unpack_from('<h', variant['raw'], 0x10a)[0] or 10
            index = 8 if struct.unpack_from('<I', variant['raw'], 0x104)[0] == 2 else 0
            for label, ticks, ready, client in (
                    ('before-expiry', fuse * 30 - 1, True, False),
                    ('expiry', fuse * 30, True, False),
                    ('not-ready-delays-expiry', fuse * 30 + 1, False, False),
                    ('client-authority', 2, True, True)):
                add(9, methods, -1, f'fuse-no-home-{variant["name"]}-{label}',
                    entry=0x249b20, args=(0x1030002,), variant=variant, playing=True,
                    initialize=False, ticks=ticks, objective_index=index, observe_scores=True,
                    setup=lambda b, ready=ready, client=client:placed_bomb_banks(
                        b, variant, seconds=fuse, ready=ready, client=client))
            markers = mcc_objective_markers()
            # Actual authored enemy bomb-goal origin, with whole249460
            # choosing eligible centers/bounds from the original32-byte rows.
            goal = next(markers[i:i+12] for i in range(0, len(markers), 32)
                        if struct.unpack_from('<hhh', markers, i + 0x10) == (3, 1, 0))
            seconds = struct.unpack_from('<h', variant['raw'], 0x108)[0]
            arm_ticks = max(1, 30 * seconds)
            index = 8 if struct.unpack_from('<I', variant['raw'], 0x104)[0] == 2 else 0
            inside = max(1, arm_ticks // 2)
            sequences = [(ticks, None) for ticks in sorted({1, max(1, arm_ticks - 1), arm_ticks})]
            sequences += [(inside + outside, inside) for outside in (1, (inside + 3) // 3 + 1)]
            outside = struct.pack('<3f', *(v + 1000 for v in struct.unpack('<3f', goal))).hex()
            for ticks, move_at_tick in sequences:
                def setup(b):
                    carrier_objects(b, item_type=1, active=True, scalar=0.75)
                    for slot in range(2):
                        b.w(PR + slot * 0x21c + 0xc0, slot, 1)
                    b.w(OB + 0xa00 + 0x17e, index, 2)
                    b.w(S + 0x118, len(markers) // 32)
                    b.raw(ROWS, markers)
                    b.raw(OB + 0x64, goal)
                suffix = f'{ticks}' if move_at_tick is None else f'{inside}-inside-{ticks-inside}-outside'
                add(9, methods, 15, f'arming-mcc-lockout-{variant["name"]}-{suffix}',
                    args=(0x1010000,), variant=variant, playing=True, setup=setup,
                    ticks=ticks, objective_index=index, observe_scores=True, observe_objects=True,
                    move_at_tick=move_at_tick, position=outside)
    methods = next(m for kind, _, _, m in engines if kind == 2)
    for entry in (0x248ec0, 0x248ef0):
        for criteria in (0x466164, 0x466170, 0x466180, 0x466188):
            for tie in (0, 1):
                for score in (-1, 0, 1):
                    for reverse in (False, True):
                        def setup(b, criteria=criteria, tie=tie, score=score):
                            occupied_players(b, teams=True, score=score)
                            for stat in range(0, 0x12, 2):
                                b.w(E + 0x304 + 0x1c4 + stat, score, 2)
                            for stat in range(4, 0x14, 2):
                                b.w(E + 0x304 + stat, score, 2)
                            b.w(ROWS + 0x800, criteria)
                            b.w(ROWS + 0x804, tie, 1)
                        ids = (0x1020001, 0x1010000) if reverse else (0x1010000, 0x1020001)
                        # The team callback's inputs are team indices; the
                        # player callback receives full salted player IDs.
                        if entry == 0x248ef0:
                            ids = (1, 0) if reverse else (0, 1)
                        add(2, methods, -1, f'ranking-{entry:x}-{criteria:x}-{tie}-{score}-{int(reverse)}',
                            entry=entry, args=(*ids, ROWS + 0x800), setup=setup)
    methods = next(m for kind, _, _, m in engines if kind == 4)
    for vector in ((0.0, 0.0), (3.0, 4.0), (-3.0, 4.0)):
        add(4, methods, -1, f'engine-vector-normalize-{vector}', entry=0x2b3e0,
            regs={0:ROWS + 0x800}, setup=lambda b, vector=vector:b.raw(ROWS + 0x800, struct.pack('<2f', *vector)))
    for user in (-1, 0):
        for species in (0, 1, 2, 3):
            def setup(b, species=species):
                occupied_players(b, teams=False)
                b.w(LU + 0xc, 0x1010000)
                b.w(PR + 0x88, species, 1)
            add(4, methods, -1, f'engine-player-palette-{user}-{species}', entry=0x13bd5e,
                regs={1:user}, setup=setup)
    methods = next(m for kind, _, _, m in engines if kind == 4)
    for groups in ((), (10, 19), (11, 11, 18, 18, 10, 19),
                   tuple(range(11, 19)), tuple(range(18, 10, -1))):
        def setup(b, groups=groups):
            b.w(S + 0x118, len(groups))
            b.w(S + 0x11c, ROWS)
            for index, group in enumerate(groups):
                b.w(ROWS + index * 32 + 0x10, group, 2)
        add(4, methods, 1, f'king-authored-hill-groups-{groups}', setup=setup)
    for teams in (False, True):
        king = next(v for v in variants if v['name'] == ('Team King' if teams else 'King'))
        methods = next(m for kind, _, _, m in engines if kind == 4)
        for mask in (0, 1, 3):
            for ticks in (1, 29, 30, 31, 60):
                for rules in (0, 1, 2, 3):
                    def setup(b, mask=mask, ticks=ticks, rules=rules):
                        occupied_players(b, teams=teams, inside_mask=mask, inside_ticks=ticks, alive=True)
                        b.w(G + 0x22c, rules, 1)
                        if teams:
                            for team in range(8):
                                b.w(K + 0x180 + team * 4, ticks - 1)
                    add(4, methods, -1, f'king-score-{int(teams)}-{mask}-{ticks}-{rules}',
                        playing=True, variant=king, setup=setup, entry=0x2c8ca0,
                        observe_scores=True)
        oddball = next(v for v in variants if v['name'] == ('Team Ball' if teams else 'Oddball'))
        methods = next(m for kind, _, _, m in engines if kind == 3)
        for score in (0, 59, 89, 109, 118, 119):
            add(3, methods, -1, f'ball-score-{int(teams)}-{score}', playing=True,
                variant=oddball, entry=0x2c6c80, regs={6:0x1010000}, observe_scores=True,
                setup=lambda b,score=score:occupied_players(b, teams=teams, score=score))
    territories = next(v for v in variants if v['frontend_engine'] == 6)
    methods = next(m for kind, _, _, m in engines if kind == 8)
    for tick in (29, 30, 31, 60):
        for owners in ((-1, -1), (0x1010000, -1), (0x1010000, 0x1020001)):
            def setup(b, tick=tick, owners=owners):
                occupied_players(b, teams=True)
                b.w(T + 8, tick)
                b.w(K + 0x114, 2, 2)
                for slot, owner in enumerate(owners):
                    b.w(K + 0x60 + slot * 2, slot, 2)
                    b.w(K + 0x70 + slot * 4, owner)
            add(8, methods, -1, f'territory-score-{tick}-{owners}', playing=True,
                variant=territories, setup=setup, entry=0x2ca910, observe_scores=True)
    juggernaut = next(v for v in variants if v['frontend_engine'] == 2)
    methods = next(m for kind, _, _, m in engines if kind == 7)
    for role_mask in (1, 2, 3):
        for victim in (0, 1):
            for killer in (-1, victim, 1 - victim):
                def setup(b, mask=role_mask):
                    occupied_players(b, teams=False, alive=True)
                    b.w(K, mask, 2)
                killer_id = -1 if killer == -1 else 0x1010000 + killer * 0x10001
                victim_id = 0x1010000 + victim * 0x10001
                add(7, methods, 30, f'juggernaut-death-{role_mask}-{victim}-{killer}',
                    playing=True, variant=juggernaut, setup=setup,
                    args=(killer_id, victim_id, 0, 0), observe_scores=True)
    return output


def bank_patches(address, before, after):
    assert len(before) == len(after)
    changed = [i for i,(left,right) in enumerate(zip(before,after)) if left != right]
    result = []
    while changed:
        start = end = changed.pop(0)
        while changed and changed[0] <= end + 5:
            end = changed.pop(0)
        result.append(dict(address=address, offset=start, hex=bytes(after[start:end+1]).hex()))
    restored = bytearray(before)
    for patch in result:
        raw = bytes.fromhex(patch['hex'])
        restored[patch['offset']:patch['offset']+len(raw)] = raw
    assert restored == after
    return result


def score_observation(banks, source_type):
    engine, players, options = banks[E], banks[PR], banks[G]
    result = dict(player_scores=[struct.unpack_from('<h', engine, 0x308 + slot * 0x1c)[0]
                               for slot in range(2)],
        team_scores=[struct.unpack_from('<h', engine, 0x4c8 + slot * 0x12)[0]
                     for slot in range(2)],
        inside_ticks=[struct.unpack_from('<h', players, slot * 0x21c + 0x1b8)[0]
                      for slot in range(2)], phase=struct.unpack_from('<h', engine, 0x6c)[0],
        match_over=bool(options[0x1128]), requested_state=struct.unpack_from('<I', engine, 0xc04)[0],
        round_index=struct.unpack_from('<h', engine, 0x6e)[0],
        phase_timer=struct.unpack_from('<i', engine, 0xc00)[0],
        finalized=bool(engine[0xc08]), engine_flags=struct.unpack_from('<I', engine)[0])
    if source_type == 7:
        result['role_mask'], result['role_team_mask'] = struct.unpack_from('<HH', engine, 0xfc)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    assert scratch.is_dir()
    if not args.directory.resolve().is_relative_to(scratch):
        raise ValueError('Oracle intermediates must stay under HALO2_TEST_SCRATCH')
    path = Path(SOURCE['u5']['path'])
    assert sha(path.read_bytes()) == SOURCE['u5']['pin']
    image = Xbe(path)
    code = _Code(image, '.text')
    engines = registry(image)
    recovered = json.loads((args.directory / 'bodies.json').read_text())
    shared = json.loads((args.directory / 'test-services/bodies.json').read_text())
    entries = (set(int(p, 16) for p in recovered['entries']) | set(CTF_ENTRIES)
               | set(CONFIG_ENTRIES) | set(SUPPORT_ENTRIES) | set(ITERATOR_ENTRIES)
               | set(int(p, 16) for p in shared['entries']))
    starts = {int(row['address'], 16) for row in csv.DictReader(
        (ROOT / 'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),
        delimiter='\t')} | entries
    owned, immutable, bodies = set(), set(), {}
    readonly = image.section('.rdata')
    for entry in sorted(entries):
        body = _trace(code, entry, starts)
        bodies[f'{entry:08x}'] = sorted(body.code)
        owned.update(body.code)
        for address, width, count in body.tables:
            immutable.add((address, width * count))
        for pc in body.code:
            for operand in code.at(pc).operands:
                if operand.type == 3 and readonly.va <= operand.mem.disp < readonly.va + readonly.vsize:
                    immutable.add((operand.mem.disp, operand.size))
                elif operand.type == 2 and readonly.va <= operand.imm < readonly.va + readonly.vsize:
                    # Original LEA/MOV address operands include immutable RGB
                    # and vector records subsequently copied through registers.
                    immutable.add((operand.imm, min(16, readonly.va + readonly.vsize - operand.imm)))
    original = Original(image, owned)
    blobs, records, baseline = {}, [], None
    defaults = dict(xmm=[[0x3f000000 + i * 0x100 + j for j in range(4)] for i in range(8)],
                    x87=[], fs_base=0, x87_status=0, x87_control=0x27f, mxcsr=0x1f80)
    variants = authored_variants()
    for case in cases(image, engines, immutable, variants):
        try:
            expected = original.run(case)
        except Exception:
            print('FIRST ORIGINAL FAILURE', case['name'], 'pc', hex(getattr(original, 'last_pc', 0)), flush=True)
            raise
        if expected['boundary']:
            raise RuntimeError(f"Core engine case {case['name']} did not complete: {expected['boundary']:08x}")
        flags_mask = defined_flags_mask(code, original.last_entry_trace)
        expected['eflags'] &= flags_mask
        inputs, all_inputs, patches = [], [], []
        for address, raw in sorted(case['banks'].rows.items()):
            if baseline is not None and address in baseline and len(baseline[address]) == len(raw):
                patches.extend(bank_patches(address, baseline[address], raw))
            else:
                key = sha(raw)
                blobs[key] = raw.hex()
                item = dict(address=address, blob=key)
                all_inputs.append(item)
                inputs.append(item)
        if baseline is None:
            baseline = {address:bytes(raw) for address,raw in case['banks'].rows.items()}
            base_inputs, inputs = all_inputs, []
        record = dict(name=case['name'], source_type=case['source_type'],
            slot=case['slot'], entry=case['entry'], initialize=case['initialize'], gp=case['gp'],
            banks=inputs, bank_patches=patches, eflags_mask=flags_mask,
            expected={key:value for key,value in expected.items()
                      if key not in defaults or value != defaults[key]})
        if case['registration_gp'] != case['gp']:
            record['registration_gp'] = case['registration_gp']
        if case['ticks'] != 1:
            record.update(ticks=case['ticks'], tick_clock_address=case['tick_clock_address'])
        if case['move_at_tick'] is not None:
            record.update(move_at_tick=case['move_at_tick'], position_address=case['position_address'],
                          position_hex=case['position_hex'])
        if case['variant_name']:
            record['variant_name'] = case['variant_name']
        if case['observe_scores']:
            record['source_score_observation'] = score_observation(original.last_banks, case['source_type'])
        if case['observe_objects']:
            raw = original.last_banks[OB]
            record['source_object_observation'] = dict(
                scalar_2b0=struct.unpack_from('<f', raw, 0x2b0)[0],
                flags_134=struct.unpack_from('<I', raw, 0x134)[0])
        if case['entry'] == 0x1503d0:
            raw=original.last_banks[PR]
            record['source_player_timer_observation']=[dict(
                current_unit=struct.unpack_from('<I',raw,slot * 0x21c + 0x2c)[0],
                flags=struct.unpack_from('<H',raw,slot * 0x21c + 2)[0],
                action_ticks=struct.unpack_from('<I',raw,slot * 0x21c + 0x16c)[0],
                cooldown_3f=raw[slot * 0x21c + 0x3f],cooldown_40=raw[slot * 0x21c + 0x40],
                timer_180=struct.unpack_from('<H',raw,slot * 0x21c + 0x180)[0])for slot in range(2)]
        if case['entry'] == 0x14e73c:
            raw=original.last_banks[PR]
            record['source_fresh_player_observation']=dict(
                allocated_input=case['gp'][0],returned_datum=expected['gp'][0],
                restored_esi=expected['gp'][6],returned_esp=expected['gp'][4],
                lives=struct.unpack_from('<H',raw,0x1ac)[0],
                flags=struct.unpack_from('<H',raw,2)[0],
                census=struct.unpack_from('<I',original.last_banks[LU])[0])
        if case['objective_index'] is not None:
            index = case['objective_index']
            raw = original.last_banks[E]
            record['source_objective_observation'] = dict(index=index,
                arming_ticks=struct.unpack_from('<h', raw, 0xfc + 0x1a0 + index * 2)[0],
                state_flags=raw[0xfc + 0x170 + index],
                fuse_seconds=struct.unpack_from('<h', raw, 0xfc + 0x114 + index * 2)[0],
                fuse_subticks=struct.unpack_from('<h', raw, 0xfc + 0x126 + index * 2)[0],
                planter=struct.unpack_from('<I', raw, 0xfc + 0x17c + index * 4)[0])
        records.append(record)
    fixture = dict(schema=1, original_code_sha256=SOURCE['u5']['pin'],
        source_code_only=True, supplied_service_results=[], base_expected=defaults,
        base_banks=base_inputs, blobs=blobs, cases=records,
        variants=[{**variant, 'raw':variant['raw'].hex()} for variant in variants])
    args.fixture.write_text(json.dumps(fixture, separators=(',', ':')) + '\n')
    boundaries = {row['expected']['boundary'] for row in records}
    report = dict(passed=True, cases=len(records), source_sha256=SOURCE['u5']['pin'],
        oracle_sha256=sha(Path(__file__).read_bytes()),
        fixture_sha256=sha(args.fixture.read_bytes()), fixture_bytes=args.fixture.stat().st_size,
        complete_return_cases=sum(not row['expected']['boundary'] for row in records),
        unsupplied_boundaries={f'{p:08x}':sum(row['expected']['boundary'] == p for row in records)
                               for p in sorted(boundaries) if p},
        families=sorted({row['source_type'] for row in records}),
        authored_variants=len(variants), variant_inputs='Original TU1.5 initializer, descriptor setters, and validator over the 39 authored rows.',
        body_coverage={entry:dict(whole=len(sites), observed=len(set(sites) & original.executed))
                       for entry,sites in bodies.items()},
        supplied_service_results=[], original_instruction_bytes_modified=False,
        original_cpu_map_cache_loaded=False,
        offline_variant_catalogue=dict(path='assets/disc/maps/mainmenu.map', sha256=MENU_PIN),
        x87_status_compared_mask='fdc0',
        mxcsr_compared_mask='ffffffc0',
        arithmetic_flags='Instruction-declared defined output flags tracked through the original trace; variable-count shift flags are conservatively excluded.',
        scope='Declared callback ABI correspondence, not normal match/world acceptance.')
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({key:value for key,value in report.items() if key != 'body_coverage'}), flush=True)


if __name__ == '__main__':
    main()
