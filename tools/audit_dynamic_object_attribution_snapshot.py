#!/usr/bin/env python3
"""Run complete unchanged 14A8E0 over actual retained type-4 contacts.

Inputs come from declared diagnostic 1697C0 calls over the immutable snapshot.
No current pose, active-region, raw geometry, residency or math service is
replaced. This is an attribution audit, not native registration or sampling.
"""
from pathlib import Path
import collections,hashlib,json,random,re,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH,STACK,STOP,CONTACT,OUT=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200
snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008')
image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000)
pages={};verified=set();calls=collections.Counter();observations=[]

def page(address):
 base=address&~4095
 if base not in pages:
  raw=snapshot.read(base,4096);u.mem_map(base,4096);u.mem_write(base,raw);pages[base]=raw
def read(address,size):
 for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):
  if not SYNTH<=base<SYNTH+0x100000:page(base)
 return bytes(u.mem_read(address,size))
def unmapped(g,access,address,size,value,data):
 for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(base)
 return True
def code(g,address,size,data):
 if address in [0x14a8e0,0x12de70,0x142700,0x1efb40,0x1efd80,0x23ac30,0x11e800]:calls[f'{address:08X}']+=1
 if address==0x12de70:
  descriptor=g.reg_read(UC_X86_REG_ECX)
  observations.append({'service':'geometry-residency','descriptor':f'{descriptor:08X}','raw':read(descriptor,40).hex(),'mode':g.reg_read(UC_X86_REG_EAX)})
 if address==0x23ac30:
  esp=g.reg_read(UC_X86_REG_ESP)
  observations.append({'service':'render-position','vertex':uint(read(esp+4,4)),'geometry':f'{g.reg_read(UC_X86_REG_EDX):08X}','bounds':read(g.reg_read(UC_X86_REG_EAX),24).hex()})
 if address in verified or SYNTH<=address<SYNTH+0x100000:return
 for section in image.sections:
  if section.va<=address and address+size<=section.va+section.rsize:
   assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'changed source instruction {address:08X}'
   verified.add(address);return
 raise RuntimeError(f'uncaptured external instruction service {address:08X}')
