#!/usr/bin/env python3
"""Capture TU1.5 cinematic lighting writes and final bloom CPU programs.

Runs unchanged TU1.5 code. Lighting has allocated globals and literal inputs;
the ambient evaluator is entered after HS argument evaluation. Bloom executes
214D0/21180/20F10, with only GPU/SDK submissions intercepted. This establishes
CPU state/program parity, not GPU execution or Xbox framebuffer parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EIP,
    UC_X86_REG_EAX, UC_X86_REG_MXCSR, UC_X86_REG_FPCW)

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--xbe',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
x=args.xbe.read_bytes()
if hashlib.sha256(x).hexdigest()!=PIN:raise ValueError('Expected original Xbox TU1.5 English XBE')
args.output.mkdir(parents=True,exist_ok=False)
g=Uc(UC_ARCH_X86,UC_MODE_32)
for address,size in [(0x10000,0x600000),(0x2100000,0x100000),(0x2300000,0x100000),(0x3000000,4096)]:
    g.mem_map(address,size)
base=struct.unpack_from('<I',x,0x104)[0]
n,table=struct.unpack_from('<II',x,0x11c)
for i in range(n):
    _,a,_,o,s=struct.unpack_from('<5I',x,table-base+i*56)
    if s:g.mem_write(a,x[o:o+s])
def write(address,words):g.mem_write(address,struct.pack('<'+'I'*len(words),*words))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def run(entry,argv=(),stop=0x3000000,eax=0):
    write(0x21fff00,[stop,*argv])
    g.reg_write(UC_X86_REG_ESP,0x21fff00)
    g.reg_write(UC_X86_REG_EAX,eax)
    g.reg_write(UC_X86_REG_MXCSR,0x1f80)
    g.reg_write(UC_X86_REG_FPCW,0x027f)
    g.emu_start(entry,stop,count=1000000)
    if g.reg_read(UC_X86_REG_EIP)!=stop:raise ValueError('Original execution did not return')

# Same input schedule as the retail lighting probe; independently execute TU1.5.
lighting_rng=random.Random(0xbbfc0)
angles=[(45,240),(-45,30),(0,0),(90,180),(-90,360),(180,-180)]+[
    (lighting_rng.uniform(-720,720),lighting_rng.uniform(-720,720)) for _ in range(128)]
cases=[dict(operation=op,args=[bits(a),bits(b),bits(.45),bits(.1),bits(.8)],
    before=[bits(i/32) for i in range(20)]) for op in range(3) for a,b in angles]
bank=0x2102000;write(0x4eabc4,[bank])
lighting=[]
for c in cases:
    write(bank+0x1c,c['before']);write(bank+0x6c,[0x12345678])
    if c['operation']==2:
        write(0x2103000,c['args'][2:]);run(0x2ac059,stop=0x2ac078,eax=0x2103000)
    else:run([0xbdff0,0xbe0a0][c['operation']],c['args'])
    after=list(struct.unpack('<20I',g.mem_read(bank+0x1c,80)))
    lighting.append(str(c['operation'])+' '+' '.join(f'{v:08x}' for v in c['args']+c['before']+after))
(args.output/'cinematic_lighting_tu15.tsv').write_text('\n'.join(lighting)+'\n')

write(0x411568,[0x2350000]);write(0x2350000,[0x23c0000,0x23f0000])
write(0x515ec4,[0x2100000]);write(0x51df50,[0x2101000]);write(0x4920d4,[0xffffffff])
stubs={0x14ba0:8,0x14f40:0,0x15760:4,0x222b0:0,0x1ccd0:0,0x4044b0:0,0x34a80:44}
programs=[];draws=[];effective=[]
def hook(guest,address,size,user):
    if address==0x216c1:
        sp=guest.reg_read(UC_X86_REG_ESP)
        effective.extend(struct.unpack('<2I',guest.mem_read(sp+0x2c,8)))
    if address not in stubs:return
    sp=guest.reg_read(UC_X86_REG_ESP)
    if address in (0x1ccd0,0x4044b0):programs.append(list(struct.unpack('<60I',guest.mem_read(0x491788,240))))
    if address==0x34a80:
        words=struct.unpack('<11I',guest.mem_read(sp+4,44));draws.append(words)
        guest.reg_write(UC_X86_REG_EAX,words[0])
    ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
    guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,hook)
rng=random.Random(0x214d0)
values=[(-1,-1),(.3,.5),(0,0),(1,1),(-.2,1.2),(.8,-1),(-1,.2)]
cases=[(cine,enabled,t,b) for cine in (False,True) for enabled in (False,True) for t,b in values]
cases += [(True,True,rng.uniform(-.5,1.5),rng.uniform(-.5,1.5)) for _ in range(256)]
fixtures=[]
for cine,enabled,threshold,brightness in cases:
    override=bytearray(36);override[0]=int(enabled);override[24]=1
    for off in (4,8,12,16,20,28,32):struct.pack_into('<f',override,off,-1.)
    struct.pack_into('<2f',override,8,threshold,brightness)
    g.mem_write(0x2100000,bytes(override));g.mem_write(0x2101005,bytes([cine]));g.mem_write(0x492294,b'\0')
    programs.clear();draws.clear();effective.clear()
    run(0x214d0,[0x407f7cee,bits(.9),bits(.375),bits(.25),0,1,0])
    if len(programs)!=3 or len(draws)!=6 or len(effective)!=2:raise ValueError('Incomplete original bloom capture')
    fixtures.append(dict(cinematic=cine,enabled=enabled,input=[bits(.9),bits(.375)],
        override=[bits(threshold),bits(brightness)],effective=effective.copy(),
        programs=sum(programs,[]),radii=[d[6] for d in draws[:5]]))
report=dict(xbe_sha256=PIN,generator_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    boundary=__doc__,lighting_cases=len(lighting),bloom_cases=len(fixtures),
    lighting_entries=['000bdff0','000be0a0','002ac059'],
    bloom_entries=['000214d0','00021180','00020f10'],gpu_sdk_stubs=[f'{a:08x}' for a in stubs],
    framebuffer_parity_verified=False,cases=fixtures)
(args.output/'cinematic_bloom_tu15.json').write_text(json.dumps(report,separators=(',',':'))+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='cases'},indent=2))
