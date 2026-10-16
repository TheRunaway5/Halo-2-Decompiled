#!/usr/bin/env python3
"""Unchanged TU1.5 cinematic-shadow wrapper and whole control-bank owner.

Parsed packet212010 and void HS publication211DA0 are declared boundaries.
All30E10 bank decisions execute unchanged. No object lookup, shadow frame,
geometry, GPU submission or rendered-shadow result is supplied.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert not a.output.exists();x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 definition=struct.unpack('<I',x.read(0x480c00+146*4,4))[0];assert struct.unpack('<I',x.read(definition+4,4))[0]==0x2acca0
 g=Uc(UC_ARCH_X86,UC_MODE_32)
 for at,size in [(0x10000,0x600000),(0x2100000,0x100000),(0x3000000,4096)]:g.mem_map(at,size)
 for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
 bank,packet,stack,stop=0x4c6894,0x2100000,0x21fff00,0x3000000
 word=lambda at:struct.unpack('<I',g.mem_read(at,4))[0]
 def put(at,*words):g.mem_write(at,struct.pack('<'+'I'*len(words),*(v&0xffffffff for v in words)))
 active={};executed=set();calls=[]
 def hook(_g,pc,size,_):
  assert bytes(g.mem_read(pc,size))==x.read(pc,size);executed.add(pc)
  if pc==0x212010:
   sp=g.reg_read(R.UC_X86_REG_ESP);assert word(sp+8)==4
   g.reg_write(R.UC_X86_REG_EAX,packet if active['parsed'] else 0);g.reg_write(R.UC_X86_REG_EIP,word(sp));g.reg_write(R.UC_X86_REG_ESP,sp+20)
  elif pc==0x211da0:
   assert g.reg_read(R.UC_X86_REG_EAX)==0
   sp=g.reg_read(R.UC_X86_REG_ESP);g.reg_write(R.UC_X86_REG_EIP,word(sp));g.reg_write(R.UC_X86_REG_ESP,sp+4)
  elif pc==0x30e10:
   calls.append([g.reg_read(R.UC_X86_REG_EBX)&255,g.reg_read(R.UC_X86_REG_ESI),g.reg_read(R.UC_X86_REG_EDI),word(g.reg_read(R.UC_X86_REG_ESP)+4)])
 g.hook_add(UC_HOOK_CODE,hook);rng=random.Random(0x30e10);inputs=[]
 for count in range(17):
  for flag,obj in [(1,0xaabb1000),(0,0xaabb1000),(1,0xffffffff),(0,0xffffffff)]:inputs.append((count,flag,obj,0x04000095,0x3ecccccd,True))
  if count:
   for index in sorted({0,count//2,count-1}):
    for flag in [0,1]:inputs.append((count,flag,0xbeef0000+index,0x04000095,0x3e800000,True))
 for n in range(96):
  count=n%17;obj=(0xbeef0000+rng.randrange(count))if count and n%2 else 0x11223344
  inputs.append((count,n%2,obj,rng.getrandbits(32),[0,0x80000000,0x7f800000,0xff800000,0x7fc01234,rng.getrandbits(32)][n%6],n%19!=0))
 rows=[]
 for count,flag,obj,marker,radius,parsed in inputs:
  before=bytearray(rng.randbytes(0xc4))
  for n in range(count):struct.pack_into('<III',before,n*12,0xbeef0000+n,0x10000000+n,0x3f000000+n)
  struct.pack_into('<I',before,0xc0,count);g.mem_write(bank,bytes(before));put(packet,flag,obj,marker,radius);put(stack,stop,146,0x1234,1)
  for reg in [R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI]:g.reg_write(reg,0xabcdef01)
  g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_EFLAGS,2);g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  active=dict(parsed=parsed);calls.clear();g.emu_start(0x2acca0,stop,count=10000)
  assert g.reg_read(R.UC_X86_REG_EIP)==stop and g.reg_read(R.UC_X86_REG_ESP)==stack+16
  assert calls==([[flag,obj,marker,radius]]if parsed else[])
  rows.append(dict(before=bytes(before).hex(),after=bytes(g.mem_read(bank,0xc4)).hex(),enabled=bool(flag),object=obj,marker=marker,radius=radius,parsed=parsed))
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(dict(xbe_sha256=PIN,wrapper='002acca0',owner='00030e10',bank='004c6894',cases=rows,executed=sorted(executed),boundary=__doc__),indent=2)+'\n')
 print('PASS',len(rows),'unchanged complete shadow wrapper/control-bank cases')
if __name__=='__main__':main()
