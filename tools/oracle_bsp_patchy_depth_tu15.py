#!/usr/bin/env python3
"""Retain TU1.5 direct patchy fog depth admission; GPU services are suppressed."""
import argparse,random
from pathlib import Path
import sys,hashlib,struct,json
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
x=Xbe(args.xbe)
assert hashlib.sha256(x.data).hexdigest()=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x2100000,0x100000);u.mem_map(0x2300000,0x100000);u.mem_map(0x3000000,4096)
stubs={0x14ba0:8,0x14f40:0,0x222b0:0,0x1cc10:0,0x1c6f0:4,0x3ff4b0:4,0x3ff670:4,0x3ffa90:4,0x3ffdd0:0,0x1ccd0:0}
states=[]
def hook(guest,address,size,user):
 if address not in stubs:return
 if address==0x222b0:states.append((guest.reg_read(UC_X86_REG_ESI),guest.reg_read(UC_X86_REG_EDI)))
 sp=guest.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',guest.mem_read(sp,4))[0];guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
u.hook_add(UC_HOOK_CODE,hook)
def put(p,*v):u.mem_write(p,struct.pack('<'+'f'*len(v),*v))
def run(entry,args):
 u.mem_write(0x21fff00,struct.pack('<'+'I'*(len(args)+1),0x3000000,*args));u.reg_write(UC_X86_REG_ESP,0x21fff00);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPSW,0);u.reg_write(UC_X86_REG_FPTAG,0xffff)
 u.emu_start(entry,0x3000000,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==0x3000000,hex(u.reg_read(UC_X86_REG_EIP))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]
rows=[];rng=random.Random(0x3a5b0)
cases=[(40,50,.015625,1024),(40,50,40,1024),(40.01,50,.015625,1024),(1,2,.015625,1024),(0,50,.015625,1024),(5,5.00001,.015625,1024),(-1,20,.015625,1024),(40,50,.015625,512),(40,50,.015625,2048)]
for i in range(256):
 far=[128.,512.,1024.,2048.][i%4];start=rng.uniform(0,far*.8);end=start+rng.uniform(.001,far*.2);near=start if i%5==0 else .015625
 cases.append((start,end,near,far))
# Quantize every declared input to the original binary32 call ABI.
for start,end,near,far in [struct.unpack('<4f',struct.pack('<4f',*c)) for c in cases]:
 states.clear();u.mem_write(0x411568,struct.pack('<I',0x2350000));u.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000));put(0x491e7c,far);put(0x491e78,near);put(0x4922f4,near,far);put(0x492300,16777215);put(0x2100000,0,.5,.6,.35,.2,0);put(0x515ee0,-1)
 run(0x3a5b0,[0x2100000,bits(start),bits(end),2,0,0,1,0,0x515ee0]);ok=u.reg_read(UC_X86_REG_EAX)&255;plane=struct.unpack('<f',u.mem_read(0x515ee0,4))[0];w=None
 if ok:
  put(0x2100200,0,640,0,480);put(0x2100300,0,0)
  run(0x27500,[0,0x2100200,0x2100300,0x2100400,0]);w=struct.unpack('<4f',u.mem_read(0x2100400,16))[3]
 enabled=ok and [v for k,v in states if k==143][-1]==2
 row=[bits(start),bits(end),bits(near),bits(far),ok,bits(w) if enabled else 0];rows.append(row)
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(''.join(' '.join(f'{v:08x}' for v in row)+'\n' for row in rows))
provenance=dict(reference='OG Xbox TU1.5',xbe_sha256=hashlib.sha256(x.data).hexdigest(),entries={hex(va):hashlib.sha256(x.read(va,n)).hexdigest() for va,n in [(0x3a5b0,4828),(0x27500,420),(0x350d0,180)]},instruction_patches=[],supplied_services={hex(k):'GPU/state submission only; original arithmetic and output tail execute' for k in stubs},cases=len(rows),fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),scope='Quantized mode2 range, returned start, ZENABLE tail, original 27500 -> 350D0 W; declared finite CPU inputs, not live visibility or framebuffer parity')
args.output.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n')
print('PASS',len(rows),'unchanged TU1.5 quantized-fog depth producers')
