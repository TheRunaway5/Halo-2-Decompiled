#!/usr/bin/env python3
"""Compare viewport constants c8..c11 with retail 00012FA0 instructions."""
import argparse,ctypes,hashlib,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESP
ROOT=Path(__file__).resolve().parents[1]
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--library',type=Path,default=ROOT/'build-appearance/libhalo2_kernels.so')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/appearance/viewport-constants.json')
 a=p.parse_args();image=(ROOT/'assets/disc/default.xbe').read_bytes()
 if hashlib.sha256(image).hexdigest()!=EXPECTED:raise ValueError('unsupported executable')
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 base=struct.unpack_from('<I',image,0x104)[0];n,t=struct.unpack_from('<II',image,0x11c)
 for i in range(n):
  _,ad,_,off,sz=struct.unpack_from('<5I',image,t-base+i*56);u.mem_write(ad,image[off:off+sz])
 u.mem_map(0x2100000,0x100000)
 # Neutralize only the independent world-to-clip matrix helper; c8..c11 do
 # not read its output. Stop before any device upload. Viewport code stays original.
 u.mem_write(0x12d50,b'\xc2\x04\x00');u.mem_write(0x404fe8,b'\0')
 native=ctypes.CDLL(str(a.library.resolve())).halo2_viewport_constants
 native.argtypes=[ctypes.c_float]*5+[ctypes.c_uint,ctypes.c_uint,ctypes.POINTER(ctypes.c_float)]
 rng=random.Random(0x12fa0)
 cases=[(0,0,w,h,s,f,m) for w,h in [(640,480),(1280,960),(1280,720),(1920,1080),(3840,2160)]
        for s in [1,.5,.01,2] for f in range(4) for m in [0,2]]
 cases += [(rng.randrange(300),rng.randrange(300),rng.randrange(1,8000),rng.randrange(1,8000),
            ctypes.c_float(rng.uniform(-1,2)).value,rng.randrange(4),rng.randrange(3)) for _ in range(4096)]
 failures=[]
 for n,(l,t,w,h,s,f,m) in enumerate(cases):
  u.mem_write(0x2100030,struct.pack('<4h',t,l,t+h,l+w))
  u.mem_write(0x4670c8,struct.pack('<f',s));u.mem_write(0x5093fc,bytes([bool(f&2)]))
  u.mem_write(0x21fff00,struct.pack('<3I',0x3000000,bool(f&1),m))
  u.reg_write(UC_X86_REG_EAX,0x2101000);u.reg_write(UC_X86_REG_ECX,0x2100000)
  u.reg_write(UC_X86_REG_ESP,0x21fff00);u.emu_start(0x12fa0,0x132b3,count=10000)
  expected=bytes(u.mem_read(0x4077b8,64));out=(ctypes.c_float*16)()
  if native(l,t,w,h,s,f,m,out)!=1:raise RuntimeError('native viewport binder failed')
  if bytes(out)!=expected:failures.append({'case':n,'inputs':[l,t,w,h,s,f,m],
    'native':list(out),'retail':list(struct.unpack('<16f',expected))})
 report={'retail_xbe_sha256':EXPECTED,
 'native_library_sha256':hashlib.sha256(a.library.read_bytes()).hexdigest(),
 'function':'00012FA0','cases':len(cases),
 'passed':len(cases)-len(failures),'failures':failures[:20],
 'scope':'c8..c11 only; original viewport arithmetic, render scale clamp and shadow mode; no framebuffer or effect completeness claim'}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
 print(f"Viewport constants: {report['passed']}/{report['cases']} exact")
 return bool(failures)
if __name__=='__main__':raise SystemExit(main())
