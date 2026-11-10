#!/usr/bin/env python3
"""Private complete30D5D0 typed selection and genuine30D1A0 child transport."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from native_simplex_face_select_replacements import canonical_body
from validate_native_simplex_face_refine_tu15 import ROOT,reference as refine_reference
from validate_native_simplex_direction_tu15 import sha
PINS={'src/physics/runtime/physics_world_runtime_generated.cpp':'14092a34987ac851b948c1a49773cf6e32e9a2b93b27d47e7be504f6d3944e12','src/physics/runtime/physics_contact_runtime_generated.cpp':'87cb283fd90aca633c7c28f738a0115f2dc9a57d5ce9e5b5fcce277a085f84c5'}
def reference(scratch):
    blocks=[]
    for path,pin in PINS.items():
        b=canonical_body(next(b for sig,b in function_bodies((ROOT/path).read_text())if sig.symbol=='309ae0'))
        if hashlib.sha256(b.encode()).hexdigest()!=pin:raise ValueError('Complete30D5D0 canonical owner drift: '+path)
        blocks.append(b)
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',blocks[0]).replace('run_309ae0(ObservedWorldState','select_unmodified(WorldState',1)
    child_receipt,_=refine_reference(scratch)
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',(ROOT/next(iter(PINS))).read_text())[0]
    if hashlib.sha256(resolver.encode()).hexdigest()!='20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2':raise ValueError('Complete30D5D0 resolver drift')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)};operands={int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',actual)}
    selected='static std::uint32_t selected(std::uint32_t pc,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Wrong face-select reference image");switch(pc){'+''.join(f'case 0x{p:x}u:return 0x{mapping[p]:x}u;'for p in sorted(operands))+'default:throw std::runtime_error("Wrong face-select literal");}}\n'
    prefix='''static unsigned select_child_entries{};static std::vector<SourcePhysicsWorldRuntimeAbi>select_child_packets;static bool select_ordered_children{};
static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){if(target!=0x30d1a0u||site!=0x309d2fu||tail)throw std::runtime_error("Wrong genuine face-select child");++select_child_entries;select_child_packets.push_back(s.save());if(select_ordered_children)child_ordered(s);else child_unmodified(s);}
static void select_load(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};fp.require_capacity(1u);native_simplex_projection_numeric::load(bits);}
static void select_compare_memory(WorldState&s,std::uint32_t bits,bool pop){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::compare_memory(bits,pop);}
static void select_divide(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::divide(bits);}
static void select_compare_next(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::compare_next_pop();}
static void select_fucompp(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::compare_unordered_pop_two();}
static void select_compp(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_numeric::compare_pop_two();}
static void select_multiply_next(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::multiply_next();}
static void select_discard(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_numeric::discard();}
struct SelectStore:native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_face_select_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>{using Base=native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_face_select_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>;using Base::Base;using Base::store;};
static void select_fstp(WorldState&s,std::uint32_t pc,std::uint32_t address){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_face_select_detail::Noop before;SelectStore memory{s,before,native_mopp_ray_caller_detail::ScopeIo<WorldState>{&fp}};const auto pending=fp.stage_single(true);memory.store(pc,address,pending.bits,[&]{fp.commit(pending);});}
'''
    ordered=actual.replace('select_unmodified(','select_ordered(',1).replace('s.push(','atomic_push(s,');ordered=ordered.replace('{\n','{\nselect_ordered_children=true;\n',1)
    ordered,n=re.subn(r's\.fp\.push\(ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'select_load(s,\1);',ordered)
    if n!=12:raise ValueError('Face-select FLD reference primitive drift: '+str(n))
    ordered,n=re.subn(r's\.fp\.st\(0\)=divide\(s\.fp\.st\(0\),ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'select_divide(s,\1);',ordered)
    if n!=1:raise ValueError('Face-select FDIV primitive drift')
    ordered,n=re.subn(r's\.fp\.comparison\(ext\(f\((r\(s.memory,.*?,4\))\)\),([01])\);',r'select_compare_memory(s,\1,\2u);',ordered)
    if n!=2:raise ValueError('Face-select FCOMP reference primitive drift')
    for pc,helper in [(0x30d752,'select_fucompp'),(0x30d802,'select_compp')]:
        needle=f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}} s.require_x87();s.fp.comparison(s.fp.st(1),2);'
        if ordered.count(needle)!=1:raise ValueError('Face-select double-pop compare drift')
        ordered=ordered.replace(needle,f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}} s.require_x87();{helper}(s);')
    ordered,n=re.subn(r's\.fp\.comparison\(s.fp.st\(1\),1\);','select_compare_next(s);',ordered)
    if n!=2:raise ValueError('Face-select rank compare primitive drift')
    if ordered.count('s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));')!=1:raise ValueError('Face-select rank multiply drift')
    ordered=ordered.replace('s.fp.st(0)=mul(s.fp.st(0),s.fp.st(1));','select_multiply_next(s);')
    ordered,n=re.subn(r'w\(s.memory,s.observe,(0x[0-9a-f]+u),(.*?),4,u\(spill\(s.fp.st\(0\)\)\)\);s.fp.pop\(\);',r'select_fstp(s,\1,\2);',ordered)
    if n!=3:raise ValueError('Face-select staged FSTP primitive drift: '+str(n))
    if ordered.count('s.fp.st(0)=s.fp.st(0);s.fp.pop();')!=3:raise ValueError('Face-select rank discard primitive drift')
    ordered=ordered.replace('s.fp.st(0)=s.fp.st(0);s.fp.pop();','select_discard(s);')
    text=prefix+selected+actual+'\n'+ordered+'\n';(scratch/'simplex_face_select_reference.inc').write_text(text)
    return dict(complete_body_sha256=PINS,reference_sha256=hashlib.sha256(text.encode()).hexdigest(),child=child_receipt,normal_reference='Unmodified complete parent and child bodies',prefix_reference='Explicit native x87 primitives/staged FSTP and CPU-atomic PUSH; actual child algorithm retained'),actual

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from validate_native_simplex_face_refine_tu15 import PINS as CHILD_PINS
    from native_simplex_face_refine_replacements import canonical_body
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30d5d0);traces=[];sites=set()
    child=canonical_body(next(b for sig,b in function_bodies((ROOT/next(iter(CHILD_PINS))).read_text())if sig.symbol=='30d1a0'))
    if hashlib.sha256(child.encode()).hexdigest()!=next(iter(CHILD_PINS.values())):raise ValueError('Complete genuine refinement child drift')
    delta={0x30d74a:1,0x30d74e:1,0x30d75f:1,0x30d763:1,0x30d785:1,0x30d792:1,0x30d796:1,0x30d7a4:1,0x30d7af:1,0x30d7bb:1,0x30d7bf:1,0x30d7d9:1,0x30d7dd:2,0x30d7f6:1,0x30d7fe:2,0x30d1d1:1,0x30d1e5:1,0x30d2b3:1,0x30d2b7:1,0x30d2bb:2,0x30d2ef:1,0x30d307:1,0x30d322:1}
    for line in(block+'\n'+child).splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta.get(pc,0)))
    table=image.read(0x41d000,0x1000);constant=image.read(0x46a000,0x1000);mask=image.read(0x461000,0x1000)
    if constant[0x190:0x194]!=bytes.fromhex('cdcc8c3f')or constant[0x454:0x458]!=bytes.fromhex('be378635')or mask[0x950:0x960]!=struct.pack('<4I',*[0x7fffffff]*4):raise ValueError('Original face-select immutable constants drift')
    base,size,stack=0x61000000,0x8000,0x61007000;rows=[];requests=[];finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    for k in range(256):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=base+0x2000;gp[3]=base+0x1000;gp[4]=stack+k%16;frame=((gp[4]-4)&~15)-0x60
        if k%16==6:gp[0]=frame+0x10
        if k%16==10:gp[3]=frame+0x30
        for p in(gp[0],gp[3]):
            for row in range(8):
                for lane in range(4):put(p+row*16+lane*4,finite((rng.randrange(63)-31)/8)if k<192 else edges[(k+row*3+lane*5)%len(edges)])
        if k<48:
            tetra=[0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0]
            for n in range(16):put(gp[0]+n*4,finite(0 if k%8==5 else tetra[n]))
            for lane in range(4):put(gp[3]+lane*4,finite(0 if lane==3 else 2 if k%8==2 else .25))
        scale=finite(0 if k%7==0 else-1 if k%7==1 else 1)if k<192 else edges[k%len(edges)]
        put(gp[4],0x63000000);put(gp[4]+4,scale);put(gp[4]+8,0x23456789)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,3,5)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)];xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'face-select-{k}',entry=0x30d5d0,gp=gp,eflags=flags,matrix=scale,source=0x23456789,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=1200,banks=[dict(address=base,before=bank.hex()),dict(address=0x41d000,before=table.hex()),dict(address=0x46a000,before=constant.hex()),dict(address=0x461000,before=mask.hex())]));requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30d5d0,0x30d1a0),regions=((base,size),(0x41d000,0x1000),(0x46a000,0x1000),(0x461000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/n for n in('face-select.in','face-select.expected','face-select.actual'));segments=[];packets=0;executed=set();choices=set();children=0;vector_ops={}
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for v in live:out.write(bytes.fromhex(v))
        for v in xmm:out.write(struct.pack('<4I',*v))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original face-select did not return: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for v in live:inp.write(bytes.fromhex(v))
            for v in xmm:inp.write(struct.pack('<4I',*v))
            inp.write(bank);inp.write(table);inp.write(constant);inp.write(mask);segments.append((out.tell(),name,'normal_abi'));emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for b in case['banks']:out.write(bytes.fromhex(b['after']))
            events=[]
            for e in case['writes']:
                before=bytes.fromhex(e['before']);data=bytes.fromhex(e['written']);pc,address=e['instruction'],e['address']
                if len(data)==16:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Original face-select vector opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+n,before[n:n+8],data[n:n+8])for n in(0,8))
                else:events.append((pc,address,before,data))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,data in events:out.write(struct.pack('<III',pc,address,len(data)));out.write(before);out.write(data)
            child_count=sum(p['entry']==0x30d1a0 for p in case['calls']);children+=child_count;choice=case['boundary_gp'][0];choices.add(choice);ret=0x30d82b if child_count else 0x30d7f5 if choice==0xffffffff else 0x30d813;out.write(struct.pack('<2I',ret,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:result=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise ValueError('Native physical face-select stopped: '+result.stderr.decode()[-5000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset);raise ValueError(f'Unchanged-i386 face-select differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    parent_sites={p for p in sites if 0x30d5d0<=p<=0x30d82b};missing=parent_sites-executed
    if missing:raise ValueError('Physical parent face-select coverage incomplete: '+','.join(f'{p:x}'for p in sorted(missing)))
    return dict(cases=len(rows),parent_sites=len(parent_sites),executed_parent_sites=len(parent_sites&executed),executed_child_sites=len(executed-parent_sites),actual_refine_entries=children,full_pre_io_gp_flags_xmm_fp_live80_packets=packets,comparison_sha256=hashlib.sha256(b).hexdigest(),returned_choices=[f'{n:08x}'for n in sorted(choices)],complete_normal_abi_banks_ordered_stores=True,original_text_unmodified=True,supplied_child_outcomes=False,declared_inputs=True,original=proof,physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_page_fault_atomicity=False)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    if args.xbe is not None and not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_face_select_kernel.hpp','src/physics/runtime/native_tu15_simplex_face_select_abi.hpp','tests/native_tu15_simplex_face_select_tests.cpp',str(Path(__file__).relative_to(ROOT))]
    paths.extend(['tools/validate_native_simplex_face_refine_tu15.py','tools/native_simplex_face_refine_replacements.py']);pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include \"([^\"\n]+)\"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)}
    try:
        report['reference'],block=reference(scratch);binary=scratch/'face-select';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_face_select_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Face-select fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise ValueError('Face-select source proof failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        if args.xbe is not None:report['hardware']=hardware(args,scratch,binary,block)
        kernel=ROOT/'include/halo2/native_tu15_simplex_face_select_kernel.hpp';text=kernel.read_text();mutants={
            'reverse_first_face_normal':('d.normal_first=N::subtract(d.positive_first,d.negative_first);','d.normal_first=N::subtract(d.negative_first,d.positive_first);'),
            'omit_fresh_apex_read':('d.apex_fresh=memory.vector_read(P::apex_fresh,d,d.vertices+0x30u);','d.apex_fresh=d.apex;'),
            'skip_genuine_refinement':('return memory.refine(d);','memory.finish(d,d.choice);return d.choice;'),
        };report['semantic_mutants']={}
        for name,(old,new)in mutants.items():
            if text.count(old)!=1:raise ValueError('Face-select mutation seam drift: '+name)
            changed=text.replace(old,new);directory=scratch/'mutants'/name;target=directory/'halo2'/kernel.name;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(changed)
            mutant=list(command);at=mutant.index('-I'+str(ROOT/'include'));mutant[at:at]=['-I'+str(directory)];mutant[-1]=str(directory/'probe');built=subprocess.run(mutant,capture_output=True,text=True)
            if built.returncode:raise ValueError('Face-select mutant did not compile: '+name+'\n'+built.stderr[-5000:])
            probe=subprocess.run([str(directory/'probe')],capture_output=True,text=True)
            if probe.returncode==0:raise ValueError('Face-select semantic mutant accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-2000:])
        report['source_sha256']=pins;report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
        if not report['sources_unchanged']:raise ValueError('Face-select proof input drift')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
