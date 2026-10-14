#!/usr/bin/env python3
"""Unchanged TU1.5 facing and projectile attachment math over declared ABI inputs."""
import argparse,hashlib,json,math,random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(0x1000000,0x10000)
 w=lambda p,v:u.mem_write(p,struct.pack('<I',v&0xffffffff))
 w(0x4ecbd0,0x1000000);w(0x1000044,0x1002000);w(0x1002014,0x1003000)
 up=struct.unpack('<I',x.read(0x474d10,4))[0]
 constants=[(0x4ecbd0,4),(0x46a0d0,4),(0x46a0e8,8),(0x474d10,4),(up,12),(0x1000000,0x48),(0x1002000,0x18)]
 regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
 xmm=[UC_X86_REG_XMM0+i for i in range(8)];trace=[];u.hook_add(UC_HOOK_CODE,lambda uc,p,n,_:trace.append(p))
 cases=[]
 for parent in (0xffffffff,0x12340003):
  for v in ((0,0),(-0.,-0.),(.00001,0),(.001,.001),(.5,.5),(1,0),(2,0),(-2,.5),(12,12),(-.8,-.8)):
   u.mem_write(0x1003000,b'\0'*0x1b0);w(0x1003014,parent);u.mem_write(0x1003070,struct.pack('<6f',.25,.5,.75,-1,0,0));u.mem_write(0x1003168,struct.pack('<2f',*v));u.mem_write(0x100efe0,b'\xa5'*0x30);w(0x100f000,0x11000000)
   gp=[0x12340001,0x12345678,0x13572468,0x89abcdef,0x100f000,0x55667788,0x66778899,0x778899aa]
   for r,n in zip(regs,gp):u.reg_write(r,n)
   for r in xmm:u.reg_write(r,0)
   u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   spans=constants+[(0x1003000,0x1b0),(0x100efe0,0x30)]
   initial=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in spans];trace.clear();u.emu_start(0xf0550,0x11000000,count=300)
   assert u.reg_read(UC_X86_REG_EIP)==0x11000000
   cases.append(dict(parent=parent,movement=v,entry_gp=gp,initial_banks=initial,
     exit_gp=[u.reg_read(r) for r in regs],exit_xmm=[f'{u.reg_read(r):032x}' for r in xmm],eflags=u.reg_read(UC_X86_REG_EFLAGS),instructions=trace.copy(),
     final_banks=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in spans]))
 # The actual projectile attachment update calls120740. Exercise its
 # quaternion/vector/scale output without supplying any child returns.
 rng=random.Random(0x120740)
 for scale in (-2.,-1.,-0.,0.,.125,1.,2.,16.):
  for variant in range(8):
   q=[rng.uniform(-1,1) for _ in range(4)];length=math.sqrt(sum(v*v for v in q));q=[v/length for v in q]
   data=q+[rng.uniform(-20,20) for _ in range(3)]+[scale]
   u.mem_write(0x1006000,struct.pack('<8f',*data));u.mem_write(0x1006100,b'\xa5'*32)
   u.mem_write(0x100efe0,b'\xa5'*0x30);w(0x100f000,0x11000000)
   gp=[0x12345678,0x1006100,0x1006000,0x89abcdef,0x100f000,0x55667788,0x66778899,0x778899aa]
   for r,n in zip(regs,gp):u.reg_write(r,n)
   for r in xmm:u.reg_write(r,0)
   u.reg_write(UC_X86_REG_EFLAGS,0x202);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   spans=[(0x46a0d0,4),(0x46a0dc,4),(0x46a11c,4),(0x46a348,4),(0x1006000,0x200),(0x100efe0,0x30)]
   initial=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in spans];trace.clear();u.emu_start(0x120740,0x11000000,count=200)
   assert u.reg_read(UC_X86_REG_EIP)==0x11000000
   cases.append(dict(entry=0x120740,scale=scale,variant=variant,entry_gp=gp,initial_banks=initial,
     exit_gp=[u.reg_read(r) for r in regs],exit_xmm=[f'{u.reg_read(r):032x}' for r in xmm],eflags=u.reg_read(UC_X86_REG_EFLAGS),instructions=trace.copy(),
     final_banks=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex()) for p,n in spans]))
 result=dict(scope=__doc__,image_sha256=PIN,entry=0xf0550,cases=cases)
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(result,indent=2)+'\n');print('PASS unchanged TU1.5 facing/attachment:',len(cases),'cases')
if __name__=='__main__':main()
