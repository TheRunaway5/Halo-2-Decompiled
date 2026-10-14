#!/usr/bin/env python3
"""Execute whole TU1.5 decal raster/program producer and RGB consumers.

The eleven explicit SDK services observe commands and return at their declared
ABI. No game service, GPU command buffer, instruction patch or expected pixel
is supplied. RGB interpolation/packing executes without supplied services.
"""
import argparse, hashlib, json, random, struct, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True)
 a=p.parse_args();x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(0x1000000,0x20000);stack,stop=0x101f000,0x101ff00
 def w(at,v):u.mem_write(at,struct.pack('<I',v&0xffffffff))
 def r(at):return struct.unpack('<I',u.mem_read(at,4))[0]
 # Explicit native raster context, with no material override at +64.
 w(0x4922a0,0x1000000);w(0x1000064,0xffffffff)
 services={0x16af0:0,0x14ba0:8,0x3ffdd0:0,0x3ff4b0:4,
           0x3ff5d0:4,0x3ffa90:4,0x222b0:0,0x1ccd0:0,
           0x1cc10:0,0x1c600:8,0x1c6f0:4}
 states=[];programs=[];calls=[]
 def hook(g,pc,size,data):
  if pc not in services:return
  calls.append(pc)
  if pc==0x222b0:states.append([g.reg_read(UC_X86_REG_ESI),g.reg_read(UC_X86_REG_EDI)])
  if pc==0x1ccd0:programs.append(list(struct.unpack('<60I',g.mem_read(g.reg_read(UC_X86_REG_EDX),240))))
  sp=g.reg_read(UC_X86_REG_ESP);ret=r(sp)
  g.reg_write(UC_X86_REG_ESP,sp+4+services[pc]);g.reg_write(UC_X86_REG_EIP,ret)
 handle=u.hook_add(UC_HOOK_CODE,hook);rows=[]
 for layer in range(6):
  states.clear();programs.clear();calls.clear()
  u.mem_write(0x491788,bytes(240));w(stack,stop);w(stack+4,layer)
  u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.emu_start(0x43a30,stop,count=2000)
  assert u.reg_read(UC_X86_REG_EIP)==stop and len(programs)==1
  rows.append(dict(layer=layer,program=programs[0],states=states.copy(),services=calls.copy(),
    cull=r(0x4c4c68),depth_enable=r(0x4c4c58),depth_function=r(0x4c4b00),
    depth_write=r(0x4c4c70),color_write=r(0x4c4b28)))
 u.hook_del(handle)
 rng=random.Random(0x183c10);colors=[]
 values=[0.,1.,.5,1/255,254/255]
 inputs=[([v]*3,[1-v]*3,t)for v in values for t in values]
 inputs += [([rng.random()for _ in range(3)],[rng.random()for _ in range(3)],rng.random())for _ in range(512)]
 for lower,upper,fraction in inputs:
  lo=struct.pack('<3f',*lower);hi=struct.pack('<3f',*upper);fb=struct.unpack('<I',struct.pack('<f',fraction))[0]
  u.mem_write(0x1001000,lo);u.mem_write(0x1001010,hi)
  for reg,v in [(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0x1001000),(UC_X86_REG_ESI,0x1001010),
                (UC_X86_REG_EDI,0x1001020),(UC_X86_REG_ESP,stack),(UC_X86_REG_XMM5,fb),
                (UC_X86_REG_FPCW,0x27f),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff),
                (UC_X86_REG_MXCSR,0x1f80)]:u.reg_write(reg,v)
  w(stack,stop);u.emu_start(0x134730,stop,count=3000)
  assert u.reg_read(UC_X86_REG_EIP)==stop
  w(stack,stop);w(stack+4,0x1001020);u.reg_write(UC_X86_REG_ESP,stack)
  u.emu_start(0x1349e0,stop,count=1000);assert u.reg_read(UC_X86_REG_EIP)==stop
  colors.append(dict(lower=list(struct.unpack('<3I',lo)),upper=list(struct.unpack('<3I',hi)),fraction=fb,
    interpolated=list(struct.unpack('<3I',u.mem_read(0x1001020,12))),packed=u.reg_read(UC_X86_REG_EAX)))
 # Whole17FB60 on explicit live, unexpired decal records; no GPU/game services.
 w(0x4f7220,0x1002000);w(0x1002044,0x1003000);w(0x51df54,0x1004000)
 seconds=struct.unpack('<I',struct.pack('<f',1/30))[0];w(0x1004004,seconds);fades=[]
 for lifetime in (.5,1.,2.,10.):
  for decay in (0.,.25,.5,1.,4.):
   for tick in range(int(lifetime*30)):
    u.mem_write(0x1003000,bytes(64));w(0x100301c,struct.unpack('<I',struct.pack('<f',lifetime))[0])
    w(0x1003020,struct.unpack('<I',struct.pack('<f',decay))[0]);w(0x1003024,0x12345678);w(0x1004008,tick)
    w(stack,stop);u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EAX,0xabcd0000)
    u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
    u.emu_start(0x17fb60,stop,count=2000);assert u.reg_read(UC_X86_REG_EIP)==stop
    result=r(0x1003024);assert result&0xffffff==0x345678
    fades.append(dict(lifetime=lifetime,decay=decay,tick=tick,seconds=seconds,alpha=result>>24))
 a.fixture.write_text(json.dumps(dict(original_sha256=PIN,entry='00043a30',instruction_patches=[],
   boundary=__doc__,sdk_services={hex(k):v for k,v in services.items()},passes=rows,colors=colors,fades=fades),indent=2)+'\n')
 print('PASS six whole original decal passes,',len(colors),'whole RGB cases and',len(fades),'whole decal fade cases')
if __name__=='__main__':main()
