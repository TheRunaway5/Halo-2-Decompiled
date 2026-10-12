#!/usr/bin/env python3
"""Unchanged TU1.1 datum pools and bounded player/object ownership lifecycle.

Pool names, sizes, allocators, initialization, binding and leave dispatch execute
original bytes. Body storage is a supplied arena service. World registration,
placement and full object deletion remain explicit boundaries.
"""
import argparse
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_EBP, UC_X86_REG_ESP,
    UC_X86_REG_EIP)
from oracle_mp_spawn import ROOT, VERSIONS, CACHE_PIN, Cache, PLAYER_POOL, OBJECT_POOL, ENGINE_GLOBALS, SEED
from oracle_mp_spawn_lifecycle import EXTRA, TABLE, TIME
from oracle_mp_spawn_ownership import GLOBALS, GARBAGE
from oracle_mp_spawn_smg_damage import DamageOracle, NONE, sha
from oracle_mp_spawn_smg_producers import ProducerOracle, CREATION

OUT=ROOT/'analysis/multiplayer/integration/spawn-source'
HEAP=EXTRA+0x220000
ARENA=EXTRA+0x230000
DESCRIPTOR=EXTRA+0x221000
OWNERS=EXTRA+0x222000
ALLOCATOR=EXTRA+0x223000
ALLOCATE_SERVICE=EXTRA+0x3f1000

