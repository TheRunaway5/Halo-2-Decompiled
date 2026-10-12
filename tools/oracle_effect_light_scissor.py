#!/usr/bin/env python3
"""Unchanged1650A0 projected-sphere and4A3D7..4A677 scissor/depth owner.

Declared finite transform/sphere/camera/viewport inputs vary. Every executed
instruction is checked against the retained original XBE, including the CRT
asin and x87 transcendental corridor. Stops before3A550 GPU submission. No
interception, light collection, material binding or gameplay frame is claimed.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e'
def sha(b):return hashlib.sha256(b).hexdigest()
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)==PIN;x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 g=s.guest();g.mem_map(0x70000000,0x10000);stack,stop,camera,light,out=0x7000e000,0x7000ff00,0x70001000,0x70002000,0x70003000
 executed={}
 def trace(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Changed original instruction{pc:08x}';executed[pc]=raw
 g.hook_add(UC_HOOK_CODE,trace)
 def read(at,n):
  regions=list(g.mem_regions())
  for page in range(at&~4095,(at+n+4095)&~4095,4096):
   if not any(b<=page<=e for b,e,_ in regions):g.mem_map(page,4096);g.mem_write(page,s.read(page,4096))
  return bytes(g.mem_read(at,n))
 def write(at,raw):read(at,len(raw));g.mem_write(at,raw)
 def run(entry,end,regs,args):
  g.mem_write(stack,pack([stop,*args]));g.reg_write(R.UC_X86_REG_ESP,stack);g.reg_write(R.UC_X86_REG_FPCW,0x23f);g.reg_write(R.UC_X86_REG_MXCSR,0x1fa0)
  for reg,value in regs.items():g.reg_write(reg,value)
  g.emu_start(entry,end,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==end,hex(g.reg_read(R.UC_X86_REG_EIP))
 actual_camera=u(s.read(0x547f88,4));original=list(struct.unpack('<13f',s.read(actual_camera+8,52)))
 identity=[1.,1.,0.,0.,0.,1.,0.,0.,0.,1.,0.,0.,0.]
 seed=struct.pack('<6I',0x3f880001,0xbf880002,0x3f080003,0xbf080004,0x41880005,0xc1880006)
 layouts=[(0,0,480,640),(0,0,720,1280),(0,0,1080,1920),(0,0,360,640),(0,640,360,1280),(360,0,720,640),(360,640,720,1280),(-1,-3,241,319)]
 rng=random.Random(0x1650a04a1e0);rows=[];data=bytearray(pack([0x534c3248,1,0]));counts=dict(projected=0,rejected=0,bounded=0,fallback=0,absent=0)
 cases=[]
 for radius in [0.,.015625,.125,1.,2.,3.,32.]:
  for z in [-64.,-radius,-.015625,0.,radius,64.]:
   for xaxis in [0.,radius,-radius,8.]:cases.append((identity,[xaxis,-xaxis,z],radius))
 for i in range(512):
  t=(identity if i%2 else original).copy();t[0]=rng.choice([0.,.125,.5,1.,2.]);t[10:]=[rng.uniform(-16,16)for _ in range(3)]
  cases.append((t,[rng.uniform(-32,32)for _ in range(3)],rng.choice([.015625,.125,1.,2.,7.293,32.])))
 for n,(transform,center,radius) in enumerate(cases):
  viewport=layouts[n%len(layouts)];planes=[-.625,.625,-.46875,.46875]
  if n%11==0:planes=[-.5,.25,-.75,.125]
  if n%23==0:planes[1]=planes[0]
  if n%29==0:planes[3]=planes[2]
  near,far=rng.choice([.015625,.1,1.]),rng.choice([32.,1024.]);present=n%17!=0
  floats=struct.pack('<23f',*transform,*center,radius,*planes,near,far)
  # All comparisons use the same float32 input words as the native fixture.
  transform,center,radius=struct.unpack('<13f',floats[:52]),struct.unpack('<3f',floats[52:64]),struct.unpack('<f',floats[64:68])[0]
  g.mem_write(camera,bytes(0xb4));g.mem_write(camera+8,floats[:52]);g.mem_write(camera+0x74,floats[84:88]);g.mem_write(camera+0x8c,floats[88:92]);g.mem_write(camera+0xa4,floats[68:84]);g.mem_write(light+0x28,floats[52:68]);g.mem_write(out,seed)
  run(0x1650a0,stop,{R.UC_X86_REG_ECX:camera+4,R.UC_X86_REG_EAX:light+0x28,R.UC_X86_REG_EDI:out},[u(floats,64)])
  valid=g.reg_read(R.UC_X86_REG_EAX)&255;assert valid in (0,1);projected=bytes(g.mem_read(out,24));counts['projected' if valid else 'rejected']+=1
  write(0x485a8a,struct.pack('<4h',*viewport));write(0x4858d0,b'\x72');write(0x485964,seed)
  run(0x4a3d7,0x4a677,{R.UC_X86_REG_EBP:light if present else 0,R.UC_X86_REG_ESI:camera},[])
  scissor=bytes(g.mem_read(0x485964,24));mode=bytes(g.mem_read(0x4858d0,1))[0];assert mode==int(present)
  fallback=struct.pack('<6f',viewport[1],viewport[3],viewport[0],viewport[2],-far,-near)
  counts['absent' if not present else 'fallback' if scissor==fallback else 'bounded']+=1
  data+=floats+struct.pack('<4hI',*viewport,present)+seed+pack([valid])+projected+scissor+pack([mode])
  rows.append(dict(present=present,projected=valid,viewport=viewport,bounded=scissor!=fallback))
 struct.pack_into('<I',data,8,len(rows));(a.output/'fixture.bin').write_bytes(data);(a.output/'tool.py').write_bytes(Path(__file__).read_bytes())
 report=dict(boundary=__doc__,snapshot_sha256=PIN,original_xbe_sha256=sha(x.data),tool_sha256=sha(Path(__file__).read_bytes()),instruction_count=len(executed),instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),intercepted_instructions=0,fixture_sha256=sha(data),cases=len(rows),counts=counts,layouts=layouts)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'fixture.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
