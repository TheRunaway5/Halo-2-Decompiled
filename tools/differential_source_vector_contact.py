#!/usr/bin/env python3
"""Freeze unchanged1697C0 over immutable actual map/owner/spatial banks.

Target outcomes are independently executed1EFB40/1EFF40 plus168B40. They are
explicit callback inputs to the outer-dispatcher test, not invented native pose
or registration state. No guest I/O or source-instruction replacement occurs.
"""
from pathlib import Path
import argparse,collections,hashlib,json,re,struct,sys,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,P,D,OUT=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000300
LOCAL,PLANE,WORLD,CONVERT=0x20000400,0x20000500,0x20000600,0x20000700

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/retail_source_vector_contact.tsv')
 parser.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/vector-contact-original.json')
 args=parser.parse_args()
 snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter();targets=[];wrappers=[];pending=None;observing=False
 def page(address):
  base=address&~4095
  if base not in pages:
   raw=snapshot.read(base,4096);u.mem_map(base,4096);u.mem_write(base,raw);pages[base]=raw
 def read(address,size):
  for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):
   if not SYNTH<=base<SYNTH+0x100000:page(base)
  return bytes(u.mem_read(address,size))
 def put(address,*values):read(address,len(values)*4);u.mem_write(address,packed(values))
 def unmapped(g,access,address,size,value,data):
  for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(base)
  return True
 def code(g,address,size,data):
  nonlocal pending
  if address in [0x1697c0,0x169510,0x168a10,0x1efb40,0x1eff40,0x168b40,0x244980,0x244c10]:calls[f'{address:08X}']+=1
  if observing:
   if address==0x169510:
    esp=g.reg_read(UC_X86_REG_ESP);wrappers.append([uint(read(esp+4,4)),uint(read(esp+8,4))&255])
   if address==0x1695a0:targets.append({'requested':g.reg_read(UC_X86_REG_EBX),'hit':0})
   if address==0x1eff40:pending=uint(read(g.reg_read(UC_X86_REG_ESP)+20,4))
   if address==0x1695d7:
    assert pending is not None
    if g.reg_read(UC_X86_REG_EAX)&255:
     raw=read(pending,36);plane=read(uint(raw,12),16)
     targets[-1].update(hit=1,raw=raw,plane=plane)
    pending=None
  if address in verified or SYNTH<=address<SYNTH+0x100000:return
  for section in image.sections:
   if section.va<=address and address+size<=section.va+section.rsize:
    assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'changed source instruction:{address:08X}'
    verified.add(address);return
  raise RuntimeError(f'uncaptured external source service:{address:08X}')
 u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code)
 gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);ga,gl=(int(x,16)for x in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
 for reg,selector in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
 def reset():
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 owners={int(o['handle'],16):o for o in snapshot.report['objects']};table=uint(snapshot.read(0x4e3b44,4))
 def tag(datum):return uint(snapshot.read(table+(datum&65535)*16+8,4))
 def target_words(target):
  requested=target['requested']
  if not target['hit']:return [requested,0]
  raw=target['raw'];node,region=struct.unpack_from('<HH',raw,4);owner=owners[next(h for h in owners if h&65535==requested&65535)];body=snapshot.file('object-'+owner['handle']+'.bin');address=int(owner['address'],16)
  offset=struct.unpack_from('<h',body,0x114)[0];node_offset=struct.unpack_from('<h',body,0x116)[0]
  assert offset%52==0 and node<offset//52
  transform=snapshot.read(address+node_offset+node*52,52)
  # Preserve the actual target1EFF40 local packet but relocate its immutable
  # plane pointer, then execute original168B40 independently, without hooks
  # replacing its math. Outer closest-fraction does not affect this outcome.
  local=bytearray(raw[8:32]);struct.pack_into('<I',local,4,PLANE)
  u.mem_write(LOCAL,bytes(local));u.mem_write(PLANE,target['plane']);u.mem_write(WORLD,transform);u.mem_write(CONVERT,bytes(96));put(STACK,STOP);reset()
  u.reg_write(UC_X86_REG_EAX,CONVERT);u.reg_write(UC_X86_REG_ECX,WORLD);u.reg_write(UC_X86_REG_EDX,LOCAL);u.emu_start(0x168b40,STOP,count=1000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  out=words(read(CONVERT,96));mat=struct.unpack_from('<h',raw,30)[0]
  definition=tag(uint(body));model_datum=uint(snapshot.read(definition+0x38,4));global_material=struct.unpack('<h',snapshot.read(0x47d8e0,2))[0]
  if model_datum!=0xffffffff:
   model=tag(model_datum);materials=uint(snapshot.read(model+0x5c,4));global_material=struct.unpack('<h',snapshot.read(materials+mat*20+16,2))[0]
  return [requested,1,out[1],*out[10:14],uint(raw),out[19],out[20],out[21],region,node,mat&0xffffffff,global_material&0xffffffff,raw[28],raw[29]]
 bsp=uint(snapshot.read(0x4e0348,4));cluster_n=uint(snapshot.read(bsp+0x9c,4));cluster_p=uint(snapshot.read(bsp+0xa0,4));assert cluster_n==55
 material_n=uint(snapshot.read(bsp+0xc,4));material_p=uint(snapshot.read(bsp+0x10,4))
 fog_n=uint(snapshot.read(bsp+0x64,4));fog_p=uint(snapshot.read(bsp+0x68,4));bsp_index=struct.unpack('<h',snapshot.read(0x4686c4,2))[0]
 bank=[bsp_index&0xffffffff,struct.unpack('<h',snapshot.read(0x47d8e0,2))[0]&0xffffffff,material_n]
 bank += [struct.unpack('<h',snapshot.read(material_p+i*20+8,2))[0]&0xffffffff for i in range(material_n)]
 bank +=[cluster_n]+[snapshot.read(cluster_p+i*176+0x70,1)[0]for i in range(cluster_n)]+[fog_n]
 for i in range(fog_n):bank +=[struct.unpack('<h',snapshot.read(fog_p+i*24+2,2))[0]&0xffffffff]+words(snapshot.read(fog_p+i*24+4,16))
 # Current breakable bitmap follows original1697C0's active BSP slot+1.
 broken=uint(snapshot.read(0x4ed280,4))+bsp_index*32+1
 bank += [8]+words(snapshot.read(broken,32))
 def bank_row(values):return 'B '+' '.join(f'{v:08x}'for v in values)
 rows=['# Unchanged1697C0 over retained12B33E raw map/owner/spatial banks. Diagnostic rays, not source call-phase.',
       '# B: BSP/default/materials/fog/breakable inputs. Q: flags/exclusions/origin/delta/caller24 | target outcomes | hit+packet24+wrapper entries+target order.',
       bank_row(bank)]
 source=ROOT/'analysis/multiplayer-shading/dynamic-object-contact-snapshot/queries.jsonl';original=[json.loads(line)for line in source.read_text().splitlines()];assert len(original)==3792
 cases=[];static_seen=set()
 for i,q in enumerate(original):
  cases.append((f'dynamic_{i}',int(q['flags'],16),int(q['excluded'],16),q['origin_words'],q['ray_words'],[0]*24,q))
  key=(q['sphere_owner'],q['axis'],q['sign'])
  if key not in static_seen:
   static_seen.add(key);cases.append((f'static_{len(static_seen)-1}',0x04800005,0xffffffff,q['origin_words'],q['ray_words'],[0]*24,q))
 assert len(static_seen)==948
 # Bounded ordinary flag/default/type-mask/first-contact controls retain actual
 # banks and poses, with explicit caller sentinels to expose unwritten bytes.
 for i,q in enumerate(original[::32]):
  for f in [1,4,8,15,0x1480000d,0x14800c0d,0x04800002,0x04800003]:
   cases.append((f'control_{i}_{f:08x}',f,0xffffffff,q['origin_words'],q['ray_words'],[(0xa5a50000+j*0x103)&0xffffffff for j in range(24)],None))
 # Fog metadata below is explicitly synthetic input, over the same literal
 # map traversal and owner banks. Zanzibar's retained plane material is NONE;
 # it cannot exercise an admitted plane. Original code/math remain untouched.
 fog_overrides={};saved_designators=[snapshot.read(cluster_p+i*176+0x70,1)for i in range(cluster_n)];saved_fog=snapshot.read(fog_p,24)
 for i,q in enumerate(original[::128]):
  axis=next(a for a,w in enumerate(q['ray_words'])if w&0x7fffffff);origin=[struct.unpack('<f',packed([w]))[0]for w in q['origin_words']];delta=[struct.unpack('<f',packed([w]))[0]for w in q['ray_words']]
  for fi,fraction in enumerate([-.25,0.,.125,.5,.999,1.,1.25]):
   normal=[0.,0.,0.];normal[axis]=1. if (i+fi)%2 else-1.
   # Source distance arithmetic is explicit single precision input generation.
   distance=struct.unpack('<f',struct.pack('<f',normal[axis]*(origin[axis]+delta[axis]*fraction)))[0]
   for flags in [2,3,15,0x1480000f]:
    label=f'fog_{i}_{fi}_{flags:08x}';fog_overrides[label]=(128,7,[*map(fw,normal),fw(distance)])
    cases.append((label,flags,0xffffffff,q['origin_words'],q['ray_words'],[(0x5a5a0000+j*0x101)&0xffffffff for j in range(24)],None))
 # Direct palette/NONE designators and materialNONE are independent rejects.
 for i,(designator,mat)in enumerate([(0,7),(255,7),(128,-1)]):
  q=original[0];label=f'fog_reject_{i}';fog_overrides[label]=(designator,mat,[fw(1.),0,0,q['origin_words'][0]])
  cases.append((label,2,0xffffffff,q['origin_words'],q['ray_words'],[0]*24,None))
 stats=collections.Counter();began=time.monotonic()
 for index,(label,flags,excluded,origin,delta,caller,old) in enumerate(cases):
  if label in fog_overrides:
   designator,mat,plane=fog_overrides[label]
   for ci in range(cluster_n):read(cluster_p+ci*176+0x70,1);u.mem_write(cluster_p+ci*176+0x70,bytes([designator]))
   read(fog_p,24);u.mem_write(fog_p+2,struct.pack('<h',mat));u.mem_write(fog_p+4,packed(plane))
   explicit=[bsp_index&0xffffffff,bank[1],material_n,*bank[3:3+material_n],cluster_n,*([designator]*cluster_n),1,mat&0xffffffff,*plane,8,*bank[-8:]]
   rows.append(bank_row(explicit));stats['synthetic_fog_inputs']+=1
  targets.clear();wrappers.clear();pending=None;u.mem_write(P,packed(origin));u.mem_write(D,packed(delta));u.mem_write(OUT,packed(caller));put(STACK,STOP,flags,P,D,excluded,0xffffffff,OUT);reset();observing=True
  u.emu_start(0x1697c0,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP;observing=False
  hit=u.reg_read(UC_X86_REG_EAX)&255;out=words(read(OUT,96));saved=list(targets);saved_wrappers=list(wrappers)
  if old is not None:
   if label.startswith('dynamic'):assert hit==old['hit'] and out==old['result_words'] and saved_wrappers==old['target_calls']
   else:assert hit==old['static_hit'] and out==old['static_words']
  service=[len(saved)]
  for t in saved:service+=target_words(t)
  expected=[hit,*out,len(saved_wrappers),*(v for w in saved_wrappers for v in w),len(saved),*(t['requested']for t in saved)]
  inputs=[flags,excluded,0xffffffff,*origin,*delta,*caller]
  rows.append(label+'\t'+' '.join(f'{v:08x}'for v in inputs)+' | '+' '.join(f'{v:08x}'for v in service)+' | '+' '.join(f'{v:08x}'for v in expected))
  stats['cases']+=1;stats['hits']+=bool(hit);stats[f'type{out[0]}']+=1;stats['targets']+=len(saved);stats['target_hits']+=sum(t['hit']for t in saved);stats['wrapper_calls']+=len(saved_wrappers)
  if index%512==0:print(index,'elapsed',round(time.monotonic()-began,1),flush=True)
 for ci,raw in enumerate(saved_designators):u.mem_write(cluster_p+ci*176+0x70,raw)
 if fog_overrides:u.mem_write(fog_p,saved_fog)
 assert read(fog_p,24)==saved_fog and all(read(cluster_p+ci*176+0x70,1)==raw for ci,raw in enumerate(saved_designators))
 protected=[]
 for e in snapshot.report['files']:
  if e['address_space']=='CPU0 virtual' and ('object-'in e['file']or any(s in e['file']for s in ['collidable','noncollidable'])):
   at=int(e['address'],16);assert read(at,e['size'])==snapshot.read(at,e['size']),e['file'];protected.append(e['file'])
 args.fixture.write_text('\n'.join(rows)+'\n')
 provenance={'version':1,'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_manifest_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'snapshot_fixture_sha256':hashlib.sha256((ROOT/'tests/fixtures/retail_source_object_admission_snapshot.bin').read_bytes()).hexdigest(),'previous_query_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(args.fixture.read_bytes()).hexdigest(),'fpcw':'023F:captured snapshot FCW','mxcsr':'1F80','snapshot_eip':'0012B33E','snapshot_tick':snapshot.report['clock_tick'],'original_dynamic_controls':3792,'original_static_controls':948,'additional_controls':len(cases)-4740,'counts':dict(stats),'instructions_verified':len(verified),'source_calls':dict(calls),'protected_banks':len(protected),'boundary':'Unchanged1697C0 with explicit diagnostic rays over actual retained map/owner/WORLD/active/spatial metadata. Fog controls explicitly supply synthetic designator/material/plane metadata; original instructions and all geometry/owner/spatial inputs remain unchanged. Complete1EFB40/1EFF40 executes for callback outcomes; independent original168B40 exports their plane/surface packet. No replaced services, successful registration reconstruction, guest IO/advance or native activation. Flag20000000 location repair is outside the ordinary dispatcher.'}
 args.output.write_text(json.dumps(provenance,indent=2)+'\n');args.fixture.with_suffix('.provenance.json').write_text(json.dumps(provenance,indent=2)+'\n');print(json.dumps(provenance,indent=2),flush=True)

if __name__=='__main__':main()
