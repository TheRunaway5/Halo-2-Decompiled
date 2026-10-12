#!/usr/bin/env python3
"""Full-salted original SMG result/history/lethal-attribution source sequences.

Source pools, host FFA identity, constructors, amount/result consumers and actual
Slayer callbacks execute. Supplied impact input, body arena and death-event writer
boundaries are explicit; no original instructions or marker poses are patched.
"""
import argparse
import json
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP,
    UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0)
from oracle_mp_spawn import ROOT,VERSIONS,CACHE_PIN,Cache,PLAYER_POOL,OBJECT_POOL,ENGINE_GLOBALS,SEED
from oracle_mp_spawn_lifecycle import EXTRA,TIME
from oracle_mp_spawn_slayer_accounting import STAT,i32
from oracle_mp_spawn_smg_damage import EVENT,RESULT,NONE,sha,bits
from oracle_mp_spawn_datums import DatumOracle,DESCRIPTOR,OWNERS,OUT

UNIT_RECORD=EXTRA+0x226000
FROZEN_REPORTS={
    'datums-report.json':'9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629',
    'smg-producers-report.json':'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa',
    'smg-damage-report.json':'2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca'}

class AttributionOracle(DatumOracle):
    def __init__(self,c,event_template):
        self.phase='fixture';self.trace=[];self.victim_body=None;self.live_players={};self.units={}
        self.input_event_template=event_template
        super().__init__(c)
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.attribution_write)

    def history_state(self):
        if self.victim_body is None:return super().history_state()
        p=self.victim_body
        return [dict(index=n,timestamp=f'{self.u(p+0x2f0+n*16):08x}',
            damage_bits=f'{self.u(p+0x2f4+n*16):08x}',object=f'{self.u(p+0x2f8+n*16):08x}',
            player=f'{self.u(p+0x2fc+n*16):08x}',cause=self.g.mem_read(p+0x330+n,1)[0])for n in range(4)]

    def stats(self):
        return {str(s):dict(full_player=f'{d:08x}',pool_salt=f'{self.h(self.player_ptr(d))&0xffff:04x}',
            current=f'{self.u(self.player_ptr(d)+0x2c):08x}',previous=f'{self.u(self.player_ptr(d)+0x30):08x}',
            membership=self.g.mem_read(self.player_ptr(d)+0xc0,1)[0],
            fields=[self.h(STAT+4+s*0x1c+n*2)for n in range(12)],
            last_kill_tick=f'{self.u(STAT+0x1c+s*0x1c):08x}',
            counter_1c4=self.h(self.player_ptr(d)+0x1c4))for s,d in self.live_players.items()}

    def snapshot(self):
        return dict(tick=self.u(TIME+8),stat_valid=self.g.mem_read(STAT,1)[0],
            engine_pointer=f'{self.u(self.a["engine_table"]+8):08x}',mode=self.g.mem_read(self.options+0xc,1)[0],
            engine_flags=f'{self.u(ENGINE_GLOBALS):08x}',stats=self.stats(),history=self.history_state(),
            units={name:dict(datum=f'{d:08x}',body=f'{p:08x}',owner=f'{self.u(p+0x13c):08x}',
                previous_owner=f'{self.u(p+0x140):08x}',designator=self.h(p+0x138),
                dead=bool(self.g.mem_read(p+0x10a,1)[0]&4),counter_2ea=self.h(p+0x2ea),
                last_counter_tick=f'{self.u(p+0x2ec):08x}',body_bits=f'{self.u(p+0xec):08x}',
                shield_bits=f'{self.u(p+0xf0):08x}')for name,(d,p)in self.units.items()},
            rng=[f'{self.u(SEED+n):08x}'for n in (0,4)])

    def hook(self,g,a,size,user):
        if self.phase!='fixture':
            sp=g.reg_read(UC_X86_REG_ESP)
            def args(n):return [f'{self.u(sp+4+i*4):08x}'for i in range(n)]
            names={0xd9180:'damage_entry',0xdaa90:'normalized_result_producer',0xdac40:'normalized_result_consumer',
                0x1ed960:'read_previous_history',0x1edf70:'credited_kill_stats',0x1ee130:'victim_death_stats',
                0x1ee240:'nonenemy_history_counter',0x1ed6a0:'stat_delta',0x15e4d0:'death_writer_boundary',
                0x15e250:'damage_engine_callback',0xcb410:'aftermath_consumer',0xcba60:'history_writer',
                0xd7e00:'body_depleted',0xcbdc0:'death_cleanup',0x14e240:'ownership_binding',
                0x2c2350:'slayer_death_callback',0x15ce00:'score_delta',0x1ec930:'local_kill_category',
                0x1e2f50:'enemy_relationship',0x253f10:'slayer_membership_relationship'}
            if a in names:
                row=dict(stage=names[a],entry=f'{a:08x}',phase=self.phase,state=self.snapshot())
                row['registers']={name:f'{g.reg_read(reg):08x}'for name,reg in
                    (('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),
                     ('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI))}
                if a==0xdac40:
                    row.update(unit=args(1)[0],result_hex=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),0x58)).hex())
                elif a==0x1ed960:row['arguments']=args(5)
                elif a==0x15e4d0:
                    row['arguments']=args(4);row['supplied']=True
                    row['semantic_arguments']=dict(credited_player=args(4)[0],victim_player=args(4)[1],
                        relationship_byte=self.u(sp+12)&255,cause_byte=self.u(sp+16)&255)
                elif a==0xcba60:
                    row.update(arguments=args(6),victim_unit=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                        requested_amount_bits=f'{g.reg_read(UC_X86_REG_XMM0)&NONE:08x}',
                        semantic_arguments=dict(damage_class=self.h(sp+4),lethal_byte=self.u(sp+8)&255,
                            attacker_full=args(6)[2],designator=self.h(sp+16),source_object=args(6)[4],cause_byte=self.u(sp+24)&255))
                elif a==0x1ed6a0:
                    row.update(arguments=args(4),recipient_full=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                        field_index=self.u(sp+8),delta=self.h(sp+12),profile_category=g.reg_read(UC_X86_REG_ECX))
                elif a==0x1edf70:row.update(arguments=args(3),recipient_full=f'{g.reg_read(UC_X86_REG_EDX):08x}')
                elif a==0x1ee130:row.update(arguments=args(4),victim_full=f'{g.reg_read(UC_X86_REG_EDI):08x}',
                    semantic_arguments=dict(credited_player=args(4)[1],input_self_byte=self.u(sp+12)&255,team_betrayal_byte=self.u(sp+16)&255))
                elif a==0x1ee240:row.update(arguments=args(1),recipient_full=f'{g.reg_read(UC_X86_REG_ESI):08x}')
                elif a in (0x2c2350,0x253f10):row['arguments']=args(4 if a==0x2c2350 else 2)
                elif a==0x15ce00:row.update(recipient=args(1)[0],delta=i32(g.reg_read(UC_X86_REG_EAX)))
                elif a==0x14e240:row.update(player=args(1)[0],new_unit=f'{g.reg_read(UC_X86_REG_EBX):08x}')
                elif a==0x1e2f50:
                    row.update(first_signed=struct.unpack('<h',struct.pack('<H',g.reg_read(UC_X86_REG_ECX)&0xffff))[0],
                        second_signed=struct.unpack('<h',struct.pack('<H',g.reg_read(UC_X86_REG_ESI)&0xffff))[0])
                self.trace.append(row)
                if a==0x15e4d0:
                    # The named death-writer adapter runs only the actual Slayer
                    # callback at the original writer call site. Its four-argument
                    # stdcall return continues 1ED960 without nested emulation.
                    # Marker/timer/events/medals are omitted; class NONE is an
                    # explicit adapter input (ordinary 22C=2 ignores that class).
                    self.trace.append(dict(stage='supplied_death_writer_adapter',entry=f'{a:08x}',
                        callback='002c2350',callback_arguments=[args(4)[0],args(4)[1],
                            f'{self.u(sp+12)&255:08x}','ffffffff'],
                        original_writer_arguments=args(4),state=self.snapshot()))
                    self.w(sp+12,self.u(sp+12)&255);self.w(sp+16,NONE)
                    g.reg_write(UC_X86_REG_EIP,0x2c2350);return
            if a in self.source_services:
                name,pop,result=self.source_services[a]
                self.trace.append(dict(stage='supplied_service',entry=f'{a:08x}',name=name,
                    arguments=args(pop//4),result=result,state=self.snapshot()))
            if a==0xcbe95:
                self.trace.append(dict(stage='stop_after_original_ownership_release',entry=f'{a:08x}',state=self.snapshot()))
        super().hook(g,a,size,user)

    def attribution_write(self,g,access,a,size,value,user):
        if self.phase!='fixture'and a in getattr(self,'attribution_watch',{}):
            self.trace.append(dict(stage='source_write',entry=f'{g.reg_read(UC_X86_REG_EIP):08x}',
                field=self.attribution_watch[a],size=size,value=f'{value&((1<<(size*8))-1):0{size*2}x}'))

    def join_host(self,slot):
        self.reset_controllers();local=self.publish(0);host=self.host_copy(local,slot)
        self.published=host;d=self.join(slot)
        assert self.g.mem_read(self.player_ptr(d)+0xc0,1)[0]==slot
        self.live_players[slot]=d;return d

    def unit(self,player,name):
        d=self.object();p=self.body(d);record=UNIT_RECORD
        self.g.mem_write(record,bytes(0xc4));self.w(record+0x64,self.g.mem_read(self.player_ptr(player)+0xc0,1)[0])
        self.call(0xc55d1,(0,0,0,0,0,record),((UC_X86_REG_ESI,p),
            (UC_X86_REG_EBP,self.u(self.u(self.a['tags'])+(self.u(p)&0xffff)*16+8)),(UC_X86_REG_EBX,NONE)),end=0xc560f)
        self.bind(player,d);self.units[name]=(d,p)
        assert self.h(p+0x138)==self.g.mem_read(self.player_ptr(player)+0xc0,1)[0]
        self.f(p+0xe4,self.cache.f(self.info-self.cache.magic+0x28))
        self.f(p+0xe8,self.cache.f(self.info-self.cache.magic+0x8c));self.f(p+0xec,1);self.f(p+0xf0,1)
        return d

    def fixture(self,c):
        self.phase='fixture';self.victim_body=None;self.trace=[];self.live_players={};self.units={}
        super().fixture(c)
        for slot,name in ((0,'attacker0'),(3,'victim'),(7,'attacker7'),(15,'attacker15')):
            d=self.join_host(slot);self.unit(d,name)
        self.datums=dict(self.live_players);self.victim_unit,self.victim_body=self.units['victim']
        self.attribution_watch={}
        for slot,d in self.live_players.items():
            p=self.player_ptr(d)
            for off in (0x2c,0x30,0x1c4):self.attribution_watch[p+off]=f'player{slot}.{off:03x}'
            for n in range(12):self.attribution_watch[STAT+4+slot*0x1c+n*2]=f'stat{slot}.field{n}'
            self.attribution_watch[STAT+0x1c+slot*0x1c]=f'stat{slot}.last_kill_tick'
        self.add_unit_watches()
        self.trace=[];self.accounting_events=[];self.damage_events=[];self.events=[]
        self.skip_writer=True

    def add_unit_watches(self):
        for name,(d,p)in self.units.items():
            for off in (0x13c,0x140,0x2ea,0x2ec):self.attribution_watch[p+off]=f'{name}.{off:03x}'
            for n in range(4):
                for off in (0x2f0,0x2f4,0x2f8,0x2fc):self.attribution_watch[p+off+n*16]=f'{name}.history{n}.{off:03x}'
                self.attribution_watch[p+0x330+n]=f'{name}.history{n}.cause'

    def seed_history(self,slot,amount,tick,cause=8,source_unit=None):
        self.phase='explicit_previous_history_seed';self.w(TIME+8,tick)
        player=self.live_players[slot];unit=self.units['victim'if slot==3 else f'attacker{slot}'][0]if source_unit is None else source_unit
        self.g.reg_write(UC_X86_REG_XMM0,bits(amount))
        self.call(0xcba60,(2,0,player,slot,unit,cause),((UC_X86_REG_EAX,self.victim_unit),))

    def shot(self,attacker,owner,designator):
        projectile=self.object(5);p=self.body(projectile)
        self.w(OWNERS,attacker);self.w(OWNERS+4,owner);self.w(OWNERS+8,designator&NONE)
        self.call(0xb8820,(self.u(p),NONE,OWNERS),((UC_X86_REG_ESI,DESCRIPTOR),))
        self.call(0xb8d0d,registers=((UC_X86_REG_EBP,DESCRIPTOR),(UC_X86_REG_EDI,p)),end=0xb8d2a)
        return projectile

    def hit(self,attacker,owner,designator,tick,stop=False,captured_projectile=None):
        self.phase='original_damage_and_attribution';self.trace=[];self.accounting_events=[];self.damage_events=[]
        self.w(TIME+8,tick);self.g.mem_write(EVENT,self.input_event_template)
        # Original BD080 copies the already captured full owner triple; the
        # event's remaining real first-impact bytes are the frozen hit fixture.
        if owner is not None:
            projectile=self.shot(attacker,owner,designator)if captured_projectile is None else captured_projectile
            self.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,EVENT+8)))
        else:
            self.w(EVENT+8,attacker);self.w(EVENT+0xc,NONE);self.sh(EVENT+0x10,designator)
        self.phase='original_damage_and_attribution';before=self.snapshot();event=bytes(self.g.mem_read(EVENT,0x88)).hex()
        self.stop_at=0xcbe95 if stop else None
        try:
            self.call(0xd9180,(EVENT,self.victim_unit,NONE,NONE,NONE,0),end=0xcbe95 if stop else 0x3000000)
        except AssertionError:
            if not(stop and self.g.reg_read(UC_X86_REG_EIP)==0x3000000):raise
        actual_endpoint=self.g.reg_read(UC_X86_REG_EIP)
        self.stop_at=None;after=self.snapshot();result=next(e for e in self.trace if e['stage']=='normalized_result_consumer')
        raw=bytes.fromhex(result['result_hex']);writer=next((e for e in self.trace if e['stage']=='death_writer_boundary'),None)
        trace=list(self.trace);self.trace=[];self.phase='fixture'
        stages=[e['stage']for e in trace]
        if writer:
            assert stages.index('read_previous_history')<stages.index('slayer_death_callback')<stages.index('victim_death_stats')<stages.index('aftermath_consumer')
            reader=next(e for e in trace if e['stage']=='read_previous_history')
            callback=next(e for e in trace if e['stage']=='slayer_death_callback')
            assert reader['state']['history']==callback['state']['history']
        history_call=next((e for e in trace if e['stage']=='history_writer'),None)
        if history_call:
            assert history_call['requested_amount_bits']==f'{bits(sum(struct.unpack_from("<2f",raw,0x44))):08x}'
            if writer:assert stages.index('victim_death_stats')<stages.index('history_writer')<stages.index('death_cleanup')
        captured=None if owner is None else dict(datum=f'{projectile:08x}',preexisting_for_transition=captured_projectile is not None,
            c4=f'{self.u(self.body(projectile)+0xc4):08x}',c8=f'{self.u(self.body(projectile)+0xc8):08x}',
            c2=self.h(self.body(projectile)+0xc2))
        return dict(tick=tick,attacker=f'{attacker:08x}',owner=f'{NONE if owner is None else owner:08x}',designator=designator,
            captured_projectile=captured,event_hex=event,result_hex=raw.hex(),result_fields={f'{off:02x}':f'{struct.unpack_from("<I",raw,off)[0]:08x}'for off in (4,0xc,0x10,0x44,0x48)},
            before=before,after_damage=after,death_writer=writer,original_trace=trace,
            stop_exclusive=f'{actual_endpoint:08x}',complete_original_damage_return=actual_endpoint==0x3000000)

    def leave_rejoin(self):
        self.phase='actual_leave_rejoin';old=self.live_players[0]
        self.call(0x14dcb0,registers=((UC_X86_REG_EAX,old),))
        new=self.join_host(0);self.unit(new,'rejoined0');self.add_unit_watches()
        assert old!=new and self.lookup(PLAYER_POOL,old)==0
        return dict(old=f'{old:08x}',new=f'{new:08x}',original_entries=list(self.datum_events),state=self.snapshot())

    def respawn_attacker(self):
        self.phase='source_attacker_ownership_transition';d,p=self.units['attacker0']
        self.stop_at=0xd7e42;self.call(0xd7e00,(d,0,0,0),end=0xd7e42);self.stop_at=None
        self.stop_at=0xcbe95;self.call(0xcbdc0,(d,NONE,0,0,0),end=0xcbe95);self.stop_at=None
        new=self.unit(self.live_players[0],'respawned0');self.add_unit_watches()
        return dict(old_unit=f'{d:08x}',new_unit=f'{new:08x}',state=self.snapshot(),trace=list(self.trace))


def sequences(o,c):
    rows=[]
    for case in ('direct_enemy','direct_self','self_after_enemy_history','stale_after_leave_rejoin',
                 'old_unit_after_attacker_respawn','two_enemy_assist','environment_after_enemy_history'):
        o.fixture(c);hits=[];transition=None
        captured_player=o.live_players[0];captured_unit=o.units['attacker0'][0]
        for i in range(25):
            captured_projectile=None
            slot=3 if case=='direct_self'or(case=='self_after_enemy_history'and i==24)else 15 if case=='two_enemy_assist'and i>=20 else 0
            a=o.live_players[slot];u=o.units['victim'if slot==3 else f'attacker{slot}'][0];designator=slot
            if i==24:
                if case=='stale_after_leave_rejoin':
                    captured_projectile=o.shot(captured_player,captured_unit,0)
                    transition=o.leave_rejoin();a=captured_player;u=captured_unit;designator=0
                elif case=='old_unit_after_attacker_respawn':
                    captured_projectile=o.shot(captured_player,captured_unit,0)
                    transition=o.respawn_attacker();a=captured_player;u=captured_unit;designator=0
                elif case=='environment_after_enemy_history':a=NONE;u=None;designator=-1
            hits.append(o.hit(a,u,designator,200+2*i,stop=i==24,captured_projectile=captured_projectile))
        assert all(not h['death_writer']for h in hits[:-1])and hits[-1]['death_writer']
        writer=hits[-1]['death_writer']['arguments'];expected=NONE if case=='stale_after_leave_rejoin'else o.live_players[3]if case=='direct_self'else o.live_players[15]if case=='two_enemy_assist'else captured_player
        assert int(writer[0],16)==expected,(case,writer,hex(expected))
        stats=hits[-1]['after_damage']['stats'];assert stats['3']['fields'][3]==1
        assert stats['3']['fields'][4]==int(case in ('direct_self','self_after_enemy_history'))
        if case=='two_enemy_assist':assert stats['0']['fields'][6]==1 and stats['15']['fields'][2]==1
        if case=='stale_after_leave_rejoin':assert stats['0']['fields'][2]==0 and stats['0']['fields'][0]==0
        if case=='old_unit_after_attacker_respawn':
            assert hits[-1]['after_damage']['units']['respawned0']['counter_2ea']==1
            assert hits[-1]['after_damage']['units']['attacker0']['counter_2ea']==0
        rows.append(dict(case=case,hits=hits,transition=transition,source_players={str(s):f'{d:08x}'for s,d in o.live_players.items()}))
    return rows


def history_cases(o,c):
    rows=[]
    fixtures=(
        ('four_enemy_self_entries',None,[(0,.8,199),(7,.4,198),(15,.2,197),(3,.9,199)],200),
        ('assist_exact_threshold',None,[(0,.5,199),(7,.2,198),(15,.20000001788139343,197),(3,.9,199)],200),
        ('equal_enemy_max_first',None,[(0,.5,199),(7,.5,198),(15,.1,197),(3,.9,199)],200),
        ('expired_enemy_larger',None,[(0,.8,20),(7,.4,21),(15,.2,197),(3,.9,199)],200),
        ('direct_enemy_no_matching_history',15,[(0,.8,199),(7,.4,198),(3,.9,199)],200),
        ('stale_direct_suppresses_other_valid_enemy',0,[(0,.5,199),(7,.8,198),(15,.4,197),(3,.1,199)],200),
        ('environment_stale_plus_valid_history',None,[(0,.5,199),(7,.8,198),(15,.4,197),(3,.1,199)],200),
        ('direct_enemy_smaller_history',15,[(0,.8,199),(7,.4,198),(15,.1,197),(3,.9,199)],200),
        ('direct_self_with_four_entries',3,[(0,.8,199),(7,.4,198),(15,.2,197),(3,.9,199)],200),
        ('no_attacker_no_history',None,[],200),
        ('unsigned_initial_cutoff',None,[(0,.8,1),(7,.4,1),(15,.2,1),(3,.9,1)],2))
    for name,incoming,seeds,tick in fixtures:
        o.fixture(c);seed_traces=[]
        for slot,amount,at in seeds:
            o.trace=[];o.seed_history(slot,amount,at);seed_traces.extend(o.trace)
        previous=o.history_state();o.f(o.victim_body+0xf0,0);o.f(o.victim_body+0xec,.001)
        a=NONE if incoming is None else o.live_players[incoming]
        u=None if incoming is None else o.units['victim'if incoming==3 else f'attacker{incoming}'][0]
        transition=None;captured_projectile=None
        if name in ('stale_direct_suppresses_other_valid_enemy','environment_stale_plus_valid_history'):
            if incoming is not None:captured_projectile=o.shot(a,u,incoming)
            transition=o.leave_rejoin()
        hit=o.hit(a,u,-1 if incoming is None else incoming,tick,True,captured_projectile=captured_projectile)
        assert hit['death_writer']
        stats=hit['after_damage']['stats']
        expected_credit={
            'four_enemy_self_entries':0,'assist_exact_threshold':0,'equal_enemy_max_first':0,
            'expired_enemy_larger':7,'direct_enemy_no_matching_history':15,
            'stale_direct_suppresses_other_valid_enemy':None,'environment_stale_plus_valid_history':7,'direct_enemy_smaller_history':15,'direct_self_with_four_entries':3,
            'no_attacker_no_history':None,'unsigned_initial_cutoff':None}[name]
        expected=NONE if expected_credit is None else o.live_players[expected_credit]
        assert int(hit['death_writer']['arguments'][0],16)==expected
        expected_assists={
            'four_enemy_self_entries':(7,),'assist_exact_threshold':(15,),'equal_enemy_max_first':(7,),
            'expired_enemy_larger':(0,15),'direct_enemy_no_matching_history':(),
            'stale_direct_suppresses_other_valid_enemy':(),'environment_stale_plus_valid_history':(15,),'direct_enemy_smaller_history':(0,7),'direct_self_with_four_entries':(0,7),
            'no_attacker_no_history':(),'unsigned_initial_cutoff':()}[name]
        assert all(v['fields'][6]==int(int(slot)in expected_assists)for slot,v in stats.items())
        rows.append(dict(case=name,explicit_seed_requests=[dict(slot=s,amount_bits=f'{bits(d):08x}',tick=t)for s,d,t in seeds],
            source_previous_history=previous,previous_history_source_trace=seed_traces,lethal_hit=hit,transition=transition,
            explicit_health_boundary=dict(body_bits=f'{bits(.001):08x}',shield_bits='00000000')))
    return rows


def engine_gate_cases(o,c):
    rows=[]
    for gate in ('engine_absent','stats_invalid','mode_four'):
        o.fixture(c);o.seed_history(0,.5,199);o.f(o.victim_body+0xf0,0);o.f(o.victim_body+0xec,.001)
        if gate=='engine_absent':o.w(o.a['engine_table']+8,0)
        elif gate=='stats_invalid':o.g.mem_write(STAT,b'\0')
        else:o.g.mem_write(o.options+0xc,b'\x04')
        hit=o.hit(o.live_players[0],o.units['attacker0'][0],0,200,True)
        assert hit['death_writer']is None and hit['after_damage']['stats']['3']['fields'][3]==0
        rows.append(dict(case=gate,explicit_runtime_gate=gate,lethal_hit=hit))
    for gate in ('result_bit_800','result_lethal_bit_clear'):
        o.fixture(c);o.seed_history(0,.5,199);o.f(o.victim_body+0xf0,0);o.f(o.victim_body+0xec,.001)
        o.phase='original_preconsumer_result_generation';o.trace=[];o.w(TIME+8,200)
        o.g.mem_write(EVENT,o.input_event_template)
        projectile=o.shot(o.live_players[0],o.units['attacker0'][0],0)
        o.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,EVENT+8)))
        o.stop_at=0xdac40;o.call(0xd9180,(EVENT,o.victim_unit,NONE,NONE,NONE,0),end=0xdac40);o.stop_at=None
        original=bytes(o.g.mem_read(o.g.reg_read(UC_X86_REG_EDI),0x58));generation=list(o.trace)
        o.g.mem_write(RESULT,original);flags=o.u(RESULT+4)
        changed=flags|0x800 if gate=='result_bit_800'else flags&~1;o.w(RESULT+4,changed)
        o.trace=[];o.phase='explicit_normalized_result_gate';before=o.snapshot()
        end=0xcbe95 if gate=='result_bit_800'else 0x3000000;o.stop_at=0xcbe95 if end==0xcbe95 else None
        o.call(0xdac40,(o.victim_unit,),((UC_X86_REG_EDI,RESULT),),end=end);o.stop_at=None
        assert not any(e['stage']=='read_previous_history'for e in o.trace)
        rows.append(dict(case=gate,original_generated_result_hex=original.hex(),
            explicitly_changed_result_hex=bytes(o.g.mem_read(RESULT,0x58)).hex(),changed_field='04',
            original_flags=f'{flags:08x}',changed_flags=f'{changed:08x}',source_result_generation_trace=generation,
            before=before,after=o.snapshot(),original_consumer_trace=list(o.trace),stop_exclusive=f'{end:08x}'))
        o.phase='fixture'
    return rows


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'lethal-attribution-report.json');args=ap.parse_args()
    for n,pin in FROZEN_REPORTS.items():assert sha(OUT/n)==pin
    producer=json.loads((OUT/'smg-producers-report.json').read_text())
    impact=producer['stock_direct_impact_events'][0]
    event=bytes.fromhex(impact['event_hex'])
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    o=AttributionOracle(c,event);seq=sequences(o,c);history=history_cases(o,c);gates=engine_gate_cases(o,c)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    md=Cs(CS_ARCH_X86,CS_MODE_32);lines=[]
    for a,b in ((0xd9180,0xd9640),(0xdaa90,0xdac40),(0xdac40,0xdb160),(0x1ed960,0x1ee2f0),
        (0x1e2f50,0x1e2fc0),(0x15f680,0x15f6a0),(0x253f10,0x253f30),(0xcb410,0xcbdc0),
        (0xcbdc0,0xcbe95),(0x14e240,0x14e3b0),(0x15e4d0,0x15ed20),(0x15e250,0x15e4d0),(0x2c2350,0x2c2450)):
        lines.append(f'\n# original TU1.1 {a:08x}..{b:08x} exclusive')
        for i in md.disasm(bytes(o.g.mem_read(a,b-a)),a):lines.append(f'{i.address:08x} {i.bytes.hex():<28} {i.mnemonic} {i.op_str}')
    for a in (0x465080,0x46506c,0x4651a0,0x465048,0x447020):lines.append(f'constant {a:08x} {bytes(o.g.mem_read(a,4)).hex()}')
    lp=args.output.with_name('tu11-lethal-attribution-instructions.txt');lp.write_text('\n'.join(lines)+'\n')
    deps=('oracle_mp_spawn_datums.py','oracle_mp_spawn_smg_producers.py','oracle_mp_spawn_smg_damage.py',
        'oracle_mp_spawn_slayer_accounting.py','oracle_mp_spawn_ownership.py','oracle_mp_spawn_lifecycle.py','oracle_mp_spawn.py')
    result=dict(schema=1,source_sha256=VERSIONS['tu11']['pin'],cache_sha256=CACHE_PIN,
        oracle_sha256=sha(__file__),listing_sha256=sha(lp),frozen_reports=FROZEN_REPORTS,
        dependencies={str((ROOT/'tools'/p).relative_to(ROOT)):sha(ROOT/'tools'/p)for p in deps},
        impact_fixture_event_hex=event.hex(),sequence_cases=seq,previous_history_cases=history,engine_gate_cases=gates,
        case_count=len(seq)+len(history)+len(gates),instruction_patches=[],marker_pose_fixtures=[],
        boundaries=['Frozen original first-impact event/world-hit bytes; owner triple replaced by original descriptor/BD080 full-datum copy.',
            'Actual player/host/property/pool source; body arena and active object registration inherited explicit datum fixtures.',
            '25-hit source sequences have no recharge, weapon/reload or world update between supplied impacts.',
            'Four-entry reader cases use complete original CBA60 with explicit varied normalized amount/tick requests and low-health fixture.',
            '15E4D0 is a named writer adapter: actual Slayer 2C2350 executes at this original call site; marker/countdown/event/medal/profile delivery is omitted.',
            'Writer adapter forwards source-selected full attacker/victim and relationship byte, with explicit callback class NONE; ordinary default 22C=2 ignores this class.',
            'Lethal damage stops at CBE95 after original ownership detach, before further animation/AI/physics/type aftermath.'])
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=result['case_count'],output=str(args.output),sha256=sha(args.output))))

if __name__=='__main__':main()
