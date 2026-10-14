#!/usr/bin/env python3
"""Unchanged TU1.5 collider retirement branch and its actual virtual flag getters."""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
p=argparse.ArgumentParser();p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x1000000,0x20000)
def w(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
r=random.Random(0x2c32ff)
flags=list(range(4096))+[r.getrandbits(32) for _ in range(512)]
rows=[]
for value in flags:
 for kind in [0,1,2,3,4,0xffffffff]:
  w(0x486044,0x1000000);w(0x1000000,value);w(0x1001000,0)
  sp=0x101f000;w(sp+0xb0,kind)
  u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_EBP,0x1001000);u.reg_write(UC_X86_REG_ESI,0x48603c)
  u.emu_start(0x2c32ff,0x2c3375,count=100)
  assert u.reg_read(UC_X86_REG_EIP)==0x2c3375
  dead=bool(struct.unpack('<H',u.mem_read(0x1001002,2))[0]&1)
  rows.append(dict(flags=value,type=kind,dead=dead))
a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(''.join(json.dumps(row)+'\n' for row in rows))
print('PASS',len(rows),'original collider decisions and complete virtual getters; no original instruction patches')
