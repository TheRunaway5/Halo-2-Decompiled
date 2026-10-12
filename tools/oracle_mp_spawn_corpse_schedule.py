#!/usr/bin/env python3
"""Untouched TU1.1 death scheduling and bounded corpse collection decisions.

Real salted pools, stock SMG results, ownership and collector instructions run.
Marker poses, world deletion and animation components are explicit stop points.
No previous suite is edited and no executable instructions are patched.
"""
import argparse
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_mp_spawn import ROOT,VERSIONS,CACHE_PIN,Cache,PLAYER_POOL,OBJECT_POOL,ENGINE_GLOBALS
from oracle_mp_spawn_lifecycle import EXTRA,TIME
from oracle_mp_spawn_ownership import GARBAGE
from oracle_mp_spawn_smg_damage import EVENT,NONE,sha
from oracle_mp_spawn_datums import DatumOracle,HEAP,OUT
from oracle_mp_spawn_lethal_attribution import AttributionOracle

BUFFER=EXTRA+0x228000
STATUS=EXTRA+0x22b000
PARTITION=EXTRA+0x22c000
FROZEN={
 'lethal-attribution-report.json':'46d65d088255fc1fc357a84aa6e8d17b6065387327be9c03ba9f392ce0b1c84d',
 'datums-report.json':'9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629',
 'smg-producers-report.json':'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa',
 'smg-damage-report.json':'2b7d072e92dde4ad6af7a92f301cf5b734410df44745c988bbcf6b5b372e9aca'}

