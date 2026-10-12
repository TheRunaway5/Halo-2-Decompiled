#!/usr/bin/env python3
"""Unchanged TU1.1 stock SMG event and local handicap/property producers.

Composes separately executed original producer bytes with the frozen original
amount kernel. Collision, lobby transport and live world registration remain
explicit boundaries. Earlier tools/reports are imported read-only.
"""
import argparse
import json
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_EDI,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_XMM0)
from oracle_mp_spawn import ROOT, VERSIONS, CACHE_PIN, Cache, PLAYERS, SESSION, CONFIG, SEED, ENGINE_GLOBALS
from oracle_mp_spawn_lifecycle import EXTRA
from oracle_mp_spawn_smg_damage import DamageOracle, EVENT, NONE, sha
from oracle_smg_impact_1_1 import execute_case as impact_case
from oracle_smg_projectile_1_1 import AllocationTrace, PI, DESCRIPTOR
from oracle_smg_firing_1_1 import (UNIT as FIRING_UNIT, PROJECTILE as FIRING_PROJECTILE,
    PLAYERS as FIRING_PLAYERS, PLAYER_DATA)

OUT=ROOT/'analysis/multiplayer/integration/spawn-source'
CREATION=EXTRA+0x204000
CONTROLLERS=0x556430


class ProducerOracle(DamageOracle):
    def __init__(self,c):
        self.producer_events=[]
        super().__init__(c)

    def hook(self,g,a,size,user):
        if a in (0x190fb5,0x19277d,0x1927dd,0x19163a,0x191c94,0x58a40,
                 0x1918d0,0x1917c6,0x154f40,0x64890,0x54f30,0x64900,
                 0x65dd0,0x5aeb0,0x14d800,0x14d600,0x14d3d0,0x159730,
                 0x1a0a90,0x15f520,0x1615f0,0x253f40,0x253f10):
            row=dict(entry=f'{a:08x}',eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',
                ecx=f'{g.reg_read(UC_X86_REG_ECX):08x}',
                arguments=[f'{self.u(g.reg_read(UC_X86_REG_ESP)+4+i*4):08x}'for i in range(3)])
            self.producer_events.append(row)
        super().hook(g,a,size,user)

    def reset_controllers(self):
        # Source clear must own the value: poison its whole destination first.
        self.g.mem_write(CONTROLLERS,b'\xa5'*0x3218)
        self.stop_at=0x19100b
        self.call(0x190fb5,end=0x19100b)
        self.stop_at=None
        rows=[dict(controller=i,record_hex=bytes(self.g.mem_read(CONTROLLERS+i*0xc74,0xc74)).hex(),
            handicap_dword=self.u(CONTROLLERS+i*0xc74+0x200),
            user_id=f'{self.u(CONTROLLERS+i*0xc74+4):08x}')for i in range(4)]
        assert all(r['handicap_dword']==0 and r['user_id']=='ffffffff'for r in rows)
        return rows

    def publish(self,controller,handicap=None):
        at=CONTROLLERS+controller*0xc74
        # Explicit controller activation ID; network join/sign-in are not run.
        self.w(at+4,controller)
        if handicap is not None:self.w(at+0x200,handicap)
        # The original copy of previously registered local properties is skipped
        # by its actual inactive-record predicate, not by a supplied service.
        self.g.mem_write(0x4d4e24+controller*0xd0,b'\x00')
        self.g.mem_write(0x4d0ed8,b'\x00') # voice runtime inactive
        self.call(0x1918d0,registers=((UC_X86_REG_EDI,controller),))
        raw=bytes(self.g.mem_read(0x4d4e38+controller*0xd0,0x90))
        assert raw[0x7d]==(0 if handicap is None else handicap&255)
        return raw

    def host_copy(self,raw,slot=0):
        self.g.mem_write(0x4d5188,bytes(0x130));self.w(0x4d517c,0)
        self.w(SESSION+0x741c,5);self.w(SESSION+0x14,1)
        self.w(SESSION+0x4db0,1);self.w(SESSION+0x111c,1<<slot)
        self.g.mem_write(SESSION+0x4df8,b'\x00')
        # Explicit transport boundary: unchanged produced local 90-byte packet
        # is delivered to the host membership request, without slot adaptation.
        self.g.mem_write(SESSION+0x1138+slot*0x13c,raw)
        self.call(0x65dd0,(SESSION,))
        result=bytes(self.g.mem_read(SESSION+0x11c8+slot*0x13c,0x90))
        assert result[0x7d]==raw[0x7d]
        return result

    def player_copy(self,raw,initial=False,slot=0):
        self.g.mem_write(CONFIG,raw)
        self.stop_at=0x14d687
        if initial:
            self.g.mem_write(CREATION,bytes(0xac))
            self.g.mem_write(CREATION+1,b'\x01') # original nonlocal branch
            self.sh(CREATION+2,-1);self.w(CREATION+4,NONE)
            self.g.mem_write(CREATION+0x1c,raw)
            self.call(0x14d800,(self.datums[slot],1,CREATION),end=0x14d687)
        else:
            self.call(0x14d600,(self.datums[slot],),((UC_X86_REG_EAX,CONFIG),),end=0x14d687)
        self.stop_at=None
        p=PLAYERS+slot*0x21c
        return dict(full_player=f'{self.datums[slot]:08x}',salt=self.h(p),
            effective_property_hex=bytes(self.g.mem_read(p+0x44,0x90)).hex(),
            requested_property_hex=bytes(self.g.mem_read(p+0xd4,0x90)).hex(),
            c0=self.g.mem_read(p+0xc0,1)[0],c1=self.g.mem_read(p+0xc1,1)[0],
            current=f'{self.u(p+0x2c):08x}',flags=f'{self.h(p+2)&0xffff:04x}',
            field194=self.u(p+0x194))


