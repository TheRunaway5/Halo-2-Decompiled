#!/usr/bin/env python3
"""Compare the production model-damage lookup against untouched TU1.5 code."""
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
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generation',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--negative-control',action='store_true')
    parser.add_argument('--direct-generation',type=Path)
    parser.add_argument('--vitality-generation',type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py with oracle-capable Python')
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    generation=json.loads(args.generation.read_text())
    if bool(args.direct_generation)!=bool(args.vitality_generation):parser.error('Supply both paired generation records')
    paired=bool(args.direct_generation)
    kernel='src/gameplay/objects/source_damage_model_native_tu15_internal.hpp'
    cpp='src/gameplay/units/source_damage_delivery_runtime_generated.cpp'
    header='include/halo2/source_damage_model_native_tu15.hpp'
    for name,key in ((kernel,'kernel_sha256'),(cpp,'generated_cpp_sha256'),(header,'activity_header_sha256')):
        if sha(ROOT/name)!=generation[key]:raise ValueError('Generated native model source drift')
    extra_cpp=[]
    for name,path in (('source_direct_damage_runtime_generated.cpp',args.direct_generation),('source_vitality_runtime_generated.cpp',args.vitality_generation)):
        if not path:continue
        evidence=json.loads(path.read_text());name='src/gameplay/units/'+name
        if sha(ROOT/name)!=evidence['generated_cpp_sha256']or sha(ROOT/kernel)!=evidence['kernel_sha256']:
            raise ValueError('Paired native model source drift')
        extra_cpp.append(ROOT/name)
    source=owner/'source';source.mkdir()
    paths=[*ROOT.joinpath('include').rglob('*.hpp'),*ROOT.joinpath('src').rglob('*.hpp'),ROOT/cpp,*extra_cpp]
    pins={}
    for path in paths:
        name=path.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        if sha(target)!=before or sha(path)!=before:raise ValueError('Source changed while freezing native model test')
        pins[str(name)]=before
    fixture_test='tests/source_damage_model_native_tu15_tests.cpp'
    (source/'tests').mkdir();shutil.copyfile(ROOT/fixture_test,source/fixture_test);pins[fixture_test]=sha(source/fixture_test)
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    mutation=None
    if args.negative_control:
        path=source/kernel;text=path.read_text();needle='std::bit_cast<std::int32_t>(count)>0'
        if text.count(needle)!=1:raise ValueError('Native model negative-control seam changed')
        before=sha(path);path.write_text(text.replace(needle,'count!=0u'));pins[kernel]=sha(path)
        mutation=dict(path=kernel,before=before,after=pins[kernel],scope='Private unsigned-presence mutation; original oracle unchanged')
    if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Original TU1.5 changed')
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');body=_trace(code,0xd80a0,{0xd80a0})
    if len(body.code)!=24 or body.tables:raise ValueError('Original model lookup changed')
    canonical_map={}
    if paired:
        from oracle_smg_firing_1_1 import PIN
        from research_mp_sandbox_update_functions import full_tokens
        old_path=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
        if sha(old_path)!=PIN:raise ValueError('Canonical observation reference changed')
        old=Xbe(old_path);old_code=_Code(old,'.text');old_body=_trace(old_code,0xd7160,{0xd7160})
        if len(old_body.code)!=24 or full_tokens([old_code.at(p)for p in sorted(old_body.code)],old)!=full_tokens([code.at(p)for p in sorted(body.code)],image):
            raise ValueError('Complete canonical/TU1.5 observation correspondence changed')
        canonical_map={str(p):q for p,q in zip(sorted(old_body.code),sorted(body.code))}
    registers=(UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,
        UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI)
    xmm_regs=(UC_X86_REG_XMM0,UC_X86_REG_XMM1,UC_X86_REG_XMM2,UC_X86_REG_XMM3,
        UC_X86_REG_XMM4,UC_X86_REG_XMM5,UC_X86_REG_XMM6,UC_X86_REG_XMM7)
    cases=[];stop=0x70000000;branches=dict(null_object=0,null_model=0,no_information=0,resolved=0)
    # All three return gates, signed count limits, low-16 index selection and
    # full salt variations. Declared memory inputs supply no native expectations.
    controls=('null_object','null_model',0,1,2,0x7fffffff,0x80000000,0xffffffff)
    for case_id,(control,index_set,salt)in enumerate(itertools.product(controls,range(2),(0,0x1234,0xfffe,0xffff))):
        memory=Uc(UC_ARCH_X86,UC_MODE_32);memory.mem_map(0x10000,0x800000)
        for section in image.sections:memory.mem_write(section.va,image.section_bytes(section))
        addresses=(0x4ec000,0x4f0000,*(0x10000000+n*0x1000 for n in range(7)))
        banks={address:bytearray([0x57])*0x1000 for address in addresses}
        object_index=257 if index_set else 3;tag_index=129 if index_set else 5;model_index=193 if index_set else 7
        handle=(salt<<16)|object_index;definition=((salt^0x7777)<<16)|tag_index;model=((salt^0x8888)<<16)|model_index
        def word(address,value):
            base=address&~0xfff;struct.pack_into('<I',banks[base],address-base,value&0xffffffff)
        word(0x4ecbd0,0x10000000);word(0x4f0414,0x10001000);word(0x10000044,0x10002000)
        word(0x10002000+object_index*12+8,0x10003000);word(0x10003000,definition)
        word(0x10001000+tag_index*16+8,0x10004000);word(0x10004038,0xffffffff if control=='null_model'else model)
        word(0x10001000+model_index*16+8,0x10005000);word(0x10005060,control if isinstance(control,int)else 1)
        word(0x10005064,0xdead0042+case_id)
        sp=0x10006800;word(sp,stop)
        for address,data in banks.items():
            if address>=0x10000000:memory.mem_map(address,0x1000)
            memory.mem_write(address,bytes(data))
        memory.mem_map(stop,0x1000)
        gp=[0xcafef00d,0xffffffff if control=='null_object'else handle,0xaabbccdd,0xbadc0ffe,sp,0xd00dcafe,0xf1234567,0xe7654321]
        xmm=[[((case_id+1)*0x1020304+n*13+k)&0xffffffff for k in range(4)]for n in range(8)]
        for register,value in zip(registers,gp):memory.reg_write(register,value)
        for register,value in zip(xmm_regs,xmm):memory.reg_write(register,sum(value<<(32*n)for n,value in enumerate(value)))
        memory.reg_write(UC_X86_REG_EFLAGS,0x202);memory.reg_write(UC_X86_REG_FPCW,0x37f)
        memory.reg_write(UC_X86_REG_FPSW,0);memory.reg_write(UC_X86_REG_MXCSR,0x1f80)
        instructions=[];reads=[];pc=[0]
        def instruction(uc,address,size,user):
            if address!=stop:
                if address not in body.code:raise ValueError('Original lookup escaped its complete owner')
                pc[0]=address;instructions.append(address)
        def read(uc,access,address,size,value,user):
            reads.append(dict(instruction=pc[0],address=address,bytes=bytes(uc.mem_read(address,size)).hex()))
        def write(uc,access,address,size,value,user):raise ValueError('Original read-only lookup wrote memory')
        memory.hook_add(UC_HOOK_CODE,instruction);memory.hook_add(UC_HOOK_MEM_READ,read);memory.hook_add(UC_HOOK_MEM_WRITE,write)
        memory.emu_start(0xd80a0,stop,count=1000)
        if memory.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original lookup did not return')
        initial=dict(gp=gp,xmm=xmm,eflags=0x202,x87_control_word=0x37f,mxcsr=0x1f80)
        after=dict(gp=[memory.reg_read(register)for register in registers],eflags=memory.reg_read(UC_X86_REG_EFLAGS),
            xmm=[[(memory.reg_read(register)>>(32*n))&0xffffffff for n in range(4)]for register in xmm_regs],
            x87_control_word=memory.reg_read(UC_X86_REG_FPCW),mxcsr=memory.reg_read(UC_X86_REG_MXCSR))
        branch=control if isinstance(control,str)else 'resolved'if 0<control<0x80000000 else'no_information';branches[branch]+=1
        cases.append(dict(id=case_id,before=initial,after=after,instructions=instructions,reads=reads,
            before_banks=[dict(address=a,hex=data.hex())for a,data in banks.items()],
            after_banks=[dict(address=a,hex=bytes(memory.mem_read(a,len(data))).hex())for a,data in banks.items()]))
    fixture=owner/'original-model-cases.json'
    fixture.write_text(json.dumps(dict(schema='H2_DAMAGE_MODEL_NATIVE_ORIGINAL_V1',canonical_observation_map=canonical_map,cases=cases),separators=(',',':'))+'\n')
    (args.output/'source-hashes.json').write_text(json.dumps(pins,indent=2)+'\n')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(Halo2DamageModelNative LANGUAGES CXX)
enable_testing()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_executable(source_damage_model_native_tu15_tests tests/source_damage_model_native_tu15_tests.cpp
 src/gameplay/units/source_damage_delivery_runtime_generated.cpp)
target_include_directories(source_damage_model_native_tu15_tests PRIVATE include src)
target_compile_features(source_damage_model_native_tu15_tests PRIVATE cxx_std_20)
target_compile_options(source_damage_model_native_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_link_libraries(source_damage_model_native_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
'''
    routes=['delivery','direct','vitality']if paired else['delivery']
    if paired:
        cmake+='target_sources(source_damage_model_native_tu15_tests PRIVATE src/gameplay/units/source_direct_damage_runtime_generated.cpp src/gameplay/units/source_vitality_runtime_generated.cpp)\n'
        cmake+='target_compile_definitions(source_damage_model_native_tu15_tests PRIVATE HALO2_DAMAGE_MODEL_PAIRED_ROUTES=1)\n'
    for route in routes:
        cmake+=f'add_test(NAME native_damage_model_original_{route} COMMAND "${{Python3_EXECUTABLE}}" "{ROOT/"tools/run_test.py"}" -- $<TARGET_FILE:source_damage_model_native_tu15_tests> "{fixture}" "{route}")\n'
    (source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','source_damage_model_native_tu15_tests','--filter','^native_damage_model_original_',
        '--jobs','2','--test-jobs','1','--output',str(args.output/'validation')]
    result=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    worker=json.loads((args.output/'validation/report.json').read_text())
    log=(args.output/'validation/tests.log').read_text()if(args.output/'validation/tests.log').exists()else''
    rejected_mutation=result!=0 and all(s['exit_code']==0 for s in worker['steps']if s['name']!='tests')and log.count('native instruction boundaries differ')==len(routes)
    passed=(rejected_mutation if args.negative_control else result==0)and all(branches.values())
    unchanged=all(sha(source/name)==digest for name,digest in pins.items())
    report=dict(passed=bool(passed and unchanged),original_sha256=NEW_PIN,cases=len(cases),branches=branches,
        qualified_routes=routes,total_route_cases=len(cases)*len(routes),canonical_observation_correspondence=canonical_map,
        complete_original_instructions=24,original_expected_native_results_supplied=False,instruction_patches=[],
        oracle='Offline Unicorn execution of the pinned unchanged original integer owner',
        actual_live_i386_or_full_gameplay_acceptance=False,source_unchanged=unchanged,
        original_fixture_sha256=sha(fixture),validation_returncode=result,negative_control=mutation,
        comparison_rejected_mutation=rejected_mutation if args.negative_control else None,
        owned_scratch=str(owner),retained_large_artifacts=[],tool_sha256=sha(__file__))
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],cases=len(cases),report=str(args.output/'report.json'))),flush=True)
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