class CorpseOracle(AttributionOracle):
    def __init__(self,c,event):
        self.corpse_trace=[];self.writer_actual=False;self.retention_service=None
        self.stop_delete=False;self.stop_marker=False
        super().__init__(c,event)

    def corpse(self,d=None):
        d=self.victim_unit if d is None else d
        h,p=self.header(d),self.body(d)
        pp=self.player_ptr(self.live_players[3])
        return dict(datum=f'{d:08x}',header_hex=bytes(self.g.mem_read(h,12)).hex(),
            header_salt=f'{self.h(h)&0xffff:04x}',header_flags=self.g.mem_read(h+2,1)[0],
            body_pointer=f'{p:08x}',body_hex=bytes(self.g.mem_read(p,0x800)).hex()if p else None,
            body_fields={f'{off:03x}':f'{self.u(p+off):08x}'for off in
                (4,0xc,0x10,0x14,0x1c,0x20,0x12c,0x130,0x134,0x13c,0x140,0x148,0x264,0x2b8,0x2e0)}if p else {},
            player_current=f'{self.u(pp+0x2c):08x}',player_previous=f'{self.u(pp+0x30):08x}',
            player_countdown=struct.unpack('<i',struct.pack('<I',self.u(pp+0x164)))[0],
            player_penalty=struct.unpack('<i',struct.pack('<I',self.u(pp+0x168)))[0],
            player_snapshot_hex=bytes(self.g.mem_read(ENGINE_GLOBALS+0x558+3*0x18,0x18)).hex(),
            garbage_hex=bytes(self.g.mem_read(GARBAGE,0xc)).hex(),
            garbage_count=self.h(GARBAGE+4),garbage_head=f'{self.u(GARBAGE+8):08x}',
            tick=self.u(TIME+8),object_count=self.u(OBJECT_POOL+0x3c),high_water=self.u(OBJECT_POOL+0x38))

    def marker_arguments(self):
        assert self.g.reg_read(UC_X86_REG_EIP)==0xb9c20
        sp=self.g.reg_read(UC_X86_REG_ESP)
        return dict(endpoint='000b9c20',return_address=f'{self.u(sp):08x}',
            unit=f'{self.u(sp+4):08x}',marker=f'{self.u(sp+8):08x}',capacity=self.u(sp+12),
            output_pointer_ebx=f'{self.g.reg_read(UC_X86_REG_EBX):08x}',
            flag_cl=self.g.reg_read(UC_X86_REG_ECX)&255,
            output='No transform executed or pose bytes supplied/read.')

    def hook(self,g,a,size,user):
        sp=g.reg_read(UC_X86_REG_ESP)
        if self.phase!='fixture':
            names={0xd7e00:'body_depleted',0xd8059:'body_depleted_return',0x1ed960:'attribution',
                0x15e4d0:'death_writer',0xcb410:'aftermath',0xcba60:'history_write',0xcbdc0:'death_cleanup',
                0xe6050:'biped_death',0xbc840:'garbage_schedule',0x14e240:'bind',0x153a90:'player_status',
                0xcbe95:'post_detach',0xd22b0:'unit_death_flags',0xcbf21:'animation_component_boundary',
                0xb8040:'object_update_final',0xc0270:'collect_dispatch',0xc00b0:'pressure_gate',0xc0130:'candidates',0xc01b0:'candidate_consumer',
                0xbc970:'retention',0xb92a0:'delete_prepare',0xb9350:'delete_world',
                0x154440:'player_object_deleted',0xbd1f0:'storage_release',0x18fea0:'heap_compaction',
                0xb9c20:'marker_boundary',0xb9970:'object_partition_iterator',0xb9a10:'next_partition'}
            if a in names:
                row=dict(stage=names[a],entry=f'{a:08x}',state=self.corpse(),
                    registers={n:f'{g.reg_read(r):08x}'for n,r in
                        (('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),
                         ('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI))},
                    arguments=[f'{self.u(sp+4+i*4):08x}'for i in range(6)])
                if a==0xc01b0:row['level']=self.u(sp+4)
                if a==0xb9c20:row['marker']=dict(unit=f'{self.u(sp+4):08x}',name=f'{self.u(sp+8):08x}',capacity=self.u(sp+12))
                self.corpse_trace.append(row)
            if (a==0xb92a0 and self.stop_delete)or(a==0xb9c20 and self.stop_marker):
                g.emu_stop();return
            if a==0xbc970 and self.retention_service is not None:
                self.corpse_trace.append(dict(stage='supplied_retention_boolean',result=self.retention_service,
                    boundary='World partition/nearby player marker and view evaluation is supplied; no pose.'))
                g.reg_write(UC_X86_REG_EAX,int(self.retention_service));g.reg_write(UC_X86_REG_EIP,self.u(sp))
                g.reg_write(UC_X86_REG_ESP,sp+4);return
        if a==0x15e4d0 and self.writer_actual:
            DatumOracle.hook(self,g,a,size,user);return
        super().hook(g,a,size,user)

    def fixture(self,c):
        super().fixture(c)
        self.writer_actual=False;self.retention_service=None;self.stop_delete=False;self.stop_marker=False
        self.corpse_trace=[]
        self.source_services[0x189490]=('Nonplayer control-system object-deleted notification outside player/object datums',0,None)
        self.g.mem_write(GARBAGE,bytes(0xc));self.w(GARBAGE+8,NONE)
        # The inherited supplied body arena is outside the original loop heap.
        # Heap pressure is explicit metadata, with no tracked loop-heap blocks.
        self.w(HEAP+0x28,0);self.w(HEAP+0x2c,0x40000);self.w(HEAP+0x34,0);self.w(HEAP+0x38,0)
        self.g.mem_write(HEAP+0x3c,b'\x00')
        for name,(d,p)in self.units.items():
            for off in (4,0x1c,0x20,0x134,0x148,0x264,0x2b8,0x2e0):self.attribution_watch[p+off]=f'{name}.{off:03x}'
        for off in (0x164,0x168,0x1b4):self.attribution_watch[self.player_ptr(self.live_players[3])+off]=f'victim.player.{off:03x}'

    def damage_call(self,tick,end=None):
        self.phase='original_stock_smg_death_schedule';self.trace=[];self.corpse_trace=[];self.w(TIME+8,tick)
        self.g.mem_write(EVENT,self.input_event_template)
        projectile=self.shot(self.live_players[0],self.units['attacker0'][0],0)
        self.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,EVENT+8)))
        self.stop_at=end
        self.call(0xd9180,(EVENT,self.victim_unit,NONE,NONE,NONE,0),end=end or 0x3000000)
        self.stop_at=None
        return dict(endpoint=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',source_trace=list(self.corpse_trace),
            attribution_trace=list(self.trace),after=self.corpse())

    def dead(self,c,tick=200):
        self.fixture(c);self.phase='explicit_body_depleted_arguments';self.w(TIME+8,tick)
        self.stop_at=0xd7e42
        self.call(0xd7e00,(self.victim_unit,NONE,1,0),end=0xd7e42);self.stop_at=None
        self.phase='original_death_cleanup_prefix';self.stop_at=0xcbf21
        self.call(0xcbdc0,(self.victim_unit,NONE,0,0,0),end=0xcbf21);self.stop_at=None
        assert self.u(self.victim_body+0x13c)==NONE
        assert self.u(self.victim_body+0x140)==self.live_players[3]
        assert self.u(self.victim_body+0x20)==tick+300 and self.u(self.victim_body+0x2e0)==tick
        assert self.h(GARBAGE+4)==1
        return self.corpse()

def scheduling_cases(o,c):
    o.fixture(c);hits=[]
    for n in range(25):hits.append(o.damage_call(200+n,0xcbf21 if n==24 else None))
    fatal=hits[-1];stages=[e['stage']for e in fatal['source_trace']]
    assert stages.index('body_depleted')<stages.index('body_depleted_return')<stages.index('attribution')<stages.index('aftermath')
    assert stages.index('history_write')<stages.index('death_cleanup')<stages.index('garbage_schedule')<stages.index('bind')<stages.index('post_detach')<stages.index('unit_death_flags')
    assert fatal['after']['body_fields']['020']=='0000020c' and fatal['after']['body_fields']['2e0']=='000000e0'
    rows=[dict(case='25_actual_smg_hits_through_post_detach_death_fields',hits=hits,
        boundary='Stops CBF21 before reading weapon-animation component offset +342; 15E4D0 uses the frozen named Slayer-only adapter.')]
    for seconds,penalty,suicide in ((0,0,False),(5,0,False),(0,5,True)):
        o.fixture(c)
        for n in range(24):
            if suicide:o.hit(o.live_players[3],o.victim_unit,3,200+n)
            else:o.damage_call(200+n)
        o.w(o.options+0x1bc,seconds);o.w(o.options+0x1c0,penalty)
        o.writer_actual=True;o.skip_writer=False;o.stop_marker=True
        o.phase='original_writer_through_marker_boundary';o.trace=[];o.corpse_trace=[];o.w(TIME+8,224)
        o.g.mem_write(EVENT,o.input_event_template)
        attacker=o.live_players[3]if suicide else o.live_players[0]
        owner=o.victim_unit if suicide else o.units['attacker0'][0]
        projectile=o.shot(attacker,owner,3 if suicide else 0)
        o.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,EVENT+8)))
        o.stop_at=0xb9c20;o.call(0xd9180,(EVENT,o.victim_unit,NONE,NONE,NONE,0),end=0xb9c20);o.stop_at=None
        expected=max(30,(penalty if suicide else 0)*30+seconds*30)
        assert o.u(o.player_ptr(o.live_players[3])+0x164)==expected,(seconds,penalty,suicide,o.u(o.player_ptr(o.live_players[3])+0x164),expected)
        assert o.u(o.player_ptr(o.live_players[3])+0x2c)==o.victim_unit and o.u(o.victim_body+0x13c)==o.live_players[3]
        assert o.corpse()['player_snapshot_hex'].startswith('01') and o.h(GARBAGE+4)==0
        marker=o.marker_arguments()
        assert marker['unit']==f'{o.victim_unit:08x}' and marker['marker']=='04000095' and marker['capacity']==1
        rows.append(dict(case=f'actual_death_writer_before_marker_{seconds}s_penalty{penalty}_suicide{suicide}',
            varied_options=dict(respawn_seconds=seconds,suicide_penalty_seconds=penalty,suicide=suicide),
            expected_countdown=expected,marker_boundary=marker,after=o.corpse(),source_trace=list(o.corpse_trace),attribution_trace=list(o.trace),
            boundary='No marker pose supplied; stops at original B9C20 entry before transform, history timer copies, event delivery and Slayer callback.'))
    o.phase='fixture';return rows

