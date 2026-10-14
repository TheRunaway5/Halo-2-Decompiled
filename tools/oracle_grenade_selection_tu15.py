#!/usr/bin/env python3
"""Whole original CE3A0 grenade count scan; no patched instruction or callee."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
p=argparse.ArgumentParser();p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
x=Xbe(a.xbe);pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==pin
assert x.read(0xce3a0,0x74).hex()=='568bf18b0dd0cb4e008b514425ffff00008d0440578b7c820883c8ff6683feff750233f68bceeb088da42400000000900fbfd180bc3a3e020000007e0c663bce8bc1752d6685db74286685db7d116685c97507b901000000eb128d4affeb0d6683f901750433c9eb038d4a01663bce75bf5f5ec3'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x1000000,0x20000)
def w(a,v):u.mem_write(a,struct.pack('<I',v&0xffffffff))
w(0x4ecbd0,0x1000000);w(0x1000044,0x1001000);w(0x1001008,0x1002000)
rows=[]
for current in (-1,0,1):
 for frag in (0,1,2,4,127,128,255):
  for plasma in (0,1,2,4,127,128,255):
   for direction in (-32768,-1,0,1,32767):
    u.mem_write(0x100223e,bytes((frag,plasma)));w(0x101f000,0x101f100)
    for reg,v in ((UC_X86_REG_EAX,0x10000),(UC_X86_REG_ECX,current),(UC_X86_REG_EBX,direction),(UC_X86_REG_ESP,0x101f000)):u.reg_write(reg,v&0xffffffff)
    u.emu_start(0xce3a0,0x101f100,count=500)
    assert u.reg_read(UC_X86_REG_EIP)==0x101f100
    result=struct.unpack('<i',struct.pack('<I',u.reg_read(UC_X86_REG_EAX)))[0]
    rows.append(dict(current=current,counts=[frag,plasma],direction=direction,result=result))
a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(dict(original_sha256=pin,entry='000ce3a0',supplied_services=[],instruction_patches=[],cases=rows),indent=2)+'\n')
print('PASS',len(rows),'whole original grenade count/selection cases')
