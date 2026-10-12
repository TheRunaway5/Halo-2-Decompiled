#!/usr/bin/env python3
"""Compare title fog composite programs with retail 25E50/3A5C0.
Hardware submission is stubbed; original arithmetic and packing execute.
This is not framebuffer parity evidence.
"""
import sys, struct, json, ctypes, random, hashlib, argparse

from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library',type=Path,default=ROOT/'build-frontend/libhalo2_kernels.so')
parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/fog-composite-differential.json')
parser.add_argument('--selected-draw',action='store_true',help='execute original 25D50 dispatch and its selected setup')
parser.add_argument('--legacy-native',action='store_true',help='compare dispatch against the old unconditional 25E50 port')
args=parser.parse_args()
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
x=(ROOT/'assets/disc/default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
assert hashlib.sha256(x).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
for i in range(n):
 _,a,_,o,s=struct.unpack_from('<5I',x,table-base+i*56);g.mem_write(a,x[o:o+s])
g.mem_map(0x2100000,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(0x3000000,4096)
c=Cache(ROOT/'assets/disc/maps/mainmenu.map');m=c.magic&~4095;g.mem_map(m,(c.magic+len(c.data)-m+4095)&~4095);g.mem_write(c.magic,bytes(c.data))
_,sc=c.tag(c.scenario,'scnr');b=c.block(sc+0x210,68)[0];file,size,a=struct.unpack_from('<3I',c.data,b);g.mem_write(a,bytes(c.data[file:file+size]))
g.mem_write(0x4e0350,struct.pack('<I',c.magic+sc));g.mem_write(0x4e0348,struct.pack('<I',a+16));g.mem_write(0x4e3b44,struct.pack('<I',0x2300000))
for t in c.tags.values():g.mem_write(0x2300000+(t['datum']&65535)*16+8,struct.pack('<I',t['address']))
plane=c.block(file+16+0x64,24,a-file)[0];pv=struct.unpack_from('<4f',c.data,plane+4);pal=c.block(sc+0x348,16)[c.u16(plane)];datum=c.u32(pal+8);_,fog=c.tag(datum,'fog ');fv=struct.unpack_from('<9f',c.data,fog+12)
sky=next(t for t in c.tags.values()if t['group']=='sky 'and t['name'].endswith('main_menu'));root=sky['address']-c.magic;p=c.block(root+0x48,24)[0];av=struct.unpack_from('<6f',c.data,p)
matg=next(t for t in c.tags.values()if t['group']=='matg');r=c.block(matg['address']-c.magic+0x108,0x1c8)[0];g.mem_write(0x407488,struct.pack('<I',0x2350000));g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000));g.mem_write(0x485a80,struct.pack('<I',r+c.magic));g.mem_write(0x48565c,struct.pack('<f',1024));g.mem_write(0x485ae0,struct.pack('<f',16777215));
# Suppress hardware submission only; the actual fog and packing arithmetic runs.
stubs={0x14bc0:8,0x14f60:0,0x143c0:4,0x222d0:0,0x1cc30:0,0x1c710:4,0x3f5cf0:0,0x3f53d0:4,0x3f5590:4,0x3f54f0:4,0x3f59b0:4}
cap=[]
def hook(guest,address,size,user):
 if args.selected_draw and address==0x34420:
  cap.append(guest.reg_read(UC_X86_REG_EAX));guest.emu_stop();return
 if address==0x222d0:
  cap.append(("STATE",guest.reg_read(UC_X86_REG_ESI),guest.reg_read(UC_X86_REG_EDI)))
 if address in stubs:
  sp=guest.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',guest.mem_read(sp,4))[0];guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
 if address==0x1ccf0:
  cap.append(bytes(guest.mem_read(0x484f68,240)));guest.emu_stop()
def invalid(guest,access,address,size,value,user):
 print("UNMAPPED",hex(address),"EIP",hex(guest.reg_read(UC_X86_REG_EIP)));return False
g.hook_add(UC_HOOK_MEM_INVALID,invalid)
g.hook_add(UC_HOOK_CODE,hook)
def run(entry,words=(),regs=(),until=0x3000000):
 g.mem_write(0x21fff00,struct.pack('<'+'I'*(len(words)+1),0x3000000,*words));g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
 for k,v in regs:g.reg_write(k,v)
 g.emu_start(entry,until,count=1000000)
