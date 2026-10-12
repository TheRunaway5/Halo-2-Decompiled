#!/usr/bin/env python3
"""Unchanged team-session masks, zone collection and respawn-prefix evidence.

Retail/TU1.1 session mask and designator mapping run with no live players during
the subsequent property refresh, and NONE replicated engine entities. Spawn
cases then restore explicit real player/object fixtures and run the unchanged
Slayer collector/chooser. The existing oracle's shape boundary remains NONE.
TU1.1 death cases stop at15E7C3, after original countdown and penalty writes but
before marker/announcement/medal/scoring services. No original team, spawn or
timer result is supplied; this does not prove session balancing or a live Xbox.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_mp_spawn import (ROOT, VERSIONS, CACHE_PIN, Cache, Oracle, PLAYERS,
    BITMAP, ENGINE_GLOBALS, OPTIONS)
from oracle_mp_spawn_lifecycle import TIME
from oracle_team_slayer import TeamOracle, SLOTS

ADDR = {'base':(0x157790, 0x157670, 0x158140, 0x157970),
        'tu11':(0x158ee0, 0x158dc0, 0x1597e0, 0x159010)}


def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def mapping(o, memberships, maximum, present):
    bitmap = o.u(BITMAP);o.w(BITMAP, 0)
    o.w(ENGINE_GLOBALS + 0x24, 0xffffffff)
    o.sh(ENGINE_GLOBALS + 4, present)
    o.w(OPTIONS + 0x1f0, maximum)
    for slot in range(16):
        at = OPTIONS + 0x2dc + slot * 0xe4
        o.g.mem_write(at, bytes([slot < len(memberships)]))
        o.g.mem_write(at + 0x98, bytes([memberships[slot] & 255 if slot < len(memberships) else 255]))
    o.call(ADDR[o.version][0]);session = o.h(ENGINE_GLOBALS + 6) & 65535
    o.call(ADDR[o.version][1])
    result = dict(session=session, assigned=o.h(ENGINE_GLOBALS + 8) & 65535,
        teams=o.h(ENGINE_GLOBALS + 10) & 65535,
        designators=[o.h(ENGINE_GLOBALS + 0x10 + n * 2) for n in range(9)])
    o.w(BITMAP, bitmap)
    return result


def run(version, cache):
    o = Oracle(version);maps=[];spawns=[];presence=[];policy=[]
    for teams in (False,True):
        o.setup(cache);o.sh(ENGINE_GLOBALS+4,0x100);o.w(OPTIONS+0x184,int(teams))
        o.call(ADDR[version][3]);present=o.h(ENGINE_GLOBALS+4)&65535
        assert present==(0xff if teams else 0x100)
        policy.append(dict(team_mode=teams,previous=0x100,present=present))
    for present in (0, 1, 3, 0x55, 0xff, 0x100, 0x101):
        for maximum in (0, 1, 2, 8):
            for members in ([], [-1], [0], [7], [0, 0, 1], [3, 7]):
                o.setup(cache)
                result = mapping(o, members, maximum, present)
                maps.append(dict(present=present, maximum=maximum, memberships=members, result=result))
    for team_mode in (False, True):
        for initial in (False, True):
            for member in (-1, 0, 1, 3, 7):
                for maximum in (0, 2):
                    world = [(member, [-1., 3., -8.], False),
                             (1, [5., -1., -8.], False),
                             (member, [2., 2., -8.], True)]
                    o.setup(cache, initial=initial, players=[(m & 255,p,d) for m,p,d in world])
                    o.w(OPTIONS + 0x184, int(team_mode))
                    o.g.mem_write(PLAYERS + 0xc0, bytes([member & 255]))
                    roster = [member, member, 1]
                    result = mapping(o, roster, maximum, 0xff)
                    sample = o.choose()
                    spawns.append(dict(team_mode=team_mode, initial=initial, member=member,
                        maximum=maximum, present=0xff, roster=roster, mapping=result,
                        players=world, result=sample))
    for old in (0,1,3,0x80):
        for memberships in ([], [0], [-1], [0,0,1], [7,-1,3]):
            for suppressed in (False,True):
                o.setup(cache);o.w(ENGINE_GLOBALS+0x24,0xffffffff)
                o.sh(ENGINE_GLOBALS+0xe,old);o.sh(ENGINE_GLOBALS+0xc,old)
                o.w(BITMAP,sum(1<<s for s in SLOTS[:len(memberships)]))
                flags=[3 if suppressed and i%2==0 else 1 for i in range(len(memberships))]
                for slot,member,flag in zip(SLOTS,memberships,flags):
                    o.g.mem_write(PLAYERS+slot*0x21c+0xc0,bytes([member&255]))
                    o.sh(PLAYERS+slot*0x21c+2,flag)
                # Profile/stat recording remains disabled in the original BSS.
                # The complete presence routine, including its original leaf,
                # executes; no reset/notification/team result is supplied.
                o.call(ADDR[version][2])
                presence.append(dict(previous=old,memberships=memberships,flags=flags,
                    playing=o.h(ENGINE_GLOBALS+0xc)&65535,retained=o.h(ENGINE_GLOBALS+0xe)&65535))
    return dict(mappings=maps, spawns=spawns, presence=presence,present_policy=policy)


def timers(cache):
    o = TeamOracle();rows=[]
    for modifier in (0, 1, 2):
        for attacker in (None, 0, 3, 15):
            for countdowns in ((0, 0, 0), (60, 0, 90), (20, 0, 300), (500, 50, 70)):
                o.fixture(cache)
                o.w(o.options + 0x1bc, 5);o.w(o.options + 0x1c0, 5)
                o.w(o.options + 0x1e8, 10);o.w(o.options + 0x1e4, modifier)
                for slot, count in zip(SLOTS, countdowns):
                    o.w(PLAYERS + slot * 0x21c + 0x164, count)
                    o.w(PLAYERS + slot * 0x21c + 0x168, 0)
                o.w(TIME + 8, 200)
                owner = 0xffffffff if attacker is None else o.datums[attacker]
                o.call(0x15e4d0, (owner, o.datums[3], int(attacker != 15), 8), end=0x15e7c3)
                rows.append(dict(modifier=modifier, attacker=-1 if attacker is None else attacker,
                    memberships=[0,0,1], before=list(countdowns),
                    after=[o.u(PLAYERS + slot * 0x21c + 0x164) for slot in SLOTS],
                    penalties=[o.u(PLAYERS + slot * 0x21c + 0x168) for slot in SLOTS],
                    death_elapsed=o.u(PLAYERS + 3 * 0x21c + 0x1b4)))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/game-variants/team-session-original.json')
    p.add_argument('--fixture',type=Path)
    args=p.parse_args();c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    source={version:run(version,c) for version in ADDR}
    assert source['base']==source['tu11'], 'Retail/TU team session or spawn results differ'
    countdowns=timers(c)
    listings={}
    for version in ADDR:
        o=Oracle(version);md=Cs(CS_ARCH_X86,CS_MODE_32)
        ranges=[(ADDR[version][0],0x140),
                (ADDR[version][1],0x120),
                (ADDR[version][2],0x190),
                (ADDR[version][3],0x80),
                (0x23b170 if version=='base' else 0x23d270,0x340)]
        lines=[]
        for at,size in ranges:
            lines += [f'RANGE {at:08x}..{at+size:08x}']
            lines += [f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}' for i in md.disasm(o.x.read(at,size),at)]
        if version=='tu11':
            lines += [f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
                for at,size in ((0x15e4d0,0x2f3),(0x15ed20,0xb0))
                for i in md.disasm(o.x.read(at,size),at)]
        args.output.parent.mkdir(parents=True,exist_ok=True)
        path=args.output.with_suffix('.'+version+'.instructions.txt');path.write_text('\n'.join(lines)+'\n')
        listings[str(path.relative_to(ROOT))]=sha(path)
    report=dict(scope=__doc__, tool_sha256=sha(__file__), source_versions=VERSIONS,
        cache_sha256=CACHE_PIN, listings=listings, source=source['tu11'], timers=countdowns,
        summary=dict(mappings=len(source['tu11']['mappings']), spawns=len(source['tu11']['spawns']),
            presence=len(source['tu11']['presence']),timers=len(countdowns)),
        dependencies={f'tools/{name}.py':sha(ROOT/'tools'/f'{name}.py') for name in
            ('oracle_mp_spawn','oracle_mp_spawn_lifecycle','oracle_mp_spawn_ownership',
             'oracle_mp_spawn_slayer_accounting','oracle_team_slayer')})
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    if args.fixture:
        lines=['# Original retail/TU team session and spawn instructions; TU respawn prefix']
        for r in report['source']['mappings']:
            v=r['result'];lines.append(' '.join(map(str,['M',r['present'],r['maximum'],len(r['memberships']),
                *r['memberships'],v['session'],v['assigned'],v['teams'],*v['designators']])))
        for r in report['source']['spawns']:
            v=r['result'];values=['S',int(r['team_mode']),int(r['initial']),r['member'],r['maximum'],
                r['present'],len(r['roster']),*r['roster'],len(r['players'])]
            for member,position,dead in r['players']:values += [member,int(dead),*position]
            values += [len(v['influence_bits'])]
            for influence in v['influence_bits']:values += [int(bits,16) for bits in influence]
            values += [v['selected']];lines.append(' '.join(map(str,values)))
        for r in countdowns:
            lines.append(' '.join(map(str,['T',r['modifier'],r['attacker'],*r['before'],*r['after'],
                *r['penalties'],r['death_elapsed']])))
        for r in report['source']['presence']:
            lines.append(' '.join(map(str,['P',r['previous'],len(r['memberships']),
                *[v for pair in zip(r['memberships'],r['flags']) for v in pair],r['playing'],r['retained']])))
        args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
    print(json.dumps(report['summary']))


if __name__=='__main__':main()
