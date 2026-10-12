#!/usr/bin/env python3
"""Execute retail patchy-fog calculations with reproducible camera/RNG inputs.
The original executable is a test oracle only. Hardware submission is suppressed.
"""
import argparse, ctypes, hashlib, json, math, random, struct, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--export-only',action='store_true')
parser.add_argument('--library',type=Path,default=ROOT/'build-frontend/libhalo2_kernels.so')
parser.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/patchy-fog-differential.json')
args=parser.parse_args()
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000)
x=(ROOT/'assets/disc/default.xbe').read_bytes();base=struct.unpack_from('<I',x,0x104)[0];n,table=struct.unpack_from('<II',x,0x11c)
assert hashlib.sha256(x).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
for i in range(n):
 _,a,_,o,s=struct.unpack_from('<5I',x,table-base+i*56);g.mem_write(a,x[o:o+s])
g.mem_map(0x2100000,0x100000);g.mem_map(0x2300000,0x100000);g.mem_map(0x3000000,4096)
c=Cache(ROOT/'assets/disc/maps/mainmenu.map');m=c.magic&~4095;g.mem_map(m,(c.magic+len(c.data)-m+4095)&~4095);g.mem_write(c.magic,bytes(c.data))
g.mem_write(0x4e3b44,struct.pack('<I',0x2300000))
for t in c.tags.values():g.mem_write(0x2300000+(t['datum']&65535)*16+8,struct.pack('<I',t['address']))
matg=next(t for t in c.tags.values()if t['group']=='matg')
r=c.block(matg['address']-c.magic+0x108,0x1c8)[0];g.mem_write(0x485a80,struct.pack('<I',r+c.magic))
sky=next(t for t in c.tags.values()if t['group']=='sky 'and t['name'].endswith('main_menu'))
p=c.block(sky['address']-c.magic+0x60,80)[0];datum=c.u32(p+0x4c);_,fpch=c.tag(datum,'fpch')
g.mem_write(0x4857b8,struct.pack('<I',datum));g.mem_write(0x4857bc,bytes(c.data[p:p+40]))
g.mem_write(0x4e7408,struct.pack('<I',0x2104000));g.mem_write(0x407488,struct.pack('<I',0x2350000));g.mem_write(0x2350000,struct.pack('<II',0x23c0000,0x23f0000));g.mem_write(0x48565c,struct.pack('<f',1024));g.mem_write(0x485ae0,struct.pack('<f',16777215))
stubs={0x14bc0:8,0x14f60:0,0x143c0:4,0x222d0:0,0x1cc30:0,0x1c710:4,0x3f53d0:4,0x3f5590:4,0x3f54f0:4,0x3f59b0:4,0x3f5510:4,0x3f5cf0:0}
captured=[]
def hook(guest,address,size,user):
 if address in (0x1ccf0,0x3fa3d0):
  captured.append(list(struct.unpack('<60I',guest.mem_read(0x484f68,240))));guest.emu_stop()
 elif address in stubs:
  sp=guest.reg_read(UC_X86_REG_ESP);ret=struct.unpack('<I',guest.mem_read(sp,4))[0];guest.reg_write(UC_X86_REG_ESP,sp+4+stubs[address]);guest.reg_write(UC_X86_REG_EIP,ret)
g.hook_add(UC_HOOK_CODE,hook)
def run(entry,words=(),regs=()):
 g.mem_write(0x21fff00,struct.pack('<'+'I'*(len(words)+1),0x3000000,*words));g.reg_write(UC_X86_REG_ESP,0x21fff00);g.reg_write(UC_X86_REG_MXCSR,0x1f80);g.reg_write(UC_X86_REG_FPCW,0x027f)
 for k,v in regs:g.reg_write(k,v)
 try:g.emu_start(entry,0x3000000,count=1000000)
 except Exception:
  print('oracle entry',hex(entry),'fault eip',hex(g.reg_read(UC_X86_REG_EIP)));raise
 assert g.reg_read(UC_X86_REG_EIP) in (0x3000000,0x1ccf0,0x3fa3d0),hex(g.reg_read(UC_X86_REG_EIP))
F,U=ctypes.c_float,ctypes.c_uint32
parameters=struct.unpack_from('<20f',c.data,fpch)
native=None
if not args.export_only:
 native=ctypes.CDLL(str(args.library.resolve())).halo2_patchy_fog
 native.argtypes=[ctypes.POINTER(F),ctypes.POINTER(F),ctypes.POINTER(F),ctypes.POINTER(U),ctypes.POINTER(F),ctypes.POINTER(F),ctypes.POINTER(U)]