u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code)
gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before'])
ga,gl=(int(v,16)for v in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
for reg,selector in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
table=uint(snapshot.read(0x4e3b44,4));owners={int(v['handle'],16):v for v in snapshot.report['objects']}
def tag(datum):return uint(snapshot.read(table+(datum&65535)*16+8,4))
source=json.loads((ROOT/'analysis/multiplayer-shading/dynamic-object-contact-snapshot/report.json').read_text())
counts=collections.Counter();records=[]
for index,contact in enumerate(source['differences']):
 assert contact['result_words'][0]==4
 handle=contact['result_words'][16];owner=owners[handle];raw=snapshot.file('object-'+owner['handle']+'.bin')
 definition=uint(raw);typ=raw[0xaa];meta=tag(definition)
 policy=struct.unpack('<h',snapshot.read(meta+0xc0,2))[0]if typ==6 else-1
 model=uint(snapshot.read(meta+0x38,4));render=collision=0xffffffff;prt=leaves=0
 if model!=0xffffffff:
  m=tag(model);render=uint(snapshot.read(m+4,4));collision=uint(snapshot.read(m+12,4))
  if render!=0xffffffff and collision!=0xffffffff:
   rp=tag(render);prt=uint(snapshot.read(rp+0x74,4));leaves=uint(snapshot.read(rp+0x7c,4))
 action='reject-scenery'if typ==6 and policy!=2 else'attribute-model'if prt>0 and leaves>0 and collision!=0xffffffff else'fallback-map'
 record={'index':index,'owner':owner['handle'],'flags':contact['flags'],'type':typ,'definition':f'{definition:08X}','scenery_policy':policy,'model':f'{model:08X}','render_model':f'{render:08X}','collision':f'{collision:08X}','prt_count':prt,'render_leaves_count':leaves,'action':action}
 counts[action]+=1
 if action=='attribute-model':
  observations.clear();u.mem_write(CONTACT,packed(contact['result_words']));u.mem_write(OUT,packed([0xffffffff]*3+[0]*29));u.mem_write(STACK,packed([STOP,CONTACT,render,OUT]))
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  try:
   u.emu_start(0x14a8e0,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
   record['attributed']=bool(u.reg_read(UC_X86_REG_EAX)&255);record['output_words']=words(read(OUT,56));counts['attribution-success'if record['attributed']else'attribution-source-miss']+=1
  except Exception as error:
   record['unresolved']={'error':str(error),'eip':f'{u.reg_read(UC_X86_REG_EIP):08X}'};counts['unresolved-service']+=1
  record['observations']=list(observations)
 records.append(record)
protected=0
for entry in snapshot.report['files']:
 if entry['address_space']=='CPU0 virtual' and ('object-'in entry['file']or any(s in entry['file']for s in ['collidable','noncollidable'])):
  address=int(entry['address'],16);assert read(address,entry['size'])==snapshot.read(address,entry['size']),entry['file'];protected+=1
report={'version':1,'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_export_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'source_contact_query_sha256':source['query_sha256'],'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F literal snapshot FCW','mxcsr':'1F80','counts':dict(counts),'calls':dict(calls),'instructions_verified':len(verified),'protected_banks':protected,'records':records,'boundary':'Diagnostic original1697C0 contacts, not captured query scheduling. Current owner/pose/active/geometry/residency/tag banks are literal immutable exports. No source service replacement or guest I/O/advance. This audit does not implement D1E10 model/PRT sampling.'}
fixture_rows=['# Complete unchanged14A8E0 with literal retained owner/pose/active/resident CPU geometry.',
 '# M=model bank; O=actual owner; C=96-byte original contact/56-byte source result;',
 '# P=original23AC30 CPU position; I=original142700 inverse point. All scalar fields hexadecimal words.']
def row(kind,values):fixture_rows.append(kind+' '+' '.join(f'{v&0xffffffff:08X}'for v in values))
def bytewords(data):return [len(data)]+words(data+bytes((-len(data))%4))
def signed16(address):return struct.unpack('<h',snapshot.read(address,2))[0]
def block(address):return uint(snapshot.read(address,4)),uint(snapshot.read(address+4,4))
model_banks={};owner_banks={}
for record in records:
 if record['action']!='attribute-model':continue
 datum=int(record['render_model'],16);handle=int(record['owner'],16)
 if datum not in model_banks:
  base=tag(datum);rn,rp=block(base+0x1c);sn,sp=block(base+0x24);nn,np=block(base+0x48)
  bounds=words(snapshot.read(uint(snapshot.read(base+0x18,4)),24))
  bank=[datum]+bounds+[nn]
  for ni in range(nn):bank+=words(snapshot.read(np+ni*0x60+0x28,52))
  bank.append(rn)
  for ri in range(rn):
   pn,pp=block(rp+ri*16+8);bank.append(pn)
   for pi in range(pn):bank.append(signed16(pp+pi*16+12)&0xffffffff)
  bank.append(sn);formats=[]
  for si in range(sn):
   geometry=uint(snapshot.read(sp+si*0x5c+0x34,4));assert geometry
   descriptor=uint(snapshot.read(geometry+0x3c,4));head=snapshot.read(descriptor,16)
   form=head[0];stride={1:12,2:6,3:16,4:8}[form];vn=struct.unpack_from('<H',head,2)[0]
   pos=uint(head,8)+uint(head,12);ixn,ixp=block(geometry+0x20)
   nodes_n,nodes_p=block(geometry+0x64)
   section_maps=uint(snapshot.read(base+0x80,4))+si*8;mn,mp=block(section_maps)
   bank +=[1,form,vn]+bytewords(snapshot.read(pos,vn*stride))+[ixn]
   bank += list(struct.unpack('<'+'H'*ixn,snapshot.read(ixp,ixn*2)))
   bank += bytewords(snapshot.read(nodes_p,nodes_n))+[mn]
   for mi in range(mn):
    ln,lp=block(mp+mi*16);refn,refp=block(mp+mi*16+8);bank.append(ln)
    for li in range(ln):
     cluster,count,first=struct.unpack('<hhi',snapshot.read(lp+li*8,8));bank +=[cluster&0xffffffff,count&0xffffffff,first&0xffffffff]
    bank.append(refn)
    for refi in range(refn):
     strip,triangle,node=struct.unpack('<HHi',snapshot.read(refp+refi*8,8));bank +=[strip,triangle,node&0xffffffff]
   formats.append({'section':si,'format':form,'vertex_count':vn,'indices':ixn,'position_sha256':hashlib.sha256(snapshot.read(pos,vn*stride)).hexdigest(),'resident_geometry':f'{geometry:08X}'})
  collision=tag(int(record['collision'],16));crn,crp=block(collision+0x1c);bank.append(crn)
  for ri in range(crn):
   pn,pp=block(crp+ri*12+4);bank.append(pn)
   for pi in range(pn):
    bn,bp=block(pp+pi*20+4);bank.append(bn)
    for bi in range(bn):
     bsp=bp+bi*0x44+4;n,np=block(bsp);ln,_=block(bsp+0x10)
     bank +=[ln,n]
     for ni in range(n):bank.append(struct.unpack('<H',snapshot.read(np+ni*8,2))[0])
  model_banks[datum]={'words':bank,'positions':formats,'regions':rn};row('M',bank)
 if handle not in owner_banks:
  owner=owners[handle];raw=snapshot.file('object-'+owner['handle']+'.bin');address=int(owner['address'],16)
  active_offset=struct.unpack_from('<h',raw,0x11a)[0];node_bytes,node_offset=struct.unpack_from('<hh',raw,0x114)
  assert node_bytes>=0 and node_bytes%52==0
  active=snapshot.read(address+active_offset,model_banks[datum]['regions'])
  world=snapshot.read(address+node_offset,node_bytes)
  bank=[handle,datum]+bytewords(active)+[node_bytes//52]+words(world)
  owner_banks[handle]=bank;row('O',bank)
 contact=source['differences'][record['index']]['result_words'];row('C',[datum,handle,int(record['attributed'])]+contact+record['output_words'])
# Isolated unchanged decoder/inverse arithmetic controls, in addition to the
# complete actual-data calls. These inputs are explicit diagnostics, not game RNG.
rng=random.Random(0x14a8e0);position_cases=0;inverse_cases=0
def reset():
 for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 u.mem_write(STACK,packed([STOP,0]));u.mem_write(OUT,bytes(16))
bounds_cases=[b['words'][1:7]for b in model_banks.values()]+[words(struct.pack('<6f',-7,13,-.5,4,-100,.125))]
for form in [1,2,3,4]:
 for i in range(128):
  bounds=bounds_cases[i%len(bounds_cases)]
  if form in [2,4]:
   codes=[[-32768,-32767,-1,0,1,32766,32767][(i+j)%7]if i<7 else rng.randint(-32768,32767)for j in range(3)]
   data=struct.pack('<3h',*codes)
   if form==4:data+=bytes([(i*37)&255,0])
  else:
   values=[rng.uniform(-500,500)for _ in range(3)]if i else[-0.,0.,-1.]
   data=struct.pack('<3f',*values)
   if form==3:data+=bytes([(i*37)&255,0,0,0])
  reset();geometry=SYNTH+0x400;descriptor=SYNTH+0x500;positions=SYNTH+0x600;bounds_at=SYNTH+0x700
  u.mem_write(geometry+0x3c,packed([descriptor]));u.mem_write(descriptor,bytes([form,0])+struct.pack('<H',1)+packed([0,0,positions]));u.mem_write(positions,data);u.mem_write(bounds_at,packed(bounds))
  u.reg_write(UC_X86_REG_EAX,bounds_at);u.reg_write(UC_X86_REG_EDX,geometry);u.reg_write(UC_X86_REG_ECX,OUT);u.reg_write(UC_X86_REG_EDI,OUT+12)
  u.emu_start(0x23ac30,STOP,count=1000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  row('P',[form]+bounds+[1]+bytewords(data)+words(read(OUT,16)));position_cases+=1
inverse_inputs=[]
for owner in owner_banks.values():
 #owner,render,bytecount,padded active words,nodecount,52byte nodes.
 offset=3+(owner[2]+3)//4;node_count=owner[offset];offset+=1
 inverse_inputs +=[owner[offset+i*13:offset+(i+1)*13]for i in range(node_count)]
for model in model_banks.values():
 values=model['words'];inverse_inputs +=[values[8+i*13:8+(i+1)*13]for i in range(values[7])]
for i in range(128):
 scale=[0.,-0.,1.,-1.,.25,2.,-.7][i%7];values=[scale]+[rng.uniform(-2,2)for _ in range(9)]+[rng.uniform(-100,100)for _ in range(3)]
 inverse_inputs.append(words(struct.pack('<13f',*values)))
for transform in inverse_inputs:
 p=words(struct.pack('<3f',*(rng.uniform(-200,200)for _ in range(3))));reset();u.mem_write(SYNTH+0x400,packed(transform));u.mem_write(SYNTH+0x500,packed(p));u.reg_write(UC_X86_REG_ECX,SYNTH+0x400);u.reg_write(UC_X86_REG_ESI,SYNTH+0x500);u.reg_write(UC_X86_REG_EDX,OUT)
 u.emu_start(0x142700,STOP,count=1000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 row('I',transform+p+words(read(OUT,12)));inverse_cases+=1
fixture=ROOT/'tests/fixtures/retail_source_object_light_attribution.tsv';fixture.write_text('\n'.join(fixture_rows)+'\n')
provenance={k:report[k]for k in ['source_xbe_sha256','snapshot_ram_sha256','snapshot_export_sha256','source_contact_query_sha256','snapshot_tick','snapshot_eip','fpcw','mxcsr','counts','protected_banks','boundary']}
provenance.update({'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'instruction_sites_verified':len(verified),'models':[{ 'datum':f'{d:08X}','positions':b['positions']}for d,b in model_banks.items()],'owners':len(owner_banks),'position_cases':position_cases,'inverse_cases':inverse_cases,'services_replaced':0})
fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');report['frozen_fixture']=provenance
path=ROOT/'analysis/multiplayer-shading/dynamic-object-attribution-snapshot-original.json';path.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k]for k in ['counts','calls','instructions_verified','protected_banks']},indent=2))
