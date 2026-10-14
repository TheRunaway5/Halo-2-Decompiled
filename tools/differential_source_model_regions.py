#!/usr/bin/env python3
"""Compare live model-region decisions with unchanged Xbox16D660/16DAD0/16DCE0.
Effect retirement/allocation are explicit owner boundaries; no Havok, rendering,
attachment rebinding or original network behavior is inferred from this test.
"""
from pathlib import Path
import argparse,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
EXPECTED='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE=0x2100000;TAGS=BASE;MODEL=BASE+0x2000;VARIANT=BASE+0x4000;REGIONS=BASE+0x6000
PERMS=BASE+0x8000;CHOICES=BASE+0x9000;RNG=BASE+0xa000;STACK=BASE+0xff000;STOP=BASE+0xfe000
NONE=0xffffffff

def pack(words):return struct.pack('<'+'I'*len(words),*[v&NONE for v in words])
def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def sha(path):
 with path.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def signed(v):return struct.unpack('<i',struct.pack('<I',v))[0]

def parse(row):
 at=8;regions=[]
 for r in range(row[0]):
  current=row[at:at+7];at+=7;permutations=[]
  for p in range(current[6]):
   header=row[at:at+3];at+=3;states=[]
   for s in range(header[2]):states.append(row[at:at+5]);at+=5
   permutations.append((header,states))
  regions.append((current,permutations))
 assert at==len(row)
 return regions

