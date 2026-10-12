#!/usr/bin/env python3
"""Independent original FPS allocation, initialization and equipped registration.

Prior frozen allocated/equipped traces are imported unchanged. Heap backing,
empty first-person effects storage and the view-clock service are named inputs;
the original registration and animation-manager assignment execute.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from oracle_smg_constructor_equipped_1_1 import ConstructorTrace,ROOT,PIN,CACHE_PIN,Xbe,_Code,_trace
from oracle_smg_firing_1_1 import (UNIT,UI,HEADERS,TIME,USERS,PLAYER_DATA,STOP,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP)

DEPENDENCY='1c6ce60d4576f37548f822bfbd5667f3e97d806926db593cd98cb9d7f6c9eb99'
ARENA=0x7600000
OUT=ROOT/'analysis/multiplayer/map-pack/smg-fp-registration'
ENTRIES=(0x1677C3,0x125490,0x1254D0,0x1677E5,0x1678C1,0x14F5B0,
    0x169D8A,0x169965,0x169E15,0x179150,0xCD1A0,0x11CF00,
    0x1CEAC0,0x1CF7A0,0x1E0EB0,0x168483,0x1D0A60,0x169ED9,
    0x167D32,0x10A140,0xEFCC0,0x16800D,0x169D7A,
    0x14D600,0x14D3D0,0x159730,0x14D730)
LISTINGS=ENTRIES+(0x169DDC,0x168052,0x1CE9B0,0x1C9D30,0x1CF7D0,
    0x1CF3C0,0x1CEF10,0x1C9DE0,0x14D800,0x153B10,0x139750)

class RegistrationTrace(ConstructorTrace):
    def __init__(self):
        self.registration_enabled=False
        self.registration_calls=[];self.registration_returns=[];self.registration_writes=[]
        self.registration_steps=[];self.registration_executed=set()
        self.profile_input=None
        super().__init__()
        self.g.mem_map(ARENA,0x20000)
        self.w(0x4ED650,ARENA);self.w(0x4ED654,0);self.w(0x4ED65C,-1)
        # Empty original first-person effect datum pool is a supplied external
        # lifecycle state. Original 179150/16D680 still traverse it.
        self.w(0x4F1F0C,ARENA+0x18000);self.w(ARENA+0x18038,0)
        self.services[0x168866]=('supplied first-person per-view clock/presentation excluded from registration slice',0,4)
        self.registration_enabled=True

    def binding(self):
        p=self.u(0x4F1198)
        return {'globals_pointer':f'{p:08x}','scratch_pointer':f'{self.u(0x4F1194):08x}',
            'allocator_used':self.u(0x4ED654),'allocator_crc':f'{self.u(0x4ED65C):08x}',
            'user_player_datums':[f'{self.u(USERS+0xC+i*4):08x}'for i in range(4)],
            'player_current_unit_2c':f'{self.u(PLAYER_DATA+0x2C):08x}',
            'player_selector_88':self.u(PLAYER_DATA+0x88,1),
            'unit_current_indices_hex':bytes(self.g.mem_read(UNIT+0x212,2)).hex(),
            'unit_inventory_datums':[f'{self.u(UNIT+0x218+i*4):08x}'for i in range(4)],
            'unit_hand_timers':[self.u(UNIT+0x1FA,1),self.u(UNIT+0x1FB,1)],
            'views':[{'index':v,'flags':f'{self.u(p+v*0x20CC):08x}',
                'unit':f'{self.u(p+v*0x20CC+4):08x}','selector':f'{self.u(p+v*0x20CC+8):08x}',
                'hands':[{'index':h,'flags':f'{self.u(p+v*0x20CC+h*0x1010+0xC):08x}',
                    'weapon':f'{self.u(p+v*0x20CC+h*0x1010+0x10):08x}',
                    'weapon_node_count':self.u(p+v*0x20CC+h*0x1010+0x100),
                    'arms_node_count':self.u(p+v*0x20CC+h*0x1010+0x104),
                    'weapon_node_map_hex':bytes(self.g.mem_read(p+v*0x20CC+h*0x1010+0x108,64*4)).hex(),
                    'arms_node_map_hex':bytes(self.g.mem_read(p+v*0x20CC+h*0x1010+0x208,64*4)).hex(),
                    'channel':{'graph':f'{self.u(p+v*0x20CC+h*0x1010+0x14):08x}',
                        'animation_identifier':f'{self.u(p+v*0x20CC+h*0x1010+0x18):08x}',
                        'flags_11':f'{self.u(p+v*0x20CC+h*0x1010+0x25,1):02x}',
                        'flags_12':f'{self.u(p+v*0x20CC+h*0x1010+0x26,2):04x}',
                        'rate_18_bits':f'{self.u(p+v*0x20CC+h*0x1010+0x2C):08x}',
                        'frame_1c_bits':f'{self.u(p+v*0x20CC+h*0x1010+0x30):08x}'},
                    'manager_hex':bytes(self.g.mem_read(p+v*0x20CC+h*0x1010+0x14,0x90)).hex()}
                    for h in range(2)]}for v in range(4)]}

    def finish_registration(self,address):
        for ret,row in self.registration_returns[:]:
            if ret==address:
                row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                row['after']=self.binding()
                self.registration_returns.remove((ret,row))

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs)
        if self.registration_enabled:self.finish_registration(STOP)

    def hook(self,g,a,size,user):
        if self.registration_enabled:
            self.registration_executed.add(a);self.finish_registration(a)
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP)
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                    'return_address':f'{self.u(sp):08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)],'before':self.binding()}
                self.registration_calls.append(row);self.registration_returns.append((self.u(sp),row))
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.registration_enabled:
            p=self.u(0x4F1198)
            if p<=address and address+size<=p+0x8330:
                self.registration_writes.append({'tick':self.tick,'phase':self.active,
                    'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','offset':f'{address-p:04x}',
                    'size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
                        'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
            if PLAYER_DATA+0x44<=address and address+size<=PLAYER_DATA+0xD4:
                self.registration_writes.append({'tick':self.tick,'phase':self.active,
                    'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}','player_offset':f'{address-PLAYER_DATA:03x}',
                    'size':size,'before_hex':bytes(g.mem_read(address,size)).hex(),
                    'after_hex':(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()})
        super().mem_write(g,access,address,size,value,user)

    def step(self,name,entry,args=(),regs=()):
        self.active=name;before=self.binding();start=len(self.registration_calls);stores=len(self.registration_writes)
        self.run(entry,args,regs)
        row={'name':name,'entry':f'{entry:08x}','arguments':list(args),
            'before':before,'after':self.binding(),
            'call_range':[start,len(self.registration_calls)],'write_range':[stores,len(self.registration_writes)]}
        self.registration_steps.append(row);return row

    def authored_registration(self):
        c=self.cache
        def record(address,size):
            raw=bytes(self.g.mem_read(address,size));off=address-c.magic
            assert raw==bytes(c.data[off:off+size])
            return {'address':f'{address:08x}','file_offset':off,'raw_hex':raw.hex()}
        def sid(value):return c.text(c.u(368)+c.u(c.u(364)+(value&0xFFFF)*4))if value else 'NONE'
        globals_=next(t for t in c.tags.values()if t['group']=='matg')
        representations=c.block(globals_['offset']+0x138,0xBC)
        weapon=c.tags[0xE57B0406];fp=c.block(weapon['offset']+0x2A8,16)[0]
        graph=c.tags[c.u(fp+0xC)];g=graph['offset']
        models={c.u(fp+4)}|{c.u(p+4)for p in representations if c.u(p+4)!=0xFFFFFFFF}
        render=[]
        for datum in sorted(models):
            tag=c.tags[datum]
            nodes=c.block(tag['offset']+0x48,0x60)
            render.append({'tag':tag,'nodes':[{'index':i,'string_id':f'{c.u(p):08x}','name':sid(c.u(p)),
                'record':record(p+c.magic,0x60)}for i,p in enumerate(nodes)]})
        clips=c.block(g+0x2C,0x6C)
        indices={int.from_bytes(bytes.fromhex(h['manager_hex'])[6:8],'little')
            for step in self.registration_steps for v in step['after']['views']for h in v['hands']
            if h['weapon']!='ffffffff' and bytes.fromhex(h['manager_hex'])[:4]==struct.pack('<I',int(graph['datum'],16))}
        return {'globals':globals_,'representation_block':record(globals_['offset']+0x138+c.magic,8),
            'representations':[{'index':i,'record':record(p+c.magic,0xBC)}for i,p in enumerate(representations)],
            'weapon':weapon,'weapon_first_person_record':record(fp+c.magic,16),
            'graph':graph,'graph_nodes':[{'index':i,'string_id':f'{c.u(p):08x}','name':sid(c.u(p)),
                'record':record(p+c.magic,0x20)}for i,p in enumerate(c.block(g+0xC,0x20))],
            'models':render,'selected_clips':[{'index':i,'name':sid(c.u(clips[i])),
                'frames':c.h(clips[i]+0x14),'record':record(clips[i]+c.magic,0x6C)}
                for i in sorted(indices)if i<len(clips)]}

def dump_instructions(t,directory):
    directory.mkdir(parents=True,exist_ok=True);rows=[]
    for entry in LISTINGS:
        f=_trace(t.code,entry,t.starts|set(LISTINGS));instructions=[t.code.at(a)for a in sorted(f.code)]
        p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in instructions))
        rows.append({'entry':f'{entry:08x}','path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'instructions':len(instructions),
            'executed_in_registration_slice':sum(i.address in t.registration_executed for i in instructions),
            'direct_calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
    p=directory/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
    return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,default=OUT/'report.json')
    ap.add_argument('--instructions',type=Path,default=OUT/'instructions');args=ap.parse_args()
    assert hashlib.sha256((ROOT/'tools/oracle_smg_constructor_equipped_1_1.py').read_bytes()).hexdigest()==DEPENDENCY
    t=RegistrationTrace()
    t.step('original_fps_global_allocation',0x1677C3)
    t.step('original_fps_global_initialization',0x1677E5)
    t.step('original_registration_with_unconverted_profile_selector_zero',0x1678C1)
    assert t.binding()['views'][0]['hands'][0]['weapon']=='ffffffff'
    profile=ARENA+0x19000
    t.g.mem_write(profile,bytes(t.g.mem_read(PLAYER_DATA+0xD4,0x90)))
    t.profile_input={'address':f'{profile:08x}','source':'explicit existing unsanitized player profile block +D4',
        'raw_hex':bytes(t.g.mem_read(profile,0x90)).hex(),'options_state':t.u(t.u(0x4EDF18)+8)}
    t.step('original_multiplayer_profile_conversion',0x14D600,(0,),((UC_X86_REG_EAX,profile),))
    t.step('original_observer_after_profile_change_with_same_unit',0x1678C1)
    assert t.binding()['views'][0]['selector']=='00000000'
    assert t.binding()['views'][0]['hands'][0]['weapon']=='ffffffff'
    t.step('original_view_release_between_profile_controls',0x169D7A,(0,))
    t.step('original_equipped_view_weapon_registration',0x1678C1)
    t.step('original_registered_weapon_lookup',0x167D32,(t.initial_datum,))
    assert t.registration_calls[-1]['return_eax']=='00000000'
    assert t.u(PLAYER_DATA+0x88,1)==2
    assert t.binding()['views'][0]['hands'][0]['weapon']==f'{t.initial_datum:08x}'
    before=t.binding();start=len(t.registration_calls)
    fired=t.execute_initial(2,'held')
    assert [r['after_weapon']['weapon']['unit_hand_timers']for r in fired['rows']]==[[9,0],[8,0]]
    assert any(c['entry']=='00167d32' and c['phase']=='original_allocated_weapon_update'
        and c['stack'][0]==f'{t.initial_datum:08x}' and c['return_eax']=='00000000'for c in t.registration_calls)
    t.registration_steps.append({'name':'original_registered_two_firing_ticks','before':before,'after':t.binding(),
        'call_range':[start,len(t.registration_calls)],'request_ticks':fired['summary']['request_ticks'],
        'hand_timer_phases':[{'tick':r['tick'],'before':r['before']['weapon']['unit_hand_timers'],
            'after_unit':r['after_unit']['weapon']['unit_hand_timers'],
            'after_weapon':r['after_weapon']['weapon']['unit_hand_timers'],
            'after_postpass':r['after_postpass']['weapon']['unit_hand_timers']}for r in fired['rows']]})
    t.g.mem_write(UNIT+0x216,b'\xff')
    t.step('original_observer_after_supplied_requested_index_none',0x1678C1)
    assert t.binding()['views'][0]['hands'][0]['weapon']==f'{t.initial_datum:08x}'
    t.g.mem_write(UNIT+0x216,b'\x00')
    # A caller-supplied current-weapon index boundary, not a source inventory
    # switch. The original observer re-queries the current index via CD1A0.
    t.g.mem_write(UNIT+0x212,b'\xff')
    t.step('original_observer_after_supplied_current_index_none',0x1678C1)
    t.step('original_lookup_after_current_index_none',0x167D32,(t.initial_datum,))
    assert t.registration_calls[-1]['return_eax']=='ffffffff'
    t.g.mem_write(UNIT+0x212,b'\x00')
    t.step('original_observer_after_supplied_current_index_restore',0x1678C1)
    t.step('original_view_release_handler',0x169D7A,(0,))
    t.step('original_lookup_after_view_release',0x167D32,(t.initial_datum,))
    assert t.registration_calls[-1]['return_eax']=='ffffffff'
    print(json.dumps({'calls':len(t.registration_calls),'stores':len(t.registration_writes),
        'selector':t.u(PLAYER_DATA+0x88,1),'lookup_after_release':t.registration_calls[-1]['return_eax']}))
    corroboration=ROOT/'analysis/multiplayer/integration/spawn-source/datums-report.json'
    corroboration_sha=hashlib.sha256(corroboration.read_bytes()).hexdigest()
    assert corroboration_sha=='9d93d609507bd352f13d6bd9c202071f26b05ee2ea4bb942a87ce3fcdf945629'
    joined=json.loads(corroboration.read_text())['lifecycle_cases'][0]['stages'][0]['state']['player_pool']['occupied'][0]
    assert bytes.fromhex(joined['record_hex'])[0x88]==2
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'dependency_sha256':DEPENDENCY,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'steps':t.registration_steps,'calls':t.registration_calls,'writes':t.registration_writes,
        'profile_input':t.profile_input,'metadata':t.authored_registration(),
        'instruction_inventory':dump_instructions(t,args.instructions),
        'independent_join_profile_corroboration':{'path':str(corroboration.relative_to(ROOT)),
            'sha256':corroboration_sha,'json_path':'lifecycle_cases[0].stages[0].state.player_pool.occupied[0].record_hex',
            'player_offset':'088','value':2,'scope':'Independent existing source join report; not replayed by this registration tool.'},
        'external_inputs':{'heap_backing':{'address':f'{ARENA:08x}','capacity':0x20000,
            'allocator_used_initial':0,'allocator_crc_initial':'ffffffff'},
            'effects_pool':{'address':f'{ARENA+0x18000:08x}','active_high_water_count':0},
            'supplied_inventory_index_changes':[{'field':'unit+212','value':'ff'},
                {'field':'unit+212','value':'00'},{'field':'unit+216','value':'ff'},
                {'field':'unit+216','value':'00'}]},
        'scope':'Original FPS allocation/init, profile sanitizer/copy, carried-weapon/model/graph/node-name lookup, slot registration/removal, original two-tick fire feedback. Explicit profile, heap backing, empty effect pool and inherited service boundaries. 168866 view clock/presentation and 16A356 animation notification remain supplied services. No native runtime or prior frozen suite is edited.',
        'supplied_services':{f'{a:08x}':{'name':n,'return':r,'cleanup':p}for a,(n,r,p)in t.services.items()},
        'native_gameplay_update_applied':False}
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'path':str(args.output),'sha256':hashlib.sha256(args.output.read_bytes()).hexdigest()}))

if __name__=='__main__':main()
