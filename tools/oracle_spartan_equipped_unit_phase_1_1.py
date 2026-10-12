#!/usr/bin/env python3
"""Independent original equipped normal unit-control wrapper execution.

The frozen request/clock suite remains unchanged. This follow-up executes the
actual C7E90 wrapper after original player input conversion, including its own
C7420 invocation, against the same explicitly preallocated inventory fixture.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
from oracle_spartan_equipped_animation_1_1 import (
    EquippedTrace, ROOT, PIN, CACHE_PIN, UNIT, UI, WEAPON, WI, TIME, SEED,
    CONTROL_INPUT, MANAGER_OFFSET, REQUEST, GAME, WORLD,
    UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_ESP,
)
from oracle_smg_firing_1_1 import STOP, Xbe, _Code, _trace

EQUIPPED_PIN='3cff76069e4770392b0cee0df2bafaec5a4def1ba4945493047cdb57977e7a65'
EXTRA_ENTRIES=(0xC7E90,0xC5D10,0xC7210,0xC7AA0,0x116420,0xC7CF0,0xC7B70,0xC6400,0xCF510,0xD2530)

class UnitPhaseTrace(EquippedTrace):
    def __init__(self):
        self.phase_events=[];self.phase_returns=[];self.phase_active=False
        super().__init__()

    def capture(self):
        return {'animation':self.selected(),'unit_object_400_hex':bytes(self.g.mem_read(UNIT,0x400)).hex(),
            'weapon':self.state()}

    def run(self,address,argv=(),regs=()):
        super().run(address,argv,regs)
        for ret,row in self.phase_returns[:]:
            if ret==STOP:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';row['after']=self.capture()
                self.phase_returns.remove((ret,row))

    def hook(self,g,a,size,user):
        if self.phase_active:
            sp=g.reg_read(UC_X86_REG_ESP)
            for ret,row in self.phase_returns[:]:
                if a==ret:
                    row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';row['after']=self.capture()
                    self.phase_returns.remove((ret,row))
            if a in EXTRA_ENTRIES:
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(4)],'before':self.capture()}
                self.phase_events.append(row);self.phase_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def control(self,flags=0,yaw=0.,pitch=0.,forward=0.,left=0.,buttons=0):
        # Exact same supplied input and original conversion as the frozen tool,
        # but aiming is invoked only by original C7E90 in the unit phase.
        self.g.mem_write(CONTROL_INPUT,bytes(0x5C))
        self.w(CONTROL_INPUT,flags);self.f(CONTROL_INPUT+4,yaw);self.f(CONTROL_INPUT+8,pitch)
        self.f(CONTROL_INPUT+0xC,forward);self.f(CONTROL_INPUT+0x10,left)
        self.f(CONTROL_INPUT+0x14,1. if flags&0x10000 else 0.)
        self.g.mem_write(CONTROL_INPUT+0x1E,bytes(self.g.mem_read(UNIT+0x214,4)))
        self.sh(CONTROL_INPUT+0x22,-1);self.sh(CONTROL_INPUT+0x24,-1);self.sh(CONTROL_INPUT+0x1C,buttons)
        supplied=bytes(self.g.mem_read(CONTROL_INPUT,0x5C)).hex()
        self.active='original_player_pressed_edges'
        self.run(0x150DD0,regs=((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,CONTROL_INPUT)))
        self.active='original_player_action_request_producer';self.run(0x150F00,(0,UI,CONTROL_INPUT))
        self.active='original_player_control_conversion';self.run(0x151C90,(UI,CONTROL_INPUT),((UC_X86_REG_EAX,0),))
        return {'supplied_input_record_hex':supplied,
            'after_original_action_input_hex':bytes(self.g.mem_read(CONTROL_INPUT,0x5C)).hex(),
            'converted':self.capture()}

    def execute_unit_phase(self,ticks=15,scenario='idle'):
        initial=self.initialize_equipped();self.phase_active=True;rows=[];m=UNIT+MANAGER_OFFSET
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);before=self.capture();start=len(self.phase_events)
            held=(scenario in ('fire','reload') and tick<45) or scenario=='empty_reload'
            flags=(0x10000 if held else 0)|(0x40000 if held and tick==0 else 0)
            control=self.control(flags,.5 if scenario=='aim' else 0.,.25 if scenario=='aim' else 0.,
                1. if scenario=='move_input' else 0.,0.,1 if scenario=='reload' and tick==45 else 0)
            self.active='original_manager_blend';self.run(0x1D0ED0,regs=((UC_X86_REG_ESI,m),))
            after_blend=self.capture();self.active='original_normal_unit_control_wrapper'
            # 10CDF0's inherited type callback pushes the unit datum and calls
            # descriptor+40 in EAX. C7E90 consumes stack argument1, RET4.
            self.run(0xC7E90,(UI,),((UC_X86_REG_EAX,0xC7E90),));after_unit=self.capture()
            self.active='original_weapon_tick';self.run(0x1021D0,(WI,));after_weapon=self.capture()
            self.active='original_action_dispatch';self.run(0xE7E60,(UI,));after_actions=self.capture()
            self.active='original_equipped_animation_tick';self.run(0x112E10,(UI,REQUEST));after_animation=self.capture()
            self.active='original_local_channels';self.pose_active=True;self.run(0xBDF80,(UI,));self.pose_active=False
            self.active='original_projectile_postpass';self.run(0x1019F0)
            rows.append({'tick':tick,'scenario':scenario,'supplied_held':held,'before':before,
                'control':control,'after_blend':after_blend,'after_normal_unit_wrapper':after_unit,
                'after_weapon':after_weapon,'after_actions':after_actions,'after_animation':after_animation,
                'after_pose_and_dispatch':self.capture(),
                'local_transforms_hex':bytes(self.g.mem_read(UNIT+0x3000,33*32)).hex(),
                'unit_wrapper_event_range':[start,len(self.phase_events)]})
        return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'equipped_dependency_sha256':EQUIPPED_PIN,'scenario':scenario,'initialization':initial,
            'options_state':self.u(GAME+8),'options_mode':self.u(GAME+12,1),'world_state':self.u(WORLD+8),
            'rows':rows,'unit_wrapper_events':self.phase_events,'request_events':self.equipped_events,
            'live_events':self.live_events,'weapon_events':self.events,
            'summary':{'ticks':len(rows),'wrapper_calls':sum(e['entry']=='000c7e90' for e in self.phase_events),
                'projectile_requests':sum(e.get('service')=='000b8a30' for e in self.events),
                'animation_sound_requests':sum(e.get('service')=='00179b90' for e in self.events)},
            'scope':'Complete original C7E90 control wrapper follows original player input conversion. Explicit preallocated unit/player/weapon and original resource bytes; normal world physics classification, device producer, full carried allocation, attachment, audio/effect/projectile allocation remain external boundaries.'}

def inventory(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    entries=EXTRA_ENTRIES+(0xC6C50,0xC7420,0xF1420,0x112270,0x10CDF0,0xBD360,0xB7E00);rows=[]
    for start in entries:
        f=_trace(code,start,starts|set(entries));inst=[code.at(a) for a in sorted(f.code)]
        p=directory/f'{start:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'start':f'{start:08x}','end_exclusive':f'{f.end:08x}','instruction_count':len(inst),
            'listing':str(p.relative_to(ROOT)),'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest(),
            'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'} for a,b,k in f.sites if k=='call']})
    def original_u(a):
        for section in x.sections:
            raw=x.section_bytes(section)
            if section.va<=a and a+4<=section.va+len(raw):return int.from_bytes(raw[a-section.va:a-section.va+4],'little')
        raise ValueError(hex(a))
    type_descriptor=original_u(0x46FAD0);parents=[]
    for n in range(16):
        p=original_u(type_descriptor+0x84+n*4)
        if not p:break
        parents.append({'descriptor':f'{p:08x}','callback_40':f'{original_u(p+0x40):08x}'})
    assert any(p['descriptor']=='0046ee50' and p['callback_40']=='000c7e90' for p in parents)
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'routines':rows,
        'original_type_dispatch':{'type_table':'0046fad0','type_index':0,
            'type_descriptor':f'{type_descriptor:08x}','inheritance_field':'84','callback_field':'40','parents':parents,
            'callback_call':'0010ce3a pushes unit; 0010ce3b calls EAX callback; C7E90 RET4'}},indent=2)+'\n')

def suite(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True);cases=[]
    for scenario,ticks in (('idle',75),('aim',12),('move_input',12),('fire',60),('reload',135),('empty_reload',225)):
        t=UnitPhaseTrace();r=t.execute_unit_phase(ticks,scenario);p=directory/f'{scenario}.json'
        p.write_text(json.dumps(r,indent=2)+'\n')
        old=json.loads((ROOT/f'analysis/multiplayer/map-pack/spartan-equipped-animation-suite/{scenario}.json').read_text())
        assert len(old['rows'])==len(r['rows'])
        comparison={'primary_manager_match':all(a['after_animation']['animation']['manager_hex']==b['after_animation']['manager_hex'] for a,b in zip(r['rows'],old['rows'])),
            'secondary_channel_match':all(a['after_animation']['animation']['secondary_channel_hex']==b['after_animation']['secondary_channel_hex'] for a,b in zip(r['rows'],old['rows'])),
            'local_transforms_match':all(a['local_transforms_hex']==b['local_transforms_hex'] for a,b in zip(r['rows'],old['rows']))}
        cases.append({'name':scenario,'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'summary':r['summary'],'prior_isolated_aim_fixture_comparison':comparison})
        print(json.dumps(cases[-1]),flush=True);del t,r,old
    (directory/'suite.json').write_text(json.dumps({'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'equipped_dependency_sha256':EQUIPPED_PIN,'cases':cases,
        'ticks':sum(c['summary']['ticks'] for c in cases)},indent=2)+'\n')

def main():
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_equipped_animation_1_1.py').read_bytes()).hexdigest()==EQUIPPED_PIN
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ticks',type=int,default=15)
    p.add_argument('--scenario',choices=('idle','fire','reload','empty_reload','aim','move_input'),default='idle')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/spartan-equipped-unit-phase-1.1-oracle.json')
    p.add_argument('--suite',type=Path);p.add_argument('--instruction-inventory',type=Path)
    args=p.parse_args()
    if args.instruction_inventory:inventory(args.instruction_inventory);return
    if args.suite:suite(args.suite);return
    t=UnitPhaseTrace()
    try:r=t.execute_unit_phase(args.ticks,args.scenario)
    except Exception:
        print('recent',[(hex(a),t.code.at(a).mnemonic,t.code.at(a).op_str) for a in t.recent]);raise
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'ticks':len(r['rows']),'wrapper_calls':sum(e['entry']=='000c7e90' for e in r['unit_wrapper_events'])}))

if __name__=='__main__':main()
