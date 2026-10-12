#!/usr/bin/env python3
"""Original127750 creation prefixes with real prior CPU constructors.

The supplied ABI is three actual rejected nonpositional FP requests and their
recorded association output. It is not admitted128380, completed startup or
voice playback. The original resource continuation retains its genuine wait.
"""
import argparse,hashlib,json,struct,time
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT,OLD,NEW,PINS,Xbe,_Code,sha,save,encoded
from oracle_sound_startup_native import StartupTrace,PRIOR,AUDIO_ARENA,AUDIO_CAPACITY
from oracle_smg_firing_1_1 import STACK,STOP
from inspect_multiplayer import Cache

DIRECTORY=ROOT/'analysis/multiplayer/integration/sound-create-native'
PAIR=DIRECTORY/'paired-v2'
RAW=ROOT/'analysis/multiplayer/map-pack/smg-audio-adapter/source-fixture.json'
RAW_PIN='525ed0246e4271a14b23c8148739010f18e1c28955889631cc35ed6781300a22'
INVENTORY=ROOT/'analysis/multiplayer/integration/sound-metadata-provider-20261008/measured/report.json'
INVENTORY_PIN='f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3'
REQUEST=0xa400000;TAGS=0x1080000
ROOTS={'system':0x4ed950,'sources':0x4ed94c,'controllers':0x526468,'class_fades':0x5098f0,'local_rng':0x4ee9d8,'environment':0x518480,'cache_pool':0x5098dc,'cache_allocator':0x5098e4,'cache_base':0x5098e0}
OPERANDS=(0x447298,0x45120c,0x452044,0x4650c0,0x465028,0x465030,0x465048,0x46508c,0x451214,0x465078,0x4650e0)
class StopPrefix(Exception):pass

