#!/usr/bin/env python3
"""Execute pinned Xbox Slayer defaults, option getters, mode tests and scoring.

Locale lookup is absent (only the name is omitted). Scoring records calls to the
retail score service rather than modeling its replication or end-of-round work.
Weapon creation skips its animation reset service, preserving magazine math.
This is an instruction oracle, not a live Xbox match or network-ABI test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from inspect_multiplayer import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX,
                              UC_X86_REG_EBX, UC_X86_REG_EDX, UC_X86_REG_EDI,
                              UC_X86_REG_FPCW, UC_X86_REG_MXCSR)

PIN = '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
VARIANT, ENGINE_GLOBALS, OPTIONS = 0x1000000, 0x1010000, 0x1020000
POOL, PLAYERS, MODES, TIME = 0x1030000, 0x1040000, 0x1050000, 0x1060000
GLOBALS, REPRESENTATIONS, RESULT = 0x1070000, 0x1080000, 0x1090000
STACK, STOP = 0x200ff00, 0x3000000


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    x = Xbe(ROOT / 'assets/disc/default.xbe')
    assert hashlib.sha256(x.data).hexdigest() == PIN
    g = Uc(UC_ARCH_X86, UC_MODE_32)
    g.mem_map(0x10000, 0x600000)
    for section in x.sections:
        g.mem_write(section.va, x.section_bytes(section))
    g.mem_map(VARIANT, 0x100000)
    g.mem_map(0x2000000, 0x10000)
    g.mem_map(STOP, 0x1000)

    def w(at, value): g.mem_write(at, struct.pack('<I', value & 0xffffffff))
    def r(at): return struct.unpack('<I', g.mem_read(at, 4))[0]
    def signed(value): return value if value < 0x80000000 else value - 0x100000000

    def call(address, argv=(), registers=(), stop=STOP):
        w(STACK, STOP)
        for i, value in enumerate(argv): w(STACK + 4 + i * 4, value)
        g.reg_write(UC_X86_REG_ESP, STACK)
        g.reg_write(UC_X86_REG_FPCW, 0x027f)
        g.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        for reg, value in registers: g.reg_write(reg, value)
        g.emu_start(address, stop, count=200000)
        assert g.reg_read(UC_X86_REG_EIP) == stop
        return signed(g.reg_read(UC_X86_REG_EAX))

    score_calls = []

    def hook(uc, address, size, _):
        if address == 0x15b7c0:
            sp = uc.reg_read(UC_X86_REG_ESP)
            score_calls.append(dict(player=signed(r(sp + 4)),
                                    delta=signed(uc.reg_read(UC_X86_REG_EAX))))
            uc.reg_write(UC_X86_REG_EIP, r(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 8)
        elif address == 0x1058b0:  # weapon animation reset is outside ammo math
            sp = uc.reg_read(UC_X86_REG_ESP)
            uc.reg_write(UC_X86_REG_EIP, r(sp))
            uc.reg_write(UC_X86_REG_ESP, sp + 16)

    g.hook_add(UC_HOOK_CODE, hook)
    w(0x4e034c, 0)  # no locale/tag services: gameplay settings stay untouched.
    call(0x19d220, (VARIANT, 0))
    settings = bytes(g.mem_read(VARIANT, 0x130))
    parameters = {}
    for parameter in range(0x70):
        parameters[parameter] = call(0x2374f0, registers=[(UC_X86_REG_EAX, parameter),
                                                        (UC_X86_REG_EDI, VARIANT)])
    assert parameters[2] == 25 and parameters[10] == 480 and parameters[17] == 5
    assert parameters[18] == 5 and parameters[29] == 1 and parameters[91] == 0

    w(0x4e9ae8, ENGINE_GLOBALS)
    w(ENGINE_GLOBALS + 0xc14, 2)
    w(0x55e4d0 + 2 * 4, 0x47fc80)
    modes = []
    for entry in range(15):
        g.mem_write(MODES, struct.pack('<4h', entry, 0, 0, 0))
        result = call(0x15a090, (4,), [(UC_X86_REG_EDX, 2), (UC_X86_REG_EDI, MODES)])
        modes.append(dict(entry=entry, slayer_matching_slot=result))
    assert [m['entry'] for m in modes if m['slayer_matching_slot'] >= 0] == [2, 12, 13, 14]

    w(0x4e6948, OPTIONS)
    w(0x4e8c24, POOL)
    w(POOL + 0x44, PLAYERS)
    g.mem_write(OPTIONS + 12, b'\x02')
    g.mem_write(OPTIONS + 0x22c, struct.pack('<I', 2))
    scoring = []
    for name, killer, victim, flag in [('kill', 0, 1, False),
                                      ('suicide', 0, 0, True),
                                      ('betrayal', 0, 1, True),
                                      ('uncredited_death', -1, 1, False)]:
        score_calls.clear()
        call(0x2bc000, (killer, victim, int(flag), -1))
        scoring.append(dict(case=name, calls=list(score_calls)))
    assert scoring[0]['calls'] == [dict(player=0, delta=1)]
    assert scoring[1]['calls'] == [dict(player=0, delta=-1)]
    assert scoring[2]['calls'] == [dict(player=0, delta=-1)]
    assert scoring[3]['calls'] == []
    w(0x510c54, TIME)
    g.mem_write(TIME + 2, struct.pack('<h', 30))
    w(TIME + 8, 300)
    w(OPTIONS + 0x1bc, 5)
    w(OPTIONS + 0x1c0, 5)
    w(OPTIONS + 0x1e8, 10)
    respawn = []
    for name, attacker, victim, prior_penalty, teams in [
            ('normal_death', 0, 1, 0, (0, 1)),
            ('suicide', 0, 0, 0, (0, 1)),
            ('betrayal_victim', 0, 1, 0, (0, 0)),
            ('betrayer_subsequent_death', -1, 0, 300, (0, 1))]:
        g.mem_write(PLAYERS, bytes(2 * 0x21c))
        for i, team in enumerate(teams):
            g.mem_write(PLAYERS + i * 0x21c + 0xc0, bytes([team]))
        w(PLAYERS + victim * 0x21c + 0x168, prior_penalty)
        # Stop immediately after retail computes and stores the countdown.
        # Death markers, medal events and score calls occur later and are absent.
        call(0x15ce70, (attacker, victim, int(name == 'betrayal_victim'), 0),
             stop=0x15d151)
        respawn.append(dict(case=name,
                            victim_ticks=r(PLAYERS + victim * 0x21c + 0x164),
                            attacker_pending_penalty=r(PLAYERS + 0x168)))
    assert [row['victim_ticks'] for row in respawn] == [150, 300, 150, 450]
    assert respawn[2]['attacker_pending_penalty'] == 300
    w(0x4e034c, GLOBALS)
    w(GLOBALS + 0x138, 4)
    w(GLOBALS + 0x13c, REPRESENTATIONS)
    for i in range(4):
        w(REPRESENTATIONS + i * 0xbc + 0xb4, 0x100 + i)
        w(REPRESENTATIONS + i * 0xbc + 0xb8, 0x200 + i)
    representation = []
    w(OPTIONS + 8, 2)
    for appearance in (0, 1, 2, 3):
        g.mem_write(PLAYERS + 0x88, bytes([appearance]))
        call(0x14f040, (RESULT + 4,), [(UC_X86_REG_EAX, 0),
                                     (UC_X86_REG_EBX, RESULT)])
        representation.append(dict(appearance=appearance,
                                   selected_slot=r(RESULT) - 0x100))
    assert [row['selected_slot'] for row in representation] == [2, 3, 2, 3]

    # Run the full retail weapon creation callback with an authored SMG
    # magazine and two boundary cases. The object/tag pools are supplied;
    # weapon animation reset is skipped by the hook above.
    object_pool, headers, weapon, tags, definition, magazine = [
        VARIANT + 0xa000 + i * 0x1000 for i in range(6)]
    w(0x4e0300, object_pool); w(object_pool + 0x44, headers)
    w(headers + 8, weapon); w(weapon, 0)
    w(0x4e3b44, tags); w(tags + 8, definition)
    w(definition + 0x2c0, 1); w(definition + 0x2c4, magazine)
    ammunition = []
    for initial, maximum, capacity, inventory in ((180,240,60,180),
                                                 (20,240,60,180),
                                                 (300,240,60,180)):
        g.mem_write(magazine + 6, struct.pack('<4h', initial, maximum, capacity, inventory))
        call(0xfdad0, (0, 0, 0))
        unloaded, loaded = struct.unpack('<2h', g.mem_read(weapon + 0x22a, 4))
        ammunition.append(dict(initial=initial, total_maximum=maximum,
                               loaded_maximum=capacity, inventory_maximum=inventory,
                               loaded=loaded, unloaded=unloaded))
    assert [(a['loaded'], a['unloaded']) for a in ammunition] == [(60,120), (20,0), (60,180)]
    # 19EDA0 option 5 resolves the actual mulg Runtime.Weapons[1] reference.
    # The gameplay tag lookup pointers are supplied; no resolver code is
    # skipped. This also covers the valid SMG override on plasma-start maps.
    w(0x4e034c, GLOBALS)
    w(GLOBALS + 0x16c, 0)
    w(0x4e3b44, tags)
    w(tags + 8, GLOBALS + 0x8000)
    w(GLOBALS + 0x800c, GLOBALS + 0x9000)
    w(GLOBALS + 0x906c, GLOBALS + 0xa000)
    weapon_choices = []
    for directory in (ROOT / 'assets/disc/maps', ROOT / 'assets/map-pack/maps'):
        for path in sorted(directory.glob('*.map')):
            cache = Cache(path)
            if cache.h(cache.scenario + 0x10) != 1:
                continue
            matg = next(tag['offset'] for tag in cache.tags.values()
                        if tag['group'] == 'matg')
            mulg = cache.tags[cache.u(matg + 0x16c)]['offset']
            runtime = cache.block(mulg + 8, 0x568)[0]
            weapons = cache.block(runtime + 0x68, 8)
            g.mem_write(GLOBALS + 0xa000,
                        b''.join(bytes(cache.data[p:p + 8]) for p in weapons))
            expected = cache.u(weapons[1] + 4)
            actual = call(0x19eda0, (0x11223344, 5, -1)) & 0xffffffff
            assert actual == expected
            reference = cache.reference(actual)
            assert reference['group'] == 'weap' and reference['name'].endswith('\\smg')
            weapon_choices.append(dict(map=path.name, option=5,
                                       runtime_weapon_index=1, weapon=reference))
    report = dict(xbe_sha256=PIN,
                  routines=['0019d220', '002374f0', '0015a090', '002bc000',
                            '0015ce70..0015d151', '0014f040', '000fdad0', '0019eda0'],
                  supplied_boundaries=['locale absent; variant name omitted',
                                       '15b7c0 score service records requested deltas',
                                       '1058b0 weapon animation reset skipped'],
                  slayer_variant_bytes=settings.hex(), parameters=parameters,
                  mode_matching=modes, scoring=scoring, respawn=respawn,
                  multiplayer_representation=representation,
                  weapon_initial_ammunition=ammunition,
                  starting_weapon_option=weapon_choices)
    output = json.dumps(report, indent=2) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output)
    else:
        print(output, end='')


if __name__ == '__main__':
    main()
