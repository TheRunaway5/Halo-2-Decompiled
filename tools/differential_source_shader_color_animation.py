#!/usr/bin/env python3
"""Execute original object externs then full animated parameter dispatch.

Uses literal independent synthetic authored banks, not a substituted extern
result or an implementation-mirroring reference. No original callee is patched.
"""
import argparse,hashlib,json,random,struct,subprocess,sys
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
STATE,STACK,STOP=0x2100000,0x22fef00,0x3000000
DEFINITION,PP,IMPL,EXTERNS,FRAGMENTS,LOD,LAYERS,PASSES,ANIMIMPL,MAPS,PARAMS,REFS,OVERLAYS,CURVES,RANGES,RAW=[STATE+i for i in range(0x10000,0x110000,0x10000)]
def packed(values):return struct.pack('<'+'I'*len(values),*(v&0xffffffff for v in values))
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def mapping(destination,component,source):return destination|component<<16|source<<24
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--binary',type=Path,required=True)
 p.add_argument('--count',type=int,default=2048)
 p.add_argument('--write-fixture',type=Path)
 p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/source-shader-color-animation-original.json')
 a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe')
 assert hashlib.sha256(x.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
 for section in x.sections:u.mem_write(section.va,x.section_bytes(section))
 u.mem_map(STATE,0x200000);u.mem_map(STOP,0x1000)
 entries={v:0 for v in [0x131ed0,0x131bb0,0x19be0,0x19ca0,0x17420,0x176a0,0x17960,0x17b60,0x18560,0x1b580]}
 def trace(_u,address,_size,_data):
  if address in entries:entries[address]+=1
 u.hook_add(UC_HOOK_CODE,trace)
 def run(entry,args=(),registers=()):
  u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK)
  u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.reg_write(UC_X86_REG_FPTAG,0xffff);u.reg_write(UC_X86_REG_FPSW,0)
  for reg,value in registers:u.reg_write(reg,value)
  try:u.emu_start(entry,STOP,count=200000)
  except Exception as e:raise RuntimeError(f'original{entry:x} failed at{u.reg_read(UC_X86_REG_EIP):x}')from e
  if u.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError(f'original{entry:x} did not return')
 def original(row):
  index=0
  def take(count=1):
   nonlocal index
   out=row[index:index+count];index+=count;return out[0]if count==1 else out
  initial=take(60);constants=take(128);colors=take(12);seconds,fragmented,selected,offset=take(4)
  pixel_extern=[take()for _ in range(take())];vertex_extern=[take()for _ in range(take())]
  pixel=[take()for _ in range(take())];vertex=[take()for _ in range(take())]
  functions=[take(6)for _ in range(take())];assert index==len(row)
  u.mem_write(STATE,bytes(0x1800));u.mem_write(DEFINITION,bytes(0x100));u.mem_write(PP,bytes(0x100));u.mem_write(IMPL,bytes(0x140))
  u.mem_write(STATE+12,packed([DEFINITION]));u.mem_write(STATE+32,packed([PP,IMPL]))
  u.mem_write(STATE+0x1424,packed(initial));u.mem_write(STATE+0x1530,packed(constants))
  u.mem_write(STATE+0x300,packed([selected,0,0,0,0,0,0,0]));u.mem_write(STATE+0x1520,bytes([offset,0,0,0,0,0,0,0]))
  for i,source in enumerate([3,4,21,22]):
   u.mem_write(RAW,packed(colors[i*3:i*3+3]));run(0x131ed0,[RAW]);color=u.reg_read(UC_X86_REG_EAX)
   run(0x131bb0,registers=[(UC_X86_REG_ECX,color),(UC_X86_REG_EAX,STATE+0x108+source*12)])
  externs=pixel_extern+vertex_extern;u.mem_write(EXTERNS,packed(externs));u.mem_write(PP+0x3c,packed([EXTERNS]))
  u.mem_write(FRAGMENTS,struct.pack('<hH',0,8<<9));u.mem_write(PP+0x24,packed([FRAGMENTS]))
  u.mem_write(IMPL+0xf6,struct.pack('<H',int(bool(fragmented))<<9))
  u.mem_write(RANGES,struct.pack('<HH',len(pixel_extern)<<9,(len(vertex_extern)<<9)|len(pixel_extern)))
  u.mem_write(LOD,struct.pack('<IH',1,1<<9));u.mem_write(LAYERS,struct.pack('<H',1<<9));u.mem_write(PASSES,struct.pack('<H',1<<9))
  u.mem_write(ANIMIMPL,struct.pack('<5H',0,0,0,len(pixel)<<9,(len(vertex)<<9)|len(pixel)))
  u.mem_write(MAPS,packed(pixel+vertex))
  u.mem_write(PARAMS,b''.join(struct.pack('<H',(1<<9)|i)for i in range(len(functions))))
  u.mem_write(REFS,b''.join(struct.pack('<hh',i,0)for i in range(len(functions))))
  for i,function in enumerate(functions):
   period,*curve=function;u.mem_write(CURVES+i*0x100,packed(curve))
   u.mem_write(OVERLAYS+i*20,packed([0,0,period,20,CURVES+i*0x100]))
  for field,data in [(0x1c,[1,LOD]),(0x24,[1,LAYERS]),(0x2c,[1,PASSES]),(0x34,[1,ANIMIMPL]),
                     (0x3c,[len(functions),OVERLAYS]),(0x44,[len(functions),REFS]),(0x4c,[len(functions),PARAMS]),(0x54,[len(pixel+vertex),MAPS])]:
   u.mem_write(DEFINITION+field,packed(data))
  value=struct.unpack('<f',packed([seconds]))[0];u.mem_write(0x4858a0,struct.pack('<d',value));u.mem_write(0x485a58,packed([0]))
  run(0x19be0,[STATE,RANGES+2]);run(0x19ca0,[RANGES],[(UC_X86_REG_ESI,STATE)])
  run(0x17420,[STATE,0],[(UC_X86_REG_EAX,0),(UC_X86_REG_ECX,0),(UC_X86_REG_EBX,0)])
  return words(u.mem_read(STATE+0x1424,240))+words(u.mem_read(STATE+0x1530,512))
 rng=random.Random(0x19ca017420);rows=[];labels=[]
 def add(pixel_extern,vertex_extern,pixel,vertex,functions,seconds,fragmented=False,selected=0,offset=0,label='explicit'):
  program=[rng.getrandbits(32)for _ in range(60)]
  constants=[bits(rng.uniform(-2,2))for _ in range(128)]
  colors=[bits(rng.uniform(0,1))for _ in range(12)]
  row=[*program,*constants,*colors,bits(seconds),int(fragmented),selected,offset]
  for bank in [pixel_extern,vertex_extern,pixel,vertex]:row.extend([len(bank),*bank])
  row.append(len(functions))
  for function in functions:row.extend(function)
  rows.append(row);labels.append(label)
 def scalar(period=1,lo=0,hi=1):return [bits(period),0,bits(lo),bits(hi),0,0]
 def color(value):return [bits(1),0x1001,value,0,0,0]
 vertex_object=mapping(0x1c8a,4,3)
 for fragmented in [False,True]:
  for slot in [0,1,7,8]:
   for bank in [0,1]:
    for component in range(5):
     for matched in [False,True]:
      if fragmented and slot==7:offset=0
      else:offset=int(fragmented)
      destination=slot<<6|bank<<10|int(matched)<<3
      extern=mapping(destination,4,3)
      animate=mapping(destination,component,0)
      functions=[color(0xff196fd9)]if component==4 else[scalar(lo=.13,hi=.89)]
      add([extern],[vertex_object],[animate],[],functions,.37,fragmented,int(matched),offset,'overlap component/permutation/fragment')
      add([extern],[vertex_object],[mapping((slot if slot==8 else(slot+1)%7)<<6|bank<<10|int(matched)<<3,component,0)],[],functions,.63,fragmented,int(not matched),offset,'mixed/disjoint component/permutation')
 for mode in range(5):
  for seconds in [0,.125,.499,1.25,3.875]:
   add([mapping(0x200,4,21)],[vertex_object],[],[mapping((mode<<5)|10,0,0)],
       [color(0xff52a671)]if mode==4 else[scalar(lo=.1,hi=.8)],seconds,label='vertex extern overwritten partially/fully; pixel preserved')
 # Original131E50 multiplies the float input on x87 and converts directly
 # to an integer. An extra host float spill before nearbyint changes these
 # byte-midpoint cases, independently of the extern/animation order.
 for component in range(4):
  for byte in range(255):
   midpoint=bits((byte+.5)/255)
   for value in [midpoint-1,midpoint,midpoint+1]:
    real=struct.unpack('<f',packed([value]))[0]
    add([mapping(0,4,3)],[vertex_object],[mapping(0,component,0)],[],
        [scalar(lo=real,hi=real)],.125,label='x87 packed-byte adjacent midpoint')
 boundary_count=len(rows)
 for i in range(a.count):
  fragmented=rng.randrange(2);selected=rng.randrange(8);offset=rng.randrange(3)if fragmented else 0
  extern=[];pixel=[];vertex=[];functions=[]
  for _ in range(rng.randrange(1,6)):
   slot=rng.choice([*range(8-offset),8]);destination=slot<<6|rng.randrange(2)<<10
   if fragmented:destination|=rng.randrange(8)<<3
   extern.append(mapping(destination,4,rng.choice([3,4,21,22])))
   component=rng.randrange(5);index=len(functions)
   functions.append(color(rng.getrandbits(32))if component==4 else scalar(period=rng.choice([.25,1,4]),lo=rng.uniform(0,.3),hi=rng.uniform(.6,1)))
   pixel.append(mapping(destination,component,index))
  mode=rng.randrange(5);index=len(functions)
  functions.append(color(rng.getrandbits(32))if mode==4 else scalar(period=2,lo=.1,hi=.9))
  vertex.append(mapping((mode<<5)|10,0,index))
  add(extern,[vertex_object],pixel,vertex,functions,rng.uniform(0,10),fragmented,selected,offset,'random masked extern/animation sequence')
 expected=[original(row)for row in rows]
 encoded=[' '.join(f'{v:x}'for v in [len(row),*row])for row in rows]
 result=subprocess.run([str(a.binary.resolve()),'--probe'],input='\n'.join(encoded)+'\n',text=True,capture_output=True,check=True)
 actual=[[int(v,16)for v in line.split()]for line in result.stdout.splitlines()]
 if len(actual)!=len(expected):raise RuntimeError('native shader binding case count differs')
 for i,(source,native)in enumerate(zip(expected,actual)):
  if source!=native:
   different=[j for j,(x,y)in enumerate(zip(source,native))if x!=y]
   raise RuntimeError(f'case{i} {labels[i]} differs words{different}: source{[hex(source[j])for j in different]} native{[hex(native[j])for j in different]} input{rows[i]}')
 report=dict(xbe_sha256=PIN,native_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),cases=len(rows),compared_words=sum(map(len,expected)),all_bit_exact=True,
  fpcw='027F',mxcsr='1F80',source_entries={f'{k:08X}':v for k,v in entries.items()},
  scope='Complete original131ED0/131BB0 raw object draw colors; complete19BE0 then19CA0, then full17420 including176A0/17960 and empty authored raster/texture ranges. No original callee/result patched. Synthetic authored identity/constant curves, two combiner banks/finalconstants, fragment/permutation filtering, mixed and overlapping pixel/vertex channels. Native actualapply_object_colors→animate_forward_material with stale previous-frame poisoning. Clock is explicit; no object lifetime/scene scheduler/framebuffer/hardware precision claim.')
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(report,indent=2)+'\n')
 if a.write_fixture:
  chosen=sorted(set([*range(min(345,len(rows))),*range(345,boundary_count,12),
                    *range(boundary_count,len(rows),max(1,(len(rows)-boundary_count)//256))]))
  a.write_fixture.write_text('# Complete original object externs19BE0/19CA0 -> animated17420/176A0/17960.\n'+'\n'.join(encoded[i]+' '+' '.join(f'{v:x}'for v in expected[i])for i in chosen)+'\n')
 print(json.dumps(report,indent=2))
if __name__=='__main__':main()
