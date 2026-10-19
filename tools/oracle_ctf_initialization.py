#!/usr/bin/env python3
"""Whole original CTF/Assault initialization/marker/region CPU oracle.

Original XBE code is an offline logic reference only. No Xbox map/tag/ISO or
runtime asset bank is loaded. Authored MCC marker rows and synthetic ABI inputs
are declared data; no reached function, allocator or service result is supplied.
Native implementation is neither imported nor read to produce expected results.
"""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools/reference/halo2-decompiled/tools'),str(ROOT/'tools')]
from xbe import Xbe
from functions import _Code,_trace
from mcc_content import MccCache
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_XMM4

SOURCE={
 'retail':dict(path='assets/disc/default.xbe',pin='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d',tsv='analysis/retail/functions.tsv',
  entries=(0x240160,0x243b10,0x243c00,0x19ec40),globals=(0x4e9ae8,0x4e6948,0x4e0350,0x51ec80)),
 'tu11':dict(path='assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',pin='aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',tsv='analysis/map-pack-update/functions.tsv',
  entries=(0x242270,0x245c20,0x245d10,0x1a25a0),globals=(0x4f10b8,0x4edf18,0x4e7920,0x526510)),
 'u5':dict(path='/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',pin='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',tsv='analysis/multiplayer/title-update-1.5/decompiled/functions.tsv',
  entries=(0x249460,0x24ce00,0x24cef0,0x1a8850),globals=(0x4f63b8,0x4f3218,0x4ecc20,0x52d710)),
}
E,G,S,O,P=0x4000000,0x4003000,0x4005000,0x4008000,0x4009000
M,K,STACK,STOP=0x4100000,E+0xfc,0x3008000,0x5000000
KINDS=('initialize','select','regions','query')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def digest(b):return hashlib.sha256(b).hexdigest()
def bits(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def row(typ,team,channel,flags,position=(0.,0.,0.),opaque=0xa55a55aa):
 return (struct.pack('<3fIhhhHII',*position,0x80000000,typ,team,channel,flags,opaque,opaque^0xffffffff)).hex()
def expand(m):
 raw=bytearray(bytes.fromhex(m['fill_row_hex'])*m['count'])
 for r in m['patches']:raw[r['index']*32:(r['index']+1)*32]=bytes.fromhex(r['row_hex'])
 return bytes(raw)
def encoded_map(name,rows,**extra):
 return dict(name=name,count=len(rows),fill_row_hex=bytes(32).hex(),
  patches=[dict(index=i,row_hex=r)for i,r in enumerate(rows)if r!=bytes(32).hex()],**extra)

def maps(mcc_maps):
 result=[encoded_map('empty',[])]
 dense=[]
 for team in range(9):
  for typ in range(4):
   dense.extend(row(typ,team,0,flag,(float(team*3),float(typ),float(flag-2)))for flag in(0,1,2,4,7))
   for channel in(1,2):
    dense.extend(row(typ,team,channel,flag,(float(team*3+dx),float(typ+dy),float(z)))
      for flag,dx,dy,z in((1,3,4,-2),(2,-8,6,3),(4,1,-1,0),(7,0.1,0.2,7)))
 result.append(encoded_map('all-teams-flags-and-regions',dense))
 limit=[row(1,0,0,0)for _ in range(8)]+[row(1,0,0,7,(40,30,20))]
 result.append(encoded_map('eligible-only-after-eight-query-matches',limit))
 edges=[row(t,team,c,7,(float(i),float(-i),float(i%3)))
  for i,(t,team,c)in enumerate(((-32768,-32768,-32768),(-1,-1,-1),(0,0,0),(1,0,0),(1,0,0),(1,0,1),(1,0,2),(2,8,0),(3,8,0),(3,8,1),(3,8,2),(32767,32767,32767),(1,9,0),(1,-1,0)))]
 result.append(encoded_map('signed-filter-edges',edges))
 geometric=[row(1,0,0,7,(0.25,-0.5,0.75)),row(1,0,0,7,(12,-8,2))]
 points=((0,0,0),(3,4,-2),(-6,8,4),(1e-30,-1e-30,1e-35),(1e30,-1e30,-1e30),(-0.,0.,-0.))
 for c in(1,2):geometric.extend(row(1,0,c,7,p)for p in points)
 result.append(encoded_map('finite-geometric-edge-values',geometric))
 special=[row(1,0,0,7,(0,0,0)),row(1,0,0,7,(1,1,1))]
 for channel in(1,2):
  for xyz in((float('inf'),0,0),(float('-inf'),1,2),(float('nan'),4,float('nan')),(0,float('nan'),float('-inf'))):
   special.append(row(1,0,channel,7,xyz))
 result.append(encoded_map('unordered-and-infinite-bounds',special))
 result.append(dict(name='signed16-last-valid-index',count=32767,fill_row_hex=row(99,99,99,0),
  patches=[dict(index=32766,row_hex=row(1,7,0,7,(1,2,3)))]))
 for name in('lockout','dune','coagulation','zanzibar','midship'):
  path=mcc_maps/(name+'.map');pin=sha(path);cache=MccCache(path)
  try:
   scnr=cache.tags[cache.u32(cache.index+12)];assert scnr['group']=='scnr' and scnr['offset'] is not None
   positions=cache.block(scnr['offset']+0x118,32)
   rows=[bytes(cache.data[p:p+32]).hex()for p in positions]
   result.append(encoded_map('mcc-'+name,rows,mcc_source=dict(path=str(path),sha256=pin,
       cache_version=13,scenario_datum=f"{scnr['datum']:08x}",scenario_path=scnr['name'],
       marker_block_offset='118',stride=32,rows_sha256=digest(b''.join(bytes.fromhex(r)for r in rows)))))
  finally:cache.close()
  assert sha(path)==pin
 return result

def cases(mm,blob):
 rr=random.Random(0x249460);out=[]
 def add(operation,map_index,engine=1,kind=0,**extra):
  out.append(dict(operation=operation,map=map_index,source_engine=engine,objective_kind=kind,**extra))
 # Whole initializer covers every team and every eligibility flag combination.
 for m in range(len(mm)):
  if m==6:continue
  for engine,kind in((1,0),(1,1),(1,2),(9,0),(9,1),(9,2)):
   add('initialize',m,engine,kind,before=blob(rr.randbytes(0x208)))
 for engine,kind in((0,-1),(2,3),(-1,-2147483648),(9,2147483647)):
  add('initialize',1,engine,kind,before=blob(rr.randbytes(0x208)))
 for engine in(1,9):
  for kind in(-1,0,1,2,3):
   for team,spawn,second in((0,True,True),(0,False,True),(8,False,True),(-1,True,False),(-32768,False,True),(65536,False,True)):
    add('select',1,engine,kind,team=team,spawn=spawn,second=second,second_before=0x12345678)
 for m in(0,2,3,4,5,6,7,8):
  add('select',m,1,0,team=-1,spawn=False,second=True,second_before=0x89abcdef)
 for m in(1,2,3,4,5,7,8):
  for team in(0,8):
   for kind,which in((0,(-1,-1)),(1,(0,-1)),(2,(-1,0)),(3,(0,0))):
    b=bytearray(rr.randbytes(0x208))
    for offset,index in zip((0x14,0x26),which):struct.pack_into('<h',b,offset+team*2,index)
    add('regions',m,9 if m==1 and team==8 else 1,kind,team=team,before=blob(b))
 for m in(0,1,2,3,4,5,6,7,8):
  for n in range(12 if m!=6 else 3):
   point=None if n<3 else struct.pack('<3f',*((0,0,0),(3,4,1),(-0.,1e-30,1e30))[n%3]).hex()
   add('query',m,capacity=(0,1,8)[n%3],type=(-1,0,1,3)[n%4],team=(-1,0,8,-32768)[n%4],channel=(-1,0,1,2)[(n//2)%4],
       point_hex=point,distance_bits=(0xbf800000,0,0x3f800000,0x40a00000,0x7fc00000)[n%5],
       vertical_bits=(0,0x3f800000,0x7f800000)[n%3],output_before=blob(rr.randbytes(40)))
 return out

class Original:
 def __init__(self,name,directory):
  self.spec=SOURCE[name];self.image=Xbe(ROOT/self.spec['path']);assert digest(self.image.data)==self.spec['pin']
  self.g=Uc(UC_ARCH_X86,UC_MODE_32);self.g.mem_map(0x10000,0x800000)
  for section in self.image.sections:self.g.mem_write(section.va,self.image.section_bytes(section))
  self.g.mem_map(0x4000000,0x200000);self.g.mem_map(0x3000000,0x10000);self.g.mem_map(STOP,0x1000)
  self.executed=set();self.calls=0
  self.g.hook_add(UC_HOOK_CODE,lambda g,a,n,_:self.executed.add(a))
  code=_Code(self.image,'.text');starts={int(r['address'],16)for r in csv.DictReader((ROOT/self.spec['tsv']).open(),delimiter='\t')}|set(self.spec['entries'])
  self.bodies={};self.listings=[];self.site_bytes={}
  for kind,entry in zip(KINDS,self.spec['entries']):
   trace=_trace(code,entry,starts);ins=[code.at(p)for p in sorted(trace.code)];self.bodies[kind]=set(trace.code)
   self.site_bytes.update((i.address,bytes(i.bytes))for i in ins)
   raw=b''.join(struct.pack('<I',i.address)+bytes(i.bytes)for i in ins)
   p=directory/f'{name}-{entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   self.listings.append(dict(image=name,owner=kind,entry=f'{entry:08x}',instructions=len(ins),instruction_bytes_sha256=digest(raw),file=p.name,sha256=sha(p),bytes=p.stat().st_size))
 def w(self,p,v):self.g.mem_write(p,struct.pack('<I',v&0xffffffff))
 def call(self,entry,args=(),registers=()):
  self.calls+=1;self.w(STACK,STOP)
  for i,v in enumerate(args):self.w(STACK+4+i*4,v)
  self.g.reg_write(UC_X86_REG_ESP,STACK);self.g.reg_write(UC_X86_REG_FPCW,0x27f);self.g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  for r,v in registers:self.g.reg_write(r,v&0xffffffff)
  self.g.emu_start(entry,STOP,count=5000000)
  assert self.g.reg_read(UC_X86_REG_EIP)==STOP
  return self.g.reg_read(UC_X86_REG_EAX)
 def run(self,row,markers,blobs):
  self.g.mem_write(E,b'\xa5'*0x1000);self.g.mem_write(G,b'\x5a'*0x1000);self.g.mem_write(S,b'\x73'*0x200)
  for address,value in zip(self.spec['globals'],(E,G,S,K)):self.w(address,value)
  self.w(S+0x118,len(markers)//32);self.w(S+0x11c,M)
  if markers:self.g.mem_write(M,markers)
  self.w(G+0x180,row['source_engine']);self.w(G+0x240,row['objective_kind'])
  if 'before'in row:self.g.mem_write(K,bytes.fromhex(blobs[row['before']]))
  self.g.mem_write(O,b'\x7b'*64);self.g.mem_write(P,bytes(12))
  operation=row['operation'];entry=self.spec['entries'][KINDS.index(operation)]
  before_engine=bytes(self.g.mem_read(E,0x1000));before_options=bytes(self.g.mem_read(G,0x1000));before_scenario=bytes(self.g.mem_read(S,0x200))
  if operation=='initialize':
   self.w(self.spec['globals'][3],0x12345678)
   returned=self.call(entry);assert self.u(self.spec['globals'][3])==K
   result=dict(bank_hex=bytes(self.g.mem_read(K,0x208)).hex(),defined_al=returned&255)
  elif operation=='select':
   self.w(O,row['second_before']);returned=self.call(entry,(O if row['second']else 0,),((UC_X86_REG_EAX,int(row['spawn'])),(UC_X86_REG_EDX,row['team'])))
   result=dict(returned=returned,second=self.u(O));assert bytes(self.g.mem_read(E,0x1000))==before_engine
  elif operation=='regions':
   self.call(entry,registers=((UC_X86_REG_EDI,row['team']),));result=dict(bank_hex=bytes(self.g.mem_read(K,0x208)).hex())
  else:
   self.g.mem_write(O,bytes.fromhex(blobs[row['output_before']]))
   if row['point_hex']:self.g.mem_write(P,bytes.fromhex(row['point_hex']))
   returned=self.call(entry,(P if row['point_hex']else 0,row['vertical_bits'],row['type'],row['team'],row['channel'],row['capacity'],O),((UC_X86_REG_XMM4,row['distance_bits']),))
   result=dict(returned=returned,output_hex=bytes(self.g.mem_read(O,40)).hex());assert bytes(self.g.mem_read(E,0x1000))==before_engine
  assert bytes(self.g.mem_read(E,0xfc))==before_engine[:0xfc] and bytes(self.g.mem_read(E+0x304,0xcfc))==before_engine[0x304:]
  assert bytes(self.g.mem_read(G,0x1000))==before_options and bytes(self.g.mem_read(S,0x200))==before_scenario
  assert bytes(self.g.mem_read(M,len(markers)))==markers
  return result
 def u(self,p):return struct.unpack('<I',self.g.mem_read(p,4))[0]

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--fixture',type=Path,required=True);ap.add_argument('--mcc-maps',type=Path,required=True);a=ap.parse_args()
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);assert scratch.is_dir()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 blobs={}
 def blob(raw):
  key=digest(raw);blobs[key]=bytes(raw).hex();return key
 mm=maps(a.mcc_maps);inputs=cases(mm,blob);marker_bytes=[expand(m)for m in mm]
 expected=None;versions={};listings=[]
 for name in SOURCE:
  t=Original(name,a.output.parent);results=[]
  for index,row in enumerate(inputs):
   try:result=t.run(row,marker_bytes[row['map']],blobs)
   except Exception:
    print('FIRST FAILURE',name,index,json.dumps(row),flush=True);raise
   if 'bank_hex'in result:result['bank']=blob(bytes.fromhex(result.pop('bank_hex')))
   if 'output_hex'in result:result['output']=blob(bytes.fromhex(result.pop('output_hex')))
   results.append(result)
  if expected is None:expected=results
  else:assert results==expected,name
  allowed=set().union(*t.bodies.values());assert t.executed<=allowed,(name,sorted(t.executed-allowed))
  for p,b in t.site_bytes.items():assert bytes(t.g.mem_read(p,len(b)))==b
  listings.extend(t.listings);versions[name]=dict(image_sha256=SOURCE[name]['pin'],completed_calls=t.calls,
   executed_unique_instructions=len(t.executed),whole_body_instructions=len(allowed),
   per_owner={k:dict(whole_instructions=len(v),executed_unique_instructions=len(v&t.executed))for k,v in t.bodies.items()},
   instruction_bytes_sha256=digest(b''.join(struct.pack('<I',p)+t.site_bytes[p]for p in sorted(t.executed))))
  print(name,'complete original calls',t.calls,'unique instructions',len(t.executed),'/',len(allowed),flush=True)
 fixture=dict(schema='original-ctf-initialization-v1',scope=__doc__,maps=mm,raw_blobs=blobs,inputs=inputs,expected=expected,
  images={n:s['pin']for n,s in SOURCE.items()},instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-ctf-initialization-report-v1',scope=__doc__,oracle_sha256=sha(__file__),scratch=str(scratch),
  fixture=dict(path=str(a.fixture),sha256=sha(a.fixture),bytes=a.fixture.stat().st_size),versions=versions,instruction_listings=listings,
  summary=dict(inputs=len(inputs),completed_original_calls=sum(v['completed_calls']for v in versions.values()),mcc_maps=sum('mcc_source'in m for m in mm),
   operations={kind:sum(r['operation']==kind for r in inputs)for kind in KINDS},whole_body_instruction_count_per_image=447),
  instruction_patches=[],supplied_returns=[],native_code_used_to_generate_expected=False,
  limits='Whole original returns/full208-byte state or complete output/defined return comparisons; declared raw options/markers/poisoned state. No original source virtual-pointer publication claimed by the typed native API, no engine activation, object birth, player/world update, native gameplay admission, match, graphics/audio or transport acceptance.')
 a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)
if __name__=='__main__':main()
