#!/usr/bin/env python3
"""Freeze all authored MoppInfo banks and unchanged source visitation traces.

The original cluster caller, interpreter and contact callbacks run untouched.
Only read-only opcode/callback observers are installed. Rays across the literal
authored cluster bounds are explicit diagnostic inputs, not gameplay claims.
"""
from pathlib import Path
import hashlib,json,struct,sys,math
from collections import Counter
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent))
from inspect_multiplayer import Cache,MAPS
ROOT=Path(__file__).resolve().parents[1]
EXTRA=('backwash','containment','deltatap','dune','elongation','gemini','triplicate','turf','warlock')
NAMES=(*MAPS,*EXTRA)
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
image=(ROOT/'assets/disc/default.xbe').read_bytes();assert hashlib.sha256(image).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x601000)
base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
for i in range(count):
 _,va,_,off,size=struct.unpack_from('<5I',image,table-base+i*56);u.mem_write(va,image[off:off+size])
u.mem_map(0x2000000,0x20000);u.mem_map(0x3000000,0x1000)
def put(at,*words):u.mem_write(at,struct.pack('<'+'I'*len(words),*words))
def word(at):return struct.unpack('<I',u.mem_read(at,4))[0]
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
put(0x4ed280,0x2010000);u.mem_write(0x4686c4,b'\0\0');u.mem_write(0x2010000,b'\xff'*0x1000)
ORIGIN,RAY,RESULT,STACK,STOP=0x2000100,0x2000200,0x2000300,0x200f000,0x3000000
callbacks=[];instructions=[];contexts=[];current_info=0
def observe_callback(g,address,size,unused):
 context=g.reg_read(UC_X86_REG_ECX);shape=word(g.reg_read(UC_X86_REG_ESP)+4)
 callbacks.append([word(shape+4),word(context+4),None]);contexts.append(context)
def observe_return(g,address,size,unused):callbacks[-1][2]=word(contexts[-1]+4)
def observe_instruction(g,address,size,unused):
 pc=g.reg_read(UC_X86_REG_ESI);instructions.append([pc-current_info-0x30,bytes(g.mem_read(pc,1))[0]])
