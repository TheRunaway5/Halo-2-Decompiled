#!/usr/bin/env python3
"""Validate geometry choice against untouched retail4C450,2C490 and4BCC0 cap instructions."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x21fff00,0x3000000
MODEL,RESULT,OBJECT_POOL,DESCRIPTORS,OBJECT,CACHE_POOL,CACHE_RECORD,CONTEXT= [BASE+i*0x1000 for i in range(8)]
HANDLE=0x12340000

def f(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,default=ROOT/'build-mp-shading/source_geometry_lod_tests')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/geometry-lod-differential.json')
 p.add_argument('--write-fixture',type=Path)
 a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
 if hashlib.sha256(x.data).hexdigest()!=PIN:raise RuntimeError('unsupported original executable')
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x100000);u.mem_map(STOP,0x1000)
 def put(at,value):u.mem_write(at,struct.pack('<I',value&0xffffffff))
 def get(at):return struct.unpack('<I',u.mem_read(at,4))[0]
 put(0x4e0300,OBJECT_POOL);put(OBJECT_POOL+0x44,DESCRIPTORS);put(DESCRIPTORS+8,OBJECT)
 put(OBJECT+0xcc,HANDLE);put(0x509434,CACHE_POOL);put(CACHE_POOL+0x44,CACHE_RECORD);put(CACHE_RECORD+4,HANDLE)
 def run(at,end=STOP):
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(at,end,count=10000)
  if u.reg_read(UC_X86_REG_EIP)!=end:raise RuntimeError('original geometry function did not finish')
 def original(row):
  present,metric,previous,players,flags,prt,*bank=row
  put(STACK,STOP);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EAX,players);run(0x2c490)
  policy=[get(0x4ba01c),get(0x4ba028),get(0x4ba02c),get(0x4670e4),*u.mem_read(0x4ba020,6)]
  u.mem_write(MODEL,struct.pack('<9I',*bank));u.mem_write(CACHE_RECORD+0xbc,bytes([previous&255])*4)
  put(0x4ba050,players%4);put(STACK,STOP);put(STACK+4,HANDLE)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_ECX,MODEL if present else 0)
  u.reg_write(UC_X86_REG_EBX,RESULT);u.reg_write(UC_X86_REG_XMM4,metric);run(0x4c450)
  choice=u.reg_read(UC_X86_REG_EAX);fraction=get(RESULT)
  u.reg_write(UC_X86_REG_ESP,STACK);put(STACK+0x30,RESULT+4);put(STACK+0x14,OBJECT)
  u.reg_write(UC_X86_REG_EDI,CONTEXT);u.reg_write(UC_X86_REG_ECX,RESULT+4)
  u.mem_write(CONTEXT+0x16c,bytes([0x40 if prt else 0]));u.reg_write(UC_X86_REG_EAX,choice)
  if flags&0x80000:
   # Exact original force5 instruction, before the unrelated material branch.
   run(0x4becb,0x4bece)
  else:run(0x4be52,0x4be83)
  return [choice,fraction,u.mem_read(RESULT+4,1)[0],*policy]
 rng=random.Random(0x4c450);rows=[]
 banks=[[0]*9,[f(12),0,0,*map(f,[10,8,6,4,2,0])],[f(30),0,0,*map(f,[16,12,8,4,1,0])]]
 # Invalid authored banks independently exercise each source guard.
 for j in range(3,9):
  b=banks[2].copy();b[j]=f(-1 if j==3 else (50 if j<8 else 1));banks.append(b)
 for bank in banks:
  for metric in [-1,0,.1,.2,1,1.4,1.6,2,3,4,5,6,7,8,9,10,11,12,16,30,31]:
   for prev in [-1,0,1,2,3,4,5]:
    for players in [0,1,2,3,4,5]:
     rows.append([1,f(metric),prev&255,players,0x80000 if len(rows)%13==0 else 0,len(rows)%2,*bank])
 # All source bank fixtures, each at boundary and an independent metric/history.
 for line in (ROOT/'tests/fixtures/retail_geometry_lod_banks.tsv').read_text().splitlines():
  if not line or line.startswith('#'):continue
  fields=line.split();bank=[int(v) for v in fields[-9:]]
  for metric in [bank[3],f(rng.uniform(0,60))]:
   rows.append([1,metric,rng.choice([255,0,1,2,3,4,5]),rng.randrange(1,5),0,rng.randrange(2),*bank])
 for i in range(256):rows.append([0,f(rng.uniform(-1,20)),i%7-1&255,i%6,0,i%2,*[rng.getrandbits(32) for _ in range(9)]])
 expected=[original(r) for r in rows]
 encoded=[' '.join(f'{v:x}' for v in r) for r in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',capture_output=True,text=True,check=True)
 actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
 if len(actual)!=len(rows):raise RuntimeError('native probe lost geometry cases')
 failures=[{'case':i,'input':row,'source':s,'native':n} for i,(row,s,n) in enumerate(zip(rows,expected,actual)) if s!=n]
 if a.write_fixture and not failures:
  chosen=range(0,len(rows),max(1,len(rows)//256))
  lines=['# Untouched original4C450/2C490/4BCC0 cap slices; executable '+PIN,
   '# 15 hexadecimal input words(present,metric,previous,players,flags,prt,bank9) then13 outputs(choice,transition,cap,policy10).']
  lines.extend(encoded[i]+' '+' '.join(f'{v:08x}' for v in expected[i]) for i in chosen)
  a.write_fixture.write_text('\n'.join(lines)+'\n')
 report={'original_executable_sha256':PIN,'native_binary_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),
  'original_entries':['0004C450','0003DDD0 retained valid cache','0002C490','0004BE52..0004BE83','0004BECB'],
  'cases':len(rows),'compared_words':len(rows)*13,'bit_exact_cases':len(rows)-len(failures),'failures':failures[:12],
  'scope':'Explicit actual distance/bank/per-view prior LOD/player-count/flags/PRT inputs. Visibility and resource residency remain separate caller services.'}
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2));return bool(failures)
if __name__=='__main__':raise SystemExit(main())