class Original:
 def __init__(self,image):
  self.image=image;self.u=Uc(UC_ARCH_X86,UC_MODE_32);u=self.u;u.mem_map(0x10000,0x600000);u.mem_map(BASE,0x100000)
  self.allowed=[(0x16d660,0x16d881),(0x16dad0,0x16dcd8),(0x16dce0,0x16ddf0)]
  for a,b in self.allowed:u.mem_write(a,image.read(a,b-a))
  for a in [0x45dbb8,0x45dc50]:u.mem_write(a,image.read(a,4))
  self.put(0x4e3b44,TAGS);self.put(TAGS+16+8,MODEL);self.put(0x4e7408,RNG)
  self.visited=set();u.hook_add(UC_HOOK_CODE,self.hook)
 def put(self,a,v):self.u.mem_write(a,pack([v]))
 def word(self,a):return struct.unpack('<I',self.u.mem_read(a,4))[0]
 def ret(self,value=0,pop=0):
  u=self.u;sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_EIP,self.word(sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,u,pc,size,_):
  if pc==STOP:u.emu_stop();return
  if pc==0x177260:
   assert self.word(u.reg_read(UC_X86_REG_ESP)+4)==1
   self.events.extend([0,u.reg_read(UC_X86_REG_EDI),0,0]);self.ret(pop=4);return
  if pc==0x175ee0:
   u.mem_write(u.reg_read(UC_X86_REG_EDX),bytes(0x60));self.ret();return
  if pc==0x175fa0:
   args=self.word(u.reg_read(UC_X86_REG_ESP)+4)
   assert self.word(args)==3 and self.word(args+8)==0x80010020 and self.word(args+0x34)==0x30005a6 and self.word(args+0x38)==0x30005a6
   handle=0x90010000+self.allocations;self.allocations+=1
   self.events.extend([1,self.word(args+4),self.word(args+0x30),handle]);self.ret(handle,4);return
  if not any(a<=pc<b for a,b in self.allowed):raise RuntimeError(f'unexpected region instruction {pc:08x}')
  assert bytes(u.mem_read(pc,size))==self.image.read(pc,size),'original instruction changed'
  self.visited.add(pc)
 def run(self,row):
  u=self.u;self.events=[];self.allocations=0
  u.mem_write(MODEL,bytes(0xc000));u.mem_write(STACK-0x1000,bytes(0x1200))
  self.put(MODEL+0x70,row[0]);self.put(MODEL+0x54,VARIANT);self.put(VARIANT+0x14,row[0]);self.put(VARIANT+0x18,REGIONS)
  self.put(RNG,row[7]);heap=BASE+0x10000
  for r,(current,permutations) in enumerate(parse(row)):
   perm,choice,damage,properties,effect,mapped,_=current
   u.mem_write(PERMS+r,bytes([perm&255]));u.mem_write(CHOICES+r*8,bytes([choice&255,damage&255,properties&255,0])+pack([effect]))
   u.mem_write(VARIANT+4+r,bytes([mapped&255]));self.put(REGIONS+r*0x14+8,len(permutations));self.put(REGIONS+r*0x14+12,heap)
   locations=heap;heap+=len(permutations)*0x20
   for p,(header,states) in enumerate(permutations):
    q=locations+p*0x20;u.mem_write(q+4,bytes([header[0]&255]));self.put(q+8,header[1]);self.put(q+12,len(states));self.put(q+16,heap)
    for state in states:
     perm,props,damage,effect,marker=state
     u.mem_write(heap+4,bytes([perm&255,props&255])+struct.pack('<H',damage));self.put(heap+12,effect);self.put(heap+16,marker);heap+=0x18
  args=[STOP,0x80010020,0x80010001,*row[1:7],PERMS,CHOICES]
  assert len(args)==11;u.mem_write(STACK,pack(args));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(0x16d660,STOP,count=200000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
  result=[u.reg_read(UC_X86_REG_EAX)&255,self.word(RNG),row[0]]
  for r in range(row[0]):
   choice,damage,props,_=bytes(u.mem_read(CHOICES+r*8,4));result.extend([bytes(u.mem_read(PERMS+r,1))[0],choice,damage,props,self.word(CHOICES+r*8+4)])
  result.extend([len(self.events)//4,*self.events]);return result

def rows():
 rng=random.Random(0x16d660);result=[]
 def build(n=1,permutations=1,damage=4,previous=0,variant=0,choice=0,props=0,bit=-1,mask=0,force=0,region=-1,effect=NONE,seed=123):
  row=[n,variant&NONE,region&NONE,damage&NONE,force,bit&NONE,mask,seed]
  for r in range(n):
   row.extend([0,choice&NONE,previous,props,effect,r,permutations])
   for p in range(permutations):
    row.extend([p,fw(rng.choice([0,.1,.5,1,2,16])),3])
    for name,properties in [(4,0),(4,1),(1,0)]:row.extend([(p+1)%128,properties,name,NONE,0])
  return row
 # Actual Turf's singleton transition: no transition RNG, no looping effect.
 result.append(build())
 for damage in [-1,0,1,4,5,127,128,255]:
  for previous in [0,1,4,127,128,255]:
   for force in [0,1]:result.append(build(damage=damage,previous=previous,force=force))
 for bit in [-1,0,1,7,8,31]:
  for mask in [0,1,0xffff]:
   for props in [0,1,2,128,255]:result.append(build(n=3,bit=bit,mask=mask,props=props))
 for n in [0,1,2,4,16]:
  for p in [0,1,2,4,32,40]:result.append(build(n=n,permutations=p,choice=-1))
 for _ in range(400):
  n=rng.randrange(1,5);p=rng.randrange(1,9)
  row=build(n=n,permutations=p,damage=rng.choice([-1,0,1,4,7]),previous=rng.choice([0,1,4,128]),
      variant=rng.choice([0,0,-1]),choice=rng.choice([-1,*range(p)]),props=rng.randrange(4),bit=rng.choice([-1,0,1,7]),
      mask=rng.randrange(1<<n),force=rng.randrange(2),region=rng.choice([-1,*range(n)]),effect=rng.choice([NONE,0x90020000]),seed=rng.getrandbits(32))
  # Include allocated effect branches and repeated matching alternatives.
  at=8
  for current,permutations in parse(row):
   at+=7
   for header,states in permutations:
    at+=3
    for state in states:
     if rng.randrange(3)==0:row[at+3]=0x80030001;row[at+4]=0x7000010
     at+=5
  result.append(row)
 # Exercise weighted choices exactly at the x87/SSE rounding boundary.
 inverse=pow(0x19660d,-1,1<<32);constant=struct.unpack('<f',struct.pack('<I',0x37800080))[0]
 f32=lambda value:struct.unpack('<f',struct.pack('<f',value))[0]
 boundaries=0
 for high in range(1,65536):
  second=f32(1/(high*constant)-1)
  total=f32(1+second)
  native=f32(f32(high*constant)*total);original=f32(high*constant*total)
  if (native>=1)!=(original>=1) or (native<=1)!=(original<=1):
   seed=(((high<<16)-0x3c6ef35f)*inverse)&NONE
   row=build(permutations=2,choice=-1,seed=seed);row[16]=fw(1);row[34]=fw(second)
   result.append(row);boundaries+=1
   if boundaries==128:break
 return result

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 assert sha(a.xbe)==EXPECTED;a.output.mkdir(parents=True,exist_ok=True);original=Original(Xbe(a.xbe));inputs=rows();expected=[original.run(r) for r in inputs]
 text=''.join(str(len(r))+' '+' '.join(f'{v&NONE:x}' for v in r)+'\n' for r in inputs)
 result=subprocess.run([str(a.native),'--oracle'],input=text,capture_output=True,text=True,timeout=45,check=True)
 actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
 mismatches=[{'case':i,'input':inputs[i],'expected':e,'actual':actual[i] if i<len(actual) else None} for i,e in enumerate(expected) if i>=len(actual) or e!=actual[i]]
 report={'passed':not mismatches and len(actual)==len(expected),'cases':len(inputs),'mismatches':mismatches,'scope':__doc__,'native_sha256':sha(a.native),'xbe_sha256':EXPECTED,'visited_instructions':len(original.visited),'fpcw':'027F','mxcsr':'1F80'}
 (a.output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'inputs.tsv').write_text(text);(a.output/'original.tsv').write_text('\n'.join(' '.join(f'{v:x}' for v in r) for r in expected)+'\n')
 print(json.dumps({k:v for k,v in report.items() if k not in ['scope','mismatches']}));print('mismatches',len(mismatches));return not report['passed']
if __name__=='__main__':raise SystemExit(main())
