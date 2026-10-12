#!/usr/bin/env python3
"""Independent full allocated SMG constructor/instant and timed equip evidence.

Original1A28A0 -> B8A30 ->101BB0 ->CE510 ->E9D50 executes unchanged.
The frozen attachment fixture also executes the original parent/hand marker
attachment; the parent field is a real automatic-reload prerequisite.
No prior oracle or production file is edited. Exact user cache metadata and
explicit heap/resource/world service boundaries are inherited from the frozen
full inventory allocator fixture. First-person presentation is not emulated.
"""
import argparse, csv, hashlib, json, struct
from pathlib import Path
from oracle_mp_inventory_equipped_1_1 import (
    EquippedPickupTrace, ROOT, PIN, CACHE_PIN, HEADERS, UNIT, UI, TIME, SEED,
    UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_ESP, STOP, _trace)
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace
from oracle_smg_firing_1_1 import Xbe, _Code, PLAYER_DATA, FPS, GAME, WORLD, UC_X86_REG_EIP, UC_X86_REG_ECX
from oracle_spartan_equipped_unit_phase_1_1 import UnitPhaseTrace, MANAGER_OFFSET, REQUEST, UC_X86_REG_ESI

INVENTORY_PIN='47f56a9236ea2efe870d1ad82af16ace214f4598294f8c826966cced94dee655'
ATTACHMENT_PIN='9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c'
UNIT_PHASE_PIN='cb62dcd1595260984ae37e2e5aedc3a0e09cdc921ad49d104c4ac8d122567a38'
ENTRIES=(0x1A28A0,0xB8A30,0xBD170,0x101BB0,0xCE510,0xE9D50,
    0x1040F0,0x108230,0x109C30,0x16A06A,0x169F3B,0x1D0A60,
    0xC7E90,0xC6C50,0x1021D0,0x1019F0,0x1099B0,0x109A60,0x109D50,0x1CF100,
    0x105640,0x106030,0x106C30,0x106580,0x101D50,0xE7EF0,0xE7F30,
    0xE88B0,0xE8740,0x112270,0x1CA2B0,0x1CA090,0x1DE890)

