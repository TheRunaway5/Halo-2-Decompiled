#!/usr/bin/env python3
"""Unchanged whole30290 sphere rectangle over explicit source camera banks.

Retained original rendering bank and declared scale/sphere/projection/short
viewport variants execute every original instruction, including x87 square
roots, without replacement. This validates floating screen bounds, not integer
scissors, live collector chronology or framebuffer parity.
"""
import argparse,hashlib,json,random,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn import x86_const as R
from oracle_effect_light_snapshot import Snapshot,u
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
def sha(b):return hashlib.sha256(b).hexdigest()
def pack(v):return struct.pack('<'+'I'*len(v),*v)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 s=Snapshot(a.snapshot);assert sha(s.ram)=='68271994d841fcc777be6b2b9271c311cb784c00521020aa8b20e9694c45ff0e';x=Xbe(ROOT/'assets/disc/default.xbe');assert sha(x.data)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
 g=s.guest();g.mem_map(0x70000000,0x10000);camera,bank,point,out,stack,stop=0x70001000,0x70002000,0x70003000,0x70004000,0x7000e000,0x7000ff00
 executed={}
 def trace(vm,pc,size,_):
  if pc not in executed:
   raw=bytes(vm.mem_read(pc,size));assert raw==x.read(pc,size),f'Changed original instruction{pc:08x}';executed[pc]=raw
 g.hook_add(UC_HOOK_CODE,trace)
 actual=list(struct.unpack('<48f',s.read(0x4b9e14,192)));identity=actual.copy();identity[:13]=[1,1,0,0,0,1,0,0,0,1,0,0,0]
 layouts=[(0,0,480,640),(0,0,720,1280),(0,0,1080,1920),(0,0,239,426),(0,426,239,853),(239,0,479,426),(239,426,479,853),(-1,-3,241,319)]
 cases=[];rng=random.Random(0x302902cbf0)
 for radius in [0.,.01,.125,1.,2.,32.]:
  for z in [-64.,-radius,-.01,0.,radius,64.]:
   for axis in [-8.,-radius,0.,radius,8.]:cases.append((identity.copy(),[0.,0.,0.],[axis,-axis,z],radius))
 for i in range(2048):
  b=(actual if i%2 else identity).copy();b[0]=rng.choice([0.,.125,.5,1.,2.]);b[30]=rng.uniform(.1,4.);b[35]=rng.uniform(-4.,-.1);b[38]=rng.uniform(-.5,.5);b[39]=rng.uniform(-.5,.5)
  cases.append((b,[rng.uniform(-16,16)for _ in range(3)],[rng.uniform(-64,64)for _ in range(3)],rng.choice([0.,.125,1.,2.,7.293,32.])))
 data=bytearray(pack([0x52533248,1,len(cases)]));empty=full=bounded=0
 for n,(b,eye,center,radius) in enumerate(cases):
  viewport=layouts[n%len(layouts)];inputs=struct.pack('<48f3f4h4f',*b,*eye,*viewport,*center,radius)
  g.mem_write(bank,inputs[:192]);g.mem_write(camera,bytes(0x40));g.mem_write(camera,inputs[192:204]);g.mem_write(camera+0x30,inputs[204:212]);g.mem_write(point,inputs[212:228]);g.mem_write(out,pack([0x3f880001,0xbf880002,0x3f080003,0xbf080004]));g.mem_write(stack,pack([stop,u(inputs,224)]))
  for register,value in [(R.UC_X86_REG_EAX,point),(R.UC_X86_REG_ECX,bank),(R.UC_X86_REG_EDX,camera),(R.UC_X86_REG_ESI,out),(R.UC_X86_REG_ESP,stack),(R.UC_X86_REG_FPCW,0x23f),(R.UC_X86_REG_MXCSR,0x1fa0)]:g.reg_write(register,value)
  g.emu_start(0x30290,stop,count=1000000);assert g.reg_read(R.UC_X86_REG_EIP)==stop,hex(g.reg_read(R.UC_X86_REG_EIP))
  result=bytes(g.mem_read(out,16));w=struct.unpack('<4I',result)
  if w==(0x7f7fffff,0xff7fffff,0x7f7fffff,0xff7fffff):empty+=1
  elif w==(0xff7fffff,0x7f7fffff,0xff7fffff,0x7f7fffff):full+=1
  else:bounded+=1
  data+=inputs+result
 (a.output/'fixture.bin').write_bytes(data);report=dict(boundary=__doc__,snapshot_sha256=sha(s.ram),original_xbe_sha256=sha(x.data),fixture_sha256=sha(data),tool_sha256=sha(Path(__file__).read_bytes()),intercepted_instructions=0,instruction_count=len(executed),instruction_bank_sha256=sha(b''.join(pack([pc])+raw for pc,raw in sorted(executed.items()))),cases=len(cases),layouts=layouts,empty=empty,full=full,bounded=bounded)
 (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'fixture.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
