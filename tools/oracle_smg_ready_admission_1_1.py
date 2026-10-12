#!/usr/bin/env python3
"""Independent original equipped-SMG ready/event/countdown admission evidence.

Preserves the frozen equipped/control suites. Executes original ready lookup,
clock conversion, unit wrapper, weapon countdown and projectile postpass.
Explicit storage/world/resource services are inherited and reported.
"""
import argparse, csv, hashlib, json, struct
from pathlib import Path
from oracle_spartan_equipped_unit_phase_1_1 import (
    UnitPhaseTrace, ROOT, PIN, CACHE_PIN, UNIT, UI, WEAPON, WI, TIME,
    SEED, MANAGER_OFFSET, REQUEST, UC_X86_REG_EAX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_ESP, Xbe, _Code, _trace)
from oracle_smg_firing_1_1 import Cache, STOP, PLAYER_DATA, UC_X86_REG_EDI, UC_X86_REG_EIP

DEPENDENCY_PIN='cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38'
ENTRIES=(0x1040F0,0x16A06A,0x169F3B,0x11CF00,0x1CF470,0x1DE7A0,
         0x1DE810,0x148160,0x112270,0x1045E0,0xCD310)

class ReadyTrace(UnitPhaseTrace):
    def __init__(self):
        self.ready_events=[];self.ready_returns=[];self.ready_active=False
        super().__init__();self.ready_active=True

    def state(self):
        out=super().state()
        out.update(blocking_ticks=struct.unpack('<h',self.g.mem_read(WEAPON+0x17C,2))[0],
            weapon_16c_1a4_hex=bytes(self.g.mem_read(WEAPON+0x16C,0x38)).hex(),
            hand_and_grenade_1f0_20c_hex=bytes(self.g.mem_read(UNIT+0x1F0,0x1C)).hex())
        return out

    def record(self,p):
        if not p:return None
        raw=bytes(self.g.mem_read(p,0x6C));n=self.u(p+0x4C);events=self.u(p+0x50)
        return {'pointer':f'{p:08x}','record_hex':raw.hex(),
            'name_string_id':f'{self.u(p):08x}','frame_count':self.u(p+0x14,2),
            'events':[{'type':self.u(events+i*4,2),'frame':self.u(events+i*4+2,2),
                'raw_hex':bytes(self.g.mem_read(events+i*4,4)).hex()} for i in range(n)]}

    def complete(self,row):
        row['returned_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}';row['after']=self.state()
        if row['entry'] in ('00169f3b','001cf470'):
            row['original_selected_record']=self.record(self.g.reg_read(UC_X86_REG_EAX))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs)
        for ret,row in self.ready_returns[:]:
            if ret==STOP:self.complete(row);self.ready_returns.remove((ret,row))

    def hook(self,g,a,size,user):
        if self.ready_active:
            for ret,row in self.ready_returns[:]:
                if a==ret:self.complete(row);self.ready_returns.remove((ret,row))
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP)
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                    'registers':self.registers(),'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(3)],
                    'clock_rate':self.u(TIME+2,2),'before':self.state()}
                self.ready_events.append(row);self.ready_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,a,size,value,user):
        if self.ready_active and WEAPON+0x17C<=a<WEAPON+0x180:
            self.ready_events.append({'tick':self.tick,'phase':self.active,
                'writer':f'{g.reg_read(UC_X86_REG_EIP):08x}','address':f'{a:08x}',
                'size':size,'before':self.u(a,size),'after':value&((1<<(size*8))-1)})
        super().mem_write(g,access,a,size,value,user)

    def execute_ready(self,held=True,ticks=22):
        self.phase_active=True;initial=self.initialize_equipped();initial_state=self.state();rows=[]
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);start=len(self.events)
            self.active='original_input';control=self.control(0x10000|(0x40000 if tick==0 else 0) if held else 0)
            self.active='original_blend';self.run(0x1D0ED0,regs=((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),))
            before=self.state();self.active='original_unit_wrapper';self.run(0xC7E90,(UI,),((UC_X86_REG_EAX,0xC7E90),))
            after_unit=self.state();self.active='original_weapon_update';self.run(0x1021D0,(WI,));after_weapon=self.state()
            self.active='original_action_callbacks';self.run(0xE7E60,(UI,))
            self.active='original_animation_clock';self.run(0x112E10,(UI,REQUEST))
            self.active='original_local_channels';self.pose_active=True;self.run(0xBDF80,(UI,));self.pose_active=False
            self.active='original_projectile_pass';self.run(0x1019F0)
            rows.append({'tick':tick,'supplied_held':held,'control':control,'before_unit':before,
                'after_unit':after_unit,'after_weapon':after_weapon,'after_postpass':self.state(),
                'projectile_requests':[e for e in self.events[start:] if e.get('service')=='000b8a30']})
        return {'initialization':initial,'initial_weapon_state':initial_state,'rows':rows,
            'ready_events':self.ready_events,'unit_wrapper_events':self.phase_events,
            'request_events':self.equipped_events,'weapon_events':self.events,
            'supplied_services':{f'{a:08x}':{'name':n,'return':r,'stack_cleanup':p} for a,(n,r,p) in self.services.items()},
            'summary':{'ticks':ticks,'first_request_tick':next((r['tick'] for r in rows if r['projectile_requests']),None),
                'requests':sum(len(r['projectile_requests']) for r in rows)}}

