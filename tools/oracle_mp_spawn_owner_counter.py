#!/usr/bin/env python3
"""Original incoming-owner CBA60 counter and selected-credit timer source gate.

Complete counter instructions and actual salted ownership producers execute.
Varied state/result inputs are explicit. No old suite, instruction or pose changes.
"""
import argparse
import json
import math
import struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,
    UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_XMM0)
from oracle_mp_spawn import ROOT,VERSIONS,CACHE_PIN,Cache,OBJECT_POOL,ENGINE_GLOBALS,f32
from oracle_mp_spawn_lifecycle import TIME
from oracle_mp_spawn_smg_damage import EVENT,NONE,sha,bits
from oracle_mp_spawn_datums import OUT
from oracle_mp_spawn_corpse_schedule import CorpseOracle

FROZEN={
 'corpse-schedule-report.json':'649a0761e442ff5698476a18d4fcef36ebe89d4431ecb8619de497a46e357f6d',
 'lethal-attribution-report.json':'46d65d088255fc1fc357a84aa6e8d17b6065387327be9c03ba9f392ce0b1c84d',
 'datums-report.json':'9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629',
 'smg-producers-report.json':'c8f66fbb8f5041acad27e2b113d0f8952f3654ec8bfd31fd626e48acfef4fcfa'}

def signed32(v):return struct.unpack('<i',struct.pack('<I',v&NONE))[0]
def signed16(v):return struct.unpack('<h',struct.pack('<H',v&0xffff))[0]

class CounterOracle(CorpseOracle):
    def __init__(self,c,event):
        self.counter_trace=[]
        super().__init__(c,event)

    def counter_state(self):
        return dict(tick=f'{self.u(TIME+8):08x}',seconds_per_tick=f'{self.u(TIME+4):08x}',
            ai_enabled=self.g.mem_read(self.u(0x4fcda8)+1,1)[0],
            history=self.history_state(),units={name:dict(datum=f'{d:08x}',body=f'{p:08x}',
                header_salt=f'{self.h(self.header(d))&0xffff:04x}',header_type=self.g.mem_read(self.header(d)+3,1)[0],
                header_body=f'{self.u(self.header(d)+8):08x}',owner=f'{self.u(p+0x13c):08x}',
                previous_owner=f'{self.u(p+0x140):08x}',membership=self.h(p+0x138),
                dead=bool(self.g.mem_read(p+0x10a,1)[0]&4),forward_9=f'{self.u(p+0x248):08x}',
                forward_other=f'{self.u(p+0x24c):08x}',counter=self.h(p+0x2ea),last_tick=f'{self.u(p+0x2ec):08x}')
                for name,(d,p)in self.units.items()},
            players={str(slot):dict(full=f'{d:08x}',current=f'{self.u(self.player_ptr(d)+0x2c):08x}',
                membership=self.g.mem_read(self.player_ptr(d)+0xc0,1)[0])for slot,d in self.live_players.items()})

    def hook(self,g,a,size,user):
        if self.phase!='fixture':
            names={0xcba60:'history_then_counter_entry',0xcbc89:'fallback_object_lookup',0xcbce6:'resolved_owner_body',
                0xcbd19:'forwarded_body_dead_gate',0xcbd55:'reset_before_increment',0xcbd5e:'increment_short',
                0xcbd7f:'enabled_cap_gate',0xcbda6:'cap_reset',0xcbdaf:'counter_return',
                0x1e2f50:'membership_relationship',0x1cb3d0:'actual_AI_new_map_initializer',
                0x1cb990:'AI_dispose_reset',0x1cba00:'AI_new_map_reset'}
            if a in names:
                sp=g.reg_read(UC_X86_REG_ESP)
                row=dict(stage=names[a],entry=f'{a:08x}',state=self.counter_state(),
                    xmm0_bits=f'{g.reg_read(UC_X86_REG_XMM0)&NONE:08x}',
                    registers={n:f'{g.reg_read(r):08x}'for n,r in
                        (('eax',UC_X86_REG_EAX),('edx',UC_X86_REG_EDX),('ebx',UC_X86_REG_EBX),
                         ('ecx',UC_X86_REG_ECX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI))})
                if a==0xcba60:row.update(arguments=[f'{self.u(sp+4+i*4):08x}'for i in range(6)],
                    amount_bits=f'{g.reg_read(UC_X86_REG_XMM0)&NONE:08x}')
                self.counter_trace.append(row)
        super().hook(g,a,size,user)

    def fixture(self,c):
        super().fixture(c);self.counter_trace=[]

    def request(self,attacker=None,owner=None,designator=0,damage_class=2,lethal=1,amount=.125,tick=200):
        attacker=self.live_players[0]if attacker is None else attacker
        owner=self.units['attacker0'][0]if owner is None else owner
        self.phase='explicit_normalized_history_counter_request';self.counter_trace=[];self.trace=[]
        self.w(TIME+8,tick);before=self.counter_state()
        argv=(damage_class,lethal,attacker,designator&NONE,owner,8)
        self.g.reg_write(UC_X86_REG_XMM0,bits(amount))
        self.call(0xcba60,argv,((UC_X86_REG_EAX,self.victim_unit),))
        after=self.counter_state()
        writes=[e for e in self.trace if e['stage']=='source_write']
        trace=list(self.counter_trace);self.phase='fixture'
        return dict(input=dict(victim=f'{self.victim_unit:08x}',raw_arguments=[f'{v&NONE:08x}'for v in argv],
            damage_class_short=signed16(damage_class),lethal_byte=lethal&255,attacker=f'{attacker:08x}',
            designator_short=signed16(designator),owner=f'{owner:08x}',amount_bits=f'{bits(amount):08x}'),
            before=before,after=after,source_trace=trace,source_writes=writes,complete_original_return=True)

    def depleted(self,name):
        d,p=self.units[name];self.phase='original_body_depleted_prefix';self.stop_at=0xd7e42
        self.call(0xd7e00,(d,NONE,1,0),end=0xd7e42);self.stop_at=None
        assert self.g.mem_read(p+0x10a,1)[0]&4

