#!/usr/bin/env python3
"""Private whole135970 proof using genuine frustum and auxiliary algorithms."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from validate_native_tu15_prt_helpers import PIN
from native_geometry_visibility_original import observe as physical_original
REGIONS=((0x46a000,4096),(0x61000000,73728),(0x62000000,16384))
UNITS=('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp')
INPUTS=(*UNITS,'include/halo2/native_tu15_geometry_visibility_kernel.hpp','src/rendering/native_tu15_geometry_visibility.hpp','tools/native_geometry_visibility_replacements.py','tools/validate_native_geometry_visibility_tu15.py','tests/native_tu15_geometry_visibility_tests.cpp','src/rendering/native_tu15_cluster_frustum_abi.hpp','include/halo2/native_tu15_cluster_frustum_kernel.hpp','src/rendering/native_tu15_sphere_frustum_abi.hpp','include/halo2/native_tu15_sphere_frustum_kernel.hpp','src/rendering/native_tu15_bsp_queue_find.hpp','include/halo2/native_tu15_id_search_kernel.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_sse_mmx_scope.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
INPUTS+=('tools/native_geometry_visibility_original.py','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','tools/specialize_native_operands.py')
def corpus(image):
    rng=random.Random(0x135970);rows=[];live=('0000000000000080ff3f','35c26821a2da0fc90040','45230100000000c0ff7f')
    for family in range(32):
        for variant in range(16):
            banks=[bytearray(image.read(base,size))if base==0x46a000 else bytearray(rng.randbytes(size))for base,size in REGIONS]
            context=0x61001000+variant;center=0x61005000+variant;output=0x61006000+variant;first=output+64;second=output+128;descriptor=0x61008000+variant;queue=0x61010000+variant;items=0x61011000+variant;sp=0x62002000+variant
            if family==16:queue=0x61010001
            def put(at,value,width=4):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-width:bank[at-base:at-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
                raise ValueError('Unbacked geometry visibility fixture '+hex(at))
            def get(at):
                for(base,size),bank in zip(REGIONS,banks):
                    if base<=at<=base+size-4:return int.from_bytes(bank[at-base:at-base+4],'little')
                raise ValueError('Unbacked geometry caller argument')
            bits=lambda value:struct.unpack('<I',struct.pack('<f',value))[0]
            mode=(0,1,2,0x12345678)[family%4];radius=(1.,2.,3.,0.5)[variant%4]
            if family>=12:mode=family%2
            position=(0.,0.,0.)if variant%3 else(4.,0.,0.);volume_radius=(1.,10.,3.,0.)[variant%4]
            put(context,descriptor);put(context+0x2a60,1 if family%3 else 0,1);put(context+0x2a78,1 if family%5==0 else 0);put(context+0x2a7c,mode)
            for axis in range(3):put(context+0x2a68+axis*4,bits(0.));put(center+axis*4,bits(position[axis]))
            put(context+0x2a74,bits(volume_radius));put(descriptor,0 if family==0 else 1,2)
            for group in range(6):put(descriptor+0xa70+group*2,1 if not group else 0,2);put(descriptor+0xa7c+group*2,0,2)
            frustum=descriptor+0x1974
            for axis in range(3):put(frustum+0x60+axis*8,bits(-10.));put(frustum+0x64+axis*8,bits(10.))
            for plane in range(6):
                values=[0.,0.,0.,(9.,0.,-5.)[variant%3]];values[plane//2]=1. if plane%2==0 else -1.
                for lane,value in enumerate(values):put(frustum+0x78+plane*16+lane*4,bits(value))
            key=0x12340002;count=(0,1,2,3,257,512)[family%6];put(queue+4,count,2);put(queue+12,items)
            match=(0,count//2,count-1,count)[variant%4]
            for index in range(count):put(items+index*4,key if index==match else key+1)
            if family==12:output=context+0x2a7c # initial byte clear selects mode0
            if family==13:output=sp-4 # later saved-BX alias
            if family==14:output=center # initial byte clear affects actual sphere
            if family==15:first=output # initial flags and containment alias
            if family==16:second=sp+8 # initial byte publication changes actual auxiliary pointer
            if family==17:put(center,0x7fc12345)
            if family==18:put(context+0x2a68,0x80000001);put(center,0x00000001)
            if family==19:output=sp-8 # saved-BP alias
            if family==20:
                mode=1;put(context+0x2a7c,mode)
                for axis in range(3):put(center+axis*4,0x7fc12345+axis);put(context+0x2a68+axis*4,0x7fc54321+axis)
            if family==21:mode=1;put(context+0x2a7c,mode);radius=struct.unpack('<f',struct.pack('<I',0x7fc01234))[0];put(context+0x2a74,0x7fc05678)
            if family==22:
                mode=1;put(context+0x2a7c,mode);radius=-0.;put(context+0x2a74,0)
                for axis in range(3):put(center+axis*4,0x80000000);put(context+0x2a68+axis*4,0)
            if family==23:
                mode=1;put(context+0x2a7c,mode);radius=struct.unpack('<f',struct.pack('<I',0x00800001))[0];put(context+0x2a74,0x00800002)
                for axis in range(3):put(center+axis*4,axis+1);put(context+0x2a68+axis*4,0x80000001+axis)
            if family==24:mode=1;put(context+0x2a7c,mode);put(center,bits(1.e20));put(context+0x2a68,bits(-1.e20));put(context+0x2a74,bits(3.e20))
            if family==25:first=0x46a104;mode=0;put(context+0x2a7c,mode);radius=struct.unpack('<f',struct.pack('<I',0x40200001))[0]
            if family==26:output=context+0x2a74;put(context+0x2a74,0x40400001)
            if family==27:output=sp+4;radius=struct.unpack('<f',struct.pack('<I',0x40200001))[0]
            if family==28:output=queue+4
            if family==29:first=queue+12
            if family==30:output=sp-12 # saved-SI alias
            if family==31:output=sp-16 # conditional saved-DI alias
            put(sp,0x63000000);put(sp+4,bits(radius));put(sp+8,(queue^1)if family==16 else queue if family%3 else 0);put(sp+12,key);put(sp+16,0);put(sp+20,variant%2 if family%3==0 else 0);put(sp+24,output);put(sp+28,first);put(sp+32,second)
            gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=context;gp[1]=center;gp[4]=sp
            depth=(0,1,2,3)[variant%4];cw=(0x7f,0x27f,0x37f)[variant%3]|((variant%4)<<10);status=((variant%8)<<11)|0x4500
            rows.append(dict(id=len(rows),name=f'family{family}-variant{variant}',entry=0x135970,gp=gp,eflags=0x202|rng.choice((0,0x400))|(rng.getrandbits(12)&0x8d5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[cw,status,0x1f80|((family%4)<<13)|(0,0x40,0x8000,0x8040)[variant%4]],live_x87=list(live[:depth]),matrix=get(sp+4),source=get(sp+8),max_steps=100000,banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def prepare(scratch,require_binding=False):
    import native_geometry_visibility_replacements as binding
    from audit_native_execution_ownership import unique_function
    from generated_routine_ownership import function_bodies
    from specialize_native_operands import specialize
    text='';receipts={}
    for index,unit in enumerate(UNITS):
        source=(ROOT/unit).read_text();match,body=binding.authenticate(source)
        if not index:text+=unique_function(source,'sign_extend')+'\n'
        bound=binding.replace_geometry_visibility(source)
        if require_binding and bound!=source:raise ValueError('Production135970 default is not installed')
        if binding.replace_geometry_visibility(bound)!=bound:raise ValueError('135970 installer is not idempotent')
        prepared,_=specialize(bound)
        target=lambda source:next(body for sig,body in function_bodies(source)if sig[1]=='135970')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        helpers='''
static void run_166c00(WorldState&s){if(::capture_child)::capture_child(s,0x166c00u);if(!halo2::native_bsp_queue_find_detail::bridge(s))throw std::runtime_error("Genuine auxiliary owner refused");}
static void run_167ef0(WorldState&s){if(::capture_child)::capture_child(s,0x167ef0u);if(!halo2::native_cluster_frustum_abi_detail::bridge(s))throw std::runtime_error("Genuine frustum owner refused");}
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x166c00u)run_166c00(s);else if(target==0x167ef0u)run_167ef0(s);else throw std::runtime_error("Unknown genuine geometry child");}
'''
        for name,owner in(('retained',body),('bound',target(bound)),('prepared',target(prepared))):
            instrumented,count=re.subn(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::retained_statement();',owner)
            if count!=115:raise ValueError('Incomplete115site geometry source '+name)
            text+='namespace '+('other_'if index else'')+name+' {\n'+resolver+helpers+instrumented+'\n}\n'
        receipts[unit]=dict(original_body_sha256=binding.digest(body),bound_body_sha256=binding.digest(target(bound)),prepared_body_sha256=binding.digest(target(prepared)),original_body_sites=115,resolver_sha256=binding.digest(resolver),reference_children='Actual already-qualified native frustum/auxiliary algorithms; original i386 executes all genuine child instructions')
    (scratch/'geometry_visibility_bodies.inc').write_text(text);return receipts
def semantic_mutants(scratch,command,fixture):
    definitions=(
        ('wrong-sphere-axis','include/halo2/native_tu15_geometry_visibility_kernel.hpp','d.distance_squared=view.add(d.square_z,d.square_x);','d.distance_squared=view.add(d.square_z,d.square_y);'),
        ('exclude-tangent-overlap','src/rendering/native_tu15_geometry_visibility.hpp','return !comparison.below;','return !comparison.below&&!(comparison.flags&0x40u);'),
        ('wide-output-publication','src/rendering/native_tu15_geometry_visibility.hpp','const std::array bytes{std::byte(value)};','const std::array bytes{std::byte(value),std::byte(0),std::byte(0),std::byte(0)};'),
        ('lose-return-upper-eax','src/rendering/native_tu15_geometry_visibility.hpp','s_.set(0,g[3]&255u,1,0);','g[0]=g[3]&255u;'),
        ('wrong-extended-norm','src/rendering/native_tu15_geometry_visibility.hpp',';fsqrt;fstp',';fabs;fstp'),
        ('cleared-callable-admission','src/rendering/native_tu15_geometry_visibility.hpp','else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;','else if(s.observe)return false;'))
    receipts=[]
    for name,path,old,new in definitions:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Missing unique geometry mutant '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay));compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Geometry mutant failed to compile '+name+': '+compiled.stderr[-4000:])
        tested=subprocess.run([str(binary),'--guards'if name=='cleared-callable-admission'else str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
        if tested.returncode==0:raise ValueError('Compiled geometry mutant survived '+name)
        receipts.append(dict(name=name,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-2000:]))
    return receipts
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--require-binding',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in INPUTS};source=prepare(scratch,args.require_binding)
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            if code.at(site['instruction']).mnemonic in('ret','pop'):site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
        return result
    common.metadata=metadata;entries=(0x135970,0x167ef0,0x184690,0x166c00);common.ENTRIES=entries;sites=metadata(image)['sites'];code=common._Code(image,'.text')
    io=sorted({site['instruction']for site in sites if site['operands']or code.at(site['instruction']).mnemonic in('push','call')});write_sites=[site['instruction']for site in sites if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')]
    extra=sorted({0x167ef0,0x166c00}-set(io));deltas={0x135a87:1,0x135a89:1,0x135a8f:2,0x135a92:2,0x135a98:3,0x135ab3:1,0x135ab7:1}
    calls=[dict(entry=pc,gp=True,xmm=True,x87_delta=deltas.get(pc,0))for pc in sorted(set(io)|set(extra))];rows=corpus(image);directory=scratch/'normal';directory.mkdir();physical,authority=physical_original(image,rows,directory,entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True)
    prefix_authority=None;witnesses=set()
    if args.prefixes:
        prefixes=[]
        for row,actual in zip(rows,physical['cases'],strict=True):
            family=row['name'].split('-')[0];depth=len(row['live_x87'])
            if depth not in(0,3):continue
            domain=family if family in('family13','family19','family30','family31')else'ordinary'
            for phase,key in(('before_read','reads'),('before_write','writes'),('after_write','writes')):
                for ordinal,event in enumerate(actual[key]):
                    witness=(phase,event['instruction'],depth,domain)
                    if witness in witnesses:continue
                    witnesses.add(witness);prefix=copy.deepcopy(row);prefix.update(id=len(rows)+len(prefixes),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));prefixes.append(prefix)
        directory=scratch/'prefix';directory.mkdir();stopped,prefix_authority=physical_original(image,prefixes,directory,entries=entries,regions=REGIONS,trace_calls=calls,trace_extended=True)
        rows+=prefixes;physical['cases']+=stopped['cases']
    for row,observed in zip(rows,physical['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes')});row['expected_pc']=observed['boundary']['instruction']if row.get('fault')else observed['calls'][-1]['entry'];row['after_live_x87']=observed['boundary_live_x87'];row['extra_child_calls']=[];row['calls']=[]
        for call in observed['calls']:
            assert call.pop('arguments')==[]
            row['extra_child_calls'if call['entry']in extra else'calls'].append(call)
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=write_sites,extra_child_entries=extra),separators=(',',':')));binary=scratch/'geometry-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_geometry_visibility_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Geometry prototype compile failed: '+compiled.stderr[-6000:])
    tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    checks=[]
    if tested.returncode==0:
        for option,env in(('--guards',{}),('--reference-guards',{'HALO2_REFERENCE_GEOMETRY_VISIBILITY':'1'}),(str(fixture),{'HALO2_REFERENCE_GEOMETRY_VISIBILITY':'1','HALO2_GEOMETRY_VISIBILITY_REFERENCE_TEST':'1'})):
            extra_run=subprocess.run([str(binary),option],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**env));checks.append(dict(option=option if option.startswith('--')else'positive_forced_reference',passed=extra_run.returncode==0,diagnostic=(extra_run.stdout+extra_run.stderr)[-6000:]))
    mutants=semantic_mutants(scratch,command,fixture)if args.semantic_mutants and tested.returncode==0 and all(check['passed']for check in checks)else[]
    changed=[name for name,pin in pins.items()if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin]
    if changed:raise ValueError('Geometry prototype source input drift '+', '.join(changed))
    report=dict(passed=tested.returncode==0 and all(check['passed']for check in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,source_route_authority=source,additional_source_checks=checks,compiled_semantic_mutants=mutants,physical_cases=len(rows),normal_cases=sum('fault'not in row for row in rows),prefix_cases=sum('fault'in row for row in rows),prefix_witnesses=len(witnesses),executed_sites=sorted({hex(pc)for row in physical['cases']for pc in row['executed_sites']}),physical_original=authority,physical_prefix_original=prefix_authority,compiler=args.compiler,sanitizers=args.sanitizers,diagnostic=(tested.stdout+tested.stderr)[-16000:],retained_large_artifacts=[])
    if tested.returncode==0:report['native']=json.loads(tested.stdout)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(tested.stdout+tested.stderr);return 0 if report['passed']else 1
if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            report=Path(sys.argv[sys.argv.index('--report')+1]);report.parent.mkdir(parents=True,exist_ok=True);report.write_text(json.dumps(dict(passed=False,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-10000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
