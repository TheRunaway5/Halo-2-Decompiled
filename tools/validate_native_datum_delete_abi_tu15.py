#!/usr/bin/env python3
"""Qualify shared16E8A0 DATA and full retained ABI against original hardware."""
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
    if (seed/'capstone').is_dir():sys.path.insert(0,str(seed));break
import validate_native_array_growth_tu15 as reference
from validate_native_physical_allocation_abi_tu15 import physical_inputs
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body
from native_datum_delete_replacements import install_native_datum_delete,prefix,canonical_body

IMAGE=(146,'23b798b8683109fa7356d9ad3286103f8a10c6b359d4913263b3565af7392996')
COPY_ROUTES={
    'ai':('src/campaign/ai/source_ai_frame_tu15_generated.cpp','16e8a0'),
    'effect':('src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp','16e8a0'),
    'pool':('src/physics/physics_pool_startup_runtime_generated.cpp','16d4c0'),
}


def copy_routes(scratch,require_binding=False):
    bodies=[];qualified={};drift_checks=0
    for route,(path,entry)in COPY_ROUTES.items():
        source=(ROOT/path).read_text();bound=install_native_datum_delete(source)
        if require_binding and bound!=source:raise ValueError('Datum-delete production route is not installed '+path)
        assert install_native_datum_delete(bound)==bound
        current=leaf_body(bound,entry)[0];marker=prefix(route=='pool')
        assert marker in current
        # Count a real retained body entry after native admission declines.
        current=current.replace(marker,marker+'    ++translated_fallbacks;\n',1)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',bound,re.S)[0]
        bodies.append('namespace '+route+'_copy {\n'+resolver+'\n'+current+'\n}\n')
        qualified[route]=dict(path=path,body_sha256=hashlib.sha256(leaf_body(source,entry)[0].encode()).hexdigest(),
            resolver_sha256=hashlib.sha256(resolver.encode()).hexdigest(),store_site_domain='MapPack11 canonical labels'if route=='pool'else'actual TU1.5 labels')
        mutations=[source.replace(leaf_body(source,entry)[0],leaf_body(source,entry)[0].replace('s.reg(3,4,0)','s.reg(2,4,0)',1),1),
            source.replace(resolver,resolver.replace('switch(p){','switch(p){/* drift */',1),1)]
        for changed in mutations:
            try:install_native_datum_delete(changed)
            except ValueError:drift_checks+=1
            else:raise AssertionError('Datum-delete mutated body/resolver admitted '+route)
    (scratch/'datum_delete_copy_routes.inc').write_text('\n'.join(bodies))
    return qualified,drift_checks


def recover(image,scratch):
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x16e8a0);addresses={}
    assert hashlib.sha256(image.read(0x16e8a0,IMAGE[0])).hexdigest()==IMAGE[1]and len(body.code)==62
    for pc in body.code:
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for op in ins.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={'0016e8a0':0x16e8a0},body_sites={'0016e8a0':sorted(body.code)},
        instruction_map={f'{pc:08x}':pc for pc in body.code},address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=0x16e8a0,tables=[],byte_tables=[])])
    pairing=scratch/'delete-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'delete-reference.cpp',manifest=scratch/'delete-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);pure=canonical_body(leaf_body(args.output.read_text(),'16e8a0')[0]);start=pure.index('{\n')+2
    assert hashlib.sha256(pure.encode()).hexdigest()=='355465eabab3185888c1005836682fb0757c6708268888cf4f89c1e5eed2becd'
    (scratch/'datum_delete_reference.inc').write_text(pure+'\n')
    bound=pure[:start]+'    if(run_delete(s))return;\n    ++translated_fallbacks;\n'+pure[start:]
    (scratch/'datum_delete_body.inc').write_text(bound+'\n');return body


