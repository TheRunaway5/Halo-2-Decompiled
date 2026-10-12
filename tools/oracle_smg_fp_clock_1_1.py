#!/usr/bin/env python3
"""Separate original registered first-person clock/update research.

The frozen registration suite is imported unchanged. Camera storage and cache
residency are explicit inputs; original initialization/registration, camera reset,
first-person mode handler and per-view clock instructions execute.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from oracle_smg_fp_registration_1_1 import RegistrationTrace,ROOT,OUT as REG_OUT,ARENA
from oracle_smg_constructor_equipped_1_1 import _trace
from oracle_smg_firing_1_1 import (Trace as WeaponTrace,PIN,CACHE_PIN,UNIT,UI,PLAYER_DATA,TIME,SEED,
    UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,
    UC_X86_REG_EIP,STOP)
from oracle_spartan_equipped_unit_phase_1_1 import MANAGER_OFFSET,REQUEST

REG_PIN='06c0ddfece364a4361b60219b4f2ab26e98acc693aacccbe16eb89f8d0535e92'
CAMERA=0x7800000
RESIDENT=CAMERA+0x2000
OUT=ROOT/'analysis/multiplayer/map-pack/smg-fp-clock'
ENTRIES=(0x168866,0x1678C1,0x168841,0x168853,0x1687C2,0x16823F,
    0x16A356,0x168483,0x1CF100,0x1CA2B0,0x1CA090,0x1D0A60,
    0x1D0ED0,0x1690FD,0x188D00,0x188E50,0x189470,0x156EB0,0x1573B0,
    0x124F30,0x1E13C0,0x27FB10,0x105640,0x106030,0x106C30,0x1040F0,
    0x156AD0,0x1CAE40,0x1C9F50,0x1C9E30,0x1CEDA0,0x169F3B,
    0x16A06A,0x1680B3,0x168052,0x169DDC,0x169ED9,0x18AA90,0x18AD70,
    0xE9D50,0xEFE30)
RESOURCE_ENTRIES=(0x124F30,0x1E13C0,0x27FB10)

class ClockTrace(RegistrationTrace):
    def __init__(self):
        self.fp_enabled=False;self.fp_calls=[];self.fp_returns=[];self.fp_stores=[]
        self.fp_executed=set();self.fp_resources=[]
        super().__init__()
        self.g.mem_map(CAMERA,0x400000)
        self.w(0x4F4854,CAMERA)
        self.step('original_fps_global_allocation',0x1677C3)
        self.step('original_fps_global_initialization',0x1677E5)
        profile=ARENA+0x19000;self.g.mem_write(profile,bytes(self.g.mem_read(PLAYER_DATA+0xD4,0x90)))
        self.step('original_multiplayer_profile_conversion',0x14D600,(0,),((UC_X86_REG_EAX,profile),))
        self.step('original_view_registration_clock_excluded_during_setup',0x1678C1)
        assert self.binding()['views'][0]['hands'][0]['weapon']==f'{self.initial_datum:08x}'
        c=self.cache;tag=c.tags[0xE5AA0435];resident=RESIDENT
        with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
            for i,off in enumerate(c.block(tag['offset']+0xAC,20)):
                size,pointer=c.u(off+4),c.u(off+8);location=pointer>>30;file_offset=pointer&0x3FFFFFFF
                assert location in(0,2)
                if location==2:shared.seek(file_offset);raw=shared.read(size)
                else:raw=bytes(c.data[file_offset:file_offset+size])
                assert len(raw)==size;self.g.mem_write(resident,raw)
                row={'graph':tag,'resident_pointer':resident,'index':i,'size':size,
                    'file_offset':file_offset,'file':'assets/disc/maps/shared.map'if location==2 else'assets/disc/maps/lockout.map',
                    'location':location,'descriptor_hex':bytes(c.data[off:off+20]).hex(),
                    'raw_sha256':hashlib.sha256(raw).hexdigest()}
                self.resident[off+c.magic]=row;self.fp_resources.append(row)
                resident+=(size+4095)&~4095
        assert resident<CAMERA+0x400000
        self.services.pop(0x168866);self.services.pop(0x16A356)
        self.services[0x18AD70]=('supplied unavailable audio resource/playback leaf; original FP event and sound-request builder execute',-1,24)
        self.fp_enabled=True
        self.w(0x51845C,CAMERA+0x1000)
        self.fp_action('original_camera_controller_initialization',0x156AD0)
        self.fp_action('original_camera_storage_reset',0x188D00,
            regs=((UC_X86_REG_EAX,0),(UC_X86_REG_ECX,UI)))
        self.constructor_enabled=False
        self.registration_enabled=False
        self.fp_external_inputs={'time_record_hex':bytes(self.g.mem_read(TIME,16)).hex(),
            'player_local_view_28':struct.unpack('<h',self.g.mem_read(PLAYER_DATA+0x28,2))[0],
            'shared_seed':f'{self.u(SEED):08x}','local_seed':f'{self.u(SEED+4):08x}',
            'unit_object_400_hex':bytes(self.g.mem_read(UNIT,0x400)).hex(),
            'player_record_21c_hex':bytes(self.g.mem_read(PLAYER_DATA,0x21C)).hex(),
            'camera_primary_storage_pointer':f'{CAMERA:08x}',
            'secondary_game_time_backing_pointer':f'{CAMERA+0x1000:08x}',
            'camera_initialization_entries':['00156ad0','00188d00'],
            'source_profile_input':{'source':'copy of explicit existing PLAYER+D4 unsanitized profile',
                'raw_hex':bytes(self.g.mem_read(profile,0x90)).hex()},
            'prior_registration_report_sha256':'fe7c7ea7ef620b526ab6a9debda83068055a4e02ee700bdd1ed3dea9cb375ca2',
            'heap_resource_world_lifecycle':'unchanged frozen RegistrationTrace/ConstructorTrace setup',
            'ordinary_aim_and_movement_inputs':'zero yaw, pitch, forward and left; original control conversion executes',
            'camera_world_update_executed':False}

    def fp_state(self,pose=False):
        p=self.u(0x4F1198)
        state={'view_flags':f'{self.u(p):08x}', 'view_unit':f'{self.u(p+4):08x}',
            'view_selector':f'{self.u(p+8):08x}',
            'hands':[{'weapon':f'{self.u(p+0x10+h*0x1010):08x}',
                'flags':f'{self.u(p+0xC+h*0x1010):08x}',
                'manager_hex':bytes(self.g.mem_read(p+0x14+h*0x1010,0x90)).hex(),
                'overlay_hex':bytes(self.g.mem_read(p+0xA4+h*0x1010,0x40)).hex(),
                'scalars_e4_104_hex':bytes(self.g.mem_read(p+0xF0+h*0x1010,0x20)).hex(),
                'pose_count':self.u(p+0x30C+h*0x1010),
                'sound_identifier':f'{self.u(p+0x1014+h*0x1010):08x}',
                'sound_activity':f'{self.u(p+0x1018+h*0x1010,2):04x}',
                'channel_identifier':f'{self.u(p+0x18+h*0x1010):08x}',
                'activity':f'{self.u(p+0x90+h*0x1010):08x}',
                'frame_bits':f'{self.u(p+0x30+h*0x1010):08x}'}for h in range(2)],
            'camera_global_hex':bytes(self.g.mem_read(CAMERA,0x264)).hex(),
            'source_camera_mode_state_hex':bytes(self.g.mem_read(0x4F0214,0x140)).hex(),
            'shared_seed':f'{self.u(SEED):08x}','local_seed':f'{self.u(SEED+4):08x}',
            'weapon':self.weapon_snapshot(self.initial_datum)}
        if pose:state['view_0_hex']=bytes(self.g.mem_read(p,0x20CC)).hex()
        if pose:state['source_primary_scratch_1000_hex']=bytes(self.g.mem_read(self.u(0x4F1194),0x1000)).hex()
        return state

    def fp_finish(self,address):
        for ret,row in self.fp_returns[:]:
            if ret==address:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                if'channel_pointer'in row:
                    row['channel_after_hex']=bytes(self.g.mem_read(int(row['channel_pointer'],16),32)).hex()
                if int(row['entry'],16)not in RESOURCE_ENTRIES:row['after']=self.fp_state()
                self.fp_returns.remove((ret,row))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs)
        if self.fp_enabled:self.fp_finish(STOP)

    def hook(self,g,a,size,user):
        if self.fp_enabled:
            self.fp_executed.add(a);self.fp_finish(a)
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP)
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                    'return_address':f'{self.u(sp):08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)]}
                if a not in RESOURCE_ENTRIES:row['before']=self.fp_state()
                if a in(0x1CA2B0,0x1CA090,0x1C9F50):
                    channel=self.u(sp+4)if a==0x1CA090 else g.reg_read(UC_X86_REG_EAX)
                    row['channel_pointer']=f'{channel:08x}'
                    row['channel_before_hex']=bytes(g.mem_read(channel,32)).hex()
                if a==0x1CAE40:
                    graph=self.u(sp+4);clip=self.u(sp+8)
                    row['graph_lookup_record_hex']=bytes(g.mem_read(graph,0x28)).hex()
                    row['animation_record_hex']=bytes(g.mem_read(clip,0x6C)).hex()
                    row['event_callback_address']=row['registers']['ebx']
                if a==0x1687C2:
                    event=self.u(sp+8);row['source_event_address']=f'{event:08x}'
                    row['source_event_hex']=bytes(g.mem_read(event,20)).hex()
                if a==0x27FB10:
                    codec=self.u(0x50BC70)
                    row['source_sampler_dispatch']={'codec_pointer':f'{codec:08x}',
                        'codec_id':self.u(codec,1),'codec_header_hex':bytes(g.mem_read(codec,32)).hex(),
                        'sampling_mode':self.u(0x50BC40),'node_count':self.u(0x50BC4C),
                        'sample_frame':self.u(0x50BC54),'next_frame':self.u(0x50BC58),
                        'interpolation_bits':f'{self.u(0x50BC5C):08x}',
                        'decoder_targets':[f'{self.u(at):08x}'for at in(0x50BC74,0x50BC78,0x50BC7C)],
                        'component_mask_pointers':[f'{self.u(at):08x}'for at in(0x50BC80,0x50BC84,0x50BC88)]}
                self.fp_calls.append(row);self.fp_returns.append((self.u(sp),row))
            # The inherited allocation/attachment observers are read-only
            # logging and have completed before this separate clock slice.
            # Keep the identical supplied resource boundary without retaining
            # their unrelated per-instruction snapshots during FP decoding.
            if a==0x124F30:
                sp=g.reg_read(UC_X86_REG_ESP);descriptor=g.reg_read(UC_X86_REG_EAX)
                assert descriptor in self.resident,('unmapped original resource descriptor',hex(descriptor))
                resource=self.resident[descriptor]
                self.events.append({'tick':self.tick,'phase':self.active,'service':'00124f30',
                    'description':'supplied exact source resident bytes; resource scheduler excluded',
                    'descriptor_pointer':f'{descriptor:08x}','resident_pointer':resource['resident_pointer']})
                g.reg_write(UC_X86_REG_EAX,resource['resident_pointer'])
                g.reg_write(UC_X86_REG_ESP,sp+8);g.reg_write(UC_X86_REG_EIP,self.u(sp));return
            WeaponTrace.hook(self,g,a,size,user)
        else:super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.fp_enabled:
            p=self.u(0x4F1198)
            if p<=address and address+size<=p+0x20CC:
                self.fp_stores.append({'tick':self.tick,'phase':self.active,
                    'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','view_offset':f'{address-p:04x}',
                    'size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
                    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
            if address==SEED+4:
                self.events.append({'tick':self.tick,'phase':self.active,
                    'local_rng_write_instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                    'before':f'{self.u(SEED+4):08x}','after':f'{value&0xFFFFFFFF:08x}'})
            WeaponTrace.mem_write(self,g,access,address,size,value,user)
        else:super().mem_write(g,access,address,size,value,user)

    def fp_action(self,name,entry,args=(),regs=()):
        self.active=name;self.run(entry,args,regs)

    def execute_fp(self,ticks,scenario):
        self.services[0xB8A30]=('supplied failed projectile allocation after original loadout',-1,4)
        self.services[0x1E20C0]=('supplied clear muzzle segment for FP-clock continuation',0,16)
        self.initial_fp_state=self.fp_state(pose=True)
        self.setup_transitions=[]
        if scenario=='ready':
            before=self.fp_state(pose=True)
            self.g.mem_write(UNIT+0x212,b'\xff')
            self.fp_action('explicit_current_index_none_then_original_timed_unit_equip',0xE9D50,
                (UI,0,0,0),((UC_X86_REG_EAX,1),))
            self.setup_transitions.append({'before':before,'after':self.fp_state(pose=True),
                'supplied_unit_current_index_212':255,'source_entry':'000e9d50',
                'source_eax':1,'source_arguments':[UI,0,0,0]})
        rows=[]
        for n in range(ticks):
            self.tick=n;self.w(TIME+8,n);before=self.fp_state()
            held=scenario in('held','empty_reload')or(scenario=='reload'and n<45)or(scenario=='release'and n<12)or(scenario=='pulse'and n%12<3)
            pressed=held and(n==0 or(scenario=='pulse'and n%12==0))
            control=self.control((0x10000 if held else 0)|(0x40000 if pressed else 0),
                buttons=1 if scenario=='reload'and n==46 else 0)
            self.fp_action('original_unit_manager_blend',0x1D0ED0,regs=((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),))
            self.fp_action('original_unit_wrapper',0xC7E90,(UI,),((UC_X86_REG_EAX,0xC7E90),))
            after_unit=self.fp_state()
            self.fp_action('original_weapon_update',0x1021D0,(self.initial_datum,));after_weapon=self.fp_state()
            self.fp_action('original_unit_action_callbacks',0xE7E60,(UI,))
            self.fp_action('original_unit_animation_clock',0x112E10,(UI,REQUEST))
            self.pose_active=True;self.fp_action('original_unit_local_channels',0xBDF80,(UI,));self.pose_active=False
            self.fp_action('original_projectile_postpass',0x1019F0);after_postpass=self.fp_state()
            self.fp_action('original_first_person_observer_and_clock',0x1678C1);after_fp=self.fp_state(pose=True)
            rows.append({'tick':n,'supplied_held':held,'control':control,'before':before,'after_unit':after_unit,
                'after_weapon':after_weapon,'after_postpass':after_postpass,'after_fp':after_fp})
        return rows

    def authored_fp(self):
        c=self.cache;graph=c.tags[0xE5AA0435];g=graph['offset']
        sid=lambda value:c.text(c.u(368)+c.u(c.u(364)+(value&0xFFFF)*4))if value else'NONE'
        record=lambda off,n:{'file_offset':off,'address':f'{off+c.magic:08x}',
            'raw_hex':bytes(c.data[off:off+n]).hex()}
        clips=[]
        for i,off in enumerate(c.block(g+0x2C,0x6C)):
            clips.append({'index':i,'name':sid(c.u(off)),'frames':c.h(off+0x14),
                'loop_frame':c.h(off+0x2E),'record':record(off,0x6C),
                'timing_events':[record(p,4)for p in c.block(off+0x4C,4)],
                'sound_events':[record(p,8)for p in c.block(off+0x54,8)],
                'effect_events':[record(p,4)for p in c.block(off+0x5C,4)]})
        constants=[]
        for name,va in [('end_epsilon',0x46504C),('frames_per_second',0x46516C),
            ('fire_response',0x46508C),('short_blend',0x465140),('default_blend',0x44C6B4),
            ('seconds_per_authored_frame',0x4650DC)]:
            raw=self.image.read(va,4)
            constants.append({'name':name,'address':f'{va:08x}','raw_hex':raw.hex(),
                'value':struct.unpack('<f',raw)[0]})
        return {'registration':self.authored_registration(),'graph':graph,'clips':clips,
            'graph_sound_references':[record(p,12)for p in c.block(g+0x14,12)],
            'graph_effect_references':[record(p,12)for p in c.block(g+0x1C,12)],
            'constants':constants}

def dump_instructions(t,directory):
    directory.mkdir(parents=True,exist_ok=True);rows=[]
    listing_entries=set(ENTRIES)|(set((0x1CA3A0,0x1CF070,0x169DDC,0x156AD0,
        0x156EB0,0x188D00,0x1690FD,0x18ACD0,0x18AA40,0x139750)))
    for entry in sorted(listing_entries):
        f=_trace(t.code,entry,t.starts|listing_entries)
        ins=[t.code.at(a)for a in sorted(f.code)];p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        rows.append({'entry':f'{entry:08x}','path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'instructions':len(ins),
            'executed_in_clock_slice':sum(i.address in t.fp_executed for i in ins),
            'direct_calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
    p=directory/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
    return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--ticks',type=int,default=2)
    ap.add_argument('--scenario',choices=('idle','held','reload','empty_reload','release','pulse','ready'),default='idle')
    ap.add_argument('--output',type=Path,default=OUT/'working-report.json')
    ap.add_argument('--instructions',type=Path);args=ap.parse_args()
    assert hashlib.sha256((ROOT/'tools/oracle_smg_fp_registration_1_1.py').read_bytes()).hexdigest()==REG_PIN
    t=ClockTrace();rows=t.execute_fp(args.ticks,args.scenario)
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'registration_dependency_sha256':REG_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'scenario':args.scenario,'authored':t.authored_fp(),
        'rows':rows,'calls':t.fp_calls,'writes':t.fp_stores,'resources':t.fp_resources,
        'initial_fp_state':t.initial_fp_state,'setup_transitions':t.setup_transitions,
        'external_inputs':t.fp_external_inputs,
        'player_local_view_28':struct.unpack('<h',t.g.mem_read(PLAYER_DATA+0x28,2))[0],
        'supplied_services':{f'{a:08x}':{'name':name,'return':result,'cleanup':pop}
            for a,(name,result,pop)in t.services.items()},
        'executed_addresses':[f'{a:08x}'for a in sorted(t.fp_executed)],
        'projectile_requests':[e for e in t.events if e.get('service')=='000b8a30'],
        'local_rng_stores':[e for e in t.events if'local_rng_write_instruction'in e],
        'actual_first_person_clock_executed':True,'actual_first_person_decode_executed':True,
        'xbox_matrix_bit_parity_verified':False,'native_gameplay_update_applied':False,
        'scope':'Original allocated/equipped SMG, actual registered view, actual first-person clock/event traversal/decoder and Unicorn matrix instructions. Existing explicit player/world inputs, exact resource residency, clear muzzle query, failed post-loadout projectile allocation, and unavailable audio playback leaves remain supplied. Xbox reciprocal approximation is not validated by this report.'}
    if args.instructions:result['instruction_inventory']=dump_instructions(t,args.instructions.resolve())
    args.output.parent.mkdir(parents=True,exist_ok=True);p=args.output;p.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'path':str(p),'calls':len(t.fp_calls),'writes':len(t.fp_stores)}))

if __name__=='__main__':main()
