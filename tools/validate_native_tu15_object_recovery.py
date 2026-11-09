#!/usr/bin/env python3
"""Compare native C1270 and empty C12F0 with physical TU1.5 execution.

Compare all guest GP registers, defined arithmetic flags, every ordered
stack/output store and full retained stack bytes. Source data, including
the original callback table and caller layout, is declared as input.
Positive C12F0 stops before the first unported eligibility/deletion branch.
Run through tools/run_test.py.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import shutil
import signal
import struct
import subprocess
import sys

from generate_physics_live_support_tu15 import complete_body
from oracle_smg_firing_1_1 import Xbe, _Code
from original_audio_cpu import OriginalAudioCpu
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
ENTRY, RET, SP = 0xc1270, 0xc14ba, 0x600ed5dc
HEAP, STACK, RESULT = 0x1000000, SP-32, SP+0x224
SIM, GAME, POOL, HEADS, OBJECTS = (HEAP+n for n in (0, 0x100, 0x200, 0x1000, 0x10000))
KERNEL = 'src/physics/native_tu15_object_recovery.cpp'
TEST = 'tests/native_tu15_object_recovery_tests.cpp'
HEADER = 'include/halo2/native_tu15_object_recovery.hpp'
REGS = ('rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi')


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def packed(*words):
    return struct.pack('<'+'I'*len(words), *words)


def original(scratch, image, report):
    code = _Code(image,'.text')
    allowed = set(complete_body(code, ENTRY).code)|set(complete_body(code,0xc12f0).code)
    table = image.read(0x44a670,8)
    assert table == packed(ENTRY,0xc12f0)
    rng = random.Random(ENTRY)
    profiles = [(f'none-{mode}',mode,0,0,False) for mode in (0,4)]
    profiles += [(f'chain-{mode}-{i}',mode,7+i,i,False) for mode in (0,4) for i in range(4)]
    profiles += [('capacity',0,2049,0,False),('wrong-caller',4,3,3,True)]
    profiles += [('consumer-empty',0,0,0,False),('consumer-negative',0,0,0xffffffff,False),
                 ('consumer-positive',0,0,2,False),('consumer-wrong-caller',0,0,0,True)]
    cases,sites = [],set()
    steps=0
    with OriginalAudioCpu(image,scratch/'original.elf',0x27f,0x1f80,
                          extra_declared_regions=[(0x600c0000,0x40000)],
                          source_bank_bytes=0xa0000) as cpu:
        cpu.trace(6,cpu.debug_offset+7*8,0)
        for name,mode,n,pattern,invalid in profiles:
            consumer=name.startswith('consumer-')
            active=consumer and pattern==2
            entry,ret,sp=(0xc12f0,0xc14e1,SP-12) if consumer else (ENTRY,RET,SP)
            heap=bytearray(rng.randbytes(0x10000+n*0x100))
            stack=bytearray(rng.randbytes(RESULT+0x2800-STACK))
            def put(address,value,size=4):
                off=address-HEAP
                heap[off:off+size]=value.to_bytes(size,'little')
            datums=[0x82d40000+i for i in range(n)]
            put(SIM+8,datums[0] if n else 0xffffffff)
            put(GAME+0xc,mode,1)
            put(POOL+0x44,HEADS)
            for i in range(n):
                body=OBJECTS+i*0x100
                put(HEADS+i*12+8,body)
                put(body+0x14,datums[0] if pattern&1 and i%3==1 else 0xffffffff)
                put(body+0x1c,datums[i+1] if i+1<n else 0xffffffff)
                put(body+0xd4,0x80000000 if pattern&2 and i%3==2 else 0xffffffff)
            reason=3 if consumer else pattern&3
            args=(ret,reason,RESULT,0x2800)
            if consumer:
                args+=(sp+0x2e,sp+0x30,0x200)
                stack[RESULT-STACK:RESULT-STACK+4]=packed(pattern)
            stack[sp-STACK:sp-STACK+len(args)*4]=packed(*args)
            globals_=[(0x4eabc4,packed(SIM)),(0x4f3218,packed(GAME)),(0x4ecbd0,packed(POOL)),
                      (0x44a670,table)]
            for address,bytes_ in (*globals_,(HEAP,heap),(STACK,stack)):
                cpu.write(address,bytes_)
            cpu.prepare(entry,sp,dict(rax=sp+0x2e if consumer else ENTRY,rcx=RESULT,
                                      rdx=sp+0x30 if consumer else 8,rbx=0x2088,
                                      rbp=reason,rsi=0x600ed5ff,rdi=0x44a670))
            regs=cpu.registers()
            start_regs=[getattr(regs,key)&0xffffffff for key in REGS]
            initial_flags=regs.eflags
            stores=[]
            if not invalid:
                for _ in range(60000):
                    pc=regs.rip
                    if pc==(0xc12fe if active else ret):break
                    assert pc in allowed,(name,hex(pc))
                    insn=code.at(pc)
                    assert cpu.read(pc,insn.size)==image.read(pc,insn.size)
                    sites.add(pc)
                    address=None
                    width=4
                    if pc in (0xc1275,0xc1288,0xc128f,0xc1296,0xc12f0,0xc12f1):
                        address=regs.rsp-4
                    elif pc in (0xc127a,0xc12d9):
                        address=regs.rsi
                    elif pc==0xc12d5:
                        address=regs.rsi+regs.rdi*4+4
                    elif pc==0xc1386:
                        address=regs.rcx
                        width=1
                    if address is not None:
                        before=int.from_bytes(cpu.read(address,width),'little')
                    cpu.trace(9)
                    _, status=os.waitpid(cpu.pid,0)
                    assert os.WIFSTOPPED(status) and os.WSTOPSIG(status)==signal.SIGTRAP,(name,hex(pc),status)
                    regs=cpu.registers()
                    if address is not None:
                        after=int.from_bytes(cpu.read(address,width),'little')
                        stores.append([pc,address,before,after])
                    steps+=1
                else:
                    raise AssertionError((name,'original recovery budget'))
                assert regs.rsp==(sp-8 if active else SP+16)
            assert cpu.read(HEAP,len(heap))==heap
            after=cpu.read(STACK,len(stack))
            replay=bytearray(stack)
            for pc,address,old,new in stores:
                off=address-STACK
                width=1 if pc==0xc1386 else 4
                assert 0<=off<=len(replay)-width and replay[off:off+width]==old.to_bytes(width,'little')
                replay[off:off+width]=new.to_bytes(width,'little')
            assert replay==after
            if name=='capacity':
                assert after[RESULT-STACK:RESULT-STACK+4]==packed(2048)
            cases.append(dict(name=name,banks=[[p,v.hex(),False] for p,v in globals_]+
                              [[HEAP,heap.hex(),False],[STACK,stack.hex(),True]],
                              invalid=invalid,refused=invalid or active,consumer=consumer,
                              gp=start_regs,eflags=initial_flags,
                              after_gp=[getattr(regs,key)&0xffffffff for key in REGS],
                              after_flags=regs.eflags&0xcd5,stores=stores,after=after.hex()))
    report.update(original_sites=len(sites),original_steps=steps,cases=len(cases),
                  ordered_stores=sum(len(case['stores']) for case in cases))
    return cases


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert sha(NEW_DEFAULT)==NEW_PIN
    report=dict(passed=False,scope=__doc__,image_sha256=NEW_PIN,original_code_patches=0,
                source_hashes={p:sha(ROOT/p) for p in (KERNEL,TEST,HEADER)},
                builds=[],retained_large_artifacts=[])
    try:
        fixture=scratch/'original.json'
        fixture.write_text(json.dumps(dict(image_sha256=NEW_PIN,cases=original(scratch,Xbe(NEW_DEFAULT),report))))
        report['fixture_sha256']=sha(fixture)
        project=scratch/'project'
        project.mkdir()
        for source in (KERNEL,TEST):
            shutil.copyfile(ROOT/source,project/Path(source).name)
        shutil.copytree(ROOT/'include',project/'include')
        # These direct-only helpers retain the same dependency resolution as
        # the installed native world target.
        (project/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(recovery LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json CONFIG REQUIRED)
add_executable(recovery {Path(KERNEL).name} {Path(TEST).name})
target_include_directories(recovery PRIVATE include "{ROOT/'src'}")
target_compile_features(recovery PRIVATE cxx_std_20)
target_link_libraries(recovery PRIVATE nlohmann_json::nlohmann_json)
add_test(NAME recovery COMMAND recovery "{fixture}")
''')
        kernel=project/Path(KERNEL).name
        clean=kernel.read_text()
        for compiler in ('g++','clang++'):
            for name in ('native','overrun','parent-filter','empty-status'):
                expected=name=='native'
                before,after={
                    'native':('',''),
                    'overrun':('flags.minus(edi,0x800u,4)','flags.minus(edi,0x801u,4)'),
                    'parent-filter':('if(flags.zf) {\n                    write','if(!flags.zf) {\n                    write'),
                    'empty-status':('0xc1386u,ecx,eax,1','0xc1386u,ecx,1u,1'),
                }[name]
                if not expected:assert clean.count(before)==1
                kernel.write_text(clean if expected else clean.replace(before,after))
                dest=output/(compiler+'-'+name)
                cmd=[sys.executable,'-B',str(ROOT/'tools/run_tests.py'),'--source',str(project),
                     '--scratch-root',str(scratch/'builds'),'--output',str(dest),'--target','recovery',
                     '--filter','^recovery$','--jobs','2','--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler]
                with (output/(dest.name+'.log')).open('wb') as stream:
                    proc=subprocess.run(cmd,stdout=stream,stderr=subprocess.STDOUT)
                result=json.loads((dest/'report.json').read_text())
                if not expected:assert 'Original recovery mismatch:' in (dest/'tests.log').read_text()
                report['builds'].append(dict(compiler=compiler,case=name,exit_code=proc.returncode,
                                             passed=result['passed'],expected_pass=expected))
                assert result['passed']==expected and (proc.returncode==0)==expected
        report['passed']=True
    finally:
        (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','cases','ordered_stores','original_steps')}))


if __name__=='__main__':
    main()
