#!/usr/bin/env python3
"""Unchanged promotion/clock owners over all 23 owned serialized map banks.

The real OS clock schedule is recorded once, then explicitly replayed through
the unchanged GetTickCount data thunk. No source counter or selector is set.
The genuine map prefix remains stopped at its unavailable driver request.
"""
import argparse, hashlib, json, struct, time
from pathlib import Path
from unicorn import Uc, UcError, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_mp_title_update_display import ROOT, OLD, NEW, PINS, Xbe, _Code, sha, save, encoded
from oracle_sound_startup_native import AUDIO_ARENA, AUDIO_CAPACITY
from inspect_multiplayer import Cache

DIRECTORY = ROOT/'analysis/multiplayer/integration/sound-promotion-native'
PAIR = DIRECTORY/'paired'
PRIOR = ROOT/'analysis/multiplayer/integration/sound-startup-native/measured/report.json'
PRIOR_PIN = 'b2a1175e7dd524fc7d9587333e8055abd8cb4d30f6417e0ff293e9d1e4444287'
INVENTORY = ROOT/'analysis/multiplayer/integration/sound-metadata-provider-20261008/measured/report.json'
INVENTORY_PIN = 'f750772e6951ff065711dd3c91b207d657e85fe69c43c89d06135eb717e072d3'
TAGS, STACK, STOP, OUTPUT = 0x1080000, 0x200d000, 0x3000000, 0xa400000

def mappings(pair):
    pcs, globals_map, entries = {}, {}, {}
    for q in pair['owners']:
        entries[int(q['old'],16)] = int(q['u5'],16)
        rows = [(PAIR/'instructions'/f'{q["old"]}-{tag}.asm').read_text().splitlines() for tag in ('old','u5')]
        for old,new in zip(*rows): pcs[int(old.split()[0],16)] = int(new.split()[0],16)
        for m in q['memory']: globals_map[int(m['old_va'],16)] = int(m['new_va'],16)
    return pcs, globals_map, entries

