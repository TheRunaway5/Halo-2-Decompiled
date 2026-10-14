#!/usr/bin/env python3
"""Whole unchanged180520 traversal, cached epoch and compound alpha propagation.

Inputs are explicit raw pool/clock/cache/stack ABI records. Unexpired records
exercise full17FAF0/17FB60/32CC0C without supplied services or instruction
patches. This does not prove actual decal allocation, expiry or GPU behavior.
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
 for tick in (0,1,7,14,29):
  for decay in (0.,.25,.5,1.):
   for compound in (False,True):
    for permanent in (False,True):
     for epoch in (0,65535):
      trace.clear();g.mem_write(0x1000000,bytes(0x8000));g.mem_write(0x101e000,bytes(0x2000))
      w(0x4f7220,0x1002000);w(0x1002024,64);w(0x1002038,2 if compound else 1)
      w(0x1002044,0x1003000);w(0x1002048,0x1005000);w(0x1005000,3 if compound else 1)
      g.mem_write(0x1002029,b'\1');w(0x51df54,0x1004000);w(0x1004008,tick)
      w(0x1004004,struct.unpack('<I',struct.pack('<f',1/30))[0]);w(0x515f28,0x1006000);w(0x1006038,epoch)
      for n in range(2 if compound else 1):
       body=0x1003000+n*64;g.mem_write(body,struct.pack('<HB',0xabcd,2 if permanent else 0))
       w(body+0x1c,0x3f800000);w(body+0x20,struct.unpack('<I',struct.pack('<f',decay))[0])
       w(body+0x24,(91 if n==0 else 17)<<24|0x345678);w(body+0x38,0xabcd0000)
       w(body+0x3c,0xabcd0001 if compound and n==0 else 0xffffffff)
      w(stack,stop);entry=[0x12345678,0x23456789,0x3456789a,0x456789ab,stack,0x56789abc,0x6789abcd,0x789abcde]
      banks=[(0x4f7220,4),(0x51df54,4),(0x515f28,4),(0x46a000,4096),(0x1000000,0x8000),(0x101e000,0x2000)]
      initial=[dict(address=at,hex=bytes(g.mem_read(at,n)).hex())for at,n in banks]
      for reg,value in zip(gp_regs,entry):g.reg_write(reg,value)
      for n in range(8):g.reg_write(UC_X86_REG_XMM0+n,0)
      for reg,value in [(UC_X86_REG_EFLAGS,0x202),(UC_X86_REG_FPCW,0x27f),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),(UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(reg,value)
      g.emu_start(0x180520,stop,count=3000);assert g.reg_read(UC_X86_REG_EIP)==stop
      rows.append(dict(tick=tick,decay=decay,compound=compound,permanent=permanent,epoch=epoch,
        entry_gp=entry,exit_gp=[g.reg_read(reg)for reg in gp_regs],eflags=g.reg_read(UC_X86_REG_EFLAGS),
        exit_xmm=[f'{g.reg_read(UC_X86_REG_XMM0+n):032x}'for n in range(8)],instructions=trace.copy(),
        initial_banks=initial,final_banks=[dict(address=at,hex=bytes(g.mem_read(at,n)).hex())for at,n in banks]))
 a.fixture.write_text(json.dumps(dict(scope=__doc__,image_sha256=PIN,entry=0x180520,instruction_patches=[],supplied_services=[],cases=rows))+'\n')
 print('PASS',len(rows),'whole original decal prelude/compound/epoch cases')
if __name__=='__main__':main()
