#!/usr/bin/env python3
"""Untouched133790 x87 distance and2F970 view modifier instruction slices."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK=0x2100000,0x21fe000
CAMERA,SCENARIO=BASE,BASE+0x1000

def f(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'build-mp-shading/source_geometry_lod_tests')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/geometry-lod-metric-differential.json');p.add_argument('--write-fixture',type=Path)
 a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
 if hashlib.sha256(x.data).hexdigest()!=PIN:raise RuntimeError('unsupported original executable')
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x100000)
 def put(at,v):u.mem_write(at,struct.pack('<I',v&0xffffffff))
 def original(row):
  u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_ESP,STACK);u.mem_write(STACK+0x1a4,struct.pack('<3I',*row[:3]))
  for i in range(3):put(0x4b9da0+i*4,row[3+i])
  put(0x4b9dcc,row[6]);put(0x4b9ed0,f(1));put(0x4ba028,f(1))
  u.reg_write(UC_X86_REG_XMM0,f(1));u.reg_write(UC_X86_REG_XMM2,f(1))
  u.emu_start(0x133873,0x1338ff,count=1000)
  distance=u.reg_read(UC_X86_REG_XMM1)&0xffffffff
  u.mem_write(CAMERA+0x58,bytes([row[7]&255]));u.mem_write(CAMERA+0x68,bytes([row[8]&255]))
  put(0x4e0350,SCENARIO if row[9] else 0);u.mem_write(SCENARIO+0x10,struct.pack('<H',row[10]&65535))
  u.reg_write(UC_X86_REG_EAX,CAMERA);u.reg_write(UC_X86_REG_ESP,STACK);put(STACK,0)
  u.emu_start(0x2fa26,0x2fa63,count=1000)
  return [distance,u.mem_read(0x4ba019,1)[0]]
 rng=random.Random(0x133790);rows=[]
 for i in range(8192):
  xyz=[f(rng.uniform(-1e4,1e4))for _ in range(6)]
  if i%16==0:xyz[3:]=xyz[:3]
  elif i%16==1:xyz=[f(v) for v in [1e-5,2e-5,3e-5,-1e-5,-2e-5,-3e-5]]
  rows.append([*xyz,f(rng.choice([.5,.785,1.,1.7,2.,4.])),i%2,(i//2)%2,(i//4)%2,(i//8)%4])
 expected=[original(row)for row in rows];encoded=[' '.join(f'{v:x}'for v in row)for row in rows]
 run=subprocess.run([str(a.binary.resolve()),'--metric-probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16)for v in line.split()]for line in run.stdout.splitlines()]
 if len(actual)!=len(rows):raise RuntimeError('native metric probe lost rows')
 failures=[{'case':i,'input':r,'source':s,'native':n}for i,(r,s,n)in enumerate(zip(rows,expected,actual))if s!=n]
 if a.write_fixture and not failures:
  lines=['# Untouched133790/2F970 instruction slices; executable '+PIN,'# 11 hex inputs(center3,eye3,scale,cam58,cam68,scnrpresent,scnrtype) then2 output words.']
  lines.extend(encoded[i]+' '+' '.join(f'{v:08x}'for v in expected[i])for i in range(0,len(rows),63))
  a.write_fixture.write_text('\n'.join(lines)+'\n')
 report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
  'original_instructions':['00133873..001338FF','0002FA26..0002FA63'],'cases':len(rows),'bit_exact_cases':len(rows)-len(failures),'compared_words':len(rows)*2,'failures':failures[:12],
  'scope':'Actual cached sphere/camera/scale/explicit camera bytes and optional scenario type; no portal/admission substitution.'}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
