#!/usr/bin/env python3
"""Freeze complete unchanged1697C0 static contacts for all authored MP rays.

Original callbacks/MOPP/mainBSP/instance queries execute; no service substitution.
Source static-only04800005 and cold all-one breakables are explicit inputs.
This reads caches/XBE only, with read-only callback/cluster observers. It does
not read/advance a guest or manufacture dynamic registration/pool state.
"""
from pathlib import Path
import hashlib,json,struct,sys,time
from collections import Counter
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
sys.path.insert(0,str(Path(__file__).resolve().parent));from inspect_multiplayer import Cache,MAPS
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'analysis/multiplayer-shading/instance-spawn-contacts-original';OUT.mkdir(exist_ok=True)
EXTRA=('backwash','containment','deltatap','dune','elongation','gemini','triplicate','turf','warlock');NAMES=(*MAPS,*EXTRA)
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d';image=(ROOT/'assets/disc/default.xbe').read_bytes();assert hashlib.sha256(image).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0,0x601000);base=struct.unpack_from('<I',image,0x104)[0];count,table=struct.unpack_from('<II',image,0x11c)
for i in range(count):
 _,va,_,off,size=struct.unpack_from('<5I',image,table-base+i*56);u.mem_write(va,image[off:off+size])
u.mem_map(0x2000000,0x20000);u.mem_map(0x3000000,0x1000)
def put(at,*words):u.mem_write(at,struct.pack('<'+'I'*len(words),*words))
def word(at):return struct.unpack('<I',u.mem_read(at,4))[0]
put(0x4ed280,0x2010000);u.mem_write(0x4686c4,b'\0\0');u.mem_write(0x2010000,bytes([255])*0x1000)
ORIGIN,RAY,RESULT,STACK,STOP=0x2000100,0x2000200,0x2000300,0x200f000,0x3000000
observations=[];cluster=-1
# These two observers change no execution state.
def observe_callback(g,a,size,_):
 context=g.reg_read(UC_X86_REG_ECX);shape=word(g.reg_read(UC_X86_REG_ESP)+4);key=word(shape+4)
 observations.append([cluster,key,word(context+4),word(RESULT),word(RESULT+0x3c)])
def observe_cluster(g,a,size,_):
 global cluster
 cluster=g.reg_read(UC_X86_REG_EAX)
u.hook_add(UC_HOOK_CODE,observe_callback,begin=0x244c10,end=0x244c10);u.hook_add(UC_HOOK_CODE,observe_cluster,begin=0x244980,end=0x244980)
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
RAYS=((0,0,-10),(-10,0,0),(10,0,0),(0,-10,0),(0,10,0),(0,0,-40))
raw_ties=ROOT/'analysis/multiplayer-shading/instance-tie-census.tsv';tie_rows={}
for line in raw_ties.read_text().splitlines():
 a=line.split();p=tuple(map(float,a[5:8]));d=tuple(map(float,a[8:11]));tie_rows[(a[1],*map(bits,p),*map(bits,d))]=(p,d)
