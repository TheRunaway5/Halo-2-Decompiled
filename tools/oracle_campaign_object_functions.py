#!/usr/bin/env python3
"""Capture unchanged TU1.5 10FC30/10FB70 override interpolation.

Supplies allocated object/header/global banks. No setter, getter, clock
conversion or interpolation instructions are stubbed. The opening uses fewer
than the original32 global slots; global slot eviction is outside this fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_MXCSR,
    UC_X86_REG_FPCW,UC_X86_REG_XMM4,UC_X86_REG_EBX)

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
x=args.xbe.read_bytes();pin=hashlib.sha256(x).hexdigest()
if pin!='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd':
    raise ValueError('Expected original Xbox TU1.5 English executable')
if args.output.exists():raise ValueError('Refusing to overwrite evidence')
g=Uc(UC_ARCH_X86,UC_MODE_32)
for address,size in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(address,size)
base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
for j in range(n):
    _,a,_,o,s=struct.unpack_from('<5I',x,table-base+j*56)
    if s:g.mem_write(a,x[o:o+s])
def write(address,*values):g.mem_write(address,struct.pack('<'+'I'*len(values),*values))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def run(entry,argv=(),registers=()):
    write(0x21fff00,0x3000000,*argv);g.reg_write(UC_X86_REG_ESP,0x21fff00)
    g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
    for register,value in registers:g.reg_write(register,value)
    g.emu_start(entry,0x3000000,count=100000)
    if g.reg_read(UC_X86_REG_EIP)!=0x3000000:raise ValueError('Original execution did not return')
write(0x51daf4,0x2100000);write(0x51df54,0x2104000)
g.mem_write(0x2104002,struct.pack('<h',30))
write(0x4ecbd0,0x2105000);write(0x2105044,0x2105100);write(0x2105114,0x2106000)
rows=[]
for duration in [0.,1.,1.5,29.999998,30.,30.000002,101.,1024.,-1.]:
    g.mem_write(0x2100000,bytes(0x300));g.mem_write(0x2100280,b'\xff'*128);write(0x2104008,0)
    run(0x10fc30,[0xbeef0001,0x0c001b89,bits(0)],[(UC_X86_REG_XMM4,bits(0))])
    write(0x2104008,10)
    run(0x10fc30,[0xbeef0001,0x0c001b89,bits(1)],[(UC_X86_REG_XMM4,bits(duration))])
    samples=[]
    for tick in [5,10,11,12,15,20,25,40,41,1034,2048]:
        write(0x2104008,tick);write(0x2107000,0x12345678)
        run(0x10fb70,[0x0c001b89,0x2107000,0x2107004],[(UC_X86_REG_EBX,0xbeef0001)])
        samples.append([tick,struct.unpack('<I',g.mem_read(0x2107000,4))[0]])
    rows.append(dict(frames=bits(duration),record=list(struct.unpack('<5I',g.mem_read(0x2100000,20))),samples=samples))
report=dict(xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    boundary=__doc__,cases=rows)
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps(report,indent=2)+'\n')
print(len(rows),'original setter cases;',sum(len(r['samples']) for r in rows),'getter samples')
