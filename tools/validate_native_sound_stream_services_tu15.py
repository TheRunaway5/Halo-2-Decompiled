#!/usr/bin/env python3
"""Compare unchanged TU1.5 stream callers with native kernels and real SDL services.

The host adapter is an explicit test ABI: SDL owns queue capacity, acceptance,
consumption and completion. Its input-boundary annotation is native policy,
not Xbox SDK/DSP equivalence. Raw cache/channel/codec records are declared ABI
inputs, never source World, cache admission, readiness or Xbox voice results.
"""
import argparse,collections,ctypes,hashlib,json,os,random,sys
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bridge',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--case',type=int,action='append',help='Run declared case indices for a bounded diagnostic');args=p.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    try:
        from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,REGS,STACK,STOP
        from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
        from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_EFLAGS,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW,UC_X86_REG_XMM0
    except ModuleNotFoundError:
        print('Original stream reference requires the declared Unicorn/Capstone Python');return 77
    if not NEW_DEFAULT.is_file():print('Authenticated original TU1.5 image is unavailable');return 77
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    library=ctypes.CDLL(str(args.bridge.resolve(strict=True)))
    for name in ['h2_stream_native','h2_stream_provider']:
        method=getattr(library,name);method.argtypes=[ctypes.c_char_p];method.restype=ctypes.c_char_p
    def invoke(name,row):
        result=json.loads(getattr(library,name)(json.dumps(row,separators=(',',':')).encode()))
        if 'error'in result:raise ValueError((name,result['error']))
        return result
    base,chunk,cache,entries,allocator,block_owner,blocks,codec,sdk,pool=[0x4000000+n*0x100000 for n in range(10)]
    stream=base+12;sdk_entries=[0x7000000,0x7000010,0x7000020]
    cpu_calls={0x2b9a00,0x2b98d0,0x2b9950,0x2b9980,0x2b99c0,0x2b8530}
    rng=random.Random(0x2b8530);specs=[]
    for flags in range(256):specs.append(dict(entry=0x2b9a00,flags=flags))
    for flags in range(256):specs.append(dict(entry=0x2b9980,flags=flags))
    for flags in range(256):
        for mode in [2,3]:specs.append(dict(entry=0x2b8530,flags=flags,mode=mode,prefill=min((flags>>3)&7,2)))
    for mode in range(256):specs.append(dict(entry=0x2b8530,flags=2,mode=mode))
    for reference in [0,1,254,255]:
        for submitted in [0,1]:specs.append(dict(entry=0x2b9980,flags=2|submitted*8,reference=reference))
        specs.append(dict(entry=0x2b8530,flags=2,reference=reference,remove_on_submit=True))
    for n in range(12):
        for action in ['removed_before_status','remove_on_submit']:specs.append(dict(entry=0x2b8530,flags=2,**{action:True},prefill=n%2))
    for event in ['success','flush','failure']:
        for flags in [0,4,8,12]:
            for queued in [1,2]:specs.append(dict(entry=0x227140,flags=queued|8,guard=flags,prefill=1,event=event,render_frames=160))
    for channel in [0xffffffff,0x80000000,1,2,0x7fffffff]:specs.append(dict(entry=0x227140,flags=2,channel=channel))
    for context in [0]:specs.append(dict(entry=0x227140,flags=2,context=context))
    for fault in ['stream','cache','entries','chunk','codec','sdk','channels']:
        for entry in [0x2b9980,0x2b8530,0x227140]:specs.append(dict(entry=entry,flags=2,fault=fault))
    if args.case is not None and any(n<0 or n>=len(specs)for n in args.case):p.error('Case index outside the declared matrix')
    event_statuses=collections.Counter()
    sites=collections.Counter();counts=collections.Counter();accepted=boundaries=actual_events=faults=returns=store_count=0;input_digest=hashlib.sha256();adapter_trace_digest=hashlib.sha256()
    report={'passed':False,'scope':__doc__,'original_xbe_sha256':NEW_PIN,'source_instruction_patches':[],
        'native_sdk_entrypoints':sdk_entries,'selected_cases':args.case,'complete_declared_matrix':args.case is None,'native_adapter_policy':{'status_and_submit':'Actual SDL queue and validation outcomes with explicit HRESULT mapping','boundary':'Retained native input-boundary annotation; no Xbox discontinuity/DSP claim','decoded_pcm':'Declared synthetic native decoder/provider mapping; no original waveform claim','completion_acknowledgment':'After source callback returns; faulted source calls retain the completion until private teardown'},
        'source_world_cache_readiness_or_voice_admission':False,'xbox_or_device_waveform_equivalence':False}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.unlink(missing_ok=True)
    try:
        for ordinal,spec in enumerate(specs):
            fields=[]
            def w(a,n,v):fields.append([a,n,v&((1<<(n*8))-1)])
            for a in range(STACK-0x100,STACK+0x24,4):w(a,4,rng.getrandbits(32))
            for a,v in [(0x50ebdc,cache),(0x50ebe0,pool),(0x50ebe4,allocator),(0x52d674,base),(cache+0x44,entries),
                (allocator+0x64,block_owner),(allocator+0x34,0),(block_owner+0x44,blocks),(base+4,1),
                (stream+0x1c,chunk),(stream+0x20,chunk+0x100),(stream+0x24,sdk),(stream+0x30,codec),
                (codec,codec+0x100),(sdk,sdk+0x100),(sdk+0x10c,sdk_entries[0]),(sdk+0x110,sdk_entries[1]),(sdk+0x114,sdk_entries[2])]:w(a,4,v)
            for offset,target in [(0xc,0x2b98d0),(0x10,0x2b9950),(0x14,0x2b9980),(0x18,0x2b99c0),(0x20,0x2b9a00)]:w(codec+0x100+offset,4,target)
            for n in range(2):
                w(chunk+n*0x100+4,4,72);w(chunk+n*0x100+8,4,(rng.getrandbits(16)<<16)|n)
                w(entries+n*16+4,1,[0,1,2,255][ordinal%4]);w(entries+n*16+5,1,spec.get('reference',int(spec.get('prefill',0)>n)))
                w(blocks+n*24+8,4,n*0x100)
            w(stream+1,1,spec.get('mode',2));w(stream+2,1,spec['flags']);w(stream+3,1,spec.get('guard',0));w(stream+0x2c,4,0)
            w(STACK,4,STOP);w(STACK+4,4,stream if spec['entry']!=0x227140 else spec.get('channel',0));w(STACK+8,4,spec.get('context',chunk));w(STACK+12,4,0)
            gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK
            if spec['entry']==0x2b8530:gp[0]=stream
            fault=spec.get('fault')
            if fault=='stream':
                if spec['entry']==0x2b8530:gp[0]=0
                else:w(STACK+4,4,0xffffffff if spec['entry']==0x227140 else 0)
            elif fault=='cache':w(0x50ebdc,4,0)
            elif fault=='entries':w(cache+0x44,4,0)
            elif fault=='chunk':w(stream+0x1c,4,0)
            elif fault=='codec':w(stream+0x30,4,0)
            elif fault=='sdk':w(stream+0x24,4,0)
            elif fault=='channels':w(0x52d674,4,0)
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];fp=[[0x27f,0x37f,0x77f,0xb7f,0xf7f][ordinal%5],[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5],(ordinal%8)<<11]
            regions=[[0x50e000,4096],[0x52d000,4096],[0x2000000,0x10000]]+[[a,4096]for a in [base,chunk,cache,entries,allocator,block_owner,blocks,codec,sdk,pool]]
            row=spec|dict(fields=fields,gp=gp,xmm=xmm,fp=fp,regions=regions,chunks=[dict(descriptor=chunk+n*0x100,data=pool+n*0x100,bytes=72,frames=160+n*32)for n in range(2)])
            if args.case is not None and ordinal not in args.case:continue
            input_digest.update((json.dumps(row,separators=(',',':'))+'\n').encode())
            report['case_in_progress']=ordinal
            args.output.write_text(json.dumps(report,indent=2)+chr(10))
            if ordinal%128==0 or args.case is not None:print(json.dumps({'case_in_progress':ordinal,'spec':spec}),flush=True)
            native=invoke('h2_stream_native',row);initial=invoke('h2_stream_provider',{'op':'reset','row':row})
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
            for a,n in regions:
                if a>=0x810000:g.mem_map(a,n)
                g.mem_write(a,bytes(n)) # Declared raw data regions, never .text.
            g.mem_map(STOP,4096);g.mem_map(sdk_entries[0],4096)
            for a,n,v in fields:g.mem_write(a,v.to_bytes(n,'little'))
            if initial['event']:
                for n,key in enumerate(['channel','context','status']):g.mem_write(STACK+4+n*4,int(initial['event'][key]).to_bytes(4,'little'))
            for reg,v in zip(REGS,gp):g.reg_write(reg,v)
            for n,words in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,sum(v<<(k*32)for k,v in enumerate(words)))
            for reg,v in zip([UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW],fp):g.reg_write(reg,v)
            g.reg_write(UC_X86_REG_EFLAGS,0x202);stores=[];calls=[];bad=[];executed=set();started=False
            def u(a,n=4):return int.from_bytes(g.mem_read(a,n),'little')
            def instruction(_g,a,n,_):
                nonlocal started
                if (started and a in cpu_calls)or a in sdk_entries:
                    s=g.reg_read(REGS[4]);argc=3 if a in [sdk_entries[1],0x2b99c0]else 1 if a==sdk_entries[2]else 0 if a==0x2b8530 else 2
                    call=dict(target=a,gp=[g.reg_read(r)for r in REGS],**{'return':u(s)},args=[u(s+4+k*4)for k in range(argc)])
                    if a==sdk_entries[1]:call['packet']=[u(call['args'][1]+k*4)for k in range(6)]
                    calls.append(call)
                    if a in sdk_entries:
                        reply=invoke('h2_stream_provider',{'op':'service','target':a,'packet':call.get('packet',[])})
                        if a==sdk_entries[0]:g.mem_write(call['args'][1],int(reply['status_word']).to_bytes(4,'little'))
                        for reg,key in zip(REGS[:3],['eax','ecx','edx']):g.reg_write(reg,reply[key])
                        g.reg_write(REGS[4],s+(argc+1)*4);g.reg_write(UC_X86_REG_EIP,call['return']);return
                started=True;assert image.read(a,n)==bytes(g.mem_read(a,n));executed.add(a)
            def write(_g,kind,a,n,v,_):
                if 0x2000000<=a<STACK:return
                try:before=u(a,n)
                except UcError:return
                stores.append([g.reg_read(UC_X86_REG_EIP),a,n,before,v&((1<<(n*8))-1)])
            def invalid(_g,kind,a,n,v,_):bad.append([a,n]);return False
            g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
            try:g.emu_start(spec['entry'],STOP,count=20000)
            except UcError:
                if not bad:raise
            if not bad:
                assert g.reg_read(UC_X86_REG_EIP)==STOP
                invoke('h2_stream_provider',{'op':'account_event'})
            original=invoke('h2_stream_provider',{'op':'receipt'});original.update(gp=[g.reg_read(r)for r in REGS],xmm=[[g.reg_read(UC_X86_REG_XMM0+n)>>(k*32)&0xffffffff for k in range(4)]for n in range(8)],fp=[g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]],calls=calls,stores=stores,fault=bad[0]if bad else None,
                pages=[[a,hashlib.sha256(g.mem_read(a,n)).hexdigest()]for a,n in regions if a<0x2000000 or a>=0x2010000])
            # Native memory emits pages in address order, including split regions.
            original['pages'].sort()
            for key in original:
                if key=='gp'and bad:continue
                if original[key]!=native[key]:
                    report.update(case=ordinal,input=row,first_difference=key,native=native[key],original=original[key]);raise AssertionError(f'Stream case {ordinal} {spec} differs: {key}')
            if original['event']:event_statuses[f'{original["event"]["status"]:08x}']+=1
            accepted+=original['actual_accepted'];boundaries+=original['native_boundary_annotations'];actual_events+=bool(original['event']);faults+=bool(bad);returns+=not bad;store_count+=len(stores);sites.update(executed);counts[f'{spec["entry"]:08x}']+=1
            adapter_trace_digest.update(json.dumps(original['sdk_trace'],separators=(',',':')).encode())
            invoke('h2_stream_provider',{'op':'close'})
        report.pop('case_in_progress',None)
        report.update(passed=True,cases=returns+faults,declared_cases=len(specs),entries=dict(counts),returns=returns,faults=faults,ordered_stores=store_count,instruction_sites=len(sites),executed_instructions_lower_bound=sum(sites.values()),
            actual_native_accepted_packets=accepted,native_boundary_annotations=boundaries,actual_native_completion_events=actual_events,actual_native_completion_statuses=dict(event_statuses),input_sha256=input_digest.hexdigest(),adapter_trace_sha256=adapter_trace_digest.hexdigest(),
            all_general_registers_on_return=True,all_non_stack_page_hashes_checked=True,all_callee_entry_gprs_and_arguments_checked=True,fp_simd_preserved=True,callee_dead_stack_excluded=True)
    finally:
        invoke('h2_stream_provider',{'op':'close'});args.output.parent.mkdir(parents=True,exist_ok=True);data=json.dumps(report,indent=2)+'\n';assert len(data.encode())<=1024*1024;args.output.write_text(data)
    print(json.dumps({k:v for k,v in report.items()if k not in ['input','native','original','scope']}));return 0
if __name__=='__main__':raise SystemExit(main())
