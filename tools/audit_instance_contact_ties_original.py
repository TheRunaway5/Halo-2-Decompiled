from pathlib import Path
import sys,struct,json,hashlib,math
sys.path.insert(0,'tools');from inspect_multiplayer import Cache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32
from unicorn.x86_const import *
image=Path('assets/disc/default.xbe').read_bytes();sha=hashlib.sha256(image).hexdigest();assert sha=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x600000);base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
for i in range(count):
 _,va,_,off,size=struct.unpack_from('<5I',image,table-base+i*56);g.mem_write(va,image[off:off+size])
g.mem_map(0x2000000,0x20000);g.mem_map(0x3000000,0x1000)
# Explicit cold source breakable bank. Original183FC0 executes unchanged.
g.mem_write(0x4ed280,struct.pack('<I',0x2010000));g.mem_write(0x4686c4,b'\0\0');g.mem_write(0x2010000,bytes([255])*0x1000)
floatbits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
fbits=lambda u:struct.unpack('<f',struct.pack('<I',u))[0]
rows=[];pc=None
result,point,rayptr,callback,shape=0x2000100,0x2001100,0x2001200,0x2002000,0x2002100
for line in Path('analysis/multiplayer-shading/instance-tie-census.tsv').read_text().splitlines():
 f=line.split();name=f[1]
 if pc!=name:
  if pc:g.mem_unmap(memory,alloc);c.data.close();c.stream.close()
  path=Path('assets/disc/maps')/(name+'.map')
  if not path.exists():path=Path('assets/map-pack/maps')/(name+'.map')
  c=Cache(path);entry=c.block(c.scenario+0x210,0x44)[0];start,size,memory=c.u(entry),c.u(entry+4),c.u(entry+8);alloc=(size+4095)//4096*4096
  g.mem_map(memory,alloc);g.mem_write(memory,bytes(c.data[start:start+size]));g.mem_write(0x4e0348,struct.pack('<I',memory+16));pc=name
 origin=list(map(float,f[5:8]));ray=list(map(float,f[8:11]));fraction_bits=int(f[11]);owners=[int(h.split(':')[0]) for h in f[14:]]
 g.mem_write(point,struct.pack('<3f',*origin));g.mem_write(rayptr,struct.pack('<3f',*ray))
 def run(i,maximum):
  raw=bytearray(96);struct.pack_into('<f',raw,4,maximum);g.mem_write(result,bytes(raw));g.reg_write(UC_X86_REG_EAX,i);g.reg_write(UC_X86_REG_EBX,result)
  stack=0x200f000;g.mem_write(stack,struct.pack('<5I',0x3000000,0x04800c0d,9,point,rayptr));g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(0x1691a0,0x3000000,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000
  raw=bytes(g.mem_read(result,96));return {'hit':g.reg_read(UC_X86_REG_EAX)&255,'fraction_bits':struct.unpack_from('<I',raw,4)[0],'instance':struct.unpack_from('<I',raw,0x3c)[0],'surface':struct.unpack_from('<I',raw,0x50)[0],'leaf':struct.unpack_from('<I',raw,0x4c)[0]}
 def callorder(order):
  g.mem_write(result,bytes(96));g.mem_write(result+4,struct.pack('<f',1));g.mem_write(0x4e7c1c,struct.pack('<I',1));g.mem_write(0x4e7c20,bytes(512*4));g.mem_write(callback,struct.pack('<8I',0x459e48,0x3f800000,0x04800c0d,9,point,rayptr,0,result));out=[]
  for i in order:
   g.mem_write(shape,struct.pack('<2I',0,i));g.reg_write(UC_X86_REG_ECX,callback);stack=0x200f000;g.mem_write(stack,struct.pack('<3I',0x3000000,shape,0));g.reg_write(UC_X86_REG_ESP,stack);g.emu_start(0x244c10,0x3000000,count=1000000);assert g.reg_read(UC_X86_REG_EIP)==0x3000000
   raw=bytes(g.mem_read(result,96));out.append({'input_instance':i,'fraction_bits':struct.unpack_from('<I',raw,4)[0],'owner_instance':struct.unpack_from('<I',raw,0x3c)[0],'collector_fraction_bits':struct.unpack_from('<I',g.mem_read(callback+4,4))[0],'surface':struct.unpack_from('<I',raw,0x50)[0]})
  return out
 rows.append({'map':name,'kind':f[2],'input_source':int(f[3]),'face':int(f[4]),'origin':origin,'ray':ray,'expected_fraction_bits':fraction_bits,'native_unresolved':bool(int(f[12])),'owners':owners,'single':{str(i):{'full':run(i,1),'exact':run(i,fbits(fraction_bits)),'next':run(i,fbits(fraction_bits+1)),'next2':run(i,fbits(fraction_bits+2))} for i in owners},'orders':[callorder(owners),callorder(owners[::-1])]})
report={'xbe_sha256':sha,'source':'unchanged1691A0/1689B0/11E5E0/141590/183FC0/1DE630 and244C10; explicit cold all-one breakable bank, no instruction hooks','accelerator_order':'explicit caller input, not claimed original3009E0 order','cases':len(rows),'maps':sorted(set(r['map'] for r in rows)),'false_admissions':[r for r in rows if not r['native_unresolved']],'rows':rows}
Path('analysis/multiplayer-shading/instance-tie-original.json').write_text(json.dumps(report,indent=2)+'\n');print('cases',len(rows),'false admissions',len(report['false_admissions']))
for r in report['false_admissions']:print(json.dumps(r))