def reconcile_impact():
    suitepath=ROOT/'analysis/multiplayer/map-pack/smg-impact-suite/suite.json'
    suite=json.loads(suitepath.read_text());sources=[];rows=[]
    for mode in (1,5):
        for entry in (0,1):
            frozen=next(r for r in suite['cases']if r['name']==f'entry{entry}-shield1-body1'and r['mode']==mode)
            path=ROOT/frozen['report'];assert sha(path)==frozen['sha256']
            old=json.loads(path.read_text())
            new=impact_case(mode,1.,1.,entry,intercept=True)
            produced=next(e for e in new['impact_events']if e.get('entry')=='000d9180')
            prior=next(e for e in old['impact_events']if e.get('entry')=='000d9180')
            raw=bytes.fromhex(produced['event_before_hex'])
            assert raw.hex()==prior['event_before_hex']
            projectile=bytes.fromhex(new['before']['projectile_hex'])
            assert struct.unpack_from('<I',projectile,0x188)[0]==0x3f800000
            for off in (0x54,0x58,0x5c):assert struct.unpack_from('<I',raw,off)[0]==0x3f800000
            for off in (0x60,0x64,0x68):assert struct.unpack_from('<I',raw,off)[0]==0
            sources.append(dict(path=str(path.relative_to(ROOT)),sha256=sha(path)))
            rows.append(dict(mode=mode,material_entry=entry,event_hex=raw.hex(),
                projectile_before_hex=projectile.hex(),
                fields={f'{o:02x}':f'{struct.unpack_from("<I",raw,o)[0]:08x}'for o in (8,0xc,0x54,0x58,0x5c,0x60,0x64,0x68)},
                damage_call_stack=produced['stack'],collision_producer=new['collision_producer'],
                impact_events=new['impact_events'],supplied_services=new['supplied_services'],
                before=new['before'],after=new['after'],raw_event_matches_frozen=True))
    return dict(suite_path=str(suitepath.relative_to(ROOT)),suite_sha256=sha(suitepath),cases=sources),rows


