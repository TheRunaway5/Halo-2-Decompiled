#!/usr/bin/env python3
"""Differential unchanged Xbox loading arithmetic versus the native kernels.

The four2CF16C arithmetic windows run between real external query boundaries;
their map/duration/clock/secondary values are explicit inputs.288C0 runs whole.
No GPU, I/O, readiness, transition completion or clock owner is substituted.
Run via tools/run_test.py; native compiler products remain scratch-owned.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess

from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBP, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_XMM2, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
from oracle_tu15_team_balance import Xbe, NEW_DEFAULT, NEW_PIN, ROOT, STOP


def word(value):return struct.unpack('<I',struct.pack('<f',value))[0]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--fixture',type=Path);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    source=Xbe(NEW_DEFAULT);machine=Uc(UC_ARCH_X86,UC_MODE_32)
    machine.mem_map(0x10000,0x800000)
    for section in source.sections:machine.mem_write(section.va,source.section_bytes(section))
    machine.mem_map(0x2000000,0x10000);machine.mem_map(STOP,0x1000)
    stack=0x200f000;executed=set()
    machine.hook_add(UC_HOOK_CODE,lambda _,a,z,u:executed.add(a))
    def setup():
        machine.mem_write(stack-0x100,bytes(0x200))
        machine.reg_write(UC_X86_REG_FPCW,0x027f);machine.reg_write(UC_X86_REG_MXCSR,0x1f80)
    controls=[]
    for baseline,map_progress,secondary,duration,previous,clock in itertools.product(
        [word(-1),word(0),word(.5),word(.999),word(1)],
        [word(0),word(.5),word(1),0x7f800000,0x7fc00000],
        [word(0),word(1)],[-1,1000],[0,0xffffffff],[0,1,33,1000]):
        setup();progress=word(.1)
        machine.mem_write(0x515e10,struct.pack('<III',baseline,progress,previous))
        machine.mem_write(stack,struct.pack('<III',0,STOP,0))
        machine.reg_write(UC_X86_REG_EBP,stack);machine.reg_write(UC_X86_REG_ESP,stack-16)
        machine.reg_write(UC_X86_REG_XMM0,map_progress)
        machine.emu_start(0x2cf17f,0x2cf191,count=1000)
        machine.reg_write(UC_X86_REG_EAX,duration&0xffffffff)
        machine.emu_start(0x2cf196,0x2cf19f,count=1000)
        machine.reg_write(UC_X86_REG_EAX,clock)
        machine.emu_start(0x2cf1a4,0x2cf266,count=1000)
        machine.reg_write(UC_X86_REG_XMM0,secondary)
        machine.emu_start(0x2cf26b,STOP,count=1000)
        assert machine.reg_read(UC_X86_REG_EIP)==STOP
        output=struct.unpack('<III',machine.mem_read(0x515e10,12))
        controls.append(dict(kind=0,input=[baseline,progress,previous,map_progress,duration,clock,secondary],output=list(output)))
    for previous,delta,value in itertools.product([word(0),word(.2),word(1),0x7fc00000],
        [word(0),word(.1),0x7f800000,0x7fc00000],
        [word(-1),0x80000000,word(0),word(.5),word(.998),word(.999),word(1),word(2),0x7fc00000]):
        setup();machine.mem_write(stack,struct.pack('<I',STOP));machine.reg_write(UC_X86_REG_ESP,stack)
        machine.mem_write(0x4c61c4,struct.pack('<II',previous,delta))
        machine.reg_write(UC_X86_REG_XMM2,value);machine.emu_start(0x288c0,STOP,count=1000)
        assert machine.reg_read(UC_X86_REG_EIP)==STOP
        output=[struct.unpack('<I',machine.mem_read(0x4c61c8,4))[0],machine.reg_read(UC_X86_REG_XMM0)&0xffffffff]
        controls.append(dict(kind=1,input=[previous,delta,value],output=output))
    cpp=scratch/'loading.cpp';binary=scratch/'loading'
    cpp.write_text('''#include "halo2/original_loading_progress.hpp"
#include<iostream>
using namespace halo2;
int main(){unsigned kind;while(std::cin>>kind){if(!kind){OriginalLoadingProgress s;OriginalLoadingProgressQueries q;
std::cin>>s.baseline_bits>>s.progress_bits>>s.previous_clock>>q.map_progress_bits>>q.duration>>q.clock>>q.secondary_progress_bits;
original_loading_progress_update(s,q,{0x027f,0x1f80});std::cout<<s.baseline_bits<<" "<<s.progress_bits<<" "<<s.previous_clock<<"\\n";
}else{OriginalLoadingFilter s;unsigned input;std::cin>>s.previous_bits>>s.delta_bits>>input;
auto result=original_loading_filtered_progress(s,input,{0x027f,0x1f80});std::cout<<s.delta_bits<<" "<<result<<"\\n";}}}
''')
    subprocess.run(['c++','-std=c++20','-I',str(ROOT/'include'),str(cpp),'-o',str(binary)],check=True)
    lines=subprocess.check_output([str(binary)],input=('\n'.join(' '.join(map(str,[c['kind'],*c['input']]))for c in controls)+'\n').encode()).decode().splitlines()
    assert len(lines)==len(controls)
    for case,line in zip(controls,lines):assert list(map(int,line.split()))==case['output'],(case,line)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for at in executed:
        instruction=next(decoder.disasm(source.read(at,15),at))
        assert bytes(machine.mem_read(at,instruction.size))==instruction.bytes
    fixture=dict(schema='H2_TU15_LOADING_ARITHMETIC_V1',original_sha256=NEW_PIN,cases=controls)
    if args.fixture:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,controls=len(controls),
        bounded_progress_controls=sum(c['kind']==0 for c in controls),whole_filter_calls=sum(c['kind']==1 for c in controls),
        original_instructions=len(executed),no_instruction_patches=True,no_replaced_callees=True,
        native_header_sha256=hashlib.sha256((ROOT/'include/halo2/original_loading_progress.hpp').read_bytes()).hexdigest(),
        scratch=str(scratch),scratch_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
