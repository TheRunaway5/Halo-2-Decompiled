#!/usr/bin/env python3
"""Actual1E68E0 native setup ABI with genuine recursive and surface children."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from validate_native_tu15_prt_helpers import PIN,physical_original
from generated_routine_ownership import function_bodies
from native_bsp_segment_setup_replacements import replace_bsp_segment_setup as replace_bsp_segment,canonical_body
from native_bsp_segment_replacements import canonical_body as segment_body,ORIGINAL_SPILL,ORDERED_SPILL
ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x1000000,0x5000),(0x44a000,8192),(0x46a000,4096),(0x62000000,16384),(0x610000,0x5000))
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')
INPUTS=COPIES+('cmake/NativeTu15BspSegmentSetup.cmake','tools/generate_physics_live_support_tu15.py','include/halo2/native_tu15_bsp_segment_setup_kernel.hpp','include/halo2/native_tu15_bsp_segment_kernel.hpp','src/physics/runtime/native_tu15_bsp_segment_setup.hpp','src/physics/runtime/native_tu15_bsp_segment.hpp','tests/native_tu15_bsp_segment_setup_abi_tests.cpp','tools/validate_native_bsp_segment_setup_tu15.py','tools/native_bsp_segment_setup_replacements.py','tools/native_bsp_segment_replacements.py','src/physics/native_tu15_bsp_segment_query.cpp','include/halo2/native_tu15_bsp_leaf_surface_kernel.hpp','src/physics/runtime/native_tu15_bsp_leaf_surface.hpp','include/halo2/native_tu15_bsp_surface_polygon_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_polygon.hpp','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_2d.hpp','src/runtime/source_map_world_support_tu15_generated.cpp','tools/native_bsp_leaf_surface_replacements.py','tools/native_selected_operand_binding.py','tools/native_halfword_bit_query_replacements.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/validate_native_bsp_segment_query_tu15.py','tools/validate_native_bsp_surface_polygon_abi_tu15.py','tools/native_bsp_surface_2d_replacements.py','tools/native_bsp_surface_polygon_replacements.py','tools/oracle_smg_firing_1_1.py','tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/generated_routine_ownership.py','tools/specialize_native_operands.py')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image):
    from validate_native_bsp_segment_query_tu15 import cases
    seeds=cases(image);rows=[];rng=random.Random(0x1e68e0);live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','00000000000000800040','0000000000000080ffbf')
    epsilons=(0,0x80000000,0xbf800000,0x3e800000,0x3f000000,0x3f800000,0x40000000,0x7f800000,0xff800000,0x7fc12345,0x7f812345,1,0x80000001,0x800000,0x7f7fffff,0xbf000000)
    for flags in(0,1,2,3,4,9,16,31):
        for epsilon in epsilons:
            for kind in(0,1):
                source=seeds[flags*8+kind];n=len(rows);stack=bytearray(rng.randbytes(16384));sp=0x62002000+n%4
                arguments=[(rng.getrandbits(16)<<16)|64,0,0x1003000,0x1003100];struct.pack_into('<5I',stack,sp-0x62000000,0x63000000,*arguments)
                gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=0x1003800;gp[1]=flags;gp[2]=0x1000100;gp[4]=sp
                xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)];xmm[0][0]=epsilon
                rows.append(dict(id=n,name=f'flags-{flags}-epsilon-{epsilon:x}-kind-{kind}',entry=0x1e68e0,gp=gp,matrix=arguments[0],source=arguments[1],eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=xmm,before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(n%64)|(0x40 if n&16 else 0)|(0x8000 if n&32 else 0)],live_x87=list(live[:(0,1,4,6)[n%4]]),max_steps=100000,banks=[dict(address=0x1000000,before=source['banks'][0]['before']),dict(address=0x44a000,before=image.read(0x44a000,8192).hex()),dict(address=0x46a000,before=image.read(0x46a000,4096).hex()),dict(address=0x62000000,before=stack.hex())]))
    def extra(seed,name,change):
        row=copy.deepcopy(seed);row.update(id=len(rows),name=name)
        def put(address,value,width=4):
            for bank in row['banks']:
                data=bytearray.fromhex(bank['before']);offset=address-bank['address']
                if 0<=offset<=len(data)-width:
                    data[offset:offset+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little');bank['before']=data.hex()
                    if address==row['gp'][4]+4 and width==4:row['matrix']=value
                    if address==row['gp'][4]+8 and width==4:row['source']=value
                    return
            raise ValueError('Directed BSP setup input escaped bank')
        change(row,put);rows.append(row)
    inside=copy.deepcopy(rows[11])
    def both_leaves(row,put):
        put(0x1000605,0x800001,3);put(0x1000904,1,1);put(0x1000905,1,1);put(0x1000906,0,2)
    for axis in(0,1,2):
        for reverse in(False,True):
            def edit(row,put,axis=axis,reverse=reverse):
                both_leaves(row,put)
                for component in range(3):
                    put(0x1000800+component*4,0x3f800000 if component==axis else 0)
                    put(0x1003000+component*4,(0x3f800000 if reverse else 0xbf800000)if component==axis else 0)
                    put(0x1003100+component*4,(0xc0000000 if reverse else 0x40000000)if component==axis else 0)
            extra(inside,f'verified-polygon-axis-{axis}-reverse-{int(reverse)}',edit)
    for depth in(2,8,32):
        def edit(row,put,depth=depth):
            both_leaves(row,put)
            for index in range(depth):
                base=0x1000600+index*8;put(base,0,2);put(base+2,index+1 if index+1<depth else 0x800000,3);put(base+5,0x800001,3)
        extra(inside,f'verified-recursion-depth-{depth}',edit)
    for epsilon in(0x40000000,0x7f812345,0x80000000):extra(rows[11],f'output-alias-one-{epsilon:x}',lambda row,put,epsilon=epsilon:(row['gp'].__setitem__(0,0x46a0d0),row['xmm'][0].__setitem__(0,epsilon)))
    for offset,epsilon in((-36,0x1000100),(-20,0x1003100),(4,0x3f800000),(8,0x3f800000)):
        extra(rows[11],f'output-stack-{offset}',lambda row,put,offset=offset,epsilon=epsilon:(row['gp'].__setitem__(0,row['gp'][4]+offset),row['xmm'][0].__setitem__(0,epsilon)))
    extra(rows[11],'argument-limit-upper-flags',lambda row,put:(row['gp'].__setitem__(1,0xa55a0001),put(row['gp'][4]+4,0xc55bffff)))
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if sys.platform!='linux'or platform.machine().lower()not in ('x86_64','amd64','i386','i686'):return 77
    if not args.xbe.is_file():return 77
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,candidate_unbound=not args.require_binding,scratch_owner=str(scratch),retained_large_artifacts=[])
    try:
        pins={path:sha(ROOT/path)for path in INPUTS};sites=set();rejections=0;admission_controls=[]
        from native_bsp_leaf_surface_replacements import canonical_body as surface_body
        from native_bsp_surface_2d_replacements import canonical_body as tree_body
        from native_bsp_surface_polygon_replacements import canonical_body as polygon_body
        children=[]
        for entry,path,strip in ((0x1e6980,COPIES[0],segment_body),(0x1e6cb0,COPIES[0],surface_body),(0x2a0230,COPIES[1],tree_body),(0x1e6e80,'src/runtime/source_map_world_support_tu15_generated.cpp',polygon_body)):
            source=(ROOT/path).read_text();body=strip(next(b for m,b in function_bodies(source)if int(m[1],16)==entry));children.append(re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',body.replace(ORIGINAL_SPILL,ORDERED_SPILL)))
        declarations='\n'.join(f'static void run_{entry:x}(WorldState&);'for entry in (0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80))
        dispatch='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x2a0230u)run_2a0230(s);else if(target==0x1e6e80u)run_1e6e80(s);else throw std::runtime_error("Unknown retained segment child");}'
        retained=''
        for index,path in enumerate(COPIES):
            source=(ROOT/path).read_text();current=next(b for m,b in function_bodies(source)if m[1]=='1e68e0');original=canonical_body(current);bound=replace_bsp_segment(source)
            if args.require_binding and bound!=source:raise ValueError('Segment native source binding missing: '+path)
            if replace_bsp_segment(bound)!=bound:raise ValueError('Segment binding not idempotent')
            selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
            from native_bsp_segment_replacements import PREFIX as child_prefix,INCLUDE as child_include
            from native_bsp_segment_setup_replacements import PREFIX as own_prefix
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            child=next(b for m,b in function_bodies(source)if m[1]=='1e6980');changed_child=child.replace('s.gp[0]*8u','s.gp[0]*4u',1)
            if changed_child==child:raise ValueError('Missing recursive child mutation witness')
            mutants=[('dispatcher',source.replace(dispatcher,dispatcher.replace('run_1e6980(s);','run_1e68e0(s);',1),1)),('child_body',source.replace(child,changed_child,1)),('child_prefix',source.replace(child_prefix,'',1)),('child_include',source.replace(child_include,'',1)),('own_prefix',bound.replace(own_prefix,own_prefix.replace('::bridge','::changed_bridge'),1))]
            if 'selected('in original:mutants.append(('resolver',source.replace(selected,selected.replace('return p;','return p+1u;',1),1)))
            for label,mutant in mutants:
                if mutant in(source,bound):raise ValueError('Missing setup admission mutation witness '+label)
                try:replace_bsp_segment(mutant)
                except ValueError:rejections+=1;admission_controls.append(str(index)+':'+label)
                else:raise ValueError('Changed setup/recursive routing admitted '+label)
            for shape in(original,re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)):
                candidate=source.replace(current,shape,1);replace_bsp_segment(candidate)
                for version in(candidate,replace_bsp_segment(candidate)):
                    body=next(b for m,b in function_bodies(version)if m[1]=='1e68e0');changed=body.replace('s.xmm[2]=_mm_xor_ps(s.xmm[2],s.xmm[2])','s.xmm[2]=_mm_xor_ps(s.xmm[0],s.xmm[2])',1)
                    if changed==body:raise ValueError('Missing setup whole-body mutation witness')
                    try:replace_bsp_segment(version.replace(body,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed setup numerical body admitted')

            helper=''
            if 'native_single_spill('in original:
                helper=re.search(r'std::uint32_t native_single_spill\(.*?\n\}',source,re.S)[0]
                if hashlib.sha256(helper.encode()).hexdigest()!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Retained spill helper changed')
            prefix=declarations+'\n'+dispatch+'\n'+'\n'.join(children)+'\n'+helper+'\n'+selected+'\n'
            retained+=f'namespace bound{index} {{\n'+prefix+next(b for m,b in function_bodies(bound)if m[1]=='1e68e0')+'\n}\n'
            from specialize_native_operands import specialize
            prepared,_=specialize(bound);prepared_body=next(b for m,b in function_bodies(prepared)if m[1]=='1e68e0')
            if own_prefix not in prepared_body or 's.instruction_literal('not in prepared_body:raise ValueError('Production setup specialization route changed')
            retained+=f'namespace specialized{index} {{\n'+prefix+prepared_body+'\n}\n'
            retained+=f'namespace specialized_reference{index} {{\n'+prefix+canonical_body(prepared_body)+'\n}\n'
            retained+=f'namespace reference{index} {{\n'+prefix+original.replace(ORIGINAL_SPILL,ORDERED_SPILL)+'\n}\n'
            if index:retained+=f'namespace literal_reference{index} {{\n'+prefix+re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)+'\n}\n'

        (scratch/'segment_setup_bodies.inc').write_text(retained)
        for header in ('native_tu15_bsp_segment_setup.hpp','native_tu15_bsp_segment.hpp','native_tu15_bsp_leaf_surface.hpp','native_tu15_bsp_surface_2d.hpp','native_tu15_bsp_surface_polygon.hpp'):
            for value in re.findall(r'0x([0-9a-f]+)u',(ROOT/'src/physics/runtime'/header).read_text()):
                pc=int(value,16)
                if 0x1e68e0<=pc<=0x1e6fac or 0x2a0230<=pc<=0x2a028d:sites.add(pc)
        from oracle_smg_firing_1_1 import Xbe
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from validate_native_bsp_surface_polygon_abi_tu15 import metadata
        original=common.metadata
        def private_metadata(image):
            result=metadata(image,original);code=common._Code(image,'.text')
            for site in result['sites']:
                instruction=code.at(site['instruction'])
                if instruction.mnemonic=='pop'or instruction.mnemonic.startswith('ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=private_metadata;image=Xbe(args.xbe);rows=corpus(image)
        delta={0x1e6a60,0x1e6a64,0x1e6a6f,0x1e6d0e,0x1e6d15,0x1e6d1d,0x1e6d26,0x1e6d2e}
        observed,proof=physical_original(image,rows,scratch,entries=(0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=4 if pc in(0x1e68e0,0x1e6980)else(5 if pc in(0x1e6cb0,0x1e6e80)else 0),gp=True,xmm=True,x87_delta=int(pc in delta))for pc in sorted(sites)],trace_extended=True)
        cases=[]
        for row,out in zip(rows,observed['cases'],strict=True):
            if not out['completed']or not out['live_x87_unchanged']:raise ValueError('Original recursive parent failed normal/live80 contract')
            case=copy.deepcopy(row)
            for key in ('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes'):case[key]=out[key]
            case['packets']=[{key:packet[key]for key in ('entry','arguments','gp','eflags','xmm','fp','live_x87')}for packet in out['calls']];case['final_pc']=out['calls'][-1]['entry']
            for before,after in zip(case['banks'],out['banks'],strict=True):before['after']=after['after']
            if row['name'].startswith('recursive-saved-context-and-side-'):
                kind=int(row['name'].rsplit('-',1)[1]);returned=next(packet for packet in out['calls']if packet['entry']==0x1e6a8d)
                if returned['gp'][5]!=0x614000 or(returned['gp'][3]&255)!=kind+1:raise ValueError('Recursive alias fixture did not change the genuine returned context/side')
            if row['name']=='hit-output-stack-minus18'and any(out['boundary_gp'][reg]!=value for reg,value in ((3,0),(5,0),(6,0x1000800),(7,0))):raise ValueError('Hit alias fixture did not alter restored DI/SI/BP/BX as declared')
            if row['name'].startswith('verified-polygon-')and not any(packet['entry']==0x1e6e80 for packet in out['calls']):raise ValueError('Verified wrapper fixture did not reach real polygon')
            cases.append(case)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=sorted({e['instruction']for row in observed['cases']for e in row['reads']})),separators=(',',':')))
        binary=scratch/'segment-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_bsp_segment_setup_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compile=subprocess.run(command,capture_output=True,text=True)
        if compile.returncode:raise ValueError('Segment compile failed: '+compile.stderr[-6000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if run.returncode:raise ValueError('Native segment mismatch: '+run.stderr[-6500:])
        control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=dict(os.environ,HALO2_REFERENCE_BSP_SEGMENT_SETUP='1',HALO2_BSP_SEGMENT_SETUP_REFERENCE_TEST='1'))
        if control.returncode:raise ValueError('Forced setup reference failed: '+control.stderr[-5000:])
        reference_control=json.loads(control.stdout)
        if not reference_control.get('native_refused')or not reference_control.get('callbacks')or not reference_control.get('self_entries')or not reference_control.get('surface_entries'):raise ValueError('Forced setup reference did not execute real recursive and surface callees')
        specialized_control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=dict(os.environ,HALO2_REFERENCE_BSP_SEGMENT_SETUP='1',HALO2_BSP_SEGMENT_SETUP_REFERENCE_TEST='1',HALO2_BSP_SEGMENT_SETUP_SPECIALIZED_REFERENCE_TEST='1'))
        if specialized_control.returncode:raise ValueError('Forced specialized setup reference failed: '+specialized_control.stderr[-5000:])
        specialized_reference_control=json.loads(specialized_control.stdout)
        if not specialized_reference_control.get('native_refused')or not specialized_reference_control.get('literal_evaluations')or not specialized_reference_control.get('self_entries')or not specialized_reference_control.get('surface_entries'):raise ValueError('Forced specialized setup did not execute genuine literal continuation and callees')
        if any(sha(ROOT/path)!=pin for path,pin in pins.items()):raise ValueError('Segment proof input changed')
        executed={pc for row in observed['cases']for pc in row['executed_sites']};coverage={}
        from generate_physics_live_support_tu15 import complete_body
        code=common._Code(image,'.text')
        for entry in (0x1e68e0,0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80):
            owner=set(complete_body(code,entry).code);coverage[f'{entry:x}']=dict(total=len(owner),executed=len(owner&executed),missing=[dict(pc=f'{pc:x}',instruction=code.at(pc).mnemonic+' '+code.at(pc).op_str)for pc in sorted(owner-executed)])
        report.update(json.loads(run.stdout),source_sha256=pins,qualification='Original i386 complete normal returns, retained callee/I-O boundary ABI and pure ordinary observer protocol',limitations=['Variable-address page faults, partial stores, and observer exceptions are not qualified by this normal-return cohort','Unmasked floating-point and active MMX callers retain the original route','The wrapper cohort covers all42 wrapper sites; complete child-site qualification is retained in the independent segment/surface/tree/polygon receipts'],binding_rejections=rejections,source_admission_controls=admission_controls,reference_control=reference_control,specialized_reference_control=specialized_reference_control,physical_original=proof,original_sites_executed=len(executed),owner_coverage=coverage,read_protocol='All architectural reads including implicit POP/RET; every ordinary observer preimage read independently required');print(run.stdout.strip())
    except BaseException as error:report.update(passed=False,error=str(error));raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
