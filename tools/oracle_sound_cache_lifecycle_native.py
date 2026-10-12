#!/usr/bin/env python3
"""Unchanged lifecycle owners on genuinely read, source-created cache entries.

Each new control starts from the same complete original cached state. No cache
timestamp, pin, completion, system availability or queue flag is assigned.
"""
import argparse,json,struct,time
from pathlib import Path
from unicorn import Uc as RealUc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
import oracle_sound_cache_request_native as prior
from oracle_mp_title_update_display import ROOT,OLD,Xbe,_Code,sha,save,encoded
from oracle_smg_firing_1_1 import STACK,STOP
BASE=ROOT/'analysis/multiplayer/integration/sound-cache-lifecycle-native'
REQUEST=ROOT/'analysis/multiplayer/integration/sound-cache-request-native'
PRIOR_PIN='2568f5ded360ee5e8d7fd9967045f7c12a11fbb462b23f0e1beaf7d69b9c07dc'
class Boundary(Exception):pass

def execute(out):
    assert sha(REQUEST/'final-manifest.json')==PRIOR_PIN
    frames=[]
    def capture_uc(*args):
        g=RealUc(*args);captured=False
        def capture(_g,a,n,u):
            nonlocal captured
            if a!=0x21a960 or captured:return
            sp=g.reg_read(UC_X86_REG_ESP)
            def read(at,n=4):return int.from_bytes(g.mem_read(at,n),'little')
            if read(sp)!=STOP or read(sp+12)!=4:return
            chunk=read(sp+8);handle=read(chunk+8)
            if handle==0xffffffff:return
            pool=read(0x5098dc);entry=read(pool+0x44)+(handle&0xffff)*16
            assert read(entry+2,1)==1 and read(entry+3,1)==1
            assert read(entry+4,1)==read(entry+5,1)==0
            frames.append((g.context_save(),[(lo,bytes(g.mem_read(lo,hi-lo+1)))for lo,hi,_ in g.mem_regions()]))
            captured=True
        g.hook_add(UC_HOOK_CODE,capture);return g
    original=prior.Uc;prior.Uc=capture_uc
    try:preparation=prior.execute(out/'request-reexecution')
    finally:prior.Uc=original
    assert len(frames)==len(preparation['cases'])==3
    image=Xbe(OLD);code=_Code(image,'.text');cases=[];seen=set()
    bindings=dict(entry_pool_pointer='005098dc',page_allocator_pointer='005098e4',physical_buffer_pointer='005098e0',work_mutex_slot='004e792c',work_head='004e9ebc',requests_added='004e9ec0',sound_system_pointer='004ed950')
    replay=None
    if (out/'clock-replay.json').exists():replay=json.loads((out/'clock-replay.json').read_text())
    clock_reads=[]
    for case_index,((context,whole),original_case)in enumerate(zip(frames,preparation['cases'])):
        # Original returned flags0, but the next flags4 has not executed. Each
        # control independently restores that entire captured state, never a
        # selected flag/counter. Direct ABI calls are explicitly compositions.
        for control in ('clock_then_alias_saturation','cold_audio_update','direct_rebase','drain_unpinned'):
            h=RealUc(UC_ARCH_X86,UC_MODE_32)
            for lo,raw in whole:h.mem_map(lo,len(raw));h.mem_write(lo,raw)
            h.context_restore(context)
            def u(a,n=4):return int.from_bytes(h.mem_read(a,n),'little')
            def w(a,v,n=4):h.mem_write(a,(v&((1<<(8*n))-1)).to_bytes(n,'little'))
            sp=h.reg_read(UC_X86_REG_ESP);chunk=u(sp+8)
            retained=[(int(q['address'],16),len(bytes.fromhex(q['raw_hex'])))for q in original_case['stages'][3]['after']]
            # Include whole actual global-root operands rather than inventing
            # root values from XBE startup bytes.
            retained +=[(int(a,16),4)for a in bindings.values()if not any(lo<=int(a,16)<lo+n for lo,n in retained)]
            def snapshot():return [dict(address=f'{a:08x}',raw_hex=bytes(h.mem_read(a,n)).hex())for a,n in retained]
            events=[];stages=[];stop=None;current=''
            def writes(_h,kind,a,n,v,_):
                if any(lo<=a and a+n<=lo+size for lo,size in retained):
                    events.append(dict(store=dict(instruction=f'{h.reg_read(UC_X86_REG_EIP):08x}',address=f'{a:08x}',bytes=n,before=f'{u(a,n):0{n*2}x}',after=f'{v&((1<<(8*n))-1):0{n*2}x}')))
            def hook(_h,a,n,_):
                nonlocal stop
                instruction=code.at(a)
                if instruction:
                    assert instruction.bytes==image.read(a,n)==bytes(h.mem_read(a,n));seen.add(a)
                # Real GetTickCount implementation dereferences actual OS tick
                # backing. This host boundary records CLOCK_BOOTTIME and gives
                # its raw32 value to the unchanged full original thunk.
                if a==0x337a40:
                    key=f'{case_index}/{control}/{current}/{sum(1 for e in events if e.get("service")=="native_tick_count")}'
                    if replay is None:value=(time.clock_gettime_ns(time.CLOCK_BOOTTIME)//1000000)&0xffffffff
                    else:value=next(q['value']for q in replay if q['key']==key)
                    backing=u(0x417cd4);assert backing!=0
                    w(backing,value);events.append(dict(service='native_tick_count',arguments=[f'{value:08x}']))
                    clock_reads.append(dict(key=key,value=value,provider='actual CLOCK_BOOTTIME'if replay is None else'exact replay of separately retained actual CLOCK_BOOTTIME'))
                elif a in(0x2dc9c8,0x2dc892):
                    stop=dict(service='actual native work-mutex wait/release not provided by this unpinned control',entry=f'{a:08x}',result_supplied=False);raise Boundary()
                elif 0x80000000<=a<0x80001000:
                    stop=dict(service='unprovided actual kernel request',entry=f'{a:08x}',result_supplied=False);raise Boundary()
            h.hook_add(UC_HOOK_CODE,hook);h.hook_add(UC_HOOK_MEM_WRITE,writes)
            def stage(name,entry,repeat=1):
                nonlocal current,stop
                current=name;before=snapshot();start=len(events);stop=None;returns=[]
                for _ in range(repeat):
                    w(STACK-0x8000,STOP);h.reg_write(UC_X86_REG_ESP,STACK-0x8000)
                    if entry==0x21aa60:h.reg_write(UC_X86_REG_EDX,chunk)
                    if entry==0x13e9a0:h.reg_write(UC_X86_REG_EAX,u(0x5098e4))
                    try:h.emu_start(entry,STOP,count=1000000)
                    except Boundary:break
                    assert h.reg_read(UC_X86_REG_EIP)==STOP,(name,hex(h.reg_read(UC_X86_REG_EIP)))
                    returns.append(f'{h.reg_read(UC_X86_REG_EAX):08x}')
                stages.append(dict(name=name,entry=f'{entry:08x}',repeat=repeat,returned=stop is None,returns=returns,before=before,after=snapshot(),events=events[start:],stop=stop))
            if control=='clock_then_alias_saturation':
                stage('source_cache_clock',0x21a920)
                stage('source_alias_255_uses',0x21aa60,255)
                stage('source_alias_saturated',0x21aa60)
                entry=u(u(0x5098dc)+0x44)+(u(chunk+8)&0xffff)*16
                assert u(entry+5,1)==255
            elif control=='cold_audio_update':
                system=u(0x4ed950);assert u(system+0x78,1)==0
                stage('complete_cold_audio_update',0x127290)
            elif control=='direct_rebase':
                stage('direct_page_epoch_rebase',0x13e9a0)
                assert u(u(0x5098e4)+0x38)==257
            else:
                stage('complete_unpinned_cache_drain',0x21a800)
                assert u(chunk+8)==0xffffffff and u(u(0x5098dc)+0x3c)==0
            cases.append(dict(scenario=original_case['scenario'],control=control,chunk=f'{chunk:08x}',bindings=bindings,constants=original_case['constants'],stages=stages,
                boundaries=['Whole original state captured after actual flags0 return and actual shared.map read; no source pool, timestamp, pin, completion or availability is assigned.',
                'New clock/alias/drain calls are explicit direct-ABI compositions on that source-produced cache state; no executed ordinary game loop or active-driver claim.',
                'Direct13E9A0 rebase is a raw ABI control on actual timestamp1; its ordinary overflow caller condition is not manufactured.',
                'Drain is genuinely unpinned and completes without mutex wait; busy drain native wait/clock lifetime remains separate.']))
    save(out/'clock-values.json',encoded(clock_reads))
    save(out/'instructions/executed-old.asm',''.join(f'{code.at(a).address:08x} {code.at(a).bytes.hex()} {code.at(a).mnemonic} {code.at(a).op_str}\n'for a in sorted(seen)).encode())
    report=dict(schema='sound-cache-lifecycle-original-v1',tool_sha256=sha(__file__),prior_manifest_sha256=PRIOR_PIN,paired_report_sha256=sha(BASE/'paired/report.json'),image_sha256=sha(OLD),cases=cases,source_flag_assignments=[],instruction_patches=[],source_admitted=False,clock_provider='Real CLOCK_BOOTTIME raw OS tick input, or separately recorded exact replay only at original GetTickCount thunk.')
    save(out/'report.json',encoded(report));return report
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args();r=execute(a.output.resolve());print(json.dumps(dict(cases=len(r['cases']),report_sha256=sha(a.output/'report.json'))))
