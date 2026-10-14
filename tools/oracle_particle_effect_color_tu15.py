#!/usr/bin/env python3
"""Unchanged whole TU1.5 particle color routine and its actual flag getters."""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
 x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(0x1000000,0x10000)
 def w(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
 def run(entry):
  u.reg_write(UC_X86_REG_ESP,0x100f000);w(0x100f000,0x11000000)
  u.emu_start(entry,0x11000000,count=300)
  assert u.reg_read(UC_X86_REG_EIP)==0x11000000
 rng=random.Random(0x178920)
 colors=[0,0xffffffff,0xff808080,0x01020304,0x00ff8001,0xff7f7f7f,0x00010101]
 pairs=[(a,b) for a in colors for b in colors]+[(rng.getrandbits(32),rng.getrandbits(32)) for _ in range(512)]
 cases=[]
 for flags in (0,0x80,0x100,0x180,0x10000,0x10180):
  w(0x1000008,0x1001000);w(0x1001000,flags)
  values=[]
  for entry in (0x22ad40,0x22ad50):
   u.reg_write(UC_X86_REG_ECX,0x1000000);run(entry);values.append(u.reg_read(UC_X86_REG_EAX)&255)
  multiplied,tinted=values
  for color_a,color_b in pairs:
   previous=rng.getrandbits(32);w(0x1003050,previous);w(0x100f004,multiplied)
   u.reg_write(UC_X86_REG_EAX,tinted);u.reg_write(UC_X86_REG_ECX,color_a)
   u.reg_write(UC_X86_REG_EDI,color_b);u.reg_write(UC_X86_REG_ESI,0x1003000)
   u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   run(0x178920)
   result=struct.unpack('<I',u.mem_read(0x1003050,4))[0]
   cases.append(dict(flags=flags,a=color_a,b=color_b,previous=previous,result=result))
 a.fixture.parent.mkdir(parents=True,exist_ok=True)
 a.fixture.write_text(json.dumps(dict(image_sha256=PIN,scope=__doc__,cases=cases),indent=2)+'\n')
 print('PASS unchanged TU1.5 particle effect color:',len(cases),'cases')
if __name__=='__main__':main()