frames=[];failures=[];max_error=0.;program_failures=0;geometry_cases=0;maximum_uv_error=0.
viewports=[(640,480),(1280,720),(1280,800),(1680,720),(1920,1080),(1000,600)]
quad=((0.,0.),(0.,1.),(1.,1.),(1.,0.))
def f32(value):return F(value).value
rng=random.Random(0x27aa0)
sequences,frame_count=48,64
for sequence in range(sequences):
 seed=rng.getrandbits(32);g.mem_write(0x2104000,struct.pack('<II',0x78a8,seed));g.mem_write(0x4b9a04,bytes(0x88));g.mem_write(0x4b9a00,bytes(4))
 state=(F*34)();native_seed=U(seed)
 for frame in range(frame_count):
  # Input is the actual right/up/backward view transform ABI. Inversion and
  # composition run in the original executable, including persistent motion.
  angle=frame*.011 if sequence else 0.;cs,sn=math.cos(angle),math.sin(angle)
  pitch=math.sin(frame*.031)*.5 if sequence>1 else 0.;cp,sp=math.cos(pitch),math.sin(pitch)
  roll=math.sin(frame*.029)*.3 if sequence>2 else 0.;cr,sr=math.cos(roll),math.sin(roll)
  view=(1,cs*cp,sn*cp,-sp,cs*sp*sr-sn*cr,sn*sp*sr+cs*cr,cp*sr,
        cs*sp*cr+sn*sr,sn*sp*cr-cs*sr,cp*cr,frame*.3,frame*.2,-frame*.6)
  if sequence>20:view=(*view[:10],rng.uniform(-30,30),rng.uniform(-30,30),rng.uniform(-50,50))
  fz=rng.uniform(-1,1);fov=rng.uniform(.6,1.3);time=frame/30.
  width,height=viewports[0 if sequence<2 else (sequence+frame//16)%len(viewports)]
  inputs=(*view,fz,fov,time,width,height)
  # Original rectangle ABI is top, left, bottom, right. The previous test
  # swapped bottom/right and thereby hid the runtime's inverse aspect bug.
  g.mem_write(0x48568c,struct.pack('<13f',*view));g.mem_write(0x48562c,struct.pack('<f',fz));g.mem_write(0x485640,struct.pack('<f',fov));g.mem_write(0x485648,struct.pack('<4h',0,0,height,width));g.mem_write(0x4858a0,struct.pack('<d',time))
  run(0x27aa0)
  layers=struct.unpack('<56f',g.mem_read(0x4b9cac,224));vertical=struct.unpack('<f',g.mem_read(0x4b9d94,4))[0]
  original_seed=struct.unpack('<I',g.mem_read(0x2104004,4))[0]
  captured.clear();run(0x26880);noise=captured[-1]
  programs=[]
  for direct in [False,True]:
   g.mem_write(0x5093fe,bytes([direct]));captured.clear();run(0x26e50,[0]);programs.append(captured[-1])
  # 34420 supplies the four unit-square corners to 26CA0, one attribute per
  # layer. Execute that unchanged callback instead of assuming its UV math.
  source_uv=[]
  g.mem_write(0x2108000,struct.pack('<4f',0,width,0,height))
  for layer in range(4):
   corners=[]
   for uv in quad:
    g.mem_write(0x2108100,struct.pack('<2f',*uv))
    g.mem_write(0x2108200,struct.pack('<4f',0,0,0,1))
    run(0x26ca0,[layer+1,0x2108000,0x2108100,0x2108200,0])
    assert g.reg_read(UC_X86_REG_EAX)&255==1
    corners.append(struct.unpack('<4f',g.mem_read(0x2108200,16)))
   source_uv.append(corners)
  if native:
   output=(F*57)();words=(U*180)()
   native((F*20)(*parameters),(F*10)(*struct.unpack_from('<10f',c.data,p)),(F*18)(*inputs),ctypes.byref(native_seed),state,output,words)
   expected=(*layers,vertical)
   # First two words of each layer are integer bitmap index/interpolation.
   for i,(a,b) in enumerate(zip(output,expected)):
    if i<56 and i%14<2:
     if struct.pack('<f',a)!=struct.pack('<f',b):failures.append((sequence,frame,i,'integer'))
    else:
     error=abs(a-b);max_error=max(error,max_error)
     if not math.isfinite(a) or error>2e-5:failures.append((sequence,frame,i,a,b))
   if native_seed.value!=original_seed:failures.append((sequence,frame,'seed',native_seed.value,original_seed))
   for layer in range(4):
    r=output[layer*14+6:layer*14+10];s=output[layer*14+10:layer*14+14]
    for uv,original in zip(quad,source_uv[layer]):
     px,py=(f32(v*2.-1.) for v in uv)
     actual=[f32(f32(f32(px*row[0])-f32(f32(py*row[1])*output[56]))+row[3]) for row in (r,s)]+[0.,1.]
     geometry_cases+=1
     for channel,(a,b) in enumerate(zip(actual,original)):
      error=abs(a-b);maximum_uv_error=max(maximum_uv_error,error)
      if not math.isfinite(a) or error>2e-5:failures.append((sequence,frame,'geometry',layer,uv,channel,a,b))
   if list(words)!=noise+programs[0]+programs[1]:
    program_failures+=1
    failures.append((sequence,frame,'program',[(i,hex(a),hex(b))for i,(a,b)in enumerate(zip(words,noise+programs[0]+programs[1]))if a!=b]))
  if sequence<2 and frame<4:frames.append({'sequence':sequence,'frame':frame,'seed_input':seed,'seed_after':original_seed,'camera':inputs,'layers':layers,'vertical':vertical,'noise':noise,'composite_cached':programs[0],'composite_direct':programs[1],'source_uv':source_uv})
report={'cases':sequences*frame_count,'geometry_cases':geometry_cases,'viewports':viewports,'original_rectangle_abi':'top,left,bottom,right','failures':len(failures),'program_failures':program_failures,'examples':failures[:8],'maximum_float_error':max_error,'maximum_uv_error':maximum_uv_error,'entries':['00027aa0','00028580','00026880','00026e50','00026ca0'],'xbe_sha256':hashlib.sha256(x).hexdigest(),'export_only':args.export_only,'framebuffer_parity_verified':False,'fixtures':frames}
args.output.parent.mkdir(parents=True,exist_ok=True)
args.output.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items()if k!='fixtures'},indent=2))
raise SystemExit(bool(failures))
