#!/usr/bin/env python3
"""Freeze unchanged 169510/168A10 over immutable actual owner/attachment RAM.

Whole original posed geometry executes. Frozen target outcomes are explicit
services for the native traversal test, not an additional pose/geometry proof.
"""
from pathlib import Path
import collections,hashlib,json,re,struct,sys
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
SYNTH,STACK,STOP,P,D,OUT=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000300
snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter();targets=[];pending=None

def page(address):
 base=address&~4095
 if base not in pages:
  raw=snapshot.read(base,4096);u.mem_map(base,4096);u.mem_write(base,raw);pages[base]=raw

def unmapped(g,access,address,size,value,data):
 for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(base)
 return True

def read(address,size):
 for base in range(address&~4095,((address+size-1)&~4095)+4096,4096):
  if not SYNTH<=base<SYNTH+0x100000:page(base)
 return bytes(u.mem_read(address,size))

def put(address,*values):read(address,len(values)*4);u.mem_write(address,packed(values))

def code(g,address,size,data):
 global pending
 if address in [0x169510,0x168a10,0x1efb40,0x1eff40,0x168b40]:calls[f'{address:08X}']+=1
 if address==0x1695a0:targets.append([g.reg_read(UC_X86_REG_EBX),0])
 if address==0x1eff40:
  esp=g.reg_read(UC_X86_REG_ESP);pending=uint(bytes(g.mem_read(esp+20,4)))
 if address==0x1695d7:
  assert pending is not None
  if g.reg_read(UC_X86_REG_EAX)&255:
   raw=read(pending,36);targets[-1]=[targets[-1][0],1,uint(raw,8),uint(raw)]
  pending=None
 if address in verified or SYNTH<=address<SYNTH+0x100000:return
 for section in image.sections:
  if section.va<=address and address+size<=section.va+section.rsize:
   assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'changed instruction:{address:08X}'
   verified.add(address);return
 raise RuntimeError(f'external service:{address:08X}')
u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code)
gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);ga,gl=(int(v,16)for v in gdtr.groups());read(ga,gl+1);u.reg_write(UC_X86_REG_GDTR,(0,ga,gl,0))
for reg,selector in [(UC_X86_REG_CS,8),(UC_X86_REG_SS,0x10),(UC_X86_REG_DS,0x10),(UC_X86_REG_ES,0x10),(UC_X86_REG_FS,0x20)]:u.reg_write(reg,selector)
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
fw=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
owners={};owner_lines=[]
for owner in snapshot.report['objects']:
 handle=int(owner['handle'],16);raw=snapshot.file(f"object-{owner['handle']}.bin");assert raw==snapshot.read(int(owner['address'],16),len(raw))
 # Header bytes come from the literal captured data-array record, not bodyAA.
 header=snapshot.file('object-pool-records.bin')[12*(handle&65535):12*(handle&65535)+12]
 vals=[handle,uint(raw,4),header[2],header[3],uint(raw,0xc),uint(raw,0x10),raw[0x10a],raw[0x348]if len(raw)>0x348 else 0,uint(raw,0x12c)if len(raw)>=0x130 else 0,*words(raw[0x40:0x50])]
 owners[handle]=(raw,vals);owner_lines.append('O '+' '.join(f'{x:08x}'for x in vals))
selected={h for h,(raw,_)in owners.items()if uint(raw,0x10)!=0xffffffff}
for record in json.loads((ROOT/'analysis/multiplayer-shading/dynamic-object-contact-snapshot/report.json').read_text())['differences']:
 if record['result_words'][0]==4:selected.add(record['result_words'][16])
