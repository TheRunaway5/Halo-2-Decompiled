#!/usr/bin/env python3
"""Compare the automatic-door owner with unchanged Xbox and Vista instructions.

Candidate enumeration, named-marker lookup and group requests are explicit
caller boundaries. This executes all door eligibility/side/timer decisions;
it does not emulate BB050 registration, Havok, portal submission or effects.
"""
from pathlib import Path
import argparse,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
XBOX_SHA='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
VISTA_SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BANK=0x2100000;STACK=BANK+0xff000
OBJECT,DEFINITION,HEADER,POOL,TAGS,GROUP_HEADER,GROUPS,CLOCK,CONTEXT,UNITS,UNIT_TAGS=[BANK+i*0x2000 for i in range(11)]
def fw(v):return struct.unpack('<I',struct.pack('<f',v))[0]
def ff(v):return struct.unpack('<f',struct.pack('<I',v))[0]
def pack(words):return struct.pack('<'+'I'*len(words),*[x&0xffffffff for x in words])
def sha(path):
 with path.open('rb') as source:return hashlib.file_digest(source,'sha256').hexdigest()
class PE:
 def __init__(self,path):
  self.f=path.open('rb');self.f.seek(60);off=struct.unpack('<I',self.f.read(4))[0];self.f.seek(off);h=self.f.read(24)
  assert h[:4]==b'PE\0\0';_,count,_,_,_,size,_=struct.unpack('<HHIIIHH',h[4:]);o=self.f.read(size);self.base=struct.unpack_from('<I',o,28)[0];self.sections=[]
  for _ in range(count):
   s=self.f.read(40);_,rva,n,raw=struct.unpack_from('<IIII',s,8);self.sections.append((rva,n,raw))
 def read(self,va,n):
  for rva,size,raw in self.sections:
   offset=va-self.base-rva
   if 0<=offset and offset+n<=size:self.f.seek(raw+offset);data=self.f.read(n);assert len(data)==n;return data
  raise ValueError(hex(va))
