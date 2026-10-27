#!/usr/bin/env python3
"""Unchanged TU1.5 map-UI intro, map-init stamp and delta-clock producer.

Executes14A70D..14A7A3, its real13B2E0 game query, the actual14A3B7
new-map clock stamp, and14A500's delta conversion through32CC0C. Stops
before the original draw/subsystem services; no instructions/callees replaced.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_EBP, UC_X86_REG_EBX,
    UC_X86_REG_ESI, UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_FPCW, UC_X86_REG_MXCSR)
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
    assert image.read(0x46a180,4)==struct.pack('<I',0x3a83126f)
    uc=Uc(UC_ARCH_X86,UC_MODE_32);uc.mem_map(0x10000,0x800000)
    for section in image.sections:uc.mem_write(section.va,image.section_bytes(section))
    owner,stack=0x1100000,0x112f000;uc.mem_map(owner,0x30000)
    uc.mem_write(0x4f3218,struct.pack('<I',owner))
    stops=set();visited=set()
    def observe(machine,pc,size,_):
        if pc in stops:machine.emu_stop()
        else:visited.add((pc,bytes(machine.mem_read(pc,size))))
    uc.hook_add(UC_HOOK_CODE,observe)
    def execute(start,ends):
        stops.clear();stops.update(ends);uc.emu_start(start,0x3000000,count=300)
        end=uc.reg_read(UC_X86_REG_EIP);assert end in ends,hex(end);return end
    stamps=[]
    for now in (0,1,2999,3000,3999,4000,0xfffffff0,0xffffffff):
        uc.mem_write(0x55c448,struct.pack('<I',now))
        uc.reg_write(UC_X86_REG_ESP,stack)
        execute(0x14a3b7,{0x14a3c7})
        actual=struct.unpack('<I',uc.mem_read(0x55c450,4))[0]
        assert actual==now;stamps.append(dict(ui_clock=now,initialized=actual))
    clocks=[(0x12345678,e)for e in
        (0,2999,3000,3001,3500,3999,4000,4001,0x7fffffff,0xffffffff)]
    clocks += [(0xfffffff0,e)for e in (2999,3000,3999,4000)]
    rows=[]
    for context in range(12):
        precision=(0x7f,0x27f,0x37f)[context//4];rounding=context%4
        word=precision|(rounding<<10);sse=0x1f80|(rounding<<13)
        for output in (-1,4):
            for active in (False,True):
                for game_type in (1,2,3):
                    uc.mem_write(owner+8,struct.pack('<I',game_type))
                    uc.mem_write(owner+0x1120,bytes([active]))
                    for initialized,elapsed in clocks:
                        now=(initialized+elapsed)&0xffffffff
                        uc.mem_write(0x55c448,struct.pack('<II',now,0))
                        uc.mem_write(0x55c450,struct.pack('<I',initialized))
                        uc.reg_write(UC_X86_REG_ESP,stack)
                        uc.reg_write(UC_X86_REG_EBP,stack+0x100)
                        uc.reg_write(UC_X86_REG_EBX,output&0xffffffff)
                        uc.reg_write(UC_X86_REG_ESI,4)
                        uc.reg_write(UC_X86_REG_FPCW,word);uc.reg_write(UC_X86_REG_MXCSR,sse)
                        end=execute(0x14a70d,{0x14a818,0x14a7a3,0x14a7f2})
                        color=None if end!=0x14a818 else struct.unpack('<I',uc.mem_read(uc.reg_read(UC_X86_REG_ESP)+4,4))[0]
                        rows.append(dict(initialized=initialized,ui_clock=now,
                            output_user=output,game_in_progress=active,game_type=game_type,
                            x87_control_word=word,mxcsr=sse,color=color))
    deltas=[]
    for elapsed_bits in (0,0x3a83126f,0x3c888889,0x3d088889,0x3f800000,0x407fef9e):
        for previous in (0,0xfffffff0):
            uc.mem_write(0x55c448,struct.pack('<I',previous))
            uc.mem_write(stack,struct.pack('<II',0x110ff00,elapsed_bits))
            uc.reg_write(UC_X86_REG_ESP,stack)
            uc.reg_write(UC_X86_REG_FPCW,0x27f);uc.reg_write(UC_X86_REG_MXCSR,0x1f80)
            execute(0x14a500,{0x14a517})
            actual=struct.unpack('<I',uc.mem_read(0x55c448,4))[0]
            expected=(previous+int(struct.unpack('<f',struct.pack('<I',elapsed_bits))[0]*1000))&0xffffffff
            assert actual==expected,(elapsed_bits,actual,expected)
            deltas.append(dict(previous=previous,delta_binary32=elapsed_bits,ui_clock=actual))
    source=scratch/'ui-map-intro.cpp';binary=scratch/'ui-map-intro'
    source.write_text('#include "halo2/original_ui_map_intro.hpp"\n#include <iostream>\n'
        'int main(){unsigned o,n,a,t,w,m;int u;while(std::cin>>o>>n>>u>>a>>t>>w>>m){'
        'auto c=halo2::original_ui_map_intro_color(o,n,u,a,t,w,m);'
        'if(c)std::cout<<*c;else std::cout<<"none";std::cout<<"\\n";}}\n')
    subprocess.run(['c++','-std=c++20','-O2','-Iinclude',str(source),'-o',str(binary)],check=True)
    actual=subprocess.check_output([str(binary)],input=''.join(
        f"{r['initialized']} {r['ui_clock']} {r['output_user']} {int(r['game_in_progress'])} {r['game_type']} {r['x87_control_word']} {r['mxcsr']}\n"
        for r in rows).encode()).decode().splitlines()
    expected=['none'if r['color']is None else str(r['color'])for r in rows]
    assert actual==expected,next(((rows[i],a,b)for i,(a,b)in enumerate(zip(actual,expected))if a!=b),None)
    assert all(image.read(pc,len(raw))==raw for pc,raw in visited)
    fixture=dict(schema='H2_ORIGINAL_TU15_UI_MAP_INTRO_1',source_sha256=NEW_PIN,
        cases=rows,map_stamps=stamps,delta_clock=deltas)
    args.fixture.parent.mkdir(parents=True,exist_ok=True)
    args.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(passed=True,scope=__doc__,source_sha256=NEW_PIN,controls=len(rows),
        map_stamp_controls=len(stamps),delta_clock_controls=len(deltas),
        unchanged_original_instructions=len(visited),no_instruction_patches=True,
        no_replaced_callees=True,original_draw_device_executed=False,
        fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
        native_kernel_sha256=hashlib.sha256(Path('include/halo2/original_ui_map_intro.hpp').read_bytes()).hexdigest(),scratch_owner=str(scratch))
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'PASS{len(rows)} unchanged map-UI intro controls; {len(stamps)} map stamps; {len(deltas)} delta-clock controls')


if __name__=='__main__':main()