def admission_cases(o,c):
    rows=[]
    variants=('ordinary','zero_amount','not_lethal','upper_bits_only_lethal','low_byte_lethal','no_designator',
        'upper_bits_none_designator','same_membership','none_player_valid_object','none_player_no_object',
        'stale_player_valid_object','out_of_range_player_valid_object','current_none_fallback',
        'valid_player_preferred_over_other_object','stale_object','out_of_range_object','zero_object_salt',
        'fallback_type0','fallback_type1','fallback_type2','fallback_type5','dead_primary_no_fallback',
        'dead_primary_forward_alive','alive_primary_forward_dead','forward_class9','forward_class2',
        'forward_class9_upper_bits','forward_class_minus1','forwarded_salt_not_rechecked',
        'current_unit_salt_not_rechecked','current_unit_type_not_rechecked','current_body_null_fallback')
    for name in variants:
        o.fixture(c);args={};expected='attacker0'
        a0,p0=o.units['attacker0'];a7,p7=o.units['attacker7'];a15,p15=o.units['attacker15']
        if name=='zero_amount':args['amount']=0
        elif name=='not_lethal':args['lethal']=0;expected=None
        elif name=='upper_bits_only_lethal':args['lethal']=0x100;expected=None
        elif name=='low_byte_lethal':args['lethal']=0x101
        elif name=='no_designator':args['designator']=NONE;expected=None
        elif name=='upper_bits_none_designator':args['designator']=0x1234ffff;expected=None
        elif name=='same_membership':args['designator']=3;expected=None
        elif name=='none_player_valid_object':args.update(attacker=NONE,owner=a7);expected='attacker7'
        elif name=='none_player_no_object':args.update(attacker=NONE,owner=NONE);expected=None
        elif name=='stale_player_valid_object':args.update(attacker=o.live_players[0]^0x10000,owner=a7);expected='attacker7'
        elif name=='out_of_range_player_valid_object':args.update(attacker=0xec700010,owner=a7);expected='attacker7'
        elif name=='current_none_fallback':o.bind(o.live_players[0],NONE);args['owner']=a7;expected='attacker7'
        elif name=='valid_player_preferred_over_other_object':args['owner']=a7
        elif name=='stale_object':args.update(attacker=NONE,owner=a7^0x10000);expected=None
        elif name=='out_of_range_object':args.update(attacker=NONE,owner=0xe26f0800);expected=None
        elif name=='zero_object_salt':o.sh(o.header(a7),0);args.update(attacker=NONE,owner=a7);expected=None
        elif name.startswith('fallback_type'):
            typ=int(name[-1]);o.g.mem_write(o.header(a7)+3,bytes([typ]));args.update(attacker=NONE,owner=a7)
            expected='attacker7'if typ in(0,1)else None
        elif name=='dead_primary_no_fallback':o.depleted('attacker0');args['owner']=a7;expected=None
        elif name=='dead_primary_forward_alive':o.depleted('attacker0');o.w(p0+0x24c,a7);expected='attacker7'
        elif name=='alive_primary_forward_dead':o.depleted('attacker7');o.w(p0+0x24c,a7);expected=None
        elif name.startswith('forward_class'):
            o.w(p0+0x248,a7);o.w(p0+0x24c,a15)
            args['damage_class']={'forward_class9':9,'forward_class2':2,'forward_class9_upper_bits':0x10009,
                'forward_class_minus1':NONE}[name]
            expected='attacker7'if args['damage_class']&0xffff==9 else 'attacker15'
        elif name=='forwarded_salt_not_rechecked':o.w(p0+0x24c,a7^0x10000);expected='attacker7'
        elif name=='current_unit_salt_not_rechecked':o.w(o.player_ptr(o.live_players[0])+0x2c,a0^0x10000)
        elif name=='current_unit_type_not_rechecked':o.g.mem_write(o.header(a0)+3,b'\x05')
        elif name=='current_body_null_fallback':
            o.bind(o.live_players[0],NONE);o.call(0xbd1f0,(a0,))
            o.w(o.player_ptr(o.live_players[0])+0x2c,a0);args['owner']=a7;expected='attacker7'
        row=o.request(**args);changed=[n for n,u in row['after']['units'].items()if u['counter']!=row['before']['units'][n]['counter']]
        assert changed==([]if expected is None else[expected]),(name,changed,expected)
        for n in changed:assert row['after']['units'][n]['counter']==1 and row['after']['units'][n]['last_tick']=='000000c8'
        rows.append(dict(case=name,expected_recipient=expected,request=row,
            boundary='All variants are explicit normalized-result/reference/header-state inputs; invalid-reference variants demonstrate unchecked source preconditions, not source-produced healthy state.'))
    return rows

