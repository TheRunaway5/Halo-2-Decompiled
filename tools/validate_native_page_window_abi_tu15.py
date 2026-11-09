#!/usr/bin/env python3
"""Qualify shared13FE00 DATA, physical pre-I/O ABI and retained faults."""
import argparse
import hashlib
import json
import os
from pathlib import Path
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
from native_page_window_replacements import install_native_page_window,canonical_body,PREFIX,BODY_PIN

IMAGE=(77,'230c795e07348fef2d581eb576c36a83dfd67c884ea8872c9eaad37a7eb936d1')
BOUNDARIES=(0x13fe00,0x13fe03,0x13fe04,0x13fe1a,0x13fe32,0x13fe37,0x13fe42,0x13fe45,0x13fe4c)


def recover(image,scratch):
    assert hashlib.sha256(image.read(0x13fe00,IMAGE[0])).hexdigest()==IMAGE[1]
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x13fe00);assert len(body.code)==27
    addresses={}
    for pc in body.code:
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for op in ins.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={'0013fe00':0x13fe00},body_sites={'0013fe00':sorted(body.code)},
        instruction_map={f'{pc:08x}':pc for pc in body.code},address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=0x13fe00,tables=[],byte_tables=[])])
    pairing=scratch/'page-window-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'page-window-reference.cpp',manifest=scratch/'page-window-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);pure=canonical_body(leaf_body(args.output.read_text(),'13fe00')[0]);assert hashlib.sha256(pure.encode()).hexdigest()==BODY_PIN
    (scratch/'page_window_reference.inc').write_text(pure+'\n');return body


