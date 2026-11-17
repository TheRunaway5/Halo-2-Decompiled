#!/usr/bin/env python3
"""Private complete30DC10 DATA/raw Frame and real Triangle/Closest proof."""
import argparse,ast,hashlib,json,os,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_paired_tu15 import ROOT,reference as paired_reference
from validate_native_simplex_direction_tu15 import sha
from native_simplex_boundary_replacements import canonical_body
PINS={'src/physics/physics_live_support_tu15_generated.cpp':'2894b1698fbc50d86cfbb6d63f47f24aafd79b622bc2585ee2909b2596818229','src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp':'ddb1c7e82c16409c6684cc8b8b99b22a90284d69f77e02d279d9bef621254208'}
def owning_bodies():return {name:canonical_body(next(b for sig,b in function_bodies((ROOT/name).read_text())if sig.symbol=='30dc10'))for name in PINS}
def reference(scratch):
    bodies=owning_bodies()
    for name,body in bodies.items():
        if hashlib.sha256(body.encode()).hexdigest()!=PINS[name]:raise ValueError('Complete30DC10 body drift: '+name)
    scene=(ROOT/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp').read_text();resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',scene)[0]
    if hashlib.sha256(resolver.encode()).hexdigest()!='618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a':raise ValueError('Complete30DC10 Scene resolver drift')
    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)};operands={int(v,16)for v in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',list(bodies.values())[1])}
    if any(mapping.get(value)!=value for value in operands):raise ValueError('30DC10 Scene literal identity drift')
    actual=next(iter(bodies.values())).replace('run_30dc10(','run_unmodified(',1)
    actual,count=re.subn(r'dispatch\(s,target,0x[0-9a-f]+u,false\);','child(s,target);',actual)
    if count!=3:raise ValueError('30DC10 real child transport drift')
    ordered=actual.replace('run_unmodified(','run_ordered(',1).replace('s.push(','reference::atomic_push(s,')
    ordered,shifts=re.subn(r's.set\(([0-7]),s.flags.shift\(s.reg\(\1,4,0\),0x4u,4\),4,0\);',lambda m:f's.set({m[1]},native_mopp_aabb_abi_detail::shifted_immediate<4>(s,s.reg({m[1]},4,0)),4,0);',ordered)
    if shifts!=11:raise ValueError('30DC10 exact immediate-SHL primitive drift')
    ordered,loads=re.subn(r's.fp.push\(ext\(f\((r\(s.memory,.*?,4\))\)\)\);',r'closest_reference::closest_load(s,\1);',ordered)
    ordered,compares=re.subn(r's.fp.comparison\(ext\(f\((r\(s.memory,.*?,4\))\)\),1\);',r'closest_reference::closest_compare(s,\1);',ordered)
    if (loads,compares)!=(1,1):raise ValueError('30DC10 original FLD/FCOMP primitive drift')
    for old,new in [('_mm_sub_ps(','native_simplex_direction_numeric::subtract(')]:ordered=ordered.replace(old,new)
    actual_add='s.set(3,s.flags.plus(s.reg(3,4,0),0xau,4),4,0);'
    if ordered.count(actual_add)!=1:raise ValueError('30DE2E exact canonical ADD-flag seam drift')
    shift_only=ordered.replace('run_ordered(','run_shift_only(',1).replace(actual_add,'s.set(3,(0xau+s.gp[3]),4,0);')
    source='using reference::sign_extend;\nstatic std::function<void(WorldState&,std::uint32_t)>child;\n'+actual+'\n'+shift_only+'\n'+ordered;(scratch/'simplex_boundary_reference.inc').write_text(source)
    child_report,_,triangle,closest=paired_reference(scratch)
    return dict(complete_owner_sha256=PINS,scene_resolver_sha256=hashlib.sha256(resolver.encode()).hexdigest(),reference_sha256=hashlib.sha256(source.encode()).hexdigest(),child_reference=child_report,source_seam='Pinned unchanged complete parent using independently backed DATA and identical genuinely computed native Triangle/Closest children; separate original proof includes original children',phased_reference='Original-controlled native SHL flags, FLD/FCOMP and atomic PUSH; canonical actual83C30A ADD flags at30DE2E stay unchanged. A separate deliberate missing-ADD mutant reproduces the first private-adapter defect.',primitive_counts=dict(shl4=shifts,fld=loads,fcomp=compares,atomic_push=ordered.count('reference::atomic_push('))),actual,triangle,closest
def hardware(args,scratch,binary,blocks):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from research_mp_sandbox_update_functions import Xbe
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    if sha(args.xbe)!=PIN:raise ValueError('Wrong unchanged original TU1.5 image')
    image=Xbe(args.xbe);code=_Code(image,'.text');path=scratch/'boundary-inputs.bin'
    if code.at(0x30de2e).bytes.hex()!='83c30a':raise ValueError('Genuine30DE2E ADD-EBX opcode drift')
    with path.open('wb')as out:dump=subprocess.run([str(binary),'--dump-inputs'],stdout=out,stderr=subprocess.PIPE)
    if dump.returncode:raise ValueError('Boundary input export failed: '+dump.stderr.decode()[-4000:])
    data=path.read_bytes();position=0
    def take(size):
        nonlocal position
        result=data[position:position+size];position+=size
        if len(result)!=size:raise ValueError('Truncated boundary input')
        return result
    def words(n):return struct.unpack('<'+'I'*n,take(n*4))
    count=words(1)[0];rows=[]
    for k in range(count):
        fields=words(13);gp=list(fields[:8]);flags,mxcsr,cw,status,depth=fields[8:];live=[take(10).hex()for _ in range(depth)];xmm=[list(words(4))for _ in range(8)];banks=[take(0x8000)]+[take(0x1000)for _ in range(4)]
        for address,size,index in((0x41d760,5,1),(0x41c500,16,3),(0x461960,16,4),(0x46a0d0,4,2),(0x46a0e8,4,2),(0x46a25c,4,2)):
            bankbase=(0x61000000,0x41d000,0x46a000,0x41c000,0x461000)[index]
            if banks[index][address-bankbase:address-bankbase+size]!=image.read(address,size):raise ValueError('Boundary genuine immutable input drift')
        segment,triangle=struct.unpack_from('<2I',banks[0],gp[4]-0x61000000+4)
        rows.append(dict(id=k,name=f'boundary-{k}',entry=0x30dc10,gp=gp,eflags=flags,matrix=segment,source=triangle,before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=8000,banks=[dict(address=address,before=bank.hex())for address,bank in zip((0x61000000,0x41d000,0x46a000,0x41c000,0x461000),banks)]))
    if position!=len(data):raise ValueError('Trailing boundary input')
    tree=ast.parse((ROOT/'tools/validate_native_closest_segments_frame_tu15.py').read_text());fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef)and n.name=='hardware');node=next(n for n in ast.walk(fn)if isinstance(n,ast.Assign)and any(isinstance(t,ast.Name)and t.id=='delta'for t in n.targets));delta=ast.literal_eval(node.value)
    delta.update({0x30dd3d:1,0x30d5a3:1,0x30d5a9:1,0x30d5af:1})
    traces=[];sites=set()
    for block in blocks:
        for line in block.splitlines():
            match=re.search(r'services.instruction\(0x([0-9a-f]+)u\)',line)
            if not match:continue
            pc=int(match[1],16);sites.add(pc)
            if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=delta.get(pc,0)))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30dc10,0x30d3f0,0x312620),regions=((0x61000000,0x8000),(0x41d000,0x1000),(0x46a000,0x1000),(0x41c000,0x1000),(0x461000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    expected=scratch/'boundary-expected.bin';actual=scratch/'boundary-actual.bin';segments=[];packets=0;executed=set();children=0;witnesses={}
    def abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for v in live:out.write(bytes.fromhex(v))
        for v in xmm:out.write(struct.pack('<4I',*v))
    with expected.open('wb')as out:
        for case in observed['cases']:
            k=case['id'];name=rows[k]['name'];live=rows[k]['live_x87']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Unchanged boundary original did not return: '+name)
            if k==0:witnesses['original_shift_return']=dict(case=name,gp=case['boundary_gp'],flags=case['boundary_eflags'],fp=case['after_fp'],declared_input_sha256=hashlib.sha256(bytes.fromhex(rows[k]['banks'][0]['before'])).hexdigest(),old_modeled_flags='00000602')
            segments.append((out.tell(),name,'return_abi'));abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            for bank in case['banks']:out.write(bytes.fromhex(bank['after']))
            stores=[]
            for event in case['writes']:
                before,raw=bytes.fromhex(event['before']),bytes.fromhex(event['written'])
                if len(raw)==16:
                    instruction=code.at(event['instruction'])
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=bytes.fromhex('0f29'):raise ValueError('Boundary physical vector opcode drift')
                    for offset in(0,8):stores.append((event['instruction'],event['address']+offset,before[offset:offset+8],raw[offset:offset+8]))
                else:stores.append((event['instruction'],event['address'],before,raw))
            out.write(struct.pack('<I',len(stores)))
            for pc,address,before,raw in stores:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            returns=set(case['executed_sites'])&{0x30de08,0x30de23,0x30de53}
            if len(returns)!=1:raise ValueError('Boundary return-site ownership differs')
            out.write(struct.pack('<2I',next(iter(returns)),len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1;children+=packet['entry']in(0x30d3f0,0x312620)
                if k==1 and packet['entry']==0x30de31:witnesses['original_add_before_store']=dict(case=name,packet=packet,actual_add_instruction='30de2e',actual_add_opcode='83c30a',canonical_source_has_add_flags=True,first_private_adapter_omitted_flags=True,declared_input_sha256=hashlib.sha256(bytes.fromhex(rows[k]['banks'][0]['before'])).hexdigest())
            executed.update(case['executed_sites'])
    with path.open('rb')as inp,actual.open('wb')as out:run=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if run.returncode:raise ValueError('Boundary native hardware fixture failed: '+run.stderr.decode()[-5000:])
    a,b=expected.read_bytes(),actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(v for v in reversed(segments)if v[0]<=offset);raise ValueError(f'Boundary unchanged-i386 mismatch byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    parent_sites={pc for pc in sites if 0x30dc10<=pc<=0x30de53}
    return dict(cases=count,parent_sites=len(parent_sites),executed_parent_sites=len(parent_sites&executed),missing_parent_sites=[f'{pc:x}'for pc in sorted(parent_sites-executed)],full_pre_io_gp_flags_xmm_fp_live80_packets=packets,genuine_child_entries=children,comparison_sha256=hashlib.sha256(b).hexdigest(),original_text_unmodified=True,supplied_child_outcomes=False,physical_vector_page_fault_atomicity=False,original=proof,original_flag_witnesses=witnesses)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_boundary_kernel.hpp','src/physics/runtime/native_tu15_simplex_boundary_frame.hpp','tests/native_tu15_simplex_boundary_tests.cpp','tools/validate_native_simplex_paired_tu15.py','tools/native_simplex_paired_replacements.py','tools/native_simplex_boundary_replacements.py',str(Path(__file__).relative_to(ROOT))];pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    pins={name:sha(ROOT/name)for name in sorted(seen)};report['source_sha256']=pins
    try:
        report['reference'],*blocks=reference(scratch);binary=scratch/'boundary-frame';command=[args.compiler,'-std=c++20','-Wno-ignored-attributes','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_boundary_tests.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Boundary Frame compile failed:\n'+built.stderr[-10000:])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,blocks)
        report['retained_source_negatives']={}
        for option,expected in[('--unmodified-source','boundary-0 pc 30de23/30de23 flags 602/612'),('--missing-add-flags','boundary-1 pc 30de31/30de31 flags 612/606')]:
            negative=subprocess.run([str(binary),option],capture_output=True,text=True)
            if not negative.returncode or expected not in negative.stderr:raise ValueError('Boundary authentic flag counterexample changed:\n'+negative.stderr[-5000:])
            report['retained_source_negatives'][option]=dict(expected_failure=True,diagnostic=negative.stderr[-5000:])
        run=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=run.stderr[-10000:]
        if run.returncode:raise ValueError('Boundary complete source/children seam failed:\n'+run.stderr[-10000:])
        report['source']=json.loads(run.stderr.strip().splitlines()[-1]);report['semantic_mutants']={}
        kernel=ROOT/'include/halo2/native_tu15_simplex_boundary_kernel.hpp';frame=ROOT/'src/physics/runtime/native_tu15_simplex_boundary_frame.hpp'
        for name,path,relative,old,new in[
            ('reverse_first_segment_delta',kernel,'halo2/native_tu15_simplex_boundary_kernel.hpp','d.first_delta=N::subtract(d.first_end,d.first_point);','d.first_delta=N::subtract(d.first_point,d.first_end);'),
            ('omit_real_closest_owner',kernel,'halo2/native_tu15_simplex_boundary_kernel.hpp','memory.closest_child(d);','/* Missing actual Closest owner. */'),
            ('omit_late_second_count',kernel,'halo2/native_tu15_simplex_boundary_kernel.hpp','d.count=memory.count(P::second_count_again,d,false);','/* Reuse stale count. */'),
            ('omit_reduced_count_capture',frame,'physics/runtime/native_tu15_simplex_boundary_frame.hpp','case P::second_reduction_endpoint:s_.gp[1]=s_.gp[0];','case P::second_reduction_endpoint:'),
        ]:
            source=path.read_text()
            if source.count(old)!=1:raise ValueError('Boundary semantic mutation seam drift: '+name)
            overlay=scratch/name;header=overlay/relative;header.parent.mkdir(parents=True,exist_ok=True);changed=source.replace(old,new,1);header.write_text(changed);mutated=command.copy();mutated.insert(mutated.index('-I'+str(ROOT/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
            if built.returncode:raise ValueError('Boundary mutant compile failed '+name+':\n'+built.stderr[-5000:])
            probe=subprocess.run([mutated[-1]],capture_output=True,text=True)
            if not probe.returncode:raise ValueError('Boundary semantic mutant accepted: '+name)
            report['semantic_mutants'][name]=dict(rejected=True,changed_source_sha256=hashlib.sha256(changed.encode()).hexdigest(),diagnostic=probe.stderr[-3000:])
        report['passed']=True
    except Exception as error:report['error']=str(error)
    report['sources_unchanged']=all(sha(ROOT/n)==value for n,value in pins.items())and all(hashlib.sha256(body.encode()).hexdigest()==PINS[name]for name,body in owning_bodies().items())
    if not report['sources_unchanged']:report.update(passed=False,error='Boundary proof inputs changed')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in('passed','error')if k in report}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
