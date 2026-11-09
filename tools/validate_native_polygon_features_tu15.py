#!/usr/bin/env python3
"""Complete1F6690 native polygon feature expansion, with full retained ABI."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_direction_tu15 import ROOT,sha
from validate_native_simplex_initialize_tu15 import FEATURE_PIN
import native_polygon_features_replacements as binding

OLD_FEATURE_BRANCH='''    if(owner.identity==0x1f6690u){
        auto last=count-1u;if(!nonnegative(last))return;do{const auto id=m.word(ids+last*2u,2);const auto value=m.vector_bytes(shape.identity+(id+2u)*16u);m.vector_bytes(0x1f66beu,output+last*16u,value);if(!last)break;--last;}while(true);return;
    }'''
OLD_FEATURE_BRANCH_PIN='2eaf7f2dbc34d67abbcda6ac411ae3b31e400025d1f8c87c00fc08dcdde071b0'

def reference(scratch):
    if hashlib.sha256(OLD_FEATURE_BRANCH.encode()).hexdigest()!=OLD_FEATURE_BRANCH_PIN:raise ValueError('Frozen pre-edit polygon feature value reference drift')
    value_body=OLD_FEATURE_BRANCH.split('\n',1)[1].rsplit('\n',1)[0]
    (scratch/'polygon_features_value_reference.inc').write_text('static void original_value(ValueMemory&m,NativeTu15ContactComponent shape,std::uint32_t ids,std::uint32_t count,std::uint32_t output){\n'+value_body+'\n}\n')
    text=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text()
    child=binding.canonical_body(next(body for sig,body in function_bodies(text)if sig.symbol=='1f1da0'))
    if hashlib.sha256(child.encode()).hexdigest()!=FEATURE_PIN:raise ValueError('Complete genuine feature child body drift')
    child=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',child).replace('run_1f1da0(ObservedWorldState','run_reference(WorldState',1)
    ordered=child.replace('run_reference(','run_ordered(',1)
    for value in ('s.reg(2,4,0)','s.reg(7,4,0)'):
        needle=f's.flags.shift({value},0x4u,4)'
        if ordered.count(needle)!=1:raise ValueError('Original immediate shift context drift')
        ordered=ordered.replace(needle,f'native_simplex_candidate_detail::shift4(s,{value})')
    contact=(ROOT/'src/physics/runtime/physics_contact_runtime_generated.cpp').read_text()
    canonical=binding.canonical_body(next(body for sig,body in function_bodies(contact)if sig.symbol=='1f1da0'))
    if hashlib.sha256(canonical.encode()).hexdigest()!=binding.BODY_PINS['State']:raise ValueError('Complete Contact polygon feature child drift')
    canonical=canonical.replace('run_1f1da0(State','run_canonical(WorldState',1)
    prepared_world=binding.replace_polygon_features(text);prepared_contact=binding.replace_polygon_features(contact)
    for original,prepared in ((text,prepared_world),(contact,prepared_contact)):
        if binding.replace_polygon_features(prepared)!=prepared:raise ValueError('Polygon feature preparation not idempotent')
        if binding.remove_polygon_features(prepared)!=binding.remove_polygon_features(original):raise ValueError('Polygon feature preparation changed unrelated bytes')
    bound_world=next(body for sig,body in function_bodies(prepared_world)if sig.symbol=='1f1da0').replace('run_1f1da0(','run_bound_world(',1)
    bound_contact=next(body for sig,body in function_bodies(prepared_contact)if sig.symbol=='1f1da0').replace('run_1f1da0(','run_bound_contact(',1)
    bound_world=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',bound_world)
    source=child+'\n'+ordered+'\n'+canonical+'\n'+bound_world+'\n'+bound_contact;(scratch/'polygon_features_reference.inc').write_text(source)
    return dict(complete_world_body_sha256=FEATURE_PIN,source_sha256=hashlib.sha256(source.encode()).hexdigest(),source_sites=23,
        normal_reference_unmodified=True,real_bound_world_and_contact_bodies=True,retained_prefix_reference='Exact body with two immediate4 SHL flag primitives executed natively; no flag masking'),child

def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x1f6690);rows=[];requests=[];traces=[];all_sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True))
    base,size,shape,stack=0x61000000,0x8000,0x61001000,0x61007000
    counts=(0,1,2,3,4,5,7,0xffffffff,0x80000001)
    for k in range(144):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=shape;gp[4]=stack
        count=counts[k%len(counts)];ids=base+0x4000;output=base+0x5000
        if k%8==1:output=stack-16;count=1
        if k%8==2:output=shape+0x30
        for n in range(8):struct.pack_into('<H',bank,ids+n*2-base,n%4)
        for row in range(4):
            for lane in range(4):put(shape+(row+2)*16+lane*4,rng.getrandbits(32))
        put(stack,0x63000000);put(stack+4,ids);put(stack+8,count);put(stack+12,output)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
        mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,8)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'polygon-features-{k}',entry=0x1f6690,gp=gp,eflags=flags,matrix=ids,source=count,
            before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=500,banks=[dict(address=base,before=bank.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x1f6690,),regions=((base,size),),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('features.in','features.expected','features.actual'));segments=[];packets=0;executed=set();vector_ops={};af_packets=[]
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original polygon feature owner did not return: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            out.write(bytes.fromhex(case['banks'][0]['after']));events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address']
                if len(raw)==16:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Polygon feature vector-store opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                else:events.append((pc,address,before,raw))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            out.write(struct.pack('<2I',0x1f66cb,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
                if packet['entry']==0x1f66ba and packet['eflags']&0x10 and len(af_packets)<4:af_packets.append(dict(case=name,gp=packet['gp'],eflags=packet['eflags'],instruction=packet['entry']))
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical polygon feature failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 polygon features differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical polygon feature coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),source_sites=len(executed),full_pre_io_gp_flags_xmm_fp_live80_packets=packets,complete_normal_abi_banks_store_operands=True,
        physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_fault_atomicity=False,original_AF1_packets=af_packets,
        comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,declared_algorithm_inputs=True,original_text_unmodified=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_polygon_features_kernel.hpp','include/halo2/native_tu15_polygon_features_value.hpp','src/physics/runtime/native_tu15_polygon_features_abi.hpp','tests/native_tu15_polygon_features_tests.cpp',str(Path(__file__).relative_to(ROOT)),'tools/native_polygon_features_replacements.py','src/physics/native_tu15_contact_features.cpp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_simplex_candidate_abi.hpp','src/physics/runtime/native_tu15_simplex_direction_abi.hpp','src/physics/runtime/physics_step_internal.hpp','src/physics/runtime/physics_world_runtime_internal.hpp']
    pins={name:sha(ROOT/name)for name in paths}
    try:
        report['reference'],block=reference(scratch);binary=scratch/'features';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_polygon_features_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise RuntimeError('Polygon feature fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise RuntimeError('Polygon feature fixture failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        environment=os.environ.copy();environment['HALO2_REFERENCE_POLYGON_FEATURES']='1'
        forced=subprocess.run([str(binary),'--forced-reference'],capture_output=True,text=True,env=environment)
        if forced.returncode:raise ValueError('Polygon feature positive forced-reference control failed:\n'+forced.stderr[-5000:])
        report['forced_reference']=json.loads(forced.stderr.strip().splitlines()[-1])
        isolation=os.environ.copy();isolation['HALO2_REFERENCE_POLYGON_SUPPORT']='1';isolation['HALO2_REFERENCE_MATRIX_LEAVES']='1'
        isolated=subprocess.run([str(binary)],capture_output=True,text=True,env=isolation)
        if isolated.returncode:raise ValueError('Independent polygon-feature reference controls affected default route:\n'+isolated.stderr[-5000:])
        report['independent_reference_controls']=dict(polygon_support=True,matrix_leaves=True,positive_native_routes=256)
        if args.xbe:report['original_hardware']=hardware(args,scratch,binary,block)
        text=(ROOT/'include/halo2/native_tu15_polygon_features_kernel.hpp').read_text();report['semantic_mutants']={}
        changes={
            'wrong_feature_offset':('d.offset=(d.feature+2u)<<4u;','d.offset=(d.feature+3u)<<4u;'),
            'forward_output':('d.output-=16u','d.output+=16u'),
            'zero_count_exits_early':('if(!(d.index&0x80000000u)){','if(d.index&&!(d.index&0x80000000u)){'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Polygon feature mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_polygon_features_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Polygon feature mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or not any(message in rejected.stderr for message in ('Native polygon feature mismatch','Shared polygon feature value mismatch')):raise ValueError('Polygon feature mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-1000:])
        report['source_drift_controls']={}
        for path in ('src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/runtime/physics_contact_runtime_generated.cpp'):
            original=(ROOT/path).read_text();found=binding.authenticate(original);body=found[0][0]
            cases={'body':original.replace(body,body.replace('0x8u+s.gp[4]','0xcu+s.gp[4]',1),1),
                'dispatcher':original.replace('std::uint32_t target,std::uint32_t site,bool tail){','std::uint32_t target,std::uint32_t site,bool tail){/*declared drift*/',1),
                'resolver':original.replace('case 0x1f1da0u:return 0x1f6690u;','case 0x1f1da0u:return 0x1f6691u;',1)}
            for name,changed in cases.items():
                if changed==original:raise ValueError('Polygon feature source mutation absent: '+name)
                try:binding.replace_polygon_features(changed)
                except ValueError as error:
                    expected='Complete polygon-feature body drift'if name=='body'else'Complete polygon-feature selected resolver or dispatcher drift'
                    if str(error)!=expected:raise
                else:raise ValueError('Polygon feature drift accepted: '+name)
                report['source_drift_controls'][path+':'+name]=dict(rejected=True,expected=expected)
        report['runtime_refusals']=11
        # Load this host's standard-library prelude first. Defining _MSC_VER
        # before Clang's libc headers changes their wchar ABI, which is not
        # the portable header branch this syntax/refusal smoke is testing.
        smoke=scratch/'portable_feature_guard.cpp';smoke.write_text('#include <array>\n#include <cstddef>\n#include <cstdint>\n#include <cstdlib>\n#include <thread>\n#define _MSC_VER 1930\n#include "physics/runtime/native_tu15_polygon_features_abi.hpp"\nstruct UnownedState{};int main(){UnownedState s;return halo2::native_polygon_features_detail::bridge(s)?1:0;}\n')
        portable=scratch/'portable_feature_guard';portable_command=[args.compiler,'-std=c++20','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(smoke),'-o',str(portable)]
        built=subprocess.run(portable_command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Unsupported feature backend guard failed to compile:\n'+built.stderr[-5000:])
        refused=subprocess.run([str(portable)],capture_output=True,text=True)
        if refused.returncode:raise ValueError('Unsupported feature backend admitted owner')
        report['portable_backend_guard']=dict(command=portable_command,compiled_without_native_dependencies=True,bridge_refused=True,actual_MSVC_runtime_qualified=False)
        report['shared_value_view']=dict(cases=128,pre_edit_algorithm_sha256=OLD_FEATURE_BRANCH_PIN,complete_banks_ordered_reads_stores=True,
            public_semantic_vector_protocol='One16-byte store; distinct from raw8+8 provider protocol')
        report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Polygon feature proof inputs changed')
        report.update(passed=True,original_hardware_verified=bool(args.xbe),physical_fault_atomicity_verified=False)
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
