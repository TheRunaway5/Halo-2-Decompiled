#!/usr/bin/env python3
"""Independent original first-person weapon registration / EFCC0 hand feedback.

Frozen firing/allocated suites are imported without modification. Controls
explicitly vary only the named registration or ammunition boundary.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import (Trace,ROOT,PIN,CACHE_PIN,UNIT,WEAPON,WI,FPS,USERS,PLAYER_DATA,
    STOP,UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_smg_constructor_equipped_1_1 import ConstructorTrace

CTOR_PIN='1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99'
BARE_PIN='1ed4cdb19b7370b0349a62fb9fc796d5044f9b4125684e30546925e8ffba3b96'
ENTRIES=(0x10A140,0x167D32,0xEFCC0,0x16A356)
OUT=ROOT/'analysis/multiplayer/map-pack/smg-hand-feedback'

class Observe:
    def __init__(self,*args,**kwargs):
        self.feedback_calls=[];self.feedback_returns=[];self.hand_writes=[]
        super().__init__(*args,**kwargs)

    def binding(self):
        return {'fp_global_pointer':f'{self.u(0x4F1198):08x}',
            'fp_weapon_datums':[[f'{self.u(FPS+0x10+view*0x20CC+hand*0x1010):08x}' for hand in range(2)] for view in range(4)],
            'user_player_datums':[f'{self.u(USERS+0xC+view*4):08x}' for view in range(4)],
            'player_current_unit_2c':f'{self.u(PLAYER_DATA+0x2C):08x}',
            'unit_player_d4':f'{self.u(UNIT+0xD4):08x}',
            'player_local_view_28':struct.unpack('<h',self.g.mem_read(PLAYER_DATA+0x28,2))[0],
            'unit_current_primary_index_212':self.u(UNIT+0x212,1),
            'unit_current_primary_datum':f'{self.u(UNIT+0x218):08x}',
            'unit_hand_timers':[self.u(UNIT+0x1FA,1),self.u(UNIT+0x1FB,1)]}

    def finish_feedback(self,address):
        for ret,row in self.feedback_returns[:]:
            if ret!=address:continue
            row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';row['after']=self.binding()
            self.feedback_returns.remove((ret,row))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs);self.finish_feedback(STOP)

    def hook(self,g,a,size,user):
        self.finish_feedback(a)
        if a in ENTRIES:
            sp=g.reg_read(UC_X86_REG_ESP)
            row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                'return_address':f'{self.u(sp):08x}','registers':self.registers(),
                'stack':[f'{self.u(sp+4+n*4):08x}' for n in range(3)],'before':self.binding()}
            self.feedback_calls.append(row);self.feedback_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if UNIT+0x1FA<=address and address+size<=UNIT+0x1FC:
            self.hand_writes.append({'tick':self.tick,'phase':self.active,
                'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','offset':f'{address-UNIT:03x}',
                'before_hex':bytes(g.mem_read(address,size)).hex(),
                'after_hex':int(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
        super().mem_write(g,access,address,size,value,user)

class Bare(Observe,Trace):pass
class Attached(Observe,ConstructorTrace):pass

def one(kind):
    t=Attached() if kind.startswith('attached') else Bare()
    datum=t.initial_datum if isinstance(t,Attached) else WI
    p=t.initial_pointer if isinstance(t,Attached) else WEAPON
    supplied=[]
    if kind=='bare_registration_cleared':
        supplied.append({'address':f'{FPS+0x10:08x}','field':'first-person view0/hand0 weapon datum',
            'before':f'{t.u(FPS+0x10):08x}','after':'ffffffff'});t.w(FPS+0x10,-1)
    if kind in ('attached_registration_supplied','attached_registration_empty'):
        supplied.append({'address':f'{FPS+0x10:08x}','field':'first-person view0/hand0 weapon datum',
            'before':f'{t.u(FPS+0x10):08x}','after':f'{datum:08x}'});t.w(FPS+0x10,datum)
    if kind in ('bare_empty','attached_registration_empty'):
        supplied.extend([{'address':f'{p+off:08x}','field':name,'after':0} for off,name in ((0x22A,'reserve'),(0x22C,'loaded'))])
        if isinstance(t,Attached):t.sh(p+0x22A,0);t.sh(p+0x22C,0)
    initial=t.binding();calls_start=len(t.feedback_calls);writes_start=len(t.hand_writes)
    if isinstance(t,Attached):
        r=t.execute_initial(2,'held')
        rows=[{'tick':row['tick'],'phases':{phase:{k:row[phase]['weapon'][k] for k in ('loaded','reserve','unit_hand_timers','weapon_state_u32')}
            for phase in ('before','after_unit','after_weapon','after_postpass')},
            'requests':len(row['projectile_requests'])} for row in r['rows']]
    else:
        r=t.execute(hold_ticks=2,release_ticks=0,loaded_ammo=0 if kind=='bare_empty' else None,
                    reserve_ammo=0 if kind=='bare_empty' else None)
        rows=[{'tick':row['tick'],'phases':{phase:{k:row[source][k] for k in ('loaded','reserve','hand_timers','weapon_state')}
            for phase,source in (('before','tick_start'),('after_unit','before'),('after_weapon','after'),('after_postpass','post_dispatch'))}}
            for row in r['rows']]
    return {'name':kind,'datum':f'{datum:08x}','supplied_boundary_changes':supplied,
        'initial_binding':initial,'calls':t.feedback_calls[calls_start:],'hand_writes':t.hand_writes[writes_start:],
        'rows':rows,'final_binding':t.binding(),'source_summary':r['summary'],
        'scope':'Only two original firing ticks are observed. A supplied registration control is not proof of its original first-person producer or presentation clock.'}

def frozen_context_audit():
    base=ROOT/'analysis/multiplayer/map-pack';manifest=base/'smg-constructor-equipped-suite/suite.json'
    digest=hashlib.sha256(manifest.read_bytes()).hexdigest()
    assert digest=='54dad96620c0ee23b2e4be3086984a4f2d0696cdd2b07e1ec8ec1cb91119a764'
    suite=json.loads(manifest.read_text());out=[]
    for case in suite['cases']:
        p=ROOT/case['path'];assert hashlib.sha256(p.read_bytes()).hexdigest()==case['sha256']
        r=json.loads(p.read_text())
        calls=[e for e in r['weapon_events'] if e.get('service')=='0016a356' and
            len(e.get('stack_arguments',[]))==3 and e['stack_arguments'][1]=='00010002']
        views=sorted(set(e['stack_arguments'][0] for e in calls));assert views in ([],['ffffffff'])
        count=sum(row[phase]['weapon']['unit_hand_timers']==[0,0] for row in r['rows']
            for phase in ('before','after_unit','after_weapon','after_animation','after_postpass'))
        assert count==case['ticks']*5
        out.append({'case':case['name'],'source_sha256':case['sha256'],
            'first_person_notifications':len(calls),'observed_view_arguments':views,
            'zero_hand_timer_phase_snapshots':count})
    return {'source_manifest_sha256':digest,'cases':out,'ticks':599,
        'first_person_notifications':sum(r['first_person_notifications'] for r in out),
        'zero_hand_timer_phase_snapshots':sum(r['zero_hand_timer_phase_snapshots'] for r in out)}

def main():
    assert hashlib.sha256((ROOT/'tools/oracle_smg_constructor_equipped_1_1.py').read_bytes()).hexdigest()==CTOR_PIN
    assert hashlib.sha256((ROOT/'tools/oracle_smg_firing_1_1.py').read_bytes()).hexdigest()==BARE_PIN
    cases=[one(name) for name in ('bare','bare_registration_cleared','bare_empty','attached',
                                'attached_registration_supplied','attached_registration_empty')]
    OUT.mkdir(parents=True,exist_ok=True)
    frozen_bare=ROOT/'analysis/multiplayer/map-pack/smg-firing-suite/held_x-mode1.json'
    frozen_attached=ROOT/'analysis/multiplayer/map-pack/smg-constructor-equipped-source-fixture.json'
    assert hashlib.sha256(frozen_bare.read_bytes()).hexdigest()=='6a0a7650aac87d41726022dda1cd1c46651c09b3998af6592f80e8e40dbd8de8'
    assert hashlib.sha256(frozen_attached.read_bytes()).hexdigest()=='8792cebe9429427e9d026dacea2adcee53deba967c18a0ba9c2b3d39fe6597b6'
    old=json.loads(frozen_bare.read_text());new=json.loads(frozen_attached.read_text())
    attached_case=next(c for c in new['cases'] if c['name']=='held')
    for i in range(2):
        bare=cases[0]['rows'][i]['phases']['after_weapon'];expected=old['rows'][i]['after']
        assert all(bare[k]==expected[k] for k in bare)
        attached=cases[3]['rows'][i]['phases']['after_weapon'];expected=attached_case['rows'][i]['phases']['after_weapon']
        assert all(attached[k]==expected[k] for k in attached)
    context=frozen_context_audit()
    audit=OUT/'frozen-context-audit.json';audit.write_text(json.dumps(context,indent=2)+'\n')
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'constructor_dependency_sha256':CTOR_PIN,'bare_dependency_sha256':BARE_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cases':cases,
        'frozen_reference_checks':{'bare_after_weapon_ticks_0_1':True,'attached_after_weapon_ticks_0_1':True,
            'bare_report_sha256':'6a0a7650aac87d41726022dda1cd1c46651c09b3998af6592f80e8e40dbd8de8',
            'attached_source_fixture_sha256':'8792cebe9429427e9d026dacea2adcee53deba967c18a0ba9c2b3d39fe6597b6'},
        'frozen_context_audit_sha256':hashlib.sha256(audit.read_bytes()).hexdigest(),
        'native_gameplay_update_applied':False}
    p=OUT/'report.json';p.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'output':str(p),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
        'cases':[{'name':c['name'],'final_hand_timers':c['final_binding']['unit_hand_timers'],
                  'feedback_calls':len(c['calls'])} for c in cases]}))

if __name__=='__main__':main()