def strength_cases(o,c,event_raw):
    rows=[]
    for controller in range(4):
        for handicap in (None,0,1,2,3,4,127,128,255):
            o.fixture(c)
            o.producer_events=[]
            reset=o.reset_controllers()
            published=o.publish(controller,handicap)
            host=o.host_copy(published)
            player=o.player_copy(host,initial=handicap is None)
            expected=0 if handicap is None or handicap>=128 else min(handicap,3)
            assert player['c1']==expected
            # Explicit composition boundary between separate original oracle
            # address spaces: keep original event amount fields verbatim, then
            # supply this fixture's full salted attacker/object/designator.
            o.g.mem_write(EVENT,event_raw)
            o.w(EVENT+8,o.datums[0]);o.w(EVENT+0xc,0x10001);o.sh(EVENT+0x10,0)
            before=bytes(o.g.mem_read(EVENT,0x88)).hex()
            seed_before=[f'{o.u(SEED+n):08x}'for n in (0,4)]
            shield,_=o.kernels()
            kernel=next(e for e in o.damage_events if e.get('entry')=='000da710')
            amount=kernel['raw_amount_bits']
            expected_bits={0:'40980000',1:'40640000',2:'40180000',3:'3f980000'}[expected]
            assert amount==expected_bits
            assert seed_before==[f'{o.u(SEED+n):08x}'for n in (0,4)]
            rows.append(dict(controller=controller,handicap_input=handicap,
                source_default=handicap is None,controller_reset=reset,
                published_local_property_hex=published.hex(),host_property_hex=host.hex(),
                player_after_copy=player,original_initializer_prefix=handicap is None,
                composed_event_hex=before,raw_damage_bits=amount,shield_after=shield,
                seed_before=seed_before,seed_after=[f'{o.u(SEED+n):08x}'for n in (0,4)],
                producer_events=o.producer_events,damage_events=o.damage_events))
    return rows


def shot_identity_cases():
    rows=[]
    for mode in (1,5):
        for salt in (7,8):
            t=AllocationTrace(mode,(1.,0.,0.),(0.,0.,0.))
            full=salt<<16
            t.w(FIRING_UNIT+0x13c,full)
            t.sh(FIRING_UNIT+0x138,0)
            t.sh(PLAYER_DATA,salt)
            t.w(FIRING_PLAYERS+0x38,1);t.w(FIRING_PLAYERS+0x24,0x21c)
            t.execute(1,0)
            request=next(e for e in t.events if e.get('entry')=='000b8a30')
            descriptor=bytes.fromhex(request['request_descriptor_hex'])
            created=bytes(t.g.mem_read(FIRING_PROJECTILE,428))
            assert struct.unpack_from('<I',descriptor,0x68)[0]==full
            assert struct.unpack_from('<I',descriptor,0x6c)[0]==0x10001
            assert struct.unpack_from('<I',created,0xc4)[0]==full
            assert struct.unpack_from('<I',created,0xc8)[0]==0x10001
            # Execute the actual owner-transfer function invoked by FEB40. Its
            # target is a source-initialized event; no slot re-encoding occurs.
            t.run(0xd7c60,(0xe59d0428,),regs=((UC_X86_REG_EDX,DESCRIPTOR),))
            t.run(0xbd080,regs=((UC_X86_REG_EAX,PI),(UC_X86_REG_ECX,DESCRIPTOR+8)))
            event=bytes(t.g.mem_read(DESCRIPTOR,0x88))
            assert struct.unpack_from('<I',event,8)[0]==full
            assert struct.unpack_from('<I',event,0xc)[0]==0x10001
            valid=t.run(0x16d560,regs=((UC_X86_REG_EAX,full),(UC_X86_REG_ESI,FIRING_PLAYERS)))
            assert t.g.reg_read(UC_X86_REG_EAX)==PLAYER_DATA
            t.sh(PLAYER_DATA,salt+1) # explicit departure/reuse state, not allocator proof
            t.run(0x16d560,regs=((UC_X86_REG_EAX,full),(UC_X86_REG_ESI,FIRING_PLAYERS)))
            stale=t.g.reg_read(UC_X86_REG_EAX)
            assert stale==0
            t.run(0xbd080,regs=((UC_X86_REG_EAX,PI),(UC_X86_REG_ECX,DESCRIPTOR+8)))
            retained=bytes(t.g.mem_read(DESCRIPTOR,0x88))
            assert event==retained
            rows.append(dict(mode=mode,player_datum=f'{full:08x}',unit_owner=f'{full:08x}',
                source_unit='00010001',membership=0,request_descriptor_hex=descriptor.hex(),
                created_projectile_hex=created.hex(),owner_event_hex=event.hex(),
                owner_event_after_slot_reuse_hex=retained.hex(),
                stale_player_lookup_pointer=f'{stale:08x}',new_player_salt=salt+1,
                source_events=t.events,
                boundaries='Owner/player pool runtime fields are supplied, full current datum and salt agree. Original firing/allocator/type callback/BD080/16D560 execute; lower allocation, marker and world services remain imported fixtures. Salt change is explicit reuse input, not execution of departure or allocation.'))
    return rows