u.hook_add(UC_HOOK_CODE,observe_callback,begin=0x244c10,end=0x244c10)
u.hook_add(UC_HOOK_CODE,observe_return,begin=0x244c06,end=0x244c06)
u.hook_add(UC_HOOK_CODE,observe_instruction,begin=0x300a10,end=0x300a10)
bank_lines=[];query_lines=[];maps=[];ops=Counter();banks=[];logical_total=0;clusters_total=0
for ordinal,name in enumerate(NAMES):
 path=ROOT/('assets/disc/maps' if ordinal<len(MAPS) else 'assets/map-pack/maps')/(name+'.map');c=Cache(path)
 entry=c.block(c.scenario+0x210,0x44)[0];start,size,memory=c.u(entry),c.u(entry+4),c.u(entry+8);allocated=(size+4095)//4096*4096;magic=memory-start
 u.mem_map(memory,allocated);u.mem_write(memory,bytes(c.data[start:start+size]));put(0x4e0348,memory+16);put(0x4e0340,c.u(start+16+0x18))
 cluster_count=c.u(start+16+0x9c);clusters=c.u(start+16+0xa0)-magic;clusters_total+=cluster_count;map_banks=0
 for cluster in range(cluster_count):
  at=clusters+cluster*176;capacity=c.u(at+0xa8)
  if not capacity:continue
  off=c.u(at+0xac)-magic;current_info=off+magic;length=c.u(off+0x20);raw=bytes(c.data[off:off+0x30+capacity]);assert (length+15)&~15==capacity
  key=f'{name}_{cluster}';bank_lines.append(f'B {key} {capacity} {raw.hex()}');logical_total+=length;map_banks+=1
  bank={'map':name,'cluster':cluster,'file_offset':off,'padded_capacity':capacity,'length':length,'word24':c.u(off+0x24),'info_words':list(struct.unpack_from('<4I',raw)),'serialized_sha256':hashlib.sha256(raw).hexdigest(),'code_sha256':hashlib.sha256(raw[0x30:0x30+length]).hexdigest()};banks.append(bank)
  # Literal +54 axis min/max pairs, also independently verified by the
  # transparent-parts metadata fixture. Cross each box in both directions.
  bounds=struct.unpack_from('<6f',c.data,at+0x54)
  bank['ray_bounds_source']='literal cluster+54 six floats'
  if any(not math.isfinite(bounds[2*j]) or not math.isfinite(bounds[2*j+1]) or bounds[2*j]>bounds[2*j+1] for j in range(3)):
   # Elongation29 has the literal empty FLT_MAX/-FLT_MAX sentinel even
   # though it owns four instances and valid MOPP bytecode. Use an explicit
   # diagnostic grid cube there, not invented render/collision bounds.
   info=struct.unpack_from('<4f',raw);extent=f32((256.*65536.)/info[3])
   bounds=tuple(v for j in range(3) for v in (info[j],f32(info[j]+extent)))
   bank['ray_bounds_source']='explicit diagnostic grid0..256 through literal MoppInfo; authored cluster has empty bounds sentinel'
  mid=[f32((bounds[2*j]+bounds[2*j+1])*.5) for j in range(3)]
  cases=[]
  for axis in range(3):
   a=list(mid);b=list(mid);a[axis]=f32(bounds[axis*2]-.5);b[axis]=f32(bounds[axis*2+1]+.5);cases.extend([(a,b),(b,a)])
  for mask in range(8):
   a=[f32(bounds[2*j+((mask>>j)&1)]+(.5 if (mask>>j)&1 else -.5)) for j in range(3)]
   b=[f32(bounds[2*j+(1-((mask>>j)&1))]+(-.5 if (mask>>j)&1 else .5)) for j in range(3)];cases.append((a,b))
  for ri,(p,end) in enumerate(cases):
   d=[f32(end[j]-p[j]) for j in range(3)];callbacks.clear();contexts.clear();instructions.clear()
   u.mem_write(ORIGIN,struct.pack('<3f',*p));u.mem_write(RAY,struct.pack('<3f',*d));u.mem_write(RESULT,bytes(96));put(RESULT+4,0x3f800000)
   put(0x4e7c1c,1);u.mem_write(0x4e7c20,bytes(512*4));put(0x4e7414,1);u.mem_write(0x4e7418,bytes(512*4))
   for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
   u.reg_write(UC_X86_REG_EAX,cluster);u.reg_write(UC_X86_REG_ESI,ORIGIN);u.reg_write(UC_X86_REG_EDI,RAY)
   put(STACK,STOP,0x04800005,9,RESULT);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2)
   u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   u.emu_start(0x244980,STOP,count=5000000);assert u.reg_read(UC_X86_REG_EIP)==STOP,(name,cluster,ri,hex(u.reg_read(UC_X86_REG_EIP)))
   flat=[*map(bits,p),*map(bits,d),0x3f800000,len(callbacks),*[v for row in callbacks for v in row],len(instructions),*[v for row in instructions for v in row],word(RESULT+4)]
   query_lines.append(f'Q {key}_{ri} {key} '+' '.join(f'{v:x}' for v in flat));ops.update(row[1] for row in instructions)
 maps.append({'map':name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'clusters':cluster_count,'banks':map_banks});print(name,map_banks,flush=True)
 u.mem_unmap(memory,allocated);c.data.close();c.stream.close()
fixture=ROOT/'tests/fixtures/retail_instance_accelerator_banks.tsv'
fixture.write_text('# Literal all21 MoppInfo banks and unchanged source cluster visitation on explicit authored-bounds rays\n'+f'C {len(query_lines)} {len(banks)} {len(ops)}\n'+'\n'.join(bank_lines+query_lines)+'\n')
report={'version':1,'xbe_sha256':PIN,'source':'unchanged244980/2DF760/2DF720/301800/3009E0/244BB0/244C10/1691A0','hooks':'read-only244C10 input,244C06 return and300A10 opcode observers; no replacement services','fpcw':'027F','mxcsr':'1F80','input':'04800005,testflags9,cold all-one breakables; fourteen explicit rays across each authored cluster bounds','maps':maps,'clusters':clusters_total,'banks':banks,'logical_code_bytes':logical_total,'queries':len(query_lines),'observed_opcodes':dict(ops),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'boundary':'Static authored instance accelerator with explicit cold collision state. Diagnostic rays do not imply original session/player traversal, dynamic registration or pixel parity.'}
fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n')
print('complete',len(banks),len(query_lines),len(ops),flush=True)