class ConstructorTrace(AttachmentTrace):
    control=UnitPhaseTrace.control
    def __init__(self):
        self.constructor_calls=[];self.constructor_returns=[];self.constructor_writes=[]
        self.constructor_datums=set();self.active_weapons=[];self.constructor_enabled=True
        super().__init__()

    def weapon_snapshot(self,datum):
        p=self.u(HEADERS+(datum&0xFFFF)*12+8)
        s16=lambda off:struct.unpack('<h',self.g.mem_read(p+off,2))[0]
        fbits=lambda off:f'{self.u(p+off):08x}'
        manager=s16(0x12A)
        selected=None
        if manager>=0 and hasattr(self,'cache') and self.u(p+manager) in self.cache.tags:
            graph=self.cache.tags[self.u(p+manager)]
            animations=self.cache.block(graph['offset']+0x2C,0x6C)
            index=self.u(p+manager+4)>>16
            if index<len(animations):
                record=animations[index];sid=self.cache.u(record)
                name=self.cache.text(self.cache.u(368)+self.cache.u(self.cache.u(364)+(sid&0xFFFF)*4))
                selected={'graph':graph,'index':index,'name':name,'file_offset':record,
                    'frame_count':self.cache.h(record+0x14),
                    'raw_hex':bytes(self.cache.data[record:record+0x6C]).hex(),
                    'requested_activity_string_id':f'{self.u(p+manager+0x7C):08x}'}
        return {'datum':f'{datum:08x}','address':f'{p:08x}',
            'definition':f'{self.u(p):08x}','owner':f'{self.u(p+0x154):08x}',
            'parent':f'{self.u(p+0x14):08x}','parent_node':self.u(p+0x18,1),
            'manager_offset':manager,'manager_hex':None if manager<0 else bytes(self.g.mem_read(p+manager,0x90)).hex(),
            'source_selected_weapon_own_animation':selected,
            'weapon_own_primary_channel':None if manager<0 else {
                'graph':f'{self.u(p+manager):08x}','animation_identifier':f'{self.u(p+manager+4):08x}',
                'delay_10':self.u(p+manager+0x10,1),'flags_11':f'{self.u(p+manager+0x11,1):02x}',
                'flags_12':f'{self.u(p+manager+0x12,2):04x}',
                'events_14':f'{self.u(p+manager+0x14,2):04x}',
                'events_16':f'{self.u(p+manager+0x16,2):04x}',
                'rate_18_bits':f'{self.u(p+manager+0x18):08x}',
                'frame_1c_bits':f'{self.u(p+manager+0x1C):08x}'},
            'weapon_16c_260_hex':bytes(self.g.mem_read(p+0x16C,0xF4)).hex(),
            'loaded':s16(0x22C),'reserve':s16(0x22A),
            'magazine_mode':s16(0x224),'magazine_ticks':s16(0x226),
            'reload_transfer_ticks':s16(0x230),'magazine_232':s16(0x232),
            'blocking_ticks':s16(0x17C),'weapon_animation_index':s16(0x17E),
            'weapon_state_u32':self.u(p+0x178),'weapon_flags':f'{self.u(p+0x16C,2):04x}',
            'input_flags':f'{self.u(p+0x16E,2):04x}',
            'trigger_state':self.u(p+0x20C,1),'trigger_ticks':s16(0x20E),
            'trigger_flags':fbits(0x210),'trigger_effect_handle':fbits(0x214),
            'trigger_state_hex':bytes(self.g.mem_read(p+0x20C,12)).hex(),
            'magazine_state_hex':bytes(self.g.mem_read(p+0x224,16)).hex(),
            'barrel_state_hex':bytes(self.g.mem_read(p+0x1A4,0x34)).hex(),
            'barrel_counter':struct.unpack('<b',self.g.mem_read(p+0x1A4,1))[0],
            'barrel_mode':self.u(p+0x1A5,1),'barrel_ticks':s16(0x1A6),
            'barrel_flags':f'{self.u(p+0x1A8,2):04x}','barrel_shot_count':s16(0x1AA),
            'barrel_effect_handle':fbits(0x1D4),
            'unit_hand_timers':[self.u(UNIT+0x1FA,1),self.u(UNIT+0x1FB,1)],
            'unit_equipped_indices_hex':bytes(self.g.mem_read(UNIT+0x212,6)).hex(),
            'shared_seed':f'{self.u(SEED):08x}'}

    def capture(self):
        return {'weapon':self.weapon_snapshot(self.initial_datum),
            'unit_manager':self.manager_state(),
            'unit_object_400_hex':bytes(self.g.mem_read(UNIT,0x400)).hex()}

    def finish_constructor_returns(self,address):
        for ret,row,datum in self.constructor_returns[:]:
            if ret!=address:continue
            row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
            if datum is not None:row['after']=self.weapon_snapshot(datum)
            if row['entry'] in ('00169f3b','001de890'):
                ptr=self.g.reg_read(UC_X86_REG_EAX)
                row['selected_animation_pointer']=f'{ptr:08x}'
                row['selected_animation_record_hex']=None if not ptr else bytes(self.g.mem_read(ptr,0x6C)).hex()
            self.constructor_returns.remove((ret,row,datum))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs)
        if hasattr(self,'constructor_returns'):self.finish_constructor_returns(STOP)

    def hook(self,g,a,size,user):
        if self.constructor_enabled:
            self.finish_constructor_returns(a)
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP);datum=None
                if a==0x101BB0:
                    datum=self.u(sp+4);self.constructor_datums.add(datum)
                elif a==0x1040F0:datum=g.reg_read(UC_X86_REG_EDI)
                elif a==0x108230:datum=self.u(sp+4)
                elif a==0x1021D0:datum=self.u(sp+4)
                elif a in (0x1099B0,0x109A60,0x109D50,0x105640,0x106030,0x106C30,0x106580,
                            0x101D50,0x1CF100,0x1CA2B0,0x1CA090,0x1DE890) and hasattr(self,'initial_datum'):
                    datum=self.initial_datum
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                    'return_address':f'{self.u(sp):08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(6)]}
                if datum is not None:row['before']=self.weapon_snapshot(datum)
                if a==0x1D0A60:
                    row['request_manager_pointer']=row['registers']['ecx']
                    row['requested_activity_string_id']=row['registers']['eax']
                    for candidate in self.constructor_datums:
                        cp=self.u(HEADERS+(candidate&0xFFFF)*12+8)
                        mo=struct.unpack('<h',self.g.mem_read(cp+0x12A,2))[0]
                        if mo>=0 and cp+mo==g.reg_read(UC_X86_REG_ECX):
                            datum=candidate;row['before']=self.weapon_snapshot(datum)
                            row['actual_weapon_own_manager']=True
                if a==0x106030:
                    p=self.u(HEADERS+(g.reg_read(UC_X86_REG_EAX)&0xFFFF)*12+8)
                    row['admission_operands']={'eax_datum':row['registers']['eax'],
                        'trigger_20c':self.u(p+0x20C,1),'trigger_218':self.u(p+0x218,1),
                        'weapon_state_178':self.u(p+0x178)}
                self.constructor_calls.append(row);self.constructor_returns.append((self.u(sp),row,datum))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.constructor_enabled:
            for datum in self.constructor_datums:
                p=self.u(HEADERS+(datum&0xFFFF)*12+8)
                if p+0x16C<=address and address+size<=p+0x260:
                    self.constructor_writes.append({'tick':self.tick,'phase':self.active,
                        'datum':f'{datum:08x}','instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                        'object_offset':f'{address-p:03x}','size':size,
                        'before_hex':bytes(g.mem_read(address,size)).hex(),
                        'after_hex':int(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
        super().mem_write(g,access,address,size,value,user)

    def presentation_boundary(self):
        return {'player_local_view_28':struct.unpack('<h',self.g.mem_read(PLAYER_DATA+0x28,2))[0],
            'player_first_person_selector_88':self.u(PLAYER_DATA+0x88,1),
            'first_person_globals_first_0x20_hex':bytes(self.g.mem_read(FPS,0x20)).hex(),
            'notification_service':self.services.get(0x16A356),
            'actual_first_person_clock_executed':False,'actual_first_person_pose_executed':False}

    def report(self):
        initial=self.weapon_snapshot(self.initial_datum)
        call_start=len(self.constructor_calls);write_start=len(self.constructor_writes)
        self.active='explicit_existing_equipped_weapon_timed_ready_handler'
        self.run(0x1040F0,(0,0),((UC_X86_REG_EDI,self.initial_datum),))
        timed={'supplied_handler_arguments':[0,0],'before':initial,
            'after':self.weapon_snapshot(self.initial_datum),
            'calls':self.constructor_calls[call_start:],'writes':self.constructor_writes[write_start:]}
        predicates=self.admission_cases()
        return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'inventory_dependency_sha256':INVENTORY_PIN,
            'attachment_dependency_sha256':ATTACHMENT_PIN,
            'execution_options':{'state':self.u(GAME+8),'mode_byte':self.u(GAME+12,1),
                'world_state':self.u(WORLD+8),'engine_vtable':'00463b48'},
            'authored':self.authored_initial_values(),
            'actual_original_initial_loadout':{'weapon':initial,
                'source_call_order':self.constructor_calls[:call_start],
                'source_writes':self.constructor_writes[:write_start],
                'source_attachment_calls':self.attachment_calls,
                'source_attachment_writes':self.attachment_writes},
            'explicit_timed_handler_case':timed,
            'explicit_reload_admission_operand_cases':predicates,
            'first_person_presentation_boundary':self.presentation_boundary(),
            'heap_requests':self.heap_requests,'original_allocated_weapon_resources':self.weapon_resources,
            'supplied_services':{f'{a:08x}':{'name':n,'return':r,'stack_cleanup':pop} for a,(n,r,pop) in self.services.items()},
            'scope':'Full original stock SMG allocation/constructor, original instant initial equip and original parent/hand marker attachment execute. Exact allocated object and instruction writes are captured independently of the inherited fixed-WEAPON state helper. Timed handler arguments are explicitly supplied after initial equip. Player/unit storage, heap backing/residency and recorded world/effect/audio services remain boundaries. No first-person clock or pose is supplied or asserted.'}

    def admission_cases(self):
        p=self.initial_pointer;old=[bytes(self.g.mem_read(p+off,size)) for off,size in ((0x178,4),(0x20C,1),(0x218,1))]
        rows=[]
        for state,t0,t1 in ((0,0,0),(1,0,0),(2,0,0),(5,0,0),(9,0,0),(-1,0,0),
                            (10,0,0),(0,1,0),(0,0,1),(0,255,0),(0,0,255),(0,1,1)):
            self.active='explicit_reload_admission_operand_fixture'
            self.w(p+0x178,state&0xFFFFFFFF);self.g.mem_write(p+0x20C,bytes([t0]));self.g.mem_write(p+0x218,bytes([t1]))
            start=len(self.constructor_calls);self.run(0x106030,regs=((UC_X86_REG_EAX,self.initial_datum),))
            rows.append({'supplied_weapon_state_178':state,'supplied_trigger_20c':t0,'supplied_trigger_218':t1,
                'return_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
                'source_calls':self.constructor_calls[start:]})
        for (off,size),data in zip(((0x178,4),(0x20C,1),(0x218,1)),old):self.g.mem_write(p+off,data)
        return {'scope':'Only these three admission operands are explicitly varied on the original allocated weapon. This is a direct original106030 predicate fixture, not a supplied full-tick action state.', 'cases':rows}

    def execute_initial(self,ticks,scenario):
        # The original initial allocation/equip has already completed. Only
        # successful shot allocation and its world query become explicit new
        # services for this bounded unit/weapon/postpass continuation.
        self.services[0xB8A30]=('supplied failed projectile allocation after successful original starting loadout',-1,4)
        self.services[0x1E20C0]=('supplied clear muzzle segment for bounded allocated-SMG continuation',0,16)
        initial=self.capture();rows=[];requests=[]
        for tick in range(ticks):
            self.tick=tick;self.w(TIME+8,tick);before=self.capture()
            held=(scenario in ('fire','reload_rejected_45','reload_accepted_46') and tick<45) or scenario in ('held','held_empty')
            self.active='original_allocated_player_input'
            control=self.control((0x10000 if held else 0)|(0x40000 if held and tick==0 else 0),
                buttons=1 if (scenario=='reload_rejected_45' and tick==45) or (scenario=='reload_accepted_46' and tick==46) else 0)
            self.active='original_allocated_unit_blend';self.run(0x1D0ED0,regs=((UC_X86_REG_ESI,UNIT+MANAGER_OFFSET),))
            self.active='original_allocated_unit_wrapper';self.run(0xC7E90,(UI,),((UC_X86_REG_EAX,0xC7E90),))
            after_unit=self.capture();self.active='original_allocated_weapon_update'
            self.run(0x1021D0,(self.initial_datum,));after_weapon=self.capture()
            self.active='original_allocated_unit_action_callbacks';self.run(0xE7E60,(UI,))
            self.active='original_allocated_unit_animation_clock';self.run(0x112E10,(UI,REQUEST));after_animation=self.capture()
            self.active='original_allocated_unit_local_channels';self.pose_active=True;self.run(0xBDF80,(UI,));self.pose_active=False
            self.active='original_allocated_projectile_postpass';start=len(self.events);self.run(0x1019F0)
            emitted=[{k:v for k,v in e.items() if k!='state'} for e in self.events[start:] if e.get('service')=='000b8a30']
            requests.extend(emitted)
            rows.append({'tick':tick,'scenario':scenario,'supplied_held':held,'before':before,
                'control':control,'after_unit':after_unit,'after_weapon':after_weapon,
                'after_animation':after_animation,'after_postpass':self.capture(),
                'local_transforms_hex':bytes(self.g.mem_read(UNIT+0x3000,33*32)).hex(),
                'projectile_requests':emitted})
        return {'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'inventory_dependency_sha256':INVENTORY_PIN,'unit_phase_dependency_sha256':UNIT_PHASE_PIN,
            'attachment_dependency_sha256':ATTACHMENT_PIN,
            'options':{'state':self.u(GAME+8),'mode_byte':self.u(GAME+12,1),'world_state':self.u(WORLD+8),'engine_vtable':'00463b48'},
            'scenario':scenario,'initial':initial,'rows':rows,
            'source_calls':self.constructor_calls,'source_writes':self.constructor_writes,
            'source_attachment_calls':self.attachment_calls,'source_attachment_writes':self.attachment_writes,
            'weapon_events':[{k:v for k,v in e.items() if k!='state'} for e in self.events],
            'first_person_presentation_boundary':self.presentation_boundary(),
            'supplied_services':{f'{a:08x}':{'name':n,'return':r,'stack_cleanup':pop} for a,(n,r,pop) in self.services.items()},
            'summary':{'ticks':ticks,'projectile_requests':len(requests),'request_ticks':[e['tick'] for e in requests],
                'final_weapon':self.weapon_snapshot(self.initial_datum)},
            'scope':'Actual original allocated instant starting SMG and actual source parent/marker attachment; original device-record conversion/unit wrapper, real weapon-manager clock and full weapon update, unit action/animation/local-channel and projectile postpass. Explicit supplied failed projectile creation/clear muzzle/world effects and inherited backing/residency services. No physical DE960/item F9B10/full recursive world update or first-person presentation clock/pose is claimed. Actual field snapshots resolve allocator datum10002, not inherited fake datum10000.'}

    def authored_initial_values(self):
        c=self.cache;tag=c.tags[0xE57B0406];mag=c.block(tag['offset']+0x2C0,0x5C)[0]
        own=c.tags[0xE5860411];own_records=[]
        for index,record in enumerate(c.block(own['offset']+0x2C,0x6C)):
            sid=c.u(record);name=c.text(c.u(368)+c.u(c.u(364)+(sid&0xFFFF)*4))
            own_records.append({'index':index,'name':name,'file_offset':record,'frame_count':c.h(record+0x14),
                'raw_hex':bytes(c.data[record:record+0x6C]).hex()})
        fp=c.block(tag['offset']+0x2A8,16)[0];graph=c.tags[c.u(fp+0xC)]
        fp_records=c.block(graph['offset']+0x2C,0x6C);ready=fp_records[29];selected_fp=[]
        for index in (29,30,31):
            record=fp_records[index];sid=c.u(record);name=c.text(c.u(368)+c.u(c.u(364)+(sid&0xFFFF)*4))
            selected_fp.append({'index':index,'name':name,'file_offset':record,
                'raw_hex':bytes(c.data[record:record+0x6C]).hex(),'frame_count':c.h(record+0x14),
                'events':[{'file_offset':e,'type':c.h(e),'frame':c.h(e+2),
                    'raw_hex':bytes(c.data[e:e+4]).hex()} for e in c.block(record+0x4C,4)]})
        return {'weapon':tag,'magazine_file_offset':mag,
            'magazine_raw_hex':bytes(c.data[mag:mag+0x5C]).hex(),
            'magazine_06_total':c.h(mag+6),'magazine_0a_loaded':c.h(mag+0xA),
            'magazine_0c_reserve_limit':c.h(mag+0xC),
            'weapon_own_graph':own,'weapon_own_animations':own_records,
            'executed_clock_constants':{'46516c_bits':f'{self.u(0x46516C):08x}',
                'time_4_step_bits':f'{self.u(TIME+4):08x}'},
            'first_person_record_file_offset':fp,'first_person_graph':graph,
            'first_person_ready_reload_records':selected_fp,
            'first_person_ready_record':{'index':29,'file_offset':ready,
                'raw_hex':bytes(c.data[ready:ready+0x6C]).hex(),
                'frame_count':c.h(ready+0x14),
                'events':[{'file_offset':e,'type':c.h(e),'frame':c.h(e+2),
                    'raw_hex':bytes(c.data[e:e+4]).hex()} for e in c.block(ready+0x4C,4)]}}

def inventory(directory):
    directory=directory.resolve();directory.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert hashlib.sha256(x.data).hexdigest()==PIN
    c=_Code(x,'.text');starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')};rows=[]
    for entry in dict.fromkeys(ENTRIES+(0x16A356,)):
        f=_trace(c,entry,starts);inst=[c.at(a) for a in sorted(f.code)];p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in inst))
        rows.append({'entry':f'{entry:08x}','end_exclusive':f'{f.end:08x}','listing':str(p.relative_to(ROOT)),
            'listing_sha256':hashlib.sha256(p.read_bytes()).hexdigest(),
            'instruction_bytes_sha256':hashlib.sha256(b''.join(i.bytes for i in inst)).hexdigest()})
    (directory/'inventory.json').write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')