def schedule_extension_cases(o,c):
    rows=[]
    for flag in (0,0x4000,0x8000,0xc000):
        o.fixture(c);o.phase='explicit_gc_flags';p=o.victim_body;d=o.victim_unit
        o.w(p+4,flag);o.w(p+0x20,100);o.w(p+0x1c,NONE)
        if flag&0x4000:o.w(GARBAGE+8,d);o.sh(GARBAGE+4,1)
        o.w(TIME+8,200);before=o.corpse()
        o.call(0xbc840,(1,300),((UC_X86_REG_EAX,d),));first=o.corpse()
        o.call(0xbc840,(1,10),((UC_X86_REG_EAX,d),));second=o.corpse()
        assert o.u(p+0x20)==500
        if not flag&0x8000:assert o.h(GARBAGE+4)==1 and o.u(GARBAGE+8)==d
        else:assert o.h(GARBAGE+4)==int(bool(flag&0x4000))
        o.call(0xbc840,(0,NONE),((UC_X86_REG_EAX,d),));after=o.corpse()
        assert o.u(GARBAGE+8)==NONE and o.h(GARBAGE+4)==0
        rows.append(dict(case=f'schedule_flags_{flag:04x}',explicit_initial_flags=f'{flag:08x}',
            before=before,first_enable=first,shorter_enable=second,after_disable=after,source_trace=list(o.corpse_trace)))
    o.phase='fixture';return rows

