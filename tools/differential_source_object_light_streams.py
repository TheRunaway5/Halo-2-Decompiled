#!/usr/bin/env python3
"""Execute unchanged D33A0/D31C0 on explicit resident CPU data banks.

Uses all42 authored extra-stream sections, eight complete retained diagnostic
attribution outputs and explicit declaration/offset/fallback arithmetic controls.
Authored resources are relocated inputs to the pure service, not a live object
residency/pose/cache producer. No guest/GPU I/O or substituted source services.
"""
from pathlib import Path
import collections,hashlib,json,random,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

SYNTH,GEOMETRY,DESCRIPTORS,INDICES,DATA,OUT,STACK,STOP=0x20000000,0x20000100,0x20000200,0x20001000,0x20020000,0x20000040,0x200fe000,0x200ff000
image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);mapped=set();verified=set();calls=collections.Counter()
for section in image.sections:
 lo=section.va&~4095;hi=(section.va+max(section.vsize,section.rsize)+4095)&~4095
 for page in range(lo,hi,4096):
  if page not in mapped:u.mem_map(page,4096);mapped.add(page)
 u.mem_write(section.va,image.section_bytes(section))
def code(g,address,size,data):
 if address in [0xd33a0,0xd31c0,0x131bb0]:calls[f'{address:08X}']+=1
 if address in verified or SYNTH<=address<SYNTH+0x100000:return
 for section in image.sections:
  if section.va<=address and address+size<=section.va+section.rsize:
   assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size]
   verified.add(address);return
 raise RuntimeError(f'uncaptured source service:{address:08X}')
u.hook_add(UC_HOOK_CODE,code)
def bits(v):return uint(struct.pack('<f',v))
def bytewords(raw):return [len(raw)]+words(raw+bytes((-len(raw))%4))
rows=['# Unchanged D33A0/D31C0. G=literal ordered descriptor/data/index bank; C=triangle,weights,source optional RGB/incident.',
 '# First3 ignored; source data+C is direct, descriptor+8 retained/ignored. Authored banks are relocated CPU-service inputs.']
def row(kind,values):rows.append(kind+' '+' '.join(f'{v&0xffffffff:08X}'for v in values))
geometries=[];case_count=0;counts=collections.Counter();actual=[]
def install(streams,indices,label):
 ident=len(geometries);values=[ident,len(indices)]+list(indices)+[len(streams)]
 u.mem_write(GEOMETRY,bytes(0x70));u.mem_write(GEOMETRY+0x24,packed([INDICES]));u.mem_write(GEOMETRY+0x38,packed([len(streams),DESCRIPTORS]))
 u.mem_write(INDICES,struct.pack('<'+'H'*len(indices),*indices));cursor=DATA
 for i,(declaration,offset,raw)in enumerate(streams):
  assert cursor+len(raw)+16<STACK
  u.mem_write(DESCRIPTORS+i*32,bytes([declaration,0,0,0])+packed([0,offset,cursor,0,0,0,0]));u.mem_write(cursor,raw+bytes(16))
  values +=[declaration,offset]+bytewords(raw);cursor=(cursor+len(raw)+31)&~15
 row('G',values);geometries.append({'id':ident,'label':label,'streams':[{'declaration':d,'offset':o,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}for d,o,raw in streams],'index_count':len(indices)})
 return ident
