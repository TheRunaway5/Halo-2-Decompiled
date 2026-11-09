#!/usr/bin/env python3
"""Private30E930 feature-call/transform stage, retaining real1F6690 children."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_direction_tu15 import BODIES,ROOT,original_bodies,sha
from native_polygon_features_replacements import canonical_body

FEATURE_PIN='3e8f283d4b28809a1080ad526e6a03b338c0740b9f3cedde58b221a1ecc41df5'

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30e930);rows=[];requests=[];traces=[];all_sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
    traces.append(dict(entry=0x1f6690,arguments=0,gp=True,xmm=True))
    base,size,robase,rosize=0x61000000,0x8000,0x462000,0x1000;work=base+0x100;stack=base+0x7000
    section=next(s for s in image.sections if s.va<=robase and robase+rosize<=s.va+s.rsize)
    ro=image.data[section.raw+robase-section.va:section.raw+robase-section.va+rosize]
    if struct.unpack_from('<I',ro,0x104)[0]!=0x1f6690:raise ValueError('Genuine polygon feature vtable+34 differs')
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    for k in range(128):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=work;gp[4]=stack+k%16
        first,second,cache,relative=base+0x1000,base+0x2000,base+0x4000,base+0x5000
        put(first,0x4620d0);put(second,0x4620d0)
        first_count,second_count=k%4,(k//4)%4
        if k%32==31:first_count,second_count=0xff,2
        if k%32==30:first_count,second_count=1,0xff
        put(cache-2,0);put(cache,0x00010000);put(cache+4,0x00030002);put(cache+8,first_count|(second_count<<8))
        for object in (first,second):
            for row in range(4):
                for lane in range(4):put(object+(row+2)*16+lane*4,finite((1+rng.randrange(31))/8)if k<64 else edges[(k+row*3+lane)%len(edges)])
        for row in range(4):
            for lane in range(4):put(relative+row*16+lane*4,finite((1+rng.randrange(31))/8)if k<64 else edges[(k+row*3+lane*5)%len(edges)])
        if k%16==7:
            frame=((gp[4]-4)&~15)-128;gp[1]=frame-0x40
            put(cache+8,0x101);put(first+0x28,base+0x900);put(first+0x2c,base+0x300);put(base+0x300,1);put(base+0x900,1)
        put(gp[4],0x63000000);put(gp[4]+4,first);put(gp[4]+8,second);put(gp[4]+12,relative);put(gp[4]+16,cache);put(gp[4]+20,base+0x6000)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
        mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,8)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'initialize-{k}',entry=0x30e930,gp=gp,eflags=flags,matrix=first,source=second,
            before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=1000,stop_before_entries=[0x30ea00],
            banks=[dict(address=base,before=bank.hex()),dict(address=robase,before=ro.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30e930,0x1f6690),regions=((base,size),(robase,rosize)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('initialize.in','initialize.expected','initialize.actual'));segments=[];packets=0;executed=set();vector_ops={};child_entries=0
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']or case.get('stopped_at')!=0x30ea00:raise ValueError('Original initialization did not reach real iteration boundary: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);inp.write(ro);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for region in case['banks']:out.write(bytes.fromhex(region['after']))
            events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address']
                if len(raw)==16:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Initialization vector-store opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                else:events.append((pc,address,before,raw))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            calls=[p for p in case['calls']if p['entry']==0x1f6690]
            if len(calls)!=2:raise ValueError('Original feature child count differs')
            child_entries+=len(calls);out.write(struct.pack('<3I',0x30e9fb,len(calls),len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
            executed.update(set(case['executed_sites'])&all_sites)
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical initialization failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 initialization differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    native_child=scratch/'initialize.native-feature-closure'
    with inputs.open('rb')as inp,native_child.open('wb')as out:r=subprocess.run([str(binary),'--fixture-native-features'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native initialization/feature closure failed: '+r.stderr.decode()[-5000:])
    c=native_child.read_bytes()
    if a!=c:
        offset=next((n for n,(x,y)in enumerate(zip(a,c))if x!=y),min(len(a),len(c)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 fully native initialization/feature closure differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{c[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical initialization coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),parent_source_sites=len(executed),full_parent_pre_io_and_child_entry_gp_flags_xmm_fp_live80_packets=packets,
        actual_feature_entries=child_entries,complete_normal_abi_banks_store_operands=True,retained_literal_feature_child=True,
        also_full_native_feature_closure=True,native_feature_closure_literal_callbacks=0,
        authentic_vtable=dict(address=0x4620d0,slot=0x34,owner=0x1f6690,page_sha256=hashlib.sha256(ro).hexdigest()),
        physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_fault_atomicity=False,
        comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,declared_algorithm_inputs=True,
        genuine_original_child_code_unmodified=True,whole_parent_closed=False)

def reference(scratch):
    bodies=original_bodies();world=next(iter(bodies.values()))
    block=world.split('\n',1)[1].split('L30af10:',1)[0]
    if block.count('goto L30af10;')!=1:raise ValueError('Initialization continuation drift')
    actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',block).replace('goto L30af10;','return;')
    text=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
    child=canonical_body(next(body for sig,body in function_bodies(text)if sig.symbol=='1f1da0'))
    if hashlib.sha256(child.encode()).hexdigest()!=FEATURE_PIN:raise ValueError('Complete genuine feature child body drift')
    child=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',child).replace('run_1f1da0(ObservedWorldState','child(WorldState',1)
    helper=re.search(r'std::uint32_t sign_extend\(std::uint32_t x,unsigned n\) \{.*?\n\}',text,re.S)[0]
    ordered=child.replace('child(','child_ordered(',1)
    for value in ('s.reg(2,4,0)','s.reg(7,4,0)'):
        needle=f's.flags.shift({value},0x4u,4)'
        if ordered.count(needle)!=1:raise ValueError('Initialization child immediate shift context drift')
        ordered=ordered.replace(needle,f'native_simplex_candidate_detail::shift4(s,{value})')
    source=helper+'\n'+child+'\n'+ordered+'\nstatic void run_stage(WorldState&s){\n'+actual+'}\n'
    (scratch/'simplex_initialize_reference.inc').write_text(source)
    return dict(complete_parent_body_sha256=BODIES,feature_child_body_sha256=FEATURE_PIN,
        stage_sha256=hashlib.sha256(source.encode()).hexdigest(),parent_sites=67,
        actual_child=0x1f6690,child_is_retained_literal_not_native=True),actual

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_initialize_kernel.hpp','src/physics/runtime/native_tu15_simplex_initialize_abi.hpp','tests/native_tu15_simplex_initialize_tests.cpp',str(Path(__file__).relative_to(ROOT)),'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_simplex_direction_abi.hpp','include/halo2/native_tu15_simplex_direction_kernel.hpp']
    paths+=['src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp','cmake/NativeTu15SimplexInitialize.cmake','include/halo2/native_tu15_polygon_features_kernel.hpp','src/physics/runtime/native_tu15_polygon_features_abi.hpp','src/physics/runtime/native_tu15_simplex_candidate_abi.hpp','tools/native_polygon_features_replacements.py']
    pins={name:sha(ROOT/name)for name in paths}
    try:
        report['reference'],block=reference(scratch)
        binary=scratch/'initialize';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_initialize_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Initialization fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Initialization fixture failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        closure=subprocess.run([str(binary),'--native-feature-closure'],capture_output=True,text=True)
        if closure.returncode:raise ValueError('Native initialization/feature canonical closure failed:\n'+closure.stderr[-5000:])
        report['native_feature_closure']=json.loads(closure.stderr.strip().splitlines()[-1])
        report['native_feature_closure']['prefix_reference']='Exact child with two immediate4 SHL flag primitives executed natively; normal returns use unmodified body'
        if args.xbe:report['original_hardware']=hardware(args,scratch,binary,block)
        text=(ROOT/'include/halo2/native_tu15_simplex_initialize_kernel.hpp').read_text();report['semantic_mutants']={}
        changes={
            'omit_translation':('d.zt=native_simplex_initialize_numeric::add(d.z_term,d.translation);','d.zt=d.z_term;'),
            'skip_live_first_count':('d.current_count=memory.word_read(P::second_first,d,d.work,4u);','d.current_count=d.first_count;'),
            'wrong_matrix_column':('P::matrix_y,d,d.matrix+0x10u','P::matrix_y,d,d.matrix+0x20u'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Initialization mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_initialize_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Initialization mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or'Native simplex initialization mismatch'not in rejected.stderr:raise ValueError('Initialization mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-1000:])
        original_bodies();report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Initialization proof inputs changed')
        report.update(passed=True,original_hardware_verified=bool(args.xbe),physical_fault_atomicity_verified=False,whole_parent_closed=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
