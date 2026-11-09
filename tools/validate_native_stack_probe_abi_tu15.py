#!/usr/bin/env python3
"""Qualify32A660 stack reservation against unchanged physical TU1.5 instructions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
for seed in sorted(ROOT.glob('.local/*/python')):
    if(seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body
from native_stack_probe_replacements import install_native_stack_probe,canonical_body,prefix,LITERAL_PIN

IMAGE=(65,'b77ad187f6f3db2a5ae4f8e214b023d7c62ce25a38ce769a993d2a8c81643760')
BOUNDARIES=(0x32a672,0x32a675,0x32a677,0x32a678,0x32a679,0x32a689,0x32a696,0x32a69a,0x32a69c,0x32a69f,0x32a6a0)
COPY_ROUTES={
    'ai':('src/campaign/ai/source_ai_frame_tu15_generated.cpp','32a660'),
    'shadow':('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp','32a660'),
    'player':('src/gameplay/units/source_player_departure_tu15_generated.cpp','32a660'),
    'vehicle':('src/physics/vehicles/physics_vehicle_runtime_generated.cpp','326af0')}


def copy_routes(scratch,require_binding):
    bodies=[];qualified={};drift_checks=0
    for route,(path,entry)in COPY_ROUTES.items():
        source=(ROOT/path).read_text();bound=install_native_stack_probe(source)
        if require_binding:assert bound==source,route+' stack-probe prefix missing'
        assert install_native_stack_probe(bound)==bound
        body=leaf_body(bound,entry)[0];raw=canonical_body(body,route=='vehicle')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source)[0]
        bodies.append(f'namespace {route}_copy {{\n'+resolver+'\n'+body.replace(prefix(route=='vehicle'),prefix(route=='vehicle')+'    ++stack_probe_fallbacks;\n',1)+'\n}\n')
        if route=='vehicle':bodies.append('namespace vehicle_reference {\n'+raw+'\n}\n')
        qualified[route]=dict(source=path,body_sha256=hashlib.sha256(body.encode()).hexdigest(),canonical_cursor_and_store_pc_offset=0x3b70 if route=='vehicle'else 0)
        changed=raw.replace('0x1000u','0x2000u',1);assert changed!=raw
        for mutated in(source.replace(leaf_body(source,entry)[0],changed,1),source.replace(resolver,resolver.replace('switch(p){','switch(p){/* drift */',1),1)):
            try:install_native_stack_probe(mutated)
            except ValueError:drift_checks+=1
            else:raise AssertionError('Stack-probe mutated body/resolver admitted '+route)
    (scratch/'stack_probe_copy_routes.inc').write_text('\n'.join(bodies));return qualified,drift_checks


def recover(image,scratch):
    assert hashlib.sha256(image.read(0x32a660,IMAGE[0])).hexdigest()==IMAGE[1]
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x32a660);assert len(body.code)==27
    addresses={}
    for pc in body.code:
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for op in ins.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={'0032a660':0x32a660},body_sites={'0032a660':sorted(body.code)},
        instruction_map={f'{pc:08x}':pc for pc in body.code},address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=0x32a660,tables=[],byte_tables=[])])
    pairing=scratch/'stack-probe-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'stack-probe-reference.cpp',manifest=scratch/'stack-probe-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);pure=canonical_body(leaf_body(args.output.read_text(),'32a660')[0]);assert hashlib.sha256(pure.encode()).hexdigest()==LITERAL_PIN
    (scratch/'stack_probe_reference.inc').write_text(pure+'\n');return body,pure


def cases():
    rows=[]
    def add(size,alignment,feedback=False):
        sp=reference.SP+alignment;words=[]
        # Small reservations can overlap the genuine return word. Preserve
        # it as input; the unchanged PUSH/RET must derive the relocated word.
        if size>=8:words.append((sp+4-size,0x80000001^(alignment*0x01010101)))
        if size>=4096 and size!=4096:words.append((sp+4-4096,0x7fffffff^(alignment*0x01010101)))
        row=dict(name=f'stack_{size}_{alignment}'+('_saved_feedback'if feedback else''),entry=0x32a660,arguments=[],receiver=0xc1c1c1c1,header=reference.HEADER,
            full_abi=True,stack_pointer=sp,direction_flag=bool(alignment&1),abi_entries=list(BOUNDARIES),input_words=words,
            fault_case=alignment==0,input_registers=dict(eax=size,edx=0xd2d2d2d2,ebx=0xb3b3b3b3,ebp=0xb5b5b5b5,esi=0x51515151,edi=0xd7d7d7d7))
        if feedback:row['mutations']=[dict(instruction=0x32a679,address=sp-4,written=(0x76543210+alignment).to_bytes(4,'little').hex())]
        rows.append(row)
    for size in(0,1,2,3,4,5,7,8,15,4095,4096,4097,8191,8192,0x1054,0x7fff):
        for alignment in(0,1,2,3,7,8,14,15):add(size,alignment)
    for alignment in(4,5,6,9,10,11,12,13):add(0x1054,alignment)
    for size in(4096,4097,0x1054,8192):
        for alignment in(0,1,2,3,7,8,14,15):add(size,alignment,True)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--binding-copies',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);body,pure=recover(image,scratch)
        copies,drift_checks=copy_routes(scratch,args.require_binding)if(args.binding_copies or args.require_binding)else({},0)
        binding=None
        if args.require_binding:
            from specialize_native_operands import specialize,STACK_PROBE_ACTUAL_OWNER_IMAGES
            assert STACK_PROBE_ACTUAL_OWNER_IMAGES=={0x32a660:IMAGE}
            source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();specialized,receipt=specialize(source)
            assert receipt['native_stack_probe_actual_owner_entries']==['32a660']
            context=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            for changed in(context.replace('switch(target)','switch(target+0u)',1),context.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
                assert changed!=context
                try:specialize(source.replace(context,changed))
                except ValueError as error:assert str(error)=='Complete13FE50 dispatcher drift'
                else:raise AssertionError('Changed complete dispatcher escaped bound page-owner authentication')
            binding=dict(actual_entries=receipt['native_stack_probe_actual_owner_entries'],canonical_sha256=hashlib.sha256(source.encode()).hexdigest(),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest(),complete_dispatch_drift_rejections=2)
        rows=cases();reference.OWNERS=(0x32a660,);directory,authority=reference.original(image,{},rows,scratch);fixture=directory/'stack-probe-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'stack-probe';sources=['tests/native_tu15_stack_probe_abi_tests.cpp']
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),*[str(ROOT/name)for name in sources],'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if copies:command+=['-DHALO2_STACK_PROBE_COPY_ROUTES']
        if args.require_binding:command+=['-DHALO2_STACK_PROBE_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in(
            'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')]]
        built=subprocess.run(command,text=True,capture_output=True);report['compiler_command']=command
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        modes={}
        for route in('candidate',*copies,*(('owner',)if args.require_binding else())):
            for mode in(('native','reference','raw-isolation','tick-isolation','epoch-isolation','delete-isolation')if route=='owner'else('native','reference')):
                env=dict(os.environ)
                for key in('HALO2_REFERENCE_PAGE_ALLOCATOR','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES','HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_PHYSICAL_EPOCHS','HALO2_REFERENCE_DATUM_DELETE'):env.pop(key,None)
                control={'reference':'HALO2_REFERENCE_PAGE_ALLOCATOR','raw-isolation':'HALO2_REFERENCE_RAW_ALLOCATOR','tick-isolation':'HALO2_REFERENCE_TICK_CONVERSION','epoch-isolation':'HALO2_REFERENCE_PHYSICAL_EPOCHS','delete-isolation':'HALO2_REFERENCE_DATUM_DELETE'}.get(mode)
                if control:env[control]='1'
                result=subprocess.run([str(binary),str(fixture),route],env=env,text=True,capture_output=True,timeout=120)
                if result.returncode:report['comparison_errors']=result.stderr[-6000:];sys.stderr.write(result.stderr);result.check_returncode()
                modes[mode if route=='candidate'else route+'_'+mode]=json.loads(result.stdout)
        files=sources+['include/halo2/native_tu15_stack_probe_kernel.hpp','src/physics/runtime/native_tu15_stack_probe_abi.hpp','tests/native_tu15_allocator_abi_fixture.hpp',str(Path(__file__).relative_to(ROOT)),
            'tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tools/native_stack_probe_replacements.py']
        if copies:files +=[path for path,_ in COPY_ROUTES.values()]
        if args.require_binding:files+=['src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp','tools/generate_vehicle_runtime_20261011.py']
        executed={pc for row in rows for pc in row['executed_sites']};report.update(passed=True,modes=modes,source_authority=authority,original_instructions_unchanged=True,
            body_coverage=dict(total=len(body.code),executed=len(set(body.code)&executed)),physical_pre_io_boundaries=[hex(pc)for pc in BOUNDARIES],
            canonical_reference_body_sha256=hashlib.sha256(pure.encode()).hexdigest(),stack_input_scope='Declared backed caller stacks; unchanged original computes every page probe, saved counter and relocated return word. No guard-page/platform success supplied.',
            production_bound=args.require_binding,actual_owner_binding=binding,generated_copy_routes=copies,binding_drift_checks=drift_checks,
            retained_memory_exception_protocol_qualified=True,physical_instruction_page_fault_atomicity_qualified=False,
            exception_qualification_scope='Canonical Memory faults, observed and observer-free, plus throwing store observers; full partial GP/full flags/XMM/live80-bit/FP/banks/cursor. Separate from physical instruction/page-fault atomicity.',
            address_undefined_sanitizers=args.sanitizers,source_sha256={name:reference.sha(ROOT/name)for name in files})
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