def authored():
    c=Cache(ROOT/'assets/disc/maps/lockout.map');weap=c.tags[0xE57B0406];off=weap['offset']
    def sid(v):return c.text(c.u(368)+c.u(c.u(364)+(v&0xFFFF)*4))
    fps=[]
    for i,a in enumerate(c.block(off+0x2A8,16)):
        graph=c.tags[c.u(a+0xC)];animations=[]
        for j,p in enumerate(c.block(graph['offset']+0x2C,0x6C)):
            if p+c.magic!=0x8021DC40:continue
            animations.append({'index':j,'file_offset':p,'pointer':f'{p+c.magic:08x}',
                'name':sid(c.u(p)),'name_string_id':f'{c.u(p):08x}',
                'frame_count':c.h(p+0x14),'raw_hex':bytes(c.data[p:p+0x6C]).hex(),
                'events':[{'file_offset':e,'type':c.h(e),'frame':c.h(e+2),
                    'raw_hex':bytes(c.data[e:e+4]).hex()} for e in c.block(p+0x4C,4)]})
        fps.append({'index':i,'record_file_offset':a,'raw_hex':bytes(c.data[a:a+16]).hex(),
            'graph':graph,'selected_animations':animations})
    result={'weapon':weap,'first_person_block':fps};c.data.close();c.stream.close();return result

def instruction_inventory(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')};rows=[]
    for entry in ENTRIES+(0xE9D50,0x1021D0,0xC6C50,0xC7E90):
        f=_trace(code,entry,starts);inst=[code.at(a) for a in sorted(f.code)]
        p=directory/f'{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'entry':f'{entry:08x}','end_exclusive':f'{f.end:08x}','listing':str(p.relative_to(ROOT)),
            'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest()})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')

def main():
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_equipped_unit_phase_1_1.py').read_bytes()).hexdigest()==DEPENDENCY_PIN
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/smg-ready-admission-1.1-oracle.json');p.add_argument('--instruction-inventory',type=Path)
    args=p.parse_args()
    if args.instruction_inventory:instruction_inventory(args.instruction_inventory);return
    cases={}
    for held in (False,True):
        t=ReadyTrace();cases['held' if held else 'idle']=t.execute_ready(held)
    gates=[]
    for blocking,flags,weapon_flags in ((-1,2,0),(0,2,0),(1,2,0),(2,2,0),(15,2,0),(0,34,0),(0,2,2)):
        t=ReadyTrace();t.initialize_equipped();t.tick=0;t.sh(WEAPON+0x17C,blocking)
        t.sh(WEAPON+0x16C,weapon_flags);t.active='explicit_weapon_admission_state'
        t.run(0x1045E0,(WI,flags,0x3F800000));before=t.state()
        t.run(0x1021D0,(WI,));after=t.state();start=len(t.events)
        t.run(0x1019F0)
        gates.append({'supplied_blocking_ticks':blocking,'supplied_input_flags':flags,
            'supplied_weapon_flags':weapon_flags,'before':before,'after_update':after,
            'after_postpass':t.state(),'requests':[e for e in t.events[start:] if e.get('service')=='000b8a30'],
            'events':t.ready_events})
    t=ReadyTrace();t.initialize_equipped();clock=[]
    for rate in (1,15,24,30,45,60,90):
        t.sh(TIME+2,rate);t.active='explicit_varied_clock_ready_query';start=len(t.ready_events)
        t.run(0x16A06A,(0x05000024,1),((UC_X86_REG_ECX,WI),))
        clock.append({'supplied_clock_rate':rate,'result':t.g.reg_read(UC_X86_REG_EAX)&0xFFFF,
            'events':t.ready_events[start:]})
    t.sh(TIME+2,30);t.active='explicit_instant_ready_argument';start=len(t.ready_events)
    t.run(0x1040F0,(0,1),((UC_X86_REG_EDI,WI),));instant={'state':t.state(),'events':t.ready_events[start:]}
    t.g.mem_write(PLAYER_DATA+0x88,b'\xff');t.active='explicit_absent_first_person_slot';start=len(t.ready_events)
    t.run(0x16A06A,(0x05000024,1),((UC_X86_REG_ECX,WI),));absent={'result':t.g.reg_read(UC_X86_REG_EAX)&0xFFFF,'events':t.ready_events[start:]}
    r={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'unit_phase_dependency_sha256':DEPENDENCY_PIN,'authored':authored(),
        'seconds_per_animation_frame_bits':f'{t.u(0x4650DC):08x}',
        'stock_30_tick_cases':cases,'explicit_varied_clock_queries':clock,
        'explicit_modified_admission_states':gates,
        'explicit_instant_ready':instant,'explicit_absent_first_person_slot':absent,
        'scope':'Unchanged source instructions; actual ready record/events. Stock cases retain frozen supplied allocation/resources/world services. Varied clock and absent-first-person/instant arguments are explicitly modified boundary cases, not claimed production defaults. Full normal DE960/physical producer remains separate.'}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'output':str(args.output),'cases':{k:v['summary'] for k,v in cases.items()},'clock':[{'rate':v['supplied_clock_rate'],'result':v['result']} for v in clock]}))

if __name__=='__main__':main()