fixture=ROOT/'tests/fixtures/retail_source_object_vector_contact.tsv';rows=[];stats=collections.Counter();examples=[]
flags_bank=[0x0481fffd,0x4481fffd,0x04800c0d,0x0483fffd,0x1481fffd,0x04dffffd]
for handle in sorted(selected):
 raw,vals=owners[handle];center=struct.unpack_from('<3f',raw,0x40);radius=struct.unpack_from('<f',raw,0x4c)[0]
 child=uint(raw,0x10)
 for axis in range(3):
  for sign in [-1,1]:
   origin=list(center);origin[axis]=f32(origin[axis]-f32(sign*f32(radius+1)));delta=[0.,0.,0.];delta[axis]=f32(sign*f32(2*f32(radius+1)))
   for fi,flags in enumerate(flags_bank):
    # Cross actual owner/attachment roots with explicit independent source
    # controls. These are diagnostic inputs, never claimed observed game calls.
    for admitted in [0,1]:
     excluded=[0xffffffff,handle,child][(axis+fi+admitted)%3]
     for maximum in [1.,.25]:
      targets.clear();pending=None;u.mem_write(P,struct.pack('<3f',*origin));u.mem_write(D,struct.pack('<3f',*delta));u.mem_write(OUT,bytes(96));put(OUT+4,fw(maximum));put(STACK,STOP,handle,admitted,flags,9,P,D,excluded,0xffffffff,OUT)
      for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
      u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.emu_start(0x169510,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
      hit=u.reg_read(UC_X86_REG_EAX)&255;out=words(bytes(u.mem_read(OUT,96)));expected=[hit]
      if hit:assert out[0]==4;expected +=[out[16],out[1],out[18]]
      inputs=[handle,admitted,flags,9,excluded,0xffffffff,fw(maximum),*map(fw,origin),*map(fw,delta)]
      target_words=[len(targets),*(word for target in targets for word in target)]
      expected +=[len(targets),*(target[0]for target in targets)]
      label=f'{handle:08X}_{axis}_{sign}_{fi}_{admitted}_{fw(maximum):08X}';rows.append(label+'\t'+' '.join(f'{v:08x}'for v in inputs)+' | '+' '.join(f'{v:08x}'for v in target_words)+' | '+' '.join(f'{v:08x}'for v in expected));stats['queries']+=1;stats['hits']+=bool(hit);stats['targets']+=len(targets)
 print(f'{handle:08X}',len(rows),flush=True)
# Low16 slot lookup ignores salt: preserve the requested salt in query output.
# Reuse actual attachment root with a different salt, without changing RAM.
for handle in sorted(selected):
 raw,vals=owners[handle];origin=struct.unpack_from('<3f',raw,0x40);delta=(0.,0.,-10.);requested=handle^0x01000000
 for admitted in [0,1]:
  targets.clear();pending=None;u.mem_write(P,struct.pack('<3f',*origin));u.mem_write(D,struct.pack('<3f',*delta));u.mem_write(OUT,bytes(96));put(OUT+4,fw(1.));put(STACK,STOP,requested,admitted,0x0481fffd,9,P,D,handle,0xffffffff,OUT)
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.emu_start(0x169510,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  hit=u.reg_read(UC_X86_REG_EAX)&255;out=words(bytes(u.mem_read(OUT,96)));expected=[hit]+([out[16],out[1],out[18]]if hit else[])+[len(targets),*(t[0]for t in targets)]
  inputs=[requested,admitted,0x0481fffd,9,handle,0xffffffff,fw(1.),*map(fw,origin),*map(fw,delta)];rows.append(f'salt_alias_{handle:08X}_{admitted}\t'+' '.join(f'{v:08x}'for v in inputs)+' | '+' '.join(f'{v:08x}'for v in [len(targets),*(w for t in targets for w in t)])+' | '+' '.join(f'{v:08x}'for v in expected));stats['queries']+=1;stats['hits']+=bool(hit);stats['targets']+=len(targets);stats['salt_alias_queries']+=1
# Strict fraction replacement: use actual single-hit target outcomes, retaining
# the original target and the entire original segment. No endpoint shortening.
equality_seeds={}
for row in rows:
 label,encoded=row.split('\t');a,b,c=encoded.split('|');inp=[int(v,16)for v in a.split()];services=[int(v,16)for v in b.split()];expected=[int(v,16)for v in c.split()]
 if expected[0] and services[0]==1 and services[2]==1 and services[3]==expected[2] and services[3]>0:
  equality_seeds.setdefault(inp[0]&65535,(inp,services))
for _,(inp,services) in sorted(equality_seeds.items()):
 fraction=services[3]
 for offset in [-1,0,1]:
  maximum_word=fraction+offset;maximum=struct.unpack('<f',struct.pack('<I',maximum_word))[0]
  targets.clear();pending=None;u.mem_write(P,packed(inp[7:10]));u.mem_write(D,packed(inp[10:13]));u.mem_write(OUT,bytes(96));put(OUT+4,maximum_word);put(STACK,STOP,inp[0],inp[1],inp[2],inp[3],P,D,inp[4],inp[5],OUT)
  for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
  u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.emu_start(0x169510,STOP,count=2000000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  hit=u.reg_read(UC_X86_REG_EAX)&255;out=words(bytes(u.mem_read(OUT,96)));assert bool(hit)==(offset==1)
  actual_services=[len(targets),*(w for t in targets for w in t)];assert actual_services==services
  expected=[hit]+([out[16],out[1],out[18]]if hit else[])+[len(targets),*(t[0]for t in targets)]
  control_inputs=inp[:];control_inputs[6]=maximum_word
  rows.append(f'exact_fraction_{inp[0]:08X}_{offset:+d}\t'+' '.join(f'{v:08x}'for v in control_inputs)+' | '+' '.join(f'{v:08x}'for v in actual_services)+' | '+' '.join(f'{v:08x}'for v in expected))
  stats['queries']+=1;stats['hits']+=bool(hit);stats['targets']+=len(targets);stats['fraction_controls']+=1
fixture.write_text('# Unchanged 169510/168A10 and full posed geometry over immutable 202-owner RAM; target outcomes are an explicit native callback boundary.\n'+'\n'.join(owner_lines+rows)+'\n')
protected_banks=0
for entry in snapshot.report['files']:
 if entry['address_space']=='CPU0 virtual' and ('object-' in entry['file'] or any(side in entry['file']for side in ['collidable','noncollidable'])):
  address=int(entry['address'],16);expected=snapshot.read(address,entry['size']);assert read(address,len(expected))==expected,entry['file'];protected_banks+=1
report={'version':1,'xbe_sha256':PIN,'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_export_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'owners':len(owners),'selected_roots':len(selected),'attachment_roots':sum(uint(owners[h][0],0x10)!=0xffffffff for h in selected),'stats':dict(stats),'calls':dict(calls),'instructions_verified':len(verified),'protected_banks':protected_banks,'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),'flags':flags_bank,'lower_flags':9,'fpcw':'023F literal captured FCW','mxcsr':'1F80','services':'Native fixture supplies original full 1EFB40/1EFF40 target hit/fraction/packed-BSP outcomes. Whole original 169510/168A10/11E5E0 executes unchanged; no original service is patched. Source geometry and 168B40 world-plane conversion remain an explicit native target callback boundary.','boundary':'Diagnostic source calls over retained live owner/header/attachment/primitive state, not original captured query scheduling or successful registration chronology. No guest I/O or advance.'};fixture.with_suffix('.provenance.json').write_text(json.dumps(report,indent=2)+'\n');(ROOT/'analysis/multiplayer-shading/object-vector-contact-snapshot-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