def collector_cases(o,c):
    rows=[]
    cases=[('ordinary_no_pressure',500,1,0x40000,0,0,None,None),
        ('ordinary_count49',500,49,0x40000,0,0,None,None),('ordinary_count50',500,50,0x40000,0,0,None,2),
        ('ordinary_heap_equal_threshold',500,1,0xcccc,0,0,None,3),
        ('ordinary_heap_above_threshold',500,1,0xcccd,0,0,None,None)]
    for level in range(4):
        for tick in (499,500,501):
            cases.append((f'level{level}_deadline_tick{tick}',tick,1,0x40000,1 if level<2 else 0,1 if level==0 else 0,False,level))
        cases.append((f'level{level}_nearby_retained',500,1,0x40000,1 if level<2 else 0,1 if level==0 else 0,True,level))
    cases.extend([('level2_inactive_header',500,50,0x40000,0,0,False,2),
        ('pressure_highwater_threshold',500,1,0x40000,0,0,False,3),
        ('level3_inactive_header',500,1,0xcccc,0,0,False,3)])
    for name,tick,count,remaining,request,force,nearby,expect_level in cases:
        o.dead(c);o.phase='original_collector_dispatch';o.corpse_trace=[]
        o.w(TIME+8,tick);o.sh(GARBAGE+4,count);o.w(HEAP+0x2c,remaining)
        o.g.mem_write(GARBAGE+1,bytes([request,force]))
        if expect_level==2:o.sh(GARBAGE+4,count if name=='level2_inactive_header'or'count50'in name else 50)
        if expect_level==3 and remaining>0xcccc:
            # Explicit header-pressure boundary. C0270 uses count; C00B0 uses high water.
            o.w(OBJECT_POOL+0x3c,0x800-0x66);o.w(OBJECT_POOL+0x38,0x800-0x32)
        if 'inactive_header'in name:o.g.mem_write(o.header(o.victim_unit)+2,b'\x00')
        before=o.corpse();o.retention_service=nearby;o.stop_delete=True;o.stop_at=0xb92a0
        try:o.call(0xc0270,end=0xb92a0)
        except AssertionError:
            if o.g.reg_read(UC_X86_REG_EIP)!=0x3000000:raise
        o.stop_at=None;endpoint=o.g.reg_read(UC_X86_REG_EIP)
        consumed=[e for e in o.corpse_trace if e['stage']=='candidate_consumer']
        actual=consumed[0]['level']if consumed else None
        assert actual==expect_level,(name,actual,expect_level)
        should_delete=expect_level is not None and tick>=500 and not('level2_inactive'in name)and(not nearby or expect_level==0)
        assert (endpoint==0xb92a0)==should_delete,(name,endpoint,should_delete)
        if should_delete:assert o.u(GARBAGE+8)==NONE and not o.u(o.victim_body+4)&0x4000
        else:assert o.u(GARBAGE+8)==o.victim_unit
        rows.append(dict(case=name,explicit_inputs=dict(tick=tick,active_garbage_count=count,heap_remaining=remaining,
            request_byte=request,force_byte=force,nearby_service=nearby,level=expect_level),
            before=before,after=o.corpse(),delete_preparation_requested=should_delete,endpoint=f'{endpoint:08x}',
            source_trace=list(o.corpse_trace),boundary='Deletion cases stop before B92A0 world/global callback preparation; retained cases return from complete C0270.'))
        o.phase='fixture'
    return rows

