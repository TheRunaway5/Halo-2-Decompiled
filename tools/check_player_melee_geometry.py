#!/usr/bin/env python3
"""D0830 geometry differential. Explicit eye and collision service boundaries.

Executes unchanged original D0830,11E720,30BD0. Collision records are synthetic
stimuli; the real original owner chooses their target and damage descriptor.
No native output supplies an expected ray or selection.
"""
import argparse,hashlib,json,math,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from unicorn import *
from unicorn.x86_const import *
p=argparse.ArgumentParser();p.add_argument('xbe',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
sha=hashlib.sha256(a.xbe.read_bytes()).hexdigest()
assert sha=='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
x=Xbe(a.xbe);u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
for s in x.sections:
 if s.va>=0x10000 and s.va+s.vsize<0x610000:u.mem_write(s.va,x.section_bytes(s))
u.mem_map(0x60000000,0x10000);u.mem_map(0x70000000,0x10000)
def put(p,v):u.mem_write(p,struct.pack('<I',v&0xffffffff))
def get(p):return struct.unpack('<I',u.mem_read(p,4))[0]
def floats(p,v):u.mem_write(p,struct.pack('<'+'f'*len(v),*v))
def words(p,n):return struct.unpack('<'+'I'*n,u.mem_read(p,n*4))
def ret(pop=0):
 esp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EIP,get(esp));u.reg_write(UC_X86_REG_ESP,esp+4+pop)
unit=0x60002000;weapon=0x60003000;definition=0x60004000
put(0x4e78d0,0x60000000);put(0x60000044,0x60001000)
put(0x60001008,unit);put(0x60001020,weapon);put(unit,0);put(weapon,2)
put(unit+0x14,0xffffffff);put(unit+0x13c,0xffffffff);u.mem_write(unit+0x212,b'\0');put(unit+0x218,0xabcd0002)
put(0x4eb114,0x60005000);put(0x60005008,0x60006000);put(0x60005028,definition)
put(definition+0x190,0x1234abcd)
for off in [0x198,0x1c0,0x1c8,0x1d0,0x1d8,0x1e0,0x1e8,0x1f0,0x1f8]:put(definition+off,0xffffffff)
u.mem_write(0x484de0,b'\xff\xff')
put(0x4edf18,0x60007000)
for i,t in [(3,7),(4,0),(5,0)]:
 obj=0x60008000+i*0x100;put(0x60001008+i*12,obj);u.mem_write(obj+0xaa,bytes([t]));put(obj+0x14,0xffffffff)
rays=[];selected=[];pattern=0
# Each original cast gets one controlled contact. The later closer prop
# cannot displace a selected biped; nearer bipeds can displace earlier ones.
def contact(i):
 if pattern==0:return None
 return [(3,7,.05),(4,0,.7),(3,7,.01),(5,0,.6),(4,0,.65)][i%5]
def hook(uc,address,size,ctx):
 if address==0xcc320:
  floats(u.reg_read(UC_X86_REG_EAX),[0,0,0]);ret()
 elif address==0x16b240:
  esp=u.reg_read(UC_X86_REG_ESP);assert get(esp+4)==0x2480000f
  delta=get(esp+12);rays.append(words(delta,3));c=contact(len(rays)-1)
  if c:
   i,t,f=c;out=get(esp+24);u.mem_write(out,bytes(0x60));put(out,4);floats(out+4,[f]);put(out+0x40,0xabcd0000+i);u.mem_write(out+0x24,b'\xff\xff');u.reg_write(UC_X86_REG_EAX,1)
  else:u.reg_write(UC_X86_REG_EAX,0)
  ret(24)
 elif address==0xa8460:u.reg_write(UC_X86_REG_EAX,0);ret(4)
 elif address==0xd10f0:
  esp=u.reg_read(UC_X86_REG_ESP);packet=get(esp+12);selected.append((get(packet),get(esp+8)));ret(12)
 elif address==0x70000000:u.emu_stop()
u.hook_add(UC_HOOK_CODE,hook)
rows=[]
for k in range(24):
 yaw=k*.37;pitch=(k%7-3)*.2
 aim=[math.cos(yaw)*math.cos(pitch),math.sin(yaw)*math.cos(pitch),math.sin(pitch)]
 pyramid=[.3+(k%3)*.2,.4+(k%5)*.1,.7+(k%4)*.2]
 for pattern in [0,1]:
  floats(unit+0x168,aim);floats(definition+0x1b0,pyramid);rays=[];selected=[]
  esp=0x70008000;put(esp,0x70000000)
  for i,v in enumerate([0xabcd0000,0x0e000607,0,0]):put(esp+4+i*4,v)
  u.reg_write(UC_X86_REG_ESP,esp);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  try:u.emu_start(0xd0830,0,count=100000)
  except Exception as e:raise RuntimeError(f'eip={u.reg_read(UC_X86_REG_EIP):x} rays={len(rays)}') from e
  assert len(rays)==25
  target=selected[0][0]if selected else 0xffffffff
  assert target==(0xabcd0005 if pattern else 0xffffffff),(selected,pattern)
  row=['C',str(pattern),*(f'{v:08x}'for v in words(unit+0x168,3)),*(f'{v:08x}'for v in words(definition+0x1b0,3)),f'{target:08x}']
  rows.append(' '.join(row));rows.extend('R '+' '.join(f'{v:08x}'for v in r)for r in rays)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(rows)+'\n')
a.output.with_suffix('.json').write_text(json.dumps({'source_xbe_sha256':sha,'fixture_sha256':hashlib.sha256(a.output.read_bytes()).hexdigest(),'cases':48,'rays':1200,'boundaries':['CC320 explicit zero eye position','16B240 controlled no-contact/object-contact records','A8460 offline native authority','D10F0 records original selected target and damage; no damage result provided'],'instruction_patches':[]},indent=2)+'\n')
print('48 original D0830 cases,1200 ordered rays and object selections')
