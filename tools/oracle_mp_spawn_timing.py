#!/usr/bin/env python3
"""Bounded original-instruction multiplayer death/countdown/early-spawn timeline.

Original active-tick caller, early player stage, membership flag-zero branch,
eligibility/countdown, chooser, placement, assignment and final clock increment
run. Frame input supplies zero controls and an explicit active-tick flag; later
subsystems, notifications, world
shape/creation, representation/customization/loadout and final frame bookkeeping
are explicit services. Death is inserted at a later-subsystem boundary by a
paused caller context: original writer runs through penalty reset, then original
body-depleted/release prefixes. No head-marker pose or history is fabricated.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import capstone
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ESP, UC_X86_REG_EIP)
from oracle_mp_spawn import (ROOT, VERSIONS, Cache, CACHE_PIN, PLAYERS, BITMAP,
    OBJECTS, ENGINE_GLOBALS, OPTIONS, SEED, MULG, STOP)
from oracle_mp_spawn_lifecycle import EXTRA, TIME, TABLE, CAPSULES, DEFINITION
from oracle_mp_spawn_ownership import OwnershipOracle, NONE

CLOCK_STATE = EXTRA+0x12e000
ADDR={
    'base':dict(active_tick=0x137fe0,active_end=0x138102,early=0x14d3a0,
        eligible=0x15dd40,scan_return=0x14d4a0,recheck_return=0x14d435,
        writer=0x15ce70,writer_stop=0x15d163,clock_state_global=0x510c6c,
        active_flag=0x68250,
        frame_input=0x682e0,frame_finish=0x684e0,member=0x686e0,
        event=0x19eb90,pause=0x138db0),
    'tu11':dict(active_tick=0x139750,active_end=0x139872,early=0x14eae0,
        eligible=0x15f3a0,scan_return=0x14ebe0,recheck_return=0x14eb75,
        writer=0x15e4d0,writer_stop=0x15e7c3,clock_state_global=0x51845c,
        active_flag=0x68460,
        frame_input=0x684f0,frame_finish=0x686f0,member=0x688f0,
        event=0x1a24f0,pause=0x13a520),
}


def i32(x):return struct.unpack('<i',struct.pack('<I',x&0xffffffff))[0]


class TimingOracle(OwnershipOracle):
    def __init__(self,version):
        super().__init__(version)
        self.timing=ADDR[version]
        md=Cs(CS_ARCH_X86,CS_MODE_32)
        ins=list(md.disasm(self.x.read(self.timing['active_tick'],
            self.timing['active_end']-self.timing['active_tick']),self.timing['active_tick']))
        calls=[(i.address,int(i.op_str,16)) for i in ins if i.mnemonic=='call']
        first=next(n for n,(_,target) in enumerate(calls) if target==self.timing['early'])
        finish=next(n for n,(_,target) in enumerate(calls) if target==self.timing['frame_finish'])
        self.world_phases=calls[first+1:finish]
        # Membership's first flag check has the same original local-runtime branch.
        member=next(i for i in md.disasm(self.x.read(self.timing['member'],0x40),self.timing['member'])
            if i.mnemonic=='mov' and 'byte ptr [' in i.op_str)
        self.member_flag=int(member.op_str.split('[')[1].split(']')[0],16)
        self.eligibility=[];self.notices=[];self.pending_eligibility=None
        self.pause_requested=False
        self.tick_active=True

    def hook(self,uc,address,size,unused):
        if hasattr(self,'timing'):
            if address==self.timing['pause'] and self.pause_requested:
                uc.emu_stop()
                return
            sp=uc.reg_read(UC_X86_REG_ESP)
            if address==self.timing['frame_input']:
                # Source routine clears this whole input frame before gathering it.
                uc.mem_write(uc.reg_read(UC_X86_REG_EAX),bytes(0x4048))
                uc.mem_write(uc.reg_read(UC_X86_REG_EAX)+4,bytes([int(self.tick_active)]))
            if address==self.timing['eligible']:
                datum=uc.reg_read(UC_X86_REG_EAX)
                p=PLAYERS+(datum&0xffff)*0x21c
                self.pending_eligibility=dict(player=f'{datum:08x}',game_tick=self.u(TIME+8),
                    before=i32(self.u(p+0x164)),before_current=f'{self.u(p+0x2c):08x}')
            if address in (self.timing['scan_return'],self.timing['recheck_return']):
                row=self.pending_eligibility
                assert row is not None
                p=PLAYERS+(int(row['player'],16)&0xffff)*0x21c
                row.update(stage='scan' if address==self.timing['scan_return'] else 'recheck',
                    after=i32(self.u(p+0x164)),displayed_seconds=i32(self.u(p+0x170)),
                    result=uc.reg_read(UC_X86_REG_EAX)&255)
                self.eligibility.append(row);self.pending_eligibility=None
            if address==self.timing['event']:
                event=uc.reg_read(UC_X86_REG_EAX)
                self.notices.append(dict(game_tick=self.u(TIME+8),subtype=self.u(event+4),
                    player=f'{self.u(event+8):08x}',cause=f'{self.u(event+0xc):08x}'))
        super().hook(uc,address,size,unused)

    def fixture(self,cache,slot=0,initial=False):
        super().fixture(cache,slot)
        self.g.mem_write(PLAYERS+slot*0x21c+2,bytes([9 if initial else 1]))
        self.sh(PLAYERS+slot*0x21c,7)
        self.sh(PLAYERS+slot*0x21c+0x1ac,-1)
        self.w(PLAYERS+slot*0x21c+0x164,0)
        self.w(ENGINE_GLOBALS+0x70,0)
        self.w(self.timing['clock_state_global'],CLOCK_STATE)
        self.f(TIME+4,1/30)
        self.g.mem_write(self.member_flag,b'\0')
        # Marker/history capture is outside this timing oracle, including XYZ.
        self.g.mem_write(ENGINE_GLOBALS+0x558,bytes(16*0x18))
        self.watch.update({PLAYERS+slot*0x21c+off:name for off,name in
            ((0x164,'player.countdown'),(0x168,'player.penalty'),(0x170,'player.display_seconds'),
             (0x1b4,'player.death_elapsed'))})
        self.watch[TIME+8]='game.clock'
        self.tag=next(t for t in cache.tags.values() if t['group']=='bipd' and t['name'].endswith('masterchief_mp'))
        self.w(self.a['tags'],TABLE)
        self.w(TABLE+(int(self.tag['datum'],16)&0xffff)*16+8,DEFINITION)
        self.g.mem_write(DEFINITION,bytes(cache.data[self.tag['offset']:self.tag['offset']+0x340]))
        self.w(TABLE+8,MULG)
        qs=cache.block(self.tag['offset']+0x28c,0x50)
        self.g.mem_write(CAPSULES,b''.join(bytes(cache.data[q:q+0x50]) for q in qs))
        self.w(DEFINITION+0x290,CAPSULES)
        for i in range(3):
            self.w(OBJECTS+i*0x800,int(self.tag['datum'],16))
            self.sh(OBJECTS+i*0x800+0x2c,-1)
        self.spawn_tag=int(self.tag['datum'],16)
        self.shape_hit=False
        self.services={self.owner['representation']:('representation',1,None),
            self.owner['create']:('create',1,0x10001),self.owner['custom']:('customization',0,0),
            self.owner['loadout']:('loadout',0,None),self.timing['frame_input']:('zero_frame_input',0,None),
            self.timing['frame_finish']:('frame_bookkeeping',0,None),self.timing['event']:('countdown_event',0,None)}
        self.services.update({target:(f'world_phase_{n}',0,None) for n,(_,target) in enumerate(self.world_phases)})
        self.eligibility=[];self.notices=[];self.pending_eligibility=None

    def timer_state(self):
        p=PLAYERS+self.slot*0x21c
        return dict(game_tick=self.u(TIME+8),engine_elapsed=i32(self.u(TIME+8)-self.u(ENGINE_GLOBALS+0x70)),countdown=i32(self.u(p+0x164)),
            penalty=i32(self.u(p+0x168)),displayed_seconds=i32(self.u(p+0x170)),
            current=f'{self.u(p+0x2c):08x}',previous=f'{self.u(p+0x30):08x}',
            full_player=f'{(self.h(p)&0xffff)<<16|self.slot:08x}',
            flags=f'{self.h(p+2)&0xffff:04x}',spawn_count=self.h(p+0x218),
            death_elapsed=i32(self.u(p+0x1b4)))

    def death(self,seconds,suicide_seconds=0,penalty=0,release=True):
        p=PLAYERS+self.slot*0x21c;datum=0x70000|self.slot
        self.w(OPTIONS+0x1bc,seconds);self.w(OPTIONS+0x1c0,suicide_seconds or 0)
        self.w(p+0x168,penalty)
        self.call(self.timing['writer'],(datum if suicide_seconds is not None else NONE,datum,0,0),
            end=self.timing['writer_stop'])
        # The writer stops before history/marker capture, preserving the owned unit.
        assert self.u(p+0x2c)==0x10000 and not self.marker_calls
        expected=max(30,seconds*30+penalty+(suicide_seconds or 0)*30)
        assert self.u(p+0x164)==expected and self.u(p+0x168)==0
        self.call(self.owner['body_depleted'],(0x10000,0,0,0),end=self.owner['body_depleted_stop'])
        if release:self.release()
        return expected

    def release(self):
        self.call(self.owner['death'],(0x10000,NONE,0,0,0),end=self.owner['death_stop'])
        assert self.u(PLAYERS+self.slot*0x21c+0x2c)==NONE

    def tick(self,active=True,death_input=None,release_now=False):
        self.events=[];self.writes=[];self.eligibility=[];self.notices=[]
        self.tick_active=active
        before=self.timer_state()
        if death_input is not None or release_now:
            assert active
            self.pause_requested=True
            self.call(self.timing['active_tick'],end=self.timing['pause'])
            self.pause_requested=False
            # Preserve the original caller context while supplying this later-world event.
            context=self.g.context_save()
            stack=bytes(self.g.mem_read(0x2000000,0x10000))
            before_world=self.timer_state()
            if death_input is not None:self.death(**death_input)
            if release_now:self.release()
            after_world=self.timer_state()
            self.g.mem_write(0x2000000,stack);self.g.context_restore(context)
            self.g.emu_start(self.timing['pause'],STOP,count=1000000)
            assert self.g.reg_read(UC_X86_REG_EIP)==STOP
        else:
            self.call(self.timing['active_tick'])
            before_world=after_world=None
        after=self.timer_state()
        assert after['game_tick']==(before['game_tick']+int(active))&0xffffffff
        assert not self.marker_calls
        interesting=[e for e in self.events if e['event'] in ('bind','active') or
            (e['event']=='write' and e['field'] in ('player.countdown','player.penalty','player.current',
                'player.previous','game.clock','player.flags','player.spawn_count'))]
        return dict(active=active,before=before,before_world_event=before_world,
            after_world_event=after_world,after=after,eligibility=self.eligibility,
            notices=self.notices,ordered_writes_and_bindings=interesting)

    def timelines(self,cache):
        cases=[]
        modes=[(f'normal_{seconds}',seconds,None,0) for seconds in (0,1,5)]
        modes += [(f'suicide_{seconds}_plus_{suicide}',seconds,suicide,0)
            for seconds in (0,1,5) for suicide in (0,1,5)]
        modes += [('stored_penalty',5,None,7),('suicide_stored_penalty',5,1,7)]
        for slot in (0,3,15):
            for name,seconds,suicide,penalty in modes:
                self.fixture(cache,slot)
                p=PLAYERS+slot*0x21c;datum=0x70000|slot
                self.w(TIME+8,100);self.w(p+0x2c,0x10000);self.w(OBJECTS+0x13c,datum)
                death_input=dict(seconds=seconds,suicide_seconds=suicide,penalty=penalty)
                ticks=[self.tick(death_input=death_input)]
                delay=max(30,seconds*30+penalty+(suicide or 0)*30)
                zero_tick=100+delay
                spawn_tick=zero_tick+((slot-zero_tick)&31)
                while self.u(TIME+8)<=spawn_tick:
                    ticks.append(self.tick())
                assert ticks[0]['eligibility']==[]
                assert ticks[0]['after']['countdown']==delay
                assert ticks[1]['eligibility'][0]['before']==delay and ticks[1]['eligibility'][0]['after']==delay-1
                spawns=[t for t in ticks if t['after']['current']=='00010001']
                assert len(spawns)==1 and spawns[0]['before']['game_tick']==spawn_tick
                assert spawns[0]['after']['game_tick']==spawn_tick+1
                assert all(t['after']['full_player']==f'{datum:08x}' for t in ticks)
                assert spawns[0]['after']['previous']=='ffffffff' and int(spawns[0]['after']['flags'],16)&8==0
                assert spawns[0]['after']['spawn_count']==1
                assert len(spawns[0]['eligibility'])==2
                assert spawns[0]['eligibility'][1]['before']==spawns[0]['eligibility'][1]['after']==0
                cases.append(dict(case=name,slot=slot,death_game_tick=100,interval_seconds=seconds,
                    suicide_penalty_seconds=suicide,input_stored_penalty_ticks=penalty,
                    initial_countdown=delay,first_decrement_game_tick=101,zero_game_tick=zero_tick,
                    successful_spawn_game_tick=spawn_tick,clock_after_spawn_tick=spawn_tick+1,
                    ticks=ticks,weapon_rng_after=f'{self.u(SEED):08x}',spawn_rng_after=f'{self.u(SEED+4):08x}'))
        return cases

    def penalty_sequences(self,cache):
        rows=[]
        for same_identity,victim_countdown in ((s,n) for s in (False,True) for n in (0,1)):
            self.fixture(cache,3)
            victim=0x70003;attacker=0x70000
            vp=PLAYERS+3*0x21c;ap=PLAYERS
            self.sh(ap,7);self.w(BITMAP,9)
            self.w(vp+0x2c,0x10000);self.w(OBJECTS+0x13c,victim)
            self.w(ap+0x2c,0x10001);self.w(OBJECTS+0x800+0x13c,attacker)
            self.g.mem_write(ap+0xc0,bytes([3 if same_identity else 0]))
            self.w(vp+0x164,victim_countdown);self.w(ap+0x168,7)
            self.watch.update({ap+0x168:'attacker.penalty',ap+0x164:'attacker.countdown'})
            self.w(OPTIONS+0x1e8,2);self.w(OPTIONS+0x1bc,5)
            self.w(TIME+8,100)
            self.events=[];self.writes=[]
            self.call(self.timing['writer'],(attacker,victim,0,0),end=self.timing['writer_stop'])
            gained=60 if same_identity and victim_countdown==0 else 0
            assert self.u(ap+0x168)==7+gained
            after_kill=dict(attacker_penalty=self.u(ap+0x168),victim_countdown=self.u(vp+0x164),
                victim_penalty=self.u(vp+0x168),attacker_current=f'{self.u(ap+0x2c):08x}',
                victim_current=f'{self.u(vp+0x2c):08x}')
            self.w(OPTIONS+0x1c0,1)
            self.call(self.timing['writer'],(attacker,attacker,0,0),end=self.timing['writer_stop'])
            # A stored penalty combines with suicide penalty and the variant interval.
            assert self.u(ap+0x164)==150+30+7+gained and self.u(ap+0x168)==0
            assert not self.marker_calls
            rows.append(dict(case='identity_penalty_then_suicide',attacker=f'{attacker:08x}',victim=f'{victim:08x}',
                attacker_c0=3 if same_identity else 0,victim_c0=3,
                valid_ffa_membership_fixture=not same_identity,
                victim_countdown_before_kill=victim_countdown,input_attacker_penalty=7,
                option_1e8_penalty_seconds=2,suicide_penalty_seconds=1,interval_seconds=5,
                after_kill=after_kill,after_attacker_death=dict(countdown=self.u(ap+0x164),
                    penalty=self.u(ap+0x168),current=f'{self.u(ap+0x2c):08x}'),
                ordered_writes=[e for e in self.events if e['event']=='write'],ticks=[]))
        return rows

    def boundary_sequences(self,cache):
        rows=[]
        for slot in (0,3,15):
            # Keeping the dead unit assigned prevents early-phase countdown calls.
            self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
            self.w(TIME+8,100);self.w(p+0x2c,0x10000);self.w(OBJECTS+0x13c,datum)
            ticks=[self.tick(death_input=dict(seconds=1,suicide_seconds=None,release=False))]
            ticks.append(self.tick());ticks.append(self.tick(release_now=True));ticks.append(self.tick())
            assert [t['after']['countdown'] for t in ticks]==[30,30,30,29]
            assert [len(t['eligibility']) for t in ticks]==[0,0,0,1]
            rows.append(dict(case='release_delayed_two_ticks',slot=slot,ticks=ticks))
            # A false input-frame active flag skips the player stage and clock increment.
            self.fixture(cache,slot);self.w(TIME+8,100);self.w(p+0x164,7)
            ticks=[self.tick(False),self.tick(False),self.tick()]
            assert [t['after']['countdown'] for t in ticks]==[7,7,6]
            rows.append(dict(case='inactive_tick_flag',slot=slot,ticks=ticks))
            # Active ticks in a non-playing engine phase keep the countdown frozen.
            self.fixture(cache,slot);self.w(TIME+8,100);self.w(p+0x164,7)
            self.sh(ENGINE_GLOBALS+0x6c,2)
            ticks=[self.tick(),self.tick()]
            self.sh(ENGINE_GLOBALS+0x6c,1);ticks.append(self.tick())
            assert [t['after']['countdown'] for t in ticks]==[7,7,6]
            rows.append(dict(case='engine_phase_gate',slot=slot,ticks=ticks))
            self.fixture(cache,slot);self.w(TIME+8,100);self.w(p+0x164,-1)
            ticks=[self.tick(),self.tick()]
            assert [t['after']['countdown'] for t in ticks]==[-1,-1]
            assert all(t['after']['current']=='ffffffff' for t in ticks)
            rows.append(dict(case='negative_countdown',slot=slot,ticks=ticks))
            # New-player initial flag does not impose the death writer's one-second minimum.
            for start in (-1,0,3,4):
                self.fixture(cache,slot,initial=True);self.w(TIME+8,start)
                tick=self.tick()
                expected=start<4 or (start&31)==slot
                assert (tick['after']['current']=='00010001')==expected
                assert bool(int(tick['after']['flags'],16)&8)==(not expected)
                rows.append(dict(case='initial_zero_countdown',slot=slot,elapsed_tick=start,
                    expected_spawn=expected,ticks=[tick]))
        return rows

    def active_flag_cases(self,cache):
        rows=[]
        for case in ('ready','runtime_flag_zero','options_null','scenario_flag_zero',
                'option_index_none','secondary_flag_set','runtime_phase_three'):
            self.fixture(cache)
            runtime=CLOCK_STATE+0x100
            self.g.mem_write(self.member_flag,bytes([case!='runtime_flag_zero']))
            self.g.mem_write(self.member_flag+2,bytes([case=='secondary_flag_set']))
            self.w(self.member_flag+0xc,runtime)
            self.w(runtime+0x18,3 if case=='runtime_phase_three' else 4)
            self.sh(OPTIONS+2,-1 if case=='option_index_none' else 0)
            self.g.mem_write(OPTIONS+0x1120,bytes([case!='scenario_flag_zero']))
            if case=='options_null':self.w(self.a['options'],0)
            result=self.call(self.timing['active_flag'])&255
            assert result==int(case=='ready')
            rows.append(dict(case=case,runtime_flag=int(case!='runtime_flag_zero'),
                options_is_null=case=='options_null',scenario_flag=int(case!='scenario_flag_zero'),
                option_index=-1 if case=='option_index_none' else 0,
                secondary_flag=int(case=='secondary_flag_set'),
                runtime_phase=3 if case=='runtime_phase_three' else 4,result=result,
                original_entry=f'{self.timing["active_flag"]:08x}',ticks=[]))
        return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/timing-report.json')
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    oracles={v:TimingOracle(v) for v in VERSIONS}
    rows={v:dict(timelines=o.timelines(cache),penalties=o.penalty_sequences(cache),
        boundaries=o.boundary_sequences(cache),active_flags=o.active_flag_cases(cache)) for v,o in oracles.items()}
    def clean(x):
        if isinstance(x,dict):return {k:clean(v) for k,v in x.items() if k not in ('instruction','original_entry')}
        if isinstance(x,list):return [clean(v) for v in x]
        return x
    for kind in rows['base']:assert clean(rows['base'][kind])==clean(rows['tu11'][kind]),kind
    report=dict(passed=True,oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        imported_tool_sha256={n:hashlib.sha256((ROOT/f'tools/{n}.py').read_bytes()).hexdigest() for n in
            ('oracle_mp_spawn','oracle_mp_spawn_lifecycle','oracle_mp_spawn_ownership')},
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        authorities={v:dict(path=a['path'],sha256=a['pin']) for v,a in VERSIONS.items()},
        scope=__doc__,cache=dict(path='assets/disc/maps/lockout.map',sha256=CACHE_PIN),
        tick_rate=30,engine_start_tick=0,death_insertion_boundary='First subsystem after the complete early player stage.',
        service_boundaries=['Input-frame producer supplies zero controls and the active-tick flag at frame +4; original frame consumer executes.',
            'Every later subsystem and final frame bookkeeping is supplied as no-op.',
            'Countdown event delivery is supplied; original notification construction executes.',
            'Representation, customization, object creation, world contact and loadout services from ownership oracle.',
            'Death writer stops after penalty reset, before any history capture; no head pose is supplied.',
            'Body-depleted setter and death cleanup stop at the previously proved ownership boundaries.'],
        world_phase_calls={v:[dict(callsite=f'{at:08x}',target=f'{target:08x}',name=f'world_phase_{n}')
            for n,(at,target) in enumerate(o.world_phases)] for v,o in oracles.items()},
        engine_identity_predicate={v:dict(engine=f'{o.u(o.a["engine_table"]+8):08x}',
            entry=f'{o.u(o.u(o.u(o.a["engine_table"]+8))+0x6c):08x}',
            result='Signed 16-bit identity arguments differ; original routine executes.')
            for v,o in oracles.items()},
        case_counts={k:sum(len(r[k]) for r in rows.values()) for k in rows['base']},
        tick_count=sum(len(c['ticks']) for r in rows.values() for k in r for c in r[k]),rows=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    ranges={'base':[(0x137fe0,0x138102),(0x14d2a0,0x14d8b0),(0x15dd40,0x15dea0),
            (0x15ce70,0x15d163),(0x68250,0x68290),(0x682e0,0x683a0),(0x24dd70,0x24dd82)],
        'tu11':[(0x139750,0x139872),(0x14e9e0,0x14eff0),(0x15f3a0,0x15f500),
            (0x15e4d0,0x15e7c3),(0x68460,0x684a0),(0x684f0,0x685b0),(0x253f10,0x253f22)]}
    for v,blocks in ranges.items():
        o=oracles[v];lines=[f'{v} {o.a["pin"]}']
        for start,end in blocks:
            data=o.x.read(start,end-start)
            lines.append(f'\nRANGE {start:08x}..{end:08x} SHA256 {hashlib.sha256(data).hexdigest()}')
            lines.extend(f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in md.disasm(data,start))
        (args.output.parent/f'{v}-timing-instructions.txt').write_text('\n'.join(lines)+'\n')
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(passed=True,case_counts=report['case_counts'],tick_count=report['tick_count'])))


if __name__=='__main__':main()
