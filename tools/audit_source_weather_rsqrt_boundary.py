#!/usr/bin/env python3
"""Numerical boundary of original 38B02 RSQRTSS, without a fitted replacement.

Execute exactly the unchanged source instruction on eight explicit squared-speed
inputs and compare with the host's actual SSE intrinsic. The different words
are a Unicorn/host-CPU boundary, not evidence of an Xbox instruction answer.
No full 38450 particle update, resource publication or readiness is claimed.
"""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys, tempfile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
CPP='''#include <bit>
#include <cstdint>
#include <iostream>
#include <xmmintrin.h>
int main(){_mm_setcsr(0x1f80);std::uint32_t b;
while(std::cin>>std::hex>>b){
const auto v=_mm_rsqrt_ss(_mm_set_ss(std::bit_cast<float>(b)));
std::cout<<std::hex<<std::bit_cast<std::uint32_t>(_mm_cvtss_f32(v))<<"\\n";}}
'''
def sha(raw):return hashlib.sha256(raw).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    xbe=Xbe(ROOT/'assets/disc/default.xbe');assert sha(xbe.data)==PIN
    source=xbe.read(0x38b02,6);assert source.hex()=='f30f52442440'
    values=[0x3f800000,0x40000000,0x40800000,0x41100000,
        0x42100000,0x42c80000,0x461c4000,0x3cf5c28f]
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x38000,4096);u.mem_write(0x38b02,source)
    u.mem_map(0x2000000,4096);stack=0x2000100;executed=[]
    def code(g,pc,size,data):
        assert pc==0x38b02 and size==6
        assert bytes(g.mem_read(pc,size))==source
        executed.append(pc)
    u.hook_add(UC_HOOK_CODE,code)
    original=[]
    for value in values:
        u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_XMM0,0)
        u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.mem_write(stack+0x40,value.to_bytes(4,'little'))
        u.emu_start(0x38b02,0x38b08,count=1)
        assert u.reg_read(UC_X86_REG_EIP)==0x38b08 and u.reg_read(UC_X86_REG_ESP)==stack
        assert bytes(u.mem_read(stack+0x40,4))==value.to_bytes(4,'little')
        original.append(u.reg_read(UC_X86_REG_XMM0)&0xffffffff)
    with tempfile.TemporaryDirectory(prefix='halo2-weather-rsqrt-')as directory:
        directory=Path(directory);source_path=directory/'native.cpp';binary=directory/'native'
        source_path.write_text(CPP)
        compile_args=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',str(source_path),'-o',str(binary)]
        subprocess.run(compile_args,text=True,capture_output=True,check=True)
        native=subprocess.run([str(binary)],input='\n'.join(f'{v:08X}'for v in values)+'\n',
            text=True,capture_output=True,check=True)
        actual=[int(v,16)for v in native.stdout.split()];assert len(actual)==len(values)
        disassembly=subprocess.run(['objdump','-d',str(binary)],text=True,capture_output=True,check=True).stdout
        native_instructions=[line.strip()for line in disassembly.splitlines()if 'rsqrtss' in line]
        assert len(native_instructions)==1,'native intrinsic must emit one actual RSQRTSS'
        binary_hash=sha(binary.read_bytes())
    cases=[dict(explicit_squared_speed=f'{value:08X}',unchanged_source_unicorn=f'{old:08X}',
        native_sse=f'{new:08X}',equal=old==new)for value,old,new in zip(values,original,actual)]
    report=dict(boundary=__doc__,source_xbe_sha256=PIN,source_pc='00038B02',
        original_instruction=source.hex(),original_instruction_sha256=sha(source),
        source_instruction_executions=len(executed),cases=cases,equal_cases=sum(row['equal']for row in cases),
        native_intrinsic='_mm_rsqrt_ss',native_instruction_disassembly=native_instructions,
        native_source=CPP,native_source_sha256=sha(CPP.encode()),native_binary_sha256=binary_hash,
        compiler=subprocess.run(['c++','--version'],text=True,capture_output=True,check=True).stdout.splitlines()[0],
        native_compile_flags=['-std=c++20','-O2','-fno-fast-math','-ffp-contract=off'],mxcsr='1F80',
        tool_sha256=sha(Path(__file__).read_bytes()),
        dependency_audit_sha256=sha((ROOT/'analysis/multiplayer-shading/weather-simulation-dependency-snapshot-original.json').read_bytes()),
        original_instruction_patches=[],service_replacements=[],approximation_fitted=False,
        whole_particle_update_proved=False,physical_xbox_result_proved=False,live_activation=False)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(cases=len(cases),equal_cases=report['equal_cases'],source_pc=report['source_pc'],
        whole_particle_update_proved=False,physical_xbox_result_proved=False),indent=2))

if __name__=='__main__':main()
