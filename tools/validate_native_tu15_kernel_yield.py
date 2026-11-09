#!/usr/bin/env python3
"""Qualify the real host yield boundary and unchanged i386 wrapper segments."""
import argparse,copy,hashlib,json,os,platform,random,struct,subprocess,sys
from pathlib import Path
from specialize_native_operands import leaf_body
import native_kernel_yield_replacements as binding
from validate_native_tu15_prt_helpers import PIN,physical_original

ROOT=Path(__file__).resolve().parents[1]
SOURCE='src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN='d6497ece40ee9bfbdf4c108fb878701f7a705a1316fdc3187eda7bb673d94f0b'
REGIONS=((0x41b000,4096),(0x62000000,8192))
FILES=('include/halo2/native_kernel_yield.hpp','include/halo2/source_kernel_yield_tu15.hpp',
    'include/halo2/native_tu15_kernel_yield_kernel.hpp','src/platform/native_tu15_kernel_yield.hpp',
    'tests/native_tu15_kernel_yield_tests.cpp','tools/validate_native_tu15_kernel_yield.py',
    'tools/native_kernel_yield_replacements.py','src/gameplay/units/death_startup_native.cpp','src/physics/physics_live_world_tu15.cpp')
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def read(banks,address,size=4):
    return next(int.from_bytes(bytes.fromhex(b['before'])[address-b['address']:address-b['address']+size],'little')for b in banks if b['address']<=address<=b['address']+len(b['before'])//2-size)

def corpus(image):
    rng=random.Random(0x2dbf74);rows=[]
    for n in range(128):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        # This page crosses XBE sections, so a page-sized Xbe.read would pad
        # at the first section's end. Mount the exact four-byte import word;
        # neighboring bytes remain declared caller/stack backing.
        banks[0][0x69c:0x6a0]=image.read(0x41b69c,4)
        stack=0x41b6a0 if n%16==0 else 0x62001000+n%16
        def put(address,value):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-4:struct.pack_into('<I',bank,address-base,value);return
            raise ValueError('Yield fixture escaped backing')
        put(stack,0x63000000);put(stack+4,0x10203040);put(stack+8,0x50607080)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=stack
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
        rows.append(dict(id=n,name=f'yield-{n}-'+('push-over-import'if n%16==0 else 'stack'),entry=0x2dbf74,gp=gp,eflags=0x202|(rng.getrandbits(12)&0xcd5),
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64),0x1f80|((n%4)<<13)|(0x8040 if n&4 else 0)],
            live_x87=list(values[:(0,1,4,8)[n%4]]),matrix=0x10203040,source=0x50607080,max_steps=100,banks=[dict(address=base,before=bank.hex())for (base,_),bank in zip(REGIONS,banks)]))
    return rows

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686'):return 77
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);source=(ROOT/SOURCE).read_text();body=binding.canonical_body(leaf_body(source,'2dbf74')[0]);source_pins={name:sha(ROOT/name)for name in(*FILES,SOURCE)}
    if hashlib.sha256(body.encode()).hexdigest()!=BODY_PIN:raise ValueError('Complete original kernel-yield wrapper drift')
    bound=binding.replace_kernel_yield(source)
    if args.require_binding and bound!=source:raise ValueError('Production kernel-yield binding absent')
    if binding.replace_kernel_yield(bound)!=bound:raise ValueError('Kernel-yield binder is not idempotent')
    rejected=0
    for text in(source,bound):
        victim=leaf_body(text,'2dbf74')[0]
        for old,new in(('0x41b69cu','0x41b6a0u'),('0x40000024u','0x40000025u'),('dispatch(s,target,0x2dbf74u,false);','dispatch(s,target,0x2dbf74u,true);')):
            changed=victim.replace(old,new,1)
            if changed==victim:raise ValueError('Missing complete kernel-yield mutation witness')
            try:binding.replace_kernel_yield(text.replace(victim,changed,1))
            except ValueError:rejected+=1
            else:raise ValueError('Kernel-yield binder admitted source drift')
    dispatcher=__import__('re').search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
    changed=source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site+1u,target,tail);',1),1)
    if changed==source:raise ValueError('Missing kernel-yield dispatcher mutation witness')
    try:binding.replace_kernel_yield(changed)
    except ValueError:rejected+=1
    else:raise ValueError('Kernel-yield binder admitted changed dispatcher')
    wrong=bound.replace(binding.PREFIX,binding.PREFIX.replace('dispatch(v,target,site,tail);','dispatch(v,target,site+1u,tail);'),1)
    try:binding.replace_kernel_yield(wrong)
    except ValueError:rejected+=1
    else:raise ValueError('Kernel-yield binder admitted changed prefix')
    if (ROOT/FILES[-1]).read_text().count('if(source_try_kernel_yield_tu15(m,c,a))return;')!=1:raise ValueError('Actual World kernel-yield service owner missing')
    if (ROOT/FILES[-2]).read_text().count('const auto status=native_kernel_yield_status();')!=1:raise ValueError('Existing DeathStartup host-yield factoring missing')
    (scratch/'kernel_yield_reference.inc').write_text(body)
    binary=scratch/'kernel-yield';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_kernel_yield_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:raise ValueError('Yield fixture compile failed '+result.stderr[-5000:])
    from oracle_smg_firing_1_1 import Xbe
    image=Xbe(args.xbe)
    if image.read(0x41b69c,4)!=bytes.fromhex('ee000080'):raise ValueError('Genuine NtYieldExecution import identity changed')
    rows=corpus(image);inputs=scratch/'yield-input.json';inputs.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
    result=subprocess.run([str(binary),str(inputs)],capture_output=True,text=True)
    if result.returncode:raise ValueError('Native yield fixture failed '+result.stderr[-5000:])
    output=json.loads(result.stdout);cases=output['cases'];segments=[];wanted=[]
    forced=subprocess.run([str(binary),str(inputs)],capture_output=True,text=True,env=dict(os.environ,HALO2_REFERENCE_KERNEL_YIELD='1'))
    if forced.returncode:raise ValueError('Forced-reference yield failed '+forced.stderr[-5000:])
    if json.loads(forced.stdout)!=output:raise ValueError('Forced-reference yield ABI/DATA differs')
    for n,row in enumerate(rows):
        actual,reference=cases[n*2:n*2+2]
        if actual|{'route':1}!=reference:raise ValueError('Native/retained yield differs '+row['name'])
        phase=actual['phase']
        if phase['call']!=dict(instruction=0x2dbf74,target=0x800000ee,tail=False):raise ValueError('Yield import call packet drift')
        if phase['after']['gp'][0]!=0:raise ValueError('POSIX yield did not produce genuine success status')
        before=copy.deepcopy(row);before.update(name=row['name']+'-before-real-import',stop_before_entries=[0x800000ee]);segments.append(before)
        wanted.append(dict(abi=phase,banks=phase['banks'],reads=phase['reads'],writes=phase['writes'],stop=0x800000ee))
        after=copy.deepcopy(row);after.update(phase['after']);after.update(name=row['name']+'-after-real-import',entry=0x2dbf7a,banks=phase['banks'])
        stack=after['gp'][4];after.update(return_pc=read(after['banks'],stack),matrix=read(after['banks'],stack+4),source=read(after['banks'],stack+8));segments.append(after)
        service_count=len(phase['service_reads']);wanted.append(dict(abi=actual,banks=actual['banks'],reads=actual['reads'][len(phase['reads'])+service_count:],writes=actual['writes'][len(phase['writes']):],stop=0x63000000))
    for n,row in enumerate(segments):row['id']=n
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    metadata=common.metadata
    def truthful_metadata(image):
        value=metadata(image);code=common._Code(image,'.text')
        for site in value['sites']:
            if code.at(site['instruction']).mnemonic=='ret':site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return value
    common.metadata=truthful_metadata
    physical,proof=physical_original(image,segments,scratch,entries=(0x2dbf74,),regions=REGIONS,preserve_x87_prefix=True)
    executed=set()
    for original,expected,row in zip(physical['cases'],wanted,segments,strict=True):
        a=expected['abi'];normal=dict(gp=original['boundary_gp'],eflags=original['boundary_eflags'],xmm=original['boundary_xmm'],fp=original['after_fp'])
        required=dict(gp=a['gp'],eflags=a['eflags'],xmm=a['xmm'],fp=a['before_fp'])
        if normal!=required:raise ValueError('Original yield ABI differs '+row['name']+' '+str(normal)+' expected '+str(required))
        if not original['completed']or not original['live_x87_unchanged']:raise ValueError('Original yield FP/completion differs '+row['name'])
        if original.get('stopped_at',0x63000000)!=expected['stop']:raise ValueError('Original yield stopped at wrong owner')
        original_banks=[dict(address=b['address'],before=b['after'])for b in original['banks']]
        if original_banks!=expected['banks']:raise ValueError('Original yield DATA differs '+row['name'])
        # Physical CALL/RET implicit accesses are measured; this source ABI's
        # call push and return reads must have the same ordered data accesses.
        reads=[dict(address=e['address'],bytes=e['bytes'])for e in original['reads']]
        stores=[{k:e[k]for k in('address','before','written')}for e in original['writes']]
        if reads!=expected['reads']or stores!=expected['writes']:raise ValueError('Original yield access stream differs '+row['name']+' '+str(reads)+' expected '+str(expected['reads']))
        executed.update(original['executed_sites'])
    source_sites={int(v,16)for v in __import__('re').findall(r'instruction\(0x([0-9a-f]+)u\)',body)}
    if source_sites!=executed:raise ValueError('Original yield source-site coverage incomplete')
    report=dict(passed=True,native_and_retained_comparisons=len(rows)*2,forced_reference_comparisons=len(rows)*2,physical_original_segments=len(segments),actual_host_yields=len(rows)*4,
        original_source_sites=len(source_sites),push_over_import_alias_cases=sum('push-over-import'in row['name']for row in rows),admission_refusals=output['admission_refusals'],provider_controls=output['provider_controls'],
        whole_wrapper_body_sha256=BODY_PIN,original_xbe_sha256=PIN,physical_original=proof,
        normal_gp_flags_simd_fp_live80_banks_and_ordered_accesses_exact=True,native_instruction_callbacks=0,
        host_policy='Actual POSIX sched_yield success mapped to NTSTATUS0; same existing DeathStartup policy. Windows implementation calls real NtYieldExecution but is not qualified by this Linux cohort.',
        segmentation_scope='Unchanged original wrapper executes to real import entry. Actual host service owns status and frame return; unchanged original continuation starts from that actual service ABI. No Xbox kernel instructions/outcomes supplied or Xbox scheduler equivalence claimed.',
        original_kernel_scheduler_qualified=False,pending_request_completion_qualified=False,exceptional_partial_abi_qualified=False,production_wrapper_bound=args.require_binding,rejected_mutations=rejected,
        source_sha256=source_pins,source_drift=[name for name,pin in source_pins.items()if sha(ROOT/name)!=pin],compile_command=command,address_undefined_sanitizers=args.sanitizers,scratch_directory=str(scratch),retained_large_artifacts=[])
    if report['source_drift']:raise ValueError('Kernel-yield proof input drift '+str(report['source_drift']))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(f'PASS {len(rows)*2} actual-host native/retained yield calls, {len(segments)} unchanged-original segments');return 0

if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            out=Path(sys.argv[sys.argv.index('--report')+1]);out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(dict(passed=False,error=repr(error),scratch_directory=os.environ.get('HALO2_TEST_SCRATCH'),retained_large_artifacts=[]),indent=2)+'\n')
        raise
