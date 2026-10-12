#!/usr/bin/env python3
"""Execute unchanged retail1697C0 against immutable restored raw owner/pool RAM.

Queries are explicitly diagnostic rays through retained owner spheres, not
captured original game calls. No guest I/O, registration reconstruction or
posed-target/math service substitution is performed.
"""
from pathlib import Path
import collections,hashlib,json,re,struct,sys,time
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_UNMAPPED
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from differential_source_object_admission_snapshot import Snapshot,PIN,uint,packed,words
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
SYNTH,STACK,STOP,ORIGIN,RAY,OUT=0x20000000,0x200fe000,0x200ff000,0x20000100,0x20000200,0x20000300
snapshot=Snapshot(ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008');image=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(image.data).hexdigest()==PIN
u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(SYNTH,0x100000);pages={};verified=set();calls=collections.Counter();target_calls=[]
def page(address):
 p=address&~4095
 if p not in pages:
  raw=snapshot.read(p,4096);u.mem_map(p,4096);u.mem_write(p,raw);pages[p]=raw

def unmapped(g,access,address,size,value,data):
 try:
  for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):page(p)
 except Exception as error:
  raise RuntimeError(f"unmapped {address:08X} size{size} instruction{g.reg_read(UC_X86_REG_EIP):08X}") from error
 return True

def code(g,address,size,data):
 if address in [0x1697c0,0x169510,0x168a10,0x1efb40,0x1eff40,0x1efbd0,0x1efca0,0x168b40]:calls[f'{address:08X}']+=1
 if address==0x169510:
  esp=g.reg_read(UC_X86_REG_ESP);target_calls.append([uint(bytes(g.mem_read(esp+4,4))),uint(bytes(g.mem_read(esp+8,4)))&255])
 if address in verified or SYNTH<=address<SYNTH+0x100000:return
 for section in image.sections:
  if section.va<=address and address+size<=section.va+section.rsize:
   assert bytes(g.mem_read(address,size))==image.section_bytes(section)[address-section.va:address-section.va+size],f'original instruction changed:{address:08X}'
   verified.add(address);return
 raise RuntimeError(f'uncaptured external instruction service:{address:08X}')
u.hook_add(UC_HOOK_MEM_UNMAPPED,unmapped);u.hook_add(UC_HOOK_CODE,code)
def read(address,size):
 for p in range(address&~4095,((address+size-1)&~4095)+4096,4096):
  if not SYNTH<=p<SYNTH+0x100000:page(p)
 return bytes(u.mem_read(address,size))
