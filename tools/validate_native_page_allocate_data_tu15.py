#!/usr/bin/env python3
"""First13FE50 DATA gate: real original heap outcomes, separate from full ABI."""
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
import generate_physics_live_support_tu15 as generator
from specialize_native_operands import leaf_body
from native_resource_busy_replacements import canonical_body as busy_canonical_body

IMAGE=(1213,'0221277cd0ba57b61aa637b4e0df13eed97a84db1906b01ba521378d7f8a795b')
OWNERS=(0x13fe50,0x32a660,0x13fe00,0x140310,0x16e6a0,0x16e8a0,0x12fcb0)
BUSY_IMAGE=(83,'1df96e347163f4151ccb82bf996d1aea2abae0e06e9c425b1525e2b450b4861a')


def callback_body(image,scratch):
    assert hashlib.sha256(image.read(0x12fcb0,BUSY_IMAGE[0])).hexdigest()==BUSY_IMAGE[1]
    code=reference._Code(image,'.text');body=reference.complete_body(code,0x12fcb0);addresses={}
    for pc in body.code:
        ins=code.at(pc);addresses[pc]=pc;addresses[pc+ins.size]=pc+ins.size
        for op in ins.operands:
            if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
            if op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=reference.NEW_PIN,entries={'0012fcb0':0x12fcb0},body_sites={'0012fcb0':sorted(body.code)},
        instruction_map={f'{pc:08x}':pc for pc in body.code},address_map={f'{p:08x}':q for p,q in addresses.items()},routines=[dict(entry=0x12fcb0,tables=[],byte_tables=[])])
    pairing=scratch/'page-data-busy-pairing.json';pairing.write_text(json.dumps(paired));args=SimpleNamespace(output=scratch/'page-data-busy-reference.cpp',manifest=scratch/'page-data-busy-reference.json',correspondence=pairing,additional_entries=pairing)
    generator.generate(args,paired,complete_listings=True);pure=busy_canonical_body(leaf_body(args.output.read_text(),'12fcb0')[0],'12fcb0')
    assert hashlib.sha256(pure.encode()).hexdigest()=='70937fa42338b49117d717cfa93e231ab79060e50cd6c99dd4a02f09864eb918'
    (scratch/'page_data_busy_reference.inc').write_text(pure+'\n');return hashlib.sha256(pure.encode()).hexdigest()


