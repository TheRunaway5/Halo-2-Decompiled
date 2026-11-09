#!/usr/bin/env python3
"""Private30E930 projection/threshold continuation; no production admission."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from validate_native_simplex_direction_tu15 import ROOT,BODIES,original_bodies,sha

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30edd5);traces=[];all_sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line):
            delta={0x30ee44:1,0x30ee48:1,0x30ee4c:1,0x30ee50:2,0x30ee56:2,0x30ee6b:1,0x30ee6f:1,0x30ee76:1,0x30ee7a:1}.get(pc,0)
            traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta))
    if image.read(0x41d658,4)!=struct.pack('<I',0x34000000):raise ValueError('Original projection threshold bits drift')
    fp_opcodes={}
    for pc,mnemonic in {0x30ee40:'fld',0x30ee44:'fmul',0x30ee48:'fmul',0x30ee4c:'fld',0x30ee50:'fmul',0x30ee56:'fmul',0x30ee5a:'fcompp',0x30ee67:'fld',0x30ee6f:'fmul',0x30ee7a:'fstp'}.items():
        instruction=code.at(pc)
        if instruction.mnemonic!=mnemonic:raise ValueError('Projection x87 opcode/stack ledger drift')
        fp_opcodes[f'{pc:x}']=dict(mnemonic=mnemonic,bytes=instruction.bytes.hex())
    ro=image.read(0x41d000,0x1000);base,size,work,stack=0x61000000,0x4000,0x61000100,0x61003000;rows=[];requests=[]
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    for k in range(160):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[0]=0 if k%5 else (1,0xffffffff,0x80000000)[(k//5)%3];gp[4]=work+0x100 if k%8==1 else stack;gp[6]=work
        for lane in range(4):
            put(work+0x20+lane*4,finite((1+rng.randrange(31))/8)if k<96 else edges[(k+lane*3)%16])
            put(work+0xa0+lane*4,finite((-1 if k&4 else 1)*(32+rng.randrange(31))/8)if k<96 else edges[(k+lane*5+3)%16])
            put(work+0x120+lane*4,finite((1+rng.randrange(7))/8)if k<96 else edges[(k+lane*7+1)%16])
        put(gp[4]+0x14,0x2edbe6ff if k<128 else edges[k%16]);put(gp[4]+0x1c,k)
        put(gp[4],0x63000000);put(gp[4]+4,0x12345678);put(gp[4]+8,0x23456789)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
        mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,6)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'projection-{k}',entry=0x30edd5,gp=gp,eflags=flags,matrix=0x12345678,source=0x23456789,
            before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=100,stop_before_entries=[0x30f581,0x30ee84,0x30f274],
            banks=[dict(address=base,before=bank.hex()),dict(address=0x41d000,before=ro.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,),regions=((base,size),(0x41d000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('projection.in','projection.expected','projection.actual'));segments=[];packets=0;executed=set();branches=set();c1_examples=[]
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name'];stop=case.get('stopped_at')
            if not case['completed']or not case['live_x87_unchanged']or stop not in(0x30f581,0x30ee84,0x30f274):raise ValueError('Original projection did not reach real next owner: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);inp.write(ro);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for region in case['banks']:out.write(bytes.fromhex(region['after']))
            events=case['writes'];out.write(struct.pack('<I',len(events)))
            for e in events:
                before=bytes.fromhex(e['before']);raw=bytes.fromhex(e['written']);out.write(struct.pack('<III',e['instruction'],e['address'],len(raw)));out.write(before);out.write(raw)
            next_branch={0x30f581:0,0x30ee84:1,0x30f274:2}[stop];branches.add(next_branch)
            pc=0x30ee3a if gp[0]else(0x30ee61 if next_branch==0 else 0x30ee7e)
            out.write(struct.pack('<3I',pc,next_branch,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            if next_branch and case['after_fp'][1]&0x200 and len(c1_examples)<4:
                c1_examples.append(dict(case=name,initial_gp=gp,initial_eflags=flags,initial_fp=[cw,status,mxcsr],initial_live80=live,initial_xmm=xmm,
                    initial_points=[dict(address=base+o,bytes=bank[o:o+16].hex())for o in(0x120,0x1a0,0x220)],
                    initial_epsilon=struct.unpack_from('<I',bank,gp[4]+0x14-base)[0],initial_iteration=struct.unpack_from('<I',bank,gp[4]+0x1c-base)[0],
                    input_bank_sha256=hashlib.sha256(bank).hexdigest(),after_fp=case['after_fp'],next_owner=stop))
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical projection failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 projection differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites or len(branches)!=3:raise ValueError('Physical projection site/next-owner coverage incomplete')
    return dict(cases=len(rows),source_sites=len(executed),next_owner_branches=len(branches),full_pre_io_gp_flags_xmm_fp_live80_packets=packets,
        complete_normal_abi_banks_ordered_stores=True,physical_c1_after_fstp_examples=c1_examples,physical_fault_atomicity=False,
        original_threshold_page_sha256=hashlib.sha256(ro).hexdigest(),comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,
        authenticated_live_x87_delta_ledger={f'{p["entry"]:x}':p['x87_delta']for p in traces},
        original_x87_opcodes=fp_opcodes,
        original_text_unmodified=True,no_supplied_outcomes_or_observer_feedback=True)

def reference(scratch):
    bodies=original_bodies();world=next(iter(bodies.values()))
    block=world[world.index('L30b2e5:'):world.index('if(s.services.instruction){s.services.instruction(world_observation_pc(0x30b394u')]
    canonical=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',block).replace('s.require_x87();','')
    contact=bodies['src/physics/runtime/physics_contact_runtime_generated.cpp'];other=contact[contact.index('L30b2e5:'):contact.index('if(s.services.instruction){s.services.instruction(0x30b394u')]
    if canonical!=other:raise ValueError('Complete World/Contact projection numerical body differs')
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',block)
    resolver_pins={'src/physics/runtime/physics_world_runtime_generated.cpp':'20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
        'src/physics/runtime/physics_contact_runtime_generated.cpp':'d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e'}
    resolver=None
    for path,pin in resolver_pins.items():
        owner=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',(ROOT/path).read_text(),re.S)[0]
        if hashlib.sha256(owner.encode()).hexdigest()!=pin:raise ValueError('Complete projection selected resolver drift')
        if 'case 0x419bdcu:return 0x41d658u;'not in owner:raise ValueError('Projection threshold constant mapping drift')
        if resolver is None:resolver=owner
    def routine(name,body):return 'static Next '+name+'(WorldState&s){\n'+body+'return Next::first_support;\nL30ba91:return Next::reduce;\nL30b784:return Next::second_support;\n}\n'
    primitive='''struct FstpStore: native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_projection_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>{
    using Base=native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_projection_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>;
    using Base::Base;using Base::at;using Base::store;
};
static void completed_fstp(WorldState&s,std::uint32_t pc,std::uint32_t address){
    native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_projection_detail::Noop before;
    FstpStore memory{s,before,native_mopp_ray_caller_detail::ScopeIo<WorldState>{&fp}};
    memory.at(pc);const auto pending=fp.stage_single(true);
    memory.store(pc,address,pending.bits,[&]{fp.commit(pending);},true);
}
static void logical_load(WorldState&s,std::uint32_t bits){
    native_raw_x87_detail::Scope<WorldState>fp{s};fp.require_capacity(1u);
    asm volatile("flds %0"::"m"(bits):"memory","st");
}
static void logical_multiply(WorldState&s,std::uint32_t bits){
    native_raw_x87_detail::Scope<WorldState>fp{s};
    asm volatile("fmuls %0"::"m"(bits):"memory","st");
}
static void logical_compare_pop_two(WorldState&s){
    native_raw_x87_detail::Scope<WorldState>fp{s};
    asm volatile("fcompp":::"memory","st","st(1)");
}
'''
    needle='w(s.memory,s.observe,0x30ee7au,(0x10u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();'
    if actual.count(needle)!=1:raise ValueError('Exact original FSTP publication seam drift')
    corrected=actual.replace(needle,'completed_fstp(s,0x30ee7au,0x10u+s.gp[4]);')
    corrected,loads=re.subn(r's\.fp\.push\(ext\(f\((r\(s\.memory,.*?,4\))\)\)\);',r'logical_load(s,\1);',corrected)
    corrected,multiplies=re.subn(r's\.fp\.st\(0\)=mul\(s\.fp\.st\(0\),ext\(f\((r\(s\.memory,.*?,4\))\)\)\);',r'logical_multiply(s,\1);',corrected)
    comparison='s.fp.comparison(s.fp.st(1),2);'
    if loads!=3 or multiplies!=5 or corrected.count(comparison)!=1:raise ValueError('Exact projection x87 primitive ledger drift')
    corrected=corrected.replace(comparison,'logical_compare_pop_two(s);')
    source=resolver+'\n'+primitive+routine('run_unmodified',actual)+routine('run_stage',corrected)
    (scratch/'simplex_projection_reference.inc').write_text(source)
    return dict(complete_parent_body_sha256=BODIES,stage_sha256=hashlib.sha256(source.encode()).hexdigest(),
        source_sites=len(re.findall(r'services.instruction\(0x',actual)),normal_reference_unmodified=False,declared_midowner_continuation=True,
        actual_x87_primitive_overlays=dict(fld=loads,fmul=multiplies,fcompp=1,fstp=1),primitive_sha256=hashlib.sha256(primitive.encode()).hexdigest(),
        old_observer_before_pop_fault_protocol_not_qualified=True,unmodified_source_counterexample_retained=True),actual

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);p.add_argument('--hardware-only',action='store_true');args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():print('Authenticated TU1.5 XBE is not configured');return 77
    if args.hardware_only and not args.xbe:p.error('--hardware-only requires original XBE')
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_projection_kernel.hpp','src/physics/runtime/native_tu15_simplex_projection_abi.hpp','tests/native_tu15_simplex_projection_tests.cpp',str(Path(__file__).relative_to(ROOT)),'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_mopp_ray_scale_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_step_internal.hpp','include/halo2/native_tu15_simplex_direction_kernel.hpp','src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp','tools/validate_native_simplex_direction_tu15.py','tools/validate_native_tu15_prt_helpers.py','cmake/NativeTu15SimplexProjection.cmake']
    pins={name:sha(ROOT/name)for name in paths}
    try:
        report['reference'],block=reference(scratch);binary=scratch/'projection';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_projection_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Projection fixture compile failed:\n'+built.stderr[-10000:])
        if not args.hardware_only:
            result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
            if result.returncode:raise RuntimeError('Projection fixture failed:\n'+report['diagnostic'])
            report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
            old=subprocess.run([str(binary),'--unmodified-reference'],capture_output=True,text=True)
            report['unmodified_reference_control']=dict(passed=old.returncode==0,exit_code=old.returncode,diagnostic=old.stderr[-3000:],qualified=False)
        else:report['canonical']=dict(executed=False,prior_unmodified_failure='canonical-sanitizers-r2.json; oldSW1120 vs native1320 C1 difference retained without masking')
        if args.xbe:report['original_hardware']=hardware(args,scratch,binary,block)
        if not args.hardware_only:
            changes={
                'wrong_projection_relation':('include/halo2/native_tu15_simplex_projection_kernel.hpp','if(d.relation&0x100u){','if(d.relation&0x4000u){'),
                'stale_cube_operand':('include/halo2/native_tu15_simplex_projection_kernel.hpp','memory.scalar_read(P::projection_cube,d)','X::low(d.projection)'),
                'clear_retired_C1':('src/physics/runtime/native_tu15_simplex_projection_abi.hpp','fp_.commit(pending);','fp_.commit(pending);auto image=native_raw_x87_detail::capture();image.word(4,std::uint16_t(image.word(4)&~0x200u));native_raw_x87_detail::restore(image);'),
            };report['semantic_mutants']={}
            for name,(path,needle,replacement)in changes.items():
                text=(ROOT/path).read_text()
                if text.count(needle)!=1:raise ValueError('Projection mutant seam drift: '+name)
                directory=scratch/name;relative=Path(path).relative_to('include'if path.startswith('include/')else'src');target=directory/relative;target.parent.mkdir(parents=True);variant=text.replace(needle,replacement);target.write_text(variant)
                changed=command.copy();changed.insert(changed.index('-I'+str(ROOT/'include')),'-I'+str(directory));changed[-1]=str(directory/'mutant')
                built=subprocess.run(changed,capture_output=True,text=True)
                if built.returncode:raise ValueError('Projection mutant compile failed:\n'+built.stderr[-5000:])
                rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
                if rejected.returncode==0 or'Native simplex projection mismatch'not in rejected.stderr:raise ValueError('Projection mutant not rejected: '+name+'\n'+rejected.stderr[-3000:])
                report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-1500:])
        original_bodies();report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Projection proof inputs changed')
        report.update(passed=True,original_hardware_verified=bool(args.xbe),physical_fault_atomicity_verified=False,whole_parent_closed=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report.get('diagnostic','Unchanged-i386 hardware-only scope; unmodified canonical gap remains explicit').strip());return 0
if __name__=='__main__':raise SystemExit(main())
