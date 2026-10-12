#!/usr/bin/env python3
"""Freeze unchanged original137800 compound arithmetic on explicit WORLD inputs.

Uses actual beam-rifle compound records and declared arithmetic controls. No
original callee is replaced. Does not claim live source pose/residency proof.
"""
from pathlib import Path
import struct,sys,random,hashlib
from audit_rendering import Cache
from audit_world_vertex_declarations import Xbe,ROOT
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return words(struct.pack('<f',v))[0]
x=Xbe(ROOT/'assets/disc/default.xbe');u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x2100000,0x100000);u.mem_map(0x3000000,4096)
MODE,TAGS,INVERSE,WORLD,GROUP,COMPOUND,OUTPUT=[0x2100000+i*0x10000 for i in range(7)]
STACK=0x21fff00;STOP=0x3000000
c=Cache(ROOT/'assets/disc/maps/lockout.map')
t=next(t for t in c.tags.values() if t['group']=='mode' and 'fp_beam_rifle' in t['name']);_,m=c.tag(t['datum'],'mode')
g=c.block(m+0x34,12)[0];actual=[bytes(c.data[p:p+16]) for p in c.block(g+4,16)]
assert len(actual)==6
rng=random.Random(0x137800);rows=[]
identity=[1,1,0,0,0,1,0,0,0,1,0,0,0]
for case in range(40):
 nodes=4;world=[]
 for n in range(nodes):
  values=[rng.uniform(-3,3) for _ in range(13)] if case else identity
  world.extend(map(bits,values))
 definitions=actual if case<32 else [bytes([0,1,2,3])+struct.pack('<3f',.1,.2,.3),bytes([4,0,255,255])+struct.pack('<3f',.75,.25,0)]
 for dst in (MODE,TAGS,INVERSE,WORLD,GROUP,COMPOUND,OUTPUT):u.mem_write(dst,bytes(0x10000))
 u.mem_write(0x4e3b44,packed([TAGS]));u.mem_write(TAGS+8,packed([MODE]));u.mem_write(MODE+0x34,packed([1,GROUP]));u.mem_write(MODE+0x48,packed([nodes,INVERSE]));u.mem_write(GROUP+4,packed([len(definitions),COMPOUND]));u.mem_write(COMPOUND,b''.join(definitions));u.mem_write(WORLD,packed(world))
 for n in range(nodes):u.mem_write(INVERSE+n*96+0x28,packed(list(map(bits,identity))))
 u.mem_write(0x4e6494,packed([0]));u.reg_write(UC_X86_REG_ECX,0xffffffff);u.reg_write(UC_X86_REG_EDX,0)
 u.mem_write(STACK,packed([STOP,WORLD,0,5,1,OUTPUT]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 u.emu_start(0x137800,STOP,count=200000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 count=struct.unpack('<H',u.mem_read(OUTPUT,2))[0];assert count==nodes+len(definitions)
 row=[nodes,len(definitions),*world]
 for raw in definitions:row.extend([*raw[:4],*words(raw[4:])])
 row.extend(words(u.mem_read(OUTPUT+0x44,count*48)));rows.append(' '.join(f'{v:08x}' for v in row))
out=ROOT/'tests/fixtures/weapon_compound_palette_original.tsv'
out.write_text('# Untouched original137800/1376C0/142A60; actual beam-rifle compound metadata plus explicit WORLD controls\n# XBE '+hashlib.sha256(x.data).hexdigest()+'\n'+'\n'.join(rows)+'\n')
print('Wrote',len(rows),'independent original compound cases to',out.relative_to(ROOT))
