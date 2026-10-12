#!/usr/bin/env python3
"""Independent original FP sound request and Xbox audio dependency evidence.

Does not emulate Xbox kernel allocation, kernel clock, DirectSound handles,
listeners, successful playback or source scheduler clocks. Earlier frozen
oracles are imported unchanged. Stops precede unsupported platform operations.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import UcError
from oracle_smg_fp_clock_1_1 import ClockTrace, ROOT, PIN, CACHE_PIN, _trace
from oracle_smg_firing_1_1 import (UNIT, UI, TAGS, TIME, SEED, STOP,
    UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_EDX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP)

CLOCK_PIN = '7d5548e26d3503ea1057aef3202a9285d7e9fa431d0a943c372788b64282b5d6'
OUT = ROOT / 'analysis/multiplayer/map-pack/smg-fp-sound'
AUDIO_ARENA = 0x7C00000
AUDIO_CAPACITY = 0x23000
ENTRIES = (0x12000, 0x126B20, 0x126DE0, 0x11E150, 0x11E340,
    0x21A6E0, 0x21BC50, 0x21C1D0, 0x21F5A0, 0x21FEB0, 0x2205C0,
    0x33785C, 0x337A40, 0x1687C2, 0x18AA90, 0x18AA40, 0x18ACD0,
    0x18AD70, 0x18EF20, 0x18DDE0, 0x18BA10, 0x18EAC0, 0x18EBA0,
    0x21F220, 0x21FE40, 0x128380, 0x128610, 0x127750, 0x223920,
    0xBBCB0, 0xBBEA0, 0x156EB0, 0x21ABD0, 0x21AD70)
GLOBALS = (0x517FF0, 0x517FF4, 0x517FF8, 0x4ED944, 0x4ED948,
    0x4ED94C, 0x4ED950, 0x5098DC, 0x5098E0, 0x5098E4, 0x5098E8,
    0x5098EC, 0x526464, 0x526468, 0x52646C, 0x526470, 0x526474)
PLATFORM_STOPS = {
    0x33786F: ('NtAllocateVirtualMemory', 0x417C34,
        'Original XAPILIB VirtualAlloc has prepared its original kernel arguments; kernel allocation has no supplied result.'),
    0x337A45: ('KeTickCount', 0x417CD4,
        'Original GetTickCount loaded the original unresolved kernel data import; no kernel clock is supplied.'),
}

class BoundaryReached(RuntimeError):
    pass

class SoundTrace(ClockTrace):
    def __init__(self):
        self.audio_enabled = False
        self.audio_calls = []; self.audio_returns = []; self.audio_writes = []
        self.audio_stack_writes = []
        self.audio_executed = set(); self.boundaries = []
        super().__init__()
        self.services.pop(0x18AD70)
        self.audio_enabled = True
        self.initial_audio = self.audio_state()

    def audio_state(self):
        result = {'globals': {f'{a:08x}': f'{self.u(a):08x}' for a in GLOBALS},
            'shared_seed': f'{self.u(SEED):08x}', 'local_seed': f'{self.u(SEED+4):08x}'}
        p = self.u(0x4ED950)
        if p:
            result['sound_system_pointer'] = f'{p:08x}'
            result['sound_system_20c_hex'] = bytes(self.g.mem_read(p, 0x20C)).hex()
            result['initialized_78'] = self.u(p+0x78, 1)
            result['hardware_available_79'] = self.u(p+0x79, 1)
            result['enabled_7a'] = self.u(p+0x7A, 1)
            result['clock_origin_80'] = f'{self.u(p+0x80):08x}'
            result['clock_time_84'] = f'{self.u(p+0x84):08x}'
            result['master_fade_1e4'] = f'{self.u(p+0x1E4):08x}'
        return result

    def finish_audio(self, address):
        for ret, row in self.audio_returns[:]:
            if ret == address:
                row['return_eax'] = f'{self.g.reg_read(UC_X86_REG_EAX):08x}'
                row['audio_after'] = self.audio_state()
                for key, size in (('request_pointer', 0x58), ('location_pointer', 0x28),
                                  ('play_state_pointer', 0xA0)):
                    if key in row:
                        row[key.replace('_pointer','_after_hex')] = bytes(self.g.mem_read(int(row[key],16),size)).hex()
                if 'type_pointer' in row:
                    row['type_after'] = self.u(int(row['type_pointer'],16),1)
                self.audio_returns.remove((ret,row))

    def run(self, a, argv=(), regs=()):
        super().run(a,argv,regs)
        if self.audio_enabled:
            self.finish_audio(STOP)

    def hook(self,g,a,size,user):
        if self.audio_enabled:
            self.audio_executed.add(a); self.finish_audio(a)
            if a in ENTRIES:
                sp=g.reg_read(UC_X86_REG_ESP)
                row={'tick':self.tick,'phase':self.active,'entry':f'{a:08x}',
                    'return_address':f'{self.u(sp):08x}','registers':self.registers(),
                    'stack':[f'{self.u(sp+4+i*4):08x}' for i in range(6)],
                    'stack_write_count_at_entry':len(self.audio_stack_writes),
                    'audio_before':self.audio_state()}
                if a==0x1687C2:
                    p=self.u(sp+8);row['event_hex']=bytes(g.mem_read(p,20)).hex()
                if a==0x18AD70:
                    p=self.u(sp+12);row['location_pointer']=f'{p:08x}'
                    row['location_before_hex']=bytes(g.mem_read(p,0x28)).hex()
                if a==0x18EF20:
                    p=self.u(sp+8);row['type_pointer']=f'{p:08x}';row['type_before']=self.u(p,1)
                if a==0x18DDE0:
                    p=self.u(sp+16);row['request_pointer']=f'{p:08x}'
                    row['request_before_hex']=bytes(g.mem_read(p,0x58)).hex()
                if a==0x18BA10:
                    p=g.reg_read(UC_X86_REG_EAX);row['request_pointer']=f'{p:08x}'
                    row['request_before_hex']=bytes(g.mem_read(p,0x58)).hex()
                    q=self.u(p+0x50)
                    if q:row['location_pointer']=f'{q:08x}';row['location_before_hex']=bytes(g.mem_read(q,0x28)).hex()
                if a==0x21F220:
                    p=g.reg_read(UC_X86_REG_EAX);row['play_state_pointer']=f'{p:08x}'
                    row['play_state_before_hex']=bytes(g.mem_read(p,0xA0)).hex()
                self.audio_calls.append(row);self.audio_returns.append((self.u(sp),row))
            if a in PLATFORM_STOPS:
                name,slot,reason=PLATFORM_STOPS[a];sp=g.reg_read(UC_X86_REG_ESP)
                row={'tick':self.tick,'phase':self.active,'instruction':f'{a:08x}',
                    'dependency':name,'import_slot':f'{slot:08x}',
                    'original_import_value':f'{self.u(slot):08x}',
                    'registers':self.registers(),'stack_64_hex':bytes(g.mem_read(sp,64)).hex(),
                    'audio_state':self.audio_state(),'reason':reason,
                    'platform_result_supplied':False}
                self.boundaries.append(row)
                raise BoundaryReached(name)
        super().hook(g,a,size,user)

    def mem_write(self,g,access,address,size,value,user):
        if self.audio_enabled and (any(a<=address< a+4 for a in GLOBALS) or
                                  AUDIO_ARENA<=address< AUDIO_ARENA+AUDIO_CAPACITY):
            self.audio_writes.append({'tick':self.tick,'phase':self.active,
                'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                'address':f'{address:08x}','size':size,
                'before_hex':bytes(g.mem_read(address,size)).hex(),
                'after_hex':(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()})
        if self.audio_enabled and 0x2000000<=address<0x2200000 and any(
                row['entry']=='0018ad70'for _,row in self.audio_returns):
            self.audio_stack_writes.append({'tick':self.tick,'phase':self.active,
                'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                'address':f'{address:08x}','size':size,
                'before_hex':bytes(g.mem_read(address,size)).hex(),
                'after_hex':(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()})
        super().mem_write(g,access,address,size,value,user)

    def invalid(self,g,access,address,size,value,user):
        if self.audio_enabled:
            self.boundaries.append({'tick':self.tick,'phase':self.active,
                'instruction':f'{g.reg_read(UC_X86_REG_EIP):08x}',
                'dependency':'unmapped original context','address':f'{address:08x}',
                'size':size,'registers':self.registers(),'audio_state':self.audio_state(),
                'recent':[f'{a:08x}' for a in self.recent],
                'platform_result_supplied':False})
            return False
        return super().invalid(g,access,address,size,value,user)

    def bounded_action(self,name,entry,args=(),regs=()):
        before=self.audio_state();first=len(self.boundaries)
        try:
            self.fp_action(name,entry,args,regs)
            completed=True
        except BoundaryReached:
            completed=False
            # These calls have not returned. A later independent execution
            # must not mark their shared STOP address as a completed return.
            for _,row in self.audio_returns:
                row['halted_at_dependency']=self.boundaries[-1]['dependency']
            self.audio_returns.clear()
        return {'entry':f'{entry:08x}','before':before,'after':self.audio_state(),
                'completed':completed,'boundaries':self.boundaries[first:]}

    def partial_initialization(self):
        # Explicit storage for the static arena only. No platform handle or
        # platform allocation result is returned to original instructions.
        self.g.mem_map(AUDIO_ARENA,AUDIO_CAPACITY)
        self.w(0x517FF0,AUDIO_ARENA);self.w(0x517FF4,0)
        init=self.bounded_action('original_audio_initialization_with_explicit_static_backing',0x126B20)
        assert not init['completed']
        assert self.u(0x4ED950)==AUDIO_ARENA and self.u(0x517FF4)==0x20C
        assert self.u(AUDIO_ARENA+0x78,1)==0 and self.u(AUDIO_ARENA+0x79,1)==1
        reset=self.bounded_action('original_map_sound_reset_until_kernel_clock',0x126DE0)
        assert not reset['completed']
        assert self.u(0x526464)!=0 and self.u(AUDIO_ARENA+0x7A,1)==1
        return {'explicit_input':{'arena_pointer':f'{AUDIO_ARENA:08x}',
            'capacity':AUDIO_CAPACITY,'initial_used':0,
            'original_startup_VirtualAlloc_completed':False},'initialization':init,'map_reset':reset}

    def authored_sound(self,execute_probes=False):
        c=self.cache
        def record(p,n):
            return {'file_offset':p,'original_va':f'{p+c.magic:08x}',
                'raw_hex':bytes(c.data[p:p+n]).hex()}
        matg=next(t for t in c.tags.values()if t['group']=='matg')
        global_rows=c.block(matg['offset']+0xC0,0x24)
        assert len(global_rows)==1
        global_row=global_rows[0];ugh=c.tags[c.u(global_row+0x20)]
        assert ugh['group']=='ugh!' and self.u(0x526464)==ugh['offset']+c.magic
        g=ugh['offset'];ranges=c.block(g+0x20,12);perms=c.block(g+0x28,16)
        chunks=c.block(g+0x40,12);playback=c.block(g,0x38);scales=c.block(g+8,0x14)
        pitch_parameters=c.block(g+0x18,10)
        graph=c.tags[0xE5AA0435];refs=c.block(graph['offset']+0x14,12)
        sounds=[]
        with (ROOT/'assets/disc/maps/shared.map').open('rb')as shared:
            for ref in refs:
                datum=c.u(ref+4);tag=c.tags[datum];s=tag['offset']
                assert tag['group']=='snd!'
                sound={'tag':tag,'graph_reference':record(ref,12),'definition':record(s,20),
                    'byte_2_class':c.data[s+2],'byte_3_sample_rate':c.data[s+3],
                    'byte_4_encoding':c.data[s+4],'byte_5_compression':c.data[s+5],
                    'byte_13_custom_playback_index':struct.unpack_from('<b',c.data,s+13)[0],
                    'playback_parameters':record(playback[c.h(s+6)],0x38),
                    'scale':record(scales[c.data[s+11]],0x14),'ranges':[]}
                for pitch in range(c.data[s+10]):
                    r=ranges[c.h(s+8)+pitch];first,count=c.h(r+8),c.h(r+10)
                    range_row={'pitch':pitch,'range':record(r,12),
                        'pitch_parameters':record(pitch_parameters[c.h(r+2)],10),'permutations':[]}
                    for permutation in range(count):
                        p=perms[first+permutation];chunk_first,chunk_count=c.h(p+12),c.h(p+14)
                        pr={'permutation':permutation,'record':record(p,16),
                            'chunks':[],'enumerated_not_selected_by_playback':True}
                        for n in range(chunk_count):
                            ch=chunks[chunk_first+n];pointer=c.u(ch);encoded_size=c.u(ch+4)
                            location=pointer>>30;offset=pointer&0x3FFFFFFF
                            size=encoded_size&0x3FFFFFFF
                            assert location in(0,2)
                            if location==2:shared.seek(offset);raw=shared.read(size)
                            else:raw=bytes(c.data[offset:offset+size])
                            assert len(raw)==size
                            pr['chunks'].append({'record':record(ch,12),'location':location,
                                'file':'assets/disc/maps/shared.map'if location==2 else'assets/disc/maps/lockout.map',
                                'file_offset':offset,'size_from_original_21abd0_mask':size,
                                'raw_sha256':hashlib.sha256(raw).hexdigest(),
                                'original_raw_codec_bytes_decoded':False})
                        if execute_probes:
                            self.fp_action('original_metadata_neutral_time_query',0x21AD70,(pitch,permutation),
                                ((UC_X86_REG_EAX,datum),))
                            pr['original_21ad70_word']=self.g.reg_read(UC_X86_REG_EAX)&0xFFFF
                            self.fp_action('original_metadata_chunk_byte_total_query',0x21ABD0,
                                regs=((UC_X86_REG_ECX,p+c.magic),(UC_X86_REG_EDX,chunk_count)))
                            pr['original_21abd0_byte_total']=self.g.reg_read(UC_X86_REG_EAX)
                            assert pr['original_21abd0_byte_total']==sum(x['size_from_original_21abd0_mask']for x in pr['chunks'])
                        range_row['permutations'].append(pr)
                    sound['ranges'].append(range_row)
                sounds.append(sound)
        return {'matg_tag':matg,'matg_sound_global_block':record(matg['offset']+0xC0,8),
            'matg_sound_globals':record(global_row,0x24),'ugh_tag':ugh,
            'ugh_header':record(g,0x58),'sounds':sounds,
            'format_names_navigation_only':'Assembly Halo2 snd!/ugh!/matg plugins; recorded bytes and original instruction operands remain authority',
            'source_probes_executed':execute_probes}

def dump_instructions(t,directory):
    directory.mkdir(parents=True,exist_ok=True);rows=[]
    entries=set(ENTRIES)|(set((0x21FE00,0x18E0F0,0x18E060,0x18E0D0,
        0x127540,0x192CA0,0x12CB70,0x1656A0,0x16D070,0x13E8E0)))
    for entry in sorted(entries):
        f=_trace(t.code,entry,t.starts|entries);ins=[t.code.at(a)for a in sorted(f.code)]
        p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
        rows.append({'entry':f'{entry:08x}','path':str(p.relative_to(ROOT)),
            'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'instructions':len(ins),
            'executed':sum(i.address in t.audio_executed for i in ins),
            'calls':[{'site':f'{s:08x}','target':f'{d:08x}'}for s,d,k in f.sites if k=='call']})
    p=directory/'inventory.json';p.write_text(json.dumps({'xbe_sha256':PIN,'routines':rows},indent=2)+'\n')
    return {'path':str(p.relative_to(ROOT)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--case',choices=('startup','cache_init','resources','uninitialized_ready','ready','reload','empty_reload'),default='ready')
    ap.add_argument('--output',type=Path,default=OUT/'working-report.json');ap.add_argument('--instructions',type=Path)
    args=ap.parse_args();assert hashlib.sha256((ROOT/'tools/oracle_smg_fp_clock_1_1.py').read_bytes()).hexdigest()==CLOCK_PIN
    t=SoundTrace();steps=[];rows=[];authored=None;terminal_error=None
    if args.case=='startup':
        # Begin at the actual static arena allocation argument producer, after
        # startup's unrelated disk-cache probe. EBX=0 is produced at12001.
        steps.append(t.bounded_action('original_startup_static_arena_allocation_fragment',0x12008,
            regs=((UC_X86_REG_EBX,0),)))
    elif args.case=='uninitialized_ready':
        t.g.mem_write(UNIT+0x212,b'\xff')
        try:rows=t.execute_fp(2,'idle')
        except UcError as exc:
            terminal_error=str(exc)
            assert t.boundaries[-1]['instruction']=='0021f366'
            assert t.boundaries[-1]['address']=='00000078'
            for _,row in t.audio_returns:row['halted_at_dependency']='unmapped original sound system'
            t.audio_returns.clear()
    else:
        steps.append(t.partial_initialization())
        authored=t.authored_sound(execute_probes=args.case=='resources')
        if args.case in('ready','reload','empty_reload'):
            if args.case=='ready':t.g.mem_write(UNIT+0x212,b'\xff')
            ticks={'ready':2,'reload':48,'empty_reload':125}[args.case]
            scenario='idle'if args.case=='ready'else args.case
            rows=t.execute_fp(ticks,scenario)
    result={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'clock_dependency_sha256':CLOCK_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'case':args.case,
        'steps':steps,'authored_sound':authored,'terminal_error':terminal_error,
        'initial_audio':t.initial_audio,'final_audio':t.audio_state(),
        'calls':t.audio_calls,'writes':t.audio_writes,'stack_writes':t.audio_stack_writes,
        'boundaries':t.boundaries,
        'tick_states':[{'tick':r['tick'],'after_fp':r['after_fp']}for r in rows],
        'source_fp_events':[r for r in t.audio_calls if r['entry']=='001687c2'],
        'external_inputs':t.fp_external_inputs,
        'supplied_services':{f'{a:08x}':{'name':name,'return':value,'cleanup':pop}
            for a,(name,value,pop)in t.services.items()},
        'executed_addresses':[f'{a:08x}'for a in sorted(t.audio_executed)],
        'original_18ad70_executed':any(r['entry']=='0018ad70'for r in t.audio_calls),
        'successful_audio_initialization':False,'successful_audio_playback':False,
        'platform_return_or_clock_supplied':False,'native_gameplay_update_applied':False,
        'scope':'Untouched original request/event instructions and partial source initialization. Static audio arena backing alone is explicit. Original kernel allocation and kernel clock are stopped before use, without returns; initialized remains source-produced false. Original playback request must reject this unavailable system. Existing world/resource/input fixture boundaries are retained.'}
    if args.instructions:result['instruction_inventory']=dump_instructions(t,args.instructions.resolve())
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'case':args.case,'path':str(args.output),'audio_calls':len(t.audio_calls),
        'boundaries':len(t.boundaries),'ticks':len(rows),'request_returns':[
            r.get('return_eax')for r in t.audio_calls if r['entry']=='0018ad70']}))

if __name__=='__main__':main()
