#!/usr/bin/env python3
"""Separate original assigned-player/equipped-SMG animation evidence.

Explicit preallocated player/object storage is supplied. Ownership and equipped
request builders execute unchanged instructions, as do graph lookup and clocks.
No existing frozen oracle or runtime code is modified.
"""
import argparse, csv, hashlib, json, struct
from pathlib import Path
from oracle_spartan_live_pose_1_1 import LivePoseTrace, ROOT, PIN, CACHE_PIN, MANAGER_OFFSET, REQUEST
from oracle_smg_firing_1_1 import (UNIT, UI, WEAPON, WI, TIME, SEED, PLAYERS, PLAYER_DATA, GAME, WORLD,
    Cache, Xbe, _Code, _trace, UC_X86_REG_EAX, UC_X86_REG_EBX,
    UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI,
    UC_X86_REG_ESP, UC_X86_REG_EIP)
CONTROL_INPUT, MOVEMENT_REQUEST=0x10FD000,0x10FD200
LIVE_DEPENDENCY_PIN='28afd44db12c96a67bc82bca1c617cbd9fb8fa631e949f71e9b74743d2de10f5'

class EquippedTrace(LivePoseTrace):
    def __init__(self):
        super().__init__(5)
        self.services[0xFB560]=('supplied carried-object world attachment; marker/physics producer not executed',0,8)
        for entry in (0x115650,0xCB060,0xC9A40):self.services.pop(entry,None)
        self.services[0x1E20C0]=('supplied clear muzzle-world segment; world query producer not executed',0,16)
        self.services[0x179B90]=('supplied unavailable sound allocator; original animation sound request is recorded',-1,8)
        self.equipped_active=False;self.equipped_events=[];self.equipped_returns=[]
        self.w(PLAYERS+0x24,0x21C);self.w(PLAYERS+0x38,1)
        self.w(PLAYERS+0x48,0x10FC000);self.w(0x10FC000,1)
        c=Cache(ROOT/'assets/disc/maps/lockout.map')
        weapon=self.authored['tag'];at=weapon['offset']
        sid=lambda v:c.text(c.u(368)+c.u(c.u(364)+(v&0xFFFF)*4))
        self.equipped_metadata={'weapon':weapon,
            'weapon_class':{'field':'288','bits':f'{c.u(at+0x288):08x}','name':sid(c.u(at+0x288))},
            'weapon_name':{'field':'28c','bits':f'{c.u(at+0x28C):08x}','name':sid(c.u(at+0x28C))}}
        self.w(WEAPON,int(weapon['datum'],16))
        # The original resource descriptor itself selects shared-cache location
        # 2. Supplying resident bytes is explicit; resource I/O is not executed.
        self.g.mem_map(0x6000000,0x1000000);resident_at=0x6000000
        shared=(ROOT/'assets/disc/maps/shared.map').read_bytes()
        self.equipped_metadata['shared_cache_sha256']=hashlib.sha256(shared).hexdigest()
        for i,off in enumerate(c.block(self.live_metadata['graph']['offset']+0xAC,20)):
            size,pointer=c.u(off+4),c.u(off+8)
            if pointer>>30!=2:continue
            file_offset=pointer&0x3FFFFFFF
            assert file_offset+size<=len(shared)
            raw=shared[file_offset:file_offset+size];self.g.mem_write(resident_at,raw)
            self.resident[off+c.magic]={'resident_pointer':resident_at,'index':i,'size':size,
                'file_offset':file_offset,'file':'assets/disc/maps/shared.map','location':2,
                'descriptor_hex':bytes(c.data[off:off+20]).hex(),
                'raw_sha256':hashlib.sha256(raw).hexdigest()}
            resident_at+=(size+4095)&~4095
        assert resident_at<=0x7000000
        c.data.close();c.stream.close()

    def selected(self):
        m=UNIT+MANAGER_OFFSET;packed=self.u(m+4);index=packed>>16
        secondary=UNIT+self.u(UNIT+0x33E,2)+0x9C;secondary_index=self.u(secondary+4)>>16
        return {'mode_string_id':f'{self.u(m+0x70):08x}',
            'weapon_class_string_id':f'{self.u(m+0x74):08x}',
            'weapon_name_string_id':f'{self.u(m+0x78):08x}',
            'activity_string_id':f'{self.u(m+0x7C):08x}',
            'packed':f'{packed:08x}','frame_bits':f'{self.u(m+0x1C):08x}',
            'clip':self.live_metadata['animations'][index] if index<len(self.live_metadata['animations']) else None,
            'secondary_clip':self.live_metadata['animations'][secondary_index] if secondary_index<len(self.live_metadata['animations']) else None,
            'secondary_frame_bits':f'{self.u(secondary+0x1C):08x}',
            'owner':f'{self.u(UNIT+0x13C):08x}','player_unit':f'{self.u(PLAYER_DATA+0x2C):08x}',
            'equipped_indices_hex':bytes(self.g.mem_read(UNIT+0x212,6)).hex(),
            'unit_aim_hex':bytes(self.g.mem_read(UNIT+0x14C,0x54)).hex(),
            'secondary_channel_hex':bytes(self.g.mem_read(secondary,32)).hex(),
            'manager_hex':bytes(self.g.mem_read(m,0x90)).hex(),
            'unit_extension_hex':bytes(self.g.mem_read(UNIT+self.u(UNIT+0x33E,2),0x124)).hex(),
            'unit_control_hex':bytes(self.g.mem_read(UNIT+self.u(UNIT+0x346,2),0x44)).hex()}

    def hook(self,g,a,size,user):
        if getattr(self,'equipped_active',False):
            sp=g.reg_read(UC_X86_REG_ESP)
            for ret,row in reversed(self.equipped_returns[:]):
                if a==ret:
                    row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}';row['after']=self.selected()
                    self.equipped_returns.remove((ret,row));break
            if a in (0x14E240,0xCD3B0,0xCDC60,0x153A90,0xE9D50,0x111490,
                     0x111500,0x11CC90,0x110BF0,0x1D0A60,0x1CFDE0,0xE7E60,
                     0x112E10,0x115650,0xC9A40,0xCB060,0x1CF100,0x113830,
                     0x151C90,0x151AA0,0x151A20,0xCCC60,0xC8140,0xC7420,
                     0xC6C50,0xE7EF0,0xE7F30,0xE44C0,0xE1800,0x1D14F0,
                     0x150DD0,0x150F00,0xE88B0,0xE8740,0x101D50):
                row={'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
                     'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(7)],'before':self.selected()}
                if a==0xC8140:row['original_control_record_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_ECX),0x7C)).hex()
                if a==0xE7F30:row['original_action_request_hex']=bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),32)).hex()
                self.equipped_events.append(row);self.equipped_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def initialize_equipped(self):
        initial=self.initialize();self.equipped_active=True
        # This fixture supplies the already allocated player record, with no
        # local view. Original14E240 creates the actual unit ownership linkage.
        self.w(PLAYER_DATA+0x2C,-1);self.w(UNIT+0xD4,-1)
        self.active='original_player_unit_assignment';start=len(self.events)
        self.run(0x14E240,(0,),((UC_X86_REG_EBX,UI),))
        assigned=self.selected()
        assert self.u(UNIT+0x13C)==0 and self.u(PLAYER_DATA+0x2C)==UI
        # Original inventory allocation is a separate producer. The actual
        # stock SMG datum is supplied in slot0; E9D50 chooses that slot itself.
        self.w(WEAPON+0x14,UI);self.w(WEAPON+0x154,UI)
        self.w(UNIT+0x218,WI)
        self.active='original_equipped_weapon_constructor';self.run(0x101BB0,(WI,))
        self.g.mem_write(UNIT+0x212,b'\xff\xff\xff\xff\x00\xff')
        self.active='original_equip_primary_slot'
        self.run(0xE9D50,(UI,0x07000101,0,0),((UC_X86_REG_EAX,1),))
        return {'startup':initial,'assigned':assigned,'equipped':self.selected(),
            'setup_services':[e for e in self.events[start:] if 'service' in e],
            'player_record_hex':bytes(self.g.mem_read(PLAYER_DATA,0x21C)).hex()}

    def control(self,flags=0,yaw=0.,pitch=0.,forward=0.,left=0.,buttons=0):
        self.g.mem_write(CONTROL_INPUT,bytes(0x5C))
        self.w(CONTROL_INPUT,flags);self.f(CONTROL_INPUT+4,yaw);self.f(CONTROL_INPUT+8,pitch)
        self.f(CONTROL_INPUT+0xC,forward);self.f(CONTROL_INPUT+0x10,left)
        self.f(CONTROL_INPUT+0x14,1. if flags&0x10000 else 0.)
        # Original151C90 copies these four input bytes into C8140's +6 field,
        # which preserves the pending/current primary/secondary selectors.
        self.g.mem_write(CONTROL_INPUT+0x1E,bytes(self.g.mem_read(UNIT+0x214,4)))
        self.sh(CONTROL_INPUT+0x22,-1);self.sh(CONTROL_INPUT+0x24,-1)
        self.sh(CONTROL_INPUT+0x1C,buttons)
        supplied=bytes(self.g.mem_read(CONTROL_INPUT,0x5C)).hex()
        self.active='original_player_pressed_edges'
        self.run(0x150DD0,regs=((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,CONTROL_INPUT)))
        self.active='original_player_action_request_producer'
        self.run(0x150F00,(0,UI,CONTROL_INPUT))
        self.active='original_player_control_conversion'
        self.run(0x151C90,(UI,CONTROL_INPUT),((UC_X86_REG_EAX,0),))
        control=self.selected();self.active='original_unit_aim_update';self.run(0xC7420,(UI,))
        return {'supplied_input_record_hex':supplied,
            'after_original_action_input_hex':bytes(self.g.mem_read(CONTROL_INPUT,0x5C)).hex(),
            'converted_control_state':control,'after_original_aim_update':self.selected()}

    def execute_equipped(self,ticks=15,scenario='idle'):
        initial=self.initialize_equipped();rows=[];m=UNIT+MANAGER_OFFSET
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);before=self.selected();start=len(self.equipped_events)
            held=(scenario in ('fire','reload') and tick<45) or scenario=='empty_reload'
            flags=(0x10000 if held else 0)|(0x40000 if held and tick==0 else 0)
            yaw=.5 if scenario=='aim' else 0.;pitch=.25 if scenario=='aim' else 0.
            control=self.control(flags,yaw,pitch,1. if scenario=='move_input' else 0.,0.,1 if scenario=='reload' and tick==45 else 0)
            self.active='original_manager_blend';self.run(0x1D0ED0,regs=((UC_X86_REG_ESI,m),))
            after_blend=self.selected()
            self.active='original_unit_control_and_weapon_input';self.run(0xC6C50,(UI,))
            after_control=self.selected()
            self.active='original_weapon_tick';self.run(0x1021D0,(WI,))
            after_weapon=self.selected();weapon_after_update=self.state()
            self.active='original_action_dispatch';self.run(0xE7E60,(UI,))
            after_actions=self.selected()
            self.active='original_equipped_animation_update';self.run(0x112E10,(UI,REQUEST))
            after_animation=self.selected();self.active='original_equipped_local_channels'
            self.pose_active=True;self.run(0xBDF80,(UI,));self.pose_active=False
            after_pose=self.selected();self.active='original_projectile_postpass';self.run(0x1019F0)
            rows.append({'tick':tick,'scenario':scenario,'supplied_held':held,'control_conversion':control,
                'before':before,'after_blend':after_blend,'after_controls':after_control,
                'after_weapon':after_weapon,'weapon_after_update':weapon_after_update,
                'after_actions':after_actions,'after_animation':after_animation,'weapon_state':self.state(),
                'after_pose':after_pose,'after_dispatch':self.selected(),
                'local_transforms_hex':bytes(self.g.mem_read(UNIT+0x3000,33*32)).hex(),
                'event_range':[start,len(self.equipped_events)]})
        return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'scenario':scenario,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'authored':self.equipped_metadata,'initialization':initial,'rows':rows,
            'events':self.equipped_events,'live_events':self.live_events,'weapon_events':self.events,
            'original_graph':self.live_metadata['graph'],
            'options_state':self.u(GAME+8),'options_mode':self.u(GAME+12,1),'simulation_world_state':self.u(WORLD+8),
            'dependencies':{'tools/oracle_spartan_live_pose_1_1.py':LIVE_DEPENDENCY_PIN},
            'summary':{'ticks':len(rows),'events':len(self.equipped_events),
                'primary_clips':sorted({r['after_animation']['clip']['index'] for r in rows if r['after_animation']['clip']}),
                'secondary_clips':sorted({r['after_animation']['secondary_clip']['index'] for r in rows if r['after_animation']['secondary_clip']}),
                'projectile_requests':sum(e.get('service')=='000b8a30' for e in self.events),
                'animation_sound_requests':sum(e.get('service')=='00179b90' for e in self.events),
                'distinct_local_transforms':len({r['local_transforms_hex'] for r in rows})},
            'scope':'Bounded assigned player/equipped original SMG request/lookup/channel slice. Explicit preallocated player and weapon inventory storage, raw controller record, resident resources, carried-world attachment, clear muzzle query, failed projectile/effect/audio allocation. Original player action/control producer executes; device producer, world physics classification, full carried object allocation and full world simulation remain separate.'}

    def execute_movement_selection(self,state=1,physical_state=0,blend_byte=0):
        initial=self.initialize_equipped();self.control()
        self.active='explicit_physics_state_fixture';self.g.mem_write(UNIT+0x3DC,bytes([physical_state]))
        self.g.mem_write(MOVEMENT_REQUEST,struct.pack('<3I',0x07000101,0x0400000C,0))
        before=self.selected();self.active='original_movement_activity_selector'
        self.run(0xE44C0,(state,blend_byte,MOVEMENT_REQUEST+8),
            ((UC_X86_REG_EAX,UI),(UC_X86_REG_EBX,MOVEMENT_REQUEST),(UC_X86_REG_EDI,MOVEMENT_REQUEST+4)))
        request=bytes(self.g.mem_read(MOVEMENT_REQUEST,12)).hex()
        self.active='original_selected_movement_request';self.run(0x112E10,(UI,MOVEMENT_REQUEST))
        selected=self.selected();self.pose_active=True;self.run(0xBDF80,(UI,));self.pose_active=False
        return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'initialization':initial,'state_input':state,'physical_state_input':physical_state,
            'blend_byte_input':blend_byte,'before':before,'original_request_hex':request,
            'selected':selected,'after_pose':self.selected(),'events':self.equipped_events,
            'live_events':self.live_events,'weapon_events':self.events,
            'local_transforms_hex':bytes(self.g.mem_read(UNIT+0x3000,33*32)).hex(),
            'scope':'Explicit classified-physics inputs to untouched E44C0 then actual equipped request/lookup/decode. Upstream world physics classification is not executed.'}

