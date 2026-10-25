#!/usr/bin/env python3
"""Original goal circle and polygon wall CPU outputs at real GPU boundaries.

Runs unchanged24D760 to1A9290, the unchanged1A9290 visibility/width prefix
to its first GPU service, and the unchanged CPU emission interval1A94BD..
1A9822 with declared vertex-buffer/caller inputs. No callee returns are
supplied and no original SDK/GPU execution is claimed.
"""
import argparse
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as x

B, SP, STOP = 0x4000000, 0x4018000, 0x10000

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
    image=Xbe(NEW_DEFAULT);cpu=Uc(UC_ARCH_X86,UC_MODE_32);cpu.mem_map(0x10000,0x1000000);cpu.mem_map(B,0x20000)
    for section in image.sections:cpu.mem_write(section.va,image.section_bytes(section))
    def write(at,value):cpu.mem_write(at,struct.pack('<I',value&0xffffffff))
    def word(at):return struct.unpack('<I',cpu.mem_read(at,4))[0]
    def floats(at,values):cpu.mem_write(at,struct.pack('<'+'f'*len(values),*values))
    boundaries=set();visited=[]
    def hook(uc,at,size,user):
        visited.append(at)
        if at in boundaries:uc.emu_stop()
    cpu.hook_add(UC_HOOK_CODE,hook)
    def run(entry,stop,regs,fp):
        boundaries.clear();boundaries.update(stop);visited.clear()
        for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI'):cpu.reg_write(getattr(x,'UC_X86_REG_'+name),0)
        for name,value in regs.items():cpu.reg_write(getattr(x,'UC_X86_REG_'+name),value)
        cpu.reg_write(x.UC_X86_REG_EFLAGS,2);cpu.reg_write(x.UC_X86_REG_FPCW,fp[0]);cpu.reg_write(x.UC_X86_REG_FPSW,0)
        cpu.reg_write(x.UC_X86_REG_FPTAG,0xffff);cpu.reg_write(x.UC_X86_REG_MXCSR,fp[1]);cpu.emu_start(entry,STOP,count=50000)
        assert cpu.reg_read(x.UC_X86_REG_EIP) in {*stop,STOP}
        return cpu.reg_read(x.UC_X86_REG_EIP)
    cases=[]
    for center,radius,designator,fp,height in itertools.product(((0.,0.,0.),(12.25,-4.5,2.75)),(.125,1.,4.),(0,2,-1,-3),((0x27f,0x1f80),(0x67f,0x1f80)),(.4,.8)):
        cpu.mem_write(B,bytes(0x20000));write(0x4ecc20,B);write(B+0x11c,B+0x2000);write(0x52d710,B+0x1000)
        index=designator if designator>=0 else ~designator
        cpu.mem_write(B+0x1000+(0x14 if designator>=0 else 0x26)+index*2,struct.pack('<h',0))
        floats(B+0x1000+(0x3c if designator>=0 else 0xa8)+index*12,(radius,))
        floats(B+0x2000,center);write(SP,STOP)
        for i in range(1,8):write(SP+i*4,i*111)
        write(SP+0x18,designator);run(0x24d760,{0x1a9290},{'ESP':SP},fp)
        call=cpu.reg_read(x.UC_X86_REG_ESP);points=bytes(cpu.mem_read(word(call+12),32*8))
        assert word(call+16)==32 and word(call+24)==struct.unpack('<I',struct.pack('<f',radius))[0]
        perimeter=word(call+28);rgb=(.25,.5,.75);eye=(center[0]+radius,center[1],center[2]+.2)
        # Actual1A9290 prolog/prefix, stopping before the first GPU owner.
        entry_sp=B+0x16000;write(entry_sp,STOP)
        for i,value in enumerate((0,0,B+0x3000,32,B+0x4000,word(call+24),perimeter,B+0x5000,struct.unpack("<I",struct.pack("<f",height))[0])):
            write(entry_sp+4+i*4,value)
        cpu.mem_write(B+0x3000,points);floats(B+0x4000,center);floats(B+0x5000,rgb);floats(0x4c65c0,eye)
        stopped=run(0x1a9290,{0x1a934c},{'ESP':entry_sp},fp);assert stopped==0x1a934c
        stack=cpu.reg_read(x.UC_X86_REG_ESP);outer=word(stack+0x10);inner=word(stack+0x14)
        # The GPU-prepared CPU interval has a declared empty immediate buffer
        # and semantic3 binding. It executes without any external calls.
        stack=B+0x16000;cpu.mem_write(stack,bytes(0x100));write(stack+0x10,outer);write(stack+0x14,inner)
        for at,value in ((0x44,0),(0x4c,B+0x3000),(0x50,32),(0x54,B+0x4000),(0x5c,perimeter),(0x60,B+0x5000),(0x64,struct.unpack("<I",struct.pack("<f",height))[0])):
            write(stack+at,value)
        write(0x411568,B+0x9000);write(B+0x9000,B+0xa000);write(B+0x9004,B+0xe000)
        write(0x51df54,B+0x6000);cpu.mem_write(B+0x6002,struct.pack('<h',30));write(B+0x6008,10117)
        run(0x1a94bd,{0x1a9822},{'ESP':stack,'EAX':B+0xa000,'ESI':B+0x9000},fp)
        assert all(0x1a94bd<=pc<=0x1a9822 for pc in visited)
        p=B+0xa000;assert word(p)==0x417fc and word(p+4)==6;p+=8;vertices=[]
        for pair in range(33):
            assert word(p)==0x101ab0;color=[word(p+4+i*4) for i in range(4)];p+=20
            for n in range(2):
                assert word(p)==0x81880;uv=[word(p+4),word(p+8)];p+=12
                assert word(p)==0x101a00;position=[word(p+4+i*4) for i in range(4)];p+=20
                vertices.append(position+uv+color)
        assert p==word(B+0x9000)
        cases.append(dict(center=center,radius=radius,designator=designator,eye=eye,color=rgb,fp=fp,height=height,
            points=list(struct.unpack('<64I',points)),perimeter=perimeter,outer=outer,inner=inner,vertices=vertices))
    result=dict(schema='H2_OBJECTIVE_GOAL_GEOMETRY_TU15_V1',original_sha256=NEW_PIN,supplied_service_results=[],cases=cases)
    args.output.write_text(json.dumps(result,separators=(',',':'))+'\n');print(len(cases),'original circle/prefix/66-vertex CPU cases',args.output.stat().st_size,'bytes')
if __name__=='__main__':main()