def clock_schedule():
    rows=[]
    for delay in (0,0.025,0.025,2.05):
        if delay: time.sleep(delay)
        ns=time.clock_gettime_ns(time.CLOCK_BOOTTIME)
        rows.append(dict(provider='actual clock_gettime_ns(CLOCK_BOOTTIME)',observed_ns=ns,tick_ms_u32=(ns//1000000)&0xffffffff))
    return rows

def run(tag,path,item,pair,clocks,out):
    p=Path(item['path']);p=p if p.is_absolute()else ROOT/p
    assert sha(p)==item['sha256'] and sha(path)==PINS[str(path)]
    prior=json.loads(PRIOR.read_text())['stopped_original_caller'];c=Cache(p);x=Xbe(path);code=_Code(x,'.text')
    pcs,globals_map,entries=mappings(pair);reverse={b:a for a,b in pcs.items()}
    va=lambda a:a if tag=='old'else globals_map[a]
    entry=lambda a:a if tag=='old'else entries[a]
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    for at,size in ((TAGS,0x100000),(0x2000000,0x10000),(STOP,4096),(OUTPUT,4096),(AUDIO_ARENA,AUDIO_CAPACITY),(0x80000000,4096)):g.mem_map(at,size)
    resident=[t for t in c.tags.values()if t['offset']is not None]
    base=(min(t['offset']for t in resident)+c.magic)&~4095;end=(max(t['offset']+t['size']for t in resident)+c.magic+4095)&~4095
    g.mem_map(base,end-base);g.mem_write(base,bytes(c.data[base-c.magic:min(len(c.data),end-c.magic)]))
    index=c.u(16);table=c.u(index+8)-(c.u(index)-(index+32));table_raw=bytes(c.data[table:table+c.u(index+24)*16]);g.mem_write(TAGS,table_raw)
    def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
    def w(a,v):g.mem_write(a,struct.pack('<I',v))
    system=int(prior['globals']['system'],16);g.mem_write(AUDIO_ARENA,bytes.fromhex(prior['arena_hex']))
    w(va(0x4ed950),system);w(va(0x4eb114),TAGS)
    matg=next(t for t in c.tags.values()if t['group']=='matg');w(va(0x4e791c),matg['offset']+c.magic)
    ugh=c.tags[c.u(c.block(matg['offset']+0xc0,0x24)[0]+0x20)]
    assert u(va(0x4edf18))==0, 'Loader options must remain genuinely NULL'
    pending=None;clock_index=0;executed=set();boundary=None
    def hook(_g,a,n,_u):
        nonlocal clock_index
        if code.at(a):executed.add(a)
        if a==(0x337a40 if tag=='old'else clock_entry):
            assert clock_index<len(clocks)
            assert u(clock_operand)==0x8000009c
            w(0x8000009c,clocks[clock_index]['tick_ms_u32']);clock_index+=1
    def store(_g,k,a,n,v,_u):
        if pending is not None and (AUDIO_ARENA<=a<AUDIO_ARENA+AUDIO_CAPACITY or base<=a<end or a==OUTPUT or a==va(0x526464)):
            pc=g.reg_read(UC_X86_REG_EIP);pending.append(dict(instruction=f'{pc if tag=="old"else reverse.get(pc,pc):08x}',actual_instruction=f'{pc:08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}'))
    def invalid(_g,k,a,n,v,_u):
        nonlocal boundary
        boundary=dict(instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,access=k);return False
    # Call correspondence proves the whole platform clock entry; its data
    # operand is checked directly, never an assigned code return.
    map_row=next(q for q in pair['owners']if q['old']=='00126de0')
    call=next(q for q in map_row['calls']if q['old_target']=='00337a40')
    clock_entry=int(call['new_target'],16)
    thunk=_Code(x,'.text').at(0x337a40 if tag=='old'else clock_entry)
    clock_operand=thunk.operands[1].mem.disp
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,store);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    def call_source(e,sound=None):
        nonlocal boundary
        boundary=None;g.mem_write(STACK,struct.pack('<II',STOP,OUTPUT));g.reg_write(UC_X86_REG_ESP,STACK)
        if sound is not None:g.reg_write(UC_X86_REG_ECX,sound)
        g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        try:g.emu_start(entry(e),STOP,count=1000000)
        except UcError:assert boundary
        return g.reg_read(UC_X86_REG_EAX)
    pending=[];call_source(0x126de0)
    assert boundary and u(va(0x526474))==0 and u(system+0x78,1)==0
    init=dict(stores=pending,boundary=boundary,arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex(),clock_calls=1,whole_owner_returned=False)
    assert clock_index==1 and u(system+0x84)==0
    assert u(va(0x526464))==ugh['offset']+c.magic
    sounds=[t for t in c.tags.values()if t['group']=='snd!'and t['offset']is not None]
    promoted=[t for t in sounds if c.data[t['offset']+0xc]!=255]
    rows=c.block(ugh['offset']+0x48,28)
    assert all(c.u(a+0x10)==0xffffffff and c.u(a+0x14)==c.u(a+0x18)==0 for a in rows)
    counter_arrays=[]
    for a in rows:
        count=c.u(a+8);at=c.u(a+0xc);raw=bytes(c.data[at-c.magic:at-c.magic+4*count])
        assert not any(raw)
        counter_arrays.append(dict(address=f'{at:08x}',raw_hex=raw.hex(),serialized_zero=True))
    regions=[dict(address=f'{TAGS:08x}',raw_hex=table_raw.hex()),dict(address=f'{ugh["offset"]+c.magic:08x}',raw_hex=bytes(g.mem_read(ugh['offset']+c.magic,ugh['size'])).hex())]
    regions += [dict(address=f'{t["offset"]+c.magic:08x}',raw_hex=bytes(c.data[t['offset']:t['offset']+20]).hex())for t in sounds]
    cases=[]
    def clock_step(phase):
        nonlocal pending
        pending=[];before=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY));call_source(0x128140);assert boundary is None
        cases.append(dict(kind='clock',phase=phase,stores=pending,tick=clocks[clock_index-1]['tick_ms_u32'],system_clock=u(system+0x84),arena_sha256=hashlib.sha256(bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY))).hexdigest()))
    def promote(sound,phase):
        nonlocal pending
        pending=[];value=call_source(0x128610,int(sound['datum'],16));assert boundary is None
        cases.append(dict(kind='promotion',phase=phase,sound=sound['datum'],return_eax=f'{value:08x}',selector=f'{u(OUTPUT):08x}',stores=pending))
    clock_step('all_resident')
    for t in sounds:promote(t,'all_resident')
    clock_step('burst')
    for repeat in range(32):
        for t in promoted:promote(t,f'burst_{repeat}')
    clock_step('aged')
    for repeat in range(2):
        for t in promoted:promote(t,f'aged_{repeat}')
    assert clock_index==len(clocks)
    listing=out/'instructions'/f'{tag}-{p.stem}-executed.asm';ins=[code.at(a)for a in sorted(executed)]
    for i in ins:assert i.bytes==x.read(i.address,len(i.bytes))==bytes(g.mem_read(i.address,len(i.bytes)))
    save(listing,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode())
    result=dict(image=tag,map=dict(path=str(p),sha256=item['sha256']),state=dict(system=f'{system:08x}',tag_table=f'{TAGS:08x}',current_ugh=f'{ugh["offset"]+c.magic:08x}',options='00000000'),initialization=init,regions=regions,counter_arrays=counter_arrays,promotion_rows=len(rows),promoted_sounds=[t['datum']for t in promoted],cases=cases,final_ugh_hex=bytes(g.mem_read(ugh['offset']+c.magic,ugh['size'])).hex(),final_arena_hex=bytes(g.mem_read(AUDIO_ARENA,AUDIO_CAPACITY)).hex(),listing=dict(file=str(listing.relative_to(out)),sha256=sha(listing),rows=len(ins)),source_available=False)
    c.data.close();c.stream.close();return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--replay',type=Path);a=p.parse_args()
    assert sha(PRIOR)==PRIOR_PIN and sha(INVENTORY)==INVENTORY_PIN
    pair=json.loads((PAIR/'report.json').read_text());inventory=json.loads(INVENTORY.read_text())
    replay=json.loads(a.replay.read_text())if a.replay else None
    clocks=replay['clock_schedule']if replay else clock_schedule();results=[]
    for item in inventory['maps']:
        for tag,path in (('old',OLD),('u5',NEW)):
            r=run(tag,path,item,pair,clocks,a.output);results.append(r)
        print(json.dumps(dict(map=Path(item['path']).name,cases=len(results[-1]['cases']),suppressed=sum(q.get('return_eax')=='00000001'for q in results[-1]['cases']))),flush=True)
    report=dict(schema='original-source-sound-promotion-v1',oracle_sha256=sha(__file__),paired_report_sha256=sha(PAIR/'report.json'),prior_startup_report_sha256=PRIOR_PIN,inventory_sha256=INVENTORY_PIN,clock_schedule=clocks,maps=results,instruction_patches=[],source_counter_or_selector_assignments=[],platform_results=[],limits=['Whole original map activation stops at unavailable driver; +7A is not admission. Literal current-map bindings are explicit ABI composition, not88callback activation.','Full prior1.1startup arena is retained in both images; this is not complete u5 startup.','All promotion counters/rows/rules are untouched actual owned serialized cache outputs. Repeated source calls alone produce selectors, cooldown and counter values.','Real OS boot-millisecond observations are explicitly replayed through the actual unchanged GetTickCount data thunk, not fake sourceclock values or an Xbox timing claim.','Whole128140 ordinary loader-optionsNULL branch is executed. Alternate campaign fade/control owners are not given selected state.','No voice/admission/device/playback/platform return or RNG seed is supplied.'])
    save(a.output/'report.json',encoded(report));print(json.dumps(dict(report_sha256=sha(a.output/'report.json'),cases=sum(len(r['cases'])for r in results),stores=sum(len(q['stores'])for r in results for q in r['cases']))))
if __name__=='__main__':main()
