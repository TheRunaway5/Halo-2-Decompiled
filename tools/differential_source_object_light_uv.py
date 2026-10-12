#!/usr/bin/env python3
"""Execute unchanged D2DC0 on literal resident/authored CPU UV banks.

Reuses independently frozen data/index banks. Explicit offset, declaration,
midpoint, interpolation and clamp controls are not observed game inputs.
"""
from pathlib import Path
import collections,hashlib,json,math,random,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import PIN,uint,packed,words,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);SYNTH=0x20000000;G=SYNTH+0x100;DESC=SYNTH+0x200;IX=SYNTH+0x1000;DATA=SYNTH+0x20000;OUT=SYNTH+0x40;STACK=SYNTH+0xfe000;STOP=SYNTH+0xff000
u.mem_map(SYNTH,0x100000);mapped=set();verified=set()
for sec in image.sections:
 for page in range(sec.va&~4095,(sec.va+max(sec.vsize,sec.rsize)+4095)&~4095,4096):
  if page not in mapped:u.mem_map(page,4096);mapped.add(page)
 u.mem_write(sec.va,image.section_bytes(sec))
def code(g,a,n,data):
 if a in verified or SYNTH<=a<SYNTH+0x100000:return
 for sec in image.sections:
  if sec.va<=a and a+n<=sec.va+sec.rsize:
   assert bytes(g.mem_read(a,n))==image.section_bytes(sec)[a-sec.va:a-sec.va+n];verified.add(a);return
 raise RuntimeError(f'unknown source service{a:08X}')
u.hook_add(UC_HOOK_CODE,code);rows=['# Complete unchangedD2DC0 CPU UV. G=id/declaration/offset/index-bank/data; C=id/triangle/weights/output2words.'];banks={};counts=collections.Counter();ident=0
def emit(k,v):rows.append(k+' '+' '.join(f'{x&0xffffffff:08X}'for x in v))
def install(decl,offset,indices,raw):
 global ident
 key=ident;ident+=1;emit('G',[key,decl,offset,len(indices),*indices,len(raw),*words(raw+bytes((-len(raw))%4))]);banks[key]=(decl,offset,indices,raw)
 u.mem_write(G,bytes(0x70));u.mem_write(G+0x24,packed([IX]));u.mem_write(G+0x3c,packed([DESC]));u.mem_write(DESC+0x20,bytes([decl,0,0,0])+packed([0,offset,DATA]));u.mem_write(IX,struct.pack('<'+'H'*len(indices),*indices));u.mem_write(DATA,raw);return key
def run(key,triangle,b,c,category):
 u.mem_write(OUT,packed([0xcdcdcdcd]*2));u.mem_write(STACK,packed([STOP,b,c]))
 for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_EAX,triangle);u.reg_write(UC_X86_REG_ECX,G);u.reg_write(UC_X86_REG_EBX,OUT);u.reg_write(UC_X86_REG_EDI,OUT+4);u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_FPCW,0x023f)
 u.emu_start(0xd2dc0,STOP,count=10000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 emit('C',[key,triangle,b,c,*words(bytes(u.mem_read(OUT,8)))]);counts[category]+=1
source=ROOT/'tests/fixtures/retail_source_object_light_streams.tsv';current=None
for line in source.read_text().splitlines():
 if not line or line[0]=='#':continue
 vals=[int(v,16)for v in line[2:].split()]
 if line[0]=='G':
  _,n=vals[:2];indices=vals[2:2+n];at=2+n;ns=vals[at];at+=1;streams=[]
  for _ in range(ns):
   decl,offset,size=vals[at:at+3];at+=3;raw=packed(vals[at:at+(size+3)//4])[:size];at+=(size+3)//4;streams.append((decl,offset,raw))
  current=None
  if ns>=2:
   decl,offset,raw=streams[1];stride=8 if decl==24 else 4
   if offset+max(indices,default=0)*stride+stride<=len(raw):
    #Finite data only; original malformed-memory/NaN controls are separate.
    if decl!=24 or all(math.isfinite(x)for x in struct.unpack('<'+'f'*(len(raw)//4),raw[:len(raw)//4*4])):current=install(decl,offset,indices,raw)
 elif line[0]=='C' and current is not None:run(current,*vals[1:4],'frozen_resident_authored_and_stream_controls')
rng=random.Random(0xd2dc0)
weights=[(0.,0.),(1.,0.),(0.,1.),(.3,.4),(-2.,3.),(1e5,-1e5),(-0.,0.),(.5,.5)]
for decl in [0,1,23,24,25,57,255]:
 for offset in [0,1,4,17]:
  if decl==24:
   points=[(-1001.,1001.),(-1000.,1000.),(-999.9999389648438,999.9999389648438),(1e-5,-1e-5)]+[(rng.uniform(-2000,2000),rng.uniform(-2000,2000))for _ in range(28)]
   raw=b''.join(struct.pack('<2f',*p)for p in points)
  else:
   points=[(-32768,32767),(-1,0),(1,-2),(32767,-32768)]+[(rng.randrange(-32768,32768),rng.randrange(-32768,32768))for _ in range(28)]
   raw=b''.join(struct.pack('<2h',*p)for p in points)
  indices=[0,1,2,2,3,0,*[rng.randrange(32)for _ in range(90)]];key=install(decl,offset,indices,bytes([0xa5])*offset+raw)
  for triangle in range(len(indices)//3):
   for b,c in weights:run(key,triangle,fw(b),fw(c),'explicit_declaration_offset_interpolation_clamp')
fixture=ROOT/'tests/fixtures/retail_source_object_light_uv.tsv';fixture.write_text('\n'.join(rows)+'\n');report={'source_xbe_sha256':PIN,'source_geometry_fixture_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'counts':dict(counts),'cases':sum(counts.values()),'geometries':len(banks),'source_instruction_sites_verified':len(verified),'boundary':__doc__}
fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/object-light-uv-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
