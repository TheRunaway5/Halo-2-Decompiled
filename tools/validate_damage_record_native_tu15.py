#!/usr/bin/env python3
"""Compare the production indexed-record kernel against untouched original code."""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--admission',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--negative-control',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py with oracle-capable Python')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    admission=json.loads(args.admission.read_text());borrowed=Path(admission['scratch'])/'source'
    kernel='src/gameplay/objects/source_damage_record_native_tu15_internal.hpp'
    cpp='src/gameplay/objects/source_damage_receiver_tu15_generated.cpp'
    for name in (kernel,cpp,'include/halo2/source_damage_record_native_tu15.hpp'):
        if sha(borrowed/name)!=admission['patches'][name]['after']:raise ValueError('Admitted native source drift')
    source=owner/'source';source.mkdir()
    paths=[*borrowed.joinpath('include').rglob('*.hpp'),*borrowed.joinpath('src').rglob('*.hpp'),borrowed/cpp]
    pins={}
    for path in paths:
        name=path.relative_to(borrowed);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        if sha(target)!=before or sha(path)!=before:raise ValueError('Source changed while freezing native record test')
        pins[str(name)]=before
    fixture_test='tests/source_damage_record_native_tu15_tests.cpp'
    (source/'tests').mkdir();shutil.copyfile(ROOT/fixture_test,source/fixture_test)
    pins[fixture_test]=sha(source/fixture_test)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    mutation=None
    if args.negative_control:
        path=source/kernel;text=path.read_text();needle='r(s.memory,selected->flag,1)|4u'
        if text.count(needle)!=1:raise ValueError('Native record mutation seam changed')
        before=sha(path);path.write_text(text.replace(needle,'r(s.memory,selected->flag,1)|8u'))
        pins[kernel]=sha(path);mutation=dict(path=kernel,before=before,after=pins[kernel],scope='Private flag-bit mutation; oracle unchanged')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=_trace(code,0x105460,{0x105460})
    if len(body.code)!=23 or body.tables:raise ValueError('Indexed-record original owner changed')
    controls=((0,0),(1,0),(1,1),(2,0),(2,1),(2,2),(0,0xffffffff),
        (0xffffffff,0xfffffffe),(0x80000000,0),(0x7fffffff,0),(0x80000000,0x7fffffff),
        (0x7fffffff,0x80000001),(0,0x80000000),(0xffffffff,0xffffffff))
    registers=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
        UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
    xmm_regs=(UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3,
        UC_X86_REG_XMM4,UC_X86_REG_XMM5,UC_X86_REG_XMM6,UC_X86_REG_XMM7)
    cases=[];admitted=rejected=0;stop=0x70000000
    for case_id,((count,selector),flag) in enumerate(itertools.product(controls,(0,4,0xfb,0xff))):
        memory=Uc(UC_ARCH_X86,UC_MODE_32);memory.mem_map(0x10000,0x800000)
        for section in image.sections:memory.mem_write(section.va,image.section_bytes(section))
        addresses=(0x4ec000,0x4f0000,*(0x10000000+n*0x1000 for n in range(7)))
        banks={address:bytearray([0x57])*0x1000 for address in addresses}
        object_index=257 if case_id%2 else 3;tag_index=129 if case_id%2 else 5
        handle=((0x1234+case_id)<<16)|object_index;definition=((0x7777+case_id)<<16)|tag_index
        def word(address,value):
            base=address&~0xfff;struct.pack_into('<I',banks[base],address-base,value&0xffffffff)
        word(0x4ecbd0,0x10000000);word(0x4f0414,0x10001000);word(0x10000044,0x10002000)
        word(0x10002000+object_index*12+8,0x10003000);word(0x10003000,definition)
        word(0x10001000+tag_index*16+8,0x10004000);word(0x100042d0,count)
        flag_address=(0x10003000+selector*0x34+0x1a8)&0xffffffff
        if (flag_address&~0xfff) in banks:banks[flag_address&~0xfff][flag_address&0xfff]=flag
        sp=0x10006800;word(sp,stop)
        for address,data in banks.items():
            if address>=0x10000000:memory.mem_map(address,0x1000)
            memory.mem_write(address,bytes(data))
        memory.mem_map(stop,0x1000)
        gp=[handle,selector,0xaabbccdd,0xbadc0ffe,sp,0xd00dcafe,0xf1234567,0xe7654321]
        xmm=[[((case_id+1)*0x1020304+n*13+k)&0xffffffff for k in range(4)]for n in range(8)]
        for register,value in zip(registers,gp):memory.reg_write(register,value)
        for register,value in zip(xmm_regs,xmm):memory.reg_write(register,sum(word<<(32*n)for n,word in enumerate(value)))
        memory.reg_write(UC_X86_REG_EFLAGS,0x202);memory.reg_write(UC_X86_REG_FPCW,0x37f)
        memory.reg_write(UC_X86_REG_FPSW,0);memory.reg_write(UC_X86_REG_MXCSR,0x1f80)
        instructions=[];writes=[];pc=[0]
        def instruction(uc,address,size,user):
            if address!=stop:
                if address not in body.code:raise ValueError('Original record escaped its complete owner')
                pc[0]=address;instructions.append(address)
        def store(uc,access,address,size,value,user):
            writes.append(dict(instruction=pc[0],address=address,before=bytes(uc.mem_read(address,size)).hex(),
                written=(value&((1<<(8*size))-1)).to_bytes(size,'little').hex()))
        memory.hook_add(UC_HOOK_CODE,instruction);memory.hook_add(UC_HOOK_MEM_WRITE,store)
        memory.emu_start(0x105460,stop,count=1000)
        if memory.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original record did not return')
        initial=dict(gp=gp,xmm=xmm,eflags=0x202,x87_control_word=0x37f,mxcsr=0x1f80)
        after=dict(gp=[memory.reg_read(register)for register in registers],eflags=memory.reg_read(UC_X86_REG_EFLAGS),
            xmm=[[(memory.reg_read(register)>>(32*n))&0xffffffff for n in range(4)]for register in xmm_regs],
            x87_control_word=memory.reg_read(UC_X86_REG_FPCW),mxcsr=memory.reg_read(UC_X86_REG_MXCSR))
        cases.append(dict(id=case_id,before=initial,after=after,instructions=instructions,stores=writes,
            before_banks=[dict(address=a,hex=data.hex())for a,data in banks.items()],
            after_banks=[dict(address=a,hex=bytes(memory.mem_read(a,len(data))).hex())for a,data in banks.items()]))
        accepted=0x10549d in instructions;admitted+=accepted;rejected+=not accepted
    fixture=owner/'original-record-cases.json'
    fixture.write_text(json.dumps(dict(schema='H2_DAMAGE_RECORD_NATIVE_ORIGINAL_V1',cases=cases),separators=(',',':'))+'\n')
    (args.output/'source-hashes.json').write_text(json.dumps(pins,indent=2)+'\n')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2DamageRecordNative LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(source_damage_record_native_tu15_tests tests/source_damage_record_native_tu15_tests.cpp
 src/gameplay/objects/source_damage_receiver_tu15_generated.cpp)