def cases():
    rows=[]
    profiles=((0,0,0,1,2),(300,300,0,1,2),(300,0,300,2,1),(1000,999,700,1,2),(1000,700,999,2,1),
        (1000,850,700,1,2),(1000,849,700,2,1),(1000,700,850,1,2),(1000,700,849,2,1),
        (0xffffffff,0xffffff00,0xfffffffe,0x80000000,0x7fffffff),(0,1,0xffffffff,0x7fffffff,0x80000000),
        (1000,990,990,0xffffffff,0),(1000,990,990,0,0xffffffff),(1000,990,990,0xffffffff,0xffffffff))
    def add(name,alignment,owner,candidate,best,words,fault=False):
        rows.append(dict(name=name,entry=0x13fe00,arguments=[0x11223344,0x55667788],receiver=owner,header=reference.HEADER,full_abi=True,
            stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),abi_entries=list(BOUNDARIES),input_words=words,fault_case=fault,
            input_registers=dict(eax=0xabcd1234,edx=0xc3c3c3c3,ebx=0x12345678,ebp=0x87654321,esi=candidate,edi=best)))
    for alignment in range(16):
        candidate,best=reference.UI+0x200,reference.UI+0x300
        add(f'genuine_window_tick_{alignment}',alignment,0x1017d000,candidate,best,[(candidate+4,0),(candidate+12,1),(best+4,20),(best+12,2)],alignment==0)
    for n,(tick,current,prior,count,best_count)in enumerate(profiles):
        for alignment in(0,1,7,15):
            owner,candidate,best=reference.UI+0x100,reference.UI+0x200,reference.UI+0x300
            add(f'declared_window_{n}_{alignment}',alignment,owner,candidate,best,[(owner+0x38,tick),(candidate+4,current),(candidate+12,count),(best+4,prior),(best+12,best_count)],alignment==0)
    for alignment in(0,1,7,15):
        sp=reference.SP+alignment
        for field in('owner_tick','candidate_tick','best_tick','candidate_count','best_count'):
            owner,candidate,best=reference.UI+0x100,reference.UI+0x200,reference.UI+0x300
            if field=='owner_tick':owner=sp-4-0x38
            elif field=='candidate_tick':candidate=sp-8
            elif field=='best_tick':best=sp-8
            elif field=='candidate_count':candidate=sp-16
            else:best=sp-16
            add(f'saved_window_{field}_{alignment}',alignment,owner,candidate,best,
                [(owner+0x38,1000),(candidate+4,990),(candidate+12,1),(best+4,990),(best+12,2)],alignment==0)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');p.add_argument('--require-binding',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT);body=recover(image,scratch);banks,provenance=physical_inputs(image)
        binding=None
        if args.require_binding:
            from specialize_native_operands import specialize
            source=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text();assert install_native_page_window(source)==source
            assert specialize(source)[1]['native_page_window_entries']==['13fe00']
            function=leaf_body(source,'13fe00')[0];changed=function.replace('0x38u','0x3cu',1);assert changed!=function
            for attempt in(install_native_page_window,specialize):
                try:attempt(source.replace(function,changed,1))
                except ValueError:pass
                else:raise AssertionError('Changed bound13FE00 body admitted')
            (scratch/'page_window_source.inc').write_text(function.replace(PREFIX,PREFIX+'    ++page_window_fallbacks;\n',1)+'\n')
            binding=dict(entries=['13fe00'],body_sha256=hashlib.sha256(function.encode()).hexdigest(),complete_body_drift_rejections=2)
        rows=cases();reference.OWNERS=(0x13fe00,);directory,authority=reference.original(image,banks,rows,scratch);fixture=directory/'page-window-abi.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'page-window';sources=['tests/native_tu15_page_window_abi_tests.cpp','src/core/native_tu15_page_allocator.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp']
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),*[str(ROOT/name)for name in sources],'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        if args.require_binding:command+=['-DHALO2_PAGE_WINDOW_SOURCE_ROUTE']
        built=subprocess.run(command,text=True,capture_output=True);report['compiler_command']=command
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        modes={}
        for route in(('bridge','source')if args.require_binding else('bridge',)):
            for mode in('native','reference'):
                env=dict(os.environ)
                for key in('HALO2_REFERENCE_PAGE_ALLOCATOR','HALO2_REFERENCE_MATRIX_LEAVES','HALO2_REFERENCE_PARTITION_TEMPORARIES'):env.pop(key,None)
                if mode=='reference':env['HALO2_REFERENCE_PAGE_ALLOCATOR']='1'
                result=subprocess.run([str(binary),str(fixture),*(['source']if route=='source'else[])],env=env,text=True,capture_output=True,timeout=120)
                if result.returncode:report['comparison_errors']=result.stderr[-6000:];sys.stderr.write(result.stderr);result.check_returncode()
                modes[mode if route=='bridge'else route+'_'+mode]=json.loads(result.stdout)
        files=sources+['include/halo2/native_tu15_page_window_kernel.hpp','src/physics/runtime/native_tu15_page_window_abi.hpp','tests/native_tu15_allocator_abi_fixture.hpp',str(Path(__file__).relative_to(ROOT))]
        files+=['tools/native_page_window_replacements.py','tools/generate_physics_live_support_tu15.py','tools/specialize_native_operands.py']
        files+=['tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tools/validate_native_physical_allocation_abi_tu15.py']
        if args.require_binding:files+=['src/physics/physics_live_support_tu15_generated.cpp']
        executed={pc for row in rows for pc in row['executed_sites']};report.update(passed=True,modes=modes,input_provenance=provenance,source_authority=authority,original_instructions_unchanged=True,
            body_coverage=dict(total=len(body.code),executed=len(set(body.code)&executed)),physical_pre_io_boundaries=[hex(pc)for pc in BOUNDARIES],
            production_bound=args.require_binding,binding=binding,physical_packet_scope='Direct ABI template before-memory snapshots; production source route additionally checks complete returns, all banks/ordered reads/stores and retained fault prefixes without invoking instruction callbacks.',
            retained_memory_exception_protocol_qualified=True,physical_instruction_page_fault_atomicity_qualified=False,
            exception_qualification_scope='Canonical Memory faults, observed and observer-free, plus throwing store observers; full partial GP/full flags/XMM/live80-bit/FP/banks/cursor. Separate from physical instruction/page-fault atomicity.',
            legacy_scratch_flags_api_preserved=True,address_undefined_sanitizers=args.sanitizers,source_sha256={name:reference.sha(ROOT/name)for name in files})
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