def put(address,*w):read(address,len(w)*4);u.mem_write(address,packed(w))
def query(flags,p,d,excluded):
 target_calls.clear();u.mem_write(ORIGIN,struct.pack('<3f',*p));u.mem_write(RAY,struct.pack('<3f',*d));u.mem_write(OUT,bytes(96));put(STACK,STOP,flags,ORIGIN,RAY,excluded,0xffffffff,OUT)
 for reg in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(reg,0)
 u.reg_write(UC_X86_REG_ESP,STACK);u.reg_write(UC_X86_REG_EFLAGS,2);u.reg_write(UC_X86_REG_FPCW,0x023f);u.reg_write(UC_X86_REG_MXCSR,0x1f80)
 try:u.emu_start(0x1697c0,STOP,count=2000000)
 except Exception as error:raise RuntimeError(f"query flags{flags:08X} owner{excluded:08X} pc{u.reg_read(UC_X86_REG_EIP):08X} esp{u.reg_read(UC_X86_REG_ESP):08X} origin{p} delta{d} pages{len(pages)}") from error
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 return u.reg_read(UC_X86_REG_EAX)&255,words(bytes(u.mem_read(OUT,96))),list(target_calls)
f32=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
bits=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
gdtr=re.search(r'GDT=\s*([0-9a-fA-F]+)\s+([0-9a-fA-F]+)',snapshot.report['registers_before']);gdt_address,gdt_limit=(int(x,16)for x in gdtr.groups());read(gdt_address,gdt_limit+1);u.reg_write(UC_X86_REG_GDTR,(0,gdt_address,gdt_limit,0));u.reg_write(UC_X86_REG_CS,8);u.reg_write(UC_X86_REG_SS,0x10);u.reg_write(UC_X86_REG_DS,0x10);u.reg_write(UC_X86_REG_ES,0x10);u.reg_write(UC_X86_REG_FS,0x20) # Literal captured FS descriptor, needed by source MOPP SEH constructor.
OUTDIR=ROOT/'analysis/multiplayer-shading/dynamic-object-contact-snapshot';OUTDIR.mkdir(exist_ok=True);began=time.monotonic();counts=collections.Counter();differences=[];records=[]
with (OUTDIR/'queries.jsonl').open('w') as output:
 for oi,owner in enumerate(snapshot.report['objects']):
  if not int(owner['flags'],16)&512:continue
  sphere=[struct.unpack('<f',struct.pack('<I',int(w,16)))[0]for w in owner['spatial_sphere_words']]
  center,radius=sphere[:3],sphere[3]
  for axis in range(3):
   for sign in [-1,1]:
    p=list(center);p[axis]=f32(p[axis]-f32(sign*f32(radius+1)));d=[0.,0.,0.];d[axis]=f32(sign*f32(2*f32(radius+1)))
    static=query(0x04800005,p,d,0xffffffff)
    for flags in [0x4480000d,0x04800c0d]:
     for excluded in [0xffffffff,int(owner['handle'],16)]:
      hit,raw,targets=query(flags,p,d,excluded);kind=raw[0] if hit else 0;counts[f'{flags:08X}:type{kind}']+=1
      record={'sphere_owner':owner['handle'],'axis':axis,'sign':sign,'flags':f'{flags:08X}','excluded':f'{excluded:08X}','origin_words':list(map(bits,p)),'ray_words':list(map(bits,d)),'hit':hit,'result_words':raw,'target_calls':targets,'static_hit':static[0],'static_words':static[1]};output.write(json.dumps(record,separators=(',',':'))+'\n');records.append(record)
      if kind==4 or hit!=static[0] or (hit and raw[1]!=static[1][1]):differences.append({k:v for k,v in record.items() if k!='static_words'})
  print(oi,owner['handle'],'cases',sum(counts.values()),'elapsed',round(time.monotonic()-began,2),flush=True)
# Owner/spatial pool state must remain literal. Query stamps/scratch are allowed.
protected=[]
for entry in snapshot.report['files']:
 if entry['address_space']=='CPU0 virtual' and ('object-' in entry['file'] or any(side in entry['file']for side in ['collidable','noncollidable'])):
  address=int(entry['address'],16);expected=snapshot.read(address,entry['size']);assert read(address,len(expected))==expected,entry['file'];protected.append(entry['file'])
report={'version':1,'source_xbe_sha256':PIN,'snapshot_ram_sha256':hashlib.sha256(snapshot.ram).hexdigest(),'snapshot_export_sha256':hashlib.sha256((snapshot.directory/'export.json').read_bytes()).hexdigest(),'snapshot_tick':snapshot.report['clock_tick'],'snapshot_eip':'0012B33E','fpcw':'023F:literal snapshot FCW','mxcsr':'1F80','cases':len(records),'counts':dict(counts),'calls':dict(calls),'instructions_verified':len(verified),'protected_banks':len(protected),'differences':differences,'boundary':'Explicit diagnostic rays through actual cached primitive spheres; original1697C0/169510/1EFF40 execute with literal owner/pose/active-permutation/spatial/tag banks. No source call-phase or runtime query schedule claim; no guest I/O/advancement; no service replacement.'}
report['query_sha256']=hashlib.sha256((OUTDIR/'queries.jsonl').read_bytes()).hexdigest();(OUTDIR/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ['cases','counts','calls','instructions_verified','protected_banks']}),flush=True)
