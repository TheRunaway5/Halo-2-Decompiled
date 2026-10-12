#!/usr/bin/env python3
"""Execute unchanged player/unit ownership paths with explicit object fixtures.

Complete original assignment and deletion callbacks run. Death cleanup runs from
its entry through assignment release; ragdoll state 3 skips animation services.
Respawn runs through the complete source function with supplied representation,
creation and loadout services; original chooser/placement/binding instructions
run. These services are explicit boundaries, not a complete damage/spawn world.
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
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX,
    UC_X86_REG_ESP, UC_X86_REG_EIP)
from oracle_mp_spawn import (ROOT, VERSIONS, Cache, CACHE_PIN, PLAYERS,
    BITMAP, HEADERS, OBJECTS, ENGINE_GLOBALS,
    SEED, MULG, SCENARIO, LIST)
from oracle_mp_spawn_lifecycle import LifecycleOracle, EXTRA, TIME, TABLE, CAPSULES, DEFINITION

GLOBALS, GARBAGE = EXTRA+0x12c000, EXTRA+0x12d000
ADDR = {
    'base':dict(body_depleted=0xd6800,body_depleted_stop=0xd6842,bind=0x14cad0,death=0xcaa60,death_stop=0xcab35,delete=0x152cf0,
        delete_tail=0x187a60,active=0xcbf60,status=0x152340,spawn=0x14f2e0,
        representation=0x14f040,create=0xb7b40,custom=0x15f330,loadout=0x15e970,
        garbage_global=0x4de2f4,player_globals=0x4e8c20,players_collect=0x23b8e0),
    'tu11':dict(body_depleted=0xd7e00,body_depleted_stop=0xd7e42,bind=0x14e240,death=0xcbdc0,death_stop=0xcbe95,delete=0x154440,
        delete_tail=0x189490,active=0xcd3b0,status=0x153a90,spawn=0x1509c0,
        representation=0x150780,create=0xb8a30,custom=0x1609d0,loadout=0x15ffd0,
        garbage_global=0x4e58c4,player_globals=0x4f01f0,players_collect=0x23d9e0),
}
NONE=0xffffffff


class OwnershipOracle(LifecycleOracle):
    def __init__(self,version):
        super().__init__(version)
        self.owner=ADDR[version]
        self.services={}
        self.events=[]
        self.writes=[]
        self.watch={}
        self.g.hook_add(unicorn.UC_HOOK_MEM_WRITE,self.write_hook)

    def hook(self,uc,address,size,unused):
        super().hook(uc,address,size,unused)
        if not hasattr(self,'owner'):return
        sp=uc.reg_read(UC_X86_REG_ESP)
        for name in ('bind','active','status'):
            if address==self.owner[name]:
                row=dict(event=name,instruction=f'{address:08x}')
                if name=='bind':row.update(player=f'{self.u(sp+4):08x}',new_unit=f'{uc.reg_read(UC_X86_REG_EBX):08x}')
                if name=='active':row.update(unit=f'{self.u(sp+4):08x}',requested_active=self.u(sp+8))
                self.events.append(row)
        if address in self.services:
            name,argc,result=self.services[address]
            row=dict(event='supplied_service',name=name,entry=f'{address:08x}',
                arguments=[f'{self.u(sp+4+i*4):08x}' for i in range(argc)])
            if name=='representation':
                self.w(self.u(sp+4),-1)
                self.w(uc.reg_read(UC_X86_REG_EBX),self.spawn_tag)
                row.update(supplied_definition=f'{self.spawn_tag:08x}',supplied_variant='ffffffff')
            if name=='create':
                p=self.u(sp+4)
                row.update(placement_bytes=bytes(self.g.mem_read(p,0xc4)).hex(),
                    supplied_result=f'{result:08x}')
            self.events.append(row)
            if result is not None:uc.reg_write(UC_X86_REG_EAX,result)
            uc.reg_write(UC_X86_REG_EIP,self.u(sp))
            uc.reg_write(UC_X86_REG_ESP,sp+4+argc*4)

    def write_hook(self,uc,access,address,size,value,unused):
        if address in self.watch:
            row=dict(field=self.watch[address],size=size,value=f'{value&((1<<(size*8))-1):0{size*2}x}',
                instruction=f'{uc.reg_read(UC_X86_REG_EIP):08x}')
            self.writes.append(row);self.events.append(dict(event='write',**row))

    def fixture(self,cache,slot=0):
        super().fixture(cache)
        self.w(self.owner['player_globals'],GLOBALS)
        self.w(self.owner['garbage_global'],GARBAGE)
        self.w(GARBAGE+8,-1);self.w(GLOBALS+0xac,-1)
        self.w(BITMAP,1<<slot)
        for i in range(16):
            p=PLAYERS+i*0x21c
            self.sh(p+0x28,-1);self.w(p+0x30,-1);self.w(p+0x34,-1);self.w(p+0x174,-1)
        for i in range(3):
            p=OBJECTS+i*0x800
            self.g.mem_write(HEADERS+i*12,struct.pack('<HBBHHI',1,1,0,0xffff,0x800,p))
            self.w(p,1)
            for off in (0xc,0x10,0x14,0x12c,0x130,0x13c,0x140,0x248,0x24c,0xd4):self.w(p+off,-1)
            for off in range(0x218,0x228,4):self.w(p+off,-1)
            self.g.mem_write(p+0x3dc,b'\x03')
        p=PLAYERS+slot*0x21c
        self.g.mem_write(p+2,b'\x09')
        self.w(p+0x38,0x12345678);self.sh(p+0x3c,123);self.sh(p+0x17c,456);self.sh(p+0x17e,789)
        self.w(p+0x164,150)
        history=ENGINE_GLOBALS+0x558+slot*0x18
        self.g.mem_write(history,b'\x01');self.sh(history+0x10,150);self.sh(history+0x12,149)
        for n,v in enumerate((3.,4.,5.)):self.f(history+4+n*4,v)
        self.slot=slot
        self.services={};self.events=[];self.writes=[]
        self.watch={p+off:name for off,name in ((0x2c,'player.current'),(0x30,'player.previous'),
            (0x38,'player.latched_buttons'),(0x3c,'player.latched_flags'),(0x17c,'player.17c'),
            (0x17e,'player.17e'),(0x218,'player.spawn_count'),(2,'player.flags'),(0x194,'player.194'))}
        for i in range(3):
            self.watch.update({OBJECTS+i*0x800+off:f'unit{i}.{name}' for off,name in
                ((0x13c,'current_player'),(0x140,'previous_player'),(0x134,'flags134'),(0x10a,'damage_flags'))})

    def state(self):
        p=PLAYERS+self.slot*0x21c
        return dict(current=f'{self.u(p+0x2c):08x}',previous=f'{self.u(p+0x30):08x}',
            player_flags=f'{self.h(p+2)&0xffff:04x}',spawn_count=self.h(p+0x218),
            latched_buttons=f'{self.u(p+0x38):08x}',latched_flags=self.h(p+0x3c),
            field17c=self.h(p+0x17c),field17e=self.h(p+0x17e),
            live_countdown=self.u(p+0x164),history=self.history(self.slot),
            units=[dict(current_player=f'{self.u(OBJECTS+i*0x800+0x13c):08x}',
                previous_player=f'{self.u(OBJECTS+i*0x800+0x140):08x}',
                flags134=f'{self.u(OBJECTS+i*0x800+0x134):08x}',
                dead=bool(self.g.mem_read(OBJECTS+i*0x800+0x10a,1)[0]&4)) for i in range(3)])

    def assign_cases(self,cache):
        rows=[]
        for slot in (0,3,15):
            for case,old,new,dead in [('none_noop',NONE,NONE,False),('same_noop',0x10000,0x10000,False),
                ('release_alive',0x10000,NONE,False),('release_dead',0x10000,NONE,True),
                ('new_assignment',NONE,0x10001,False),('dead_new_assignment',NONE,0x10001,True),
                ('replace',0x10000,0x10001,False)]:
                self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
                self.sh(p,7);self.w(p+0x2c,old);self.w(p+0x30,0x10002)
                if old!=NONE:self.w(OBJECTS+0x13c,datum)
                self.g.mem_write(OBJECTS+(0 if new==NONE else 0x800)+0x10a,bytes([4 if dead else 0]))
                before=self.state()
                self.call(self.owner['bind'],(datum,),[(UC_X86_REG_EBX,new)])
                after=self.state()
                assert after['current']==f'{new:08x}'
                assert after['history']==before['history'] and after['live_countdown']==150
                if old==new:assert before==after and not self.writes
                else:
                    assert after['latched_buttons']=='00000000' and after['latched_flags']==after['field17c']==after['field17e']==0
                    if old!=NONE:assert after['units'][0]['current_player']=='ffffffff' and after['units'][0]['previous_player']==f'{datum:08x}'
                    if new!=NONE:
                        assert after['previous']=='ffffffff'
                        assert after['units'][1]['current_player']==f'{datum:08x}' and after['units'][1]['previous_player']=='ffffffff'
                    else:assert after['previous']==before['previous']
                rows.append(dict(case=case,slot=slot,player=f'{datum:08x}',new_unit=f'{new:08x}',
                    before=before,after=after,events=self.events,service_boundaries=[]))
        return rows

    def body_depleted_cases(self,cache):
        rows=[]
        for slot in (0,3,15):
            self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
            self.sh(p,7);self.w(p+0x2c,0x10000);self.w(OBJECTS+0x13c,datum)
            before=self.state()
            self.call(self.owner['body_depleted'],(0x10000,0,0,0),end=self.owner['body_depleted_stop'])
            after=self.state()
            expected=json.loads(json.dumps(before));expected['units'][0]['dead']=True
            assert after==expected
            rows.append(dict(slot=slot,before=before,after=after,events=self.events,
                prefix_start=f'{self.owner["body_depleted"]:08x}',
                prefix_stop=f'{self.owner["body_depleted_stop"]:08x}',
                service_boundaries=['Stops immediately before body-depleted animation notification.']))
        return rows

    def death_cases(self,cache):
        rows=[]
        for slot in (0,3,15):
            for owned in (False,True):
                self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
                self.sh(p,7);self.w(p+0x2c,0x10000)
                self.w(OBJECTS+0x13c,datum if owned else -1)
                self.g.mem_write(OBJECTS+0x10a,b'\x04')
                before=self.state()
                self.call(self.owner['death'],(0x10000,NONE,0,0,0),end=self.owner['death_stop'])
                after=self.state()
                assert after['current']==('ffffffff' if owned else '00010000')
                assert after['previous']==('00010000' if owned else 'ffffffff')
                assert after['history']==before['history']
                if owned:
                    assert after['units'][0]['previous_player']==f'{datum:08x}'
                    assert next(e for e in self.events if e['event']=='bind')['new_unit']=='ffffffff'
                assert self.u(OBJECTS+0x20)==500 and self.u(GARBAGE+8)==0x10000
                rows.append(dict(slot=slot,unit_has_owner=owned,before=before,after=after,
                    events=self.events,prefix_start=f'{self.owner["death"]:08x}',
                    prefix_stop=f'{self.owner["death_stop"]:08x}',
                    fixtures=dict(unit_dead_bit=True,biped_ragdoll_state=3,local_user=-1,
                        damage_point=None,knocked=False),service_boundaries=[]))
        return rows

    def history_admission_cases(self,cache):
        rows=[]
        for slot in (0,3,15):
            self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
            self.sh(p,7);self.w(p+0x2c,0x10000);self.w(OBJECTS+0x13c,datum)
            self.g.mem_write(OBJECTS+0x10a,b'\x04')
            self.w(BITMAP,(1<<slot)|1)
            spawning=datum if slot==0 else 0x10000
            states=[]
            for stage in ('dead_still_owned','after_original_death_release','after_history_expiration'):
                if stage=='after_original_death_release':
                    self.call(self.owner['death'],(0x10000,NONE,0,0,0),end=self.owner['death_stop'])
                if stage=='after_history_expiration':
                    self.sh(ENGINE_GLOBALS+0x558+slot*0x18+0x12,1)
                    self.call(self.extra['tick'],registers=[(UC_X86_REG_EAX,datum)])
                self.w(LIST,0)
                self.call(self.owner['players_collect'],(spawning,LIST))
                influences=[self.floats(LIST+4+n*32,8) for n in range(self.u(LIST))]
                expected=stage!='after_history_expiration' and (slot==0 or stage=='after_original_death_release')
                assert len(influences)==int(expected),(self.version,slot,stage,influences)
                if influences:assert influences[0][:3]==[3.,4.,5.]
                states.append(dict(stage=stage,current=f'{self.u(p+0x2c):08x}',
                    previous=f'{self.u(p+0x30):08x}',history=self.history(slot),influences=influences))
            rows.append(dict(slot=slot,spawning_player=f'{spawning:08x}',victim=f'{datum:08x}',
                own_history=slot==0,stages=states,events=self.events,service_boundaries=[]))
        return rows

    def delete_cases(self,cache):
        rows=[]
        for slot in (0,3,15):
            for case,current,previous,target in [('current',0x10000,NONE,0x10000),
                ('corpse',NONE,0x10000,0x10000),('unrelated',0x10001,0x10002,0x10000)]:
                self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
                self.sh(p,7);self.w(p+0x2c,current);self.w(p+0x30,previous);self.w(p+0x174,target)
                if current!=NONE:self.w(OBJECTS+(current&0xffff)*0x800+0x13c,datum)
                before=self.state()
                self.services[self.owner['delete_tail']]=('object_deleted_control_tail',0,None)
                self.call(self.owner['delete'],(target,))
                after=self.state()
                assert after['current']==('ffffffff' if current==target else before['current'])
                assert after['previous']==('ffffffff' if current==target or previous==target else before['previous'])
                assert self.u(p+0x174)==NONE and after['history']==before['history']
                rows.append(dict(case=case,slot=slot,before=before,after=after,events=self.events,
                    service_boundaries=['Final object-deleted control notification is supplied as no-op.']))
        return rows

    def respawn_cases(self,cache):
        rows=[]
        tag=next(t for t in cache.tags.values() if t['group']=='bipd' and t['name'].endswith('masterchief_mp'))
        for slot in (0,3,15):
            for case in ('success','create_failure','no_starts','no_definition'):
                self.fixture(cache,slot);p=PLAYERS+slot*0x21c;datum=0x70000|slot
                self.sh(p,7);self.w(p+0x30,0x10000);self.w(OBJECTS+0x140,datum)
                self.w(self.a['tags'],TABLE)
                self.w(TABLE+(int(tag['datum'],16)&0xffff)*16+8,DEFINITION)
                self.g.mem_write(DEFINITION,bytes(cache.data[tag['offset']:tag['offset']+0x340]))
                # Preserve original runtime/constants lookup fixture in the relocated real tag table.
                self.w(TABLE+8,MULG)
                qs=cache.block(tag['offset']+0x28c,0x50)
                self.g.mem_write(CAPSULES,b''.join(bytes(cache.data[q:q+0x50]) for q in qs))
                self.w(DEFINITION+0x290,CAPSULES)
                # Created/released objects use real biped definition; node state is unused by binding.
                for i in range(3):self.w(OBJECTS+i*0x800,int(tag['datum'],16))
                self.spawn_tag=NONE if case=='no_definition' else int(tag['datum'],16)
                if case=='no_starts':self.w(SCENARIO+0x100,0)
                self.shape_hit=False
                self.services={self.owner['representation']:('representation',1,None),
                    self.owner['create']:('create',1,NONE if case=='create_failure' else 0x10001),
                    self.owner['custom']:('customization',0,0),self.owner['loadout']:('loadout',0,None)}
                before=self.state()
                result=self.call(self.owner['spawn'],(datum,))&255
                after=self.state()
                assert result==int(case=='success'),(self.version,slot,case,result)
                assert after['history']==before['history'] and after['live_countdown']==150
                assert after['spawn_count']==int(case!='no_starts')
                if case=='success':
                    assert after['current']=='00010001' and after['previous']=='ffffffff'
                    assert int(after['player_flags'],16)&8==0
                else:
                    assert after['current']==before['current'] and after['previous']==before['previous']
                    assert int(after['player_flags'],16)&8
                creation=[e for e in self.events if e.get('name')=='create']
                assert len(creation)==int(case in ('success','create_failure'))
                rows.append(dict(case=case,slot=slot,before=before,after=after,result=result,
                    events=self.events,shape_requests=self.shape_requests,
                    weapon_rng_after=f'{self.u(SEED):08x}',spawn_rng_after=f'{self.u(SEED+4):08x}',
                    service_boundaries=['Representation lookup supplies real cache biped datum and variant NONE.',
                        'World shape query supplies no contact; original helper constructs query.',
                        'Object creation supplies replacement datum or NONE; it does not create a Havok body.',
                        'Customization returns false and loadout service is no-op.']))
        return rows


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/integration/spawn-source/ownership-report.json')
    args=parser.parse_args()
    cache=Cache(ROOT/'assets/disc/maps/lockout.map')
    assert hashlib.sha256(cache.data).hexdigest()==CACHE_PIN
    oracles={v:OwnershipOracle(v) for v in VERSIONS}
    rows={v:dict(assignments=o.assign_cases(cache),body_depleted_prefixes=o.body_depleted_cases(cache),
        death_prefixes=o.death_cases(cache),
        history_admission=o.history_admission_cases(cache),
        deletion_callbacks=o.delete_cases(cache),respawns=o.respawn_cases(cache)) for v,o in oracles.items()}
    def clean(x):
        if isinstance(x,dict):return {k:clean(v) for k,v in x.items() if k not in
            ('instruction','entry','prefix_start','prefix_stop')}
        if isinstance(x,list):return [clean(v) for v in x]
        return x
    for kind in rows['base']:assert clean(rows['base'][kind])==clean(rows['tu11'][kind]),kind
    report=dict(passed=True,oracle_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        dependencies=dict(python=sys.version,unicorn=unicorn.__version__,capstone=capstone.__version__),
        imported_tool_sha256={n:hashlib.sha256((ROOT/f'tools/{n}.py').read_bytes()).hexdigest()
            for n in ('oracle_mp_spawn','oracle_mp_spawn_lifecycle')},
        authorities={v:dict(path=a['path'],sha256=a['pin']) for v,a in VERSIONS.items()},
        cache=dict(path='assets/disc/maps/lockout.map',sha256=CACHE_PIN),
        scope=__doc__,fixture_cpu_state=dict(x87='027f',mxcsr='00001f80'),
        case_counts={k:sum(len(r[k]) for r in rows.values()) for k in rows['base']},rows=rows)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    md=Cs(CS_ARCH_X86,CS_MODE_32)
    ranges={'base':[(0xd6800,0xd6860),(0xcaa60,0xcab40),(0x14cad0,0x14cc91),(0xcbf60,0xcc010),
        (0x152cf0,0x152de2),(0x14f2e0,0x14f6f0)],
        'tu11':[(0xd7e00,0xd7e60),(0xcbdc0,0xcbea0),(0x14e240,0x14e3e0),(0xcd3b0,0xcd456),
        (0x154440,0x154532),(0x1509c0,0x150dd0),(0xdac40,0xdb160),
        (0xcb410,0xcbdc0),(0x1ed960,0x1ee130),(0x15f3a0,0x15f500),
        (0x14eae0,0x14ed20)]}
    for v,blocks in ranges.items():
        o=oracles[v];lines=[f'{v} {o.a["pin"]}']
        for start,end in blocks:
            data=o.x.read(start,end-start)
            lines.append(f'\nRANGE {start:08x}..{end:08x} SHA256 {hashlib.sha256(data).hexdigest()}')
            lines.extend(f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in md.disasm(data,start))
        (args.output.parent/f'{v}-ownership-instructions.txt').write_text('\n'.join(lines)+'\n')
    args.output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    print(json.dumps(dict(passed=True,case_counts=report['case_counts'])))


if __name__=='__main__':main()