target_include_directories(source_damage_record_native_tu15_tests PRIVATE include src)
target_compile_features(source_damage_record_native_tu15_tests PRIVATE cxx_std_20)
target_compile_options(source_damage_record_native_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(source_damage_record_native_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+=f'add_test(NAME native_damage_record_original COMMAND "${{Python3_EXECUTABLE}}" "{ROOT / "tools/run_test.py"}" -- $<TARGET_FILE:source_damage_record_native_tu15_tests> "{fixture}")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','source_damage_record_native_tu15_tests','--filter','^native_damage_record_original$',
        '--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    worker=json.loads((args.output/'validation/report.json').read_text())
    log=(args.output/'validation/tests.log').read_text()if(args.output/'validation/tests.log').exists()else''
    rejected_mutation=code!=0 and all(s['exit_code']==0 for s in worker['steps']if s['name']!='tests')and 'native ordered stores differ' in log
    passed=(rejected_mutation if args.negative_control else code==0)and admitted>0 and rejected>0
    unchanged=all(sha(source/name)==digest for name,digest in pins.items())
    report=dict(passed=passed and unchanged,original_sha256=NEW_PIN,cases=len(cases),admitted=admitted,rejected=rejected,
        complete_original_instructions=23,original_expected_native_results_supplied=False,instruction_patches=[],
        oracle='Offline Unicorn execution of the pinned unchanged original integer owner',
        actual_live_i386_or_full_gameplay_acceptance=False,source_unchanged=unchanged,
        original_fixture_sha256=sha(fixture),validation_returncode=code,negative_control=mutation,
        comparison_rejected_mutation=rejected_mutation if args.negative_control else None,
        owned_scratch=str(owner),retained_large_artifacts=[],tool_sha256=sha(__file__))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases),report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
