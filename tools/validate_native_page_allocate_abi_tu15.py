#!/usr/bin/env python3
"""Private13FE50 full ABI proof; no production admission or parent binding."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import random
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
import validate_native_page_allocate_data_tu15 as data
from specialize_native_operands import leaf_body
from native_stack_probe_replacements import canonical_body as stack_body,authenticate as authenticate_stack
from native_page_window_replacements import canonical_body as window_body,BODY_PIN as WINDOW_PIN
from native_page_retire_replacements import canonical_body as retire_body,BODY_PINS as RETIRE_PINS
from native_datum_delete_replacements import canonical_body as delete_body,BODY_PINS as DELETE_PINS,RESOLVER_PINS as DELETE_RESOLVERS
from native_effect_datum_allocate_replacements import canonical_body as allocate_body,PINS as ALLOCATE_PINS
from native_resource_busy_replacements import canonical_body as busy_body,PINS as BUSY_PINS,RESOLVER_PINS as BUSY_RESOLVERS
from native_page_allocate_replacements import install_native_page_allocate,installed_entries as page_entries,PREFIX as PAGE_PREFIX,body as page_body,canonical_body as canonical_page_body,dispatcher as page_dispatcher,dispatch_context as page_dispatch_context


def copy_routes(scratch,require_binding):
    from specialize_native_operands import specialize
    output=[];proof={};rejections=0
    for route,path in(('primary','src/physics/physics_live_support_tu15_generated.cpp'),('shadow','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp')):
        source=(ROOT/path).read_text();bound=install_native_page_allocate(source)
        if require_binding:assert bound==source,route+' page allocation default prefix missing'
        assert install_native_page_allocate(bound)==bound and page_entries(bound)==['13fe50']
        original=page_body(bound)[0];changed=original.replace('0x1054u','0x1058u',1);assert changed!=original
        for candidate in(bound.replace(original,changed,1),bound+'\n'+original):
            try:install_native_page_allocate(candidate)
            except ValueError:rejections+=1
            else:raise AssertionError('Changed/duplicate complete page owner admitted')
        dispatch=page_dispatcher(bound);changed_dispatch=dispatch.replace('run_13fe50(s);','run_13fe00(s);',1);assert changed_dispatch!=dispatch
        try:install_native_page_allocate(bound.replace(dispatch,changed_dispatch,1))
        except ValueError:rejections+=1
        else:raise AssertionError('Changed complete page dispatcher admitted')
        if route=='shadow':
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',bound,re.S)[0]
            try:install_native_page_allocate(bound.replace(resolver,resolver.replace('switch(p){','switch(p){/* drift */',1),1))
            except ValueError:rejections+=1
            else:raise AssertionError('Changed whole page image resolver admitted')
        for specialized in(False,True):
            current=specialize(bound)[0]if specialized else bound;assert page_entries(current)==['13fe50']
            name=route+('_specialized'if specialized else'');function=page_body(current)[0]
            resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',current,re.S)[0]if route=='shadow'else''
            block='namespace source_'+name+' {\n'+resolver+'\nusing page_allocate_reference::sign_extend;\n'
            block+='static void dispatch(WorldState &s,std::uint32_t owner,std::uint32_t site,bool tail){page_reference_dispatch(s,owner,site,tail);}\n'
            block+=''.join(f'static void run_{entry:x}(WorldState &s){{page_reference_dispatch(s,0x{entry:x}u,0u,false);}}\n'for entry in(0x32a660,0x13fe00,0x140310,0x16e6a0))
            block+=function.replace(PAGE_PREFIX,PAGE_PREFIX+'    ++source_parent_fallbacks;\n',1)+'\n}\n';output.append(block)
            proof[name]=dict(source=path,body_sha256=hashlib.sha256(function.encode()).hexdigest(),dispatcher=page_dispatch_context(current,specialized),production_bound=bound==source)
    (scratch/'page_allocate_copy_routes.inc').write_text('\n'.join(output));return proof,rejections


def recover(scratch):
    primary='src/physics/physics_live_support_tu15_generated.cpp'
    paths={0x32a660:'src/campaign/ai/source_ai_frame_tu15_generated.cpp',0x16e8a0:'src/campaign/ai/source_ai_frame_tu15_generated.cpp',0x12fcb0:'src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'}
    functions=[];pins={}
    for entry in data.OWNERS:
        source=(ROOT/paths.get(entry,primary)).read_text();body=leaf_body(source,f'{entry:x}')[0]
        if entry==0x13fe50:body=canonical_page_body(body)
        elif entry==0x32a660:body=stack_body(body);authenticate_stack(source,body,False)
        elif entry==0x13fe00:body=window_body(body)
        elif entry==0x140310:body=retire_body(body)
        elif entry==0x16e8a0:body=delete_body(body)
        elif entry==0x16e6a0:body=allocate_body(body)
        elif entry==0x12fcb0:body=busy_body(body,'12fcb0')
        pins[f'{entry:x}']=hashlib.sha256(body.encode()).hexdigest()
        assert 'native_'not in body
        allowed={0x13fe50:('9f7baf1b966181785b74d8cb4c1b82fc501beff0aed6ccc98a430240e7bbf5cf',),0x13fe00:(WINDOW_PIN,),0x140310:RETIRE_PINS,0x16e6a0:ALLOCATE_PINS,0x16e8a0:DELETE_PINS,0x12fcb0:BUSY_PINS['12fcb0']}
        if entry!=0x32a660:assert pins[f'{entry:x}']in allowed[entry]
        if 'selected('in body:
            from native_selected_operand_binding import require_selected_identity
            from native_stack_probe_replacements import RESOLVER_PINS as STACK_RESOLVERS
            require_selected_identity(source,body,STACK_RESOLVERS if entry==0x32a660 else DELETE_RESOLVERS if entry==0x16e8a0 else BUSY_RESOLVERS)
            body=re.sub(r'selected\((0x[0-9a-f]+u),s.image\)',r'\1',body)
        start=body.index('{\n')+2
        functions.append(body[:start]+f'    if(publish_entry)publish_entry(s,0x{entry:x}u);\n'+body[start:].replace('dispatch(s,','page_reference_dispatch(s,').replace('run_4057f0(s);','page_reference_dispatch(s,0x4057f0u,0x12fcf5u,false);'))
    assert pins['13fe50']=='9f7baf1b966181785b74d8cb4c1b82fc501beff0aed6ccc98a430240e7bbf5cf'
    helper='std::uint32_t sign_extend(std::uint32_t x,unsigned n){return n==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(x)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(x))));}\n'
    text=helper+''.join(f'static void run_{entry:x}(WorldState &s);\n'for entry in data.OWNERS)+'\n'.join(functions)
    (scratch/'page_allocate_reference.inc').write_text(text);return pins


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--limit',type=int);p.add_argument('--parent-io',action='store_true');p.add_argument('--canonical-io-diagnostic',action='store_true');p.add_argument('--faults',action='store_true');p.add_argument('--copy-routes',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        reference=data.reference;assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT)
        source='tests/native_tu15_page_allocate_abi_tests.cpp'
        paths=[source,'src/physics/runtime/native_tu15_page_allocate_abi.hpp','include/halo2/native_tu15_page_allocate_kernel.hpp',str(Path(__file__).relative_to(ROOT)),
            'tools/validate_native_page_allocate_data_tu15.py','tools/validate_native_physical_allocation_abi_tu15.py','tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp',
            'tests/native_tu15_allocator_abi_fixture.hpp','src/physics/runtime/native_tu15_stack_probe_abi.hpp','src/physics/runtime/native_tu15_page_window_abi.hpp','src/physics/runtime/native_tu15_page_retire_abi.hpp',
            'src/physics/runtime/native_tu15_datum_delete_abi.hpp','src/physics/runtime/native_tu15_resource_busy_abi.hpp','src/gameplay/weapons/native_tu15_effect_datum_allocate.hpp',
            'include/halo2/native_tu15_stack_probe_kernel.hpp','include/halo2/native_tu15_page_window_kernel.hpp','include/halo2/native_tu15_page_retire_kernel.hpp','include/halo2/native_tu15_datum_delete_kernel.hpp',
            'include/halo2/native_tu15_resource_busy_kernel.hpp','include/halo2/native_tu15_datum_allocate_kernel.hpp','include/halo2/native_tu15_bitmap_scan_kernel.hpp','tools/native_page_allocate_fp_reference.py','tests/native_tu15_page_allocate_faults.hpp']
        if args.copy_routes or args.require_binding:paths+=['tools/native_page_allocate_replacements.py','tools/native_halfword_bit_query_replacements.py','tools/specialize_native_operands.py','src/physics/physics_live_support_tu15_generated.cpp','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp']
        frozen_sources={name:reference.sha(ROOT/name)for name in paths}
        assert hashlib.sha256(image.read(0x13fe50,data.IMAGE[0])).hexdigest()==data.IMAGE[1]
        report['canonical_body_sha256']=recover(scratch);rows=data.cases()
        if args.copy_routes or args.require_binding:report['copy_routes'],report['copy_binding_rejections']=copy_routes(scratch,args.require_binding)
        for index in range(0,30,2):
            backward=copy.deepcopy(rows[index]);backward['name']+='__DF_set';backward['direction_flag']=True;rows.append(backward)
        from validate_native_physical_allocation_abi_tu15 import physical_inputs
        banks,provenance=physical_inputs(image)
        # The captured allocator inputs contain selected globals, not the
        # whole original page needed by the separately declared busy cases.
        # Merge their unchanged bytes over the pinned original global page;
        # avoid ambiguous overlapping Memory banks in the native test owner.
        globals_page=bytearray(image.read(0x4f2000,4096));global_overlays={}
        for address,raw in list(banks.items()):
            if 0x4f2000<=address and address+len(raw)<=0x4f3000:
                globals_page[address-0x4f2000:address-0x4f2000+len(raw)]=raw
                global_overlays[hex(address)]=dict(bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest());del banks[address]
        banks[0x4f2000]=bytes(globals_page)
        report['global_backing']=dict(original_page_sha256=hashlib.sha256(image.read(0x4f2000,4096)).hexdigest(),captured_overlays=global_overlays,merged_sha256=hashlib.sha256(globals_page).hexdigest())
        for alignment in range(16):
            for size in((4096,0x10000000)if alignment in(0,1,7,15)else(4096,)):
                rows.append(dict(name=f'genuine_owner_{alignment}_{size}',entry=0x13fe50,arguments=[0x1017d000,size,1],receiver=0x12345678,header=data.reference.HEADER,full_abi=True,
                    stack_pointer=data.reference.SP+alignment,input_registers=dict(eax=0xabcd1234,edx=0xd2d2d2d2,ebx=0xb3b3b3b3,ebp=0xb5b5b5b5,esi=0x51515151,edi=0xd7d7d7d7),cohort='genuine-initialized-owner-and-descriptor-banks'))
        for alignment in(0,1,7,15):
            for offset,backward in((0,False),(4,False),(12,False),(24,False),(24,True),(56,False),(56,True),(64,False),(64,True)):
                alias=copy.deepcopy(rows[0]);alias['name']=f'caller_record_alias_{alignment}_{offset}_{backward}'
                alias['stack_pointer']=reference.SP+alignment;alias['direction_flag']=backward;frame=alias['stack_pointer']-0x1064
                alias['input_words'].append((reference.UI+0x1000+0x44,frame+offset))
                alias['mutations']=[dict(instruction=0x140127,address=frame+0x40,written=(7).to_bytes(4,'little').hex())]
                alias['cohort']='declared-record-caller-save-and-live-selection-alias'
                rows.append(alias)
        if args.limit:rows=rows[:args.limit]
        parent=leaf_body((ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text(),'13fe50')[0]
        parent_io=[int(re.search(r'instruction\((0x[0-9a-f]+)u\)',line)[1],16)for line in parent.splitlines()if any(token in line for token in('r(s.memory,','w(s.memory,','s.push(','s.pop()'))]
        rng=random.Random(0x13fe50)
        values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f',
            '00000000000000800040','00000000000000c0ff3f','00000000000000000000','0000000000000080ffbf')
        for ordinal,row in enumerate(rows):
            row['declared_fp_inputs']=dict(x87=list(values[:(0,1,4,8)[ordinal%4]]),xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],cw=(0x7f,0x27f,0x37f)[ordinal%3]|((ordinal%4)<<10),mxcsr_extra=0x8040 if ordinal&1 else 0)
            row['parent_boundaries']=parent_io if args.parent_io else[]
            row['parent_write_boundaries']=[int(re.search(r'instruction\((0x[0-9a-f]+)u\)',line)[1],16)for line in parent.splitlines()if any(token in line for token in('w(s.memory,','s.push('))]
            row['abi_entries']=[*data.OWNERS,0x4057f0,*row['parent_boundaries']]
        reference.OWNERS=data.OWNERS
        from native_page_allocate_fp_reference import original_with_fp
        directory,authority,fp_receipt=original_with_fp(reference,image,banks,rows,scratch)
        fixture=directory/'page-allocation-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows,run_parent_faults=args.faults),separators=(',',':')))
        binary=scratch/'page-allocation-abi';source='tests/native_tu15_page_allocate_abi_tests.cpp'
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/source),'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.copy_routes or args.require_binding:command+=['-DHALO2_PAGE_ALLOCATE_COPY_ROUTES']
        report['compiler_command']=command;built=subprocess.run(command,text=True,capture_output=True)
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        modes={}
        if args.canonical_io_diagnostic:
            assert args.parent_io
            diagnostic=subprocess.run([str(binary),str(fixture),'canonical-io-diagnostic'],text=True,capture_output=True,env=dict(os.environ,HALO2_REFERENCE_PAGE_ALLOCATE='1'),timeout=120)
            assert diagnostic.returncode==3,diagnostic.stderr
            gap=json.loads(diagnostic.stdout);original=gap['original'];canonical=gap['canonical']
            assert original['entry']==canonical['entry']==0x13ff45 and original['abi']['eflags']==84 and canonical['abi']['eflags']==68
            assert {k:v for k,v in original['abi'].items()if k!='eflags'}=={k:v for k,v in canonical['abi'].items()if k!='eflags'}
            report['canonical_intermediate_flags_counterexample']=gap
        for name,controls in(('native',{}),('reference',{'HALO2_REFERENCE_PAGE_ALLOCATE':'1'}),
            ('window-reference',{'HALO2_REFERENCE_PAGE_ALLOCATOR':'1'}),('busy-reference',{'HALO2_REFERENCE_RESOURCE_BUSY':'1'}),
            ('effect-reference',{'HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE':'1'}),('datum-reference',{'HALO2_REFERENCE_DATUM_DELETE':'1'})):
            result=subprocess.run([str(binary),str(fixture)],text=True,capture_output=True,env=dict(os.environ,**controls),timeout=300 if args.faults else 120)
            if result.returncode:report['comparison_error']=result.stderr[-8000:];sys.stderr.write(result.stderr);result.check_returncode()
            modes[name]=json.loads(result.stdout)
            if name!='reference':assert modes[name]['parent_instruction_callbacks']==0
        if args.copy_routes or args.require_binding:
            for route in report['copy_routes']:
                for name,controls in(('native',{}),('parent-reference',{'HALO2_REFERENCE_PAGE_ALLOCATE':'1'}),
                    ('child-reference',{'HALO2_REFERENCE_PAGE_ALLOCATOR':'1','HALO2_REFERENCE_RESOURCE_BUSY':'1','HALO2_REFERENCE_EFFECT_DATUM_ALLOCATE':'1','HALO2_REFERENCE_DATUM_DELETE':'1'})):
                    result=subprocess.run([str(binary),str(fixture),route],text=True,capture_output=True,env=dict(os.environ,**controls),timeout=120)
                    if result.returncode:report['comparison_error']=result.stderr[-8000:];sys.stderr.write(result.stderr);result.check_returncode()
                    modes[route+'-'+name]=json.loads(result.stdout)
                    if name!='parent-reference':assert modes[route+'-'+name]['parent_instruction_callbacks']==0
        assert frozen_sources=={name:reference.sha(ROOT/name)for name in paths},'Page proof sources changed during qualification'
        executed={pc for row in rows for pc in row['executed_sites']};body=reference.complete_body(reference._Code(image,'.text'),0x13fe50)
        report.update(passed=True,production_bound=args.require_binding,full_page_allocator_child_owner_closure_qualified=False,modes=modes,source_authority=authority,input_provenance=provenance,fp_input_and_observation_extension=fp_receipt,
            body_coverage=dict(total=len(body.code),executed=len(set(body.code)&executed),unexecuted=[hex(pc)for pc in sorted(set(body.code)-executed)]),physical_cases=len(rows),original_instructions_unchanged=True,parent_io_boundaries_qualified=args.parent_io,
            retained_fault_protocol_qualified=False,physical_page_fault_atomicity_qualified=False,parent_atomic_provider_protocol_qualified=args.faults,
            fault_scope='Declared atomic-provider exceptions before parent reads/writes, observed store peeks and completed store-observer throws. Expected ABI from genuine hardware pre-I/O packets, expected banks/store-prefix from original ordered writes plus declared feedback. Actual signal/page-fault atomicity and child-internal fault domains remain separately open.',
            fp_scope='Declared0/1/4/8 live80-bit payloads and random nonzero XMM caller banks, plus varied masked control/status/TOP/MXCSR. All full packets and final returned caller payloads compared against genuine GETFPREGS samples; no original math/result/outcome substitution.',
            reference_scope='Complete authenticated canonical bodies remain the independent normal-return/child-entry/read/store/bank oracle. Their physical undefined intermediate SHL AF disagreement is retained explicitly; native parent pre-I/O is compared directly with unchanged original hardware.',
            source_sha256=frozen_sources)
        print(json.dumps(dict(passed=True,cases=len(rows),modes={name:dict(cases=value['cases'],packets=value['complete_child_entry_packets'],parent_callbacks=value['parent_instruction_callbacks'])for name,value in modes.items()},retained_report=str(args.report)),indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
