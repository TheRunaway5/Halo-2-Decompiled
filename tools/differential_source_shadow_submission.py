#!/usr/bin/env python3
"""Untouched owner3D7F0/B9CE0/31F50 and pre-selection33400 collector.

4BAF0 fade bytes and3E9C0 local-player priority are explicit caller services.
No source owner/fade/hierarchy/collector math or admission output is replaced.
"""
from pathlib import Path
import argparse,collections,hashlib,json,random,struct,subprocess,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return words(struct.pack('<f',v))[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'build-mp-lightmap/source_shadow_submission_tests');a=p.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 BASE=0x2100000;u.mem_map(BASE,0x400000);STOP=0x3000000;u.mem_map(STOP,4096)
 TABLE,HEADERS,HANDLES,OBJECTS,DEFS,CLOCK,COLLECTOR,FLAGS,IDS,OUT,TEMP=[BASE+i*0x10000 for i in range(11)]
 STACK=BASE+0x3fff00;calls=collections.Counter();active={};visited=[];candidate_count=0;capture=False
 def put(at,*v):u.mem_write(at,packed(v))
 def read(at):return words(u.mem_read(at,4))[0]
 def ret(cleanup,value):
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,value);u.reg_write(UC_X86_REG_ESP,sp+4+cleanup);u.reg_write(UC_X86_REG_EIP,read(sp))
 def hook(g,pc,n,_):
  nonlocal candidate_count
  if pc in [0x3d7f0,0xbaf80,0xb9ce0,0x31f50,0x33400,0x4baf0,0x3e9c0,0x4a7c0]:calls[f'{pc:X}']+=1
  if pc==0x4baf0:
   o=active[g.reg_read(UC_X86_REG_EBX)];sp=g.reg_read(UC_X86_REG_ESP);u.mem_write(read(sp+4),b'\0');u.mem_write(read(sp+8),bytes([o[18]]));ret(8,0)
  elif pc==0x3e9c0:ret(0,active[g.reg_read(UC_X86_REG_ESI)][19])
  elif pc==0x31f99:
   visited.extend(words(u.mem_read(g.reg_read(UC_X86_REG_EDI)+0x30,16)))
  elif pc==0x4a7c0 and capture:candidate_count=read(g.reg_read(UC_X86_REG_ESP)+8);g.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook)
 def run(pc,regs,stack):
  for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
  for i in range(8):u.reg_write(UC_X86_REG_XMM0+i,0)
  for r,v in regs.items():u.reg_write(r,v)
  u.mem_write(STACK,packed(stack));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x27f);u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
  u.emu_start(pc,STOP,count=500000);assert u.reg_read(UC_X86_REG_EIP)==(0x4a7c0 if capture else STOP)
 def original(row):
  nonlocal active,visited,capture
  mode,n,q,tick,dt=row[:5];plane=row[5:9];scale=row[9];at=10;active={};owners=[]
  for i in range(n):o=row[at:at+20];at+=20;owners.append(o);active[o[0]]=o
  queue=row[at:at+q*2];at+=q*2;scratch=row[at:at+32];assert at+32==len(row)
  for addr in [TABLE,HEADERS,HANDLES,OBJECTS,DEFS,OUT]:u.mem_write(addr,bytes(0x10000))
  put(0x4e3b44,TABLE);put(0x4e0300,HEADERS);put(HEADERS+0x44,HANDLES);put(0x510c54,CLOCK);put(CLOCK+4,dt,tick)
  for index,o in enumerate(owners):
   handle,parent,link_c,link_10,flags,definition_mode,typ,header_flags,kind,lod_flags,death=o[:11]
   slot=handle&65535;obj=OBJECTS+index*0x400;definition=DEFS+index*0x100;put(HANDLES+slot*12+8,obj);u.mem_write(HANDLES+slot*12+2,bytes([header_flags,kind]));put(obj,index,flags);put(obj+0xc,link_c,link_10,parent);put(obj+0x30,*o[11:15]);u.mem_write(obj+0xaa,bytes([typ]));u.mem_write(obj+0x10a,bytes([lod_flags]));put(obj+0xbc,death);put(obj+0x2b0,o[16]);put(TABLE+index*16+8,definition);u.mem_write(definition+0x18,struct.pack('<H',definition_mode))
  put(0x4b9da0,*map(bits,[0,0,0]));put(0x4b9dcc,bits(1));put(0x4b9e20,plane[0]);put(0x4b9e2c,plane[1]);put(0x4b9e38,plane[2]);put(0x4b9e44,plane[3]);put(0x4b9ed0,scale);put(0x4b9ed4,0)
  out=[];capture=False
  for o in owners:
   run(0x3d7f0,{},[STOP,o[0],TEMP,TEMP+4]);out.extend([u.reg_read(UC_X86_REG_EAX)&255,read(TEMP),u.mem_read(TEMP+4,1)[0]])
   run(0xb9ce0,{UC_X86_REG_ECX:o[0]},[STOP]);out.append(u.reg_read(UC_X86_REG_EAX)&255)
  visited=[];put(TEMP,*map(bits,[0,0,0,0,0,1,0.01,0.01]));run(0x31f50,{},[STOP,owners[0][0],TEMP,TEMP+12,TEMP+24,TEMP+28]);out.extend([len(visited)//4,*visited])
  u.mem_write(COLLECTOR+4,struct.pack('<h',q));put(COLLECTOR+8,FLAGS,IDS)
  for i in range(q):put(IDS+i*4,queue[i*2]);u.mem_write(FLAGS+i*2,struct.pack('<H',queue[i*2+1]))
  for i,value in enumerate(scratch):u.mem_write(OUT+i*20+3,bytes([value]))
  capture=True;run(0x33400,{UC_X86_REG_EBX:COLLECTOR},[STOP,OUT,mode]);capture=False
  out.extend([candidate_count,*words(u.mem_read(OUT,candidate_count*20))]);return out
 rng=random.Random(0x33400);rows=[];labels=[]
 for case in range(360):
  n=1+case%40;handles=[0xe0000000+i for i in range(n)];parents=[-1]+[rng.randrange(i)for i in range(1,n)];objects=[]
  for i in range(n):
   typ=rng.choice([0,1,2,6,7,11,12]);kind=typ;children=[j for j,p in enumerate(parents)if p==i];siblings=[j for j,p in enumerate(parents)if p==parents[i]];k=siblings.index(i)
   flags=rng.choice([0,0,0,0x10000,1]);mode=rng.choice([0,0,2,2,1]);camo=rng.choice([-.25,0,.25,.75,1,1.25])
   objects.extend([handles[i],handles[parents[i]]if parents[i]>=0 else 0xffffffff,handles[children[0]]if children else 0xffffffff,handles[siblings[k+1]]if k+1<len(siblings)else 0xffffffff,flags,mode,typ,rng.choice([0,0,16]),kind,rng.choice([0,0,4]),100,*map(bits,[rng.uniform(-10,10),rng.uniform(-10,10),rng.uniform(-10,10),rng.uniform(.05,3)]),1,bits(camo),1,rng.choice([0,24,127,254,255]),rng.randrange(2)])
  q=list(range(n));rng.shuffle(q);queue=[v for i in q for v in [handles[i],rng.choice([0,0x4000,0x8000,0xc000,0xffff])]]
  row=[case%4,n,n,100+rng.choice([0,30,60,75,90,120]),bits(1/30),*map(bits,[.25,-.125,.875,-.5]),bits(240),*objects,*queue,*[rng.choice([0,1,2,3,255])for i in range(32)]]
  rows.append(row);labels.append(f'explicit-owner-collector{case}')
 expected=[original(row)for row in rows];text=''.join(' '.join(f'{v:x}'for v in row)+'\n'for row in rows)
 native=[list(map(lambda x:int(x,16),line.split()))for line in subprocess.check_output([str(a.binary.resolve()),'--probe'],input=text,text=True).splitlines()]
 failures=[]
 for i,(a,b)in enumerate(zip(native,expected)):
  if a!=b:
   j=next(j for j,(x,y)in enumerate(zip(a,b))if x!=y);failures.append(dict(case=i,label=labels[i],word=j,native=a[j],original=b[j]))
 report=dict(executable_sha256=PIN,cases=len(rows),words=sum(map(len,expected)),source_calls=dict(calls),failures=failures,explicit_services=['4BAF0: supplied actual per-owner/per-view fade byte','3E9C0: supplied actual local-player priority byte'],scope='Complete unchanged3D7F0 includingBAF80 root recursion/header camouflage; unchangedB9CE0; full31F50 observed sphere traversal; unchanged33400 stops at4A7C0, preserving explicit output scratch byte3. Declared current object/header/clock/ordered collector/priority/fade inputs; source successful registration/PVS/cache selection/3DC90 opacity and receiverOBB/secondary queues not produced.')
 (ROOT/'analysis/multiplayer-shading/shadow-submission-original.json').write_text(json.dumps(report,indent=2)+'\n');assert len(native)==len(expected) and not failures,failures[:5]
 fixture=ROOT/'tests/fixtures/retail_source_shadow_submission.tsv';fixture.write_text('# Original3D7F0/B9CE0/31F50/33400 with explicit4BAF0/3E9C0 services and retained scratch flag3.\n'+''.join(' '.join(f'{v:x}'for v in row)+' | '+' '.join(f'{v:x}'for v in out)+'\n'for row,out in zip(rows,expected)))
 fixture.with_suffix('.provenance.json').write_text(json.dumps(dict(report,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest()),indent=2)+'\n');print(json.dumps(report,indent=2))
if __name__=='__main__':main()
