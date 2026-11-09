#!/usr/bin/env python3
"""Private30E930 loop/epsilon/reduction-classification native continuation."""
import argparse,ast,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from validate_native_simplex_direction_tu15 import ROOT,BODIES,original_bodies,sha
import validate_native_simplex_projection_tu15 as projection

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');tables=image.read(0x30f6e0,69)
    if hashlib.sha256(tables).hexdigest()!='4863a4ed7c190dcb96aad512ffa1c04a8a0c75d271309c8b0fb5ea487bd6675b':raise ValueError('Whole original simplex classification tables drift')
    targets=list(struct.unpack('<11I',tables[:44]));traces=[];sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);sites.add(pc)
        if re.search(r'\b(?:r|w)\(s.memory',line):traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=1 if pc in(0x30ea06,0x30ea0a,0x30ea13)else 0))
    fp_opcodes={}
    for pc,mnemonic in ((0x30ea02,'fld'),(0x30ea08,'fadd'),(0x30ea13,'fstp')):
        instruction=code.at(pc)
        if instruction.mnemonic!=mnemonic:raise ValueError('Iteration original FP opcode drift')
        fp_opcodes[f'{pc:x}']=instruction.bytes.hex()
    rng=random.Random(0x30ea00);base,size,work,stack=0x61000000,0x4000,0x61000100,0x61003000;rows=[];requests=[]
    eps=(0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x2edbe6ff,0xbf800000)
    for k in range(160):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=work if k%8==1 else stack;gp[6]=work;gp[7]=work+4
        put(work,k%5 if k<100 else rng.getrandbits(32));put(work+4,(k//5)%5 if k<100 else rng.getrandbits(32))
        if k%16==7:gp[7]=work
        put(gp[4]+0x14,eps[k%16]);put(gp[4]+0x1c,k if k<80 else 0xffffffff)
        put(gp[4],0x63000000);put(gp[4]+4,0x12345678);put(gp[4]+8,0x23456789)
        # The common observer's three declared entry words can alias counts.
        # Native receives precisely those same initial caller-backed words.
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11);mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,7)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)];xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'iteration-{k}',entry=0x30ea00,gp=gp,eflags=flags,matrix=0x12345678,source=0x23456789,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=30,stop_before_entries=targets,banks=[dict(address=base,before=bank.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,),regions=((base,size),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('iteration.in','iteration.expected','iteration.actual'));segments=[];packets=0;executed=set();kinds=set()
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name'];stop=case.get('stopped_at')
            if not case['completed']or not case['live_x87_unchanged']or stop not in targets:raise ValueError('Original iteration did not reach geometric reduction boundary: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);inp.write(tables);segments.append((out.tell(),name,'normal_abi'));emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live);out.write(bytes.fromhex(case['banks'][0]['after']));out.write(struct.pack('<I',len(case['writes'])))
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);out.write(struct.pack('<III',event['instruction'],event['address'],len(raw)));out.write(before);out.write(raw)
            kind=targets.index(stop);kinds.add(kind);used_tables=0x30ea37 in case['executed_sites'];out.write(struct.pack('<3I',0x30ea37 if used_tables else 0x30ea29,kind,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native original-input iteration failed: '+r.stderr.decode()[-5000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset);raise ValueError(f'Unchanged-i386 iteration differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=sites or len(kinds)!=11:raise ValueError('Original iteration instruction/reduction coverage incomplete')
    return dict(cases=len(rows),source_sites=len(executed),reduction_kinds=len(kinds),full_pre_io_gp_flags_xmm_fp_live80_packets=packets,complete_normal_abi_banks_ordered_stores=True,
        authentic_immutable_code_tables_sha256=hashlib.sha256(tables).hexdigest(),original_code_unmodified=True,code_tables_neither_patched_nor_replaced=True,original_fp_opcodes=fp_opcodes,
        live_x87_delta_ledger={f'{p["entry"]:x}':p['x87_delta']for p in traces},physical_fault_atomicity=False,comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof)

def reference(scratch):
    bodies=original_bodies();world=next(iter(bodies.values()));block=world[world.index('L30af10:'):world.index('L30af4e:')]
    canonical=re.sub(r'world_observation_pc\((0x[0-9a-f]+u),0x[0-9a-f]+u,s\)',r'\1',block).replace('s.require_x87();','')
    contact=bodies['src/physics/runtime/physics_contact_runtime_generated.cpp'];other=contact[contact.index('L30af10:'):contact.index('L30af4e:')]
    if canonical!=other:raise ValueError('World/Contact iteration body differs')
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',block)
    resolver=None;inverse=None
    pins={'physics_world_runtime_generated.cpp':('20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','e5779c8ed411532bc15334539a7814df2a584d81946b32d413bb5346d166e5c6'),
        'physics_contact_runtime_generated.cpp':('d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','4e328c43da59895ab6c3f14a24129e538924d7467cb173bd65cdc3373d2a703d')}
    for path,(selected_pin,inverse_pin)in pins.items():
        text=(ROOT/'src/physics/runtime'/path).read_text();selected=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',text,re.S)[0];branch=re.search(r'std::uint32_t canonical_branch\(.*?\}\}',text,re.S)[0]
        if hashlib.sha256(selected.encode()).hexdigest()!=selected_pin or hashlib.sha256(branch.encode()).hexdigest()!=inverse_pin:raise ValueError('Complete iteration resolver/inverse owner drift')
        if resolver is None:resolver,inverse=selected,branch
    assignments=[node.value for node in ast.walk(ast.parse(Path(projection.__file__).read_text()))if isinstance(node,ast.Assign)and any(isinstance(target,ast.Name)and target.id=='primitive'for target in node.targets)]
    if len(assignments)!=1:raise ValueError('Qualified native x87 reference primitive seam changed')
    primitive=ast.literal_eval(assignments[0])+'''static void logical_double(WorldState&s){
    native_raw_x87_detail::Scope<WorldState>fp{s};asm volatile("fadd %%st(0),%%st(0)":::"memory","st");
}
'''
    corrected,n=re.subn(r's\.fp\.push\(ext\(f\((r\(s\.memory,.*?,4\))\)\)\);',r'logical_load(s,\1);',actual)
    add='s.fp.st(0)=add(s.fp.st(0),s.fp.st(0));';store='w(s.memory,s.observe,0x30ea13u,(0x14u+s.gp[4]),4,u(spill(s.fp.st(0))));s.fp.pop();'
    if n!=1 or corrected.count(add)!=1 or corrected.count(store)!=1:raise ValueError('Exact iteration FP primitive ledger drift')
    corrected=corrected.replace(add,'logical_double(s);').replace(store,'completed_fstp(s,0x30ea13u,0x14u+s.gp[4]);')
    labels={'30af4e':'paired_two_three','30af6c':'paired_three_two','30af9e':'segments','30b041':'second_face','30b086':'second_triangle','30b1b2':'first_face','30b1e3':'first_triangle','30b2e0':'degenerate','30b576':'second_edge','30b685':'first_edge','30b763':'points'}
    source=resolver+'\n'+inverse+'\n'+primitive+'static Kind run_stage(WorldState&s){\n'+corrected+'\n'+''.join('L'+label+':return Kind::'+kind+';\n'for label,kind in labels.items())+'}\n'
    (scratch/'simplex_iteration_reference.inc').write_text(source)
    return dict(complete_parent_body_sha256=BODIES,complete_resolver_inverse_sha256=pins,source_sha256=hashlib.sha256(source.encode()).hexdigest(),source_sites=17,
        original_software_FP_reference_unmodified=False,native_FP_primitives=['FLD','FADD','FSTP'],primitive_sha256=hashlib.sha256(primitive.encode()).hexdigest(),whole_parent_qualified=False),actual

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_iteration_kernel.hpp','src/physics/runtime/native_tu15_simplex_iteration_abi.hpp','tests/native_tu15_simplex_iteration_tests.cpp',str(Path(__file__).relative_to(ROOT)),'include/halo2/native_tu15_simplex_projection_kernel.hpp','src/physics/runtime/native_tu15_simplex_projection_abi.hpp','tools/validate_native_simplex_projection_tu15.py','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','src/physics/runtime/native_tu15_mopp_ray_scale_frame.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp']
    hashes={name:sha(ROOT/name)for name in paths}
    try:
        report['reference'],block=reference(scratch);binary=scratch/'iteration';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_iteration_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Iteration fixture compile failed:\n'+built.stderr[-8000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-6000:]
        if result.returncode:raise ValueError('Iteration fixture failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,block)
        changes={
            'add_instead_of_packed_or':('include/halo2/native_tu15_simplex_iteration_kernel.hpp','d.packed=(d.first<<3u)|d.second;','d.packed=(d.first<<3u)+d.second;'),
            'wrong_counter_publication':('include/halo2/native_tu15_simplex_iteration_kernel.hpp','++d.iteration;','d.iteration+=2u;'),
            'wrong_edge_class':('src/physics/runtime/native_tu15_simplex_iteration_abi.hpp','case 0x30f175u:return Kind::first_edge;','case 0x30f175u:return Kind::second_edge;'),
        };report['semantic_mutants']={}
        for name,(path,needle,replacement)in changes.items():
            text=(ROOT/path).read_text()
            if text.count(needle)!=1:raise ValueError('Iteration mutation seam drift: '+name)
            directory=scratch/name;relative=Path(path).relative_to('include'if path.startswith('include/')else'src');target=directory/relative;target.parent.mkdir(parents=True);variant=text.replace(needle,replacement);target.write_text(variant)
            changed=command.copy();changed.insert(changed.index('-I'+str(ROOT/'include')),'-I'+str(directory));changed[-1]=str(directory/'mutant');built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Iteration mutant compile failed:\n'+built.stderr[-5000:])
            failed=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if failed.returncode==0 or'Native simplex iteration mismatch'not in failed.stderr:raise ValueError('Iteration mutant not rejected: '+name+'\n'+failed.stderr[-3000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=failed.stderr[-1000:])
        original_bodies();report['sources_unchanged']=hashes=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Iteration inputs changed during proof')
        report.update(passed=True,whole_parent_qualified=False,physical_fault_atomicity=False)
    except BaseException as error:report['error']=str(error);raise
    finally:report['source_sha256']=hashes;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