class DatumOracle(ProducerOracle):
    def __init__(self,c):
        self.datum_events=[]; self.storage_success=True; self.storage_cursor=ARENA
        super().__init__(c)

    def hook(self,g,a,size,user):
        sp=g.reg_read(UC_X86_REG_ESP)
        if a==ALLOCATE_SERVICE:
            n=self.u(sp+4);pool=self.allocation_destination
            self.datum_events.append(dict(kind='supplied_pool_storage',entry=f'{a:08x}',
                bytes_requested=n,allocator=f'{g.reg_read(UC_X86_REG_ECX):08x}',result=f'{pool:08x}'))
            self.g.mem_write(pool,b'\xa5'*n)
            g.reg_write(UC_X86_REG_EAX,pool);g.reg_write(UC_X86_REG_EIP,self.u(sp))
            g.reg_write(UC_X86_REG_ESP,sp+8);return
        if a==0x18fce0:
            dst,n,flag=(self.u(sp+i)for i in (4,8,12))
            at=self.storage_cursor+0x10
            if self.storage_success:
                # Only lower body storage/address admission is supplied. Header
                # allocation, datum generation and body clearing are original.
                self.g.mem_write(self.storage_cursor,b'\xa5'*(n+0x10))
                self.w(at-0x10,n+0x10);self.w(at-8,0);self.w(at-4,0)
                self.w(dst,at);self.storage_cursor+=(n+0x10+0xf)&~0xf
            self.datum_events.append(dict(kind='supplied_storage',entry=f'{a:08x}',
                destination=f'{dst:08x}',requested_length=n,flag=flag,
                success=self.storage_success,body=f'{at:08x}'if self.storage_success else None))
            g.reg_write(UC_X86_REG_EAX,int(self.storage_success))
            g.reg_write(UC_X86_REG_EIP,self.u(sp));g.reg_write(UC_X86_REG_ESP,sp+16);return
        if a in (0x16d070,0x16d0d0,0x16d210,0x16d220,0x16d2c0,0x16d410,0x16d480,
                 0x16d4c0,0x16d560,0x14d350,0x14d800,0x14d600,0x159280,0x14e240,
                 0x14dcb0,0xbc7e0,0xbbda0,0x10d0f0,0xd0300,0xbd170,0xbd1f0,
                 0xc52c0,0xbd080,0xb8820,0xd7e00,0xcbdc0,0x154440):
            self.datum_events.append(dict(kind='original_entry',entry=f'{a:08x}',
                eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',ebx=f'{g.reg_read(UC_X86_REG_EBX):08x}',
                edx=f'{g.reg_read(UC_X86_REG_EDX):08x}',esi=f'{g.reg_read(UC_X86_REG_ESI):08x}',
                edi=f'{g.reg_read(UC_X86_REG_EDI):08x}',
                stack_arguments=[f'{self.u(sp+4+i*4):08x}'for i in range(3)]))
        super().hook(g,a,size,user)

    def fixture(self,c):
        super().fixture(c)
        self.datum_events=[];self.storage_success=True;self.storage_cursor=ARENA
        self.w(0x4e58bc,HEAP);self.g.mem_write(HEAP,bytes(0x40))
        self.w(0x518490,0);self.w(TIME+8,0)
        self.w(GLOBALS+0x98,NONE);self.w(GLOBALS+0x2c,0);self.w(GLOBALS,0)
        self.g.mem_write(0x4d6d38,b'\x00')
        matg=self.u(self.a['globals']);self.w(self.a['globals'],0)
        self.call(0x1a0a90,(self.options+0x13c,0));self.w(self.a['globals'],matg)
        self.initialize_callers()
        self.published=None
        self.datum_events=[]

    def initialize_callers(self):
        self.w(ALLOCATOR,ALLOCATOR+4);self.w(ALLOCATOR+4,ALLOCATE_SERVICE)
        self.w(0x51841c,ALLOCATOR)
        self.allocation_destination=PLAYER_POOL;self.stop_at=0x14cc49
        self.call(0x14cc00,end=0x14cc49);self.stop_at=None
        assert self.u(0x4f01f4)==PLAYER_POOL
        self.allocation_destination=OBJECT_POOL;self.stop_at=0xb76e0
        self.call(0xb76c2,end=0xb76e0);self.stop_at=None
        assert self.u(0x4e78d0)==OBJECT_POOL
        self.w(0x4fcda4,EXTRA+0x224000);self.g.mem_write(0x4f496c,b'\0')
        self.call(0x14ccd0)
        self.call(0x16d210,registers=((UC_X86_REG_EAX,OBJECT_POOL),))

    def construct(self,pool,name,n,stride):
        self.g.mem_write(pool,b'\xa5'*(0x4c+n*stride+((n+31)//32)*4))
        self.call(0x16d070,(pool,name,0),((UC_X86_REG_ECX,0),(UC_X86_REG_EDX,stride),(UC_X86_REG_EBX,n)))
        self.call(0x16d210,registers=((UC_X86_REG_EAX,pool),))
        assert self.u(pool+0x44)==pool+0x4c and self.u(pool+0x48)==pool+0x4c+n*stride

    def pool_state(self,pool):
        n,stride,data,bitmap=(self.u(pool+x)for x in (0x20,0x24,0x44,0x48))
        return dict(name=bytes(self.g.mem_read(pool,32)).split(b'\0')[0].decode(),
            capacity=n,stride=stride,valid=self.g.mem_read(pool+0x29,1)[0],
            flags=f'{self.h(pool+0x2a)&0xffff:04x}',first_free=self.u(pool+0x34),
            high_water=self.u(pool+0x38),count=self.u(pool+0x3c),
            next_salt=f'{self.h(pool+0x40)&0xffff:04x}',
            bitmap_hex=bytes(self.g.mem_read(bitmap,((n+31)//32)*4)).hex(),
            occupied=[dict(index=i,salt=f'{self.h(data+i*stride)&0xffff:04x}',
                record_hex=bytes(self.g.mem_read(data+i*stride,stride)).hex())
                for i in range(n)if self.h(data+i*stride)],
            header_hex=bytes(self.g.mem_read(pool,0x4c)).hex())

    def allocate(self,pool,index=None):
        if index is None:return self.call(0x16d2c0,registers=((UC_X86_REG_EDX,pool),))
        return self.call(0x16d410,registers=((UC_X86_REG_EDX,pool),(UC_X86_REG_EDI,index&NONE)))
    def release(self,pool,d):self.call(0x16d4c0,registers=((UC_X86_REG_EDX,pool),(UC_X86_REG_EAX,d)))
    def lookup(self,pool,d):return self.call(0x16d560,registers=((UC_X86_REG_ESI,pool),(UC_X86_REG_EAX,d)))
    def player_ptr(self,d):return self.u(PLAYER_POOL+0x44)+(d&0xffff)*0x21c
    def header(self,d):return self.u(OBJECT_POOL+0x44)+(d&0xffff)*12
    def body(self,d):return self.u(self.header(d)+8)

    def join(self,index):
        if self.published is None:
            self.reset_controllers();self.published=self.publish(0)
        self.g.mem_write(CREATION,bytes(0xac));self.sh(CREATION+2,-1);self.w(CREATION+4,NONE)
        self.g.mem_write(CREATION+0x1c,self.published)
        return self.call(0x14d350,registers=((UC_X86_REG_EDI,index),(UC_X86_REG_EBX,CREATION)))

    def object(self,typ=0):
        d=self.call(0xbd170,(0x800,))
        if d==NONE:return d
        h,p=self.header(d),self.body(d)
        assert bytes(self.g.mem_read(p,0x800))==bytes(0x800)
        self.g.mem_write(h+2,bytes([1,typ]))
        tag=next(t for t in self.cache.tags.values()if t['group']==('bipd'if typ==0 else 'proj')and
            t['name'].endswith('masterchief_mp'if typ==0 else 'projectiles\\smg_bullet'))
        self.w(p,int(tag['datum'],16));self.g.mem_write(p+0xaa,bytes([typ]))
        for off in (0xc,0x10,0x14,0x12c,0x130,0x13c,0x140,0x248,0x24c,0xd4,0x1bc,0x1fc):self.w(p+off,NONE)
        for off in range(0x218,0x228,4):self.w(p+off,NONE)
        self.g.mem_write(p+0x212,b'\xff');self.g.mem_write(p+0x3dc,b'\x03')
        if typ==0:self.call(0xc52c0,(d,),end=0xc5464)
        return d

    def bind(self,player,unit):self.call(0x14e240,(player,),((UC_X86_REG_EBX,unit),))
    def identities(self,player,units,projectile=None):
        p=self.player_ptr(player)
        r=dict(player=f'{player:08x}',player_lookup=f'{self.lookup(PLAYER_POOL,player):08x}',
            player_record_salt=f'{self.h(p)&0xffff:04x}',current=f'{self.u(p+0x2c):08x}',
            previous=f'{self.u(p+0x30):08x}',flags=f'{self.h(p+2)&0xffff:04x}',
            spawn_count=self.h(p+0x218),rng=[f'{self.u(SEED+n):08x}'for n in (0,4)],player_pool=self.pool_state(PLAYER_POOL),
            object_pool=self.pool_state(OBJECT_POOL),units=[])
        for d in units:
            h=self.header(d);body=self.u(h+8)
            r['units'].append(dict(datum=f'{d:08x}',header_lookup=f'{self.lookup(OBJECT_POOL,d):08x}',
                pointer=f'{body:08x}',current_player=f'{self.u(body+0x13c):08x}'if body else None,
                previous_player=f'{self.u(body+0x140):08x}'if body else None,
                dead=bool(self.g.mem_read(body+0x10a,1)[0]&4)if body else None,
                player_removed_field=f'{self.u(body+0x2a8):08x}'if body else None))
        if projectile is not None:
            self.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,OWNERS)))
            r['captured_projectile']=dict(datum=f'{projectile:08x}',c4=f'{self.u(self.body(projectile)+0xc4):08x}',
                c8=f'{self.u(self.body(projectile)+0xc8):08x}',designator=self.h(self.body(projectile)+0xc2),
                source_BD080_hex=bytes(self.g.mem_read(OWNERS,12)).hex())
        return r


def allocator_cases(o,c):
    rows=[]
    o.fixture(c);o.initialize_callers()
    p=o.pool_state(PLAYER_POOL);obj=o.pool_state(OBJECT_POOL)
    assert p['capacity']==16 and p['stride']==0x21c and p['next_salt']=='ec70'
    assert obj['capacity']==0x800 and obj['stride']==12 and obj['next_salt']=='e26f'
    rows.append(dict(case='actual_initializer_callers',players=p,objects=obj,entries=o.datum_events,
        boundaries=['Virtual arena allocation returns mapped storage; allocator implementation is supplied.',
            'Player caller executes 14CC00 through 14CC49 exclusive; subsequent arena allocation for player globals is outside this case.',
            'Object caller executes B76C2 through B76E0 exclusive; preceding other-object initialization and following dynamic body heap setup are outside this case.',
            'Complete original player new-map reset 14CCD0 executes with mapped player globals and AI globals.']))
    for pool in (PLAYER_POOL,OBJECT_POOL):
        o.fixture(c);initial=o.pool_state(pool);d0=o.allocate(pool);d1=o.allocate(pool);d2=o.allocate(pool)
        after_three=o.pool_state(pool);o.release(pool,d1);hole=o.pool_state(pool);reuse=o.allocate(pool)
        assert reuse&0xffff==1 and reuse>>16==(d2>>16)+1
        assert o.lookup(pool,d1)==0 and o.lookup(pool,reuse)!=0
        o.release(pool,d2);trailing=o.pool_state(pool);o.release(pool,reuse);o.release(pool,d0)
        final=o.pool_state(pool);assert final['high_water']==final['count']==final['first_free']==0
        rows.append(dict(case='automatic_hole_and_trailing_release',pool=initial['name'],initial=initial,
            allocated=[f'{d:08x}'for d in (d0,d1,d2)],after_three=after_three,hole=hole,
            reused=f'{reuse:08x}',after_trailing_release=trailing,final=final,entries=o.datum_events))
        o.fixture(c);o.sh(pool+0x40,-3);ds=[o.allocate(pool)for _ in range(3)]
        assert [d>>16 for d in ds]==[0xfffd,0xfffe,0x8000]
        rows.append(dict(case='salt_wrap',pool=o.pool_state(pool)['name'],supplied_counter='fffd',
            allocated=[f'{d:08x}'for d in ds],after=o.pool_state(pool),entries=o.datum_events))
        o.fixture(c);d=o.allocate(pool,5);first=o.pool_state(pool)
        for bad in (-1,5,o.u(pool+0x20)):
            before=o.pool_state(pool);r=o.allocate(pool,bad);assert r==NONE and before==o.pool_state(pool)
            rows.append(dict(case='explicit_index_reject',index=bad,result=f'{r:08x}',before=before,after=o.pool_state(pool)))
        automatic=o.allocate(pool);assert automatic&0xffff==0 and first['first_free']==0
        rows.append(dict(case='explicit_index_preserves_cursor',explicit=f'{d:08x}',after_explicit=first,
            automatic=f'{automatic:08x}',after=o.pool_state(pool),entries=o.datum_events))
        o.fixture(c);o.g.mem_write(pool+0x2a,b'\x08');o.call(0x16d220,registers=((UC_X86_REG_ESI,pool),))
        data=o.u(pool+0x44);stride=o.u(pool+0x24)
        assert bytes(o.g.mem_read(data+2,stride-2))==b'\xba'*(stride-2)
        d=o.allocate(pool);assert bytes(o.g.mem_read(data+2,stride-2))==bytes(stride-2)
        o.release(pool,d);released=bytes(o.g.mem_read(data,stride));assert released[:2]==b'\0\0'and released[2:]==b'\xba'*(stride-2)
        o.call(0x16d220,registers=((UC_X86_REG_ESI,pool),));reset=o.allocate(pool)
        assert reset==d
        rows.append(dict(case='poison_fill_and_map_reset',allocated=f'{d:08x}',released_hex=released.hex(),
            reset_first=f'{reset:08x}',after=o.pool_state(pool),entries=o.datum_events))
    o.fixture(c);o.construct(PLAYER_POOL,0x45aaf4,3,0x21c)
    ds=[o.allocate(PLAYER_POOL)for _ in range(3)];before=o.pool_state(PLAYER_POOL)
    r=o.allocate(PLAYER_POOL);assert r==NONE and before==o.pool_state(PLAYER_POOL)
    rows.append(dict(case='bounded_capacity_exhaustion',supplied_capacity=3,allocated=[f'{d:08x}'for d in ds],
        result=f'{r:08x}',before=before,after=o.pool_state(PLAYER_POOL),entries=o.datum_events))
    return rows


def storage_cases(o,c):
    rows=[]
    for success in (True,False):
        o.fixture(c);o.storage_success=success;before=o.pool_state(OBJECT_POOL)
        d=o.call(0xbd170,(0x800,));after=o.pool_state(OBJECT_POOL)
        assert (d!=NONE)==success and after['next_salt']=='e270'
        if not success:assert after['count']==after['high_water']==after['first_free']==0
        rows.append(dict(case='body_storage_success'if success else 'body_storage_failure',before=before,
            result=f'{d:08x}',after=after,zeroed_body_hex=bytes(o.g.mem_read(o.body(d),0x800)).hex()if success else None,
            entries=o.datum_events))
        if success:
            o.call(0xbd1f0,(d,));assert o.lookup(OBJECT_POOL,d)==0
            rows.append(dict(case='body_and_header_primitive_release',released=f'{d:08x}',after=o.pool_state(OBJECT_POOL),
                reclaimed_bytes=o.u(HEAP+0x30),heap_hex=bytes(o.g.mem_read(HEAP,0x40)).hex(),entries=o.datum_events))
    return rows


def lifecycle_case(o,c,slot,delete_corpse):
    o.fixture(c);player=o.join(slot);assert player==0xec700000|slot
    original=o.object();o.bind(player,original)
    proj=o.object(5);pp=o.body(proj)
    # Reconcile the exact original descriptor copy with the separately frozen
    # firing producer; no slot encoding is performed in this fixture.
    o.w(OWNERS,player);o.w(OWNERS+4,original);o.w(OWNERS+8,o.h(o.body(original)+0x138))
    o.call(0xb8820,(o.u(pp),NONE,OWNERS),((UC_X86_REG_ESI,DESCRIPTOR),))
    o.call(0xb8d0d,registers=((UC_X86_REG_EBP,DESCRIPTOR),(UC_X86_REG_EDI,pp)),end=0xb8d2a)
    assert o.u(pp+0xc4)==player and o.u(pp+0xc8)==original
    stages=[dict(stage='joined_and_bound_shot',state=o.identities(player,[original],proj))]
    o.stop_at=0xd7e42;o.call(0xd7e00,(original,0,0,0),end=0xd7e42);o.stop_at=None
    assert o.u(o.player_ptr(player)+0x2c)==original
    stages.append(dict(stage='body_depleted_still_owned',state=o.identities(player,[original],proj)))
    o.stop_at=0xcbe95;o.call(0xcbdc0,(original,NONE,0,0,0),end=0xcbe95);o.stop_at=None
    assert o.u(o.player_ptr(player)+0x2c)==NONE and o.u(o.player_ptr(player)+0x30)==original
    stages.append(dict(stage='source_death_cleanup_detached',state=o.identities(player,[original],proj)))
    if delete_corpse:
        o.services[0x189490]=('control notification outside ownership',0,None)
        o.call(0x154440,(original,));o.services={}
        o.call(0xbd1f0,(original,))
        stages.append(dict(stage='delete_callback_then_header_release',state=o.identities(player,[original],proj)))
    respawn=o.object();o.bind(player,respawn)
    assert respawn!=original and o.u(o.player_ptr(player)+0x2c)==respawn
    stages.append(dict(stage='same_player_bound_to_respawn_unit',state=o.identities(player,[original,respawn],proj)))
    o.w(o.body(respawn)+0x2a8,player)
    o.call(0x14dcb0,registers=((UC_X86_REG_EAX,player),))
    assert o.lookup(PLAYER_POOL,player)==0 and o.u(pp+0xc4)==player and o.u(pp+0xc8)==original
    assert o.u(o.body(respawn)+0x2a8)==NONE
    stages.append(dict(stage='actual_player_leave_with_object_callbacks',state=o.identities(player,[original,respawn],proj)))
    rejoin=o.join(slot);assert rejoin==0xec710000|slot
    newunit=o.object();o.bind(rejoin,newunit)
    assert o.lookup(PLAYER_POOL,player)==0 and o.lookup(PLAYER_POOL,rejoin)!=0
    stages.append(dict(stage='same_slot_rejoined_new_player_and_unit',state=o.identities(rejoin,[original,respawn,newunit],proj),
        stale_player_lookup=f'{o.lookup(PLAYER_POOL,player):08x}',stale_object_lookup=f'{o.lookup(OBJECT_POOL,original):08x}'))
    inherited_supplied=[e for e in o.damage_events if e.get('kind')=='supplied_service']
    inherited_supplied+=[e for e in o.accounting_events if 'supplied' in e.get('kind','')]
    assert not inherited_supplied,inherited_supplied
    return dict(slot=slot,delete_corpse=delete_corpse,player=f'{player:08x}',rejoined_player=f'{rejoin:08x}',
        original_unit=f'{original:08x}',respawn_unit=f'{respawn:08x}',new_unit=f'{newunit:08x}',
        projectile=f'{proj:08x}',stages=stages,entries=o.datum_events,
        supplied_control_notifications=[e for e in o.events if e.get('event')=='supplied_service'],
        source_published_player_properties_hex=o.published.hex(),
        boundaries=['Empty object pool and scripted unit/projectile allocation order; no authored world object population.',
            '18FCE0 body storage arena success/addresses and requested 800-byte body size are supplied.',
            'Object active/type fields and real bipd/proj tag assignment are supplied; only C52C0 unit/history prefix executes.',
            'Owner triple is read from the actual bound fixture unit; B8820 descriptor construction and B8D0D owner copy execute. Complete firing remains the separately frozen producer.',
            'Body-depleted prefix ends D7E42 before animation notification; death cleanup prefix ends CBE95 after detach, with real bipd ragdoll-state fixture 3 and explicit NONE/zero arguments.',
            'Respawn representation, world creation/placement and scheduling are not executed; original pool allocation and binding execute.',
            'Optional object deletion executes player deletion callback then body/header primitive; full world/type deletion orchestration remains outside this case.'])


def listing(o,path):
    ranges=[(0x14cc00,0x14cc75),(0x14ccd0,0x14cd4d),(0xb76b0,0xb7720),
        (0x16cff0,0x16d6a0),(0x14d350,0x14d3d0),(0x14d600,0x14dc30),
        (0x14dcb0,0x14dde0),(0x14e240,0x14e3b0),(0xbd170,0xbd290),
        (0xbc7e0,0xbc860),(0xbbda0,0xbbec0),(0x10d0f0,0x10d170),
        (0xd0300,0xd0340),(0xb8d0d,0xb8d2a),(0xbd080,0xbd0c0),
        (0xb8820,0xb8a30),(0xc52c0,0xc5464),(0xd7e00,0xd7e42),(0xcbdc0,0xcbe95),(0x154440,0x154570)]
    cs=Cs(CS_ARCH_X86,CS_MODE_32);lines=[]
    for a,b in ranges:
        lines.append(f'\n# original TU1.1 {a:08x}..{b:08x} (exclusive)')
        for i in cs.disasm(bytes(o.g.mem_read(a,b-a)),a):lines.append(f'{i.address:08x}  {i.bytes.hex():<28} {i.mnemonic} {i.op_str}')
    lines.append('\n# original pool name/type-descriptor data')
    for a,n in ((0x45aaf4,8),(0x45a3a4,8),(0x46fad0,48),(0x46ef18+0x84,16),(0x46f170+0x84,16),(0x46f3c8+0x84,16),
                (0x46ed88+0x5c,4),(0x46ee50+0x5c,4),(0x46ef18+0x5c,4),
                (0x46f0a8+0x5c,4),(0x46f170+0x5c,4),(0x46f3c8+0x5c,4)):
        lines.append(f'{a:08x} {bytes(o.g.mem_read(a,n)).hex()}')
    path.write_text('\n'.join(lines)+'\n')


def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'datums-report.json');args=ap.parse_args()
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    frozen={
        'smg-producers-report.json':'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa',
        'smg-damage-report.json':'2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca',
        'slayer-accounting-report.json':'284cbcd6bb293bb8e9edc74e62dc9ffe3b3a2929877983884abc93d05b66f128'}
    for n,pin in frozen.items():assert sha(OUT/n)==pin
    o=DatumOracle(c)
    rows=allocator_cases(o,c);storage=storage_cases(o,c)
    life=[lifecycle_case(o,c,slot,delete)for slot in (0,3,15)for delete in (False,True)]
    args.output.parent.mkdir(parents=True,exist_ok=True)
    listing_path=args.output.with_name('tu11-datums-instructions.txt');listing(o,listing_path)
    deps=('oracle_mp_spawn.py','oracle_mp_spawn_lifecycle.py','oracle_mp_spawn_ownership.py',
        'oracle_mp_spawn_slayer_accounting.py','oracle_mp_spawn_smg_damage.py','oracle_mp_spawn_smg_producers.py')
    result=dict(schema=1,source_path=VERSIONS['tu11']['path'],source_sha256=VERSIONS['tu11']['pin'],cache_sha256=CACHE_PIN,
        dependencies={str((ROOT/'tools'/n).relative_to(ROOT)):sha(ROOT/'tools'/n)for n in deps},
        oracle_sha256=sha(__file__),frozen_report_inputs=frozen,listing_sha256=sha(listing_path),allocator_cases=rows,storage_cases=storage,lifecycle_cases=life,
        case_count=len(rows)+len(storage)+len(life),instruction_patches=[],head_pose_fixtures=[])
    args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(output=str(args.output),sha256=sha(args.output),cases=result['case_count'])))

if __name__=='__main__':main()