def timing_cases(o,c):
    rows=[]
    sp=bits(1/30)
    variants=[('none_tick',5,NONE,200,sp),('below_four_seconds',2,100,219,sp),
        ('equal_four_seconds',2,100,220,sp),('above_four_seconds',2,100,221,sp),
        ('previous_in_future',2,200,199,sp),('zero_seconds_per_tick',2,100,10000,0),
        ('nan_seconds_per_tick',2,100,10000,0x7fc00000),('next_float_above_rate',2,100,220,sp+1),
        ('next_float_below_rate',2,100,220,sp-1),('subtraction_wrap_negative',2,0x80000000,0,sp),
        ('tick_wrap_one_step',2,0x7fffffff,0x80000000,sp),('short_wrap',32767,200,200,sp),
        ('short_negative_to_zero',-1,200,200,sp),('short_min_increment',-32768,200,200,sp)]
    for name,count,prior,tick,sec_bits in variants:
        o.fixture(c);p=o.units['attacker0'][1];o.sh(p+0x2ea,count);o.w(p+0x2ec,prior);o.w(TIME+4,sec_bits)
        delta=signed32(tick-prior);sec=struct.unpack('<f',struct.pack('<I',sec_bits))[0]
        elapsed=f32(f32(float(delta))*sec);reset=prior==NONE or elapsed>4
        expected=signed16((0 if reset else count)+1);row=o.request(tick=tick)
        assert row['after']['units']['attacker0']['counter']==expected,(name,expected,row['after']['units']['attacker0'])
        assert row['after']['units']['attacker0']['last_tick']==f'{tick:08x}'
        if prior!=NONE:
            observed=next(e['xmm0_bits']for e in row['source_trace']if e['stage']in('reset_before_increment','increment_short'))
            assert observed==f'{bits(elapsed):08x}',(name,observed,f'{bits(elapsed):08x}')
        rows.append(dict(case=name,explicit_inputs=dict(counter=count,last_tick=f'{prior:08x}',tick=f'{tick:08x}',seconds_per_tick_bits=f'{sec_bits:08x}'),
            signed_wrapping_delta=delta,elapsed_bits=f'{bits(elapsed):08x}',reset=reset,expected_counter=expected,request=row))
    return rows