def main():
    assert hashlib.sha256((ROOT/'tools/oracle_mp_inventory_equipped_1_1.py').read_bytes()).hexdigest()==INVENTORY_PIN
    assert hashlib.sha256((ROOT/'tools/oracle_mp_weapon_attachment_1_1.py').read_bytes()).hexdigest()==ATTACHMENT_PIN
    assert hashlib.sha256((ROOT/'tools/oracle_spartan_equipped_unit_phase_1_1.py').read_bytes()).hexdigest()==UNIT_PHASE_PIN
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/map-pack/smg-constructor-equipped-1.1-oracle.json');p.add_argument('--instruction-inventory',type=Path);p.add_argument('--suite',type=Path)
    args=p.parse_args()
    if args.instruction_inventory:inventory(args.instruction_inventory);return
    if args.suite:
        directory=args.suite.resolve();directory.mkdir(parents=True,exist_ok=True);cases=[]
        for scenario,ticks in (('idle',22),('held',22),('fire',60),('reload_rejected_45',135),('reload_accepted_46',135),('held_empty',225)):
            t=ConstructorTrace();r=t.execute_initial(ticks,scenario);path=directory/f'{scenario}.json'
            path.write_text(json.dumps(r,indent=2)+'\n')
            row={'name':scenario,'path':str(path.relative_to(ROOT)),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                'ticks':ticks,'requests':r['summary']['projectile_requests']};cases.append(row);print(json.dumps(row),flush=True)
        (directory/'suite.json').write_text(json.dumps({'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
            'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'cases':cases,'ticks':sum(c['ticks'] for c in cases),'requests':sum(c['requests'] for c in cases)},indent=2)+'\n');return
    t=ConstructorTrace();r=t.report();args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(r,indent=2)+'\n')
    print(json.dumps({'output':str(args.output),'initial':r['actual_original_initial_loadout']['weapon'],
        'timed':r['explicit_timed_handler_case']['after']}))

if __name__=='__main__':main()
