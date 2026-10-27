#!/usr/bin/env python3
"""Bounded unchanged TU1.5 matchmaking widget decisions versus native kernels.

Title and game-icon routines run in full (the icon's bitmap owner is absent).
The option-category selector stops before its original external goof lookup;
no external UI/tag/session callee is replaced or claimed to have executed.
The downloaded-tip LCG/index and signed15-second clock comparison are bounded
original blocks with explicit inputs. Whole screen/helper byte correspondence
is separate from these bounded calls.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_tu15_team_balance import Xbe,NEW_DEFAULT,NEW_PIN,ROOT,STOP


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--fixture',type=Path);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    source=Xbe(NEW_DEFAULT);m=Uc(UC_ARCH_X86,UC_MODE_32);m.mem_map(0x10000,0x800000)
    for section in source.sections:m.mem_write(section.va,source.section_bytes(section))
    m.mem_map(0x1100000,0x10000);m.mem_map(0x2000000,0x10000);m.mem_map(STOP,0x1000)
    owner,stack=0x1100000,0x200ff00;executed=set();m.hook_add(UC_HOOK_CODE,lambda g,a,z,u:executed.add(a))
    def prepare():m.mem_write(stack,struct.pack('<I',STOP));m.reg_write(UC_X86_REG_ESP,stack)
    cases=[]
    inputs=[(v,0,1) for v in (-0x80000000,*range(-2,18),14999,15000,15001,30000,0x7fffffff)]
    inputs.extend((0,seed,count)for seed in (0,1,0x7fffffff,0x80000000,0xffffffff)for count in range(33))
    for value,seed,count in inputs:
        prepare();m.mem_write(owner+0x610,struct.pack('<i',value));m.reg_write(UC_X86_REG_EAX,owner)
        m.emu_start(0x257f2d,STOP,count=1000);assert m.reg_read(UC_X86_REG_EIP)==STOP
        title=m.reg_read(UC_X86_REG_EAX)
        prepare();m.mem_write(owner+0x44,struct.pack('<i',value));m.reg_write(UC_X86_REG_EAX,owner);m.reg_write(UC_X86_REG_ECX,0)
        m.emu_start(0x256d51,STOP,count=1000);assert m.reg_read(UC_X86_REG_EIP)==STOP
        icon=m.reg_read(UC_X86_REG_EDX)&65535
        prepare();m.reg_write(UC_X86_REG_EAX,0);m.reg_write(UC_X86_REG_ECX,owner)
        m.emu_start(0x256c3a,0x256c94,count=1000);assert m.reg_read(UC_X86_REG_EIP)==0x256c94
        group=struct.unpack('<i',m.mem_read(m.reg_read(UC_X86_REG_ESP),4))[0]
        prepare();m.mem_write(0x4f3cd8,struct.pack('<I',owner+0x2000));m.mem_write(owner+0x2004,struct.pack('<I',seed));m.reg_write(UC_X86_REG_ECX,count)
        m.emu_start(0x6f238,0x6f25e,count=1000);assert m.reg_read(UC_X86_REG_EIP)==0x6f25e
        index=m.reg_read(UC_X86_REG_EAX);updated=struct.unpack('<I',m.mem_read(owner+0x2004,4))[0]
        prepare();m.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        m.emu_start(0x25832c,STOP,count=2)
        expired=m.reg_read(UC_X86_REG_EIP)==0x258333
        cases.append(dict(input=value,title=title,category=group,icon=icon,seed=seed,count=count,updated_seed=updated,tip_index=index,tip_clock_expired=expired))
    cpp=scratch/'widgets.cpp';binary=scratch/'widgets'
    cpp.write_text('#include "halo2/original_matchmaking_widgets.hpp"\n#include<iostream>\nint main(){int x;unsigned seed,count;while(std::cin>>x>>seed>>count){auto tip=halo2::original_matchmaking_tip_index(seed,count);halo2::OriginalMatchmakingTipClock clock{0,true};auto expired=clock.update(std::bit_cast<unsigned>(x));std::cout<<halo2::original_matchmaking_title(x)<<" "<<halo2::original_matchmaking_option_category(x).value_or(0xffffffffu)<<" "<<halo2::original_matchmaking_game_icon(x)<<" "<<seed<<" "<<tip.value()<<" "<<expired<<"\\n";}}\n')
    subprocess.run(['c++','-std=c++20','-I',str(ROOT/'include'),str(cpp),'-o',str(binary)],check=True)
    lines=subprocess.check_output([str(binary)],input=('\n'.join(f"{c['input']} {c['seed']} {c['count']}"for c in cases)+'\n').encode()).decode().splitlines()
    for case,line in zip(cases,lines):assert list(map(int,line.split()))==[case['title'],case['category']&0xffffffff,case['icon'],case['updated_seed'],case['tip_index'],int(case['tip_clock_expired'])],(case,line)
    decoder=Cs(CS_ARCH_X86,CS_MODE_32)
    for at in executed:
        instruction=next(decoder.disasm(source.read(at,15),at));assert bytes(m.mem_read(at,instruction.size))==instruction.bytes
    fixture=dict(schema='H2_TU15_MATCHMAKING_WIDGET_DECISIONS_V1',original_sha256=NEW_PIN,cases=cases,
        source_entries=dict(title='0x257F2D',game_icon='0x256D51',category_selector=['0x256C3A','0x256C94'],tip_selector=['0x6F238','0x6F25E'],tip_clock=['0x25832C','0x258333','0x258341']))
    if args.fixture:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(passed=True,cases=len(cases),whole_original_calls=2*len(cases),bounded_category_selectors=len(cases),bounded_original_tip_selectors=len(cases),
        original_instructions=len(executed),original_sha256=NEW_PIN,no_instruction_patches=True,no_replaced_callees=True,
        scope=__doc__,native_source_sha256=hashlib.sha256((ROOT/'include/halo2/original_matchmaking_widgets.hpp').read_bytes()).hexdigest(),scratch=str(scratch),scratch_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