def candidate_cases(o,c):
    rows=[]
    for mode in (0,4):
        o.dead(c);o.phase='original_candidate_enumeration';o.g.mem_write(o.options+0xc,bytes([mode]))
        included=[]
        for name,parent,attached in (('attacker0',NONE,NONE),('attacker7',o.victim_unit,NONE),('attacker15',NONE,0x12340000)):
            d,p=o.units[name];o.w(p+0x14,parent);o.w(p+0xd4,attached)
            o.call(0xbc840,(1,300),((UC_X86_REG_EAX,d),))
            if parent==NONE and(mode!=4 or attached==NONE):included.append(d)
        o.call(0xc0130,(3,BUFFER,0x2800))
        n=o.u(BUFFER);actual=[o.u(BUFFER+4+i*4)for i in range(n)]
        assert actual==list(reversed(included))+[o.victim_unit]
        rows.append(dict(case=f'candidate_enumeration_mode{mode}',candidates=[f'{d:08x}'for d in actual],
            enumeration_order='Garbage linked-list head to tail; consumer later pops from the end.',
            source_trace=list(o.corpse_trace),explicit_parent_and_entity_fields=True))
        o.phase='fixture'
    o.dead(c);o.phase='original_multiple_corpse_schedule'
    for name in ('attacker0','attacker7'):
        d,p=o.units[name];o.w(TIME+8,o.u(TIME+8)+1);o.stop_at=0xd7e42
        o.call(0xd7e00,(d,NONE,1,0),end=0xd7e42);o.stop_at=0xcbf21
        o.call(0xcbdc0,(d,NONE,0,0,0),end=0xcbf21);o.stop_at=None
    o.call(0xc0130,(1,BUFFER,0x2800));candidates=[o.u(BUFFER+4+i*4)for i in range(o.u(BUFFER))]
    o.w(TIME+8,502);o.retention_service=True;o.corpse_trace=[]
    for n in range(3):
        value=o.call(0xc01b0,(1,BUFFER,0x2800,STATUS,STATUS+0x100,0x200))
        assert value==0 and o.g.mem_read(STATUS,1)[0]==int(n<2)
    consumed=[e['registers']['eax']for e in o.corpse_trace if e['stage']=='retention']
    assert consumed==[f'{d:08x}'for d in reversed(candidates)]
    rows.append(dict(case='three_corpse_candidate_pop_order',candidate_head_to_tail=[f'{d:08x}'for d in candidates],
        actual_consumer_order=consumed,source_trace=list(o.corpse_trace),after=o.corpse(),
        boundary='Three original cleanup prefixes enqueue units. Nearby retention boolean true is supplied; no deletion callbacks execute.'))
    o.phase='fixture'
    return rows

def pressure_cases(o,c):
    rows=[]
    values=[(2,30,0x40000,2048),(2,31,0x40000,2048),(2,-1,0x40000,2048)]
    for remaining,free in ((0x19999,0xcc),(0x19998,0xcc),(0x19999,0xcb),
        (0x6666,0x33),(0x6665,0x33),(0x6666,0x32)):
        values.append((3,1,remaining,free))
    for level,count,remaining,free in values:
        o.fixture(c);o.phase='explicit_pressure_predicate';o.sh(GARBAGE+4,count);o.w(HEAP+0x2c,remaining)
        o.w(OBJECT_POOL+0x38,2048-free);o.g.mem_write(STATUS,b'\x7f')
        result=o.call(0xc00b0,registers=((UC_X86_REG_EAX,level),(UC_X86_REG_ESI,STATUS)))&255
        critical=o.g.mem_read(STATUS,1)[0]
        assert result==int(count>30 if level==2 else remaining<0x19999 or free<0xcc)
        assert critical==int(level>2 and(remaining<0x6666 or free<0x33))
        rows.append(dict(case=f'pressure_level{level}_count{count}_remaining{remaining:x}_free{free}',
            explicit_inputs=dict(level=level,signed_active_garbage_count=count,remaining_heap_bytes=remaining,free_highwater_headers=free),
            pressure=result,critical=critical,source_trace=list(o.corpse_trace)))
        o.phase='fixture'
    return rows

