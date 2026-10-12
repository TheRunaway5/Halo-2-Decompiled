#!/usr/bin/env python3
"""Freeze retail1EFB40/1EFF40/168B40 with actual retained collision/WORLD banks.

Diagnostic calls use literal frozen CPU state. No guest I/O or replaced source
services; this does not establish native live pose or owner-lifetime production.
"""
from pathlib import Path
import collections,hashlib,json,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words,fw
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,LOOKUP,P,D,OUT,WORLD,MATERIAL=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000300,0x20000400,0x20000500,0x20000600

def main():
 s=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
 u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter()
 def page(a):
  base=a&~4095
  if base not in pages:
   raw=s.read(base,4096);u.mem_map(base,4096);u.mem_write(base,raw);pages[base]=raw
 def read(a,n):
  for base in range(a&~4095,((a+n-1)&~4095)+4096,4096):
   if not SYNTH<=base<SYNTH+0x100000:page(base)
  return bytes(u.mem_read(a,n))
 def unmapped(g,access,a,n,value,data):
  for base in range(a&~4095,((a+n-1)&~4095)+4096,4096):page(base)
  return True
 def code(g,a,n,data):
  if a in [0x1efb40,0x1eff40,0x141590,0x168b40,0xd8ae0,0x1de6d0]:calls[f'{a:08X}']+=1
  if a in verified or SYNTH<=a<SYNTH+0x100000:return
  for sec in image.sections:
   if sec.va<=a and a+n<=sec.va+sec.rsize:
    assert read(a,n)==image.section_bytes(sec)[a-sec.va:a-sec.va+n],f'changed instruction{a:08X}'
    verified.add(a);return
  raise RuntimeError(f'unknown source service{a:08X}')
 u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code)
 def run(a,args=(),regs={}):
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,regs.get(reg,0))
  u.mem_write(STACK,packed([STOP,*args]));u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
  u.emu_start(a,STOP,count=3000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  return u.reg_read(UC_X86_REG_EAX)&255
 table=uint(s.read(0x4e3b44,4))
 def tag(d):return uint(s.read(table+(d&65535)*16+8,4))
 def block(a):return struct.unpack('<II',s.read(a,8))
 def short(a):return struct.unpack('<h',s.read(a,2))[0]&0xffffffff
 models={};rows=[];owners={};stats=collections.Counter();control_seeds={}
 def emit(k,w):rows.append(k+' '+' '.join(f'{v&0xffffffff:08X}'for v in w))
 def model(d):
  if d in models:return
  m=tag(d);cd=uint(s.read(m+12,4));c=tag(cd);mn,mp=block(m+0x58);rn,rp=block(m+0x70);cn,cp=block(c+0x1c)
  vals=[d,cd,mn]
  for i in range(mn):vals +=[short(mp+i*20+6),short(mp+i*20+16)]
  vals.append(rn)
  for i in range(rn):
   p=rp+i*16;pn,pp=block(p+8);vals +=[s.read(p+4,1)[0],pn,*[s.read(pp+j*8+5,1)[0]for j in range(pn)]]
  vals.append(cn)
  strides=[8,16,4,4,16,8,12,16]
  for ri in range(cn):
   pn,pp=block(cp+ri*12+4);vals.append(pn)
   for pi in range(pn):
    bn,bp=block(pp+pi*20+4);vals.append(bn)
    for bi in range(bn):
     b=bp+bi*68;vals.append(short(b))
     for j,stride in enumerate(strides):
      n,p=block(b+4+j*8);raw=s.read(p,n*stride)if n else b'';vals +=[n,*words(raw)]
      stats['collision_raw_bytes']+=len(raw)
  models[d]=(rn,m);emit('M',vals)
 for owner in s.report['objects']:
  h=int(owner['handle'],16);raw=s.file('object-'+owner['handle']+'.bin');u.mem_write(LOOKUP,packed([0xcdcdcdcd]*5))
  hit=run(0x1efb40,[h],{UC_X86_REG_ESI:LOOKUP});stats['lookup_cases']+=1
  if not hit:stats['lookup_misses']+=1;emit('N',[h]);continue
  lookup=words(read(LOOKUP,20));definition=tag(uint(raw));d=uint(s.read(definition+0x38,4));model(d);rn,m=models[d]
  nodes_bytes=struct.unpack_from('<h',raw,0x114)[0];assert nodes_bytes>=0 and nodes_bytes%52==0
  nodes_n=nodes_bytes//52;node_raw=s.read(lookup[4],nodes_bytes);active=s.read(lookup[3],rn)
  vals=[h,d,nodes_n,*words(node_raw),len(active),*active];emit('O',vals);owners[h]=lookup
  center=struct.unpack_from('<3f',raw,0x40);radius=struct.unpack_from('<f',raw,0x4c)[0]
  for axis in range(3):
   for sign in [-1,1]:
    origin=list(center);origin[axis]=struct.unpack('<f',struct.pack('<f',origin[axis]-sign*(radius+1)))[0]
    delta=[0.,0.,0.];delta[axis]=struct.unpack('<f',struct.pack('<f',sign*2*(radius+1)))[0]
    for flags in [1,2,5,9,13]:
     u.mem_write(P,struct.pack('<3f',*origin));u.mem_write(D,struct.pack('<3f',*delta));u.mem_write(OUT,packed([0xcdcdcdcd]*9))
     hit=run(0x1eff40,[LOOKUP,flags,P,D,OUT]);v=[h,flags,*map(fw,origin),*map(fw,delta),hit]
     if hit:
      out=read(OUT,36);packet=words(out);node,region=struct.unpack_from('<hh',out,4);material=struct.unpack_from('<h',out,30)[0]
      if flags==9 and len(control_seeds)<8:
       control_seeds.setdefault(h,(d,list(lookup),list(map(fw,origin)),list(map(fw,delta)),list(packet)))
      u.mem_write(WORLD,packed([0xcdcdcdcd]*24));run(0x168b40,regs={UC_X86_REG_EAX:WORLD,UC_X86_REG_EDX:OUT+8,UC_X86_REG_ECX:lookup[4]+node*52})
      run(0xd8ae0,regs={UC_X86_REG_ECX:h,UC_X86_REG_ESI:material,UC_X86_REG_EAX:MATERIAL})
      global_material=struct.unpack('<h',read(MATERIAL,2))[0]
      v +=[packet[2],*words(read(WORLD+0x28,16)),packet[0],packet[4],packet[5],packet[6],region,node,material,global_material,out[28],out[29]]
      stats['hits']+=1
     emit('Q',v);stats['queries']+=1
  print(f'{h:08X}',stats['queries'],flush=True)
 # Explicit mutated-data controls distinguish the crossed geometric plane
 # from the surface's low15 plane ID, which coincide in all retained banks.
 for h,(d,lookup,origin,delta,seed) in control_seeds.items():
  c=tag(uint(s.read(tag(d)+12,4)));_,regions=block(c+0x1c);region=seed[0]>>24;perm=(seed[0]>>16)&255;bsp=(seed[0]>>8)&255
  _,perms=block(regions+region*12+4);_,bsps=block(perms+perm*20+4);b=bsps+bsp*68
  plane_count,_=block(b+12);_,surfaces=block(b+44);surface=surfaces+seed[5]*8;original=read(surface,2)
  if plane_count<2:continue
  old=struct.unpack('<H',original)[0];replacement=((old&32767)+1)%plane_count
  for sign in [0,32768]:
   u.mem_write(surface,struct.pack('<H',replacement|sign));u.mem_write(LOOKUP,packed(lookup));u.mem_write(P,packed(origin));u.mem_write(D,packed(delta));u.mem_write(OUT,packed([0xcdcdcdcd]*9))
   hit=run(0x1eff40,[LOOKUP,9,P,D,OUT]);assert hit
   out=read(OUT,36);packet=words(out);node,region=struct.unpack_from('<hh',out,4);material=struct.unpack_from('<h',out,30)[0]
   u.mem_write(WORLD,packed([0xcdcdcdcd]*24));run(0x168b40,regs={UC_X86_REG_EAX:WORLD,UC_X86_REG_EDX:OUT+8,UC_X86_REG_ECX:lookup[4]+node*52})
   run(0xd8ae0,regs={UC_X86_REG_ECX:h,UC_X86_REG_ESI:material,UC_X86_REG_EAX:MATERIAL})
   glob=struct.unpack('<h',read(MATERIAL,2))[0]
   emit('X',[h,9,*origin,*delta,seed[0]>>24,(seed[0]>>16)&255,(seed[0]>>8)&255,seed[5],replacement|sign,hit,packet[2],*words(read(WORLD+40,16)),packet[0],packet[4],packet[5],packet[6],region,node,material,glob,out[28],out[29]])
   stats['crossed_plane_controls']+=1
  u.mem_write(surface,original);assert read(surface,2)==s.read(surface,2)
 protected=0
 for entry in s.report['files']:
  if entry['address_space']=='CPU0 virtual' and ('object-'in entry['file']or any(x in entry['file']for x in ['collidable','noncollidable'])):
   a=int(entry['address'],16);assert read(a,entry['size'])==s.read(a,entry['size']),entry['file'];protected+=1
 fixture=ROOT/'tests/fixtures/retail_source_object_posed_target.tsv';fixture.write_text('# Actual retained ModelCollision, WORLD and active banks; unchanged retail1EFF40/168B40 diagnostic queries.\n'+'\n'.join(rows)+'\n')
 report={'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(s.ram).hexdigest(),'snapshot_export_sha256':hashlib.sha256((s.directory/'export.json').read_bytes()).hexdigest(),'models':len(models),'owners':len(owners),'stats':dict(stats),'calls':dict(calls),'instructions_verified':len(verified),'protected_banks':protected,'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'boundary':__doc__}
 fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/posed-target-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