def cap_cases(o,c):
    rows=[]
    for enabled in (0,1):
        for owned in (False,True):
            for count in (0,1,2,3,4,5,32767,-1):
                o.fixture(c);d,p=o.units['attacker0']
                if not owned:o.bind(o.live_players[0],NONE)
                o.g.mem_write(o.u(0x4fcda8)+1,bytes([enabled]));o.sh(p+0x2ea,count);o.w(p+0x2ec,200)
                post_increment=signed16(count+1);threshold=5 if owned else 3
                expected=0 if enabled and post_increment>=threshold else post_increment
                row=o.request(attacker=o.live_players[0]if owned else NONE,owner=d)
                assert row['after']['units']['attacker0']['counter']==expected
                assert row['after']['units']['attacker0']['last_tick']=='000000c8'
                rows.append(dict(case=f'ai{enabled}_owned{owned}_counter{count}',explicit_inputs=dict(ai_enabled=enabled,owned=owned,count=count),
                    post_increment=post_increment,threshold=threshold,expected_counter=expected,request=row))
    return rows

def lifecycle_cases(o,c):
    rows=[]
    for name in ('old_unit_after_respawn','old_player_after_leave_rejoin','old_object_after_leave_release_reuse'):
        o.fixture(c);old_player=o.live_players[0];old_unit=o.units['attacker0'][0]
        transition=o.respawn_attacker()if name=='old_unit_after_respawn'else o.leave_rejoin()
        if name=='old_object_after_leave_release_reuse':
            o.call(0x154440,(old_unit,));o.call(0xbd1f0,(old_unit,))
            d=o.object();o.units['reused_object']=(d,o.body(d));assert d&0xffff==old_unit&0xffff and d!=old_unit
        row=o.request(attacker=old_player,owner=old_unit)
        recipient='respawned0'if name=='old_unit_after_respawn'else 'attacker0'if name=='old_player_after_leave_rejoin'else None
        changed=[n for n,u in row['after']['units'].items()if u['counter']!=row['before']['units'][n]['counter']]
        assert changed==([]if recipient is None else[recipient]),(name,changed,recipient)
        rows.append(dict(case=name,source_transition=transition,expected_recipient=recipient,request=row))
    return rows

def AI_producer_cases(o,c):
    rows=[]
    for fn in (0x1cb990,0x1cba00,0x1cb3d0):
        o.fixture(c);o.phase='actual_AI_state_producer';o.counter_trace=[];ai=o.u(0x4fcda8)
        o.g.mem_write(ai,b'\xa5'*0x374);before=bytes(o.g.mem_read(ai,0x374)).hex();o.call(fn)
        assert o.g.mem_read(ai+1,1)[0]==0
        rows.append(dict(case=f'complete_{fn:08x}_multiplayer_state2',options_state=o.u(o.options+8),
            before_hex=before,after_hex=bytes(o.g.mem_read(ai,0x374)).hex(),source_trace=list(o.counter_trace),
            complete_original_return=True,boundary='AI global and two local-player AI records are mapped; full world bootstrap is not executed.'))
        o.phase='fixture'
    return rows

