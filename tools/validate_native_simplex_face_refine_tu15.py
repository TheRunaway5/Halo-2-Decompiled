#!/usr/bin/env python3
"""Private complete actual30D1A0 shared DATA policy and retained raw ABI."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from native_simplex_face_refine_replacements import canonical_body
from validate_native_simplex_direction_tu15 import ROOT,sha
PINS={'src/physics/physics_live_support_tu15_generated.cpp':'2f64e35aeeb2232dfb02a2426a0bef6c5ac794e0c145a53e29bf2d107cb3ff06','src/gameplay/units/death_cycle_runtime_tu15_generated.cpp':'747e16bb89ef518bf8a9ea41b957b80be85b8c2e788071e581f79247ea8c91f7'}
def reference(scratch):
    bodies={}
    for name,pin in PINS.items():
        body=canonical_body(next(b for sig,b in function_bodies((ROOT/name).read_text())if sig.symbol=='30d1a0'))
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Complete30D1A0 source body drift: '+name)
        bodies[name]=body
    original=next(iter(bodies.values())).replace('run_30d1a0(','child_unmodified(',1)
    ordered=original.replace('child_unmodified(','child_ordered(',1).replace('s.push(','atomic_push(s,')
    ordered,count=re.subn(r's\.fp\.push\(ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'logical_load(s,\1);',ordered)
    if count!=7:raise ValueError('Face-refine FLD primitive count drift: '+str(count))
    ordered,count=re.subn(r's\.fp\.comparison\(ext\(f\((r\(s.memory,.*?,4\))\)\),1\);',r'logical_compare(s,\1);',ordered)
    if count!=5:raise ValueError('Face-refine FCOMP primitive count drift')
    ordered,count=re.subn(r's\.fp\.st\(0\)=mul\(s\.fp\.st\(0\),ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'logical_multiply(s,\1);',ordered)
    if count!=2:raise ValueError('Face-refine FMUL primitive count drift')
    if ordered.count('s.fp.comparison(s.fp.st(1),2);')!=1:raise ValueError('Face-refine FCOMPP primitive drift')
    ordered=ordered.replace('s.fp.comparison(s.fp.st(1),2);','logical_compp(s);')
    if ordered.count('s.flags.shift(s.reg(0,4,0),0x4u,4)')!=2:raise ValueError('Face-refine SHL primitive drift')
    ordered=ordered.replace('s.flags.shift(s.reg(0,4,0),0x4u,4)','native_mopp_aabb_abi_detail::shifted_immediate<4>(s,s.reg(0,4,0))')
    primitive='''static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore e{pc,address,{},{}};if(s.observe){e.before.resize(4);s.memory.read(address,e.before);}std::array<std::byte,4>b{};std::memcpy(b.data(),&value,4);s.memory.write(address,b);s.gp[4]=address;if(s.observe){e.written.assign(b.begin(),b.end());s.observe(e);}}
static void logical_load(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};fp.require_capacity(1u);native_simplex_projection_numeric::load(bits);}
static void logical_compare(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};asm volatile("fcomps %0"::"m"(bits):"memory","st");}
static void logical_multiply(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_numeric::multiply(bits);}
static void logical_compp(WorldState&s){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_numeric::compare_pop_two();}
'''
    helper=re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}',(ROOT/next(iter(PINS))).read_text(),re.S)[0]
    if hashlib.sha256(helper.encode()).hexdigest()!='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b':raise ValueError('Face-refine genuine sign-extension helper drift')
    primitive=helper+'\n'+primitive
    source=primitive+original+'\n'+ordered+'\n';(scratch/'simplex_face_refine_reference.inc').write_text(source)
    return dict(complete_source_body_sha256=PINS,reference_sha256=hashlib.sha256(source.encode()).hexdigest(),normal_reference='Unmodified complete Primary source body',prefix_reference='Explicit CPU-atomic PUSH, seven FLD, five FCOMP, two FMUL, one FCOMPP and two actual immediate-SHL primitives',primitive_sha256=hashlib.sha256(primitive.encode()).hexdigest(),production_bound=False),original

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30d1a0);traces=[];all_sites=set()
    delta={0x30d1d1:1,0x30d1e5:1,0x30d2b3:1,0x30d2b7:1,0x30d2bb:2,0x30d2ef:1,0x30d307:1,0x30d322:1}
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta.get(pc,0)))
    table=image.read(0x41d000,0x1000);constant=image.read(0x46a000,0x1000)
    if table[0x760:0x765]!=bytes([2,0,1,2,0])or constant[0xe8:0xec]!=bytes(4):raise ValueError('Immutable face-refine DATA drift')
    base,size,stack=0x61000000,0x8000,0x61007000;rows=[];requests=[];edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    for k in range(224):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[7]=base+0x2000;gp[4]=stack+k%16;point,scores=base+0x1000,base+0x3000;frame_base=((gp[4]-4)&~15)-0x20
        if k%16==6:gp[7]=frame_base-0x30
        if k%16==10:scores=frame_base+0x10
        for p in (point,gp[7]):
            for row in range(12):
                for lane in range(4):put(p+row*16+lane*4,finite((rng.randrange(63)-31)/8)if k<160 else edges[(k+row*3+lane*5)%len(edges)])
        for n in range(3):put(scores+n*4,finite(((k>>(n*2))%4)-1)if k<160 else edges[(k+n*3)%len(edges)])
        put(gp[4],0x63000000);put(gp[4]+4,point);put(gp[4]+8,scores)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,6)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)];xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'face-refine-{k}',entry=0x30d1a0,gp=gp,eflags=flags,matrix=point,source=scores,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=600,banks=[dict(address=base,before=bank.hex()),dict(address=0x41d000,before=table.hex()),dict(address=0x46a000,before=constant.hex())]));requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30d1a0,),regions=((base,size),(0x41d000,0x1000),(0x46a000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/n for n in ('face-refine.in','face-refine.expected','face-refine.actual'));segments=[];packets=0;executed=set();choices=set();shift_packets=[]
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for v in live:out.write(bytes.fromhex(v))
        for v in xmm:out.write(struct.pack('<4I',*v))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original face-refine did not return: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for v in live:inp.write(bytes.fromhex(v))
            for v in xmm:inp.write(struct.pack('<4I',*v))
            inp.write(bank);inp.write(table);inp.write(constant);segments.append((out.tell(),name,'normal_abi'));emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for b in case['banks']:out.write(bytes.fromhex(b['after']))
            out.write(struct.pack('<I',len(case['writes'])))
            for e in case['writes']:
                before=bytes.fromhex(e['before']);raw=bytes.fromhex(e['written']);out.write(struct.pack('<III',e['instruction'],e['address'],len(raw)));out.write(before);out.write(raw)
            choice=case['boundary_gp'][0];choices.add(choice);ret={0:0x30d303,1:0x30d31e,2:0x30d339,0xffffffff:0x30d342}[choice];out.write(struct.pack('<2I',ret,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
                if packet['entry']in(0x30d206,0x30d215)and len(shift_packets)<4:shift_packets.append(dict(case=name,pc=f'{packet["entry"]:x}',eflags=packet['eflags'],gp=packet['gp']))
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:result=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if result.returncode:raise ValueError('Native physical face-refine stopped: '+result.stderr.decode()[-5000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset);raise ValueError(f'Unchanged-i386 face-refine differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical face-refine coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),source_sites=len(executed),returned_choices=[f'{c:08x}'for c in sorted(choices)],full_pre_io_gp_flags_xmm_fp_live80_packets=packets,comparison_sha256=hashlib.sha256(b).hexdigest(),complete_normal_abi_banks_ordered_stores=True,original_text_unmodified=True,declared_algorithm_inputs=True,original=proof,shift_packets=shift_packets,physical_page_fault_atomicity=False)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_face_refine_kernel.hpp','src/physics/runtime/native_tu15_simplex_face_refine_abi.hpp','tests/native_tu15_simplex_face_refine_tests.cpp',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={n:sha(ROOT/n)for n in sorted(seen)}
    try:
        report['reference'],block=reference(scratch);binary=scratch/'face-refine';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_face_refine_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Face-refine compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise ValueError('Face-refine canonical failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        if args.xbe is not None:report['hardware']=hardware(args,scratch,binary,block)
        mutants={
            'reverse_geometric_invalidation':('include/halo2/native_tu15_simplex_face_refine_kernel.hpp','const bool discard_second=memory.compare_products(d);','const bool discard_second=!memory.compare_products(d);'),
            'skip_first_positive_face':('include/halo2/native_tu15_simplex_face_refine_kernel.hpp','if(memory.positive(d,face)){','if(memory.positive(d,face)&&face!=0u){'),
            'wrong_streaming_stride':('src/physics/runtime/native_tu15_simplex_face_refine_abi.hpp','g[1]=s_.flags.plus(g[1],16u,4);','g[1]=s_.flags.plus(g[1],32u,4);'),
        }
        report['semantic_mutants']={}
        for name,(path,old,new)in mutants.items():
            text=(ROOT/path).read_text()
            if text.count(old)!=1:raise ValueError('Face-refine semantic mutation seam drift: '+name)
            changed=text.replace(old,new);directory=scratch/'mutants'/name;relative=path.removeprefix('include/').removeprefix('src/');target=directory/relative;target.parent.mkdir(parents=True,exist_ok=True);target.write_text(changed)
            mutant_command=list(command);mutant_command[mutant_command.index('-I'+str(ROOT/'include')):mutant_command.index('-I'+str(ROOT/'include'))]=['-I'+str(directory)];mutant_command[-1]=str(directory/'probe')
            compiled=subprocess.run(mutant_command,capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Face-refine semantic mutant did not compile: '+name+'\n'+compiled.stderr[-5000:])
            probe=subprocess.run([str(directory/'probe')],capture_output=True,text=True)
            if probe.returncode==0:raise ValueError('Face-refine semantic mutant accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-1500:])
        report['source_sha256']=pins;report['sources_unchanged']=all(sha(ROOT/n)==v for n,v in pins.items())
        if not report['sources_unchanged']:raise ValueError('Face-refine proof input drift')
        report['passed']=True
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','production_bound','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
