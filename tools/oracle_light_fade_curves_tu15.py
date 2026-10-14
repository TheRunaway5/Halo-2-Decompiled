#!/usr/bin/env python3
"""Whole unchanged TU1.5 17F8A0 transition lookup, all sixteen original tables."""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
p=argparse.ArgumentParser();p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
x=Xbe(a.xbe);pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==pin
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x1800000,0x10000);stack,stop=0x180f000,0x180ff00
rng=random.Random(0x17f8a0);values=[-1.,-.001,0.,.001,.1,.5,.999,1.,1.001,2.]
values += [i/1023 for i in range(1024)]+[rng.uniform(-.1,1.1)for _ in range(128)]
rows=[]
for kind in range(16):
 for value in values:
  bits=struct.unpack('<I',struct.pack('<f',value))[0]
  u.mem_write(stack,struct.pack('<I',stop));u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EAX,kind)
  u.reg_write(UC_X86_REG_XMM0,bits);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(0x17f8a0,stop,count=500)
  assert u.reg_read(UC_X86_REG_EIP)==stop
  rows.append([kind,bits,u.reg_read(UC_X86_REG_XMM0)&0xffffffff])
a.fixture.parent.mkdir(parents=True,exist_ok=True)
a.fixture.write_text(json.dumps(dict(original_sha256=pin,entry='0017f8a0',instruction_patches=[],supplied_services=[],tables=[x.read(0x4782a8+n*1024,1024).hex()for n in range(16)],cases=rows))+'\n')
print('PASS',len(rows),'whole original transition lookups across sixteen actual tables')
