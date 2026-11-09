#!/usr/bin/env python3
"""Qualify140310 shared DATA/full ABI with genuine datum and callback owners."""
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
from validate_native_physical_allocation_abi_tu15 import physical_inputs
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body
from native_datum_delete_replacements import canonical_body as canonical_datum
from native_page_retire_replacements import canonical_body as canonical_retire,install_native_page_retire,PREFIX

IMAGES={0x140310:(124,'0eb1dce8cccca3a9784e72d7ccaa2443cd0684d2f8a31fb30be844cb20e6eaf1'),
    0x12fd10:(238,'18996751545b0def5356655fb755661820244e2435c42945521fe0954fcaee27'),
    0x16e8a0:(146,'23b798b8683109fa7356d9ad3286103f8a10c6b359d4913263b3565af7392996')}
BOUNDARIES=(0x140310,0x140313,0x140320,0x140324,0x14032b,0x14032c,0x14032e,0x140336,0x140339,0x14033c,0x140347,
    0x14034d,0x140350,0x140353,0x14035b,0x14035e,0x140361,0x14036c,0x140370,0x140375,0x140376,0x14037b,0x14037e,0x140381,0x140386,0x140387)


def recover(image,scratch):
    code=reference._Code(image,'.text');bodies={p:reference.complete_body(code,p)for p in IMAGES};addresses={};sites={}
    for entry,(length,pin)in IMAGES.items():assert hashlib.sha256(image.read(entry,length)).hexdigest()==pin
    for body in bodies.values():
        for pc in body.code:
            ins=code.at(pc);sites[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
            for op in ins.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={f'{p:08x}':p for p in IMAGES},body_sites={f'{p:08x}':sorted(b.code)for p,b in bodies.items()},instruction_map=sites,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=p,tables=[],byte_tables=[])for p in IMAGES])
    pairing=scratch/'retire-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'retire-reference.cpp',manifest=scratch/'retire-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);source=args.output.read_text();declarations=''.join(f'static void run_{p:x}(WorldState &s);\n'for p in IMAGES);pure=declarations;production=declarations
    for p in IMAGES:
        b=leaf_body(source,f'{p:x}')[0]
        if p==0x16e8a0:b=canonical_datum(b)
        if p==0x140310:b=canonical_retire(b)
        start=b.index('{\n')+2;entry=f'    publish_entry(s,0x{p:x}u);\n'if p!=0x140310 else''
        pure+=b[:start]+entry+b[start:].replace('dispatch(s,','canonical_dispatch(s,')+'\n'
        extra='    ++parent_fallbacks;\n'if p==0x140310 else('    if(native_datum_delete_detail::run(s))return;\n'if p==0x16e8a0 else'')+f'    ++child_fallbacks[0x{p:x}u];\n'
        production+=b[:start]+entry+extra+b[start:].replace('dispatch(s,','production_dispatch(s,')+'\n'
    (scratch/'page_retire_reference.inc').write_text(pure);(scratch/'page_retire_production.inc').write_text(production);return bodies


