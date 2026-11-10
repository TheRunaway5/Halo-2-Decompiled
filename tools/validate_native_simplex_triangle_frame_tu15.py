#!/usr/bin/env python3
"""Private complete actual30D3F0 triangle DATA policy and retained raw ABI."""
import argparse,hashlib,json,os,random,re,struct,subprocess
from pathlib import Path
from generated_routine_ownership import function_bodies
from validate_native_simplex_direction_tu15 import ROOT,sha
import native_simplex_triangle_replacements as triangle_binding
import native_simplex_triangle_scene_replacements as scene_binding
import specialize_native_operands as operand_spec
from fix_physics_world_observation_tu15 import observation_baseline
PINS={'physics_world_runtime_generated.cpp':'4e6518483279df6cc9969a6a52b0048995d21dcd3a1eb08e16e408ee1852db9b','physics_contact_runtime_generated.cpp':'bbc0adb751c6c4f44c2d3e19640c3ee7feca2e988c29d2bfe5e8103d15fc8141'}
def reference(scratch):
    bodies={}
    for name,pin in PINS.items():
        body=triangle_binding.canonical_body(next(b for sig,b in function_bodies((ROOT/'src/physics/runtime'/name).read_text())if sig.symbol=='309900'))
        if hashlib.sha256(body.encode()).hexdigest()!=pin:raise ValueError('Complete triangle owner drift')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',(ROOT/'src/physics/runtime'/name).read_text(),re.S)[0]
        resolver_pin='20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2'if 'world'in name else'd25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e'
        if hashlib.sha256(resolver.encode()).hexdigest()!=resolver_pin or'case 0x418ae0u:return 0x461960u;'not in resolver:raise ValueError('Complete triangle selected resolver drift')
        bodies[name]=body
    body=next(iter(bodies.values()));actual=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body).replace('run_309900(ObservedWorldState','child_unmodified(WorldState',1)
    corrected=actual.replace('child_unmodified(','child(',1).replace('s.push(','atomic_push(s,')
    needle='s.fp.push(ext(f(r(s.memory,(0x160u+s.gp[1]),4))));'
    if corrected.count(needle)!=1:raise ValueError('Triangle FLD primitive drift')
    corrected=corrected.replace(needle,'logical_load(s,r(s.memory,(0x160u+s.gp[1]),4));')
    needle='w(s.memory,s.observe,0x30d5afu,(0x164u+s.gp[1]),4,u(spill(s.fp.st(0))));s.fp.pop();'
    if corrected.count(needle)!=1:raise ValueError('Triangle FSTP primitive drift')
    corrected=corrected.replace(needle,'completed_fstp(s,0x30d5afu,0x164u+s.gp[1]);')
    primitive='''static void atomic_push(WorldState&s,std::uint32_t pc,std::uint32_t value){const auto address=s.gp[4]-4u;SourcePhysicsContactRecordStore e{pc,address,{},{}};if(s.observe){e.before.resize(4);s.memory.read(address,e.before);}std::array<std::byte,4>b{};std::memcpy(b.data(),&value,4);s.memory.write(address,b);s.gp[4]=address;if(s.observe){e.written.assign(b.begin(),b.end());s.observe(e);}}
struct FstpStore:native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_triangle_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>{using Base=native_mopp_ray_caller_detail::IntegerFrame<WorldState,native_simplex_triangle_detail::Noop,native_mopp_ray_caller_detail::ScopeIo<WorldState>>;using Base::Base;using Base::store;};
static void logical_load(WorldState&s,std::uint32_t bits){native_raw_x87_detail::Scope<WorldState>fp{s};fp.require_capacity(1u);asm volatile("flds %0"::"m"(bits):"memory","st");}
static void completed_fstp(WorldState&s,std::uint32_t pc,std::uint32_t address){native_raw_x87_detail::Scope<WorldState>fp{s};native_simplex_triangle_detail::Noop before;FstpStore memory{s,before,native_mopp_ray_caller_detail::ScopeIo<WorldState>{&fp}};const auto pending=fp.stage_single(true);memory.store(pc,address,pending.bits,[&]{fp.commit(pending);});}
static std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){if(p!=0x418ae0u)throw std::runtime_error("Unknown triangle mapping");return 0x461960u;}
'''
    canonical=bodies['physics_contact_runtime_generated.cpp'].replace('run_309900(State','child_canonical(WorldState',1)
    world=(ROOT/'src/physics/runtime/physics_world_runtime_generated.cpp').read_text();contact=(ROOT/'src/physics/runtime/physics_contact_runtime_generated.cpp').read_text()
    baseline=observation_baseline(triangle_binding.remove_simplex_triangle(world))[0]
    originals=[world,contact,baseline];bound=[triangle_binding.replace_simplex_triangle(v)for v in originals]
    variants=bound+[operand_spec.specialize(v)[0]for v in bound]
    source_bodies=[];source_receipts=[]
    for n,value in enumerate(variants):
        if triangle_binding.installed_entries(value)!=['30d3f0']:raise ValueError('Complete source-copy triangle not installed')
        entry=triangle_binding.BODY.search(value);body=entry[0]
        body=body.replace('run_309900(',f'bound_{n}(',1).replace('ObservedWorldState &s','ActualState &s',1).replace('State &s','WorldState &s',1)if entry[1]=='State' else body.replace('run_309900(',f'bound_{n}(',1).replace('ObservedWorldState &s','ActualState &s',1)
        body=re.sub(r'world_observation_pc\(0x[0-9a-f]+u,(0x[0-9a-f]+u),s\)',r'\1',body)
        at=body.index('{')+1+len(triangle_binding.PREFIX)
        body=body[:at]+f'\n++bound_literal_entries[{n}];'+body[at:]
        changes={
            'body':value[:entry.start()]+entry[0].replace('0x160u','0x170u',1)+value[entry.end():],
            'include':value.replace(triangle_binding.INCLUDE,'',1),
            'missing_body':value[:entry.start()]+value[entry.end():],
            'prefix':value.replace(triangle_binding.PREFIX,'',1),
        }
        owner=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',value,re.S)[0]
        changes['resolver']=value.replace(owner,owner.replace('0x461960u','0x461970u',1),1)
        dispatch_owner=re.search(r'static void dispatch\('+entry[1]+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',value)[0]
        changes['dispatcher']=value.replace(dispatch_owner,dispatch_owner.replace('bool tail){','bool tail){if(false)return;',1),1)
        rejections={}
        for control,changed in changes.items():
            if changed==value:raise ValueError('Triangle auth control seam drift: '+control)
            try:triangle_binding.installed_entries(changed)
            except ValueError as error:rejections[control]=str(error)
            else:raise ValueError('Triangle owner drift accepted: '+control)
        if n<3:
            if triangle_binding.replace_simplex_triangle(value)!=value or triangle_binding.replace_simplex_triangle(triangle_binding.remove_simplex_triangle(value))!=value:raise ValueError('Triangle source overlay not reversible/idempotent')
        expected_prepared=variants[n+3]if n<3 else value
        if operand_spec.specialize(value)[0]!=expected_prepared:raise ValueError('Triangle repeated preparation changed source')
        source_bodies.append(body);source_receipts.append(dict(variant=n,context_state=entry[1],complete_input_sha256=triangle_binding.digest(value),body_before_test_instrumentation_sha256=triangle_binding.digest(entry[0]),strict_context_controls=rejections))
    scene_source=(ROOT/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp').read_text()
    spill_helper=re.search(r'std::uint32_t native_single_spill\(long double x,std::uint16_t &status\) \{.*?\n\}',scene_source,re.S)[0]
    if hashlib.sha256(spill_helper.encode()).hexdigest()!='e7000a01d437d173c93ab95f59cfe991ba439b4db8cb86f014fc966a2730aad1':raise ValueError('Scene genuine spill helper drift')
    primitive+=spill_helper+'\n'
    scene_raw=scene_binding.replace_simplex_triangle_scene(scene_source)
    for n,value in enumerate((scene_raw,operand_spec.specialize(scene_raw)[0]),start=6):
        scene_binding.installed_entries(value);entry,resolver,dispatcher=scene_binding.context(value)
        body=entry[0].replace('run_30d3f0(',f'bound_{n}(',1).replace('selected(','scene_selected(')
        at=body.index('{')+1+len(scene_binding.PREFIX);body=body[:at]+f'\n++bound_literal_entries[{n}];'+body[at:]
        changes={'body':value[:entry.start()]+entry[0].replace('0x160u','0x170u',1)+value[entry.end():],
            'include':value.replace(scene_binding.INCLUDE,'',1),'prefix':value.replace(scene_binding.PREFIX,'',1),'missing_body':value[:entry.start()]+value[entry.end():],
            'resolver':value.replace(resolver,resolver.replace('0x461960u','0x461970u',1),1),
            'dispatcher':value.replace(dispatcher,dispatcher.replace('bool tail){','bool tail){if(false)return;',1),1)}
        controls={}
        for key,changed in changes.items():
            if changed==value:raise ValueError('Scene triangle mutation seam drift')
            try:scene_binding.installed_entries(changed)
            except ValueError as error:controls[key]=str(error)
            else:raise ValueError('Scene triangle context mutation accepted')
        if n==6 and scene_binding.replace_simplex_triangle_scene(value)!=value:raise ValueError('Scene triangle overlay not idempotent')
        if operand_spec.specialize(value)[0]!=(operand_spec.specialize(scene_raw)[0]if n==6 else value):raise ValueError('Scene repeated preparation drift')
        source_bodies.append(body);source_receipts.append(dict(variant=n,context_state='SceneWorldState_actual',complete_input_sha256=scene_binding.digest(value),body_before_test_instrumentation_sha256=scene_binding.digest(entry[0]),strict_context_controls=controls))
    dispatch='\nstatic void run_bound_actual(ActualState&s,unsigned variant){if(variant==0u)bound_0(s);else if(variant==3u)bound_3(s);else throw std::runtime_error("Wrong actual source variant");}\nstatic void run_bound_canonical(WorldState&s,unsigned variant){switch(variant){case 1:bound_1(s);break;case 2:bound_2(s);break;case 4:bound_4(s);break;case 5:bound_5(s);break;case 6:bound_6(s);break;case 7:bound_7(s);break;default:throw std::runtime_error("Wrong canonical source variant");}}\n'
    source=primitive+actual+'\n'+corrected+'\n'+canonical+'\nstatic std::uint32_t scene_selected(std::uint32_t p,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Wrong Scene identity image");return p;}\nstatic std::array<unsigned,8>bound_literal_entries{};\n'+'\n'.join(source_bodies)+dispatch
    (scratch/'simplex_triangle_frame_reference.inc').write_text(source)
    return dict(complete_owner_body_sha256=PINS,reference_sha256=hashlib.sha256(source.encode()).hexdigest(),primitive_sha256=hashlib.sha256(primitive.encode()).hexdigest(),bound_source_copies=source_receipts,test_instrumentation='One literal-body-entry counter placed after each authenticated native prefix; no original i386 code patched',normal_reference='Unmodified complete source body',prefix_reference='Explicit atomic PUSH and native FLD/staged FSTP primitives',production_bound=False),actual
def hardware(args,scratch,binary,block):
    from validate_native_tu15_prt_helpers import physical_original,PIN
    from oracle_native_campaign_screen_lifecycle_tu15 import _Code
    from research_mp_sandbox_update_functions import Xbe
    if sha(args.xbe)!=PIN:raise ValueError('Wrong original TU1.5 XBE')
    image=Xbe(args.xbe);code=_Code(image,'.text');rng=random.Random(0x30d3f0);rows=[];requests=[];traces=[];all_sites=set()
    for line in block.splitlines():
        match=re.search(r'services.instruction\((0x[0-9a-f]+)u\)',line)
        if not match:continue
        pc=int(match[1],16);all_sites.add(pc)
        if re.search(r'\b(?:r|v|w|vw)\(s.memory',line)or's.push('in line or's.pop()'in line:traces.append(dict(entry=pc,arguments=0,gp=True,xmm=True,x87_delta=1 if pc in(0x30d5a3,0x30d5a9,0x30d5af)else 0))
    base,size,stack=0x61000000,0x8000,0x61007000
    section=next(s for s in image.sections if s.va<=0x461000 and 0x462000<=s.va+s.rsize)
    ro=image.data[section.raw+0x461000-section.va:section.raw+0x462000-section.va]
    finite=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    edges=[0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3f800000,0xbf800000]
    for k in range(256):
        bank=bytearray(rng.randbytes(size))
        def put(at,value):struct.pack_into('<I',bank,at-base,value&0xffffffff)
        gp=[rng.getrandbits(32)for _ in range(8)];gp[1]=base+0x100;gp[4]=stack+k%16
        point,vertices=base+0x1000,base+0x2000;aligned=(gp[4]-4)&~15
        if k%16==6:vertices=aligned-0x30
        if k%16==10:gp[1]=aligned-0x180
        for p in (point,vertices,gp[1]):
            for row in range(24):
                for lane in range(4):put(p+row*16+lane*4,finite((rng.randrange(63)-31)/8)if k<128 else edges[(k+row*3+lane*5)%len(edges)])
        put(gp[4],0x63000000);put(gp[4]+4,point);put(gp[4]+8,vertices);put(gp[4]+12,0 if k%5==0 else 1)
        flags=0x202|(rng.getrandbits(12)&0xcd5);cw=(0x7f,0x27f,0x37f)[k%3]|((k%4)<<10);status=0x4100|((k%8)<<11)
        mxcsr=0x1f80|((k%4)<<13)|(0x8040 if k&4 else 0)|(k&0x3f);depth=(0,1,4,7)[k%4]
        values=('0000000000000080ff3f','35c26821a2da0fc90040','00000000000000000080','45230100000000c0ff7f');live=[values[n%4]for n in range(depth)]
        xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)]
        rows.append(dict(id=k,name=f'triangle-{k}',entry=0x30d3f0,gp=gp,eflags=flags,matrix=point,source=vertices,
            before_fp=[cw,status,mxcsr],live_x87=live,xmm=xmm,max_steps=500,banks=[dict(address=base,before=bank.hex()),dict(address=0x461000,before=ro.hex())]))
        requests.append((gp,flags,mxcsr,cw,status,live,xmm,bank))
    observed,proof=physical_original(image,rows,scratch,entries=(0x30d3f0,),regions=((base,size),(0x461000,0x1000)),trace_calls=traces,trace_extended=True,preserve_x87_prefix=True)
    inputs,expected,actual=(scratch/name for name in ('features.in','features.expected','features.actual'));segments=[];packets=0;executed=set();vector_ops={};af_packets=[]
    def emit_abi(out,gp,flags,fp,xmm,live):
        out.write(struct.pack('<13I',*gp,flags,fp[2],fp[1],fp[0],len(live)))
        for value in live:out.write(bytes.fromhex(value))
        for vector in xmm:out.write(struct.pack('<4I',*vector))
    with inputs.open('wb')as inp,expected.open('wb')as out:
        inp.write(struct.pack('<I',len(rows)))
        for case,request in zip(observed['cases'],requests,strict=True):
            gp,flags,mxcsr,cw,status,live,xmm,bank=request;name=rows[case['id']]['name']
            if not case['completed']or not case['live_x87_unchanged']:raise ValueError('Original triangle owner did not return: '+name)
            inp.write(struct.pack('<13I',*gp,flags,mxcsr,cw,status,len(live)))
            for value in live:inp.write(bytes.fromhex(value))
            for vector in xmm:inp.write(struct.pack('<4I',*vector))
            inp.write(bank);inp.write(ro);segments.append((out.tell(),name,'normal_abi'))
            emit_abi(out,case['boundary_gp'],case['boundary_eflags'],case['after_fp'],case['boundary_xmm'],live)
            out.write(bytes.fromhex(case['banks'][0]['after']));out.write(bytes.fromhex(case['banks'][1]['after']));events=[]
            for event in case['writes']:
                raw=bytes.fromhex(event['written']);before=bytes.fromhex(event['before']);pc=event['instruction'];address=event['address']
                if len(raw)==16:
                    instruction=code.at(pc)
                    if instruction.mnemonic!='movaps'or instruction.bytes[:2]!=b'\x0f\x29':raise ValueError('Polygon feature vector-store opcode drift')
                    vector_ops[f'{pc:x}']=instruction.bytes.hex();events.extend((pc,address+at,before[at:at+8],raw[at:at+8])for at in (0,8))
                else:events.append((pc,address,before,raw))
            out.write(struct.pack('<I',len(events)))
            for pc,address,before,raw in events:out.write(struct.pack('<III',pc,address,len(raw)));out.write(before);out.write(raw)
            retpc=0x30d5b9 if any(w['instruction']==0x30d5af for w in case['writes'])else 0x30d5c2
            out.write(struct.pack('<2I',retpc,len(case['calls'])))
            for packet in case['calls']:
                segments.append((out.tell(),name,f'pre_io_{packet["entry"]:x}'));out.write(struct.pack('<I',packet['entry']));emit_abi(out,packet['gp'],packet['eflags'],packet['fp'],packet['xmm'],packet['live_x87']);packets+=1
                if packet['entry']==0x1f66ba and packet['eflags']&0x10 and len(af_packets)<4:af_packets.append(dict(case=name,gp=packet['gp'],eflags=packet['eflags'],instruction=packet['entry']))
            executed.update(case['executed_sites'])
    with inputs.open('rb')as inp,actual.open('wb')as out:r=subprocess.run([str(binary),'--fixture'],stdin=inp,stdout=out,stderr=subprocess.PIPE)
    if r.returncode:raise ValueError('Native physical triangle failed: '+r.stderr.decode()[-5000:])
    a=expected.read_bytes();b=actual.read_bytes()
    if a!=b:
        offset=next((n for n,(x,y)in enumerate(zip(a,b))if x!=y),min(len(a),len(b)));segment=next(row for row in reversed(segments)if row[0]<=offset)
        raise ValueError(f'Unchanged-i386 triangles differs byte{offset},segment{segment},relative{offset-segment[0]},expected{a[offset:offset+32].hex()},actual{b[offset:offset+32].hex()}')
    if executed!=all_sites:raise ValueError('Physical triangle coverage incomplete: '+','.join(f'{p:x}'for p in sorted(all_sites-executed)))
    return dict(cases=len(rows),source_sites=len(executed),full_pre_io_gp_flags_xmm_fp_live80_packets=packets,complete_normal_abi_banks_store_operands=True,
        physical_vector_opcodes=vector_ops,runtime_vector_protocol=[8,8],physical_fault_atomicity=False,
        comparison_sha256=hashlib.sha256(b).hexdigest(),original=proof,declared_algorithm_inputs=True,original_text_unmodified=True)

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--compiler',default='c++');p.add_argument('--sanitizers',action='store_true');p.add_argument('--xbe',type=Path);p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if args.xbe is not None and not args.xbe.is_file():return 77
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_owner=str(scratch),retained_large_artifacts=[])
    paths=['include/halo2/native_tu15_simplex_triangle_kernel.hpp','src/physics/runtime/native_tu15_simplex_triangle_abi.hpp','tests/native_tu15_simplex_triangle_frame_tests.cpp','tests/reference/native_tu15_simplex_triangle_value_before.cpp','tests/reference/native_tu15_simplex_triangle_shared_candidate.cpp','src/physics/native_tu15_simplex_triangle.cpp','src/physics/native_tu15_simplex_helpers_handoff.cpp','src/physics/native_tu15_closest_segments.cpp','tools/native_simplex_triangle_replacements.py','tools/native_simplex_triangle_scene_replacements.py','tools/specialize_native_operands.py','tools/fix_physics_world_observation_tu15.py','tools/native_polygon_prepared_owners.py','include/halo2/native_tu15_simplex_triangle_value.hpp',str(Path(__file__).relative_to(ROOT))]
    pending=list(paths);seen=set(paths)
    while pending:
        current=ROOT/pending.pop()
        for header in re.findall(r'^#include "([^"\n]+)"',current.read_text(),re.M):
            target=next((v for v in(current.parent/header,ROOT/'include'/header,ROOT/'src'/header)if v.is_file()),None)
            if target is None:continue
            relative=str(target.relative_to(ROOT))
            if relative not in seen:seen.add(relative);pending.append(relative)
    paths=sorted(seen);pins={name:sha(ROOT/name)for name in paths}
    if pins['tests/reference/native_tu15_simplex_triangle_value_before.cpp']!='b3b71640bc63ce672441be056e5664c7cc71742b31ac845af3b59544456a00f1':raise ValueError('Frozen previous triangle DATA source drift')
    try:
        if (ROOT/'src/physics/native_tu15_simplex_triangle.cpp').read_bytes()!=(ROOT/'tests/reference/native_tu15_simplex_triangle_shared_candidate.cpp').read_bytes():raise ValueError('Qualified DATA candidate publication drift')
        if args.require_binding:
            for name in PINS:
                source=(ROOT/'src/physics/runtime'/name).read_text()
                if triangle_binding.replace_simplex_triangle(source)!=source or triangle_binding.installed_entries(source)!=['30d3f0']:raise ValueError('Complete production triangle source binding absent')
            source=(ROOT/'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp').read_text()
            if scene_binding.replace_simplex_triangle_scene(source)!=source or scene_binding.installed_entries(source)!=['30d3f0']:raise ValueError('Complete production Scene triangle binding absent')
            report['production_bound']=True
            report['production_source_copies']=3
        report['reference'],block=reference(scratch);binary=scratch/'triangle';command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_simplex_triangle_frame_tests.cpp'),str(ROOT/'src/physics/native_tu15_simplex_triangle.cpp'),str(ROOT/'src/physics/native_tu15_simplex_helpers_handoff.cpp'),str(ROOT/'src/physics/native_tu15_closest_segments.cpp'),'-o',str(binary)]
        if args.sanitizers:command[1:1]=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        report['command']=command;built=subprocess.run(command,capture_output=True,text=True)
        if built.returncode:raise ValueError('Triangle fixture compile failed:\n'+built.stderr[-10000:])
        result=subprocess.run([str(binary)],capture_output=True,text=True);report['diagnostic']=result.stderr[-12000:]
        if result.returncode:raise ValueError('Triangle fixture failed:\n'+report['diagnostic'])
        report['canonical']=json.loads(result.stderr.strip().splitlines()[-1])
        value=subprocess.run([str(binary),'--value'],capture_output=True,text=True)
        if value.returncode:raise ValueError('Shared triangle DATA value proof failed:\n'+value.stderr[-5000:])
        report['value']=json.loads(value.stderr.strip().splitlines()[-1])
        environment=os.environ.copy();environment.pop('HALO2_REFERENCE_SIMPLEX_TRIANGLE',None)
        guard=subprocess.run([str(binary),'--refusals'],capture_output=True,text=True,env=environment)
        if guard.returncode:raise ValueError('Triangle admission guard failed:\n'+guard.stderr[-5000:])
        report['refusals']=json.loads(guard.stderr.strip().splitlines()[-1])
        report['routes']={}
        for mode in('default','reference','unrelated-controls'):
            environment=os.environ.copy()
            if mode=='reference':environment['HALO2_REFERENCE_SIMPLEX_TRIANGLE']='1'
            else:environment.pop('HALO2_REFERENCE_SIMPLEX_TRIANGLE',None)
            if mode=='unrelated-controls':
                for name in('HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_POLYGON_SUPPORT','HALO2_REFERENCE_POLYGON_FEATURES'):environment[name]='1'
            routes=subprocess.run([str(binary),'--routes'],capture_output=True,text=True,env=environment)
            if routes.returncode:raise ValueError('Triangle bridge '+mode+' failed:\n'+routes.stderr[-5000:])
            report['routes'][mode]=json.loads(routes.stderr.strip().splitlines()[-1])
            handoff=subprocess.run([str(binary),'--handoff'],capture_output=True,text=True,env=environment)
            if handoff.returncode:raise ValueError('Triangle actual World handoff '+mode+' failed:\n'+handoff.stderr[-5000:])
            report.setdefault('world_handoff',{})[mode]=json.loads(handoff.stderr.strip().splitlines()[-1])
            copies=subprocess.run([str(binary),'--source-routes'],capture_output=True,text=True,env=environment)
            if copies.returncode:raise ValueError('Triangle bound source '+mode+' failed:\n'+copies.stderr[-5000:])
            report.setdefault('source_routes',{})[mode]=json.loads(copies.stderr.strip().splitlines()[-1])
        if args.xbe:report['hardware']=hardware(args,scratch,binary,block)
        text=(ROOT/'include/halo2/native_tu15_simplex_triangle_kernel.hpp').read_text();report['semantic_mutants']={}
        changes={
            'wrong_mask':('std::uint32_t(_mm_movemask_ps(d.weights))&7u','std::uint32_t(_mm_movemask_ps(d.weights))&3u'),
            'skip_orientation':('if(!(_mm_movemask_ps(d.dot)&1u))','if(true)'),
            'reuse_stale_scratch':('d.scratch=memory.vector_read(P::scratch_read,d,memory.scratch_address(d));','d.scratch=d.ac_zxy;'),
        }
        for name,(needle,replacement)in changes.items():
            if text.count(needle)!=1:raise ValueError('Triangle mutant seam drift: '+name)
            directory=scratch/name;(directory/'halo2').mkdir(parents=True);variant=text.replace(needle,replacement);(directory/'halo2/native_tu15_simplex_triangle_kernel.hpp').write_text(variant)
            changed=command.copy();at=changed.index('-I'+str(ROOT/'include'));changed[at:at]=['-I'+str(directory)];changed[-1]=str(directory/'mutant')
            built=subprocess.run(changed,capture_output=True,text=True)
            if built.returncode:raise ValueError('Triangle mutant compile failed: '+built.stderr[-5000:])
            rejected=subprocess.run([str(directory/'mutant')],capture_output=True,text=True)
            if rejected.returncode==0 or'Native simplex triangle mismatch'not in rejected.stderr:raise ValueError('Triangle mutant not rejected: '+name+'\n'+rejected.stderr[-5000:])
            report['semantic_mutants'][name]=dict(rejected=True,sha256=hashlib.sha256(variant.encode()).hexdigest(),diagnostic=rejected.stderr[-1000:])
        reference(scratch)
        report['sources_unchanged']=pins=={name:sha(ROOT/name)for name in paths}
        if not report['sources_unchanged']:raise ValueError('Triangle inputs changed')
        report['passed']=True
    except Exception as error:report['error']=str(error);raise
    finally:report['source_sha256']=pins;args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(report['diagnostic'].strip());return 0
if __name__=='__main__':raise SystemExit(main())
