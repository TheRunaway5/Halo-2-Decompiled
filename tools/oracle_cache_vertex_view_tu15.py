#!/usr/bin/env python3
"""Whole439C0 original CPU vertex view with an explicit nonflushing GPU fixture.

The command-buffer/cursor writes are fixture-only GPU observations. Native CPU
view comparison covers GP/defined flags and the actual cache epoch store; it
makes no assertion about GPU cache invalidation or device creation.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent/'reference/halo2-decompiled/tools'))
from xbe import Xbe
p=argparse.ArgumentParser();p.add_argument('--xbe',type=Path,required=True);p.add_argument('--fixture',type=Path,required=True);a=p.parse_args()
x=Xbe(a.xbe);pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd';assert hashlib.sha256(x.data).hexdigest()==pin
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x800000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x1800000,0x10000);u.mem_map(0x81000000,0x2000)
regs=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
cache,pool,pages,bits,resource,device,commands,stack=0x1800000,0x1800100,0x1800200,0x1800300,0x1800400,0x1800500,0x1800600,0x1807f00
def w(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
rows=[]
for index in (0,1,3,7):
 for shift in (0,4,6):
  for block in (0,1,3,31,63):
   for epoch in (0,1,0xffffffff):
    u.mem_write(cache,bytes(0x10000));w(0x515f24,resource);w(0x515f28,cache);w(0x411568,device)
    w(cache+0x64,pool);w(cache+0x34,shift);w(cache+0x38,epoch)
    w(pool+0x24,0x18);w(pool+0x38,8);w(pool+0x44,pages);w(pool+0x48,bits);w(bits,1<<index)
    row=pages+index*0x18;w(row,0x1234);w(row+8,block);w(row+0x14,17)
    w(resource,0x10000);w(resource+4,0x1000000);w(device,commands);w(device+4,commands+0x100)
    w(stack,0x183e1c)
    initial=[0x12340000|index,0xdeadbeef,0xcafebabe,0x76543210,stack,0x1810180,0x1810184,0x1810188]
    for reg,v in zip(regs,initial):u.reg_write(reg,v)
    u.reg_write(UC_X86_REG_EFLAGS,0x202)
    banks=[dict(address=p,hex=bytes(u.mem_read(p,n)).hex())for p,n in ((0x515f24,8),(cache,0x700),(stack,4),(0x81000000+(block<<shift),1))]
    u.emu_start(0x439c0,0x183e1c,count=100)
    assert u.reg_read(UC_X86_REG_EIP)==0x183e1c
    assert struct.unpack('<II',u.mem_read(commands,8))==(0x41710,0)
    assert struct.unpack('<I',u.mem_read(device,4))[0]==commands+8
    rows.append(dict(initial_banks=banks,entry_gp=initial,exit_gp=[u.reg_read(r)for r in regs],eflags=u.reg_read(UC_X86_REG_EFLAGS),epoch_address=row+0x14,epoch=epoch))
a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(dict(original_sha256=pin,entry='000439c0',scope=__doc__,instruction_patches=[],supplied_services=[],cases=rows),indent=2)+'\n')
print('PASS',len(rows),'whole original CPU vertex-view cases; explicit GPU fixture excluded from native claim')
