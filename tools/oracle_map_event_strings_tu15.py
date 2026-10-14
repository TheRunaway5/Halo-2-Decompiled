#!/usr/bin/env python3
"""Whole original locale/UTF lookup and non-template formatter fixtures.

All tag/string banks and cached language values are declared inputs. No function
is replaced. This does not validate locale resource loading or template tokens.
"""
import argparse,hashlib,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert hashlib.sha256(NEW_DEFAULT.read_bytes()).hexdigest()==NEW_PIN
 x=Xbe(NEW_DEFAULT);u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(0x2400000,0x20000);u.mem_map(0x3000000,4096)
 gp_regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
 xmm_regs=[UC_X86_REG_XMM0+i for i in range(8)];trace=[];writes=[]
 u.hook_add(UC_HOOK_CODE,lambda g,pc,size,data:trace.append(pc))
 u.hook_add(UC_HOOK_MEM_WRITE,lambda g,access,at,size,value,data:writes.append([at,(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()]))
 rng=random.Random(0x1a9d90);cases=[];stack=0x2410080;source=0x2401000;output=0x2402000
 texts=['','Slayer','Caf\u00e9','\u2603 snow','\u65e5\u672c\u8a9e','abcdefghijklmno']
 def add(entry,args=(),registers=None,banks=None):
  memory={stack-0x800:bytearray(rng.randbytes(0x1000)),source:bytearray(1024),output:bytearray(rng.randbytes(1024))}
  memory[0x11f47c]=bytearray(x.read(0x11f47c,36))
  if banks:memory.update(banks)
  def w(at,value):
   for start,b in memory.items():
    if start<=at and at+4<=start+len(b):struct.pack_into('<I',b,at-start,value&0xffffffff);return
   memory[at]=bytearray(struct.pack('<I',value&0xffffffff))
  w(stack,0x3000000)
  for i,value in enumerate(args):w(stack+4+i*4,value)
  regs=[rng.getrandbits(32)for _ in gp_regs];regs[4]=stack
  for i,v in (registers or {}).items():regs[i]=v
  c=dict(entry=entry,gp=regs,xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in xmm_regs],eflags=2,cw=0x37f,mxcsr=0x1f80,regions=[dict(address=at,bytes=b.hex())for at,b in sorted(memory.items())])
  for b in c['regions']:u.mem_write(b['address'],bytes.fromhex(b['bytes']))
  for reg,v in zip(gp_regs,regs):u.reg_write(reg,v)
  for reg,v in zip(xmm_regs,c['xmm']):u.reg_write(reg,int.from_bytes(struct.pack('<4I',*v),'little'))
  u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x37f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  trace.clear();writes.clear();u.emu_start(entry,0x3000000,count=20000);assert u.reg_read(UC_X86_REG_EIP)==0x3000000
  original=dict(gp=[u.reg_read(r)for r in gp_regs],xmm=[list(struct.unpack('<4I',u.reg_read(r).to_bytes(16,'little')))for r in xmm_regs],eflags=u.reg_read(UC_X86_REG_EFLAGS),instructions=trace.copy(),writes=writes.copy(),banks=[bytes(u.mem_read(b['address'],len(b['bytes'])//2)).hex()for b in c['regions']])
  cases.append(dict(input=c,original=original))
 for raw in range(12):add(0x11f440,registers={0:raw})
 for text in texts:
  utf8=bytearray(text.encode()+b'\0'+bytes(1024-len(text.encode())-1));utf16=bytearray(text.encode('utf-16le')+bytes(1024-len(text.encode('utf-16le'))))
  for count in [0,1,2,3,4,5,7,8,15,16,17,255]:
   add(0x32a8ed,[output,source,count],banks={source:utf16})
   add(0x142f40,[source,output,count],banks={source:utf8})
  add(0x1a7c10,[0x2404000],{0:output,1:source,2:256},banks={source:utf16})
  for loaded in [0,1]:
   tag=bytearray(64);struct.pack_into('<hh',tag,16,0,1)
   globals=bytearray(0x300);struct.pack_into('<III',globals,0x188,0x2405000,source,1);globals[0x1a0]=loaded
   table=bytearray(16);struct.pack_into('<I',table,8,0x2403000)
   index=bytearray(struct.pack('<II',0x12345,0))
   add(0x1a9d90,[output],{0:0xabcd0000,7:0x12345},banks={source:utf8,output:bytearray(1024),0x2403000:tag,0x2404000:globals,0x2405000:index,0x2406000:table,0x48c750:bytearray(4),0x4ecc1c:bytearray(struct.pack('<I',0x2404000)),0x4f0414:bytearray(struct.pack('<I',0x2406000))})
 for entry in [0x31e1f0,0x325a60,0x3233a0]:
  for _ in range(32):add(entry,registers={1:output})
 for _ in range(16):
  add(0x25aaf1,[source],{0:0},banks={source:bytearray(1024)})
  add(0x25aaf1,[source],{0:0xffffffff})
 # Complete queued announcement admission/duplicate suppression. The real
 # unqueued sound engine is outside these explicit queue-input fixtures.
 for n in range(32):
  event=bytearray(1024);struct.pack_into('<IIIIIII',event,0,0,n%24,0,0xffffffff,n%4,0xffffffff,n%3)
  tick=bytearray(struct.pack('<III',0x001e0000,0,100+n))
  queue=bytearray(0xb4);struct.pack_into('<I',queue,0x9c,n%6)
  for row in range(4):struct.pack_into('<I',queue,0x50+row*24,50+n)
  add(0x248270,[n%3,source,1],{7:0xf0000000+n%4},banks={source:event,0x2407000:tick,0x51df54:bytearray(struct.pack('<I',0x2407000)),0x50ee28:queue,0x46a194:bytearray(x.read(0x46a194,4))})
 for n in range(32):
  definition=bytearray(1024);definition[0x40]=n%2
  struct.pack_into('<f',definition,0x3c,[0.,.1,.25,1.][n%4]);struct.pack_into('<I',definition,0x48,0xffffffff)
  tick=bytearray(struct.pack('<III',0x001e0000,0,100+n))
  add(0x1a8290,[0x2404000],{6:source},banks={source:definition,0x48c750:bytearray(4),0x2407000:tick,0x51df54:bytearray(struct.pack('<I',0x2407000))})
 for _ in range(64):add(0x31e330,[output])
 for n in range(128):
  matrix=bytearray(rng.randbytes(1024))
  for at in [0,0x14,0x28]:struct.pack_into('<f',matrix,at,rng.uniform(-4.,4.) if n%8 else 0.)
  add(0x323bb0,[source],{1:output},banks={source:matrix,0x46a0d0:bytearray(x.read(0x46a0d0,0x20))})
 for n in range(128):
  body=bytearray(rng.randbytes(1024));struct.pack_into('<I',body,0,0x41e458)
  value=struct.unpack('<I',struct.pack('<f',rng.uniform(-4.,4.) if n%8 else 0.))[0]
  add(0x323860,[value],{1:output},banks={output:body,0x41e458:bytearray(x.read(0x41e458,0x24)),0x46a0d0:bytearray(x.read(0x46a0d0,0x20))})
 for n in range(32):
  pool=bytearray(128);struct.pack_into('<I',pool,0x44,0x2404000)
  headers=bytearray(16);struct.pack_into('<I',headers,8,0x2405000)
  body=bytearray(1024);struct.pack_into('<I',body,0,0xbcde0000)
  struct.pack_into('<f',body,0x134,n/32.);struct.pack_into('<f',body,0x140,n/16.);struct.pack_into('<h',body,0x148,n)
  table=bytearray(16);struct.pack_into('<I',table,8,0x2407000)
  definition=bytearray(1024);struct.pack_into('<f',definition,0x10c,1.)
  tick=bytearray(struct.pack('<IfI',0x001e0000,1/30,100+n))
  for key in [0x8000080,0x5000081,0x40005ab,0x6000560]:
   add(0x10be20,[0xabcd0000,key,output,output+4],banks={0x2403000:pool,0x2404000:headers,0x2405000:body,0x2406000:table,0x2407000:definition,0x2408000:tick,0x4ecbd0:bytearray(struct.pack('<I',0x2403000)),0x4f0414:bytearray(struct.pack('<I',0x2406000)),0x51df54:bytearray(struct.pack('<I',0x2408000)),0x46a0c0:bytearray(x.read(0x46a0c0,0x600))})
 # Original random-direction table selection, with the actual immutable bank.
 for n in range(128):
  seed=bytearray(struct.pack('<I',rng.getrandbits(32)))
  add(0x148de0,registers={0:output,2:source},banks={source:seed,0x44b8e8:bytearray(x.read(0x44b8e8,0x402*12))})
 # Exact authored bit3 exclusion across the original512-instance mask.
 for n in range(96):
  count=[0,1,31,32,33,63,127,511][n%8]
  bsp=bytearray(128);struct.pack_into('<II',bsp,0x5c,count,0x2404000)
  instances=bytearray(rng.randbytes(0x24*512));mask=bytearray(rng.randbytes(64))
  add(0x2190f0,banks={0x2403000:bsp,0x2404000:instances,0x2409000:mask,0x4ecc18:bytearray(struct.pack('<I',0x2403000)),0x505e7c:bytearray(struct.pack('<I',0x2409000))})
 a.output.write_text(''.join(json.dumps(c)+'\n'for c in cases))
 a.output.with_suffix('.provenance.json').write_text(json.dumps(dict(original_sha256=NEW_PIN,cases=len(cases),hooked_functions=[],boundary=__doc__,full_goal_complete=False),indent=2)+'\n')
 print('PASS generated',len(cases),'whole original locale/UTF/CRT/formatter cases')
if __name__=='__main__':main()