def caller_cases(o,c):
    rows=[]
    for tick in (499,1000):
        o.dead(c);o.phase='complete_object_final_update_caller';o.w(TIME+8,tick);o.corpse_trace=[]
        o.source_services[0x1019f0]=('Pending object movement/render queue update outside this header-flag fixture',0,None)
        before=o.corpse();o.call(0xb8040);after=o.corpse()
        assert o.u(GARBAGE+8)==o.victim_unit and o.u(o.header(o.victim_unit)+8)==o.victim_body
        assert any(e['stage']=='collect_dispatch'for e in o.corpse_trace)
        assert o.g.mem_read(GARBAGE+3,1)[0]==0
        rows.append(dict(case=f'complete_B8040_caller_tick{tick}',before=before,after=after,
            source_trace=list(o.corpse_trace),
            boundaries=['All allocated headers have explicit active-only flags; no object/type/Havok update is entered.',
                '1019F0 pending movement/render queue is a named supplied leaf; original B8040 and C0270 execute completely.']))
        o.phase='fixture'
    return rows

def retention_cases(o,c):
    rows=[]
    for name,active,flags,partition in (('inactive',False,0x100,False),('no_flag8',True,0,False),
        ('flag18',True,0x40100,False),('no_partition',True,0x100,False),('marker_required',True,0x100,True),
        ('no_current_units',True,0x100,True)):
        o.dead(c);o.phase='original_retention_gate';o.corpse_trace=[];o.w(o.victim_body+4,o.u(o.victim_body+4)|flags)
        if not active:o.g.mem_write(o.header(o.victim_unit)+2,b'\x00')
        o.w(o.victim_body+0x60,NONE)
        if partition:
            # Explicit minimal world partition relation, never a marker/head pose.
            o.w(0x4e58a4+8,PARTITION);o.w(PARTITION+0x24,12);o.w(PARTITION+0x44,PARTITION+0x80)
            o.sh(PARTITION+0x84,0);o.w(PARTITION+0x88,NONE);o.w(o.victim_body+0x60,0x10000)
            o.w(o.options+0x1138,1)
        if name=='no_current_units':
            for player in o.live_players.values():o.bind(player,NONE)
        o.stop_marker=True;o.stop_at=0xb9c20
        try:value=o.call(0xbc970,registers=((UC_X86_REG_EAX,o.victim_unit),),end=0xb9c20)
        except AssertionError:
            if o.g.reg_read(UC_X86_REG_EIP)!=0x3000000:raise
            value=o.g.reg_read(UC_X86_REG_EAX)
        o.stop_at=None;marker=o.g.reg_read(UC_X86_REG_EIP)==0xb9c20
        assert marker==(name=='marker_required')
        if not marker:assert value&255==0
        rows.append(dict(case=f'retention_{name}',explicit_inputs=dict(header_active=active,flags=f'{flags:08x}',partition=partition),
            reaches_marker=marker,result_low_byte=None if marker else value&255,after=o.corpse(),source_trace=list(o.corpse_trace),
            marker_boundary=o.marker_arguments()if marker else None,
            boundary='Minimal world partition relation is an explicit varied boundary; original marker transform stops without supplying a pose.'))
        o.phase='fixture'
    return rows

