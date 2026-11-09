#!/usr/bin/env python3
"""Private actual1E6980 native ABI, real recursion and unchanged surface children."""
import argparse,copy,hashlib,json,os,platform,random,re,struct,subprocess,sys
from pathlib import Path
from validate_native_tu15_prt_helpers import PIN,physical_original
from generated_routine_ownership import function_bodies
from native_bsp_segment_replacements import replace_bsp_segment,canonical_body,ORIGINAL_SPILL,ORDERED_SPILL
ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x1000000,0x5000),(0x44a000,8192),(0x46a000,4096),(0x62000000,16384),(0x610000,0x5000))
COPIES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp')
INPUTS=COPIES+('include/halo2/native_tu15_bsp_segment_kernel.hpp','src/physics/runtime/native_tu15_bsp_segment.hpp','tests/native_tu15_bsp_segment_abi_tests.cpp','tools/validate_native_bsp_segment_abi_tu15.py','tools/native_bsp_segment_replacements.py','include/halo2/native_tu15_bsp_leaf_surface_kernel.hpp','src/physics/runtime/native_tu15_bsp_leaf_surface.hpp','include/halo2/native_tu15_bsp_surface_polygon_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_polygon.hpp','include/halo2/native_tu15_bsp_surface_2d_kernel.hpp','src/physics/runtime/native_tu15_bsp_surface_2d.hpp','src/runtime/source_map_world_support_tu15_generated.cpp','tools/native_bsp_leaf_surface_replacements.py','tools/native_selected_operand_binding.py','tools/native_halfword_bit_query_replacements.py','tools/validate_native_tu15_prt_helpers.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image):
    from validate_native_bsp_segment_query_tu15 import cases
    rows=[];rng=random.Random(0x1e6980);live=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f','0100000000000080ff7f','0000000000000080ff7f')
    for n,source in enumerate(row for row in cases(image)if row['entry']==0x1e6980):
        stack=bytearray(rng.randbytes(16384));sp=0x62002000+n%4;struct.pack_into('<5I',stack,sp-0x62000000,0x63000000,*source['arguments']);gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
        rows.append(dict(id=n,name=source['name'],entry=0x1e6980,gp=gp,matrix=source['arguments'][0],source=source['arguments'][1],eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],before_fp=[(0x7f,0x27f,0x37f)[n%3]|((n%4)<<10),((n%8)<<11)|(n%64)|((n*0x100)&0x4700),0x1f80|((n%4)<<13)|(n%64)],live_x87=list(live[:(0,1,4,6)[n%4]]),max_steps=100000,banks=[dict(address=0x1000000,before=source['banks'][0]['before']),dict(address=0x44a000,before=image.read(0x44a000,8192).hex()),dict(address=0x46a000,before=image.read(0x46a000,4096).hex()),dict(address=0x62000000,before=stack.hex())]))
    def extra(seed,name,edit):
        row=copy.deepcopy(seed);n=len(rows);row.update(id=n,name=name)
        def put(address,value,width=4):
            for bank in row['banks']:
                data=bytearray.fromhex(bank['before']);offset=address-bank['address']
                if 0<=offset<=len(data)-width:
                    data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');bank['before']=data.hex();return
            raise ValueError('Directed recursive fixture address escaped bank')
        edit(row,put);row['live_x87']=list(live[:(0,1,4,6)[n%4]])
        for offset,value in ((0,0x63000000),(4,row['matrix']),(8,row['source'])):put(row['gp'][4]+offset,value)
        rows.append(row)
    for origin,delta in ((0x3f800000,0xc0400000),(1,0xc0000000),(0x00800000,0xc1000000),(0x7effffff,0xff7fffff)):
        for pc in (0x7f,0x27f,0x37f):
            for rc in range(4):
                for flush in (0,0x40,0x8000,0x8040):
                    def ratio(row,put,origin=origin,delta=delta,pc=pc,rc=rc,flush=flush):
                        put(0x1004000,0);put(0x1004020,0,1);put(0x1003000,origin);put(0x1003100,delta)
                        row['before_fp']=[pc|(rc<<10),((len(rows)%8)<<11)|0x4500,0x1f80|(rc<<13)|flush]
                    extra(rows[256],f'crossing-{origin:x}-{delta:x}-pc{pc:x}-rc{rc}-flush{flush:x}',ratio)
    for flags in (2,10,18,26):
        for material_flags in (0,2,8,10):
            def filtering(row,put,flags=flags,material_flags=material_flags):
                put(0x1004000,flags)
                for index in range(2):put(0x1001804+index*8,material_flags,1);put(0x1001800+index*8,0x8000+index,2)
            extra(rows[22],f'surface-filter-{flags}-{material_flags}',filtering)
    for limit in (0,0x3f000000,0x7fc23456,0x7f812345):
        def pruning(row,put,limit=limit):put(0x1004000,0);put(0x1004020,0,1);put(0x1003800,limit)
        extra(rows[256],f'near-output-limit-{limit:x}',pruning)
    for mode in ('axis-z','axis-y','axis-negative','tree-positive','tree-invalid','plane-mismatch','polygon-outside','polygon-inside','bitmap-reject','bitmap-accept','bitmap-signed-limit'):
        def surface_path(row,put,mode=mode):
            for lane,value in enumerate((0,0,0x3f800000)):put(0x1000800+lane*4,value)
            if mode=='axis-y':put(0x1000804,0x3f800000);put(0x1000808,0)
            if mode=='axis-negative':put(0x1000808,0xbf800000)
            if mode in ('axis-z','axis-y','axis-negative','tree-positive','tree-invalid','plane-mismatch'):
                row['source']=0xffffffff;put(0x1004000,1);put(0x1004020,1,1)
            if mode=='tree-positive':put(0x1003000,0x3f800000);put(0x1003004,0x3f800000)
            if mode=='tree-invalid':put(0x100100c,0xffff,2);put(0x100100e,0xffff,2)
            if mode=='plane-mismatch':put(0x1004024,1)
            if mode in ('polygon-inside','bitmap-reject','bitmap-accept','bitmap-signed-limit'):put(0x1003000,0x3e800000);put(0x1003004,0x3e800000)
            if mode.startswith('bitmap-'):
                put(0x100400c,0x1003300);put(0x1003304,0 if mode=='bitmap-reject'else 8)
                if mode=='bitmap-signed-limit':put(0x1004008,0x8000,2)
        seed=rows[22]if mode in ('axis-z','axis-y','axis-negative','tree-positive','tree-invalid','plane-mismatch')else rows[5]
        extra(seed,'surface-path-'+mode,surface_path)
    for depth in (3,8,64):
        def nested(row,put,depth=depth):
            put(0x1004000,4);put(0x1004020,0,1)
            for index in range(depth):
                put(0x1000600+index*8,0,2);put(0x1000602+index*8,index+1 if index+1<depth else 0x800000,3);put(0x1000605+index*8,0x800000,3)
        extra(rows[259],f'nested-near-depth{depth}',nested)
    for index in (0x8000,0xffff,0x7fff):
        def signed_plane(row,put,index=index):
            signed=index if index<0x8000 else index-0x10000;put(0x1000600,index,2);put(0x100010c,(0x1000800-signed*16)&0xffffffff);put(0x1004000,4)
        extra(rows[256],f'signed-plane-{index:x}',signed_plane)
    for distance in (0x18,0x1c,0x20,0x24):
        def output_stack(row,put,distance=distance):
            output=row['gp'][4]-distance;put(0x1004018,output)
            for offset,value in ((0,0x3f800000),(24,0)):put(output+offset,value)
        extra(rows[22],f'hit-output-stack-minus{distance:x}',output_stack)
    for distance in (0x30,0x40,0x50):
        def descriptor_stack(row,put,distance=distance):
            descriptor=row['gp'][4]-distance;put(0x1004004,descriptor);put(0x1004000,4)
            for offset,value in ((4,0x1000600),(12,0x1000800),(20,0x1000900),(28,0x1000a00),(36,0x1001000),(44,0x1001800),(52,0x1002000),(60,0x1002800)):put(descriptor+offset,value)
        extra(rows[0],f'leaf-descriptor-stack-minus{distance:x}',descriptor_stack)
    for offset in (-24,0,4,8):
        for count in (0,0xffffffff):
            def count_alias(row,put,offset=offset,count=count):
                output=0x1004000+offset;put(0x1004000,4);put(0x1004018,output);put(output+24,count)
                if offset==0:put(0x1004018,output) # This field is the count itself.
            extra(rows[0],f'leaf-count-context-offset{offset}-value{count:x}',count_alias)
    for kind in (0,1):
        def changed_continuation(row,put,kind=kind):
            row['banks'].append(dict(address=0x610000,before=bytes(0x5000).hex()));context=row['gp'][4]-84;continued=0x614000;output=0x613800
            row['matrix']=context
            for base in (context,continued):
                for offset,value in ((0,4),(4,0x1000100),(8,64),(12,0),(16,0x1003000),(20,0x1003100),(24,output),(28,0),(32,0),(36,0)):put(base+offset,value)
            put(output,0x3f800000);put(output+24,0)
            put(0x1000114,(0x1000900-continued*4)&0xffffffff);put(0x1000900,kind,1);put(0x1000602,0x800000|continued,3);put(0x1000608,0xffffff,3);put(0x1000618,0)
        extra(rows[256],f'recursive-saved-context-and-side-{kind}',changed_continuation)
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
        for entry,path,strip in ((0x1e6cb0,COPIES[0],surface_body),(0x2a0230,COPIES[1],tree_body),(0x1e6e80,'src/runtime/source_map_world_support_tu15_generated.cpp',polygon_body)):
            source=(ROOT/path).read_text();body=strip(next(b for m,b in function_bodies(source)if int(m[1],16)==entry));children.append(re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',body))
        declarations='\n'.join(f'static void run_{entry:x}(WorldState&);'for entry in (0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80))
        dispatch='static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t,bool){if(target==0x2a0230u)run_2a0230(s);else if(target==0x1e6e80u)run_1e6e80(s);else throw std::runtime_error("Unknown retained segment child");}'
        retained=''
        for index,path in enumerate(COPIES):
            source=(ROOT/path).read_text();current=next(b for m,b in function_bodies(source)if m[1]=='1e6980');original=canonical_body(current);bound=replace_bsp_segment(source)
            if args.require_binding and bound!=source:raise ValueError('Segment native source binding missing: '+path)
            if replace_bsp_segment(bound)!=bound:raise ValueError('Segment binding not idempotent')
            selected=next(line for line in source.splitlines()if line.startswith('std::uint32_t selected('))
            from native_bsp_leaf_surface_replacements import PREFIX as child_prefix,INCLUDE as child_include
            dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            child=next(b for m,b in function_bodies(source)if m[1]=='1e6cb0')
            changed_child=child.replace('s.gp[1]*4u','s.gp[1]*8u',1)
            if changed_child==child:raise ValueError('Child body mutation witness missing')
            mutants=(('dispatcher',source.replace(dispatcher,dispatcher.replace('run_1e6cb0(s);','run_1e6980(s);',1),1)),
                     ('child_body',source.replace(child,changed_child,1)),
                     ('child_prefix',source.replace(child_prefix,'',1)),
                     ('child_include',source.replace(child_include,'',1)))
            if 'selected('in original:mutants+=(('resolver',source.replace(selected,selected.replace('return p;','return p+1u;',1),1)),)
            for name,mutant in mutants:
                if mutant==source:raise ValueError('Missing admission mutation witness: '+name)
                try:replace_bsp_segment(mutant)
                except ValueError:rejections+=1;admission_controls.append(str(index)+':'+name)
                else:raise ValueError('Changed source admission accepted: '+name)
            for shape in (original,re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)):
                candidate=source.replace(current,shape,1);replace_bsp_segment(candidate)
                for version in (candidate,replace_bsp_segment(candidate)):
                    body=next(b for m,b in function_bodies(version)if m[1]=='1e6980');changed=body.replace('s.gp[0]*8u','s.gp[0]*4u',1)
                    if changed==body:raise ValueError('Segment mutation witness missing')
                    try:replace_bsp_segment(version.replace(body,changed,1))
                    except ValueError:rejections+=1
                    else:raise ValueError('Changed whole segment body accepted')
            helper=''
            if 'native_single_spill('in original:
                helper=re.search(r'std::uint32_t native_single_spill\(.*?\n\}',source,re.S)[0]
                if hashlib.sha256(helper.encode()).hexdigest()!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Retained spill helper changed')
            prefix=declarations+'\n'+dispatch+'\n'+'\n'.join(children)+'\n'+helper+'\n'+selected+'\n'
            retained+=f'namespace bound{index} {{\n'+prefix+next(b for m,b in function_bodies(bound)if m[1]=='1e6980')+'\n}\n'
            retained+=f'namespace reference{index} {{\n'+prefix+original.replace(ORIGINAL_SPILL,ORDERED_SPILL)+'\n}\n'
            if index:retained+=f'namespace literal_reference{index} {{\n'+prefix+re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',original)+'\n}\n'
        (scratch/'segment_bodies.inc').write_text(retained)
        for header in ('native_tu15_bsp_segment.hpp','native_tu15_bsp_leaf_surface.hpp','native_tu15_bsp_surface_2d.hpp','native_tu15_bsp_surface_polygon.hpp'):
            for value in re.findall(r'0x([0-9a-f]+)u',(ROOT/'src/physics/runtime'/header).read_text()):
                pc=int(value,16)
                if 0x1e6980<=pc<=0x1e6fac or 0x2a0230<=pc<=0x2a028d:sites.add(pc)
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
        observed,proof=physical_original(image,rows,scratch,entries=(0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80),regions=REGIONS,trace_calls=[dict(entry=pc,arguments=4 if pc==0x1e6980 else(5 if pc in(0x1e6cb0,0x1e6e80)else 0),gp=True,xmm=True,x87_delta=int(pc in delta))for pc in sorted(sites)],trace_extended=True)
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
            cases.append(case)
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=cases,explicit_read_sites=sorted({e['instruction']for row in observed['cases']for e in row['reads']})),separators=(',',':')))
        binary=scratch/'segment-tests';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_bsp_segment_abi_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        compile=subprocess.run(command,capture_output=True,text=True)
        if compile.returncode:raise ValueError('Segment compile failed: '+compile.stderr[-6000:])
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120)
        if run.returncode:raise ValueError('Native segment mismatch: '+run.stderr[-6500:])
        control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=120,env=dict(os.environ,HALO2_REFERENCE_BSP_SEGMENT='1',HALO2_BSP_SEGMENT_REFERENCE_TEST='1'))
        if control.returncode:raise ValueError('Forced segment reference control failed: '+control.stderr[-5000:])
        reference_control=json.loads(control.stdout)
        if not reference_control.get('native_refused')or reference_control.get('callbacks',0)<=0 or not reference_control.get('self_entries')or not reference_control.get('surface_entries'):raise ValueError('Forced segment reference did not execute actual retained recursion/surface owner')
        if any(sha(ROOT/path)!=pin for path,pin in pins.items()):raise ValueError('Segment proof input changed')
        executed={pc for row in observed['cases']for pc in row['executed_sites']};coverage={}
        from generate_physics_live_support_tu15 import complete_body
        code=common._Code(image,'.text')
        for entry in (0x1e6980,0x1e6cb0,0x2a0230,0x1e6e80):
            owner=set(complete_body(code,entry).code);coverage[f'{entry:x}']=dict(total=len(owner),executed=len(owner&executed),missing=[dict(pc=f'{pc:x}',instruction=code.at(pc).mnemonic+' '+code.at(pc).op_str)for pc in sorted(owner-executed)])
        report.update(json.loads(run.stdout),source_sha256=pins,binding_rejections=rejections,source_admission_controls=admission_controls,reference_control=reference_control,physical_original=proof,original_sites_executed=len(executed),owner_coverage=coverage,read_protocol='All architectural reads including implicit POP/RET; every ordinary observer preimage read independently required');print(run.stdout.strip())
    except BaseException as error:report.update(passed=False,error=str(error));raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
