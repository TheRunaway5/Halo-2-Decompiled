#!/usr/bin/env python3
"""Bounded unchanged TU1.1 FFA attribution, Slayer score and ending instructions.

Explicit salted pools/results/damage-history fixtures exercise original source
writers and the actual Slayer vtable. The death-history/marker/loadout writer is
one named supplied boundary in full attribution cases; score is separately
executed through the original Slayer callback. Persistent profile recording is
disabled, replicated engine entity is NONE, and session/event notifications are
named leaves. No original instructions or head poses are patched or fabricated.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_XMM0)
from oracle_mp_spawn import (ROOT, VERSIONS, CACHE_PIN, Cache, ENGINE_GLOBALS,
    PLAYERS, PLAYER_POOL, BITMAP, OBJECT_POOL, OBJECTS, SEED)
from oracle_mp_spawn_ownership import OwnershipOracle, NONE
from oracle_mp_spawn_lifecycle import TIME

OUT=ROOT/'analysis/multiplayer/integration/spawn-source'
STAT=ENGINE_GLOBALS+0x304
RANK=0x4132000


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def i32(x):
    return x-(1<<32) if x & (1<<31) else x


class SlayerOracle(OwnershipOracle):
    def __init__(self):
        self.accounting_events=[]
        self.notification_services={}
        self.skip_writer=False
        super().__init__('tu11')

    def fixture(self,c,slots=(0,3,15)):
        super().fixture(c)
        self.w(BITMAP,sum(1<<s for s in slots))
        self.w(OBJECT_POOL+0x38,3); self.w(OBJECT_POOL+0x24,12)
        self.w(ENGINE_GLOBALS+0x24,NONE); self.w(ENGINE_GLOBALS+0x28,NONE)
        self.w(ENGINE_GLOBALS+0xc04,1); self.sh(ENGINE_GLOBALS+0x6c,1)
        self.sh(ENGINE_GLOBALS+0x6e,0); self.w(ENGINE_GLOBALS+0x70,0)
        self.g.mem_write(STAT,bytes(0x254)); self.g.mem_write(STAT,b'\x01')
        self.w(0x518490,0)
        self.f(TIME+4,1/30); self.w(TIME+8,200)
        options=self.u(self.a['options'])
        for off,v in ((0x180,2),(0x184,0),(0x188,0),(0x18c,0),
                      (0x190,0),(0x1b8,0),(0x22c,2)):
            self.w(options+off,v)
        self.options=options
        self.datums={s:(7+5*s)<<16|s for s in slots}
        for s in slots:
            p=PLAYERS+s*0x21c
            self.sh(p,self.datums[s]>>16); self.sh(p+2,1)
            self.w(p+0x2c,NONE); self.sh(p+0x28,-1)
            self.sh(p+0x1ac,-1)
            self.g.mem_write(p+0xc0,bytes([s]))
        for n in range(4):
            self.w(OBJECTS+0x2f0+n*16,NONE)
            self.f(OBJECTS+0x2f4+n*16,0)
            self.w(OBJECTS+0x2f8+n*16,NONE)
            self.w(OBJECTS+0x2fc+n*16,NONE)
        self.accounting_events=[]; self.skip_writer=False
        self.notification_services={
            0x13a130:('session match completion notification; source c04 transition already executed',0),
            0x1a24f0:('game event delivery outside match datums',0),
        }

    def stats(self):
        return {str(s):dict(full_player=f'{d:08x}',
            fields=[self.h(STAT+4+s*0x1c+n*2) for n in range(12)],
            last_kill_tick=f'{self.u(STAT+0x1c+s*0x1c):08x}',
            player_counter_1c4=self.h(PLAYERS+s*0x21c+0x1c4))
            for s,d in self.datums.items()}

    def engine_state(self):
        return dict(flags=f'{self.u(ENGINE_GLOBALS):08x}',
            lifecycle=self.h(ENGINE_GLOBALS+0x6c),round_index=self.h(ENGINE_GLOBALS+0x6e),
            status_c04=self.u(ENGINE_GLOBALS+0xc04),
            clock_seconds=self.h(ENGINE_GLOBALS+0xe0),stats=self.stats(),
            rng=[f'{self.u(SEED+n):08x}' for n in (0,4)])

    def hook(self,g,a,size,user):
        sp=g.reg_read(UC_X86_REG_ESP)
        if a==0x15ce00:
            self.accounting_events.append(dict(entry='0015ce00',kind='original_score_request',
                player=f'{self.u(sp+4):08x}',delta=i32(g.reg_read(UC_X86_REG_EAX))))
        elif a==0x15c9e0:
            self.accounting_events.append(dict(entry='0015c9e0',kind='original_end_request',
                winner=f'{g.reg_read(UC_X86_REG_EBX):08x}',force=self.u(sp+4)&255))
        elif a==0x15e4d0 and self.skip_writer:
            self.accounting_events.append(dict(entry='0015e4d0',kind='supplied_history_and_death_event_writer_boundary',
                attacker=f'{self.u(sp+4):08x}',victim=f'{self.u(sp+8):08x}',
                relationship_flag=self.u(sp+12)&255,cause=self.u(sp+16)&255))
            g.reg_write(UC_X86_REG_EIP,self.u(sp)); g.reg_write(UC_X86_REG_ESP,sp+20)
            return
        elif a in self.notification_services:
            name,pop=self.notification_services[a]
            self.accounting_events.append(dict(entry=f'{a:08x}',kind='supplied_notification',
                name=name,engine_status_before=self.u(ENGINE_GLOBALS+0xc04)))
            g.reg_write(UC_X86_REG_EIP,self.u(sp)); g.reg_write(UC_X86_REG_ESP,sp+4+pop)
            return
        super().hook(g,a,size,user)

    def history_state(self):
        return [dict(timestamp=self.u(OBJECTS+0x2f0+n*16),
            damage_bits=f'{self.u(OBJECTS+0x2f4+n*16):08x}',
            object=f'{self.u(OBJECTS+0x2f8+n*16):08x}',
            player=f'{self.u(OBJECTS+0x2fc+n*16):08x}',
            cause=self.g.mem_read(OBJECTS+0x330+n,1)[0]) for n in range(4)]

    def history_hit(self,player,damage,tick,cause=2,object_datum=NONE):
        self.w(TIME+8,tick)
        self.g.reg_write(UC_X86_REG_XMM0,int.from_bytes(struct.pack('<f',damage),'little'))
        # The nondeath branch returns after complete history recording. It
        # excludes the separate enemy-kill animation/physics response tail.
        self.call(0xcba60,(2,0,player,0,object_datum,cause),((UC_X86_REG_EAX,0x10000),))


def score_cases(o,c):
    rows=[]
    for flags in (0,1,2,3,4,6):
        for kind in ('enemy','suicide','relationship_flag','uncredited','victim_flag1'):
            o.fixture(c)
            a,v=o.datums[0],o.datums[3]
            relation=int(kind in ('suicide','relationship_flag'))
            if kind=='suicide':v=a
            if kind=='uncredited':a=NONE
            if kind=='victim_flag1':o.sh(PLAYERS+3*0x21c+2,3)
            o.w(o.options+0x22c,flags)
            before=o.engine_state()
            o.call(0x2c2350,(a,v,relation,NONE))
            after=o.engine_state()
            assert before['rng']==after['rng']
            rows.append(dict(case=kind,flags22c=flags,attacker=f'{a:08x}',victim=f'{v:08x}',
                relationship_flag=relation,cause_class=-1,before=before,after=after,
                events=o.accounting_events))
    boundaries=[]
    for start,delta in ((0,1),(0,-1),(29999,10),(-29999,-10),(0,65535),(0,65536),
                        (0,32768),(0,-32769)):
        o.fixture(c); o.sh(STAT+4,start)
        before=o.engine_state()
        ret=o.call(0x15ce00,(o.datums[0],),((UC_X86_REG_EAX,delta&NONE),))&255
        signed_delta=struct.unpack('<h',struct.pack('<H',delta&0xffff))[0]
        assert o.h(STAT+4)==max(-30000,min(30000,start+signed_delta)) and ret==1
        boundaries.append(dict(start=start,input_delta=delta,truncated_signed_delta=signed_delta,
            result=ret,before=before,after=o.engine_state(),events=o.accounting_events))
    for gate,value in (('no_engine',0),('lifecycle',0),('lifecycle',2),('status',2),('player_none',NONE)):
        o.fixture(c)
        player=o.datums[0]
        if gate=='no_engine':o.w(o.a['engine_table']+8,0)
        elif gate=='lifecycle':o.sh(ENGINE_GLOBALS+0x6c,value)
        elif gate=='status':o.w(ENGINE_GLOBALS+0xc04,value)
        else:player=NONE
        before=o.engine_state()
        ret=o.call(0x15ce00,(player,),((UC_X86_REG_EAX,1),))&255
        assert ret==0 and o.engine_state()==before
        boundaries.append(dict(gate=gate,value=value,result=ret,before=before,after=o.engine_state()))
    return rows,boundaries


def attribution_cases(o,c):
    # Rows are (absolute attacker slot, amount, original timestamp, cause).
    fixtures=[
        ('direct_enemy',0,[],200),('uncredited_environment',None,[],200),
        ('direct_suicide',3,[],200),
        ('environment_recent_enemy',None,[(0,.5,199,2)],200),
        ('environment_at_six_second_cutoff',None,[(0,.5,20,2)],200),
        ('environment_one_tick_inside_cutoff',None,[(0,.5,21,2)],200),
        ('environment_largest_not_latest',None,[(0,.8,100,2),(15,.1,199,3)],200),
        ('environment_equal_damage_first_entry',None,[(0,.5,100,2),(15,.5,199,3)],200),
        ('direct_enemy_not_overridden',0,[(15,.9,199,3),(0,.1,100,2)],200),
        ('suicide_with_recent_enemy',3,[(0,.5,199,2)],200),
        ('environment_only_self_history',None,[(3,.5,199,2)],200),
        ('environment_initial_clock_unsigned_cutoff',None,[(0,.5,1,2)],2),
    ]
    rows=[]
    for name,incoming,hits,clock in fixtures:
        o.fixture(c)
        victim=o.datums[3]
        o.w(OBJECTS+0x13c,victim); o.w(PLAYERS+3*0x21c+0x2c,0x10000)
        for slot,damage,tick,cause in hits:
            o.history_hit(o.datums[slot],damage,tick,cause)
        o.w(TIME+8,clock)
        history=o.history_state(); before=o.engine_state()
        o.skip_writer=True; o.accounting_events=[]
        source_incoming=NONE if incoming is None else o.datums[incoming]
        source_designator=NONE if incoming is None else incoming
        o.call(0x1ed960,(STAT,0x10000,source_incoming,source_designator,1))
        after_credit=o.engine_state()
        writer=next(e for e in o.accounting_events if e['kind']=='supplied_history_and_death_event_writer_boundary')
        credit_events=list(o.accounting_events)
        assert not o.marker_calls
        # Original callback is separately executed using the actual writer
        # arguments. Marker/countdown/kill-feed/medal intermediates are excluded.
        o.skip_writer=False; o.accounting_events=[]
        o.call(0x2c2350,(int(writer['attacker'],16),int(writer['victim'],16),writer['relationship_flag'],NONE))
        after_score=o.engine_state()
        rows.append(dict(case=name,game_tick=clock,source_incoming=f'{source_incoming:08x}',
            source_incoming_designator=i32(source_designator),
            fixture_hits=[dict(slot=s,damage=d,tick=t,cause=k) for s,d,t,k in hits],
            source_produced_history=history,before=before,source_writer_request=writer,
            after_original_attribution_and_death_stats=after_credit,
            attribution_events=credit_events,after_separate_original_slayer_callback=after_score,
            slayer_events=o.accounting_events))
    for name,attacker in (('stale_direct_datum',0x12340000),('stale_history_datum',NONE)):
        o.fixture(c); victim=o.datums[3]
        o.w(OBJECTS+0x13c,victim); o.w(PLAYERS+3*0x21c+0x2c,0x10000)
        if name=='stale_history_datum':o.history_hit(0x12340000,.5,199)
        o.w(TIME+8,200); before=o.engine_state();o.skip_writer=True
        o.call(0x1ed960,(STAT,0x10000,attacker,0,1))
        writer=next(e for e in o.accounting_events if e['kind']=='supplied_history_and_death_event_writer_boundary')
        assert writer['attacker']=='ffffffff'
        rows.append(dict(case=name,source_incoming=f'{attacker:08x}',
            source_produced_history=o.history_state(),before=before,
            source_writer_request=writer,after_original_attribution_and_death_stats=o.engine_state(),
            attribution_events=o.accounting_events))
    return rows


def history_writer_cases(o,c):
    rows=[]
    cases=[
        ('accumulate_same_player',[(0,.25,10,2,NONE),(0,.5,15,3,NONE)]),
        ('accumulate_same_object_preserves_first_owner',[(0,.25,10,2,0x10002),(3,.5,15,3,0x10002)]),
        ('full_table_protect_largest_replace_oldest_other',[
            (0,1.,10,2,NONE),(3,.2,20,2,NONE),(7,.3,30,2,NONE),
            (11,.4,40,2,NONE),(15,.1,50,2,NONE)]),
        ('full_table_equal_maximum_protect_first',[
            (0,.5,10,2,NONE),(3,.5,20,2,NONE),(7,.5,30,2,NONE),
            (11,.5,40,2,NONE),(15,.1,50,2,NONE)]),
    ]
    for name,hits in cases:
        o.fixture(c,(0,3,7,11,15));snapshots=[]
        for slot,damage,tick,cause,obj in hits:
            before=o.history_state()
            o.history_hit(o.datums[slot],damage,tick,cause,obj)
            snapshots.append(dict(slot=slot,full_player=f'{o.datums[slot]:08x}',
                damage=damage,tick=tick,cause=cause,object=f'{obj:08x}',
                before=before,after=o.history_state()))
        final=o.history_state()
        if name.startswith('accumulate'):
            assert final[0]['damage_bits']=='3f400000' and final[0]['timestamp']==15
            assert final[0]['player']==f'{o.datums[0]:08x}' and final[0]['cause']==3
        else:
            assert [r['player'] for r in final]==[f'{o.datums[s]:08x}' for s in (0,15,7,11)]
        rows.append(dict(case=name,snapshots=snapshots,final=final,
            source_entry='000cba60',nondeath_branch=True))
    return rows


def rank_cases(o,c):
    rows=[]
    cases=[('unique_high',[4,5,1]),('equal_top',[5,5,1]),('all_equal',[0,0,0]),
        ('negative_unique',[-2,-1,-3]),('negative_tie',[-1,-1,-2]),
        ('tie_different_deaths',[5,5,1]),('tie_different_kills',[5,5,1]),
        ('suppressed_high',[99,5,1]),('no_membership_high',[99,5,1])]
    for name,scores in cases:
        o.fixture(c)
        for s,score in zip((0,3,15),scores):o.sh(STAT+4+s*0x1c,score)
        if name=='tie_different_deaths':o.sh(STAT+4+3*0x1c+3*2,12)
        if name=='tie_different_kills':o.sh(STAT+4+3*0x1c+2*2,12)
        if name=='suppressed_high':o.sh(PLAYERS+2,3)
        if name=='no_membership_high':o.g.mem_write(PLAYERS+0xc0,b'\xff')
        before=o.engine_state()
        o.call(0x241500,(0,0),((UC_X86_REG_ESI,RANK),))
        count=o.h(RANK+0x68)
        ordered=[f'{o.u(RANK+n*4):08x}' for n in range(count)]
        tied=[o.g.mem_read(RANK+0x50+n,1)[0] for n in range(count)]
        winner=o.call(0x15c970)
        assert o.engine_state()==before
        if name in ('equal_top','all_equal','negative_tie','tie_different_deaths','tie_different_kills'):
            assert winner==NONE
        else:assert winner==o.datums[3]
        rows.append(dict(case=name,scores=scores,before=before,ordered_players=ordered,
            rank_codes=tied,winner=f'{winner:08x}',after=o.engine_state()))
    return rows


def ending_cases(o,c):
    rows=[]
    for name,start,delta,limit in (
        ('below_limit',23,1,25),('reach_limit',24,1,25),('cross_limit',24,3,25),
        ('negative_does_not_finish',26,-1,25),('unlimited_score',30000,1,0),
        ('positive_delta_wraps_zero',25,65536,25),
        ('positive_delta_wraps_negative',26,65535,25),
        ('negative_delta_wraps_positive',24,-65535,25)):
        o.fixture(c);o.w(o.options+0x18c,limit);o.sh(STAT+4,start)
        before=o.engine_state()
        o.call(0x15ce00,(o.datums[0],),((UC_X86_REG_EAX,delta&NONE),))
        after=o.engine_state()
        assert after['status_c04']==(2 if name in ('reach_limit','cross_limit',
            'positive_delta_wraps_zero','positive_delta_wraps_negative') else 1)
        rows.append(dict(case=name,before=before,delta=delta,score_limit=limit,
            after=after,events=o.accounting_events))
    o.fixture(c);o.w(o.options+0x18c,25);o.sh(STAT+4,24);o.sh(STAT+4+3*0x1c,25)
    before=o.engine_state();o.call(0x15ce00,(o.datums[0],),((UC_X86_REG_EAX,1),))
    end=next(e for e in o.accounting_events if e['kind']=='original_end_request')
    assert end['winner']==f'{o.datums[0]:08x}'
    assert o.h(STAT+4)==o.h(STAT+4+3*0x1c)==25 and o.h(STAT+4+7*2)==1
    rows.append(dict(case='explicit_equal_scores_at_limit',
        fixture='Player three already has score 25 while status is explicitly active; this does not claim an ordinary prior-event lifecycle.',
        before=before,delta=1,after=o.engine_state(),events=o.accounting_events))
    for order in ((0,3),(3,0)):
        o.fixture(c);o.w(o.options+0x18c,25)
        for s in order:o.sh(STAT+4+s*0x1c,24)
        before=o.engine_state();events=[];samples=[]
        for s in order:
            o.accounting_events=[]
            o.call(0x2c2350,(o.datums[s],o.datums[15],0,NONE))
            events.extend(o.accounting_events)
            samples.append(dict(slot=s,after=o.engine_state(),events=o.accounting_events))
        end=next(e for e in events if e['kind']=='original_end_request')
        assert end['winner']==f'{o.datums[order[0]]:08x}'
        assert o.h(STAT+4+order[0]*0x1c)==25 and o.h(STAT+4+order[1]*0x1c)==24
        rows.append(dict(case='two_ordered_score_limit_callbacks',order=list(order),
            before=before,samples=samples,after=o.engine_state()))
    for name,scores in (('time_unique',[4,5,1]),('time_tied',[5,5,1])):
        o.fixture(c);o.w(o.options+0x190,5)
        for s,score in zip((0,3,15),scores):o.sh(STAT+4+s*0x1c,score)
        samples=[]
        for tick in (0,119,120,149,150,151):
            o.w(TIME+8,tick);o.accounting_events=[]
            before=o.engine_state()
            o.call(0x15d490)
            after=o.engine_state()
            assert after['status_c04']==(1 if tick<150 else 2)
            samples.append(dict(game_tick=tick,before=before,after=after,events=o.accounting_events))
        end=next(e for e in samples[4]['events'] if e['kind']=='original_end_request')
        assert end['winner']==('ffffffff' if name=='time_tied' else f'{o.datums[3]:08x}')
        rows.append(dict(case=name,time_limit_seconds=5,rate=30,scores=scores,samples=samples))
    for clock in (-1,0,30,983040,983041):
        o.fixture(c);o.w(TIME+8,clock&NONE);before=o.engine_state();o.call(0x15d490)
        assert o.u(ENGINE_GLOBALS+0xc04)==1
        rows.append(dict(case='unlimited_clock',game_tick=clock,before=before,after=o.engine_state(),events=o.accounting_events))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=OUT/'slayer-accounting-report.json')
    args=p.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(c.data).hexdigest()==CACHE_PIN
    o=SlayerOracle()
    scores,boundaries=score_cases(o,c)
    attribution=attribution_cases(o,c); history=history_writer_cases(o,c)
    ranks=rank_cases(o,c); endings=ending_cases(o,c)
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    ranges=((0x2c2350,0x2c2450),(0x15ce00,0x15cf70),(0x15c970,0x15c9e0),
        (0x15c9e0,0x15cc90),(0x15d490,0x15d610),(0x163b10,0x163be0),
        (0xcba60,0xcbdc0),(0x1ed960,0x1edf70),(0x1edf70,0x1ee130),
        (0x1ee130,0x1ee240),(0x1ee240,0x1ee2f0),(0x1ed6a0,0x1ed7b0),
        (0x241500,0x241800),(0x241880,0x241aa0),(0x241aa0,0x241c50),
        (0x241c50,0x241e00),(0x15d7c0,0x15dbc0),(0x253f10,0x253f30))
    listing=[]
    for start,end in ranges:
        listing.append(f'RANGE {start:08x}..{end:08x}')
        listing.extend(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
            for i in md.disasm(o.x.read(start,end-start),start))
    path=OUT/'tu11-slayer-accounting-instructions.txt';path.write_text('\n'.join(listing)+'\n')
    report=dict(tool_sha256=sha(__file__),executable_sha256=VERSIONS['tu11']['pin'],cache_sha256=CACHE_PIN,
        dependencies={str(p.relative_to(ROOT)):sha(p) for p in
            (ROOT/'tools/oracle_mp_spawn.py',ROOT/'tools/oracle_mp_spawn_lifecycle.py',ROOT/'tools/oracle_mp_spawn_ownership.py')},
        source_listing=str(path.relative_to(ROOT)),source_listing_sha256=sha(path),
        slayer_object='00487188',vtable='00463b48',death_callback_slot30='002c2350',
        fixture_globals=dict(lifecycle=1,status_c04=1,engine_index=2,team_mode=False,
            round_limit_kind=0,round_index=0,rate=30,seconds_per_tick_bits='3d088889',
            persistent_profile_recording_518490=0,replicated_engine_entity='ffffffff',
            source_membership_bytes={0:0,3:3,15:15},salt_formula='7 + 5*slot'),
        score_callbacks=scores,score_writer_boundaries=boundaries,
        attribution=attribution,damage_history_writer=history,ranking=ranks,ending=endings,
        summary=dict(score_callbacks=len(scores),score_writer_cases=len(boundaries),
            attribution_cases=len(attribution),history_writer_cases=len(history),
            ranking_cases=len(ranks),ending_cases=len(endings)),
        scope=__doc__)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['summary']))


if __name__=='__main__':main()
