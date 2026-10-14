#!/usr/bin/env python3
"""Execute whole TU1.5 44030 on a captured completed cache and declared SDK sinks.

The fixture supplies original source-owned heads/pages/records. All 512 cluster
callbacks are deliberately admitted; this tests traversal, not camera/PVS.
Four external graphics services only record arguments and return at their ABI.
The declared command buffer is never submitted. No instruction is patched.
"""
import argparse, hashlib, json, struct, sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from audit_rendering import Cache
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--xbe',type=Path,required=True);p.add_argument('--map',type=Path,required=True)
 p.add_argument('--cache-fixture',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
 a=p.parse_args();x=Xbe(a.xbe);assert hashlib.sha256(x.data).hexdigest()==PIN
 c=Cache(a.map);u=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
 def put(at,b):
  for page in range(at&~4095,(at+len(b)+4095)&~4095,4096):
   if page not in mapped:u.mem_map(page,4096);mapped.add(page)
  u.mem_write(at,b)
 def w(at,v):put(at,struct.pack('<I',v&0xffffffff))
 def r(at):return struct.unpack('<I',u.mem_read(at,4))[0]
 for section in x.sections:put(section.va,x.section_bytes(section))
 fixture=json.loads(a.cache_fixture.read_text());pins={}
 for row in fixture['regions']:
  if 'file' in row:
   file=a.cache_fixture.parent/row['file'];b=file.read_bytes();pins[row['file']]=hashlib.sha256(b).hexdigest()
  else:b=struct.pack('<I',row['value'])
  put(row['address'],b)
 # Original tag metadata/payloads, relocated only for the declared lookup bank.
 table=0x1000000;w(0x4f0414,table)
 for tag in c.tags.values():
  if tag['group']!='deca':continue
  _,offset=c.tag(tag['datum'],'deca');put(tag['address'],bytes(c.data[offset:offset+tag['size']]))
  w(table+(tag['datum']&65535)*16+8,tag['address'])
 # Explicit SDK recording context, large enough to avoid its grow callback.
 w(0x411568,0x1100000);w(0x1100000,0x1110000);w(0x1100004,0x1120000)
 put(0x1110000,bytes(65536));stack,stop=0x120f000,0x120ff00;put(0x1200000,bytes(65536))
 w(0x4c6858,1);w(0x4d4fac,0xffffffff);w(0x4d4fa8,0);w(0x4d4fa4,0);w(0x4d4fa0,1)
 services={0x144d0:12,0x1cf30:0,0x405460:20,0x404fb0:4};draws=[];bindings=[];colors=[]
 def hook(g,pc,size,data):
  if pc not in services:return
  sp=g.reg_read(UC_X86_REG_ESP)
  if pc==0x144d0:bindings.append(dict(bitmap=g.reg_read(UC_X86_REG_EAX),image=r(sp+8)))
  if pc==0x405460:
   assert r(sp+4)==10;colors.append([r(sp+i*4)for i in range(2,6)])
  if pc==0x404fb0:
   assert r(sp+4)==8
   record=g.reg_read(UC_X86_REG_ESI);slot=(record-r(r(0x4f7220)+0x44))//64
   salt=struct.unpack('<H',u.mem_read(record,2))[0]
   draws.append(dict(datum=(salt<<16)|slot,primary=r(record+0x38),vertices=g.reg_read(UC_X86_REG_EAX),
                     first_vertex=g.reg_read(UC_X86_REG_ECX),tint=r(record+0x24),layer=r(0x4d4fb0)))
  ret=r(sp);g.reg_write(UC_X86_REG_ESP,sp+4+services[pc]);g.reg_write(UC_X86_REG_EIP,ret)
 u.hook_add(UC_HOOK_CODE,hook)
 for layer in range(6):
  w(0x4d4fb0,layer)
  for cluster in range(512):
   w(stack,stop);w(stack+4,cluster);w(stack+8,0)
   u.reg_write(UC_X86_REG_ESP,stack);u.reg_write(UC_X86_REG_EFLAGS,2)
   u.emu_start(0x44030,stop,count=10000);assert u.reg_read(UC_X86_REG_EIP)==stop
 a.output.write_text(json.dumps(dict(original_sha256=PIN,map_sha256=hashlib.sha256(a.map.read_bytes()).hexdigest(),
  entry='00044030',instruction_patches=[],boundary=__doc__,services={hex(k):v for k,v in services.items()},
  pins=pins,draws=draws,texture_bindings=bindings,colors=colors),indent=2)+'\n')
 print('PASS whole original cache traversal:',len(draws),'draws,',sum(d['vertices']//4 for d in draws),'quads')
if __name__=='__main__':main()