def default_multiplier_cases(o,c):
    scenarios=(('default_enemy',0,0,0,False,False,False,'3f800000'),
        ('default_handicap3',3,0,0,False,False,False,'3e800000'),
        ('damage_trait',0,0x1000,0,False,False,False,'3fc00000'),
        ('resistance_trait',0,0x2000,0,False,False,False,'3f000000'),
        ('both_traits',0,0x3000,0,False,False,False,'3f400000'),
        ('positive_target194',0,0,1,False,False,False,'3dcccccd'),
        ('negative_target194',0,0,-1,False,False,False,'3f800000'),
        ('self_bypasses_handicap_traits194',3,0x3000,1,True,False,False,'3f800000'),
        ('no_attacker_bypasses_all',3,0x3000,1,False,True,False,'3f800000'),
        ('no_target_only_handicap',3,0x3000,1,False,False,True,'3e800000'),
        ('default_same_membership_bit7_set',0,0,0,False,False,False,'3f800000'),
        ('bit7_clear_same_membership',0,0,0,False,False,False,'00000000'),
        ('bit7_clear_different_membership',0,0,0,False,False,False,'3f800000'),
        ('engine_absent_bypasses_all',3,0x3000,1,False,False,False,'3f800000'))
    rows=[]
    for name,handicap,added,field194,self_hit,none_attacker,none_target,expected in scenarios:
        o.fixture(c);o.producer_events=[];o.reset_controllers()
        attacker_property=o.host_copy(o.publish(0,handicap))
        attacker=o.player_copy(attacker_property,initial=True)
        target_property=o.host_copy(o.publish(3),slot=3)
        target=o.player_copy(target_property,initial=True,slot=3)
        assert attacker['field194']==target['field194']==0
        # Only the optional localized variant name is omitted. The complete
        # original gameplay-default producer writes the actual inline variant.
        matg=o.u(o.a['globals']);o.w(o.a['globals'],0)
        o.call(0x1a0a90,(o.options+0x13c,0));o.w(o.a['globals'],matg)
        variant=bytes(o.g.mem_read(o.options+0x13c,0x130))
        assert o.u(o.options+0x180)==2 and o.u(o.options+0x184)==0xf9a
        flags=0xf9a|added
        if name.startswith('bit7_clear'):flags&=~0x80
        o.w(o.options+0x184,flags)
        o.w(PLAYERS+(0 if self_hit else 3)*0x21c+0x194,field194)
        if 'same_membership'in name:o.g.mem_write(PLAYERS+3*0x21c+0xc0,b'\x00')
        if name=='engine_absent_bypasses_all':o.w(o.a['engine_table']+8,0)
        a=NONE if none_attacker else o.datums[0]
        v=NONE if none_target else o.datums[0]if self_hit else o.datums[3]
        o.call(0x15f520,registers=((UC_X86_REG_ECX,a),(UC_X86_REG_EAX,v)))
        result=f'{o.g.reg_read(UC_X86_REG_XMM0)&NONE:08x}'
        assert result==expected,(name,result,expected)
        if name=='default_enemy':
            assert sum(e['entry']=='00253f40'for e in o.producer_events)==2
            assert not any(e['entry']=='00253f10'for e in o.producer_events)
        rows.append(dict(name=name,default_variant_hex=variant.hex(),default_options184='00000f9a',
            actual_options184=f'{flags:08x}',source_initialized_players=[attacker,target],
            actual_target194=field194,attacker_datum=f'{a:08x}',target_datum=f'{v:08x}',
            active_engine_pointer=f'{o.u(o.a["engine_table"]+8):08x}',
            current_memberships=[o.g.mem_read(PLAYERS+s*0x21c+0xc0,1)[0]for s in (0,3)],
            multiplier_bits=result,producer_events=o.producer_events,
            boundary='Complete original variant gameplay defaults and both player initializer prefixes execute; subsequent flag194/option/membership/engine mutations are explicit branch fixtures, not recovered runtime writers.'))
    return rows


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=OUT/'smg-producers-report.json')
    args=ap.parse_args();c=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
    sources,impacts=reconcile_impact()
    o=ProducerOracle(c);strength=strength_cases(o,c,bytes.fromhex(impacts[0]['event_hex']))
    identities=shot_identity_cases()
    defaults=default_multiplier_cases(o,c)
    listing=[];md=Cs(CS_ARCH_X86,CS_MODE_32)
    ranges=((0x190fb5,0x191010),(0x19277d,0x192918),(0x19163a,0x191694),
        (0x191c94,0x191cb7),(0x58a40,0x58a80),(0x1917c6,0x19191b),
        (0x154f40,0x154fa0),(0x64890,0x64928),(0x54f30,0x54f78),
        (0x65dd0,0x66170),(0x14d3d0,0x14d600),(0x14d600,0x14d6f0),
        (0x14d800,0x14da90),(0xd7c60,0xd7cc8),(0xfc2e0,0xfc640),
        (0xfeb40,0xfef70),(0xbd080,0xbd0c0),(0x15f520,0x15f6a0),
        (0x16d560,0x16d5d0),(0x108e62,0x108eea),(0xb8820,0xb8a30),
        (0xb8d0d,0xb8d2a),(0x1a0a90,0x1a0ec0),(0x1615f0,0x161614),
        (0x253f10,0x253f90))
    for a,b in ranges:
        listing.append(f'RANGE {a:08x}..{b:08x}')
        listing.extend(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in md.disasm(o.x.read(a,b-a),a))
    path=OUT/'tu11-smg-producers-instructions.txt';path.write_text('\n'.join(listing)+'\n')
    frozen=json.loads((OUT/'smg-damage-report.json').read_text())
    assert sha(ROOT/'tools/oracle_mp_spawn_smg_damage.py')==frozen['tool_sha256']
    assert sha(ROOT/frozen['instruction_listing'])==frozen['instruction_listing_sha256']
    for name,value in frozen['dependencies'].items():assert sha(ROOT/name)==value
    for name,value in frozen['frozen_prior_reports_sha256'].items():assert sha(ROOT/name)==value
    dependencies=('oracle_mp_spawn_smg_damage.py','oracle_smg_impact_1_1.py','oracle_smg_projectile_1_1.py')
    report=dict(scope=__doc__,tool_sha256=sha(__file__),executable_sha256=VERSIONS['tu11']['pin'],
        cache_sha256=CACHE_PIN,dependencies={str((ROOT/'tools'/n).relative_to(ROOT)):sha(ROOT/'tools'/n)for n in dependencies},
        preserved_smg_damage_report_sha256=sha(OUT/'smg-damage-report.json'),
        prior_frozen_suites_verified_unchanged=True,instruction_listing=str(path.relative_to(ROOT)),
        instruction_listing_sha256=sha(path),frozen_impact_sources=sources,
        stock_direct_impact_events=impacts,source_strength_cases=strength,source_shot_identity_cases=identities,
        original_default_slayer_multiplier_cases=defaults,
        summary=dict(executed_impact_events=len(impacts),event_frozen_byte_matches=len(impacts),
            local_controller_property_host_player_amount_cases=len(strength),
            original_default_initializer_prefixes=sum(x['source_default']for x in strength),
            full_datum_shot_owner_and_stale_lookup_cases=len(identities),
            actual_default_slayer_and_multiplier_gate_cases=len(defaults)),
        boundaries=[
            'Impact fixture collision/hit, current node poses and lower allocations are exactly the imported impact suite services; D9180 is intercepted only in this producer suite.',
            'Initial controller clear executes through 19100B; the final 24DBF6 UI reset tail is not executed. Controller activation/user ID is explicit input after source reset.',
            'Voice runtime is inactive; original54F30 takes its actual inactive branch. Local network property record is initially inactive, so original64890 does not supply saved properties.',
            'Produced local90-byte property is explicitly delivered unchanged into host1138 membership request; network transport and remote lobby join are not executed.',
            'The new-player14D800 path and property14D600 execute to14D687 immediately after sanitized effective property copy. Subsequent name/color/handicap notifications and user attachment are outside this prefix.',
            'Produced event is composed into the frozen damage oracle with explicit full runtime attacker00070000/sourceunit00010001/designator0. This is separate source kernel composition, not one continuous world simulation.',
            'The actual collision node/region fields from impact remain in the call report; amount-only composition uses frozen kernels and does not claim complete damage processing or material/world hit admission.'])
    args.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['summary']))


if __name__=='__main__':main()