fixture=ROOT/'tests/fixtures/retail_instance_accelerator_contacts.tsv';rows=[];callback_rows=[];totals=Counter();maps=[];began=time.monotonic()
for ordinal,name in enumerate(NAMES):
 path=ROOT/('assets/disc/maps' if ordinal<len(MAPS) else 'assets/map-pack/maps')/(name+'.map');c=Cache(path)
 entry=c.block(c.scenario+0x210,0x44)[0];start,size,memory=c.u(entry),c.u(entry+4),c.u(entry+8);allocated=(size+4095)//4096*4096;magic=memory-start
 u.mem_map(memory,allocated);u.mem_write(memory,bytes(c.data[start:start+size]));put(0x4e0348,memory+16);put(0x4e0340,c.u(start+16+0x18))
 instances=c.u(start+16+0x144)-magic;case_inputs=[];spawns=c.block(c.scenario+0x100,52)
 for index,at in enumerate(spawns):
  position=struct.unpack_from('<3f',c.data,at)
  for ri,d in enumerate(RAYS):
   p=tuple(f32(position[j]-f32(d[j]*f32(.001))) for j in range(3));case_inputs.append((0,index,ri,p,d))
 for key,(p,d) in tie_rows.items():
  if key[0]==name:case_inputs.append((1,len(case_inputs)-len(spawns)*6,0,p,d))
 status=Counter();callbacks=0;rawfile=OUT/(name+'.jsonl')
 with rawfile.open('w') as stream:
  for kind,index,ri,p,d in case_inputs:
   observations.clear();u.mem_write(ORIGIN,struct.pack('<3f',*p));u.mem_write(RAY,struct.pack('<3f',*d));u.mem_write(RESULT,bytes(96));put(RESULT+4,0x3f800000);put(0x4e7c1c,1);u.mem_write(0x4e7c20,bytes(512*4));put(0x4e7414,1);u.mem_write(0x4e7418,bytes(512*4))
   put(STACK,STOP,0x04800005,ORIGIN,RAY,0xffffffff,0xffffffff,RESULT)
   for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
   u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
   u.emu_start(0x1697c0,STOP,count=5000000);assert u.reg_read(UC_X86_REG_EIP)==STOP,(name,index,ri,hex(u.reg_read(UC_X86_REG_EIP)))
   hit=u.reg_read(UC_X86_REG_EAX)&255;raw=bytes(u.mem_read(RESULT,96));raw_words=list(struct.unpack('<24I',raw));record={'kind':kind,'index':index,'ray_index':ri,'origin_words':list(map(bits,p)),'ray_words':list(map(bits,d)),'hit':hit,'result_words':raw_words,'callbacks':list(observations)};stream.write(json.dumps(record,separators=(',',':'))+'\n')
   typ=raw_words[0];status['hit' if hit else 'miss']+=1;status[f'type{typ}']+=1;callbacks+=len(observations)
   out=[hit]
   if hit:
    assert typ in [1,3],(name,typ)
    instance=raw_words[15];definition=struct.unpack_from('<H',c.data,instances+instance*88+52)[0] if typ==3 else 0
    flags,broken,material=struct.unpack_from('<BBh',raw,0x58)
    out += [typ,raw_words[1],instance,definition,raw_words[19],raw_words[20],raw_words[21]&65535,flags,broken,material&0xffffffff,*raw_words[2:5]]
   row=[1,ordinal,kind,index,ri,0x04800005,*map(bits,p),*map(bits,d)]
   label=f'{name}_{"spawn" if kind==0 else "tie"}_{index}_{ri}';rows.append(label+'\t'+' '.join(f'{x:08x}' for x in row)+' | '+' '.join(f'{x:08x}' for x in out))
   callback_rows.append(label+'\t'+' '.join(f'{x:08x}' for x in row)+' | '+' '.join(f'{x:08x}' for x in [len(observations),*(v for observation in observations for v in observation)]))
 # Partial frozen output survives an interrupted host oracle run.
 fixture.write_text('# unchanged complete1697C0 static-only04800005; explicit027F/1F80; no dynamic registration\n'+'\n'.join(rows)+'\n')
 meta={'map':name,'ordinal':ordinal,'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bsp_file_offset':start,'bsp_size':size,'bsp_address':memory,'spawns':len(spawns),'cases':len(case_inputs),'callbacks':callbacks,'counts':dict(status),'raw_sha256':hashlib.sha256(rawfile.read_bytes()).hexdigest()};maps.append(meta);totals.update(status)
 print(name,'cases',len(case_inputs),'callbacks',callbacks,'status',dict(status),'elapsed',round(time.monotonic()-began,2),flush=True)
 u.mem_unmap(memory,allocated);c.data.close();c.stream.close()
callback_fixture=fixture.with_name('retail_instance_accelerator_callbacks.tsv');callback_fixture.write_text('# unchanged1697C0/244C10 ordered source callbacks; static-only04800005,027F/1F80\n'+'\n'.join(callback_rows)+'\n')
report={'version':1,'xbe_sha256':PIN,'source':'complete unchanged1697C0, main1DE630 and actual244980/3009E0/244BB0/244C10/1691A0 accelerator path','source_flags':'04800005: staticBSP+instances, no dynamic owner/fog query','fpcw':'027F','mxcsr':'1F80','breakables':'explicit cold all-one source bank, unchanged183FC0','hooks':'read-only244980 cluster and244C10 callback observers, no service/math replacement','maps':maps,'cases':len(rows),'counts':dict(totals),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'callback_fixture_sha256':hashlib.sha256(callback_fixture.read_bytes()).hexdigest(),'boundary':'Authored static geometry query, explicit cold state. No guest/session state, registration or dynamic-object contact claim. Mode/ray enumeration includes all six original lighting directions rather than source query short-circuiting.'}
(OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n');fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');print('complete',len(rows),flush=True)
