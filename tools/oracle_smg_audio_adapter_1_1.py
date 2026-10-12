#!/usr/bin/env python3
"""Separate source pool producers/permutation kernels for an audio adapter.

No initialized/available bit is forced. Original constructor fragments execute
against named backing storage; source playback remains unavailable. Selection
kernels are direct bounded probes, not accepted/successful playback requests.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from oracle_smg_fp_sound_1_1 import SoundTrace,BoundaryReached,AUDIO_ARENA,AUDIO_CAPACITY
from oracle_smg_firing_1_1 import (ROOT,PIN,CACHE_PIN,SEED,UNIT,STOP,TAGS,
    UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,
    UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP)
from oracle_smg_constructor_equipped_1_1 import _trace

SOUND_PIN='44777fe55191fe36ccdef4cca53afa3ab3b5720057bc18229ce8e9698111e4a6'
OUT=ROOT/'analysis/multiplayer/map-pack/smg-audio-adapter'
CLASS_ARENA=0x7C23000
REQUEST_ARENA=0x7C24000
ENTRIES=(0x11E340,0x16D070,0x16D220,0x16D2C0,0x21BC50,0x21C1D0,
    0x21F5A0,0x21FEB0,0x32803E,0x336983,0x21B180,0x21B4E0,0x21B420,
    0x21B3C0,0x21B5D0,0x21B480,0x21B220,0x21B670,0x1269B0,0x126A40,
    0x126930,0x21AE80,0x21AEF0,0x21B700,0x21B900,0x128760,
    0x129450,0x128610,0x127750,0x129E00,0x1280B0,0x1275B0,
    0x21A960,0x21AB20,0x13EAE0,0x19D8C0,0x223960,0x2239B0,
    0x2239F0,0x223A00,0x223A10,0x223A90,0x129570,0x2247C0,0x21B060)
POOL_SLOTS=(0x4ED94C,0x526468,0x5098E8,0x526470,0x5098EC)

class SliceEnd(RuntimeError):pass

class AdapterTrace(SoundTrace):
    def __init__(self):
        self.adapter_enabled=False;self.adapter_calls=[];self.adapter_returns=[]
        self.adapter_writes=[];self.adapter_rng=[];self.adapter_executed=set()
        self.slice_end=None
        super().__init__();self.adapter_enabled=True

    def pool_state(self):
        rows=[]
        for slot in POOL_SLOTS:
            p=self.u(slot);row={'slot':f'{slot:08x}','pointer':f'{p:08x}'}
            if p:
                capacity=self.u(p+0x20);stride=self.u(p+0x24)
                data=self.u(p+0x44);bitmap=self.u(p+0x48)
                row.update(header_hex=bytes(self.g.mem_read(p,0x4C)).hex(),
                    capacity=capacity,stride=stride,valid=self.u(p+0x29,1),
                    first_free=self.u(p+0x34),high_water=self.u(p+0x38),
                    occupied=self.u(p+0x3C),salt=self.u(p+0x40,2),
                    bitmap_hex=bytes(self.g.mem_read(bitmap,(capacity+31)//32*4)).hex(),
                    data_pointer=f'{data:08x}')
                if self.u(p+0x38):
                    row['occupied_data_prefix_hex']=bytes(self.g.mem_read(data,self.u(p+0x38)*stride)).hex()
            rows.append(row)
        return {'audio':self.audio_state(),'pools':rows,
            'class_fade_pointer':f'{self.u(0x5098F0):08x}',
            'class_fade_initialized_flag':self.u(0x5098F4,1),
            'class_fade_360_hex':None if not self.u(0x5098F0)else bytes(self.g.mem_read(self.u(0x5098F0),0x360)).hex(),
            'controller_root_1c_hex':None if not self.u(0x52646C)else bytes(self.g.mem_read(self.u(0x52646C),0x1C)).hex(),
            'voice_static_1200_hex':None if not self.u(0x4ED948)else bytes(self.g.mem_read(self.u(0x4ED948),0x1200)).hex()}

    def selection_state(self,datum):
        c=self.cache;s=c.tags[datum]['offset'];g=self.u(0x526464)
        ranges=[]
        for pitch in range(c.data[s+10]):
            p=self.u(g+0x24)+(c.h(s+8)+pitch)*12
            flag_pointer=self.u(g+0x3C)+(self.u(p+6,2)>>3)
            count=self.u(p+10,2)
            ranges.append({'pitch':pitch,'pointer':f'{p:08x}',
                'range_runtime_hex':bytes(self.g.mem_read(p,12)).hex(),
                'runtime_flag_pointer':f'{flag_pointer:08x}',
                'runtime_flag_hex':bytes(self.g.mem_read(flag_pointer,(count+7)//8)).hex()})
        return {'local_seed':f'{self.u(SEED+4):08x}','sound':c.tags[datum],'ranges':ranges,
            'audio_initialized_78':self.u(self.u(0x4ED950)+0x78,1)}

    def hook(self,g,a,size,user):
        if self.adapter_enabled:
            self.adapter_executed.add(a)
            for ret,row in self.adapter_returns[:]:
                if a==ret:
                    row['return_eax']=f'{g.reg_read(UC_X86_REG_EAX):08x}'
                    row['local_seed_after']=f'{self.u(SEED+4):08x}'
                    self.adapter_returns.remove((ret,row))
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP)
                row={'phase':self.active,'entry':f'{a:08x}','registers':self.registers(),
                    'return_address':f'{self.u(sp):08x}',
                    'stack':[f'{self.u(sp+4+i*4):08x}'for i in range(6)],
                    'local_seed_before':f'{self.u(SEED+4):08x}'}
                self.adapter_calls.append(row);self.adapter_returns.append((self.u(sp),row))
            if a==self.slice_end:
                self.slice_end=None
                raise SliceEnd()
            if a==0x13EAE0 and self.u(0x5098E4)==0:
                self.boundaries.append({'phase':self.active,'instruction':f'{a:08x}',
                    'dependency':'original sound cache allocator unavailable: genuine 21A6E0 constructor stopped at NtAllocateVirtualMemory',
                    'cache_allocator_slot':'005098e4','cache_allocator_pointer':'00000000',
                    'registers':self.registers(),
                    'stack_32_hex':bytes(g.mem_read(g.reg_read(UC_X86_REG_ESP),32)).hex(),
                    'platform_result_supplied':False})
                raise BoundaryReached('original sound cache allocator unavailable')
            if a==0x336983:
                self.boundaries.append({'phase':self.active,'instruction':f'{a:08x}',
                    'dependency':'KeQuerySystemTime through original GetSystemTimeAsFileTime import',
                    'import_slot':'00417bc8','original_import_value':f'{self.u(0x417BC8):08x}',
                    'registers':self.registers(),
                    'stack_32_hex':bytes(g.mem_read(g.reg_read(UC_X86_REG_ESP),32)).hex(),
                    'platform_result_supplied':False})
                raise BoundaryReached('KeQuerySystemTime')
        super().hook(g,a,size,user)

    def run(self,a,argv=(),regs=()):
        super().run(a,argv,regs)
        if self.adapter_enabled:
            for ret,row in self.adapter_returns[:]:
                if ret==STOP:
                    row['return_eax']=f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                    row['local_seed_after']=f'{self.u(SEED+4):08x}'
                    self.adapter_returns.remove((ret,row))

    def mem_write(self,g,access,address,size,value,user):
        if self.adapter_enabled:
            if address==SEED+4:
                self.adapter_rng.append({'phase':self.active,
                    'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                    'before':f'{self.u(SEED+4):08x}','after':f'{value&0xFFFFFFFF:08x}'})
            if AUDIO_ARENA<=address<CLASS_ARENA+0x1000 or 0x526464<=address<0x526478 or 0x5098F0<=address<0x5098F8:
                self.adapter_writes.append({'phase':self.active,
                    'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                    'address':f'{address:08x}','size':size,
                    'before_hex':bytes(g.mem_read(address,size)).hex(),
                    'after_hex':(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()})
        super().mem_write(g,access,address,size,value,user)

    def original_slice(self,name,start,end,regs=()):
        self.slice_end=end;self.active=name;before=self.pool_state()
        try:self.run(start,regs=regs)
        except SliceEnd:pass
        assert self.slice_end is None
        return {'name':name,'entry':f'{start:08x}','end_exclusive':f'{end:08x}',
            'supplied_registers':[{ 'register':r,'value':f'{v:08x}'}for r,v in regs],
            'before':before,'after':self.pool_state(),
            'normal_full_initialization_or_driver_success_claimed':False}

    def initialize_source_pools(self):
        rows=[self.partial_initialization()]
        # Source fragment begins after the blocked sound-cache constructor.
        # This isolates only the original static source/voice pool producer;
        # it does not pretend that 21A6E0 returned successfully.
        rows.append(self.original_slice('source_sources_and_static_voice_pool_producer',0x126B8D,0x126C52,
            ((UC_X86_REG_EBX,0),)))
        rows.append(self.bounded_action('source_playback_controller_pool_until_real_kernel_time',0x21BC50))
        for _,row in self.adapter_returns:row['halted_at_dependency']='source platform initialization or fragment boundary'
        self.adapter_returns.clear()
        for name,entry in (('source_looping_pool',0x21C1D0),('source_effect_pool',0x21F5A0),('source_driver_voice_pool',0x21FEB0)):
            self.fp_action(name,entry,regs=((UC_X86_REG_EAX,0x451220),)if entry==0x21FEB0 else())
            rows.append({'name':name,'entry':f'{entry:08x}','returned_eax':f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
                'after':self.pool_state()})
        rows.append(self.original_slice('source_static_voice_and_free_bit_policy_producer_without_availability_commit',
            0x126C9D,0x126D35,((UC_X86_REG_EBX,0),)))
        assert self.u(self.u(0x4ED950)+0x78,1)==0
        return rows

    def initialize_classes_and_ambient(self):
        # This is backing storage for the original class allocator, not a
        # manufactured return to the blocked Xbox/cache/driver constructor.
        self.g.mem_map(CLASS_ARENA,0x1000)
        self.w(0x4ED650,CLASS_ARENA);self.w(0x4ED654,0)
        rows=[]
        for name,entry in (('source_class_fade_constructor_explicit_static_backing',0x223960),
                           ('source_class_fade_map_reset',0x2239B0)):
            self.fp_action(name,entry)
            rows.append({'name':name,'entry':f'{entry:08x}','after':self.pool_state()})
        rows.append(self.original_slice('source_ambient_index_reset_without_clock_return',0x126E54,0x126E63,
            ((UC_X86_REG_ECX,self.u(0x4ED950)),)))
        assert self.u(self.u(0x4ED950)+0x78,1)==0
        return {'explicit_class_backing':{'pointer':f'{CLASS_ARENA:08x}','initial_used':0,
            'size':0x1000,'complete_original_startup_not_claimed':True},'steps':rows}

    def creation_probe(self,scenario):
        classes=self.initialize_classes_and_ambient()
        if scenario=='ready':self.g.mem_write(UNIT+0x212,b'\xff')
        ticks={'ready':2,'reload':48,'empty_reload':125}[scenario]
        self.execute_fp(ticks,'idle'if scenario=='ready'else scenario)
        request=[r for r in self.audio_calls if r['entry']=='0021f220'][-1]
        assert request['return_eax']=='ffffffff'
        raw=bytes.fromhex(request['play_state_before_hex'])
        self.g.mem_map(REQUEST_ARENA,0x1000);self.g.mem_write(REQUEST_ARENA,raw)
        # The supplied boundary is the *call site*, not an accepted admission
        # result or initialized flag. The actual unavailable request above
        # remains rejected. Original 127750 is inspected as its own kernel.
        datum=int(request['stack'][0],16)
        before=self.pool_state();first_rng=len(self.adapter_rng)
        self.fp_action('direct_original_owner_bound_listener_selection',0x129450,
            (REQUEST_ARENA+0x0C,0x3F800000,REQUEST_ARENA+0xA0))
        listener=self.g.reg_read(UC_X86_REG_EAX)
        assert listener==0 and self.u(REQUEST_ARENA+0xA0)==0
        step=self.bounded_action('direct_original_source_creation_from_rejected_actual_fp_request',0x127750,
            (datum,listener,REQUEST_ARENA,0xFFFFFFFF))
        assert not step['completed'] and self.u(self.u(0x4ED94C)+0x3C)==1
        for _,row in self.adapter_returns:row['halted_at_dependency']=self.boundaries[-1]['dependency']
        self.adapter_returns.clear()
        pool=self.u(0x4ED94C);p=self.u(pool+0x44)
        return {'scenario':scenario,'class_and_ambient_source_producers':classes,
            'actual_rejected_fp_request':request,'request_copy_pointer':f'{REQUEST_ARENA:08x}',
            'request_copy_hex':raw.hex(),'original_listener':listener,
            'source_creation_kernel':step,'before':before,'after':self.pool_state(),
            'created_source_pointer':f'{p:08x}','created_source_bc_hex':bytes(self.g.mem_read(p,0xBC)).hex(),
            'rng_stores':self.adapter_rng[first_rng:],
            'direct_creation_call_site_supplied':True,
            'original_available_admission_succeeded':False,
            'clock_origin_and_scheduler_time_are_source_zero_from_partial_constructor':True,
            'sound_cache_allocation_completion_and_driver_voice_not_supplied':True,
            'successful_playback_claimed':False}

    def selector_probes(self,datum,count):
        rows=[];s=self.cache.tags[datum]['offset']+self.cache.magic
        for n in range(count):
            before=self.selection_state(datum);start=len(self.adapter_rng)
            self.fp_action('direct_original_pitch_range_zero_permutation_kernel',0x21B180,(0,),
                ((UC_X86_REG_EDI,s),))
            rows.append({'iteration':n,'before':before,'selected_permutation':self.g.reg_read(UC_X86_REG_EAX)&0xFFFF,
                'after':self.selection_state(datum),'rng_stores':self.adapter_rng[start:],
                'call_site_supplied':True,'successful_playback_claimed':False})
        return rows

    def adapter_metadata(self):
        c=self.cache
        matg=next(t for t in c.tags.values()if t['group']=='matg')
        globals_row=c.block(matg['offset']+0xC0,0x24)[0]
        classes_tag=c.tags[c.u(globals_row+4)]
        assert classes_tag['group']=='sncl'
        classes=c.block(classes_tag['offset'],0x5C)
        assert len(classes)==54
        return {'class_tag':classes_tag,
            'classes':[{'index':n,'file_offset':p,'original_va':f'{p+c.magic:08x}',
                'raw_hex':bytes(c.data[p:p+0x5C]).hex()}for n,p in enumerate(classes)],
            'owner_callback_table':{'original_va':'0044b14c','raw_hex':self.image.read(0x44B14C,32).hex(),
                'entries':[f'{v:08x}'for v in struct.unpack('<8I',self.image.read(0x44B14C,32))]},
            'constants':{f'{a:08x}':self.image.read(a,n).hex()for a,n in
                ((0x4650C0,4),(0x45120C,4),(0x4650E0,4),(0x46513C,4),
                 (0x4511A4,8),(0x4652EC,4),(0x54FA64,4),(0x447298,4))}}

def dump_instructions(t,directory):
    directory.mkdir(parents=True,exist_ok=True);rows=[]
    entries=set(ENTRIES)|{0x126B20,0x126DE0,0x128380,0x128320,0x2205C0,
        0x21A6E0,0x13E8E0,0x21AAB0,0x21AAF0,0x19D820,0x126F00,
        0x127290,0x1274B0,0x1271B0,0x128140,0x220D10,0x220F90,
        0x128B50,0x21BDD0,0x21BEA0,0x21D1F0,0x12A820,0x129770,
        0x129A20,0x12A850,0x12AAD0,0x12AB40,0x129E50,0x129C50,
        0x12A070,0x12A1B0,0x12AD30,0x12BBA0,0x12BD20,0x128A70,0x128AE0,
        0x21B3A0,0x221760,0x21FFA0,0x220160,0x2B47C0,0x12C710}
    entries|={0x18DDE0,0x18E060,0x18E0D0,0x18E2F0,0x18E390,0x12A3F0,
        0x12B670,0x12B900,0x2219B0,0x221830,0x221B90,0x2B4B30,
        0x21A920,0x223B80,0x128540,0x12BF60,0x129710}
    for e in sorted(entries):
        f=_trace(t.code,e,t.starts|entries);ins=[t.code.at(a)for a in sorted(f.code)]
        p=directory/f'{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        rows.append({'entry':f'{e:08x}','path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'instructions':len(ins),
            'executed':sum(i.address in t.adapter_executed for i in ins),
            'direct_calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
    p=directory/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
    return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--case',choices=('pools','ready_select','reload_select','ready_create','reload_create','empty_reload_create'),default='pools')
    ap.add_argument('--output',type=Path,default=OUT/'working.json');ap.add_argument('--instructions',type=Path)
    args=ap.parse_args();assert hashlib.sha256((ROOT/'tools/oracle_smg_fp_sound_1_1.py').read_bytes()).hexdigest()==SOUND_PIN
    t=AdapterTrace();init=t.initialize_source_pools();rows=[];creation=None
    if args.case.endswith('_select'):rows=t.selector_probes(0xE5AC0437 if args.case=='ready_select'else 0xE5AD0438,16)
    elif args.case.endswith('_create'):creation=t.creation_probe(args.case.removesuffix('_create'))
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'sound_dependency_sha256':SOUND_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'case':args.case,
        'source_pool_producer_steps':init,'selection_probes':rows,'source_creation_probe':creation,'calls':t.adapter_calls,
        'writes':t.adapter_writes,'rng_stores':t.adapter_rng,'boundaries':t.boundaries,
        'final_state':t.pool_state(),'authored_sound':t.authored_sound(),
        'adapter_metadata':t.adapter_metadata(),
        'source_driver_policy_451220_hex':t.image.read(0x451220,16).hex(),
        'initial_local_seed_is_explicit_fixture_boundary':True,
        'supplied_services':{f'{a:08x}':{'name':name,'return':value,'cleanup':pop}
            for a,(name,value,pop)in t.services.items()},
        'original_sound_availability_forced':False,'real_audio_platform_provider_executed':False,
        'successful_sound_initialization_or_playback':False,'native_gameplay_update_applied':False,
        'scope':'Original pool producers and direct metadata permutation kernels against real original fields. Post-cache and post-driver bodies are explicit isolated source fragments; they do not supply success to callers. Initialized78 stays zero. Hardware/kernel/clock provider stops remain genuine; local seed zero and authored runtime metadata bytes are explicit bounded inputs.'}
    if args.instructions:result['instruction_inventory']=dump_instructions(t,args.instructions.resolve())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'case':args.case,'output':str(args.output),'selection':[r['selected_permutation']for r in rows],
        'rng_stores':len(t.adapter_rng),'initialized':t.u(t.u(0x4ED950)+0x78,1)}))

if __name__=='__main__':main()
