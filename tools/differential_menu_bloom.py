#!/usr/bin/env python3
"""Recover/compare the original title bloom's programs and blur schedule."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--export-only',action='store_true');args=parser.parse_args()
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000);g.mem_map(0x2100000,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(0x3000000,4096)
x=(ROOT/'assets/disc/default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
assert hashlib.sha256(x).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
for i in range(n):
 _,a,_,o,s=struct.unpack_from('<5I',x,table-base+i*56);g.mem_write(a,x[o:o+s])
g.mem_write(0x407488,struct.pack('<I',0x2350000));g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000))
g.mem_write(0x5093e4,struct.pack('<I',0x2100000));g.mem_write(0x510c50,struct.pack('<I',0x2101000));g.mem_write(0x2101005,b'\1');g.mem_write(0x4858b4,b'\xff'*4)
stubs={0x14bc0:8,0x14f60:0,0x15780:4,0x222d0:0,0x1ccf0:0,0x3fa3d0:0,0x34a90:44}
draws=[];programs=[]
def hook(guest,address,size,user):
 if address not in stubs:return
 sp=guest.reg_read(UC_X86_REG_ESP)
 if address in (0x1ccf0,0x3fa3d0):programs.append(list(struct.unpack('<60I',guest.mem_read(0x484f68,240))))
 if address==0x34a90:
  words=struct.unpack('<11I',guest.mem_read(sp+4,44));draws.append({'target':words[0],'radius':struct.unpack('<f',struct.pack('<I',words[6]))[0],'stages':words[8],'program':list(struct.unpack('<60I',guest.mem_read(0x484f68,240)))})
  guest.reg_write(UC_X86_REG_EAX,words[0])
 ret=struct.unpack('<I',guest.mem_read(sp,4))[0];guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,hook)
F,U=ctypes.c_float,ctypes.c_uint32
native=None
if not args.export_only:
 native=ctypes.CDLL(str(ROOT/'build-frontend/libhalo2_kernels.so')).halo2_menu_bloom
 native.argtypes=[F,F,ctypes.POINTER(U),ctypes.POINTER(F)]
failures=[];fixtures=[];rng=random.Random(0x214f0)
cases=[(.3,.375)]+[(rng.random(),rng.random())for _ in range(1024)]
for threshold,strength in cases:
 override=bytearray(36);override[0]=1
 for off in [4,8,12,16,20,28,32]:struct.pack_into('<f',override,off,-1.)
 struct.pack_into('<f',override,8,threshold);g.mem_write(0x2100000,bytes(override));g.mem_write(0x485a74,b'\0')
 params=[0x407f7cee,*struct.unpack('<II',struct.pack('<2f',.9,strength)),0x3e800000,0,1,0]
 g.mem_write(0x21fff00,struct.pack('<8I',0x3000000,*params));g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
 programs.clear();draws.clear();g.emu_start(0x214f0,0x3000000,count=1000000)
 assert g.reg_read(UC_X86_REG_EIP)==0x3000000,hex(g.reg_read(UC_X86_REG_EIP))
 assert len(draws)==6 and len(programs)==3,(len(draws),len(programs))
 if native:
  out=(U*180)();radii=(F*5)();native(F(threshold),F(strength),out,radii)
  expected=sum(programs,[])
  if list(out)!=expected:failures.append({'threshold':threshold,'strength':strength,'words':[(i,hex(a),hex(b))for i,(a,b)in enumerate(zip(out,expected))if a!=b]})
  if list(radii)!=[d['radius']for d in draws[:5]]:failures.append({'radii':list(radii),'expected':[d['radius']for d in draws[:5]]})
 if not fixtures:fixtures.append({'threshold':threshold,'strength':strength,'draws':draws.copy(),'programs':programs.copy()})
# Verify the default cinematic amount/brightness and its resulting strength,
# rather than taking .375 as an unexplained input to the blur-only oracle.
stubs.update({0x1f2b0:0,0x20e50:0,0x16b10:0})
g.mem_write(0x4e034c,struct.pack('<I',0x2120000))
g.mem_write(0x5093e0,bytes(4));g.mem_write(0x50934c,struct.pack('<I',0x2121000))
def invoke(entry,arguments=()):
 g.mem_write(0x21fff00,struct.pack('<'+'I'*(len(arguments)+1),0x3000000,*arguments))
 g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
 g.emu_start(entry,0x3000000,count=1000000)
 assert g.reg_read(UC_X86_REG_EIP)==0x3000000,hex(g.reg_read(UC_X86_REG_EIP))
invoke(0x22590)
# cinematic_start's timeline selection, then execute the original query.
g.mem_write(0x2121000,b'\1');g.mem_write(0x4858a0,struct.pack('<d',1./30.))
invoke(0x137a0,(0x2122000,0x2122004))
amount,brightness=struct.unpack('<2f',g.mem_read(0x2122000,8))
assert amount==brightness==.5,(amount,brightness)
cinematic_draw=[]
def capture_strength(guest,address,size,user):
 if address!=0x214f0:return
 sp=guest.reg_read(UC_X86_REG_ESP)
 cinematic_draw.append(list(struct.unpack('<3f',guest.mem_read(sp+4,12))))
 ret=struct.unpack('<I',guest.mem_read(sp,4))[0]
 guest.reg_write(UC_X86_REG_ESP,sp+32);guest.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,capture_strength)
g.mem_write(0x485890,bytes(4));g.mem_write(0x4b9fd0,bytes(4));g.mem_write(0x4e0350,bytes(4))
invoke(0x13870,(0,1))
assert len(cinematic_draw)==1 and cinematic_draw[0][2]==.375,cinematic_draw
report={'cases':len(cases),'failures':len(failures),'examples':failures[:5],
 'cinematic_defaults':{'amount':amount,'brightness':brightness,'strength':cinematic_draw[0][2]},
 'entries':['000214f0','000211a0','00020f30','00022590','000137a0','00013870'],
 'xbe_sha256':hashlib.sha256(x).hexdigest(),'export_only':args.export_only,'framebuffer_parity_verified':False,'fixtures':fixtures}
(ROOT/'analysis/frontend/bloom-differential.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items()if k!='fixtures'},indent=2));raise SystemExit(bool(failures))