def run(address,triangle,ub,vb):
 u.mem_write(OUT,packed([0xcdcdcdcd]*3))
 for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 if address==0xd33a0:u.mem_write(STACK,packed([STOP,GEOMETRY,triangle,ub,vb,OUT]))
 else:
  u.mem_write(STACK,packed([STOP,GEOMETRY,ub,vb]));u.reg_write(UC_X86_REG_EBX,triangle);u.reg_write(UC_X86_REG_ESI,OUT)
 u.emu_start(address,STOP,count=10000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 success=bool(u.reg_read(UC_X86_REG_EAX)&255);output=words(bytes(u.mem_read(OUT,12)))
 if not success:assert output==[0xcdcdcdcd]*3
 return success,output
def case(ident,triangle,u_weight,v_weight,category,weight_words=False):
 global case_count
 ub,vb=(u_weight,v_weight)if weight_words else(bits(u_weight),bits(v_weight))
 radiance=run(0xd33a0,triangle,ub,vb);incident=run(0xd31c0,triangle,ub,vb)
 row('C',[ident,triangle,ub,vb,int(radiance[0])|int(incident[0])<<1]+radiance[1]+incident[1]);case_count+=1;counts[category]+=1
 counts['radiance_success']+=int(radiance[0]);counts['incident_success']+=int(incident[0])

#Literal paused resident banks. Only successful attribution packets have an
#original D1E10 data-service caller; do not invent triangles for57 misses.
snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
table=uint(snapshot.read(0x4e3b44,4));tag=lambda d:uint(snapshot.read(table+(d&65535)*16+8,4))
attribution=json.loads((ROOT/'analysis/multiplayer-shading/dynamic-object-attribution-snapshot-original.json').read_text());resident_ids={}
for record in attribution['records']:
 if not record.get('attributed'):continue
 output=record['output_words'];datum,section=output[3],output[5];key=(datum,section)
 if key not in resident_ids:
  mode=tag(datum);p=uint(snapshot.read(mode+0x28,4))+section*0x5c;g=uint(snapshot.read(p+0x34,4));n,dp=struct.unpack('<II',snapshot.read(g+0x38,8));ixn,ixp=struct.unpack('<II',snapshot.read(g+0x20,8));indices=struct.unpack('<'+'H'*ixn,snapshot.read(ixp,ixn*2));streams=[]
  for i in range(n):
   head=snapshot.read(dp+i*32,32);vertex_count=struct.unpack_from('<H',head,2)[0];stride=head[1];pointer=uint(head,12)
   streams.append((head[0],uint(head,8),snapshot.read(pointer,vertex_count*stride)))
  resident_ids[key]=install(streams,indices,{'source':'snapshot12B33E','render_model':f'{datum:08X}','section':section,'geometry':f'{g:08X}'})
 #Re-install the particular bank because later rows may have used another.
 ident=resident_ids[key]
 if key in resident_ids and ident!=len(geometries)-1:
  gdata=geometries[ident]
  #Only actual success records use two banks, and each owner batch is contiguous.
  assert False,'source contacts must remain grouped by resident geometry'
 case(ident,output[10],output[12],output[13],'retained_attribution',True)
 actual.append({'owner':record['owner'],'contact_index':record['index'],'geometry_id':ident,'triangle':output[10]})

#All authored extra-stream sections from the previous independent raw census.
census=json.loads((ROOT/'analysis/multiplayer-shading/model-extra-stream-census.json').read_text());caches={};authored=[]
for record in census['records']:
 name=record['map']
 if name not in caches:
  path=ROOT/'assets/disc/maps'/f'{name}.map'
  if not path.exists():path=ROOT/'assets/map-pack/maps'/f'{name}.map'
  caches[name]=Cache(path)
 cache=caches[name];_,mode=cache.tag(record['mode'],'mode');at=cache.block(mode+0x24,0x5c)[record['section']];resources=cache.block(at+0x48,16)
 pointer,size,header=[cache.u32(at+o)for o in [0x38,0x3c,0x44]]
 resource=cache.path if pointer>>30==0 else cache.path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[pointer>>30]
 if not resource.exists():resource=ROOT/'assets/disc/maps'/resource.name
 with resource.open('rb')as f:f.seek(pointer&0x3fffffff);raw=f.read(size)
 assert len(raw)==size;base=size-header-4
 des=next(r for r in resources if cache.data[r]==0 and cache.u16(r+4)==56);start=base+cache.u32(des+12);n=cache.u32(des+8)//32;streams=[]
 for i in range(n):
  descriptor=raw[start+i*32:start+(i+1)*32];payload=next(r for r in resources if cache.data[r]==2 and cache.u16(r+4)==56 and cache.u16(r+6)==i)
  payload_at=base+cache.u32(payload+12);payload_size=cache.u32(payload+8);data=raw[payload_at:payload_at+payload_size]
  if i>=3:assert hashlib.sha256(data).hexdigest()==record['streams'][i-3]['sha256']
  streams.append((descriptor[0],uint(descriptor,8),data))
 indices_resource=next(r for r in resources if cache.u16(r+4)==32);ixn=struct.unpack_from('<H',raw,0x28)[0];ix_at=base+cache.u32(indices_resource+12);indices=struct.unpack_from('<'+'H'*ixn,raw,ix_at)
 assert ixn%3==0
 ident=install(streams,indices,{'source':'authored_resource','map':name,'render_model':f'{record["mode"]:08X}','section':record['section'],'raw_sha256':hashlib.sha256(raw).hexdigest()})
 authored.append({'geometry_id':ident,'map':name,'mode':f'{record["mode"]:08X}','section':record['section'],'triangles':ixn//3})
 weights=[(0.,0.),(1.,0.),(0.,1.),(.3,.4),(-.25,1.25),(.5,.5),(2.,-1.),(-0.,0.)]
 for triangle in range(ixn//3):case(ident,triangle,*weights[triangle%len(weights)],'authored_triangle')
for cache in caches.values():cache.close()

#Explicit valid relocated descriptors test scanning, first-match/fallback,
#ignored +8, independent index order, signed midpoint/extreme color/weights.
rng=random.Random(0xd33a0)
layouts=[[],[48],[46,47,48],[1,24,27,48],[1,24,27,46],[1,24,27,47],[1,24,27,7],
 [1,24,27,46,47,48],[1,24,27,47,46,48],[1,24,27,7,48,46],[48,47,46,7,57,49],
 [1,24,27,48,48,46,46],[1,24,27,7,7,7,7]]
for li,layout in enumerate(layouts):
 for variant in range(16):
  streams=[]
  for si,declaration in enumerate(layout):
   values=[0,0xffffffff,0x80200400,0xff000000,0x00ffffff,0x00ff0080,0x004080c0]+[rng.getrandbits(32)for _ in range(25)]
   rng.shuffle(values);streams.append((declaration,((si+1)*17 if variant&1 else 0),packed(values)))
  indices=[rng.randrange(32)for _ in range(24)];ident=install(streams,indices,{'source':'explicit_synthetic','layout':li,'variant':variant})
  for triangle,weights in enumerate([(0.,0.),(1.,0.),(0.,1.),(.3,.4),(-1.5,3.),(.5,.5),(-0.,0.),(2.,-1.)]):case(ident,triangle,*weights,'synthetic')
fixture=ROOT/'tests/fixtures/retail_source_object_light_streams.tsv';fixture.write_text('\n'.join(rows)+'\n')
report={'version':1,'source_xbe_sha256':PIN,'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'source_instruction_sites_verified':len(verified),'calls':dict(calls),'counts':dict(counts),'cases':case_count,'geometries':geometries,'authored_sections':authored,'retained_contacts':actual,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_export_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F','mxcsr':'1F80','boundary':__doc__}
fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/object-light-streams-original.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k]for k in ['cases','counts','calls','source_instruction_sites_verified']},indent=2))