F,U=ctypes.c_float,ctypes.c_uint32
native=getattr(ctypes.CDLL(str(args.library.resolve())),
 'halo2_menu_fog_draw' if args.selected_draw and not args.legacy_native else 'halo2_menu_fog_composite')
native.argtypes=[ctypes.POINTER(F),ctypes.POINTER(F),ctypes.POINTER(U),ctypes.POINTER(F)]
parameters=(*av,*pv,*fv[6:9],*fv[:3],fv[5])
rng=random.Random(0x25e50)
heights=[pv[3]+step for step in [-6.001,-6,-5.999,-.0001,0,.0001,1022.999,1023,1023.999,1024]]
heights+=[pv[3]+i/256 for i in range(-1538,2)]
heights+=[rng.uniform(100,400)for _ in range(2048)]
failures=[];counts={};fixtures=[]
for z in heights:
 state=bytearray(288);struct.pack_into('<6f',state,0x1c,*av);struct.pack_into('<I',state,0x6c,0xffffffff);struct.pack_into('<I',state,0x9c,datum);struct.pack_into('<3f',state,0xa0,*fv[6:9]);struct.pack_into('<3f',state,0xac,*fv[:3]);struct.pack_into('<2f',state,0xb8,*fv[3:5]);state[0xc0]=1;struct.pack_into('<f',state,0xc4,fv[5]);struct.pack_into('<4f',state,0xf4,*pv);state[0x104]=1
 g.mem_write(0x2100000,bytes(state));g.mem_write(0x2101000,struct.pack('<3f',0,0,z));
 run(0x1301c0,regs=[(UC_X86_REG_ECX,0x2100000)]);run(0x1305d0,[0],[(UC_X86_REG_ECX,0),(UC_X86_REG_EAX,0x2101000),(UC_X86_REG_ESI,0x2100000)]);run(0x130de0,regs=[(UC_X86_REG_ECX,0x2100000)]);
 mode=g.reg_read(UC_X86_REG_EAX);state=bytearray(g.mem_read(0x2100000,288));struct.pack_into('<I',state,0,mode);g.mem_write(0x48574c,bytes(state));cap.clear()
 if args.selected_draw:
  g.mem_write(0x485a75,b'\0');g.mem_write(0x485860,b'\0')
  run(0x25d50)
  if cap:
   setup=cap[-1];assert setup in (0x25e50,0x26520),hex(setup)
   cap.clear();run(setup,[0])
 else:run(0x25e50,[0])
 programs=[v for v in cap if isinstance(v,bytes)]
 expected=programs[-1] if programs else None
 actual=(U*60)();coordinates=(F*8)()
 drawn=native((F*17)(*parameters),(F*3)(0,0,z),actual,coordinates)
 counts[str(mode)]=counts.get(str(mode),0)+1
 if bool(drawn)!=bool(expected):
  failures.append({'eye_z':z,'mode':mode,'native_draws':bool(drawn),'original_draws':bool(expected)})
 elif expected and bytes(actual)!=expected:
  words=struct.unpack('<60I',expected)
  failures.append({'eye_z':z,'mode':mode,'words':[(i,hex(actual[i]),hex(v))for i,v in enumerate(words)if actual[i]!=v]})
 if expected and struct.unpack_from('<I',expected,54*4)[0]==0x2621:
  original=bytearray()
  for stage in [2,3]:
   run(0x26710,[stage,0x2102000,0x2102100,0x2102200])
   original+=g.mem_read(0x2102200,16)
  if bytes(coordinates)!=original:failures.append({'eye_z':z,'coordinates_differ':True})
 if expected and len(fixtures)<9 and z in heights[:10]:
  fixtures.append({'eye_z':z,'mode':mode,'words':list(struct.unpack('<60I',expected)),
                   'coordinates':list(coordinates)})
report={'cases':len(heights),'failures':len(failures),'examples':failures[:5],
        'classification_counts':counts,'entries':['001301c0','001305d0','00130de0','00025e50','0003a5c0','00026710']+(['00025d50','00026520','00131f40'] if args.selected_draw else []),
        'xbe_sha256':hashlib.sha256(x).hexdigest(),'library_sha256':hashlib.sha256(args.library.read_bytes()).hexdigest(),
        'scope':'draw admission, all 60 selected program words and general-path texture coordinates for the retail title atmosphere' if args.selected_draw else 'all 60 composite words and general-path texture coordinates for the retail title atmosphere',
        'retail_frame_parity_verified':False,'fixtures':fixtures}
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='fixtures'},indent=2))
raise SystemExit(bool(failures))