class Original:
 def __init__(self,image,vista=False):
  self.image=image;self.vista=vista;self.u=Uc(UC_ARCH_X86,UC_MODE_32);u=self.u
  if vista:
   u.mem_map(0x400000,0xb00000);self.start,self.stop=0x58db44,0x58df44
   self.query,self.marker,self.forward,self.group=0x5331ba,0x53277c,0x532ac5,0x564fe6
   self.object_global,self.tag_global,self.clock_global,self.context_global=0x8e461c,0x87cd50,0x8c06e4,0x882d3c
   self.allowed=[(self.start,self.stop),(0x47bfe9,0x47bff2),(0x47c0a9,0x47c0b6),(0x4498b7,0x4498c8)]
   for a,b in self.allowed:u.mem_write(a,image.read(a,b-a))
   for a in [0x79be18,0x79de6c,0x7c3df8]:u.mem_write(a,image.read(a,4))
   self.put(0x87cd54,0)
  else:
   u.mem_map(0x10000,0x600000);self.start,self.stop=0x115566,0x11599b
   self.query,self.marker,self.forward,self.group=0xbb050,0xb8d30,0xb9fc0,0x1071e0
   self.object_global,self.tag_global,self.clock_global,self.context_global=0x4e0300,0x4e3b44,0x510c54,0x4e6948
   self.allowed=[(self.start,self.stop)];u.mem_write(self.start,image.read(self.start,self.stop-self.start))
   for a in [0x45dbc0,0x45dbd8,0x45dbdc]:u.mem_write(a,image.read(a,4))
  u.mem_map(BANK,0x100000);self.put(self.object_global,HEADER);self.put(HEADER+0x44,POOL);self.put(self.tag_global,TAGS)
  self.put(self.clock_global,CLOCK);self.put(self.context_global,CONTEXT)
  self.put(0x8e6680 if vista else 0x4e0328,GROUP_HEADER);self.put(GROUP_HEADER+0x44,GROUPS)
  self.visited=set();u.hook_add(UC_HOOK_CODE,self.hook)
 def put(self,a,v):self.u.mem_write(a,pack([v]))
 def word(self,a):return struct.unpack('<I',self.u.mem_read(a,4))[0]
 def ret(self,value=0,pop=0):
  u=self.u;sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,value&0xffffffff);u.reg_write(UC_X86_REG_EIP,self.word(sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,u,pc,size,_):
  sp=u.reg_read(UC_X86_REG_ESP);w=self.row
  if pc==self.stop:u.emu_stop();return
  if pc==self.query:
   args=[self.word(sp+4+i*4) for i in range(7)]
   assert args[0:2]==[1,3] and args[2:4]==[OBJECT+0x28,OBJECT+0x30] and args[6]==16
   self.queried=1;self.radius=args[4];handles=[0x80010001+i for i in range((len(w)-24)//10)]
   if handles:u.mem_write(args[5],pack(handles))
   self.ret(len(handles),0 if self.vista else 28);return
  if pc==self.marker:
   if self.vista:
    assert self.word(sp+4)==w[4] and self.word(sp+8)==0x7000684 and self.word(sp+16)==1
    out=self.word(sp+12);pop=0
   else:
    assert self.word(sp+4)==w[4] and self.word(sp+8)==0x7000686 and self.word(sp+12)==1
    out=u.reg_read(UC_X86_REG_EBX);pop=12
   if w[16]:u.mem_write(out+0x60,pack(w[18:21]));u.mem_write(out+0x3c,pack(w[21:24]))
   self.ret(w[16],pop);return
  if pc==self.forward:
   if self.vista:
    assert self.word(sp+4)==w[4];out=self.word(sp+8);pop=0
   else:
    assert u.reg_read(UC_X86_REG_EAX)==w[4];out=u.reg_read(UC_X86_REG_ESI);pop=0
   u.mem_write(out,pack(w[13:16]));self.ret(0,pop);return
  if pc==self.group:
   if self.vista:assert self.word(sp+4)==w[4];group,value=self.word(sp+8),self.word(sp+12);pop=0
   else:group,value=self.word(sp+4),self.word(sp+8);pop=8
   assert group==0x10000;self.targets.append(value);self.ret(1,pop);return
  if not any(a<=pc<b for a,b in self.allowed):raise RuntimeError(f'unexpected original door instruction {pc:08x}')
  if bytes(u.mem_read(pc,size))!=self.image.read(pc,size):raise RuntimeError('original door instruction changed')
  self.visited.add(pc)
 def run(self,row):
  self.row=row;self.targets=[];self.queried=0;self.radius=0;u=self.u;u.mem_write(OBJECT,bytes(0x300));u.mem_write(DEFINITION,bytes(0x140));u.mem_write(STACK,bytes(0x200))
  self.put(DEFINITION+0x118,row[0]);self.put(DEFINITION+0x120,row[1]);self.put(OBJECT+0x1cc,row[2]);self.put(OBJECT+0x1d0,row[3]);self.put(OBJECT+0x140,row[8]);self.put(OBJECT+0x3c,row[9]);u.mem_write(OBJECT+0x30,pack(row[10:13]));self.put(OBJECT+0x13c,0x10000 if row[7] else 0xffffffff)
  self.put(CLOCK+4,row[17]);self.put(CLOCK+8,row[5]);u.mem_write(CONTEXT+0xc,bytes([4 if row[6] else 2]));self.put(GROUPS+4,fw(0))
  for n,i in enumerate(range(24,len(row),10)):
   h=0x80010001+n;slot=h&0xffff;body=UNITS+n*0x400;definition=UNIT_TAGS+n*0x300;u.mem_write(body,bytes(0x300));u.mem_write(definition,bytes(0x200));self.put(POOL+slot*12+8,body);u.mem_write(POOL+slot*12+3,bytes([row[i+4]]));self.put(body,0x80020001+n);self.put(TAGS+slot*16+8,definition);self.put(definition+0xbc,row[i+2]);self.put(definition+0x1ec,row[i+3]);u.mem_write(body+0x10a,bytes([row[i+5]]));u.mem_write(body+0x30,pack(row[i+6:i+9]));self.put(body+(0x140 if self.vista else 0x13c),row[i+1])
  self.put(STACK+(0xd4 if self.vista else 0xdc),row[4]);self.put(STACK+0x18,DEFINITION)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_ESI,OBJECT if self.vista else DEFINITION);u.reg_write(UC_X86_REG_EBP,DEFINITION if self.vista else OBJECT);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(self.start,self.stop,count=10000)
  if u.reg_read(UC_X86_REG_EIP)!=self.stop:raise RuntimeError('door branch did not finish')
  return [self.word(OBJECT+0x1d0),self.queried,self.radius,len(self.targets),*self.targets]
def rows():
 rng=random.Random(0x1152e0);result=[]
 def row(flags=0,tick=0,position=0,timer=0,units=(),seconds=1.5,editor=0,group=1,marker=0):
  values=[fw(2.6),fw(seconds),flags,timer&0xffffffff,0x80010000,tick,editor,group,fw(position),fw(3),fw(0),fw(0),fw(0),fw(1),fw(0),fw(0),marker,fw(1/30),fw(-2),fw(0),fw(0),fw(1),fw(0),fw(0)]
  for n,q in enumerate(units):
   kind,definition,vehicle,runtime,player,x,y,z=q;values.extend([0x80010001+n,player,definition,vehicle,kind,runtime,fw(x),fw(y),fw(z),fw(.5)])
  return values
 ordinary=(0,0,0,0,0xffffffff,-1,0,0)
 for flags in range(64):
  for tick in range(4):
   for position in [0,.5,1]:result.append(row(flags,tick,position,44,[ordinary]))
 for timer in [-3,-2,-1,0,43,44,45,46,0x7fffffff,-0x80000000]:
  for seconds in [1.5,ff(fw(1.5)-1),ff(fw(1.5)+1),0,-.1]:result.append(row(0,1,1,timer,seconds=seconds))
 for timer in range(1,61):
  result.append(row(0,1,1,timer-1,seconds=ff(fw(timer*ff(fw(1/30))))))
 for _ in range(400):
  units=[]
  for i in range(rng.randrange(17)):
   units.append((rng.randrange(2),rng.choice([0,0x4000,0x4400]),rng.choice([0,0x100000]),rng.choice([0,4,8]),rng.choice([0xffffffff,0x80010020]),rng.choice([-3,-1,0,1,3]),rng.uniform(-1,1),rng.uniform(-1,1)))
  result.append(row(rng.randrange(64),rng.randrange(20),rng.choice([0,1,.75]),rng.randrange(-3,49),units,editor=rng.randrange(2),group=rng.randrange(2),marker=rng.randrange(2)))
 return result
def main():
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native',type=Path,required=True);parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--vista',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 assert sha(a.xbe)==XBOX_SHA and sha(a.vista)==VISTA_SHA
 x=Xbe(a.xbe);xbox=Original(x);vista=Original(PE(a.vista),True);inputs=rows();outputs=[];differences=[]
 for i,row in enumerate(inputs):
  expected=xbox.run(row);other=vista.run(row);outputs.append(expected)
  if expected!=other:differences.append({'case':i,'xbox':expected,'vista':other,'input':row})
 text=''.join(str(len(row))+' '+' '.join(f'{v:x}' for v in row)+'\n' for row in inputs)
 native=subprocess.run([str(a.native.resolve()),'--oracle'],input=text,capture_output=True,text=True,check=True,timeout=30)
 actual=[[int(v,16) for v in line.split()] for line in native.stdout.splitlines()]
 mismatches=[{'case':i,'expected':expected,'actual':actual[i] if i<len(actual) else None} for i,expected in enumerate(outputs) if i>=len(actual) or actual[i]!=expected]
 report={'passed':not mismatches and len(actual)==len(outputs),'scope':__doc__,'cases':len(inputs),'xbox_instructions_visited':len(xbox.visited),'vista_instructions_visited':len(vista.visited),'native_sha256':sha(a.native),'xbox_sha256':XBOX_SHA,'vista_sha256':VISTA_SHA,'xbox_native_mismatches':mismatches,'vista_different_cases':len(differences),'vista_differences':differences,'fpcw':'027F','mxcsr':'1F80'}
 (a.output/'comparison.json').write_text(json.dumps(report,indent=2)+'\n');(a.output/'inputs.tsv').write_text(text);(a.output/'xbox-outputs.tsv').write_text('\n'.join(' '.join(f'{v:x}' for v in row) for row in outputs)+'\n')
 print(json.dumps({k:v for k,v in report.items() if k!='vista_differences'},indent=2));return 0 if report['passed'] else 1
if __name__=='__main__':raise SystemExit(main())
