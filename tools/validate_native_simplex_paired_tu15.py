#!/usr/bin/env python3
"""Private complete30D830 paired reducer with genuine Triangle/Closest children."""
import argparse,ast,hashlib,json,os,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_closest_segments_frame_tu15 import ROOT,reference as closest_reference
from validate_native_simplex_direction_tu15 import sha
import native_simplex_triangle_replacements as triangle_binding
from native_simplex_paired_replacements import canonical_body
PINS={'src/physics/runtime/physics_world_runtime_generated.cpp':'d18b28718a1fcfe8fd416bcb137c0bfe1470da3ed0bda40d7217224af2540551','src/physics/runtime/physics_contact_runtime_generated.cpp':'77409385cdbdf6207b45a3c343e1bf1bef548deabe305b9150bc3800da71971f'}
def reference(scratch):
    blocks=[]
    for path,pin in PINS.items():
        body=canonical_body(next(b for sig,b in function_bodies((ROOT/path).read_text())if sig.symbol=='309d40'))
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Complete30D830 owner drift: '+path)
        blocks.append(body)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',(ROOT/next(iter(PINS))).read_text())[0]
    if hashlib.sha256(resolver.encode()).hexdigest()!='20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2':raise ValueError('Complete30D830 selected resolver drift')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',blocks[0]).replace('run_309d40(ObservedWorldState','run_unmodified(WorldState',1)
    for value in set(int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',actual)):actual=actual.replace(f'selected(0x{value:x}u,s.image)',f'0x{mapping[value]:x}u')
    needle='if(!handoff_native_simplex_helpers(s,0x30d3f0u,0x30dad9u))run_309900(s);'
    if actual.count(needle)!=1:raise ValueError('Genuine cached-intersection helper seam drift')
    actual=actual.replace(needle,'child(s,0x30d3f0u);').replace('run_309900(s);','child(s,0x30d3f0u);').replace('run_30eb30(s);','child(s,0x312620u);')
    closest_report,closest_body=closest_reference(scratch)
    triangle=triangle_binding.canonical_body(next(b for sig,b in function_bodies((ROOT/next(iter(PINS))).read_text())if sig.symbol=='309900'))
    if hashlib.sha256(triangle.encode()).hexdigest()!='4e6518483279df6cc9969a6a52b0048995d21dcd3a1eb08e16e408ee1852db9b':raise ValueError('Real triangle child complete body drift')
    triangle=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',triangle)
    ordered=actual.replace('run_unmodified(','run_ordered(',1).replace('s.push(','reference::atomic_push(s,')
    primitive='''struct PairedStore:native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_paired_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>{using Base=native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_paired_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>;using Base::Base;using Base::store;};
static void decrement(WorldState&s,std::uint32_t pc,std::uint32_t address){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_paired_detail::Noop before;PairedStore memory{s,before,native_mopp_ray_caller_detail::ScopeIo<WorldState>{&fp}};auto flags=s.flags;std::uint32_t old;fp.io([&]{old=r(s.memory,address,4);});const auto value=flags.increment(old,4,true);memory.store(pc,address,value,[&]{s.flags=flags;},true);}
'''
    counts={}
    for name,expected,pattern,replacement in[
        ('fld',9,r's\.fp\.push\(ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_reference::closest_load(s,\1);'),
        ('fsub',6,r's\.fp\.st\(0\)=sub\(s\.fp\.st\(0\),ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_reference::closest_subtract(s,\1);'),
        ('fmul',6,r's\.fp\.st\(0\)=mul\(s\.fp\.st\(0\),ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_reference::closest_multiply(s,\1);'),
        ('fcomp',1,r's\.fp\.comparison\(ext\(f\((r\(s.memory,.*?,4\))\)\),1\);',r'closest_reference::closest_compare(s,\1);'),
        ('decrement',6,r'w\(s.memory,s.observe,(0x[0-9a-f]+u),(\(0x0u\+s.gp\[0\]\)),4,s.flags.increment\(r\(s.memory,\2,4\),4,true\)\);',r'decrement(s,\1,\2);'),
    ]:
        ordered,count=re.subn(pattern,replacement,ordered);counts[name]=count
        if count!=expected:raise ValueError('Paired phased reference operation drift: '+name+'='+str(count))
    for old,new,count in[
        ('s.fp.st(0)=std::fabs(s.fp.st(0));','closest_reference::closest_absolute(s);',2),
        ('s.fp.comparison(s.fp.st(1),2);','closest_reference::closest_compare_two(s);',4),
    ]:
        if ordered.count(old)!=count:raise ValueError('Paired exact FP reference drift')
        ordered=ordered.replace(old,new)
    for register in (0,1):
        old=f's.set({register},s.flags.shift(s.reg({register},4,0),0x4u,4),4,0);'
        if ordered.count(old)!=1:raise ValueError('Paired original SHL4 flag context drift')
        ordered=ordered.replace(old,f's.set({register},native_mopp_aabb_abi_detail::shifted_immediate<4>(s,s.reg({register},4,0)),4,0);')
    for old,new in [('_mm_mul_ps(','native_simplex_direction_numeric::multiply('),('_mm_add_ps(','native_simplex_initialize_numeric::add('),('_mm_add_ss(','native_simplex_direction_numeric::add_low('),('_mm_sub_ps(','native_simplex_direction_numeric::subtract('),('_mm_mul_ss(','native_simplex_paired_numeric::multiply_low('),('_mm_sub_ss(','native_simplex_paired_numeric::subtract_low('),('_mm_div_ss(','native_simplex_paired_numeric::divide_low(')]:ordered=ordered.replace(old,new)
    source='static std::function<void(WorldState&,std::uint32_t)>child;\n'+primitive+actual+'\n'+ordered
    (scratch/'simplex_paired_reference.inc').write_text(source)
    return dict(complete_owner_sha256=PINS,resolver_sha256=hashlib.sha256(resolver.encode()).hexdigest(),reference_sha256=hashlib.sha256(source.encode()).hexdigest(),children='Identical qualified native Triangle and Closest algorithms computed on both parent reference/native paths; independent unchanged-i386 gate includes real original children',phased_reference='Complete pinned control/GP/SSE parent with original-ordered native FP primitives, atomic PUSH and commit-after-success DEC; unchanged modeled parent retained separately',phased_operation_counts=counts,closest_reference=closest_report),actual,triangle,closest_body
def hardware(args,scratch,binary,blocks):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong unchanged original TU1.5 image')
    image=Xbe(args.xbe);code=_Code(image,'.text');path=scratch/'paired-inputs.bin'
    with path.open('wb')as out:dump=subprocess.run([str(binary),'--dump-inputs'],stdout=out,stderr=subprocess.PIPE)
    if dump.returncode:raise ValueError('Paired input export failed: '+dump.stderr.decode()[-4000:])
    data=path.read_bytes();position=0
    def take(size):
        nonlocal position
        result=data[position:position+size];position+=size
        if len(result)!=size:raise ValueError('Truncated paired input')
        return result
    def words(n):return struct.unpack('<'+'I'*n,take(n*4))
    count=words(1)[0];rows=[]
    for k in range(count):
        fields=words(13);gp=list(fields[:8]);flags,mxcsr,cw,status,depth=fields[8:];live=[take(10).hex()for _ in range(depth)];xmm=[list(words(4))for _ in range(8)];banks=[take(0x8000)]+[take(0x1000)for _ in range(4)]
        for address,size,index in((0x41c500,16,3),(0x461960,16,4),(0x46a0d0,4,2),(0x46a0e8,4,2),(0x46a25c,4,2)):
            bankbase=(0x61000000,0x41d000,0x46a000,0x41c000,0x461000)[index]
            if banks[index][address-bankbase:address-bankbase+size]!=image.read(address,size):raise ValueError('Paired genuine immutable input drift')
        segment,triangle=struct.unpack_from('<2I',banks[0],gp[4]-0x61000000+4)
        rows.append(dict(id=k,name=f'paired-{k}',entry=0x30d830,gp=gp,eflags=flags,matrix=segment,source=triangle,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=2500,banks=[dict(address=address,before=bank.hex())for address,bank in zip((0x61000000,0x41d000,0x46a000,0x41c000,0x461000),banks)]))
    if position!=len(data):raise ValueError('Trailing paired input')
    tree=ast.parse((ROOT/'tools/validate_native_closest_segments_frame_tu15.py').read_text());fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name=='hardware');node=next(n for n in ast.walk(fn)if isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='delta'for t in n.targets));delta=ast.literal_eval(node.value)
    delta.update({0x30d94f:1,0x30d953:1,0x30d9e4:1,0x30db3b:1,0x30db3f:1,0x30db43:1,0x30db49:2,0x30db4d:2,0x30db67:1,0x30db6b:1,0x30db6f:1,0x30db75:2,0x30db79:2,0x30dba4:1,0x30dba8:1,0x30dbac:1,0x30dbb2:2,0x30dbb6:2,0x30d5a3:1,0x30d5a9:1,0x30d5af:1})
    traces=[];sites=set()
    for block in blocks:
        for line in block.splitlines():
            match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not match:continue
            pc=int(match[1],16);sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta.get(pc,0)))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30d830,0x30d3f0,0x312620),regions=((0x61000000,0x8000),(0x41d000,0x1000),(0x46a000,0x1000),(0x41c000,0x1000),(0x461000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    expected=scratch/'paired-expected.bin';actual=scratch/'paired-actual.bin';segments=[];packets=0;executed=set();children=0;shift_witness=None
    def abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for v in live:out.write(bytes.fromhex(v))
        for v in xmm:out.write(struct.pack('<4I',*v))
    with expected.open('wb')as out:
        for case in observed['cases']:
            k=case['id'];name=rows[k]['name'];live=rows[k]['live_x87']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Unchanged paired original did not return: '+name)
            segments.append((out.tell(),name,'return_abi'));abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for bank in case['banks']:out.write(bytes.fromhex(bank['after']))
            stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if len(raw)==16:
                    instruction=code.at(event['instruction'])
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Paired physical vector opcode drift')
                    for offset in(0,8):stores.append((event['instruction'],event['address']+offset,before[offset:offset+8],raw[offset:offset+8]))
                else:stores.append((event['instruction'],event['address'],before,raw))
            out.write(struct.pack('<I',len(stores)))
            for pc,address,before,raw in stores:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            returns=set(case['executed_sites'])&{0x30d93a,0x30d99c,0x30da28,0x30db16,0x30db96,0x30dbe3,0x30dc05}
            if len(returns)!=1:raise ValueError('Paired return-site ownership differs')
            out.write(struct.pack('<2I',next(iter(returns)),len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1;children+=packet['entry']in(0x30d3f0,0x312620)
                if k==3 and packet['entry']==0x30da09:shift_witness=dict(case=name,packet=packet,declared_input_sha256=hashlib.sha256(bytes.fromhex(rows[k]['banks'][0]['before'])).hexdigest(),old_modeled_prefix_flags='00000646',authority='Unchanged original i386; second-phased-prefix-gcc.json retains modeled646 versus native656 at this same before-I/O boundary')
            executed.update(case['executed_sites'])
    with path.open('rb')as inp,actual.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if run.returncode:raise ValueError('Paired native hardware fixture failed: '+run.stderr.decode()[-5000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(v for v in reversed(segments)if v[0]<=offset);raise ValueError(f'Paired unchanged-i386 mismatch byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    parent_sites={pc for pc in sites if 0x30d830<=pc<=0x30dc05}
    return dict(cases=count,parent_sites=len(parent_sites),executed_parent_sites=len(parent_sites&executed),missing_parent_sites=[f'{pc:x}'for pc in sorted(parent_sites-executed)],full_pre_io_gp_flags_xmm_fp_live80_packets=packets,genuine_child_entries=children,comparison_sha256=hashlib.sha256(b).hexdigest(),original_text_unmodified=True,supplied_child_outcomes=False,physical_vector_page_fault_atomicity=False,original=proof,shift_context_witness=shift_witness)
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=[*PINS,'include/halo2/native_tu15_simplex_paired_kernel.hpp','src/physics/runtime/native_tu15_simplex_paired_frame.hpp','tests/native_tu15_simplex_paired_tests.cpp','tools/validate_native_closest_segments_frame_tu15.py','tools/validate_native_simplex_face_refine_tu15.py','tools/native_simplex_triangle_replacements.py','tools/native_simplex_paired_replacements.py',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report['source_sha256']=pins
    try:
        report['reference'],*blocks=reference(scratch);binary=scratch/'paired-frame';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_paired_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Paired frame compile failed:\n'+built.stderr[-10000:])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,blocks)
        negative=subprocess.run([str(binary),'--unmodified-shift-prefix'],capture_output=True,text=True)
        if not negative.returncode or 'paired-3 pc 30da09/30da09 flags 646/656'not in negative.stderr:raise ValueError('Paired unchanged-source SHL flag counterexample changed:\n'+negative.stderr[-5000:])
        report['unmodified_source_shift_counterexample']=dict(expected_failure=True,diagnostic=negative.stderr[-5000:],authority='First-hardware-gcc.json qualified the genuine original context; hardware.shift_context_witness records the unchanged-i386 packet in this run when --xbe is supplied')
        run=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=run.stderr[-10000:]
        if run.returncode:raise ValueError('Paired complete source/children seam failed:\n'+run.stderr[-10000:])
        report['source']=json.loads(run.stderr.strip().splitlines()[-1]);ordered=subprocess.run([str(binary),'--ordered'],capture_output=True,text=True);report['phased_diagnostic']=ordered.stderr[-10000:]
        if ordered.returncode:raise ValueError('Paired phased/fault protocol differs:\n'+ordered.stderr[-10000:])
        report['phased']=json.loads(ordered.stderr.strip().splitlines()[-1]);report['semantic_mutants']={};kernel=ROOT/'include/halo2/native_tu15_simplex_paired_kernel.hpp';text=kernel.read_text()
        for name,old,new in[
            ('reverse_derived_normal','d.normal=N::subtract(d.positive,d.negative);','d.normal=N::subtract(d.negative,d.positive);'),
            ('omit_second_genuine_closest','memory.closest_child(P::closest_second_child,d);','/* Missing real second closest owner. */'),
            ('replace_live_weight_read','memory.scalar_read(P::fresh_weight_first,d)','0x3f800000u'),
        ]:
            if text.count(old)!=1:raise ValueError('Paired semantic mutation seam drift: '+name)
            overlay=scratch/name;header=overlay/'halo2'/kernel.name;header.parent.mkdir(parents=True,exist_ok=True);changed=text.replace(old,new,1);header.write_text(changed);mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
            if built.returncode:raise ValueError('Paired semantic mutant failed to compile: '+name+'\n'+built.stderr[-5000:])
            probe=subprocess.run([mutated[-1],'--ordered'],capture_output=True,text=True)
            if not probe.returncode:raise ValueError('Paired semantic mutant accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,kernel_sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-3000:])
        report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
    if not report['sources_unchanged']:report.update(passed=False,error='Paired proof inputs changed')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
