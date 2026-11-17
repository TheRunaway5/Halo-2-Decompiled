#!/usr/bin/env python3
"""Private type-query frame against unchanged original i386.

The default corpus is callback-free; the explicit genuine-E0900 mode executes
the actual E0900 body on finite query paths. No child return is supplied and
no production route is changed. Other inherited callbacks remain unqualified.
"""
import argparse,copy,hashlib,json,os,platform,random,re,shlex,subprocess
from pathlib import Path
from validate_native_tu15_prt_helpers import physical_original,PIN
ROOT=Path(__file__).resolve().parents[1]
REGIONS=((0x474000,4096),(0x4ec000,4096),(0x61000000,16384),(0x62000000,4096))
FRAME='src/gameplay/objects/native_tu15_object_type_query_frame.hpp'
TEST='tests/native_tu15_object_type_query_frame_tests.cpp'
INPUTS=(FRAME,TEST,'include/halo2/native_tu15_object_type_query_kernel.hpp',
        'tools/validate_native_object_type_query_frame_tu15.py','tools/validate_native_tu15_prt_helpers.py',
        'tools/oracle_native_campaign_screen_lifecycle_tu15.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp')
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def corpus(image=None,genuine_child=False):
    rng=random.Random(0x10de40);rows=[]
    for k in range(128 if genuine_child else 64):
        banks=[bytearray(rng.randbytes(size))for _,size in REGIONS]
        def put(address,value,width=4):
            for (base,size),bank in zip(REGIONS,banks):
                if base<=address<=base+size-width:
                    bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Type-query input escaped declared banks')
        gp=[rng.getrandbits(32)for _ in range(8)];sp=0x62000800+k%16
        index=(0,1,31,65535)[k//4%4];datum=(rng.getrandbits(16)<<16)|index
        body=0x61000900+k%16;descriptor=0x61001000
        gp[4]=sp;gp[3]=(0x61003000,sp-4,sp-8,descriptor+0x84)[k//16%4]
        gp[7]=(0x61003100,sp-3,sp-7,descriptor+0x84)[k//16%4]
        put(0x4ecbd0,0x61000100);put(0x61000144,(0x61000500-index*12)&0xffffffff)
        put(0x61000508,body)
        kind=(-128,-1,0,12,127)[k%5];put(body+0xaa,kind,1);put(0x474b90+(kind&0xffffffff)*4&0xffffffff,descriptor)
        count=(0,1,2,7)[k%4]
        for n in range(count):
            method=0x61002000+n*0x100;put(descriptor+0x84+n*4,method);put(method+0x4c,0)
        put(descriptor+0x84+count*4,0)
        put(sp,0x63000000);put(sp+4,datum);argument=rng.getrandbits(32);put(sp+8,argument)
        if k>=64:
            # Genuine child E0900 executes its unchanged callback body. These
            # finite query choices cover decline and acceptance without the
            # callback's separate x87 speed/range domain.
            gp[3]=0x61003000;gp[7]=0x61003100
            count=1+k%3
            for n in range(count):
                method=0x61002000+n*0x100;put(descriptor+0x84+n*4,method)
                put(method+0x4c,0 if n==0 and k%2 else 0xe0900)
            put(descriptor+0x84+count*4,0)
            argument=(0,0x6000542,0x6000542,0x70000c9,0x70000c9)[k%5]
            put(sp+8,argument);put(body,0);put(body+0x3dc,2 if k%5==1 else 1,1)
            put(body+0x39d,1 if k%5==4 else 0,1)
            put(0x4f0414,0x61003400);put(0x61003408,0x61003600)
            put(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
        depth=(0,1,4,8)[k%4];values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','0000000000000080ffbf')
        rows.append(dict(id=k,name=f'type-query-{"genuine-unit"if k>=64 else"no-child"}-{k}-list{count}-alias{k//16%4}',entry=0x10de40,gp=gp,
            eflags=0x202|(rng.getrandbits(12)&0xcd5),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
            before_fp=[(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10),0x4100|((k%8)<<11),0x1f80|((k%4)<<13)|(k&0x3f)],
            live_x87=[values[n%4]for n in range(depth)],matrix=datum,source=argument,max_steps=160,
            banks=[dict(address=base,before=bank.hex())for(base,_),bank in zip(REGIONS,banks)]))
    return rows
def main():
    global REGIONS
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append');p.add_argument('--sanitizers',action='store_true');p.add_argument('--prefixes',action='store_true');p.add_argument('--semantic-mutants',action='store_true');p.add_argument('--genuine-unit-callback',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64','i386','i686') or not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    pins={name:sha(ROOT/name)for name in INPUTS}
    headers={path.resolve():sha(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.hpp','.h','.inc')}
    report=dict(passed=False,production_bound=False,genuine_children_qualified=False,scope=__doc__,owned_scratch=str(scratch),retained_large_artifacts=[],source_before=pins)
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU1.5 image')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        from oracle_smg_firing_1_1 import Xbe
        original_metadata=common.metadata
        def metadata(image):
            result=original_metadata(image);code=common._Code(image,'.text')
            for site in result['sites']:
                if code.at(site['instruction']).mnemonic in('pop','ret'):
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result
        common.metadata=metadata;common.ENTRIES=(0x10de40,);image=Xbe(a.xbe);meta=metadata(image)
        sites={site['instruction']for site in meta['sites']}
        if len(sites)!=43:raise ValueError('Original whole type-query CFG changed')
        traces=[dict(entry=site['instruction'],arguments=0,gp=True,xmm=True)for site in meta['sites']if site['operands']or common._Code(image,'.text').at(site['instruction']).mnemonic in('push','call')]
        entries=(0x10de40,);regions=REGIONS;child_header=None
        if a.genuine_unit_callback:
            REGIONS=regions=tuple(sorted((*REGIONS,(0x46a000,4096),(0x4f0000,4096))))
            entries+=(0xe0900,);traces.append(dict(entry=0xe0900,arguments=0,gp=True,xmm=True))
            source='src/physics/physics_live_support_tu15_generated.cpp';symbol='src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
            pins.update({name:sha(ROOT/name)for name in(source,symbol,'src/rendering/effects/source_object_function_symbols_tu15.inc')})
            retained=(ROOT/source).read_text()
            if 'native_flight_property_query_owner_detail'in retained:
                from native_flight_property_query_replacements import remove_flight_property_query
                from native_object_record_ratio_replacements import remove_object_record_ratio
                retained=remove_flight_property_query(remove_object_record_ratio(retained))
                name='tools/native_flight_property_query_replacements.py'
                pins[name]=sha(ROOT/name)
                name='tools/native_object_record_ratio_replacements.py'
                pins[name]=sha(ROOT/name)
            matches=re.findall(r'^static void run_e0900\(WorldState &s\) \{.*?^\}',retained,re.M|re.S)
            if len(matches)!=1 or hashlib.sha256(matches[0].encode()).hexdigest()!='8439613527b310d55ffdb2a7ce3e6f7abc0379f0ff8a0ed945343a23662f44c7':raise ValueError('Actual complete E0900 callback source changed')
            child_header=scratch/'genuine-unit-callback.inc'
            child_header.write_text('namespace genuine_unit {\nusing namespace halo2::physics_world_runtime_detail;\n'+matches[0]+'\n}\n')
            report.update(genuine_callback_source=source,genuine_callback_body_sha256=hashlib.sha256(matches[0].encode()).hexdigest(),child_fault_prefixes_qualified=False,child_x87_paths_qualified=False,hostile_child_memory_providers_qualified=False)
            report['scope']='Private whole parent frame with genuine E0900 finite query paths; seven other inherited callback owners and complete child domains remain unqualified'
        rows=corpus(image,a.genuine_unit_callback);observed,proof=physical_original(image,rows,scratch,entries=entries,regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
        normal=len(rows);report['authority']=proof
        if a.prefixes:
            stopped_rows=[]
            for k,row in enumerate(rows[:8]):
                for phase,key in(('before_read','reads'),('before_write','writes')):
                    for ordinal in range(len(observed['cases'][k][key])):
                        candidate=copy.deepcopy(row);candidate.update(id=len(stopped_rows),name=row['name']+f'-{phase}-{ordinal}',fault=dict(phase=phase,ordinal=ordinal));stopped_rows.append(candidate)
            prefix=scratch/'prefixes';prefix.mkdir()
            stopped,prefix_proof=physical_original(image,stopped_rows,prefix,entries=entries,regions=regions,trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
            rows+=stopped_rows;observed['cases']+=stopped['cases'];report['prefix_authority']=prefix_proof
        covered=set();packets=0
        for row,actual in zip(rows,observed['cases'],strict=True):
            if actual['completed']!=('fault'not in row):raise ValueError('Original completion differs '+row['name'])
            covered.update(actual['executed_sites']);calls=actual['calls']
            for packet in calls:
                if packet.pop('arguments')!=[]:raise ValueError('Undeclared callback arguments')
            pc=actual['boundary']['instruction']if 'fault'in row else actual['executed_sites'][-1]
            row.update(expected=dict(entry=pc,gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],xmm=actual['boundary_xmm'],fp=actual['after_fp'],live_x87=actual.get('boundary_live_x87',row['live_x87'])),reads=actual['reads'],writes=actual['writes'],calls=calls)
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
            packets+=len(calls)
        not_exercised={0x10de8d,0x10de91,0x10de92,0x10de93,0x10de94,0x10de95,0x10de9e,0x10de9f,0x10dea0}
        if a.genuine_unit_callback:not_exercised=set()
        if sites-covered!=not_exercised:raise ValueError('Declared parent coverage differs '+str(sorted(sites-covered)))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')));results=[]
        for compiler in a.compiler or('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if child_header:command+=['-DHALO2_TYPE_QUERY_CHILD_FIXTURE="'+str(child_header)+'"']
            if a.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Private type-query compile failed '+built.stderr[:3500]+built.stderr[-3500:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if tested.returncode:raise ValueError('Type-query comparison failed '+(tested.stdout+tested.stderr)[-10000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT/FRAME).read_text()
                for label,old,new in(
                    ('unsigned-type','std::int8_t(read(','std::uint8_t(read('),
                    ('skip-second-list-entry','plus(s_.gp[6],4u,4)','plus(s_.gp[6],8u,4)'),
                    ('wide-byte-output','const std::array data{std::byte(value)};','const std::array data{std::byte(value),std::byte{},std::byte{},std::byte{}};'),
                    ('wrong-cleanup','g[4]+=8u;return g[0]','g[4]+=4u;return g[0]')):
                    if original.count(old)!=1:raise ValueError('Missing exact defect witness '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label);target=overlay/FRAME;target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1));mutant=overlay/'probe'
                    changed=list(command);changed[1:1]=['-I'+str(overlay/'src')];changed[changed.index('-o')+1]=str(mutant)
                    built=subprocess.run(changed,capture_output=True,text=True)
                    if built.returncode:raise ValueError('Defect compile failed '+label+': '+built.stderr[-5000:])
                    tried=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True);diagnostic=(tried.stdout+tried.stderr)[-5000:]
                    if tried.returncode==0 or not any(value in diagnostic for value in('differs','Unbacked object-type-query','Object-type-query bank extent')):raise ValueError('Defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,exit_code=tried.returncode,diagnostic=diagnostic))
            results.append(dict(compiler=compiler,command=command,result=json.loads(tested.stdout),compiled_mutations=mutations))
        report.update(passed=True,normal_returns=normal,stopped_prefixes=len(rows)-normal,original_sites=len(sites),covered_sites=len(covered&sites),genuine_unit_callback_exercised=a.genuine_unit_callback,covered_child_sites=sorted(covered-sites),uncovered_callback_sites=sorted(not_exercised),full_original_pre_io_packets=packets,fixture_sha256=sha(fixture),compilers=results)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:sha(ROOT/name)for name in pins}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