def inventory(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert hashlib.sha256(x.data).hexdigest()==PIN
    code=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    entries=(0x14E240,0x14EAE0,0x150DD0,0x150F00,0x151C90,0x151AA0,0x151A20,
        0xCCC60,0xC8250,0xC8140,0xC7420,0x1207F0,0xC7E90,0xC5D10,0xC6C50,
        0xE9D50,0x111490,0x111500,0x11CC90,0x110BF0,0x115650,0xC9A40,0xCB060,
        0xE7EF0,0xE7F30,0xE88B0,0xE8740,0xE8950,0x101D50,0x105640,0x101BB0,
        0x1122D0,0x1122A0,0x112E10,0x113830,0x1CF100,0x1D0A60,0x1CFDE0,
        0xDE960,0xE1800,0xE3B20,0xE44C0,0x1D14F0,0x1E8F60,0x179B90)
    rows=[]
    for start in entries:
        f=_trace(code,start,starts|set(entries));inst=[code.at(a) for a in sorted(f.code)]
        p=directory/f'{start:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'start':f'{start:08x}','end_exclusive':f'{f.end:08x}',
            'instruction_count':len(inst),'listing':str(p.relative_to(ROOT)),
            'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'reachable_instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest(),
            'direct_calls':[{'site':f'{a:08x}','target':f'{b:08x}'} for a,b,k in f.sites if k=='call']})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'routines':rows},indent=2)+'\n')

