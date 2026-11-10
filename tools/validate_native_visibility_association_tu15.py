#!/usr/bin/env python3
"""Qualify actual136E10 visibility association against unchanged physical i386."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import physical_original,PIN
REGIONS=((0x61000000,65536),(0x62000000,32768))
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/runtime/source_map_world_support_tu15_generated.cpp')
INPUTS=(*UNITS,'include/halo2/native_tu15_visibility_association_kernel.hpp','src/rendering/native_tu15_visibility_association.hpp','tests/native_tu15_visibility_association_tests.cpp','tools/validate_native_visibility_association_tu15.py','tools/native_visibility_association_replacements.py','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/specialize_native_operands.py','src/physics/runtime/physics_step_internal.hpp','tools/native_sphere_frustum_mmx_original.py')
def corpus():
    rng=random.Random(0x136e10);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    counts=(0,1,2,7,31,255,510,511,512,513,32767,32768,65535,0,512,0,0,0)
    ordinals=(-32768,-129,-128,-33,-32,-1,0,1,15,31,32,63,64,95,127,32767)
    for family,count in enumerate(counts):
        for variant,ordinal in enumerate(ordinals):
            banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];context=0x61003000+variant;sp=0x62004000+variant
            def put(at,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Unbacked association fixture '+hex(at))
            def get(at,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:return int.from_bytes(bank[at-base:at-base+width],'little')
                raise ValueError('Unbacked association input')
            if family==13:context=sp+4-0xa5e;ordinal=0 # counter aliases lazy argument
            if family==14:sp=context+0xa62 # saturated counter aliases saved SI
            if family in(15,16,17):sp=context+0xa70;ordinal=(0,64,96)[family-15]
            put(context+0xa5e,count,2);put(sp,0x63000000);put(sp+4,ordinal)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[2]=context;gp[4]=sp
            depth=(0,1,4,8)[variant%4];cw=(0x27f,0x37f,0x77f,0xf7f)[variant%4];status=((variant%8)<<11)|0x4500
            rows.append(dict(id=len(rows),name=f'family-{family}-variant-{variant}',entry=0x136e10,gp=gp,eflags=0x202|rng.choice((0,0x400))|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5)),matrix=get(sp+4),source=get(sp+8),max_steps=10000,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)],live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def prepare(scratch,require_binding=False):
    from generated_routine_ownership import function_bodies
    from audit_native_execution_ownership import unique_function
    import native_visibility_association_replacements as binding
    from specialize_native_operands import specialize
    text='';authority={}
    for index,unit in enumerate(UNITS):
        source=(ROOT/unit).read_text();match,body=binding.authenticate(source);bound=binding.replace_visibility_association(source)
        if require_binding and bound!=source:raise ValueError('Production136E10 binding missing')
        if binding.replace_visibility_association(bound)!=bound:raise ValueError('136E10 binding nonidempotent')
        prepared,receipt=specialize(bound)
        target=lambda source:next(body for sig,body in function_bodies(source)if sig[1]=='136e10')
        def instrument(body):
            changed,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',body)
            if count!=40:raise ValueError('Missing40 association literal sites')
            return changed
        if not index:
            text+=unique_function(source,'sign_extend')+'\n'
        prefix=''if not index else'other_'
        for name,owner in((prefix+'retained',binding.ordered_reference(body)),(prefix+'bound',target(bound)),(prefix+'prepared',target(prepared))):text+='namespace '+name+' {\n'+re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]+'\n'+instrument(owner)+'\n}\n'
        authority[unit]=dict(original_body_sha256=binding.digest(body),bound_body_sha256=binding.digest(target(bound)),prepared_body_sha256=binding.digest(target(prepared)))
    (scratch/'visibility_association_bodies.inc').write_text(text);return authority
def semantic_mutants(scratch,command,fixture):
    definitions=(
        ('capacity-off-by-one','src/rendering/native_tu15_visibility_association.hpp','return index<512u;','return index<=512u;'),
        ('counter-width','src/rendering/native_tu15_visibility_association.hpp','const std::array bytes{std::byte(value),std::byte(value>>8u)};','const std::array bytes{std::byte(value),std::byte(value>>8u),std::byte(value>>16u),std::byte(value>>24u)};'),
        ('wrong-bit','include/halo2/native_tu15_visibility_association_kernel.hpp','1u<<(std::uint32_t(ordinal)&31u)','1u<<((std::uint32_t(ordinal)+1u)&31u)'),
        ('discard-physical-shift-AF','src/rendering/native_tu15_visibility_association.hpp','((ah>>8u)&0xd5u)','((ah>>8u)&0xc5u)'),
        ('cleared-callback-admission','src/rendering/native_tu15_visibility_association.hpp','else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;','else if(s.observe)return false;'))
    results=[]
    for name,path,old,new in definitions:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Missing unique association mutant '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay));compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Visibility association mutant did not compile '+name+': '+compiled.stderr[-2500:])
        tested=subprocess.run([str(binary),'--guards'if name=='cleared-callback-admission'else str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if tested.returncode==0:raise ValueError('Compiled association semantic mutant survived '+name)
        results.append(dict(name=name,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-2500:]))
    return results
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};authority_source=prepare(scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result
    common.metadata=metadata;common.ENTRIES=(0x136e10,);sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')];write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rows=corpus();directory=scratch/'normal';directory.mkdir();physical,authority=physical_original(image,rows,directory,entries=(0x136e10,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    prefix_authority=None
    if args.prefixes:
        prefixes=[]
        for index in(0,16,80,112,128,192,208,224,240,256,272):
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                for ordinal,event in enumerate(physical['cases'][index][key]):
                    row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
        directory=scratch/'prefix';directory.mkdir();stopped,prefix_authority=physical_original(image,prefixes,directory,entries=(0x136e10,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;physical['cases']+=stopped['cases']
    from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
    rng=random.Random(0x136e1080);mmx_rows=[]
    for index,source in enumerate(rows):
        row=copy.deepcopy(source);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
        patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
        row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)]);mmx_rows.append(row)
    directory=scratch/'mmx';directory.mkdir();mmx,mmx_authority=physical_original_mmx(image,mmx_rows,directory,entries=(0x136e10,),regions=REGIONS+((PROBE,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    rows+=mmx_rows;physical['cases']+=mmx['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else observed['calls'][-1]['entry']
        if 'active_mmx'in row:row['boundary_active_mmx']=observed['active_mmx']
        for call in row['calls']:assert call.pop('arguments')==[]
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=write_sites),separators=(',',':')));binary=scratch/'association-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_visibility_association_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Visibility association compile failed: '+compiled.stderr[-7000:])
    tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    checks=[]
    if tested.returncode==0:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_VISIBILITY_ASSOCIATION':'1'}),(str(fixture),{'HALO2_REFERENCE_VISIBILITY_ASSOCIATION':'1','HALO2_VISIBILITY_ASSOCIATION_REFERENCE_TEST':'1'})):
            extra=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option if option.startswith('--')else 'positive_forced_reference',passed=extra.returncode==0,diagnostic=(extra.stdout+extra.stderr)[-6000:]))
    mutants=semantic_mutants(scratch,command,fixture)if args.semantic_mutants and tested.returncode==0 and all(c['passed']for c in checks)else []
    report=dict(passed=tested.returncode==0 and all(c['passed']for c in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,source_route_authority=authority_source,additional_source_checks=checks,physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),prefix_cases=sum('fault'in row for row in rows),executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_original=authority,physical_prefix_original=prefix_authority,physical_mmx=mmx_authority,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=(tested.stdout+tested.stderr)[-18000:],retained_large_artifacts=[])
    if tested.returncode==0:report['native']=json.loads(tested.stdout)
    report['semantic_mutants']=mutants
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    if changed:raise ValueError('Visibility association proof source drift: '+', '.join(changed))
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True)
            report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
