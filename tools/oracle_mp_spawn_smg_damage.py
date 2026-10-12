#!/usr/bin/env python3
"""Unchanged TU1.1 stock SMG result/history/attribution source slices.

Lockout metadata is mapped at its original addresses. Hit amount, region absence,
salted pools and damage event are explicit fixtures. Services are named and
recorded. No instructions, marker poses or frozen earlier reports are changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
import unicorn
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_EBP,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_XMM0)
from oracle_mp_spawn import (ROOT, VERSIONS, CACHE_PIN, Cache, ENGINE_GLOBALS,
    PLAYERS, OBJECTS, HEADERS, SEED)
from oracle_mp_spawn_lifecycle import EXTRA, TABLE, TIME
from oracle_mp_spawn_slayer_accounting import SlayerOracle, STAT, NONE

OUT=ROOT/'analysis/multiplayer/integration/spawn-source'
EVENT,ACCUM,RESULT=EXTRA+0x200000,EXTRA+0x201000,EXTRA+0x202000
UNIT=0x10000


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]


class DamageOracle(SlayerOracle):
    def __init__(self,c):
        self.damage_events=[];self.source_services={};self.stop_at=None
        self.result_pointer=RESULT
        super().__init__()
        self.cache=c
        page=c.magic&~0xfff
        self.g.mem_map(page,((len(c.data)+c.magic-page+0xfff)//0x1000)*0x1000)
        self.g.mem_write(c.magic,bytes(c.data))

    def fixture(self,c,shield=1.,body=1.,incoming=0,event_flags=0):
        super().fixture(c)
        bipd=next(t for t in c.tags.values() if t['group']=='bipd' and t['name'].endswith('masterchief_mp'))
        dmg=next(t for t in c.tags.values() if t['group']=='jpt!' and t['name']=='objects\\weapons\\rifle\\smg\\damage_effects\\smg_bullet')
        projectile=next(t for t in c.tags.values() if t['group']=='proj' and t['name']=='objects\\weapons\\rifle\\smg\\projectiles\\smg_bullet')
        matg=next(t for t in c.tags.values() if t['group']=='matg')
        model=c.tags[c.u(bipd['offset']+0x38)]
        info=c.block(model['offset']+0x60,0xf8)[0]
        self.damage_tag=int(dmg['datum'],16);self.damage_definition=dmg['offset']+c.magic
        self.info=info+c.magic
        self.w(self.a['globals'],matg['offset']+c.magic)
        self.w(self.a['tags'],TABLE)
        self.w(0x4fcda8,EXTRA+0x203400)
        for d,t in c.tags.items():
            if t['offset'] is not None:
                self.w(TABLE+(d&0xffff)*16,int.from_bytes(t['group'].encode('ascii'),'big'))
                self.w(TABLE+(d&0xffff)*16+8,t['offset']+c.magic)
        for n in range(3):
            self.w(OBJECTS+n*0x800,int(bipd['datum'],16))
            self.w(OBJECTS+n*0x800+0x1bc,NONE)
            self.w(OBJECTS+n*0x800+0x1fc,NONE)
            self.g.mem_write(OBJECTS+n*0x800+0x212,b'\xff')
            self.call(0xc52c0,(UNIT+n,),end=0xc5464)
        for slot in (0,3,15):
            # Explicit ordinary-strength property input; 15F520 consumes it.
            self.g.mem_write(PLAYERS+slot*0x21c+0xc1,b'\x00')
            self.w(PLAYERS+slot*0x21c+0x194,0)
        for slot,n in ((3,0),(0,1),(15,2)):
            record=EXTRA+0x203500
            self.w(record+0x64,slot)
            # Original C52C0 membership producer, after its allocation leaves.
            # The earlier full generic constructor report proves these +64
            # inputs; this slice does not fabricate a complete constructor.
            self.call(0xc55d1,(0,0,0,0,0,record),((UC_X86_REG_ESI,OBJECTS+n*0x800),
                (UC_X86_REG_EBP,bipd['offset']+c.magic),(UC_X86_REG_EBX,NONE)),end=0xc560f)
        for slot,n in ((0,1),(15,2)):
            self.call(0x14e240,(self.datums[slot],),((UC_X86_REG_EBX,UNIT+n),))
        self.slot=3
        self.call(0x14e240,(self.datums[3],),((UC_X86_REG_EBX,UNIT),))
        self.f(OBJECTS+0xe4,c.f(info+0x28));self.f(OBJECTS+0xe8,c.f(info+0x8c))
        self.f(OBJECTS+0xec,body);self.f(OBJECTS+0xf0,shield)
        self.g.mem_write(EVENT,bytes(0x88));self.g.mem_write(ACCUM,bytes(0x34));self.g.mem_write(RESULT,bytes(0x58))
        self.w(EVENT,self.damage_tag);self.w(EVENT+4,event_flags)
        attacker=NONE if incoming is None else self.datums[incoming]
        self.w(EVENT+8,attacker);self.w(EVENT+0xc,NONE)
        self.sh(EVENT+0x10,-1 if incoming is None else incoming)
        for off in (0x14,0x18,0x1c):self.w(EVENT+off,NONE)
        for off in (0x20,0x22,0x7c,0x7e):self.sh(EVENT+off,-1)
        self.f(EVENT+0x54,1);self.f(EVENT+0x58,1);self.f(EVENT+0x5c,1)
        self.g.mem_write(EVENT+0x84,bytes([c.data[projectile['offset']+0x128]]))
        self.new_event_hex=bytes(self.g.mem_read(EVENT,0x88)).hex()
        self.w(ACCUM+4,self.info)
        for off in (8,0xc,0x10):self.w(ACCUM+off,NONE)
        self.w(ACCUM+0x30,0x200)
        self.damage_events=[];self.source_services={};self.stop_at=None
        self.source_services={
            0x1781f0:('effect emission outside damage/history state',16,None),
            0xbb6e0:('object variant/effect state update on shield depletion',16,None),
            0xb8770:('world impulse application outside damage/history state',20,None),
            0xcac00:('damage reaction animation service; no animation or marker pose supplied',12,None),
            0x115fa0:('damage feedback animation service; no animation or marker pose supplied',16,None),
        }
        self.asset=dict(biped=bipd,model=model,damage=dmg,projectile=projectile,
            reporting_type=c.data[projectile['offset']+0x128],damage_info_file_offset=f'{info:08x}',
            raw_damage_info_hex=bytes(c.data[info:info+0xf8]).hex(),
            raw_damage_tag_hex=bytes(c.data[dmg['offset']:dmg['offset']+dmg['size']]).hex())
        self.watch.update({OBJECTS+off:f'unit0.{name}' for off,name in
            ((0xec,'body'),(0xf0,'shield'),(0xfc,'damage_body_accum'),(0x100,'damage_shield_accum'))})

    def hook(self,g,a,size,user):
        if a==self.stop_at:
            self.damage_events.append(dict(kind='stop_before_source_boundary',entry=f'{a:08x}'))
            g.emu_stop();return
        sp=g.reg_read(UC_X86_REG_ESP)
        if a==0xdaa90:self.result_pointer=g.reg_read(UC_X86_REG_EAX)
        if a in (0xc52c0,0xd9180,0xdb160,0xda710,0xda2b0,0xdaa90,0xcb1d0,0xdac40,0x1ed960,0x15e4d0,
                 0xd7e00,0xb9a60,0xcb410,0xcba60,0xcac00,0x115fa0,0xc9a40,0xcbdc0,0x14e240):
            row=dict(kind='original_entry',entry=f'{a:08x}',
                history=self.history_state(),current=f'{self.u(PLAYERS+3*0x21c+0x2c):08x}',
                dead=bool(self.g.mem_read(OBJECTS+0x10a,1)[0]&4))
            if a in (0xcb410,0xdac40):row['result_hex']=bytes(g.mem_read(self.result_pointer,0x58)).hex()
            if a==0xda710:
                row.update(accumulator_hex=bytes(g.mem_read(self.u(sp+16),0x34)).hex(),
                    raw_amount_bits=f'{self.u(self.u(sp+16)+0x1c):08x}')
            if a==0xdaa90:
                row.update(accumulator_hex=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),0x34)).hex())
            if a==0xcba60:row.update(amount_xmm0=f'{g.reg_read(UC_X86_REG_XMM0)&NONE:08x}',
                arguments=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],unit_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}')
            self.damage_events.append(row)
        if a in self.source_services:
            name,pop,result=self.source_services[a]
            self.damage_events.append(dict(kind='supplied_service',entry=f'{a:08x}',name=name,
                stack_arguments=[f'{self.u(sp+4+i):08x}'for i in range(0,pop,4)]))
            if result is not None:g.reg_write(UC_X86_REG_EAX,result)
            g.reg_write(UC_X86_REG_EIP,self.u(sp));g.reg_write(UC_X86_REG_ESP,sp+4+pop);return
        super().hook(g,a,size,user)

    def health(self):
        return dict(body_bits=f'{self.u(OBJECTS+0xec):08x}',shield_bits=f'{self.u(OBJECTS+0xf0):08x}',
            flags=f'{self.h(OBJECTS+0x10a)&0xffff:04x}',body_stun=self.h(OBJECTS+0x106),shield_stun=self.h(OBJECTS+0x104),
            accumulator_hex=bytes(self.g.mem_read(ACCUM,0x34)).hex(),event_hex=bytes(self.g.mem_read(EVENT,0x88)).hex())

    def kernels(self):
        self.call(0xdb160,(EXTRA+0x203000,),((UC_X86_REG_EAX,EVENT),(UC_X86_REG_EBX,UNIT),
            (UC_X86_REG_ECX,self.damage_definition+0x10)))
        amount=self.g.reg_read(UC_X86_REG_XMM0)&NONE
        self.w(ACCUM+0x18,amount);self.w(ACCUM+0x1c,amount)
        self.call(0xda710,(UNIT,self.damage_definition+0x10,EVENT,ACCUM))
        shield=self.health()
        self.call(0xda2b0,(UNIT,self.damage_definition+0x10,EVENT),((UC_X86_REG_EDI,ACCUM),))
        return shield,self.health()

    def produce_result(self):
        self.call(0xdaa90,(UNIT,),((UC_X86_REG_EAX,RESULT),(UC_X86_REG_ESI,EVENT),(UC_X86_REG_EDI,ACCUM)))
        return bytes(self.g.mem_read(RESULT,0x58)).hex()

    def consume(self):
        self.skip_writer=True
        self.stop_at=0xcbe95 if self.u(RESULT+4)&1 else None
        self.call(0xdac40,(UNIT,),((UC_X86_REG_EDI,RESULT),),
            end=0xcbe95 if self.u(RESULT+4)&1 else 0x3000000)
        self.stop_at=None
        return dict(health=self.health(),history=self.history_state(),stats=self.stats(),ownership=self.state(),
            events=self.damage_events,accounting_events=self.accounting_events)

    def current(self):
        health=self.health()
        # D9180 owns a stack accumulator; the standalone kernel buffer is not
        # its accumulator. Actual stack bytes are captured at DAA90 below.
        health.pop('accumulator_hex')
        return dict(health=health,history=self.history_state(),stats=self.stats(),ownership=self.state(),
            object_designators=[self.h(OBJECTS+n*0x800+0x138)for n in range(3)],
            attacker_unit_counters=[dict(unit=f'{UNIT+n:08x}',counter_2ea=self.h(OBJECTS+n*0x800+0x2ea),
                last_tick=f'{self.u(OBJECTS+n*0x800+0x2ec):08x}')for n in (1,2)],
            rng=[f'{self.u(SEED+n):08x}'for n in (0,4)])

    def source_hit(self,incoming=0,tick=200,stale=False,designator=None,dead=False):
        self.g.mem_write(EVENT,bytes.fromhex(self.new_event_hex))
        a=NONE if incoming is None else self.datums[incoming]
        if stale:a^=0x10000
        self.w(EVENT+8,a)
        self.sh(EVENT+0x10,(-1 if incoming is None else incoming) if designator is None else designator)
        self.w(TIME+8,tick)
        self.damage_events=[];self.accounting_events=[];self.events=[];self.writes=[]
        input_hex=bytes(self.g.mem_read(EVENT,0x88)).hex()
        before=self.current();self.skip_writer=True
        self.stop_at=0xcbe95 if dead else None
        self.call(0xd9180,(EVENT,UNIT,NONE,NONE,NONE,0),end=0xcbe95 if dead else 0x3000000)
        self.stop_at=None
        after=self.current()
        generated=next(e for e in self.damage_events if e['entry']=='000dac40')
        raw=bytes.fromhex(generated['result_hex'])
        expected_amount=bits(sum(struct.unpack_from('<2f',raw,0x44)))
        history_call=next((e for e in self.damage_events if e['entry']=='000cba60'),None)
        if incoming is not None:
            assert history_call is not None and history_call['amount_xmm0']==f'{expected_amount:08x}'
        assert self.u(PLAYERS+3*0x21c+0x2c)==(NONE if dead else UNIT)
        writer=next((e for e in self.accounting_events if e['kind']=='supplied_history_and_death_event_writer_boundary'),None)
        return dict(incoming_full=f'{a:08x}',incoming_designator=self.h(EVENT+0x10),
            tick=tick,input_event_hex=input_hex,
            generated_result_hex=generated['result_hex'],result_body_bits=f'{struct.unpack_from("<I",raw,0x44)[0]:08x}',
            result_shield_bits=f'{struct.unpack_from("<I",raw,0x48)[0]:08x}',result_flags=f'{struct.unpack_from("<I",raw,4)[0]:08x}',
            reporting_type=raw[0],before=before,after=after,source_writer_request=writer,
            original_and_supplied_entries=list(self.damage_events),accounting_events=list(self.accounting_events),
            state_writes=list(self.writes),complete_consumer_return=not dead,
            stop_exclusive='000cbe95' if dead else '03000000')


def initialization_cases(o,c):
    rows=[]
    for n,slot in ((0,3),(1,0),(2,15)):
        o.fixture(c);p=OBJECTS+n*0x800
        o.g.mem_write(p+0x2f0,b'\x77'*0x44);o.sh(p+0x2ea,123);o.w(p+0x2ec,456)
        before=bytes(o.g.mem_read(p+0x2ea,0x4a)).hex()
        o.call(0xc52c0,(UNIT+n,),end=0xc5464)
        after=bytes(o.g.mem_read(p+0x2ea,0x4a)).hex()
        for i in range(4):
            assert o.u(p+0x2f0+i*16)==NONE and o.u(p+0x2f4+i*16)==0
            assert o.u(p+0x2f8+i*16)==o.u(p+0x2fc+i*16)==NONE
            assert o.g.mem_read(p+0x330+i,1)==b'\0'
        assert o.h(p+0x2ea)==0 and o.u(p+0x2ec)==NONE and o.u(p+0x13c)==NONE
        # The constructor prefix clears object ownership, not player +2C.
        rows.append(dict(unit=f'{UNIT+n:08x}',owning_fixture_player=f'{o.datums[slot]:08x}',
            source_prefix='000c52c0',stop_exclusive='000c5464',before_hex=before,after_hex=after,
            current_object_owner=f'{o.u(p+0x13c):08x}',player_current=f'{o.u(PLAYERS+slot*0x21c+0x2c):08x}',
            boundary='Constructor prefix ends after four source history entries; this poisoning case does not execute later constructor callbacks or rebind the existing fixture.'))
    return rows


def amount_cases(o,c):
    rows=[]
    for strength in (0,1,2,3,4,255):
        for scale in (0.,.5,1.):
            o.fixture(c)
            o.g.mem_write(PLAYERS+0xc1,bytes([strength]));o.f(EVENT+0x54,scale)
            before=[o.u(SEED+i)for i in (0,4)]
            o.call(0xdb160,(EXTRA+0x203000,),((UC_X86_REG_EAX,EVENT),(UC_X86_REG_EBX,UNIT),
                (UC_X86_REG_ECX,o.damage_definition+0x10)))
            amount=o.g.reg_read(UC_X86_REG_XMM0)&NONE
            expected=4.75*scale*{1:.75,2:.5,3:.25}.get(strength,1.)
            assert amount==bits(expected) and before==[o.u(SEED+i)for i in (0,4)]
            rows.append(dict(strength_byte=strength,event_scale54_bits=f'{bits(scale):08x}',
                event_scale5c_bits='3f800000',amount_bits=f'{amount:08x}',rng_before=[f'{x:08x}'for x in before],
                rng_after=[f'{o.u(SEED+i):08x}'for i in (0,4)],event_after_hex=bytes(o.g.mem_read(EVENT,0x88)).hex(),
                source_services=list(o.source_services)))
    return rows


def hit_cases(o,c):
    rows=[]
    for shield,body in ((1,1),(.05,1),(0,1),(0,.01),(0,.10555555671453476)):
        for name,incoming,stale,designator in (
            ('valid_enemy',0,False,None),('uncredited',None,False,None),
            ('stale_enemy',0,True,None),('self',3,False,None)):
            o.fixture(c,shield,body,incoming)
            dead=shield==0 and body<=.10555555671453476
            hit=o.source_hit(incoming,stale=stale,designator=designator,dead=dead)
            hit.update(case=name,initial_shield_bits=f'{bits(shield):08x}',initial_body_bits=f'{bits(body):08x}')
            result=bytes.fromhex(hit['generated_result_hex'])
            assert struct.unpack_from('<I',result,0xc)[0]==int(hit['incoming_full'],16)
            assert struct.unpack_from('<h',result,0x14)[0]==hit['incoming_designator']
            assert result[0]==8
            if dead:
                credit=hit['source_writer_request']['attacker']
                assert credit==('ffffffff'if name in ('uncredited','stale_enemy')else hit['incoming_full'])
                assert hit['after']['stats']['3']['fields'][3]==1
                assert hit['after']['ownership']['current']=='ffffffff'
            rows.append(hit)
    return rows


def sequence_cases(o,c):
    rows=[]
    for name,pattern in (
        ('one_attacker_25_hits',[0]*25),
        ('20_first_then_5_second',[0]*20+[15]*5),
        ('24_enemy_then_uncredited',[0]*24+[None]),
        ('24_enemy_then_self',[0]*24+[3]),
        ('24_enemy_then_self_designator_0',[0]*24+[3]),
        ('24_enemy_then_stale_new_salt',[0]*24+[0])):
        o.fixture(c);hits=[]
        for i,incoming in enumerate(pattern):
            stale=name=='24_enemy_then_stale_new_salt'and i==24
            designator=0 if name=='24_enemy_then_self_designator_0'and i==24 else None
            hit=o.source_hit(incoming,200+i*2,stale=stale,designator=designator,dead=i==24)
            hits.append(hit)
        final=hits[-1];writer=final['source_writer_request']
        assert writer is not None and final['after']['stats']['3']['fields'][3]==1
        assert final['after']['ownership']['current']=='ffffffff'
        assert len([h for h in hits if int(h['result_flags'],16)&1])==1
        # Source history at death attribution entry excludes this lethal hit.
        attr=next(e for e in final['original_and_supplied_entries']if e['entry']=='001ed960')
        assert attr['history']==final['before']['history']
        # Score is a separately explicit source composition, with cause-class
        # NONE, rather than invented continuation through marker/event services.
        o.skip_writer=False;o.accounting_events=[]
        o.call(0x2c2350,(int(writer['attacker'],16),int(writer['victim'],16),writer['relationship_flag'],NONE))
        rows.append(dict(case=name,hits=hits,writer_request=writer,after_separate_slayer_callback=o.engine_state(),
            separate_slayer_events=list(o.accounting_events),
            scope='Fresh per-hit events, explicit hit ticks, no source world update/recharge/projectile/weapon scheduling between hits; not a complete magazine replay.'))
    return rows


def stage_cases(o,c):
    rows=[]
    for name,lethal,stop in (
        ('nondeath_before_reaction',False,0xcac00),
        ('nondeath_inside_reaction_before_animation_lookup',False,0x111700),
        ('nondeath_history_complete_before_feedback',False,0x115fa0),
        ('death_before_attribution',True,0x1ed960),
        ('death_at_marker_event_writer_entry',True,0x15e4d0),
        ('death_before_aftermath',True,0xcb410),
        ('death_before_reaction',True,0xcac00),
        ('death_history_complete_before_feedback',True,0x115fa0),
        ('death_before_detach',True,0xcbdc0),
        ('death_after_detach',True,0xcbe95)):
        o.fixture(c,0 if lethal else 1,.01 if lethal else 1)
        if stop==0x111700:o.source_services.pop(0xcac00)
        before=o.current();input_hex=bytes(o.g.mem_read(EVENT,0x88)).hex();o.skip_writer=True
        o.stop_at=stop
        o.call(0xd9180,(EVENT,UNIT,NONE,NONE,NONE,0),end=stop)
        o.stop_at=None
        after=o.current()
        populated=after['history'][0]['timestamp']!=NONE
        assert populated==name.endswith(('before_feedback','before_detach','after_detach'))
        assert after['ownership']['current']==('ffffffff'if stop==0xcbe95 else '00010000')
        rows.append(dict(case=name,lethal=lethal,input_event_hex=input_hex,
            source_entry='000d9180',stop_exclusive=f'{stop:08x}',before=before,after=after,
            original_and_supplied_entries=list(o.damage_events),accounting_events=list(o.accounting_events)))
    return rows


def zero_and_identity_cases(o,c):
    rows=[]
    for incoming,object_datum in ((0,NONE),(None,NONE),(0,UNIT+1),(None,UNIT+1)):
        o.fixture(c,incoming=incoming)
        o.f(EVENT+0x54,0);o.w(EVENT+0xc,object_datum)
        input_hex=bytes(o.g.mem_read(EVENT,0x88)).hex();before=o.current()
        o.call(0xd9180,(EVENT,UNIT,NONE,NONE,NONE,0))
        after=o.current();result=next(e for e in o.damage_events if e['entry']=='000dac40')['result_hex']
        assert after['health']['body_bits']==after['health']['shield_bits']=='3f800000'
        assert after['history'][0]['timestamp']==(NONE if incoming is None and object_datum==NONE else 200)
        rows.append(dict(case='zero_source_amount',incoming=incoming,source_object=f'{object_datum:08x}',
            input_event_hex=input_hex,before=before,after=after,result_hex=result,
            original_and_supplied_entries=list(o.damage_events),complete_consumer_return=True))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'smg-damage-report.json');args=p.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert hashlib.sha256(c.data).hexdigest()==CACHE_PIN
    o=DamageOracle(c);rows=[]
    for shield,body in ((1,1),(.01,1),(0,1),(0,.01)):
        o.fixture(c,shield,body);before=o.health();after_shield,after_body=o.kernels();result=o.produce_result()
        consumed=o.consume()
        rows.append(dict(shield=shield,body=body,before=before,after_shield=after_shield,after_body=after_body,result_hex=result,consumed=consumed))
    initialized=initialization_cases(o,c);amounts=amount_cases(o,c);hits=hit_cases(o,c)
    sequences=sequence_cases(o,c);stages=stage_cases(o,c);zero=zero_and_identity_cases(o,c)
    md=Cs(CS_ARCH_X86,CS_MODE_32);listing=[]
    spans=((0xc52c0,0xc5640),(0xd9180,0xd96c0),(0xdb160,0xdb360),
        (0xd71c0,0xd7290),(0xda710,0xdaa90),(0xda2b0,0xda620),
        (0xdaa90,0xdac40),(0xdac40,0xdb160),(0xd7e00,0xd8070),
        (0xb9a60,0xb9ab0),(0xcb1d0,0xcb410),(0xcb410,0xcba60),
        (0xcba60,0xcbdc0),(0xcbdc0,0xcbe95),(0x14e240,0x14e3e0),
        (0x15e250,0x15e4d0),(0x15f520,0x15f6a0),(0x1ed960,0x1edf70),
        (0x1e2f50,0x1e2fc0),(0x253f10,0x253f30))
    for start,end in spans:
        listing.append(f'RANGE {start:08x}..{end:08x}')
        listing.extend(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
            for i in md.disasm(o.x.read(start,end-start),start))
    path=OUT/'tu11-smg-damage-instructions.txt';path.write_text('\n'.join(listing)+'\n')
    frozen_names=('oracle_mp_spawn.py','oracle_mp_spawn_lifecycle.py','oracle_mp_spawn_ownership.py',
        'oracle_mp_spawn_timing.py','oracle_mp_spawn_queue.py','oracle_mp_spawn_rounding.py',
        'oracle_mp_spawn_placement.py','oracle_mp_spawn_aftermath.py','oracle_mp_spawn_slayer_accounting.py')
    frozen_reports=('report.json','lifecycle-report.json','ownership-report.json','timing-report.json',
        'queue-report.json','rounding-report.json','placement-report.json','aftermath-report.json','slayer-accounting-report.json')
    report=dict(scope=__doc__,tool_sha256=sha(__file__),executable_sha256=VERSIONS['tu11']['pin'],
        cache_sha256=CACHE_PIN,dependencies={str((ROOT/'tools'/n).relative_to(ROOT)):sha(ROOT/'tools'/n)for n in frozen_names},
        frozen_prior_reports_sha256={str((OUT/n).relative_to(ROOT)):sha(OUT/n)for n in frozen_reports},
        rounding='x87 nearest even, 53-bit precision FPCW 027F; SSE MXCSR 1F80; source instructions own all float operation order',
        instruction_listing=str(path.relative_to(ROOT)),instruction_listing_sha256=sha(path),
        standalone_kernel_composition=rows,source_history_initialization=initialized,source_amount=amounts,
        complete_damage_entry_hits=hits,damage_entry_sequences=sequences,scheduling_prefixes=stages,
        zero_amount_identity_cases=zero,assets=o.asset,
        pool_fixture=dict(player_capacity=16,player_stride='21c',full_players={str(k):f'{v:08x}'for k,v in o.datums.items()},
            unit_full_datums=['00010000','00010001','00010002'],unit_owners=['00160003','00070000','0052000f'],
            unit_memberships=[3,0,15],header_salt=1,header_type=0,header_flags=1,allocation_size='800',
            no_active_world_registration=True,player_local_user=-1,AI_actor='ffffffff',replicated_entity='ffffffff',
            biped_ragdoll_state=3,profile_recording_disabled=True,source_tail_configuration_byte0=True),
        supplied_boundaries=[
            'World/projectile hit, node/material region NONE and event scale54/5C=1 are declared inputs; no original projectile/world hit finder executes.',
            '1781F0 effect emission and BB6E0 model/variant state update are supplied void services when invoked.',
            'CAC00 reaction animation and 115FA0 damage feedback are supplied void services in completed damage/result paths. One reaction prefix executes to 111700 before animation lookup.',
            '15E4D0 death history/marker/countdown/event writer is supplied as a recorded boundary; exact original incoming attribution/counter code executes. No head pose is supplied.',
            'Score callbacks are separately executed only after original attribution; no original event/marker writer continuation is fabricated.',
            'Lethal damage stops at CBE95 after original ownership detach; subsequent corpse weapon/action/seat cleanup does not execute.',
            'Each sequence supplies independent hit ticks and fresh events, with no world update, recharge, weapon magazine/reload scheduling or unrelated RNG consumer between hits.'],
        summary=dict(standalone_kernel_compositions=len(rows),initialization_prefixes=len(initialized),
            original_amount_cases=len(amounts),damage_entry_hit_cases=len(hits),damage_entry_sequences=len(sequences),
            sequenced_hits=sum(len(s['hits'])for s in sequences),scheduling_prefixes=len(stages),
            zero_amount_identity_cases=len(zero),source_kernel_or_entry_cases=len(rows)+len(initialized)+len(amounts)+len(hits)+len(stages)+len(zero)+sum(len(s['hits'])for s in sequences)))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['summary']))


if __name__=='__main__':main()