def cases():
    rows=[]
    def add(name,alignment,owner,words,**extra):
        inputs=dict(edx=0xc3c3c3c3,ebp=0x87654321,esi=0x11223344,edi=owner);inputs.update(extra.pop('registers',{}))
        rows.append(dict(name=name,entry=0x140310,arguments=[],receiver=0x486080,header=reference.HEADER,full_abi=True,
            stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=[*BOUNDARIES,0x12fd10,0x16e8a0],parent_boundaries=list(BOUNDARIES),input_words=words,input_registers=inputs,**extra))
    for alignment in range(16):
        words=[(0x1017d0bc+12,0xffffffff),(0x1017d0bc+16,0xffffffff),(0x1017d03c,0xe2780000),(0x1017d040,0xe2780000)]
        common=dict(prepare_datum_pools=[0x10176000,0x1017d070],prepare_datum_to_ebx=True,scalar_compatible=True,cohort='genuine-original-preallocation-with-declared-list-links',fault_case=alignment==0)
        add(f'prepared_retire_{alignment}',alignment,0x1017d000,words,**common)
        if alignment in(0,1,7,15):
            boundary=dict(common,fault_case=False)
            add(f'prepared_callback_entry_{alignment}',alignment,0x1017d000,words,**boundary,required_entries=[0x12fd10])
            add(f'prepared_datum_entry_{alignment}',alignment,0x1017d000,[*words,(0x1017d020,0)],**boundary,required_entries=[0x16e8a0])
    def descriptor(owner,pool,records,previous,next,fill,stride=24,count=3,index=1):
        words=[(owner+0x20,0),(owner+0x64,pool),(owner+0x3c,1),(owner+0x40,1),(pool+0x24,stride),(pool+0x2a,fill),(pool+0x34,3),(pool+0x38,count),(pool+0x3c,count),(pool+0x44,records),(pool+0x48,reference.UI+0x800),(reference.UI+0x800,7)]
        for row in range(count):words.append((records+row*stride,0x8000+row))
        words.extend(((records+index*24+12,next),(records+index*24+16,previous)));return words
    for profile,(previous,next)in enumerate(((0xffffffff,0xffffffff),(0,2),(0xffffffff,2),(0,0xffffffff))):
        for stride in(0,24):
            for fill in(0,8):
                for alignment in(0,1,7,15):
                    owner,pool,records=reference.UI+0x100,reference.UI+0x200,reference.UI+0x2000
                    add(f'owned_retire_{profile}_{stride}_{fill}_{alignment}',alignment,owner,descriptor(owner,pool,records,previous,next,fill,stride),
                        registers=dict(eax=0x12345678,ebx=0xface0001),scalar_compatible=True,fault_case=alignment==0 and stride==24,cohort='declared-owned-list-and-descriptor-inputs')
    for alignment in(0,1,7,15):
        for offset in(20,24,28,32):
            for fill in(0,8):
                if offset==20 and fill:continue
                owner,pool,records=reference.UI+0x100,reference.UI+0x200,reference.SP+alignment-offset
                add(f'saved_retire_{offset}_{fill}_{alignment}',alignment,owner,descriptor(owner,pool,records,0xffffffff,0xffffffff,fill,count=1,index=0),
                    registers=dict(eax=0x12345678,ebx=0xface0000,esi=0xffffffff if offset==20 else 0x11223344),scalar_compatible=False,fault_case=alignment==0,cohort='declared-page-record-saved-frame-alias')
        owner,pool,records,obj=reference.UI+0x100,reference.UI+0x200,reference.UI+0x2000,reference.UI+0x500
        words=descriptor(owner,pool,records,0xffffffff,0xffffffff,0,count=1,index=0)
        words.extend(((owner+0x20,0x12fd10),(0x4f2d24,obj),(obj+0x24,40),(obj+0x2a,0),(obj+0x34,1),(obj+0x38,1),(obj+0x3c,1),(obj+0x44,reference.SP+alignment-24),(obj+0x48,reference.UI+0x900),(reference.UI+0x900,1),(reference.UI+12,0xffffffff),(reference.UI+16,0xffffffff)))
        add(f'callback_record_resume_alias_{alignment}',alignment,owner,words,registers=dict(eax=0x12345678,ebx=0),scalar_compatible=False,fault_case=alignment==0,cohort='declared-genuine-callback-saved-record-alias')
    source_faults={'prepared_retire_0','callback_record_resume_alias_0','saved_retire_24_8_0','owned_retire_1_24_8_0'}
    for row in rows:row['source_fault_case']=row['name']in source_faults
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);bodies=recover(image,scratch);banks,provenance=physical_inputs(image)
        binding=None
        if args.require_binding:
            from specialize_native_operands import specialize
            routes=[];binding={}
            for route,name in(('primary','src/physics/physics_live_support_tu15_generated.cpp'),('combat','src/gameplay/units/source_combat_action_tu15_generated.cpp')):
                source=(ROOT/name).read_text();assert install_native_page_retire(source)==source
                assert specialize(source)[1]['native_page_retire_entries']==['140310']
                function=leaf_body(source,'140310')[0];changed=function.replace('0x64u','0x68u',1);assert changed!=function
                for attempt in(install_native_page_retire,specialize):
                    try:attempt(source.replace(function,changed,1))
                    except ValueError:pass
                    else:raise AssertionError('Changed bound140310 body admitted')
                resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]if'selected('in function else''
                routes.append('namespace source_'+route+' {\n'+resolver+'\nstatic void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){production_dispatch(s,target,site,tail);}\n'+function.replace(PREFIX,PREFIX+'    ++parent_fallbacks;\n',1)+'\n}\n')
                binding[route]=dict(source=name,body_sha256=hashlib.sha256(function.encode()).hexdigest(),body_drift_rejections=2)
            (scratch/'page_retire_source_routes.inc').write_text('\n'.join(routes))
        assert reference.datum_preparation({})==''
        pool=0x1017d070;assert reference.datum_preparation(dict(prepare_datum_pool=pool))==f'mov ${reference.SP-0x100},%esp\n mov ${pool},%edx\n call 0x16e6a0\n'
        assert reference.datum_preparation(dict(prepare_datum_pools=[pool]))==reference.datum_preparation(dict(prepare_datum_pool=pool))
        assert reference.datum_preparation(dict(prepare_datum_pools=[0x10176000,pool])).count('call 0x16e6a0\n')==2
        rows=cases();reference.OWNERS=tuple(IMAGES);directory,authority=reference.original(image,banks,rows,scratch);fixture=directory/'page-retire-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        sources=['tests/native_tu15_page_retire_abi_tests.cpp','src/core/native_tu15_page_allocator.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp','src/campaign/ai/native_campaign_ai_resource_retirement_tu15.cpp'];binary=scratch/'page-retire'
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),*[str(ROOT/name)for name in sources],'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.require_binding:command+=['-DHALO2_PAGE_RETIRE_SOURCE_ROUTES']
        built=subprocess.run(command,text=True,capture_output=True);report['compiler_command']=command
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        modes={}
        for route in(('bridge','primary','combat')if args.require_binding else('bridge',)):
            for mode in('native','page-reference','datum-reference','reference-all'):
                env=dict(os.environ)
                for key in('HALO2_REFERENCE_PAGE_ALLOCATOR','HALO2_REFERENCE_DATUM_DELETE','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
                if mode in('page-reference','reference-all'):env['HALO2_REFERENCE_PAGE_ALLOCATOR']='1'
                if mode in('datum-reference','reference-all'):env['HALO2_REFERENCE_DATUM_DELETE']='1'
                print(json.dumps(dict(comparing_route=route,control=mode)),flush=True)
                result=subprocess.run([str(binary),str(fixture),*([route]if route!='bridge'else[])],env=env,text=True,capture_output=True,timeout=180)
                if result.returncode:report['comparison_errors']=result.stderr[-7000:];sys.stderr.write(result.stderr);result.check_returncode()
                modes[mode if route=='bridge'else route+'_'+mode]=json.loads(result.stdout)
        files=sources+['include/halo2/native_tu15_page_retire_kernel.hpp','src/physics/runtime/native_tu15_page_retire_abi.hpp','tests/native_tu15_allocator_abi_fixture.hpp',str(Path(__file__).relative_to(ROOT)),'tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp']
        files+=['tools/native_page_retire_replacements.py','tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py']
        if args.require_binding:files+=['src/physics/physics_live_support_tu15_generated.cpp','src/gameplay/units/source_combat_action_tu15_generated.cpp']
        executed={pc for row in rows for pc in row['executed_sites']};report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
            body_coverage={hex(p):dict(total=len(b.code),executed=len(set(b.code)&executed))for p,b in bodies.items()},
            production_bound=args.require_binding,source_binding=binding,
            source_route_scope='All normal returns/orderedI/O/banks/fullABI; source callback packets must match the original ordered subsequence. Declared-entry diagnostics use the retained trace route. Direct adapter covers all17 fault scenarios; source prefixes repeat four callback/list/saved-frame scenarios.',
            preallocation_body_bytes=reference.DATUM_PREPARATION_IMAGE,preparation_regression_checks=4,allocation_outcomes_supplied=False,
            real_callback_owner='12FD10 unchanged canonical body and actual-entry boundaries; associated device/fence paths remain open.',
            exception_qualification_scope='Canonical Memory/observer faults through parent, the real unassociated12FD10 path and native datum closure, including saved-record callback resume. Associated callback/fence/sync exceptions and physical instruction/page-fault atomicity remain separate/open.',
            retained_memory_exception_protocol_qualified=True,physical_instruction_page_fault_atomicity_qualified=False,address_undefined_sanitizers=args.sanitizers,
            source_sha256={name:reference.sha(ROOT/name)for name in files})
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