def preparation(out,replay):
    assert sha(PRIOR/'report.json')=='43cf5c78f8fd6b2af8fd6e9de8c65ac249564ecda13463a340a1a333599b2075'
    assert sha(PRIOR/'raw-host-context.json')=='e72644599c86866bf4a27b5ad5a5a44e6bd65b0f556acfb3f40fe96edc0d2fd1'
    assert sha(OLD)==PINS[str(OLD)]
    prior=json.loads((PRIOR/'report.json').read_text());ctx=json.loads((PRIOR/'raw-host-context.json').read_text())
    t=StartupTrace(ctx,prior['clock_calls']);steps=t.execute()
    before=t.state();class_stores=[];class_executed=set();class_active=True
    def class_write(g,k,a,n,v,u):
        if class_active and (int(before['game_arena'],16)<=a<int(before['game_arena'],16)+0x10000 or a in(0x4ed654,0x4ed65c,0x5098f0,0x5098f4)):
            class_stores.append(dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before=f'{t.u(a,n):0{n*2}x}',after=f'{v&((1<<(n*8))-1):0{n*2}x}'))
    def class_code(g,a,n,u):
        if class_active and t.code.at(a):class_executed.add(a)
    t.g.hook_add(UC_HOOK_MEM_WRITE,class_write);t.g.hook_add(UC_HOOK_CODE,class_code)
    t.call('source_class_fade_constructor',0x223960);t.call('source_class_fade_reset',0x2239b0);class_active=False
    rows=[t.code.at(a)for a in sorted(class_executed)]
    for z in rows:assert z.bytes==t.image.read(z.address,len(z.bytes))==bytes(t.g.mem_read(z.address,len(z.bytes)))
    save(out/'instructions/old-class-constructors-executed.asm',''.join(f'{z.address:08x} {z.bytes.hex()} {z.mnemonic} {z.op_str}\n'for z in rows).encode())
    state=t.state();base=t.u(0x4ed650);cursor=t.u(0x4ed654)
    regions=[dict(address=f'{AUDIO_ARENA:08x}',raw_hex=bytes(t.g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex()),dict(address=f'{base:08x}',raw_hex=bytes(t.g.mem_read(base,cursor)).hex())]
    for q in t.raw_providers:
        if q.get('entry')=='0033785c':regions.append(dict(address=q['resulting_base'],raw_hex=bytes(t.g.mem_read(int(q['resulting_base'],16),q['rounded_size'])).hex()))
    roots={k:f'{t.u(v):08x}'for k,v in ROOTS.items()}
    # This is a real clock observation replayed through the original thunk;
    # it is not an assigned source clock origin/accumulator.
    ns=time.clock_gettime_ns(time.CLOCK_BOOTTIME)if replay is None else replay['clock_observation']['observed_ns']
    result=dict(prior_report_sha256=sha(PRIOR/'report.json'),context_sha256=sha(PRIOR/'raw-host-context.json'),
      prior_clock_calls=prior['clock_calls'],before_class=before,after_class=state,class_stores=class_stores,roots=roots,regions=regions,
      class_constructor_calls=t.producers[-2:],clock_observation=dict(provider='actual clock_gettime_ns(CLOCK_BOOTTIME)',observed_ns=ns,tick_ms_u32=(ns//1000000)&0xffffffff),
      bounds=['Whole old source heap/CRT/game-arena/RNG and126B20 constructors retained, including original cache storage and actual OS raw backing. Original2205C0 returns no result.',
        'The source game arena producer excludes217090 savegame/thread I/O with the original frame preserved. Declared static audio raw arena remains0A000000/23000.',
        '223960 and2239B0 execute unchanged using the original game arena/cursor; no class fade pointer or initialized flag is assigned.',
        'Whole prior compatibility context and measured RNG-clock inputs are replayed, not new Xbox hardware or Wine observations.'])
    save(out/'preparation.json',encoded(result));return result

def mappings(pair):
    pcs={};globals_map={};entries={}
    for q in pair['owners']:
        entries[int(q['old'],16)]=int(q['u5'],16)
        old=(PAIR/'instructions'/f'{q["old"]}-old.asm').read_text().splitlines();new=(PAIR/'instructions'/f'{q["old"]}-u5.asm').read_text().splitlines()
        for a,b in zip(old,new):pcs[int(a.split()[0],16)]=int(b.split()[0],16)
        for m in q['memory']:globals_map[int(m['old_va'],16)]=int(m['new_va'],16)
    oldcode=_Code(Xbe(OLD),'.text');newcode=_Code(Xbe(NEW),'.text')
    globals_map[0x447298]=newcode.at(pcs[0x127783]).operands[1].imm
    assert oldcode.at(0x127783).operands[1].imm==0x447298
    return pcs,globals_map,entries

def run(tag,path,item,pair,prep,out):
    p=Path(item['path']);p=p if p.is_absolute()else ROOT/p;assert sha(p)==item['sha256']and sha(path)==PINS[str(path)]
    c=Cache(p);x=Xbe(path);code=_Code(x,'.text');pcs,globals_map,entries=mappings(pair);reverse={b:a for a,b in pcs.items()}
    #126DE0 fullbody and whole clock/driver-call correspondence already
    # frozen independently; no guessed global address translation.
    pp=json.loads((ROOT/'analysis/multiplayer/integration/sound-promotion-native/paired/report.json').read_text())
    for q in pp['owners']:
        for m in q['memory']:globals_map[int(m['old_va'],16)]=int(m['new_va'],16)
    cache_pair=ROOT/'analysis/multiplayer/integration/sound-cache-startup-native/paired/report.json'
    assert sha(cache_pair)=='dc2c9ffcd5c9db76c2d9d5596e0e7c9165ed81c1068ba3e457a2b2c5d2aa4e1d'
    for q in json.loads(cache_pair.read_text())['owners']:
        for m in q['memory']:globals_map[int(m['old_va'],16)]=int(m['new_va'],16)
    va=lambda a:a if tag=='old'else globals_map[a]
    entry=lambda a:a if tag=='old'else entries[a]
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for q in x.sections:g.mem_write(q.va,x.section_bytes(q))
    intervals=[]
    def mapped(a,raw):
        lo=a&~4095;hi=(a+len(raw)+4095)&~4095
        if not any(lo>=b and hi<=e for b,e in intervals):
            assert all(hi<=b or lo>=e for b,e in intervals),(hex(lo),hex(hi),intervals)
            g.mem_map(lo,hi-lo);intervals.append((lo,hi))
        g.mem_write(a,raw)
    for r in prep['regions']:mapped(int(r['address'],16),bytes.fromhex(r['raw_hex']))
    mapped(0x2100000,bytes(0x100000));mapped(STOP,bytes(4096));mapped(REQUEST,bytes(4096));mapped(0x80000000,bytes(4096))
    resident=[q for q in c.tags.values()if q['offset']is not None];base=(min(q['offset']for q in resident)+c.magic)&~4095;end=(max(q['offset']+q['size']for q in resident)+c.magic+4095)&~4095
    mapped(base,bytes(c.data[base-c.magic:min(len(c.data),end-c.magic)]))
    index=c.u(16);table=c.u(index+8)-(c.u(index)-(index+32));table_raw=bytes(c.data[table:table+c.u(index+24)*16]);mapped(TAGS,table_raw)
    def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
    def w(a,v):g.mem_write(a,struct.pack('<I',v))
    for name,old in ROOTS.items():w(va(old),int(prep['roots'][name],16))
    w(va(0x4eb114),TAGS)
    matg=next(q for q in c.tags.values()if q['group']=='matg');sncl=next(q for q in c.tags.values()if q['group']=='sncl');ugh=next(q for q in c.tags.values()if q['group']=='ugh!')
    w(va(0x4e791c),matg['offset']+c.magic)
    # Complete owning map prefix executes; never overwrite +78/+79/+7A.
    clock_row=next(q for q in pp['owners']if q['old']=='00126de0')
    clockcall=next(q for q in clock_row['calls']if q['old_target']=='00337a40')
    clock_entry=0x337a40 if tag=='old'else int(clockcall['new_target'],16)
    clock_operand=code.at(clock_entry).operands[1].mem.disp;assert u(clock_operand)==0x8000009c
    phase='map';executed=set();writes=[];boundary=None;cache_request=None;clock_calls=0;continuation_calls=[];continuation_stop=None
    def instruction(_g,a,n,_u):
        nonlocal cache_request,clock_calls,continuation_stop
        if code.at(a):executed.add(a)
        if phase=='map'and a==clock_entry:
            w(0x8000009c,prep['clock_observation']['tick_ms_u32']);clock_calls+=1
        if phase=='continuation' and (a in(0x21a960,0x21ab20,0x13eae0,0x19d8c0,0x2dc8b2) or 0x80000000<=a<0x80001000):
            sp=g.reg_read(UC_X86_REG_ESP);row=dict(entry=f'{a:08x}',original_return_site=f'{u(sp):08x}',arguments=[f'{u(sp+4+i*4):08x}'for i in range(6)],registers={name:f'{g.reg_read(reg):08x}'for name,reg in(('eax',UC_X86_REG_EAX),('ebx',UC_X86_REG_EBX),('ecx',UC_X86_REG_ECX),('edx',UC_X86_REG_EDX),('esi',UC_X86_REG_ESI),('edi',UC_X86_REG_EDI))})
            continuation_calls.append(row)
            if 0x80000000<=a<0x80001000:
                continuation_stop=dict(**row,kernel_ordinal=a&0x7fffffff,service='NtWaitForSingleObject'if a==0x800000ea else'unnamed unprovided kernel service',result_supplied=False);raise StopPrefix()
        if phase=='create'and a==entry(0x21a960):
            sp=g.reg_read(UC_X86_REG_ESP);cache_request=dict(sound=f'{u(sp+4):08x}',chunk=f'{u(sp+8):08x}',flags=f'{u(sp+12):08x}',original_return_site=f'{u(sp):08x}')
            raise StopPrefix()
    ranges=[]
    def store(_g,k,a,n,v,_u):
        if any(lo<=a and a+n<=hi for lo,hi in ranges) or (phase=='continuation' and not(0x2100000<=a<0x2200000)):
            pc=g.reg_read(UC_X86_REG_EIP);writes.append(dict(instruction=f'{pc if tag=="old"else reverse.get(pc,pc):08x}',actual_instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}'))
    def invalid(_g,k,a,n,v,_u):
        nonlocal boundary
        boundary=dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,access=k);return False
    g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,store)
    from unicorn import UC_HOOK_MEM_INVALID
    g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.mem_write(STACK,struct.pack('<I',STOP));g.reg_write(UC_X86_REG_ESP,STACK)
    try:g.emu_start(0x126de0 if tag=='old'else 0x1281a0,STOP,count=1000000)
    except UcError:assert boundary
    assert boundary['instruction']==('002217e8'if tag=='old'else'002260e8')and clock_calls==1
    system=int(prep['roots']['system'],16);assert u(system+0x78,1)==0 and u(va(0x526464))==ugh['offset']+c.magic
    map_boundary=boundary
    regions=[dict(address=f'{TAGS:08x}',raw_hex=table_raw.hex())]
    regions+=prep['regions'][:2]
    regions[1]=dict(address=f'{AUDIO_ARENA:08x}',raw_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex())
    for q in (matg,sncl,ugh):regions.append(dict(address=f'{q["offset"]+c.magic:08x}',raw_hex=bytes(g.mem_read(q['offset']+c.magic,q['size'])).hex()))
    # Full immutable sound records are distinct from shared mutable ugh state.
    sounds=[q for q in c.tags.values()if q['group']=='snd!'and q['offset']is not None]
    for q in sounds:regions.append(dict(address=f'{q["offset"]+c.magic:08x}',raw_hex=bytes(g.mem_read(q['offset']+c.magic,20)).hex()))
    regions.append(dict(address=f'{va(0x54fa64):08x}',raw_hex=bytes(g.mem_read(va(0x54fa64),4)).hex()))
    regions.append(dict(address=f'{REQUEST:08x}',raw_hex='00'*160))
    ranges=[(int(r['address'],16),int(r['address'],16)+len(bytes.fromhex(r['raw_hex'])))for r in regions]
    state={**prep['roots'],'tag_table':f'{TAGS:08x}','current_matg':f'{matg["offset"]+c.magic:08x}','current_ugh':f'{ugh["offset"]+c.magic:08x}',
           'current_scenario':f'{u(va(0x4e7920)):08x}','delay_scale_operand':f'{va(0x54fa64):08x}'}
    source_rows=json.loads(RAW.read_text())['creation_kernel_probes'];cases=[];phase='create'
    for q in source_rows:
        # Every independent raw owner probe restarts from identical complete
        # source-created constructor/map-prefix buffers, not selected fields.
        for r in regions:g.mem_write(int(r['address'],16),bytes.fromhex(r['raw_hex']))
        raw=bytes.fromhex(q['actual_rejected_fp_request']['play_state_before_hex']);g.mem_write(REQUEST,raw)
        old_sound=int(q['actual_rejected_fp_request']['stack'][0],16)
        oldc=Cache(ROOT/'assets/disc/maps/lockout.map');name=oldc.tags[old_sound]['name'];oldc.data.close();oldc.stream.close()
        sound=next(v for v in sounds if v['name']==name);datum=int(sound['datum'],16)
        writes=[];cache_request=None;boundary=None
        g.mem_write(STACK,struct.pack('<IIIII',STOP,datum,q['original_listener'],REQUEST,0xffffffff));g.reg_write(UC_X86_REG_ESP,STACK)
        try:g.emu_start(entry(0x127750),STOP,count=1000000)
        except StopPrefix:pass
        except UcError:assert boundary
        returned=g.reg_read(UC_X86_REG_EIP)==STOP
        assert cache_request and not returned and boundary is None
        source=u(int(state['sources'],16)+0x44)
        cases.append(dict(scenario=q['scenario'],sound=sound['datum'],sound_name=name,raw_request_hex=raw.hex(),local_association=q['original_listener'],promotion_selector='ffffffff',
          stores=writes,cache_request=cache_request,source_bc_hex=bytes(g.mem_read(source,0xbc)).hex(),
          final_rng_hex=bytes(g.mem_read(int(state['local_rng'],16),8)).hex(),final_ugh_hex=bytes(g.mem_read(ugh['offset']+c.magic,ugh['size'])).hex(),
          final_arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex(),final_request_hex=bytes(g.mem_read(REQUEST,160)).hex(),whole_owner_returned=False))
    continuation=None
    if tag=='old'and p.stem=='lockout':
        # The last actual raw prefix retains its real source frame. Continue
        # from that exact resource entry, never a manufactured return.
        phase='continuation';writes=[];boundary=None
        cache_regions=[dict(address=q['address'],before_hex=bytes(g.mem_read(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))).hex())for q in prep['regions'][2:]]
        try:g.emu_start(0x21a960,STOP,count=1000000)
        except StopPrefix:pass
        except UcError:assert boundary
        for r in cache_regions:r['after_hex']=bytes(g.mem_read(int(r['address'],16),len(bytes.fromhex(r['before_hex'])))).hex()
        assert continuation_stop and continuation_stop['kernel_ordinal']==234,(continuation_stop,boundary)
        assert continuation_stop['arguments'][:4]==['00000000','00000001','00000000','00000000']
        continuation=dict(from_case='empty_reload',calls=continuation_calls,stores=writes,cache_regions=cache_regions,stop=continuation_stop,whole127750_returned=False,voice_or_IO_success_supplied=False,source_bc_hex=bytes(g.mem_read(source,0xbc)).hex())
    listing=out/'instructions'/f'{tag}-{p.stem}-executed.asm';ins=[code.at(a)for a in sorted(executed)]
    for i in ins:assert i.bytes==x.read(i.address,len(i.bytes))==bytes(g.mem_read(i.address,len(i.bytes)))
    save(listing,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
    result=dict(image=tag,map=item,state=state,fp_context=dict(x87_control_word='027f',mxcsr='00001f80'),map_activation_stop=map_boundary,regions=regions,cases=cases,
      source_available=False,original_admission_succeeded=False,continuation=continuation,listing=dict(file=str(listing.relative_to(out)),sha256=sha(listing),rows=len(ins)))
    c.data.close();c.stream.close();return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--replay',type=Path);p.add_argument('--prepared',type=Path);a=p.parse_args()
    assert sha(RAW)==RAW_PIN and sha(INVENTORY)==INVENTORY_PIN
    replay=json.loads(a.replay.read_text())if a.replay else None
    prep=json.loads(a.prepared.read_text())if a.prepared else preparation(a.output,None if replay is None else replay['preparation'])
    if a.prepared:save(a.output/'preparation.json',encoded(prep))
    pair=json.loads((PAIR/'report.json').read_text());inventory=json.loads(INVENTORY.read_text());results=[]
    providers=[];old=Xbe(OLD);new=Xbe(NEW);_,gm,_=mappings(pair)
    for addr in OPERANDS:
        other=gm[addr];before=old.read(addr,4);after=new.read(other,4);assert before==after
        providers.append(dict(old_va=f'{addr:08x}',u5_va=f'{other:08x}',raw_hex=before.hex()))
    save(a.output/'providers.json',encoded(dict(words=providers,source_images=[dict(path=str(p),sha256=sha(p))for p in(OLD,NEW)],mutable_delay_scale_excluded=True)))
    for item in inventory['maps']:
        for tag,path in(('old',OLD),('u5',NEW)):results.append(run(tag,path,item,pair,prep,a.output))
        assert [{k:v for k,v in q.items()if k not in('stores','cache_request')}for q in results[-1]['cases']]==[{k:v for k,v in q.items()if k not in('stores','cache_request')}for q in results[-2]['cases']]
        assert [[{k:v for k,v in s.items()if k!='actual_instruction'}for s in q['stores']]for q in results[-1]['cases']]==[[{k:v for k,v in s.items()if k!='actual_instruction'}for s in q['stores']]for q in results[-2]['cases']]
        print(json.dumps(dict(map=Path(item['path']).name,cases=6,stores=sum(len(q['stores'])for r in results[-2:]for q in r['cases']))),flush=True)
    report=dict(schema='original-sound-create-native-v1',oracle_sha256=sha(__file__),paired_report_sha256=sha(PAIR/'report.json'),raw_request_fixture_sha256=RAW_PIN,
      inventory_sha256=INVENTORY_PIN,preparation=prep,maps=results,instruction_patches=[],supplied_admission_ready_voice_or_cache_results=[],
      limits=['Direct127750 rawABI composition is explicit: actual previously rejected nonpositional FP buffers and recorded association0 are retained. No owning128380 positive admission or real local binding is asserted.',
        'Every independent request restarts whole actual old startup/class constructor/map-prefix buffers. Reusing them in u5 is explicit retained ABI composition supported by complete paired bodies, not complete u5 startup.',
        'Whole126DE0 reaches real absent-driver stop2217E8/2260E8. Current-map roots are explicit owned-map bindings, not all88activation callbacks or activeBSP/listener production.',
        'Original127750 stops immediately before actual21A960 request for native CPU differential. Cache read/pin completion and voice playback are not supplied; its independently executed full continuation is retained separately.',
        'RNG/class/pool/count/mutable range fields come from original constructors and literal owned map bytes. No chosen seed/permutation/range/availability value is assigned.'])
    save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(r['cases'])for r in results),stores=sum(len(q['stores'])for r in results for q in r['cases']))))
if __name__=='__main__':main()
