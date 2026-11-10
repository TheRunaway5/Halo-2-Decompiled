#!/usr/bin/env python3
"""Qualify actual166C00 queue lookup against unchanged physical i386."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import physical_original,PIN
REGIONS=((0x61000000,16384),(0x62000000,8192))
UNIT='src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'
INPUTS=('include/halo2/native_tu15_id_search_kernel.hpp','src/rendering/native_tu15_bsp_queue_find.hpp','tests/native_tu15_bsp_queue_find_tests.cpp','tools/validate_native_bsp_queue_find_tu15.py','tools/native_bsp_queue_find_replacements.py',UNIT,'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/specialize_native_operands.py','tools/native_sphere_frustum_mmx_original.py','src/physics/runtime/physics_step_internal.hpp')
def corpus():
    rng=random.Random(0x166c00);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f','01000000000000000000','00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    counts=(0,1,1,2,5,16,33,128,32767,32768,65535,5,2,4,0,2)
    for family,count in enumerate(counts):
        for variant in range(16):
            banks=[bytearray(rng.randbytes(size))for _,size in REGIONS];descriptor=0x61000100+variant;cursor=0x61001000+variant;sp=0x62000e00+variant
            if family==11:cursor=sp+4
            if family==12:cursor=descriptor-4
            if family==13:cursor=sp-4
            if family==14:descriptor=sp-4
            if family==15:descriptor=sp+4
            def put(at,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Unbacked lookup fixture')
            def get(at):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:return int.from_bytes(bank[at-base:at-base+4],'little')
                raise ValueError('Unbacked lookup read')
            key=rng.getrandbits(32)
            for index in range(min(count,128)):put(cursor+index*4,(key+index+1)&0xffffffff)
            found=0 if count>=32767 else count-1
            if family in(2,6):found=-1
            if family==4:found=1;put(cursor+3*4,key) # first equal must win
            if found>=0:put(cursor+found*4,key)
            put(descriptor+4,count,2);put(descriptor+12,cursor)
            put(sp,0x63000000)
            if family==13:key=0x63000000
            if family==12:key=get(cursor)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[2]=descriptor;gp[4]=sp;gp[6]=key
            depth=(0,1,4,8)[variant%4];cw=(0x27f,0x37f,0x77f,0xf7f)[variant%4];status=((variant%8)<<11)|0x4500
            rows.append(dict(id=len(rows),name=f'family-{family}-variant-{variant}',entry=0x166c00,gp=gp,eflags=0x202|rng.choice((0,0x400))|rng.choice((0,1,0x10,0x40,0x80,0x800,0x8d5)),matrix=get(sp+4),source=get(sp+8),max_steps=10000,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)],live_x87=list(live[:depth]),banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def prepare(scratch,source,require_binding=False):
    from generated_routine_ownership import function_bodies
    import native_bsp_queue_find_replacements as binding
    match,parent=binding.authenticate(source);bound=binding.replace_bsp_queue_find(source)
    if require_binding and bound!=source:raise ValueError('Production166C00 binding missing')
    if binding.replace_bsp_queue_find(bound)!=bound:raise ValueError('166C00 binding nonidempotent')
    from specialize_native_operands import specialize
    prepared,receipt=specialize(bound)
    target=lambda text:next(b for sig,b in function_bodies(text)if sig[1]=='166c00')
    def instrument(body):
        changed,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',body)
        if count!=14:raise ValueError('Missing14 literal lookup sites')
        return changed
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
    text=resolver+'\nnamespace retained {\n'+instrument(parent)+'\n}\n'
    for name,text_source in(('bound',bound),('prepared',prepared)):
        text+='namespace '+name+' {\n'+instrument(target(text_source))+'\n}\n'
    (scratch/'bsp_queue_find_bodies.inc').write_text(text)
    return dict(original_parent_sha256=binding.BODY_PIN,bound_parent_sha256=binding.digest(target(bound)),prepared_parent_sha256=binding.digest(target(prepared)))
def semantic_mutants(scratch,command,fixture):
    definitions=(
        ('unequal-key','include/halo2/native_tu15_id_search_kernel.hpp','if(value==view.key())','if(value!=view.key())'),
        ('signed-count','src/rendering/native_tu15_bsp_queue_find.hpp','return s_.gp[1];','return std::uint32_t(std::int32_t(std::int16_t(s_.gp[1]))>0?s_.gp[1]:0);'),
        ('stale-cursor','src/rendering/native_tu15_bsp_queue_find.hpp','s_.gp[2]=s_.flags.plus(s_.gp[2],4u,4);','s_.gp[2]=s_.flags.plus(s_.gp[2],8u,4);'),
        ('cleared-callback-admission','src/rendering/native_tu15_bsp_queue_find.hpp','else if(s.instruction_cursor||s.literal_service_sites)return false;','else if(false)return false;'))
    results=[]
    for name,path,old,new in definitions:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Missing unique queue lookup mutant '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay));compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Queue lookup mutant did not compile '+name+': '+compiled.stderr[-2500:])
        tested=subprocess.run([str(binary),'--guards'if name=='cleared-callback-admission'else str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if tested.returncode==0:raise ValueError('Compiled queue lookup semantic mutant survived '+name)
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
    common.metadata=metadata;common.ENTRIES=(0x166c00,);sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=[site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')];write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    rows=corpus();directory=scratch/'normal';directory.mkdir();physical,authority=physical_original(image,rows,directory,entries=(0x166c00,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    prefix_authority=None
    if args.prefixes:
        prefixes=[]
        for index in(0,16,32,64,112,176,192,208,240):
            for phase,key in(('before_read','reads'),):
                for ordinal,event in enumerate(physical['cases'][index][key]):
                    row=copy.deepcopy(rows[index]);row.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(row)
        directory=scratch/'prefix';directory.mkdir();stopped,prefix_authority=physical_original(image,prefixes,directory,entries=(0x166c00,),regions=REGIONS,trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True);rows+=prefixes;physical['cases']+=stopped['cases']
    from native_sphere_frustum_mmx_original import physical_original_mmx,PROBE
    rng=random.Random(0x166c0080);mmx_rows=[]
    for index,source in enumerate(rows):
        row=copy.deepcopy(source);row['id']=len(rows)+index;row['name']='mmx-'+row['name'];row['live_x87']=[];row['before_fp'][1]&=~0x3800
        patterns=('00000000000000000000','00000000000000000080','0000000000000080ff3f','01000000000000000000','45230100000000c0ff7f','efcdab8967452301ff3f')
        row['active_mmx']=dict(known_mask=index%256,physical=[patterns[(index+n)%11]if(index+n)%11<len(patterns)else(rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(index+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
        mmx_rows.append(row)
    directory=scratch/'mmx';directory.mkdir();mmx,mmx_authority=physical_original_mmx(image,mmx_rows,directory,entries=(0x166c00,),regions=REGIONS+((PROBE,4096),),trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in io],trace_extended=True,preserve_x87_prefix=True)
    rows+=mmx_rows;physical['cases']+=mmx['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else 0x166c20
        if 'active_mmx'in row:row['boundary_active_mmx']=observed['active_mmx']
        for call in row['calls']:assert call.pop('arguments')==[]
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=write_sites),separators=(',',':')));binary=scratch/'queue-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_bsp_queue_find_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Queue lookup compile failed: '+compiled.stderr[-7000:])
    tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    checks=[]
    if tested.returncode==0:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_BSP_QUEUE_FIND':'1'}),(str(fixture),{'HALO2_REFERENCE_BSP_QUEUE_FIND':'1','HALO2_BSP_QUEUE_FIND_REFERENCE_TEST':'1'})):
            extra=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option if option.startswith('--')else 'positive_forced_reference',passed=extra.returncode==0,diagnostic=(extra.stdout+extra.stderr)[-6000:]))
    mutants=semantic_mutants(scratch,command,fixture)if args.semantic_mutants and tested.returncode==0 and all(c['passed']for c in checks)else []
    report=dict(passed=tested.returncode==0 and all(c['passed']for c in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,source_route_authority=authority_source,additional_source_checks=checks,physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),prefix_cases=sum('fault'in row for row in rows),executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_original=authority,physical_prefix_original=prefix_authority,physical_mmx=mmx_authority,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=(tested.stdout+tested.stderr)[-18000:],retained_large_artifacts=[])
    if tested.returncode==0:report['native']=json.loads(tested.stdout)
    report['semantic_mutants']=mutants
    if pins!={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS}:raise ValueError('Queue lookup proof source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True)
            report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
