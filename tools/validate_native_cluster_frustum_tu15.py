#!/usr/bin/env python3
"""Qualify actual167EF0 with genuine184690 against unchanged physical i386."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import physical_original,PIN
REGIONS=((0x61000000,65536),(0x62000000,8192))
UNIT='src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'
INPUTS=('include/halo2/native_tu15_cluster_frustum_kernel.hpp','include/halo2/native_tu15_cluster_frustum_value.hpp','src/rendering/native_tu15_cluster_frustum_abi.hpp','tests/native_tu15_cluster_frustum_tests.cpp','tools/validate_native_cluster_frustum_tu15.py','tools/native_cluster_frustum_replacements.py','include/halo2/native_tu15_sphere_frustum_kernel.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
INPUTS+=('tools/specialize_native_operands.py','tools/native_sphere_frustum_replacements.py','src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/physics_solver_internal.hpp')
def corpus():
    rng=random.Random(0x167ef0);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    for family in range(18):
        for variant in range(16):
            n=len(rows);banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];context=0x61000100+variant;center=0x6100e000+variant;output=0x6100e100+variant;sp=0x62000e00+variant
            def put(at,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Unbacked cluster fixture')
            def scalar(v):return struct.unpack('<I',struct.pack('<f',v))[0]
            groups=(-1,0,1,2,6)[family%5];ordinal=variant%3
            if family>=16:groups=1
            if family==16:ordinal=0xffffffff
            put(context,groups,2)
            for entry in range(-1,3):
                row=context+0xa6e+entry*26
                for group in range(6):put(row+2+group*2,(0,1,2,-1)[(family+group)%4],2);put(row+14+group*2,group%2,2)
            if family>=16:put(context+0xa6e+(ordinal if ordinal!=0xffffffff else -1)*26+2,2,2)
            if family==17:put(context+0xa6e+ordinal*26+14,-1,2)
            for frustum in range(-1,3):
                address=context+0x1974+frustum*264
                for axis in range(3):put(address+0x60+axis*8,scalar(-10.));put(address+0x64+axis*8,scalar(10.))
                for plane in range(6):
                    values=[0.,0.,0.,9.];values[plane//2]=1. if plane%2==0 else -1.
                    if family%4==0:values[3]=-2.
                    elif family%4==1:values[3]=0.
                    for lane,value in enumerate(values):put(address+0x78+plane*16+lane*4,scalar(value))
            for axis in range(3):put(center+axis*4,scalar(0.))
            if family==10:output=context # output aliases the future group count
            if family==11:output=context+0xa70+ordinal*26 # output aliases current range count
            if family==12:output=context+0xa7c+ordinal*26 # output aliases first frustum index
            if family==13:output=sp-8 # output aliases saved BP, restored only after traversal
            if family==14:put(center,0x7fc12345)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=ordinal;gp[4]=sp;gp[7]=context
            xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];xmm[7][0]=scalar(1.)
            put(sp,0x63000000);put(sp+4,center);put(sp+8,output)
            depth=(0,1,4,8)[variant%4];cw=(0x27f,0x37f,0x77f,0xf7f)[variant%4];status=((variant%8)<<11)|0x4500
            rows.append(dict(id=n,name=f'family-{family}-variant-{variant}',entry=0x167ef0,gp=gp,eflags=0x202|rng.choice((0,0x400))|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5)),matrix=center,source=output,max_steps=100000,xmm=xmm,before_fp=[cw,status,0x1f80|((family%4)<<13)],live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def prepare(scratch,source,require_binding=False):
    from generated_routine_ownership import function_bodies
    import native_cluster_frustum_replacements as binding
    from native_sphere_frustum_replacements import canonical_body as child_body
    body=lambda entry:next(body for sig,body in function_bodies(source)if sig[1]==entry)
    binding.authenticate(source);child=child_body(body('184690'));parent=binding.canonical_body(body('167ef0'));bound=binding.replace_cluster_frustum(source)
    if require_binding and bound!=source:raise ValueError('Production167EF0 default binding missing')
    if binding.replace_cluster_frustum(bound)!=bound:raise ValueError('167EF0 binding nonidempotent')
    from specialize_native_operands import specialize
    prepared,receipt=specialize(bound)
    target=lambda text,entry:next(b for sig,b in function_bodies(text)if sig[1]==entry)
    def instrument(body):
        changed,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',body)
        if count<1:raise ValueError('Missing literal-body counter witness')
        return changed
    from audit_native_execution_ownership import unique_function
    helper=unique_function(source,'sign_extend')
    text=helper+'\nnamespace retained {\n'+instrument(child)+'\n'+instrument(parent)+'\n}\n'
    for name,text_source in(('bound',bound),('prepared',prepared)):
        text+='namespace '+name+' {\n'+instrument(target(text_source,'184690'))+'\n'+instrument(target(text_source,'167ef0'))+'\n}\n'
    (scratch/'cluster_frustum_bodies.inc').write_text(text)
    return dict(original_parent_sha256=binding.BODY_PIN,bound_parent_sha256=binding.digest(target(bound,'167ef0')),prepared_parent_sha256=binding.digest(target(prepared,'167ef0')))
def semantic_mutants(scratch,command,fixture):
    definitions=(
        ('containment-does-not-stop','include/halo2/native_tu15_cluster_frustum_kernel.hpp','if(view.contained())break;','if(false)break;'),
        ('inclusive-range-limit','include/halo2/native_tu15_cluster_frustum_kernel.hpp','while(range.index<range.count)','while(range.index<=range.count)'),
        ('wide-contained-publication','src/rendering/native_tu15_cluster_frustum_abi.hpp','store(0x167f51u,g[5],g[1]&255u,1);','store(0x167f51u,g[5],g[1]&255u,4);'),
        ('cleared-callback-admission','src/rendering/native_tu15_cluster_frustum_abi.hpp','else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;','else if(s.observe)return false;'))
    results=[]
    for name,path,old,new in definitions:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Missing unique cluster mutant '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay));compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Cluster mutant did not compile '+name+': '+compiled.stderr[-2500:])
        tested=subprocess.run([str(binary),'--guards'if name=='cleared-callback-admission'else str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if tested.returncode==0:raise ValueError('Compiled cluster semantic mutant survived '+name)
        results.append(dict(name=name,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-2500:]))
    return results
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};authority_source=prepare(scratch,(ROOT/UNIT).read_text(),args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result
    common.metadata=metadata;common.ENTRIES=(0x167ef0,0x184690);sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')];write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rows=corpus();directory=scratch/'normal';directory.mkdir();physical,authority=physical_original(image,rows,directory,entries=(0x167ef0,0x184690),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    prefix_authority=None
    if args.prefixes:
        prefixes=[]
        for index in(32,39,64,151):
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                for ordinal,event in enumerate(physical['cases'][index][key]):
                    row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
        directory=scratch/'prefix';directory.mkdir();stopped,prefix_authority=physical_original(image,prefixes,directory,entries=(0x167ef0,0x184690),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;physical['cases']+=stopped['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else 0x167f74
        for call in row['calls']:assert call.pop('arguments')==[]
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=write_sites),separators=(',',':')));binary=scratch/'cluster-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_cluster_frustum_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Cluster compile failed: '+compiled.stderr[-7000:])
    tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    checks=[]
    if tested.returncode==0:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_CLUSTER_FRUSTUM':'1'}),(str(fixture),{'HALO2_REFERENCE_CLUSTER_FRUSTUM':'1','HALO2_CLUSTER_FRUSTUM_REFERENCE_TEST':'1'})):
            extra=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option if option.startswith('--')else 'positive_forced_reference',passed=extra.returncode==0,diagnostic=(extra.stdout+extra.stderr)[-6000:]))
    mutants=semantic_mutants(scratch,command,fixture)if args.semantic_mutants and tested.returncode==0 and all(c['passed']for c in checks)else []
    report=dict(passed=tested.returncode==0 and all(c['passed']for c in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,source_route_authority=authority_source,additional_source_checks=checks,physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),prefix_cases=sum('fault'in row for row in rows),executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_original=authority,physical_prefix_original=prefix_authority,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=(tested.stdout+tested.stderr)[-18000:],retained_large_artifacts=[])
    if tested.returncode==0:report['native']=json.loads(tested.stdout)
    report['semantic_mutants']=mutants
    if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}:raise ValueError('Cluster proof source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True)
            report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