def cases():
    rows=[];pool=0x1017d070
    for alignment in range(16):
        rows.append(dict(name=f'genuine_preallocated_delete_{alignment}',entry=0x16e8a0,arguments=[],receiver=0x486080,header=reference.HEADER,
            prepare_datum_pool=pool,full_abi=True,stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=[0x16e8a0],cohort='genuine-original-preallocated-row',
            input_registers=dict(edx=pool,ebx=0x87654321,ebp=0x12345678,esi=0x11223344,edi=0x99887766)))
    for stride in (0,1,3,4,5,7,16,24,31):
        for index,extent,cursor in ((0,1,0xffffffff),(1,3,3),(2,3,0),(31,33,0x7fffffff),(32,33,33)):
            for fill in (0,8):
                alignment=(stride+index+fill)%16;owned=reference.UI+0x100
                words=[(owned+0x24,stride),(owned+0x2a,fill),(owned+0x34,cursor),(owned+0x38,extent),(owned+0x3c,extent),
                    (owned+0x44,reference.UI+0x2000),(owned+0x48,reference.UI+0x800),
                    (reference.UI+0x800,0xffffffff),(reference.UI+0x804,0xffffffff)]
                # Declare only live/empty row salts, not deletion outcomes.
                for slot in range(extent):words.append((reference.UI+0x2000+slot*stride,0 if slot%3==0 else 0x8000+slot))
                rows.append(dict(name=f'declared_delete_{stride}_{index}_{fill}',entry=0x16e8a0,arguments=[],receiver=0x486080,header=reference.HEADER,
                    full_abi=True,stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=[0x16e8a0],cohort='declared-owned-descriptor-and-row-inputs',input_words=words,
                    input_registers=dict(eax=0xabcd0000|index,edx=owned,ebx=0x87654321,ebp=0x12345678,esi=0x11223344,edi=0x99887766)))
    for alignment in (0,1,7,15):
        for offset in (4,8,12,16):
            for fill in (0,8):
                owned=reference.UI+0x100;sp=reference.SP+alignment
                rows.append(dict(name=f'saved_delete_{offset}_{fill}_{alignment}',entry=0x16e8a0,arguments=[],receiver=0x486080,header=reference.HEADER,
                    full_abi=True,stack_pointer=sp,direction_flag=bool(alignment&1),abi_entries=[0x16e8a0],cohort='declared-saved-register-row-alias',scalar_compatible=False,
                    input_words=[(owned+0x24,4),(owned+0x2a,fill),(owned+0x34,1),(owned+0x38,1),(owned+0x3c,1),(owned+0x44,sp-offset),(owned+0x48,reference.UI+0x800),(reference.UI+0x800,1)],
                    input_registers=dict(eax=0xabcd0000,edx=owned,ebx=0x87654321,ebp=0x12345678,esi=0x11223344,edi=0x99887766)))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--binding-copies',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,scratch_directory=str(scratch),production_bound=False,retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);body=recover(image,scratch);banks,provenance=physical_inputs(image)
        copies,drift_checks=copy_routes(scratch,args.require_binding)if(args.binding_copies or args.require_binding)else({},0)
        binding=None
        if args.require_binding:
            from specialize_native_operands import specialize,DATUM_DELETE_ACTUAL_OWNER_IMAGES
            assert DATUM_DELETE_ACTUAL_OWNER_IMAGES=={0x16e8a0:IMAGE}
            path=ROOT/'src/physics/physics_live_support_tu15_generated.cpp';source=path.read_text();specialized,receipt=specialize(source)
            assert receipt['native_datum_delete_actual_owner_entries']==['16e8a0']
            context=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
            for changed in(context.replace('switch(target)','switch(target+0u)',1),context.replace('s.required(site,target,tail)','s.required(site,target^1u,tail)',1)):
                assert changed!=context
                try:specialize(source.replace(context,changed))
                except ValueError as error:assert str(error)=='Complete13FE50 dispatcher drift'
                else:raise AssertionError('Changed complete dispatcher escaped bound page-owner authentication')
            binding=dict(canonical_sha256=reference.sha(path),specialized_sha256=hashlib.sha256(specialized.encode()).hexdigest(),
                actual_entries=receipt['native_datum_delete_actual_owner_entries'],complete_dispatch_drift_rejections=2)
        rows=cases();reference.OWNERS=(0x16e8a0,);directory,authority=reference.original(image,banks,rows,scratch)
        fixture=directory/'delete-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'datum-delete';command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
            '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/'tests/native_tu15_datum_delete_abi_tests.cpp'),
            str(ROOT/'src/core/native_tu15_data_array_delete.cpp'),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if copies:command+=['-DHALO2_DATUM_DELETE_COPY_ROUTES']
        if args.require_binding:command+=['-DHALO2_DATUM_DELETE_OWNER_DISPATCH',*[str(ROOT/'src/physics'/name)for name in(
            'native_tu15_array_allocator.cpp','native_tu15_stack_arena.cpp','native_tu15_main_heap_release.cpp','native_tu15_main_heap_allocate.cpp','native_tu15_raw_allocator.cpp','native_tu15_physical_epoch.cpp')]]
        subprocess.run(command,check=True);modes={};routes=('candidate',*copies,*(('owner',)if args.require_binding else()))
        for route in routes:
            for mode in(('native','reference','raw-isolation','tick-isolation','epoch-isolation')if route=='owner'else('native','reference')):
                env=dict(os.environ)
                for key in ('HALO2_REFERENCE_DATUM_DELETE','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES','HALO2_REFERENCE_RAW_ALLOCATOR','HALO2_REFERENCE_TICK_CONVERSION','HALO2_REFERENCE_PHYSICAL_EPOCHS'):env.pop(key,None)
                if mode=='reference':env['HALO2_REFERENCE_DATUM_DELETE']='1'
                if mode=='raw-isolation':env['HALO2_REFERENCE_RAW_ALLOCATOR']='1'
                if mode=='tick-isolation':env['HALO2_REFERENCE_TICK_CONVERSION']='1'
                if mode=='epoch-isolation':env['HALO2_REFERENCE_PHYSICAL_EPOCHS']='1'
                print(json.dumps(dict(comparing_route=route,control=mode)),flush=True)
                result=subprocess.run([str(binary),str(fixture),route],env=env,text=True,capture_output=True,timeout=120)
                if result.returncode:report['comparison_errors']=result.stderr[-6000:];sys.stderr.write(result.stderr);result.check_returncode()
                modes[mode if route=='candidate'else route+'_'+mode]=json.loads(result.stdout)
        executed={pc for row in rows for pc in row['executed_sites']};files=[ROOT/'include/halo2/native_tu15_datum_delete_kernel.hpp',ROOT/'src/core/native_tu15_data_array_delete.cpp',
            ROOT/'src/physics/runtime/native_tu15_datum_delete_abi.hpp',ROOT/'tests/native_tu15_datum_delete_abi_tests.cpp',Path(__file__),Path(reference.__file__),
            ROOT/'tools/observe_native_array_growth_tu15.cpp',Path(generator.__file__),ROOT/'tools/native_datum_delete_replacements.py']
        files += [ROOT/path for path,_ in COPY_ROUTES.values()]if copies else[]
        if args.require_binding:files += [ROOT/'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp',ROOT/'tools/specialize_native_operands.py',ROOT/'tools/generate_physics_pool_startup_tu15_20261012.py']
        report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
            production_bound=args.require_binding,actual_owner_binding=binding,
            allocator_outcomes_supplied_to_original=False,genuine_preallocation='Unchanged16E6A0 executes before capture; its computed handle remains in EAX.',
            body_coverage=dict(total=len(body.code),executed=len(executed&set(body.code))),
            generated_copy_routes=copies,binding_drift_checks=drift_checks,
            retained_memory_exception_protocol_qualified=all(mode['retained_faults']['memory_prefixes']>0 and mode['retained_faults']['observer_prefixes']>0 for mode in modes.values()),
            exception_qualification_scope='Pinned canonical observed/observer-free Memory and throwing record-observer protocols; full partial GP/flags/XMM/live80-bit/FP/access/store/bank/cursor state. Physical page/instruction fault atomicity is separate.',
            physical_instruction_page_fault_atomicity_qualified=False,
            address_undefined_sanitizers=args.sanitizers,source_sha256={str(p.relative_to(ROOT)):reference.sha(p)for p in files},recovered_reference_sha256=reference.sha(scratch/'datum_delete_reference.inc'))
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