def timer_credit_cases(o,c):
    rows=[]
    for incoming_self in (False,True):
        o.fixture(c);hits=[]
        for n in range(24):hits.append(o.damage_call(200+n))
        o.w(o.options+0x1bc,0);o.w(o.options+0x1c0,5)
        o.writer_actual=True;o.skip_writer=False;o.stop_marker=True;o.counter_trace=[];o.corpse_trace=[];o.trace=[]
        o.phase='actual_selected_credit_death_writer';o.w(TIME+8,224);o.g.mem_write(EVENT,o.input_event_template)
        attacker=o.live_players[3]if incoming_self else o.live_players[0]
        owner=o.victim_unit if incoming_self else o.units['attacker0'][0]
        projectile=o.shot(attacker,owner,3 if incoming_self else 0)
        o.call(0xbd080,registers=((UC_X86_REG_EAX,projectile),(UC_X86_REG_ECX,EVENT+8)))
        o.stop_at=0xb9c20;o.call(0xd9180,(EVENT,o.victim_unit,NONE,NONE,NONE,0),end=0xb9c20);o.stop_at=None
        writer=next(e for e in o.corpse_trace if e['stage']=='death_writer')
        assert writer['arguments'][0]==f'{o.live_players[0]:08x}'
        assert writer['arguments'][1]==f'{o.live_players[3]:08x}'
        assert o.u(o.player_ptr(o.live_players[3])+0x164)==30 and o.u(o.player_ptr(o.live_players[3])+0x168)==0
        rows.append(dict(case='incoming_self_selected_enemy_timer'if incoming_self else'incoming_enemy_selected_enemy_timer',
            preceding_original_24_hits=hits,incoming_player=f'{attacker:08x}',incoming_self=incoming_self,
            selected_writer_attacker=writer['arguments'][0],writer_victim=writer['arguments'][1],
            options=dict(respawn_seconds=0,suicide_penalty_seconds=5),countdown=30,
            marker_boundary=o.marker_arguments(),after=o.corpse(),source_trace=list(o.corpse_trace),attribution_trace=list(o.trace),
            boundary='Actual fatal damage/selection/writer prefix executes; marker transform and later stats/event callbacks stop.'))
        o.phase='fixture'
    return rows

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'owner-counter-report.json');args=ap.parse_args()
    for name,pin in FROZEN.items():assert sha(OUT/name)==pin
    producer=json.loads((OUT/'smg-producers-report.json').read_text());event=bytes.fromhex(producer['stock_direct_impact_events'][0]['event_hex'])
    c=Cache(ROOT/'assets/disc/maps/lockout.map');assert sha(c.path)==CACHE_PIN;o=CounterOracle(c,event)
    result=dict(schema=1,source_sha256=VERSIONS['tu11']['pin'],cache_sha256=CACHE_PIN,oracle_sha256=sha(__file__),
        frozen_reports=FROZEN,instruction_patches=[],marker_pose_fixtures=[],
        admission_cases=admission_cases(o,c),timing_cases=timing_cases(o,c),cap_cases=cap_cases(o,c),
        ownership_cases=lifecycle_cases(o,c),AI_producer_cases=AI_producer_cases(o,c),timer_credit_cases=timer_credit_cases(o,c))
    result['case_count']=sum(len(v)for k,v in result.items()if k.endswith('_cases'))
    md=Cs(CS_ARCH_X86,CS_MODE_32);lines=[]
    for a,b in ((0xcba60,0xcbdc0),(0x1e2f50,0x1e2fc0),(0x15f680,0x15f6a0),(0x253f10,0x253f30),
        (0x1cb990,0x1cbb40),(0x1cb3d0,0x1cb510),(0x15e4d0,0x15e870),(0x14e240,0x14e3b0),(0xbd1f0,0xbd270)):
        lines.append(f'\n# untouched TU1.1 {a:08x}..{b:08x} exclusive')
        for i in md.disasm(bytes(o.g.mem_read(a,b-a)),a):lines.append(f'{i.address:08x} {i.bytes.hex():<28} {i.mnemonic} {i.op_str}')
    for a,n in ((0x465080,4),(0x447c20,0x70)):
        lines.append(f'original_data {a:08x} '+bytes(o.g.mem_read(a,n)).hex())
    lp=args.output.with_name('tu11-owner-counter-instructions.txt');lp.write_text('\n'.join(lines)+'\n');result['listing_sha256']=sha(lp)
    deps=('oracle_mp_spawn_corpse_schedule.py','oracle_mp_spawn_lethal_attribution.py','oracle_mp_spawn_datums.py',
        'oracle_mp_spawn_smg_producers.py','oracle_mp_spawn_smg_damage.py','oracle_mp_spawn_slayer_accounting.py',
        'oracle_mp_spawn_ownership.py','oracle_mp_spawn_lifecycle.py','oracle_mp_spawn.py')
    result['dependencies']={f'tools/{p}':sha(ROOT/'tools'/p)for p in deps}
    result['boundaries']=['Original salted pools/host identity/ownership/constructor prefixes execute; lower body arena and active registration are inherited supplied fixtures.',
        'Counter kernel inputs, timer/count states, invalid-reference and header-type variants are explicit supplied boundaries.',
        'Forwarding/current references are not claimed source-produced in invalid-reference cases; original unchecked preconditions are demonstrated.',
        'Actual multiplayer AI initializer1CB3D0 executes completely with source options state2 and poisoned prior state; full registered game-world bootstrap is not executed.',
        'Selected-credit timer cases use actual 24-hit enemy history then real fatal incoming self/enemy ownership; 15E4D0 stops before original marker transform.',
        'No prior suite changes, instruction patches, marker/head poses, inferred corpse deletion or production changes.']
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(cases=result['case_count'],output=str(args.output),sha256=sha(args.output))))

if __name__=='__main__':main()
