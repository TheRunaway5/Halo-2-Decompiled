#!/usr/bin/env python3
"""Unchanged Xbox TU1.5 PCR intro, actual clock read and post-tab clock stamp.

The draw controls execute23A0F3 and its original796B0 elapsed-time callee,
stopping before the real14A818 window draw. The clock controls execute the
post-tab initializer239F39 through its real79690 clock callee. Existing source
clock-override cells supply deterministic input; no instructions or callees
are replaced. Widget initialization and NV2A drawing are not claimed here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EBX,
    UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--fixture',type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    uc=Uc(UC_ARCH_X86,UC_MODE_32)
    uc.mem_map(0x10000,0x800000)
    for section in image.sections:uc.mem_write(section.va,image.section_bytes(section))
    owner,stack,returned=0x1100000,0x112f000,0x110ff00
    uc.mem_map(owner,0x30000)
    stops=set();visited=set()
    def observe(machine,pc,size,_):
        if pc in stops:machine.emu_stop()
        else:visited.add((pc,bytes(machine.mem_read(pc,size))))
    uc.hook_add(UC_HOOK_CODE,observe)
    def execute(start,ends):
        stops.clear();stops.update(ends)
        uc.emu_start(start,0x3000000,count=200)
        actual=uc.reg_read(UC_X86_REG_EIP)
        assert actual in ends,hex(actual)
        return actual
    def clock(now):
        #79690/796B0's actual existing source override, not an import stub.
        uc.mem_write(0x51d048,b'\x01')
        uc.mem_write(0x51d04c,struct.pack('<I',now))
    stamps=[]
    for now in (0,1,1999,2000,3999,4000,0xfffffff0,0xffffffff):
        clock(now);uc.reg_write(UC_X86_REG_ESP,stack)
        uc.reg_write(UC_X86_REG_EDI,owner);uc.reg_write(UC_X86_REG_EBX,0)
        execute(0x239f39,{0x239f4a})
        actual=struct.unpack('<I',uc.mem_read(owner+0x5ef4,4))[0]
        assert actual==now;stamps.append(dict(now=now,opened=actual))
    clocks=[(0x12345678,elapsed) for elapsed in
        (0,1999,2000,2001,2500,3000,3999,4000,4001,0x7fffffff,0x80000000,0xffffffff)]
    clocks += [(0xfffffff0,elapsed) for elapsed in (1999,2000,3999,4000)]
    rows=[]
    for rounding in range(4):
        word=0x27f|(rounding<<10);mxcsr=0x1f80|(rounding<<13)
        for prior in (False,True):
            for opened,elapsed in clocks:
                now=(opened+elapsed)&0xffffffff
                clock(now)
                uc.mem_write(owner+0x5ef0,bytes([int(prior)]))
                uc.mem_write(owner+0x5ef4,struct.pack('<I',opened))
                uc.mem_write(stack,struct.pack('<II',returned,owner+0x8000))
                uc.reg_write(UC_X86_REG_ESP,stack);uc.reg_write(UC_X86_REG_ECX,owner)
                uc.reg_write(UC_X86_REG_FPCW,word);uc.reg_write(UC_X86_REG_MXCSR,mxcsr)
                end=execute(0x23a0f3,{0x14a818,returned})
                color=None if end==returned else struct.unpack('<I',uc.mem_read(uc.reg_read(UC_X86_REG_ESP)+4,4))[0]
                assert (color is None)==prior
                rows.append(dict(prior_stats=prior,opened=opened,now=now,
                    x87_control_word=word,mxcsr=mxcsr,color=color))
    source=scratch/'pcr-intro.cpp';binary=scratch/'pcr-intro'
    source.write_text('#include "halo2/original_pcr_intro.hpp"\n#include <iostream>\n'
        'int main(){unsigned p,o,n,w,m;while(std::cin>>p>>o>>n>>w>>m){'
        'auto c=halo2::original_pcr_intro_color(p,o,n,w,m);'
        'if(c)std::cout<<*c;else std::cout<<"none";std::cout<<"\\n";}}\n')
    subprocess.run(['c++','-std=c++20','-O2','-Iinclude',str(source),'-o',str(binary)],check=True)
    actual=subprocess.check_output([str(binary)],input=''.join(
        f"{int(r['prior_stats'])} {r['opened']} {r['now']} {r['x87_control_word']} {r['mxcsr']}\n"
        for r in rows).encode()).decode().splitlines()
    expected=['none' if r['color'] is None else str(r['color']) for r in rows]
    assert actual==expected,next(((rows[i],a,b)for i,(a,b)in enumerate(zip(actual,expected))if a!=b),None)
    assert all(image.read(pc,len(raw))==raw for pc,raw in visited)
    fixture=dict(schema='H2_ORIGINAL_TU15_PCR_INTRO_1',source_sha256=NEW_PIN,
        code_sha256={hex(pc):hashlib.sha256(image.read(pc,size)).hexdigest()
            for pc,size in ((0x23a0f3,120),(0x796b0,31),(0x239f39,17),(0x79690,20))},cases=rows,clock_stamps=stamps)
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(passed=True,source_sha256=NEW_PIN,controls=len(rows),clock_stamp_controls=len(stamps),
        unchanged_original_instructions=len(visited),fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        native_kernel_sha256=hashlib.sha256(Path('include/halo2/original_pcr_intro.hpp').read_bytes()).hexdigest(),
        scope=__doc__,no_instruction_patches=True,no_replaced_callees=True,
        original_widgets_executed=False,original_draw_device_executed=False,scratch_owner=str(scratch))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f"PASS{len(rows)} unchanged TU1.5 PCR16/185 intro controls; {len(stamps)} real post-tab clock-stamp controls")


if __name__=='__main__':main()
