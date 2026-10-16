#!/usr/bin/env python3
"""Capture unchanged TU1.5 149160 solo frame accounting, without callee stubs.

Allocated game/options/cinematic globals admit loaded solo, 30Hz, scale1,
no speed ramp and no network synchronization. Outputs retain the original
fraction, tick budget and accepted elapsed time, including pause/reset cases.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_MXCSR,UC_X86_REG_FPCW

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
def reset(leftover=0):
    g.mem_write(0x2100000,bytes(0x24));g.mem_write(0x2100000,struct.pack('<BBH',1,0,30))
    write(0x2100004,bits(1/30),0,bits(1),leftover)
write(0x51df54,0x2100000);write(0x4f3218,0x2101000);write(0x51df50,0x2103000)
g.mem_write(0x210100c,b'\1');g.mem_write(0x2102120,b'\1')
def capture(seconds,cinematic,paused):
    before=struct.unpack('<I',g.mem_read(0x2100010,4))[0]
    g.mem_write(0x2100001,bytes([paused]));g.mem_write(0x2103005,bytes([cinematic]))
    write(0x21fff00,0x3000000,bits(seconds),0x2104000,0x2104004)
    g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
    g.emu_start(0x149160,0x3000000,count=10000)
    if g.reg_read(UC_X86_REG_EIP)!=0x3000000:raise ValueError('Original clock did not return')
    elapsed,ticks=struct.unpack('<Ii',g.mem_read(0x2104000,8))
    fraction=struct.unpack('<I',g.mem_read(0x2100010,4))[0]
    return dict(seconds=bits(seconds),cinematic=cinematic,paused=paused,before=before,
        elapsed=elapsed,ticks=ticks,fraction=fraction)
cases=[]
for cinematic in (False,True):
    for paused in (False,True):
        for leftover in (0,.125,.5,.99999994):
            for seconds in (0,1/120,1/60,1/30,.05,.06666667,.1,.125,.5,1,4.99,5,5.1,10):
                reset(bits(leftover));cases.append(capture(seconds,cinematic,paused))
sequences=[]
for rate in (120,60,30,20,10,1):
    reset();rows=[capture(1/rate,True,False) for _ in range(rate)]
    sequences.append(dict(name=f'one-second-{rate}-fps',rows=rows,total_ticks=sum(r['ticks'] for r in rows)))
reset()
rows=[capture(s,c,p) for s,c,p in [(1/120,True,False),(.05,True,False),(.1,True,True),
    (.02,True,False),(.08,False,False),(.1,True,False),(6,True,False),(1/60,True,False)]]
sequences.append(dict(name='pause-mode-change-and-stall',rows=rows,total_ticks=sum(r['ticks'] for r in rows)))
report=dict(xbe_sha256=pin,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    entry='00149160',boundary=__doc__,cases=cases,sequences=sequences)
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
print(len(cases),'original clock cases;',len(sequences),'frame sequences')