def release_cases(o,c):
    rows=[]
    for variant in ('previous_only','respawned_new_unit','still_current'):
        o.dead(c);victim=o.live_players[3];old=o.victim_unit
        if variant=='respawned_new_unit':o.unit(victim,'new_victim')
        elif variant=='still_current':o.bind(victim,old)
        o.w(o.player_ptr(victim)+0x174,old)
        o.phase='original_player_delete_callback_and_lower_storage';o.corpse_trace=[]
        before=o.corpse();o.call(0x154440,(old,));callback=o.corpse()
        assert o.u(o.player_ptr(victim)+0x30)==NONE and o.u(o.player_ptr(victim)+0x174)==NONE
        if variant=='respawned_new_unit':assert o.u(o.player_ptr(victim)+0x2c)==o.units['new_victim'][0]
        else:assert o.u(o.player_ptr(victim)+0x2c)==NONE
        # Original collector unlink precedes world delete. Full world/type/AI/
        # animation deletion callbacks are intentionally outside this kernel.
        o.call(0xbc840,(0,NONE),((UC_X86_REG_EAX,old),))
        o.call(0xbd1f0,(old,));released=o.corpse()
        assert o.lookup(OBJECT_POOL,old)==0 and o.u(o.header(old)+8)==0
        new=o.object();assert new&0xffff==old&0xffff and new>>16!=old>>16
        rows.append(dict(case=variant,before=before,after_player_callback=callback,after_storage_release=released,
            reused_object=f'{new:08x}',source_trace=list(o.corpse_trace),
            boundary='Complete 154440 and BD1F0 execute separately. This does not execute B92A0/B9350 complete world deletion.'))
        o.phase='fixture'
    return rows

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'corpse-schedule-report.json');args=ap.parse_args()
    for name,pin in FROZEN.items():assert sha(OUT/name)==pin
    producer=json.loads((OUT/'smg-producers-report.json').read_text());event=bytes.fromhex(producer['stock_direct_impact_events'][0]['event_hex'])
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN
    o=CorpseOracle(c,event)
    result=dict(schema=1,source_sha256=VERSIONS['tu11']['pin'],cache_sha256=CACHE_PIN,
        frozen_reports=FROZEN,oracle_sha256=sha(__file__),instruction_patches=[],marker_pose_fixtures=[],
        scheduling_cases=scheduling_cases(o,c),garbage_schedule_cases=schedule_extension_cases(o,c),
        collector_cases=collector_cases(o,c),candidate_cases=candidate_cases(o,c),retention_cases=retention_cases(o,c),
        pressure_cases=pressure_cases(o,c),caller_cases=caller_cases(o,c),release_cases=release_cases(o,c))
    result['case_count']=sum(len(v)for k,v in result.items()if k.endswith('_cases'))
    md=Cs(CS_ARCH_X86,CS_MODE_32);lines=[]
    for a,b in ((0xd7e00,0xd81c0),(0xda2b0,0xda710),(0xdac40,0xdb160),(0xcb410,0xcbdc0),
        (0xcbdc0,0xcc260),(0xbc840,0xbcd00),(0xc00b0,0xc0400),(0xb8040,0xb8110),
        (0xb92a0,0xb9530),(0x154440,0x154540),(0xbd1f0,0xbd270),(0x15e4d0,0x15e870),
        (0x139750,0x139940),(0x10cd80,0x10cdf0)):
        lines.append(f'\n# untouched TU1.1 {a:08x}..{b:08x} exclusive')
        for i in md.disasm(bytes(o.g.mem_read(a,b-a)),a):lines.append(f'{i.address:08x} {i.bytes.hex():<28} {i.mnemonic} {i.op_str}')
    for a,n in ((0x446bb0,0x60),(0x46fb04,0x20)):
        lines.append(f'original_table {a:08x} '+bytes(o.g.mem_read(a,n)).hex())
    listing=args.output.with_name('tu11-corpse-schedule-instructions.txt');listing.write_text('\n'.join(lines)+'\n')
    result['listing_sha256']=sha(listing)
    deps=('oracle_mp_spawn_lethal_attribution.py','oracle_mp_spawn_datums.py','oracle_mp_spawn_smg_producers.py',
        'oracle_mp_spawn_smg_damage.py','oracle_mp_spawn_slayer_accounting.py','oracle_mp_spawn_ownership.py',
        'oracle_mp_spawn_lifecycle.py','oracle_mp_spawn.py')
    result['dependencies']={f'tools/{p}':sha(ROOT/'tools'/p)for p in deps}
    result['boundaries']=['Inherited supplied body arena and active object fixture; complete startup/world allocation order is unproved.',
        'Actual Lockout tags are read unchanged. World-impact event and effect/animation leaves are inherited named services.',
        'Death cleanup ends CBF21 before weapon-animation component; no component, pose, weapon or grenade state is fabricated.',
        'Collector pressure metadata and partition relation are explicit varied inputs. Nearby boolean variants are named service boundaries.',
        'Deletion intent stops before B92A0. Full world/animation/physics/type deletion is not replaced by lower body release.',
        '154440 and BD1F0 complete kernels prove player callback and storage identity effects separately.',
        'Actual 15E4D0 writer cases stop at B9C20 before transform and subsequent snapshot/engine event effects.']
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=result['case_count'],output=str(args.output),sha256=sha(args.output))))

if __name__=='__main__':main()
