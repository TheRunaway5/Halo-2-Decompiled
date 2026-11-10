#!/usr/bin/env python3
"""Private complete312620 phased DATA/raw ABI, initially canonical comparison."""
import argparse,hashlib,json,os,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_face_refine_tu15 import ROOT,reference as refinement_reference
from validate_native_simplex_direction_tu15 import sha
PINS={
    'src/physics/runtime/physics_world_runtime_generated.cpp':'9490b95842d933fdfbcd672f0213f7ab559a34b06e6e859690ba8a35d235f8fb',
    'src/physics/runtime/physics_contact_runtime_generated.cpp':'31497d7846de3376c18650a94a954597ec6752e28653351f874f97457a27f15e',
}
def reference(scratch):
    bodies=[]
    for path,pin in PINS.items():
        body=next(body for sig,body in function_bodies((ROOT/path).read_text())if sig.symbol=='30eb30')
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Complete312620 source body drift: '+path)
        bodies.append(body)
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',bodies[0]).replace('run_30eb30(ObservedWorldState','run_unmodified(WorldState',1)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',(ROOT/next(iter(PINS))).read_text())[0]
    if hashlib.sha256(resolver.encode()).hexdigest()!='20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2':raise ValueError('Complete312620 selected resolver drift')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    operands={int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',actual)}
    for value in operands:actual=actual.replace(f'selected(0x{value:x}u,s.image)',f'0x{mapping[value]:x}u')
    refinement_reference(scratch)
    primitive='''static void closest_load(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};fp.require_capacity(1u);native_simplex_projection_numeric::load(bits);}
static void closest_multiply(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_numeric::multiply(bits);}
static void closest_add(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_closest_segments_numeric::add(bits);}
static void closest_subtract(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_closest_segments_numeric::subtract(bits);}
static void closest_divide(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::divide(bits);}
static void closest_compare(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::compare_memory(bits,true);}
static void closest_absolute(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_closest_segments_numeric::absolute();}
static void closest_subtract_pop(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_closest_segments_numeric::subtract_pop();}
static void closest_compare_two(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_numeric::compare_pop_two();}
static void closest_multiply_next(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::multiply_next();}
static void closest_discard(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::discard();}
struct ClosestStore:native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_closest_segments_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>{using Base=native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_closest_segments_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>;using Base::Base;using Base::store;};
static void closest_fst(WorldState&s,std::uint32_t pc,std::uint32_t address,bool pop){native_raw_x87_detail::Scope<WorldState>fp{s};native_closest_segments_detail::Noop before;ClosestStore memory{s,before,native_mopp_ray_caller_detail::ScopeIo<WorldState>{&fp}};const auto pending=fp.stage_single(pop);memory.store(pc,address,pending.bits,[&]{fp.commit(pending);});}
'''
    ordered=actual.replace('run_unmodified(','run_ordered(',1).replace('s.push(','reference::atomic_push(s,');counts={}
    rules=[('load',19,r's\.fp\.push\(ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_load(s,\1);')]
    for name,operation,count in [('multiply','mul',8),('add','add',2),('subtract','sub',2),('divide','divide',3)]:
        rules.append((name,count,r's\.fp\.st\(0\)='+operation+r'\(s\.fp\.st\(0\),ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_'+name+r'(s,\1);'))
    rules.append(('compare',6,r's\.fp\.comparison\(ext\(f\((r\(s.memory,.*?,4\))\)\),1\);',r'closest_compare(s,\1);'))
    for name,count,pattern,replacement in rules:
        ordered,n=re.subn(pattern,replacement,ordered);counts[name]=n
        if n!=count:raise ValueError('Closest native reference primitive drift: '+name)
    literals={
        's.fp.st(0)=std::fabs(s.fp.st(0));':'closest_absolute(s);',
        's.fp.st(1)=sub(s.fp.st(1),s.fp.st(0));s.fp.pop();':'closest_subtract_pop(s);',
        's.fp.comparison(s.fp.st(1),2);':'closest_compare_two(s);',
        's.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));':'closest_multiply_next(s);',
        's.fp.st(0)=s.fp.st(0);s.fp.pop();':'closest_discard(s);',
    }
    for old,new in literals.items():
        if ordered.count(old)!=1:raise ValueError('Closest native reference exact operation drift: '+old)
        ordered=ordered.replace(old,new)
    def fst(match):return f'closest_fst(s,{match[1]},{match[2]},{"true"if match[3]else"false"});'
    ordered,n=re.subn(r'w\(s.memory,s.observe,(0x[0-9a-f]+u),(\([^\n]*?\)),4,u\(spill\(s.fp.st\(0\)\)\)\);(s.fp.pop\(\);)?',fst,ordered)
    if n!=8 or ordered.count('reference::atomic_push(')!=3:raise ValueError('Closest staged store/atomic PUSH primitive drift')
    source=primitive+actual+'\n'+ordered;(scratch/'closest_segments_reference.inc').write_text(source)
    return dict(complete_source_body_sha256=PINS,reference_sha256=hashlib.sha256(source.encode()).hexdigest(),unmodified_reference_sha256=hashlib.sha256(actual.encode()).hexdigest(),normal_reference='Pinned complete GP/SSE/control owner with explicit native x87 primitives and staged FST(P) after unchanged-i386 C1 authority; unchanged modeled owner retained as a negative control',native_primitive_counts=counts,staged_fst_or_fstp=8,atomic_pushes=3,primitive_sha256=hashlib.sha256(primitive.encode()).hexdigest()),actual
def hardware(args,scratch,binary,body):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong unchanged original TU1.5 image')
    image=Xbe(args.xbe);code=_Code(image,'.text');path=scratch/'closest-inputs.bin'
    with path.open('wb')as out:dump=subprocess.run([str(binary),'--dump-inputs'],stdout=out,stderr=subprocess.PIPE)
    if dump.returncode:raise ValueError('Closest declared-input export failed: '+dump.stderr.decode()[-3000:])
    data=path.read_bytes();position=0
    def take(size):
        nonlocal position
        result=data[position:position+size];position+=size
        if len(result)!=size:raise ValueError('Truncated closest declared input')
        return result
    def words(n):return struct.unpack('<'+'I'*n,take(n*4))
    count=words(1)[0];rows=[]
    for k in range(count):
        fields=words(13);gp=list(fields[:8]);flags,mxcsr,cw,status,depth=fields[8:];live=[take(10).hex()for _ in range(depth)];xmm=[list(words(4))for _ in range(8)];bank,table,constant=take(0x8000),take(0x1000),take(0x1000)
        for at,word in((0x46a0d0,0x3f800000),(0x46a0e8,0),(0x46a25c,0x35800000)):
            if struct.unpack_from('<I',constant,at-0x46a000)[0]!=word or image.read(at,4)!=struct.pack('<I',word):raise ValueError('Closest immutable scalar input drift')
        first_point,first_direction=struct.unpack_from('<2I',bank,gp[4]-0x61000000+4)
        rows.append(dict(id=k,name=f'closest-{k}',entry=0x312620,gp=gp,eflags=flags,matrix=first_point,source=first_direction,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=700,banks=[dict(address=0x61000000,before=bank.hex()),dict(address=0x41d000,before=table.hex()),dict(address=0x46a000,before=constant.hex())]))
    if position!=len(data):raise ValueError('Trailing closest declared input')
    delta={0x3126fa:1,0x3126fe:1,0x312702:1,0x31270a:1,0x31270e:1,0x312716:1,0x31271c:1,0x312724:1,0x312728:1,0x31272c:2,0x312732:1,0x312736:1,0x31273a:1,0x31273e:2,0x31274f:1,0x31276d:1,0x312771:1,0x312777:1,0x312788:1,0x312799:1,0x31279f:2,0x3127a3:2,0x3127a7:2,0x3127b2:1,0x3127c1:1,0x3127c5:2,0x3127d2:1,0x3127e5:1,0x3127e9:1,0x3127ed:1,0x3127f1:1,0x31280d:1,0x312827:1,0x31282d:1,0x312831:2,0x312835:2,0x312839:1,0x31283c:1}
    traces=[];all_sites=set()
    for line in body.splitlines():
        match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta.get(pc,0)))
    observed,proof=physical_original(image,rows,scratch,entries=(0x312620,),regions=((0x61000000,0x8000),(0x41d000,0x1000),(0x46a000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    expected=scratch/'closest-expected.bin';actual=scratch/'closest-actual.bin';segments=[];packets=0;executed=set();counterexample=None
    def abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for v in live:out.write(bytes.fromhex(v))
        for v in xmm:out.write(struct.pack('<4I',*v))
    with expected.open('wb')as out:
        for case in observed['cases']:
            k=case['id'];name=rows[k]['name'];live=rows[k]['live_x87']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Unchanged closest original did not return: '+name)
            segments.append((out.tell(),name,'return_abi'));abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for bank in case['banks']:out.write(bytes.fromhex(bank['after']))
            stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if len(raw)==16:
                    instruction=code.at(event['instruction'])
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Closest physical vector opcode drift')
                    for offset in(0,8):stores.append((event['instruction'],event['address']+offset,before[offset:offset+8],raw[offset:offset+8]))
                else:stores.append((event['instruction'],event['address'],before,raw))
            out.write(struct.pack('<I',len(stores)))
            for pc,address,before,raw in stores:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            out.write(struct.pack('<2I',0x3128b1,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(case['executed_sites'])
            if k==32:counterexample=dict(name=name,gp=rows[k]['gp'],eflags=rows[k]['eflags'],before_fp=rows[k]['before_fp'],live80=live,original_after_fp=case['after_fp'],declared_input_sha256=hashlib.sha256(bytes.fromhex(rows[k]['banks'][0]['before'])).hexdigest(),consumed_scalar_constants={'46a0d0':'3f800000','46a0e8':'00000000','46a25c':'35800000'})
    with path.open('rb')as inp,actual.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if run.returncode:raise ValueError('Closest native physical fixture failed: '+run.stderr.decode()[-4000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(v for v in reversed(segments)if v[0]<=offset);raise ValueError(f'Closest unchanged-i386 mismatch at byte{offset}, segment{segment}, relative{offset-segment[0]}, expected{a[offset:offset+24].hex()}, actual{b[offset:offset+24].hex()}')
    return dict(cases=count,source_sites=len(all_sites),executed_sites=len(executed),missing_sites=[f'{pc:x}'for pc in sorted(all_sites-executed)],full_pre_io_gp_flags_xmm_fp_live80_packets=packets,comparison_sha256=hashlib.sha256(b).hexdigest(),complete_normal_abi_banks_ordered_stores=True,original_text_unmodified=True,original=proof,unmodified_source_c1_counterexample=counterexample,physical_vector_page_fault_atomicity=False)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,hardware_qualified=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'include/halo2/native_tu15_closest_segments_kernel.hpp','src/physics/runtime/native_tu15_closest_segments_frame.hpp','tests/native_tu15_closest_segments_frame_tests.cpp','tools/validate_native_simplex_face_refine_tu15.py','tools/native_simplex_face_refine_replacements.py','tools/validate_native_tu15_prt_helpers.py',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)};report['source_sha256']=pins
    try:
        report['reference'],body=reference(scratch);binary=scratch/'closest-frame';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_closest_segments_frame_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Closest-segment frame compile failed:\n'+built.stderr[-7000:])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,body)
        negative=subprocess.run([str(binary),'--unmodified-source'],capture_output=True,text=True)
        if not negative.returncode or 'closest-32 pc 3128b1/3128b1'not in negative.stderr or 'sw 120/320'not in negative.stderr:raise ValueError('Unchanged-source C1 counterexample was lost or changed:\n'+negative.stderr[-5000:])
        report['unmodified_source_counterexample']=dict(expected_failure=True,diagnostic=negative.stderr[-5000:],original_authority='second-hardware-gcc-r18.json retains unchanged original SW320 versus modeled SW120 at identical closest-32 inputs; this run repeats physical authority when --xbe is supplied')
        run=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=run.stderr[-7000:]
        if run.returncode:raise ValueError('Closest-segment canonical gate failed:\n'+run.stderr[-7000:])
        report['canonical']=json.loads(run.stderr.strip().splitlines()[-1]);report['semantic_mutants']={}
        kernel=(ROOT/'include/halo2/native_tu15_closest_segments_kernel.hpp').read_text()
        for name,old,new in[
            ('omit_numerator_difference','C::subtract_pop();','F::discard();'),
            ('omit_fresh_second_point','d.fresh_second_point=memory.vector_read(P::fresh_second_point,d,d.second_point_address);','d.fresh_second_point=d.second_point;'),
            ('substitute_output_for_late_argument','d.output=memory.argument(P::output_argument,d);','d.output=d.first_point_address;'),
        ]:
            if kernel.count(old)!=1:raise ValueError('Closest semantic mutation seam drift: '+name)
            overlay=scratch/name;header=overlay/'halo2/native_tu15_closest_segments_kernel.hpp';header.parent.mkdir(parents=True,exist_ok=True);changed=kernel.replace(old,new,1);header.write_text(changed)
            mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
            if built.returncode:raise ValueError('Closest semantic mutant did not compile: '+name+'\n'+built.stderr[-5000:])
            probe=subprocess.run([mutated[-1]],capture_output=True,text=True)
            if not probe.returncode:raise ValueError('Closest semantic mutant was accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,kernel_sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-3000:])
        report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
        if not report['sources_unchanged']:raise ValueError('Closest-segment proof input drift')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
