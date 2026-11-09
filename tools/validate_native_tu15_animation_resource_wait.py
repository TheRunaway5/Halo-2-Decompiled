#!/usr/bin/env python3
"""Compare1262B0 with real file-worker completion and original CPU segments."""
import argparse,copy,hashlib,json,os,random,re,struct,subprocess,sys
from pathlib import Path
from specialize_native_operands import leaf_body
import native_animation_resource_wait_replacements as binding
from validate_native_tu15_prt_helpers import PIN,physical_original
ROOT=Path(__file__).resolve().parents[1]
SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp')
REGIONS=((0x41b000,4096),(0x4ec000,20480),(0x61000000,65536),(0x62000000,8192),(0x64000000,8192))
FILES=('include/halo2/native_tu15_animation_resource_wait_kernel.hpp','src/physics/runtime/native_tu15_animation_resource_wait.hpp',
    'tools/native_animation_resource_wait_replacements.py','tools/validate_native_tu15_animation_resource_wait.py','tests/native_tu15_animation_resource_wait_tests.cpp',
    'include/halo2/animation_async_worker.hpp','src/gameplay/animation/animation_async_worker.cpp','src/gameplay/animation/animation_async_constructor.cpp',
    'src/platform/native_threads.cpp','src/platform/native_synchronization.cpp','src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp',
    'src/platform/native_tu15_async_promote.hpp','src/platform/native_tu15_async_order.hpp','src/platform/native_tu15_kernel_yield.hpp','include/halo2/source_kernel_yield_tu15.hpp','include/halo2/native_kernel_yield.hpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def word(banks,address):return next(int.from_bytes(bytes.fromhex(b['before'])[address-b['address']:address-b['address']+4],'little')for b in banks if b['address']<=address<=b['address']+len(b['before'])//2-4)
def corpus(image,count):
    rng=random.Random(0x1262b0);rows=[]
    for n in range(count):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(p,value):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=p<=base+size-4:struct.pack_into('<I',bank,p-base,value);return
            raise ValueError('Wait fixture escaped backing')
        stack=0x62001000+n%16;put(stack,0x63000000);put(stack+4,0x10203040);put(stack+8,0x50607080);put(0x4ecc34,n)
        put(0x41b69c,int.from_bytes(image.read(0x41b69c,4),'little'))
        mode=n%6;alias=mode==1 and (n//6)%2
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack;gp[7]=stack-12 if alias else 0x61000100
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
        rows.append(dict(id=n,name=f'wait-{n}-mode{mode}'+('-saved-si-job-alias'if alias else ''),mode=mode,release_yield=(1,1,1,3,1,1)[mode],size=(0,17,4096,513,1024,2048)[mode],chunk=(256,16,256,128,256,128)[mode],
            gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n&4 else 0)],
            live_x87=list(values[:(0,1,4,8)[(n//6)%4]]),banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--cases',type=int,default=24);p.add_argument('--require-binding',action='store_true');p.add_argument('--compiler',default='c++');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux':return 77
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:sha(ROOT/name)for name in(*FILES,*SOURCES)};bodies=[];rejected=0
    for name in SOURCES:
        source=(ROOT/name).read_text();bound=binding.replace_animation_resource_wait(source)
        if args.require_binding and source!=bound:raise ValueError('Production wait prefix absent '+name)
        if binding.replace_animation_resource_wait(bound)!=bound:raise ValueError('Wait binder is not idempotent')
        for variant in(source,bound):
            victim=leaf_body(variant,'1262b0')[0]
            for old,new in(('0xffffffffu','0xfffffffeu'),('0x8u','0xcu'),('0x1262c9u','0x1262cau')):
                changed=victim.replace(old,new,1)
                if changed==victim:raise ValueError('Missing wait body mutation witness')
                try:binding.replace_animation_resource_wait(variant.replace(victim,changed,1))
                except ValueError:rejected+=1
                else:raise ValueError('Wait binder admitted source drift')
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        changed=source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site+1u,target,tail);',1),1)
        if changed==source:raise ValueError('Missing wait dispatcher mutation witness')
        try:binding.replace_animation_resource_wait(changed)
        except ValueError:rejected+=1
        else:raise ValueError('Wait binder admitted dispatcher drift')
        wrong=bound.replace(binding.PREFIX,binding.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);'),1)
        try:binding.replace_animation_resource_wait(wrong)
        except ValueError:rejected+=1
        else:raise ValueError('Wait binder admitted changed prefix')
        body=binding.canonical_body(leaf_body(source,'1262b0')[0]);bodies.append(re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body))
    (scratch/'animation_resource_wait_reference.inc').write_text(bodies[0]);(scratch/'animation_resource_wait_ai_reference.inc').write_text(bodies[1]);binary=scratch/'resource-wait'
    command=[args.compiler,'-std=c++20','-O2','-pthread','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_animation_resource_wait_tests.cpp'),*[str(ROOT/name)for name in('src/gameplay/animation/animation_async_worker.cpp','src/gameplay/animation/animation_async_constructor.cpp','src/platform/native_threads.cpp','src/platform/native_synchronization.cpp','src/rendering/source_bitmap_async_read.cpp','src/platform/source_async_jobs.cpp')],'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:raise ValueError('Wait fixture compile failed '+result.stderr[-5000:])
    from oracle_smg_firing_1_1 import Xbe
    image=Xbe(args.xbe);rows=corpus(image,args.cases);(scratch/'input.json').write_text(json.dumps(dict(cases=rows),separators=(',',':')))
    payload=random.Random(0x1262b0).randbytes(8192);(scratch/'owned-cache-sectors.bin').write_bytes(payload)
    result=subprocess.run([str(binary),str(scratch/'input.json'),str(scratch/'owned-cache-sectors.bin')],capture_output=True,text=True,timeout=90)
    if result.returncode:raise ValueError('Real-worker wait failed '+result.stderr[-5000:])
    output=json.loads(result.stdout);actual=output['cases'];segments=[];expected=[];counters=dict(yields=0,worker_callbacks=0,actual_bytes_read=0,promotions=0)
    forced=subprocess.run([str(binary),str(scratch/'input.json'),str(scratch/'owned-cache-sectors.bin')],capture_output=True,text=True,timeout=90,env=dict(os.environ,HALO2_REFERENCE_ANIMATION_RESOURCE_WAIT='1'))
    if forced.returncode:raise ValueError('Forced-reference real-worker wait failed '+forced.stderr[-5000:])
    forced_output=json.loads(forced.stdout)
    for case in forced_output['cases']:case['parent_callbacks']=0
    def segment(state,entry,target,name,wanted,reads,writes):
        row=copy.deepcopy(state);sp=row['gp'][4];row.update(id=len(segments),name=name,entry=entry,max_steps=1000,return_pc=word(row['banks'],sp),matrix=word(row['banks'],sp+4),source=word(row['banks'],sp+8))
        if target:row['stop_before_entries']=[target]
        segments.append(row);expected.append(dict(state=wanted,reads=reads,writes=writes,target=target or 0x63000000))
    for n,row in enumerate(rows):
        native,*references=actual[n*3:n*3+3]
        for reference in references:
            reference['parent_callbacks']=0
            if native!=reference:raise ValueError('Native/retained real-worker wait differs '+row['name'])
        for key in('yields','worker_callbacks','actual_bytes_read'):counters[key]+=native[key]
        previous=native['initial'];entry=0x1262b0;previous_reads=previous_writes=0
        for phase in native['phases']:
            target=phase['target'];counters['promotions']+=target==0x1236a0
            segment(previous,entry,target,row['name']+f'-before-{target:x}',phase,phase['reads'][previous_reads:],phase['writes'][previous_writes:])
            previous=phase['after'];entry=phase.get('resume',0x1262c9 if target==0x1236a0 else 0x1262dc);previous_reads=len(previous['reads']);previous_writes=len(previous['writes'])
        segment(previous,entry,None,row['name']+'-return',native,native['reads'][previous_reads:],native['writes'][previous_writes:])
    if forced_output!=output:raise ValueError('Forced-reference wait DATA/ABI differs')
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    old_metadata=common.metadata
    def metadata(image):
        value=old_metadata(image);code=common._Code(image,'.text')
        for site in value['sites']:
            if code.at(site['instruction']).mnemonic in('pop','ret'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return value
    common.metadata=metadata
    original,proof=physical_original(image,segments,scratch,entries=(0x1262b0,),regions=REGIONS,preserve_x87_prefix=True)
    executed=set()
    for cpu,wanted,row in zip(original['cases'],expected,segments,strict=True):
        state=wanted['state'];observed=dict(gp=cpu['boundary_gp'],eflags=cpu['boundary_eflags'],xmm=cpu['boundary_xmm'],before_fp=cpu['after_fp'])
        if observed!={key:state[key]for key in observed}:raise ValueError('Original wait ABI differs '+row['name']+' '+str(observed)+' wanted '+str({key:state[key]for key in observed}))
        if not cpu['completed']or not cpu['live_x87_unchanged']:raise ValueError('Original wait FP/completion differs '+row['name'])
        if cpu.get('stopped_at',0x63000000)!=wanted['target']:raise ValueError('Original wait owner stop differs')
        if [dict(address=b['address'],before=b['after'])for b in cpu['banks']]!=state['banks']:raise ValueError('Original wait DATA differs '+row['name'])
        reads=[dict(address=e['address'],bytes=e['bytes'])for e in cpu['reads']];writes=[{key:e[key]for key in('address','before','written')}for e in cpu['writes']]
        if reads!=wanted['reads']or writes!=wanted['writes']:raise ValueError('Original wait ordered accesses differ '+row['name'])
        executed.update(cpu['executed_sites'])
    sites={int(v,16)for v in re.findall(r'instruction\(0x([0-9a-f]+)u\)',bodies[0])}
    if sites!=executed:raise ValueError('Original wait source-site coverage incomplete '+str(sorted(sites-executed)))
    report=dict(passed=True,production_bound=args.require_binding,real_worker_cases=len(rows),native_retained_comparisons=len(actual),forced_reference_comparisons=len(actual),physical_original_segments=len(segments),source_sites=len(sites),counters=counters,counters_scope='One default native corpus; excludes the two retained-copy routes and forced-reference repetition.',admission_refusals=output['admission_refusals'],rejected_mutations=rejected,
        actual_owned_file_bytes=len(payload),owned_file_sha256=hashlib.sha256(payload).hexdigest(),physical_original=proof,full_gp_flags_simd_fp_live80_data_and_ordered_accesses_exact=True,
        provider_scope='Existing SourceAnimationAsyncWorker and SourceAsyncOwnedFiles on a scratch-owned declared file, actual NativeThreads/NativeSynchronization and computed job IDs. Scheduling controls release the real worker at a declared actual-yield call and wait for real retirement; no ready bytes or child outcomes supplied.',
        original_scope='Unchanged original parent segments around real qualified native promotion/yield children and actual host service snapshots. Xbox asynchronous/kernel scheduling, real asset/runtime worker integration, exceptional prefixes and complete child CPU execution under this parent are separate.',
        xbox_scheduling_qualified=False,real_asset_acceptance=False,exceptional_partial_state_qualified=False,compile_command=command,source_sha256=pins,source_drift=[name for name,pin in pins.items()if sha(ROOT/name)!=pin],scratch_directory=str(scratch),retained_large_artifacts=[])
    if report['source_drift']:raise ValueError('Wait input sources drifted '+str(report['source_drift']))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'PASS {len(actual)} real-worker native/retained waits, {len(segments)} original CPU segments');return 0

if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            out=Path(sys.argv[sys.argv.index('--report')+1]);out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(dict(passed=False,error=repr(error),scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