def generate_suite(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True);cases=[]
    for scenario,ticks in (('idle',75),('aim',12),('move_input',12),('fire',60),('reload',135),('empty_reload',225)):
        t=EquippedTrace();r=t.execute_equipped(ticks,scenario);p=directory/f'{scenario}.json'
        p.write_text(json.dumps(r,indent=2)+'\n')
        cases.append({'name':scenario,'path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'summary':r['summary']})
        print(json.dumps(cases[-1]),flush=True);del t,r
    for state in range(12):
        t=EquippedTrace();r=t.execute_movement_selection(state);p=directory/f'movement_selector_{state}.json'
        p.write_text(json.dumps(r,indent=2)+'\n')
        cases.append({'name':f'movement_selector_{state}','path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'request_hex':r['original_request_hex'],'selected_clip':r['selected']['clip']})
        print(json.dumps({'name':cases[-1]['name'],'clip':r['selected']['clip']}),flush=True);del t,r
    (directory/'suite.json').write_text(json.dumps({'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'cases':cases,'tick_cases':6,'ticks':sum(c.get('summary',{}).get('ticks',0) for c in cases),
        'classified_physics_selector_cases':12,
        'scope':'Original equipped request/control/clip selection and decoded channels with all supplied producer boundaries retained. Classified movement state inputs are separate fixtures, not recovered world physics.'},indent=2)+'\n')

def main():
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_live_pose_1_1.py').read_bytes()).hexdigest()==LIVE_DEPENDENCY_PIN
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,
        default=ROOT/'analysis/multiplayer/map-pack/spartan-equipped-animation-1.1-oracle.json')
    p.add_argument('--ticks',type=int,default=15);p.add_argument('--scenario',choices=('idle','fire','reload','empty_reload','aim','move_input'),default='idle')
    p.add_argument('--movement-state',type=int,choices=range(12));p.add_argument('--physical-state',type=int,default=0)
    p.add_argument('--suite',type=Path);p.add_argument('--instruction-inventory',type=Path);args=p.parse_args()
    if args.instruction_inventory:inventory(args.instruction_inventory);return
    if args.suite:generate_suite(args.suite);return
    t=EquippedTrace()
    try:report=t.execute_equipped(args.ticks,args.scenario) if args.movement_state is None else t.execute_movement_selection(args.movement_state,args.physical_state)
    except Exception:
        print('recent',[(hex(a),t.code.at(a).mnemonic,t.code.at(a).op_str) for a in t.recent]);raise
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report.get('summary',{'movement_state':args.movement_state,'selected':report.get('selected',{}).get('clip')})))
if __name__=='__main__':main()
