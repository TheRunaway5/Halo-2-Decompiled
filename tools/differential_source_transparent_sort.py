#!/usr/bin/env python3
"""Untouched retail comparator/ordinal sorter/group builders and41040 pure admission/producer slices."""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
BASE,STACK,STOP=0x2100000,0x22fff00,0x3000000
COMMANDS,ORDER,SHADER,PART=BASE,BASE+0x30000,BASE+0x40000,BASE+0x41000
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return struct.unpack('<I',struct.pack('<f',v))[0]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--binary',type=Path,default=ROOT/'analysis/multiplayer-shading/source_transparent_sort_tests')
 p.add_argument('--count',type=int,default=1024);p.add_argument('--write-fixture',type=Path)
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-transparent-sort-original.json');a=p.parse_args()
 x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
 u.mem_map(BASE,0x200000);u.mem_map(STOP,0x1000)
 entries={k:0 for k in [0x1e700,0x13df80,0x13e140,0x420a0,0x42760,0x1e2d0,0x40e10,0x41120,0x41361]}
 def trace(_u,address,_size,_data):
  if address in entries:entries[address]+=1
  if address==0x41477:u.emu_stop()
 u.hook_add(UC_HOOK_CODE,trace)
 def run(at,args=(),end=STOP):
  u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(at,end,count=2000000)
  if u.reg_read(UC_X86_REG_EIP) not in [end,0x41477]:raise RuntimeError(f'original transparent{at:x} did not finish')
 def original(row):
  mode=row[0]
  if mode==0:
   n,first=row[1:3];u.mem_write(0x4b9da0,packed(row[3:6]));u.mem_write(COMMANDS,packed(row[6:]));result=[n-first]
   if n<=64:
    for i in range(n):
     for j in range(n):run(0x1e700,[i,j,COMMANDS]);result.append(u.reg_read(UC_X86_REG_EAX)&255)
   ids=list(range(first,n));u.mem_write(ORDER,struct.pack('<'+'H'*len(ids),*ids))
   u.reg_write(UC_X86_REG_ECX,ORDER);u.reg_write(UC_X86_REG_EAX,n-first);run(0x13df80,[0,0x1e700,COMMANDS])
   result.extend(struct.unpack('<'+'H'*len(ids),u.mem_read(ORDER,len(ids)*2)));return result
  if mode==1:
   n,existing,limit,argument=row[1:5];prefix=1024-limit
   u.mem_write(COMMANDS,bytes(1024*112));u.mem_write(COMMANDS+prefix*112,packed(row[5+n*9:]));u.mem_write(0x4c152c,bytes(32*36))
   u.mem_write(0x4c152c,packed(row[5:5+n*9]));u.mem_write(0x4c19ac,packed([n]));u.mem_write(0x4b6288,packed([COMMANDS]))
   u.mem_write(0x4b6280,packed([prefix+existing]));u.mem_write(0x4b6291,b'\1')
   run(0x420a0);run(0x42760,[argument]);count=words(u.mem_read(0x4b6280,4))[0]-prefix
   return [count,*words(u.mem_read(COMMANDS+prefix*112,count*112))]
  if mode!=2:raise RuntimeError('invalid original mode')
  mask,typ,present,view,grouped,flags,shader,token=row[1:9]
  u.mem_write(COMMANDS,bytes(112));u.mem_write(0x4b6288,packed([COMMANDS]));u.mem_write(0x4b6280,packed([0]))
  u.mem_write(PART,bytes(44));u.mem_write(PART,packed([shader if present else 0xffffffff]))
  u.mem_write(PART+4,packed(row[9:12]));u.mem_write(PART+0x10,packed(row[18:19]));u.mem_write(PART+0x14,packed(row[19:20]));u.mem_write(PART+0x18,packed(row[20:24]))
  u.mem_write(0x485618,packed(row[12:18]));u.mem_write(SHADER+0x16,struct.pack('<H',flags))
  # Actual41040 first part/material gate, starting after unrelated LOD math.
  u.mem_write(STACK,bytes(0x100));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EAX,typ);u.reg_write(UC_X86_REG_ECX,PART)
  u.reg_write(UC_X86_REG_EFLAGS,0x46 if typ==0 else 2) #Actual410FF TEST eax,eax.
  run(0x41120,end=0x4113a)
  if u.reg_read(UC_X86_REG_EIP)==0x41477:return [0]
  # Actual final layer15/view/group gate and producer, including real allocator
  #1E2D0 and40E10 priority routine. LOD choice/override checks are explicit inputs.
  u.mem_write(STACK,bytes(0x100));u.mem_write(STACK+0x14,packed([mask]));u.mem_write(STACK+0x38,packed([PART]));u.mem_write(STACK+0x40,packed([grouped]))
  u.reg_write(UC_X86_REG_ESI,view);u.reg_write(UC_X86_REG_EBX,SHADER);u.reg_write(UC_X86_REG_EDI,shader);u.reg_write(UC_X86_REG_EBP,token)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(0x41361,0x41477,count=10000)
  count=words(u.mem_read(0x4b6280,4))[0];return [count,*words(u.mem_read(COMMANDS,count*112))]
 rng=random.Random(0x1e700420a0);rows=[];labels=[]
 values=[0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc00000,0xffc01234,0x7fa12345]
 def command(priority=3,depth=0,secondary=0,plane=0,center=None):
  c=[0]*28;c[0]=priority&0xffffffff;c[1]=depth;c[2]=secondary&0xffffffff;c[3]=plane
  c[4:8]=[bits(v) for v in [1,0,0,0]];c[25:28]=center or [0]*3;return c
 def sort(cs,eye=(0,0,0),first=0,label='sort'):
  rows.append([0,len(cs),first,*eye,*sum(cs,[])]);labels.append(label)
 for n in [0,1,2,3,7,8,9,15,16,31,32,63,64,65,127,1024]:
  sort([command() for _ in range(n)],label='equal keys/source unstable order')
  sort([command(i%4,bits((i%7)-3),i%3) for i in range(n)],first=n//3,label='capacity/suffix/partition')
 for a_depth in values:
  for b_depth in values:
   sort([command(depth=a_depth),command(depth=b_depth,secondary=1)],label='NaN/infinity/signedzero depth')
 for flag in [0,1,0x100,0x101,0xff,0xffffff00]:
  for eye in values:
   for center in values:
    cs=[command(plane=flag),command(center=[center,0,0])]
    sort(cs,(eye,0,0),label='camera/center plane sides with raw lowbyte flag')
 for i in range(a.count):
  cs=[]
  for j in range(rng.randrange(1,33)):
   c=command(rng.choice([0,1,2,3,0x7fffffff,0x80000000]),bits(rng.uniform(-100,100)),rng.getrandbits(32),rng.choice([0,1]),[bits(rng.uniform(-100,100)) for _ in range(3)])
   c[4:8]=[bits(rng.uniform(-2,2)) for _ in range(4)];cs.append(c)
  sort(cs,[bits(rng.uniform(-100,100)) for _ in range(3)],first=rng.randrange(len(cs)+1),label='random source comparator/ordinal sort')
 def group(ss,limit=1024,initial=None):
  initial=initial or [];rows.append([1,len(ss),len(initial),limit,0xabcdef12,*sum(ss,[]),*sum(initial,[])]);labels.append('screen groups/source capacity/kinds')
 def section(kind,rect,depth=0,center=(0,0,0)):
  return [bits(depth),*map(bits,center),*map(bits,rect),kind]
 group([]);group([section(1,(0,1,0,1)),section(3,(1,2,0,1))]);group([section(1,(0,1,0,1)),section(3,(.5,2,0,1))])
 for limit in [0,1,2,1023,1024]:group([section(1,(0,1,0,1)),section(3,(1,2,0,1))],limit)
 for value in values:
  ss=[section(1,(0,1,0,1)),section(0,(1,2,1,2))];ss[1][1]=value;ss[1][0]=value;group(ss)
 for i in range(a.count):
  ss=[]
  for j in range(rng.randrange(32)):
   rect=[rng.uniform(-2,2) for _ in range(4)]
   if j%2==0:rect[1]=rect[0]+rng.uniform(0,2);rect[3]=rect[2]+rng.uniform(0,2)
   ss.append(section(rng.choice([0,1,3,7]),rect,rng.uniform(-10,10),[rng.uniform(-10,10) for _ in range(3)]))
  limit=rng.choice([0,1,2,4,1024]);initial=[command()] if limit and i%5==0 else [];group(ss,limit,initial)
 def part(mask=0x8000,typ=1,present=1,view=0,grouped=0,flags=0,shader=0x12340001,token=0x563412,values=None):
  v=values or [*map(bits,[1,2,3,4,5,6,.25,-.5,1,.75]),1,*map(bits,[1,0,0,0])]
  rows.append([2,mask,typ,present,view,grouped,flags,shader,token,*v]);labels.append('41040 pure gate/final producer actual inputs')
 for mask in [0,0x8000,0xffffffff]:
  for typ in [0,1,4,5,6]:
   for view in [0,1,2,3]:
    for grouped in [0,1]:
     for flags in range(4):part(mask,typ,view=view,grouped=grouped,flags=flags)
 part(present=0);part(shader=0xffffffff)
 for i in range(a.count):
  v=[bits(rng.uniform(-1e4,1e4)) for _ in range(10)]+[rng.randrange(2)]+[bits(rng.uniform(-2,2)) for _ in range(4)]
  if i< len(values):v[i%10]=values[i]
  part(flags=rng.getrandbits(16),shader=rng.getrandbits(32),token=rng.getrandbits(32),values=v)
 expected=[original(row) for row in rows]
 encoded=[' '.join(f'{v&0xffffffff:x}' for v in [len(row),*row]) for row in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16) for v in line.split()] for line in result.stdout.splitlines()]
 if len(actual)!=len(expected):raise RuntimeError('transparent native case count differs')
 for i,(want,got) in enumerate(zip(expected,actual)):
  if want!=got:
   j=next((j for j,(a,b) in enumerate(zip(want,got)) if a!=b),min(len(want),len(got)))
   raise RuntimeError(f'transparent case{i} {labels[i]} field{j} source{want[j:j+1]} native{got[j:j+1]} input{rows[i]}')
 report={'xbe_sha256':PIN,'native_sha256':hashlib.sha256(a.binary.read_bytes()).hexdigest(),'cases':len(rows),'compared_words':sum(map(len,expected)),
  'source_entries':{f'{k:08X}':v for k,v in entries.items()},'all_bit_exact':True,'fpcw':'027F','mxcsr':'1F80',
  'scope':'Full1E700/13DF80/13E140/420A0/42760/1E2D0/40E10;41040 first part/material gate41120..4113A plus final ordinary layer15/view/group command producer41361..41477. No callees stubbed. Actual LOD/override/visibility/node-center/plane are explicit inputs; no native renderer integration or framebuffer claim.'}
 a.output.write_text(json.dumps(report,indent=2)+'\n')
 if a.write_fixture:
  chosen=sorted(set([*range(32),*range(32,len(rows),max(1,(len(rows)-32)//512))]))
  a.write_fixture.write_text('# Untouched source transparent comparator/sort/groups/pure part producer;FPCW027F MXCSR1F80.\n'+'\n'.join(encoded[i]+' '+' '.join(f'{v:x}' for v in expected[i]) for i in chosen)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()
