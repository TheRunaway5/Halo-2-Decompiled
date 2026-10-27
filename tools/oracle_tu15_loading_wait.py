#!/usr/bin/env python3
"""Bound unchanged Xbox map-wait arithmetic, localization and setup controls.

229DB0 executes unchanged through its first actual font-state call boundary;
no callee is replaced. The final ADD WORD rectangle window is separate. The
whole229D30 selector reads an explicit already-selected console language. This
does not execute font rasterization, GPU, map I/O or the loading lifetime.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe,ROOT
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--fixture',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
    stack=0x1000000;g.mem_map(stack,0x20000);sp=stack+0x8000;value=stack+0x1000;seen=set()
    g.hook_add(UC_HOOK_CODE,lambda _,pc,size,user:seen.add(pc))
    native=scratch/'wait.cpp';binary=scratch/'wait'
    native.write_text('#include "halo2/original_loading_wait.hpp"\n#include <iostream>\nint main(){unsigned p,l;int y;while(std::cin>>p>>l>>y){auto r=halo2::original_loading_wait_text(p,l,{short(y),0,480,640},{0x27f,0x1f80});std::cout<<r.percent<<" "<<r.rectangle[0]<<" "<<r.font<<" "<<r.style<<" "<<r.justification<<" "<<r.flags;for(auto c:r.text)std::cout<<" "<<unsigned(c);std::cout<<"\\n";}}\n')
    subprocess.run(['clang++','-std=c++20','-O1','-I'+str(ROOT/'include'),str(native),'-o',str(binary)],check=True)
    values=[0,0x80000000,0x3dcccccd,0x3f000000,0x3f800000,0xbf800000,0x7fc00000,0x7f800000,0xff800000,0x4b000000]
    rng=random.Random(0x229db0);inputs=[(v,l,t)for v in values for l in(0,1,2,3,4,5,6,7,8)for t in(0,32700)]
    inputs.extend((rng.getrandbits(32),rng.randrange(10),rng.randrange(-32768,32768))for _ in range(160))
    rows=[];lines=[]
    for bits,language,top in inputs:
        g.mem_write(value,struct.pack('<I',bits));g.mem_write(sp,struct.pack('<II',0,value))
        g.reg_write(UC_X86_REG_ESP,sp);g.reg_write(UC_X86_REG_EFLAGS,0x202);g.reg_write(UC_X86_REG_FPCW,0x27f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
        g.emu_start(0x229db0,0x141890,count=120)
        assert g.reg_read(UC_X86_REG_EIP)==0x141890
        percent=struct.unpack('<i',struct.pack('<I',g.reg_read(UC_X86_REG_EBX)))[0]
        font=g.reg_read(UC_X86_REG_ESI);args=struct.unpack('<5I',g.mem_read(g.reg_read(UC_X86_REG_ESP),20))
        assert args[0]==0x229e75 and (font,args[1:4])==(3,(0xffffffff,2,0))
        assert bytes(g.mem_read(args[4],16))==struct.pack('<4f',1,1,1,1)
        assert bytes(g.mem_read(g.reg_read(UC_X86_REG_EDI),16))==struct.pack('<4I',0x3f800000,0x3c808081,0x3cc0c0c1,0x3d20a0a1)
        g.mem_write(value,struct.pack('<h',top));g.reg_write(UC_X86_REG_ESP,value-0x24)
        g.emu_start(0x229ed2,0x229ed9,count=1);y=struct.unpack('<h',g.mem_read(value,2))[0]
        g.mem_write(0x48c750,struct.pack('<I',language));g.mem_write(sp,struct.pack('<I',0x1000))
        g.reg_write(UC_X86_REG_ESP,sp);g.emu_start(0x229d30,0x1000,count=100)
        address=g.reg_read(UC_X86_REG_EAX);caption=[]
        for i in range(64):
            c=struct.unpack('<H',g.mem_read(address+i*2,2))[0]
            if not c:break
            caption.append(c)
        caption.extend(map(ord,' '+str(percent)+'%'))
        expected=[percent,y,font,-1,2,0,*caption];rows.append(dict(input=[bits,language,top],output=expected));lines.append(f'{bits} {language} {top}\n')
    output=subprocess.check_output([str(binary)],input=''.join(lines),text=True).splitlines()
    assert len(output)==len(rows)
    for row,line in zip(rows,output):assert list(map(int,line.split()))==row['output'],row
    fixture=dict(schema='H2_TU15_LOADING_WAIT_V1',original_sha256=NEW_PIN,scope=__doc__,cases=rows)
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(fixture,indent=2)+'\n')
    report=dict(passed=True,scope=__doc__,original_sha256=NEW_PIN,controls=len(rows),unchanged_instruction_sites=len(seen),
        fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),scratch=str(scratch),scratch_retained=False,
        native_renderer_or_lifecycle_proven=False)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