def cases():
    rows=[];owner=reference.UI+0x100;pool=reference.UI+0x1000;records=reference.UI+0x2000;bitmap=reference.UI+0x6000
    profiles=(('empty',64,[],1,8),('full_empty',64,[],1,0),('one',64,[(0,8,100)],4,8),
        ('gap',64,[(8,8,990)],4,8),('full_capacity',64,[(0,8,100)],1,1),
        ('fragmented',64,[(0,8,100),(12,8,500),(28,8,900)],20,8),
        ('middle',64,[(0,8,999),(16,8,100),(32,8,999)],8,8),
        ('tail',64,[(0,8,999),(8,8,999)],8,8),('impossible',64,[(0,64,1000)],100,8),
        ('tied',64,[(0,8,800),(8,8,800),(20,8,800)],12,8))
    def add(name,total,blocks,pages,capacity,strategy,priority=1,budget=0x7fffffff,shift=4,feedback=False):
        n=len(blocks);handles=[((0x8001+i)<<16)|i for i in range(n)];sp=reference.SP+len(rows)%16
        words=[(owner+0x20,0),(owner+0x24,0),(owner+0x2c,strategy),(owner+0x30,total),(owner+0x34,shift),(owner+0x38,1000),
            (owner+0x3c,handles[0]if n else 0xffffffff),(owner+0x40,handles[-1]if n else 0xffffffff),(owner+0x64,pool),
            (pool+0x20,capacity),(pool+0x24,24),(pool+0x28,0x100),(pool+0x34,n if n<capacity else 0),(pool+0x38,n),(pool+0x3c,n),
            (pool+0x40,0x8000+n),(pool+0x44,records),(pool+0x48,bitmap),(bitmap,(1<<n)-1)]
        # The priority-eight lookup aliases the real descriptor pointer at
        # +64. Keep that actual owner field, rather than replacing it with a
        # fabricated capacity just to make a fixture pass.
        words.extend((owner+0x44+i*4,budget)for i in range(8))
        for i,(start,count,tick)in enumerate(blocks):words.extend(((records+i*24,0x8001+i),(records+i*24+4,count),(records+i*24+8,start),
            (records+i*24+12,handles[i+1]if i+1<n else 0xffffffff),(records+i*24+16,handles[i-1]if i else 0xffffffff),(records+i*24+20,tick)))
        row=dict(name=name,entry=0x13fe50,arguments=[owner,pages<<shift,priority&0xffffffff],receiver=0x12345678,header=reference.HEADER,full_abi=True,
            stack_pointer=sp,input_words=words,input_registers=dict(eax=0xabcd1234,edx=0xd2d2d2d2,ebx=0xb3b3b3b3,ebp=0xb5b5b5b5,esi=0x51515151,edi=0xd7d7d7d7))
        if feedback:row['mutations']=[dict(instruction=0x140093,address=sp-0x1064+0x14,written=(2).to_bytes(4,'little').hex())]
        rows.append(row)
    for name,total,blocks,pages,capacity in profiles:
        for strategy in(0,1,2):add(name+f'_strategy{strategy}',total,blocks,pages,capacity,strategy)
    for priority in(-2,0,1,8,9):
        for pages in(1,2,3):add(f'budget_{priority}_{pages}',64,[],pages,8,2,priority=priority,budget=2)
    for shift in(0,1,8,12,31):add(f'page_shift_{shift}',64,[],1,8,2,shift=shift)
    add('declared_ring_frame_feedback',1024,[(2,2,990)],900,8,2,feedback=True)
    add('rounded_byte_request',64,[],1,8,2);rows[-1]['arguments'][1]+=1
    add('declared_empty_bitmap_extent',64,[],1,8,2);rows[-1]['input_words'] +=[(pool+0x34,0),(pool+0x38,1),(pool+0x3c,0),(bitmap,0),(records,0)]
    for pages in(1,0x7fffffff):
        add(f'declared_signed_ring_frame_{pages}',64,[(0,1,990)],pages,8,2,priority=0,shift=0)
        base=rows[-1]['stack_pointer']-0x1064
        rows[-1]['mutations']=[dict(instruction=0x13ff52,address=base+offset,written=(0x80000000).to_bytes(4,'little').hex())for offset in(0x14,0x18)]
    objects=reference.UI+0x7000;object_rows=reference.UI+0x7200
    for name,flags,ready,associated,global_flag,boundary in(('free',0,0,0xffffffff,1,False),('pinned',1,0,0xffffffff,1,False),
        ('queued',2,0,0xffffffff,0,False),('released_queue',2,0,0xffffffff,1,False),('not_ready',0,0,1234,1,False),('genuine_fence_entry',0,1,1234,1,True)):
        add('genuine_busy_'+name,64,[(0,8,100)],4,8,2)
        rows[-1]['input_words'] +=[(owner+0x24,0x12fcb0),(0x4f2d24,objects),(0x4f2d49,global_flag),(objects+0x44,object_rows),
            (object_rows,0x8001|(flags<<16)|(ready<<24)),(object_rows+12,associated)]
        rows[-1]['abi_entries']=[0x12fcb0,0x4057f0]
        if boundary:rows[-1]['required_entries']=[0x4057f0]
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);p.add_argument('--sanitizers',action='store_true');args=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,production_bound=False,scratch_directory=str(scratch),retained_large_artifacts=[])
    try:
        assert reference.sha(reference.NEW_DEFAULT)==reference.NEW_PIN;image=reference.Xbe(reference.NEW_DEFAULT)
        assert hashlib.sha256(image.read(0x13fe50,IMAGE[0])).hexdigest()==IMAGE[1]
        callback_pin=callback_body(image,scratch);rows=cases();reference.OWNERS=OWNERS;directory,authority=reference.original(image,{0x4f2000:image.read(0x4f2000,4096)},rows,scratch);fixture=directory/'page-allocation-data.json';fixture.write_text(json.dumps(dict(image_sha256=reference.NEW_PIN,cases=rows),separators=(',',':')))
        binary=scratch/'page-allocation-data';sources=['tests/native_tu15_page_allocate_data_tests.cpp','src/core/native_tu15_data_array_allocate.cpp','src/core/native_tu15_data_array_delete.cpp']
        command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),*[str(ROOT/name)for name in sources],'-o',str(binary)]
        if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
        built=subprocess.run(command,text=True,capture_output=True);report['compiler_command']=command
        if built.returncode:report['compiler_errors']=built.stderr[-6000:];sys.stderr.write(report['compiler_errors']);built.check_returncode()
        result=subprocess.run([str(binary),str(fixture)],text=True,capture_output=True,timeout=120)
        if result.returncode:report['comparison_errors']=result.stderr[-6000:];sys.stderr.write(result.stderr);result.check_returncode()
        executed={pc for row in rows for pc in row['executed_sites']};code=reference._Code(image,'.text');body=reference.complete_body(code,0x13fe50)
        files=sources+['include/halo2/native_tu15_page_allocate_kernel.hpp','include/halo2/native_tu15_stack_probe_kernel.hpp','include/halo2/native_tu15_page_window_kernel.hpp','include/halo2/native_tu15_page_retire_kernel.hpp',str(Path(__file__).relative_to(ROOT)),
            'include/halo2/native_tu15_datum_allocate_kernel.hpp','include/halo2/native_tu15_datum_delete_kernel.hpp','include/halo2/native_tu15_bitmap_scan_kernel.hpp','tests/native_tu15_allocator_abi_fixture.hpp',
            'tools/validate_native_array_growth_tu15.py','tools/observe_native_array_growth_tu15.cpp','tools/generate_physics_live_support_tu15.py','tools/native_resource_busy_replacements.py']
        report.update(passed=True,comparison=json.loads(result.stdout),source_authority=authority,original_instructions_unchanged=True,
            original_parent_image=IMAGE,input_scope='Declared owned page descriptors/records, DF0 and caller-frame feedback. Retirement callback inputs remain null. Shared DATA children independently compute allocations/retirements. Busy cases execute the genuine canonical12FCB0 provider; it remains explicitly translated test service, with one real4057F0 entry stop and no supplied device outcome.',
            retained_test_callback=dict(entry='12fcb0',original_image=BUSY_IMAGE,canonical_body_sha256=callback_pin,production_native_closure_claimed=False),
            qualified_scope='Return handle, complete heap banks and exact ordered heap reads/stores. Caller-frame transport uses real shared stack-probe DATA; full GP/flags/FP/callee-entry/caller-bank and exception proof remain open.',
            full_retained_abi_qualified=False,callback_service_closure_qualified=False,
            body_coverage=dict(total=len(body.code),executed=len(set(body.code)&executed),unexecuted=[hex(pc)for pc in sorted(set(body.code)-executed)]),
            address_undefined_sanitizers=args.sanitizers,source_sha256={name:reference.sha(ROOT/name)for name in files})
        print(json.dumps(report,indent=2));return 0
    except BaseException as error:report['error']=repr(error);raise
    finally:args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':raise SystemExit(main())
