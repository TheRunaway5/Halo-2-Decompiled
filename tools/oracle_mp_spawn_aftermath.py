#!/usr/bin/env python3
"""Original TU1.1 damage aftermath ownership producer with explicit results.

Uses an actual Lockout damage effect and original CB410 -> CBDC0 -> 14E240.
Post-damage result flags and an existing ragdoll-state-three object are explicit
fixtures; this is not a full damage/death history/animation simulation. A second
set executes ordinary unsuppressed feedback only to its animation boundary.
The separately frozen placement/timing/ownership reports are never changed.
"""
import argparse
import hashlib
import json
from pathlib import Path

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_mp_spawn import ROOT, VERSIONS, CACHE_PIN, Cache, PLAYERS, OBJECTS, MULG
from oracle_mp_spawn_lifecycle import TABLE, DEFINITION, CAPSULES, EXTRA
from oracle_mp_spawn_ownership import OwnershipOracle, NONE

OUT = ROOT/'analysis/multiplayer/integration/spawn-source'
DAMAGE_TAG = EXTRA+0x130000
RESULT = EXTRA+0x131000


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class AftermathOracle(OwnershipOracle):
    def __init__(self):
        self.calls = []
        super().__init__('tu11')

    def hook(self, g, a, size, user):
        if a in (0xcb410, 0xcbdc0, 0xe6050, 0xbc840, 0x14e240, 0x115fa0):
            self.calls.append(dict(entry=f'{a:08x}',
                current=f'{self.u(PLAYERS+self.slot*0x21c+0x2c):08x}',
                previous=f'{self.u(PLAYERS+self.slot*0x21c+0x30):08x}',
                unit_current_player=f'{self.u(OBJECTS+0x13c):08x}',
                unit_previous_player=f'{self.u(OBJECTS+0x140):08x}'))
        super().hook(g, a, size, user)

    def prepare(self, cache, slot, dead, kill, suppress):
        self.fixture(cache, slot)
        bipd = next(t for t in cache.tags.values()
                    if t['group']=='bipd' and t['name'].endswith('masterchief_mp'))
        effect = next(t for t in cache.tags.values()
                      if t['group']=='jpt!' and t['name']=='globals\\falling')
        self.w(self.a['tags'], TABLE)
        self.w(TABLE+(int(bipd['datum'],16)&0xffff)*16+8, DEFINITION)
        self.g.mem_write(DEFINITION, bytes(cache.data[bipd['offset']:bipd['offset']+0x340]))
        self.w(TABLE+8, MULG)
        self.w(TABLE+(int(effect['datum'],16)&0xffff)*16+8, DAMAGE_TAG)
        self.g.mem_write(DAMAGE_TAG, bytes(cache.data[effect['offset']:effect['offset']+effect['size']]))
        for i in range(3):
            self.w(OBJECTS+i*0x800, int(bipd['datum'],16))
        datum=(7+slot*5)<<16|slot
        p=PLAYERS+slot*0x21c
        self.sh(p, datum>>16); self.w(p+0x2c, 0x10000)
        self.w(OBJECTS+0x13c, datum)
        self.g.mem_write(OBJECTS+0x10a, bytes([4 if dead else 0]))
        self.g.mem_write(RESULT, bytes(0x58))
        self.w(RESULT+4, int(kill)|(0x20 if suppress else 0))
        self.w(RESULT+8, int(effect['datum'],16))
        self.w(RESULT+0xc, NONE); self.w(RESULT+0x10, NONE)
        self.sh(RESULT+0x40, -1)
        self.calls=[]; self.services={}; self.events=[]; self.writes=[]
        return dict(player=f'{datum:08x}', unit='00010000', slot=slot,
            body_dead_bit=dead, result_death_bit=kill,
            damage_animation_suppression_bit_20=suppress,
            biped_state_3dc=3, local_user=-1, damage_effect=effect,
            full_result_hex=bytes(self.g.mem_read(RESULT,0x58)).hex(),
            result_position_kind='Explicit zero post-damage result; no head/marker pose supplied.',
            no_attacker=True, no_AI_actor=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=OUT/'aftermath-report.json')
    args=p.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(c.data).hexdigest()==CACHE_PIN
    o=AftermathOracle(); rows=[]
    for slot in (0,3,15):
        for dead in (False,True):
            for kill in (False,True):
                inputs=o.prepare(c,slot,dead,kill,True)
                before=o.state()
                stop=o.owner['death_stop'] if kill else 0x3000000
                o.call(0xcb410,(0x10000,RESULT),end=stop)
                after=o.state()
                assert after['current']==('ffffffff' if kill else '00010000')
                assert after['previous']==('00010000' if kill else before['previous'])
                assert after['history']==before['history']
                assert after['live_countdown']==before['live_countdown']
                assert after['units'][0]['dead']==dead
                assert not any(e.get('event')=='supplied_service' for e in o.events)
                if kill:
                    assert after['units'][0]['current_player']=='ffffffff'
                    assert after['units'][0]['previous_player']==inputs['player']
                    assert [e['entry'] for e in o.calls]==[
                        '000cb410','000cbdc0','000e6050','000bc840','0014e240']
                else:
                    assert after==before and [e['entry'] for e in o.calls]==['000cb410']
                rows.append(dict(inputs=inputs,before=before,after=after,
                    entry='000cb410',stop_exclusive=f'{stop:08x}',
                    complete_aftermath_return=not kill,
                    ordered_source_calls=o.calls,ordered_source_events=o.events,
                    scope='Complete nondeath aftermath, or death aftermath through original ownership release; later corpse cleanup excluded.'))
    prefixes=[]
    for slot in (0,3,15):
        inputs=o.prepare(c,slot,True,True,False)
        before=o.state()
        o.call(0xcb410,(0x10000,RESULT),end=0x115fa0)
        after=o.state()
        assert after==before and [e['entry'] for e in o.calls]==['000cb410']
        prefixes.append(dict(inputs=inputs,before=before,after=after,
            entry='000cb410',stop_exclusive='00115fa0',
            boundary='Original unsuppressed damage-animation entry; no animation leaf result or full normal-death completion is supplied.'))
    md=Cs(CS_ARCH_X86,CS_MODE_32); listing=[]
    for start,end in ((0xcb410,0xcbdc0),(0xcbdc0,0xcbea0),
                      (0x14e240,0x14e3e0),(0xdac40,0xdb160)):
        listing.append(f'RANGE {start:08x}..{end:08x}')
        listing.extend(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
            for i in md.disasm(o.x.read(start,end-start),start))
    path=OUT/'tu11-aftermath-instructions.txt'
    path.write_text('\n'.join(listing)+'\n')
    report=dict(tool_sha256=sha(__file__),executable_sha256=VERSIONS['tu11']['pin'],
        cache_sha256=CACHE_PIN,dependencies={str(p.relative_to(ROOT)):sha(p) for p in
            (ROOT/'tools/oracle_mp_spawn.py',ROOT/'tools/oracle_mp_spawn_lifecycle.py',ROOT/'tools/oracle_mp_spawn_ownership.py')},
        source_listing=str(path.relative_to(ROOT)),source_listing_sha256=sha(path),
        cases=rows,ordinary_animation_prefixes=prefixes,
        summary=dict(aftermath_cases=len(rows),ordinary_animation_prefixes=len(prefixes),
            original_ownership_releases=sum(r['inputs']['result_death_bit'] for r in rows),
            supplied_engine_services=0,full_normal_damage_paths=0),
        scope=__doc__)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['summary']))


if __name__=='__main__':
    main()
