#!/usr/bin/env python3
"""Unchanged whole TU1.5 1D2D0, including original D3D metadata queries.

Declared texture and device inputs; no child returns are supplied. Compare the
CPU caller's complete bitmap output and ABI, excluding its dead stack locals.
The native boundary leaves unconsumed GPU multisample state unmanufactured.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ
from unicorn.x86_const import *
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(0x1000000,0x10000)
 w=lambda p,v:u.mem_write(p,struct.pack('<I',v&0xffffffff))
 regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
 xmm=[UC_X86_REG_XMM0+i for i in range(8)];trace=[];reads=set()
 u.hook_add(UC_HOOK_CODE,lambda uc,p,n,_:trace.append(p))
 u.hook_add(UC_HOOK_MEM_READ,lambda uc,t,p,n,v,_:reads.update(range(p,p+n)) if 0x10000<=p<0x800000 else None)
 # Explicit original-device input only for the unchanged oracle. Its current
 # target has different data; production never creates this declared device.
 w(0x411568,0x1002000);w(0x1003a14,0x1004000);w(0x1004004,0x12345678)
 cases=[]
 for fmt in (0,1,4,5,6,7,12,14,15,16,17,19,25,26,28,30,31,32,38,46):
  for width,height in ((1,1),(2,4),(8,16),(32,8),(128,64)):
   for linear in (False,True):
    u.mem_write(0x1000000,b'\0'*32);w(0x1000000,0x40001);w(0x1000004,0x600000)
    bits=((width.bit_length()-1)<<20)|((height.bit_length()-1)<<24)|(fmt<<8)|0x10020
    w(0x100000c,bits)
    if linear:w(0x1000010,((width-1)&0xfff)|((height-1)<<12)|((max(1,(width*4+63)//64)-1)<<24))
    u.mem_write(0x1001000,b'\xa5'*0x80);u.mem_write(0x100ed00,b'\xa5'*0x320)
    w(0x100f000,0x11000000);w(0x100f004,0x1000000);w(0x100f008,0x1001000)
    gp=[0x12345678,0x23456789,0x3456789a,0x456789ab,0x100f000,0x56789abc,0x6789abcd,0x789abcde]
    for r,v in zip(regs,gp):u.reg_write(r,v)
    for r in xmm:u.reg_write(r,0)
    u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
    spans=[(0x1000000,32),(0x1001000,0x80),(0x1002000,0x2000),(0x1004000,0x20),(0x100ed00,0x320)]
    initial=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in spans]
    reads.clear();trace.clear();u.emu_start(0x1d2d0,0x11000000,count=3000)
    assert u.reg_read(UC_X86_REG_EIP)==0x11000000
    constants=[];start=last=None
    for address in sorted(reads):
     if last is None:start=last=address
     elif address==last+1:last=address
     else:constants.append((start,last-start+1));start=last=address
    if last is not None:constants.append((start,last-start+1))
    initial.extend(dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in constants)
    cases.append(dict(entry=0x1d2d0,format=fmt,width=width,height=height,linear=linear,entry_gp=gp,initial_banks=initial,
      exit_gp=[u.reg_read(r) for r in regs],exit_xmm=[f'{u.reg_read(r):032x}' for r in xmm],eflags=u.reg_read(UC_X86_REG_EFLAGS),
      instructions=[p for p in trace if not 0x407c00<=p<0x407ca0],
      final_banks=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in [(0x1000000,32),(0x1001000,0x80),(0x100f000,0x20)]]))
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(dict(scope=__doc__,image_sha256=PIN,entry=0x1d2d0,cases=cases),indent=2)+'\n')
 print('PASS unchanged TU1.5 whole cache bitmap caller:',len(cases),'cases')
if __name__=='__main__':main()
