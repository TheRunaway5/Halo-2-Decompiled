#!/usr/bin/env python3
"""Whole unchanged12F110 with explicit empty cache/clock ABI inputs.

Exercises counter reset, frame rollover, cache epochs and timed scale updates.
GetTickCount executes its original three instructions over supplied clock data.
No service or instruction is replaced. Live texture I/O and GPU use are separate.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 g.mem_map(0x1000000,0x20000);stack,stop=0x101f000,0x101ff00
 def w(at,v):g.mem_write(at,struct.pack('<I',v&0xffffffff))
 gp_regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
 rows=[];trace=[];g.hook_add(UC_HOOK_CODE,lambda guest,pc,size,data:trace.append(pc))
 for milliseconds in (0,199,200,201,1000):
  for frame in (0,7,0xffffffff):
   for requests in (0,10,512):
    trace.clear();g.mem_write(0x1000000,bytes(0x8000));g.mem_write(0x101e000,bytes(0x2000));g.mem_write(0x4f2d24,bytes(0x38))
    w(0x4f2d24,0x1001000);w(0x4f2d28,0x1002000);w(0x4f2d34,0x1003000)
    g.mem_write(0x1001029,b'\1');g.mem_write(0x1002029,b'\1')
    w(0x1003030,0x100);w(0x1003064,0x1004000);w(0x1003038,17)
    w(0x41b774,0x1007000);w(0x1007000,milliseconds);w(0x4f2d50,200);w(0x4f2d54,requests);w(0x4f2d58,frame)
    w(stack,stop);entry=[0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde]
    banks=[(0x41b774,4),(0x4f2d24,0x38),(0x4ef1c0,4),(0x474da1,1),(0x46a000,4096),(0x1000000,0x8000),(0x101e000,0x2000)]
    initial=[dict(address=at,hex=bytes(g.mem_read(at,n)).hex())for at,n in banks]
    for reg,value in zip(gp_regs,entry):g.reg_write(reg,value)
    for n in range(8):g.reg_write(UC_X86_REG_XMM0+n,0)
    for reg,value in [(UC_X86_REG_EFLAGS,0x202),(UC_X86_REG_FPCW,0x27f),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(reg,value)
    g.emu_start(0x12f110,stop,count=3000);assert g.reg_read(UC_X86_REG_EIP)==stop
    assert struct.unpack('<I',g.mem_read(0x4f2d54,4))[0]==0
    rows.append(dict(milliseconds=milliseconds,frame=frame,requests=requests,entry_gp=entry,
     exit_gp=[g.reg_read(reg)for reg in gp_regs],eflags=g.reg_read(UC_X86_REG_EFLAGS),
     exit_xmm=[f'{g.reg_read(UC_X86_REG_XMM0+n):032x}'for n in range(8)],instructions=trace.copy(),
     initial_banks=initial,final_banks=[dict(address=at,hex=bytes(g.mem_read(at,n)).hex())for at,n in banks]))
 a.fixture.write_text(json.dumps(dict(scope=__doc__,image_sha256=PIN,entry=0x12f110,instruction_patches=[],supplied_services=[],cases=rows))+'\n')
 print('PASS',len(rows),'whole original texture-cache frame/clock cases')
if __name__=='__main__':main()
