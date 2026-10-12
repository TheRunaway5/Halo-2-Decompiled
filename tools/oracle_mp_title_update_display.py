#!/usr/bin/env python3
"""Untouched build_number callback/marker replacement in owned 1.1 and u5.

Raw source images and declared text/ABI buffers only. No attached annotation
scripts, semantic labels, formatted returns or game-ready state are supplied.
"""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
from functions import _Code,_trace
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
OUT=ROOT/'analysis/multiplayer/integration/inventory-source/title-update-display'
NEW=Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
OLD=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
PINS={str(OLD):'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377',str(NEW):'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'}
CONFIG=[dict(id='owned_1_1',path=OLD,callback=0x22f94f,resolver=0x22f40e,formatter=0x164be0,record=0x4778b0,table=0x4776e8),
 dict(id='authenticated_u5',path=NEW,callback=0x234232,resolver=0x233cf1,formatter=0x165fc0,record=0x47cae0,table=0x47c918)]
BUFFER,STACK,STOP=0x1001000,0x200d000,0x3000000
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def save(p,b):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==b,p
 else:p.write_bytes(b)
def encoded(d):return (json.dumps(d,sort_keys=True,indent=2)+'\n').encode()
def wide(x,p):
 raw=x.read(p,256);end=next(i for i in range(0,len(raw),2)if raw[i:i+2]==b'\0\0');return raw[:end].decode('utf-16le')
def execute(c,out):
 x=Xbe(c['path']);assert sha(c['path'])==PINS[str(c['path'])];code=_Code(x,'.text')
 key,marker,callback=struct.unpack('<III',x.read(c['record'],12))
 assert callback==c['callback'] and marker==0xe415 and wide(x,key)=='build_number'
 callback_fn=_trace(code,c['callback'],{c['callback'],c['formatter'],c['resolver']})
 ins=[code.at(p)for p in sorted(callback_fn.code)]
 literal=ins[2].operands[0].imm;format_pointer=ins[3].operands[0].imm
 assert ins[2].mnemonic==ins[3].mnemonic=='push'and wide(x,format_pointer)=='%hs'
 constant=x.read(literal,4);assert constant[-1]==0
 cases=[];all_executed=set()
 for route in('direct_callback','marker_resolver'):
  g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
  for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
  for p in(0x1000000,0x2000000,STOP):g.mem_map(p,0x10000)
  initial=bytearray(512)
  if route=='marker_resolver':initial[:2]=struct.pack('<H',marker)
  g.mem_write(BUFFER,bytes(initial))
  args=(marker,BUFFER)if route=='direct_callback'else(BUFFER,256)
  g.mem_write(STACK,struct.pack('<III',STOP,*args))
  g.reg_write(UC_X86_REG_ESP,STACK);g.reg_write(UC_X86_REG_FPCW,0x027f);g.reg_write(UC_X86_REG_MXCSR,0x1f80)
  executed=[];reads=[];stores=[];order=0
  def hook(_g,p,n,u):
   nonlocal order
   executed.append(p);order+=1
  def read(_g,k,p,n,v,u):
   nonlocal order
   reads.append(dict(order=order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,raw_hex=bytes(g.mem_read(p,n)).hex()));order+=1
  def write(_g,k,p,n,v,u):
   nonlocal order
   stores.append(dict(order=order,instruction=f'{g.reg_read(UC_X86_REG_EIP):08x}',address=f'{p:08x}',size=n,before_hex=bytes(g.mem_read(p,n)).hex(),written_hex=(v&((1<<(8*n))-1)).to_bytes(n,'little').hex()));order+=1
  g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_READ,read);g.hook_add(UC_HOOK_MEM_WRITE,write)
  entry=c['callback']if route=='direct_callback'else c['resolver']
  g.emu_start(entry,STOP,count=100000);assert g.reg_read(UC_X86_REG_EIP)==STOP
  raw=bytes(g.mem_read(BUFFER,512));end=next(i for i in range(0,512,2)if raw[i:i+2]==b'\0\0')
  assert raw[:end].decode('utf-16le')==constant[:-1].decode('ascii')
  for p in set(executed):
   i=code.at(p);assert i.bytes==x.read(p,len(i.bytes))==bytes(g.mem_read(p,len(i.bytes)))
  all_executed.update(executed)
  cases.append(dict(route=route,entry=f'{entry:08x}',stack_arguments=[f'{a:08x}'for a in args],
   initial512_hex=bytes(initial).hex(),output512_hex=raw.hex(),actual_text=raw[:end].decode('utf-16le'),
   returned_eax=f'{g.reg_read(UC_X86_REG_EAX):08x}',return_esp_delta=g.reg_read(UC_X86_REG_ESP)-STACK,
   instructions=[f'{p:08x}'for p in executed],reads=reads,stores=stores,services=[]))
 listings=[]
 for label,addresses in [('executed',all_executed)]+[(f'{e:08x}',_trace(code,e,{c['callback'],c['resolver'],c['formatter']}).code)for e in(c['callback'],c['resolver'],c['formatter'])]:
  ins=[code.at(p)for p in sorted(addresses)];path=out/'instructions'/f'{c["id"]}-{label}.asm'
  for i in ins:assert i.bytes==x.read(i.address,len(i.bytes))
  save(path,''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins).encode());listings.append(dict(file=str(path.relative_to(out)),sha256=sha(path),rows=len(ins)))
 assert sha(c['path'])==PINS[str(c['path'])]
 return dict(id=c['id'],source_path=str(c['path']),source_sha256=sha(c['path']),
  record=dict(address=f'{c["record"]:08x}',rawc_hex=x.read(c['record'],12).hex(),key_pointer=f'{key:08x}',key_text=wide(x,key),marker=f'{marker:04x}',callback=f'{callback:08x}',index=(c['record']-c['table'])//12),
  table=dict(pointer=f'{c["table"]:08x}',raw624_hex=x.read(c['table'],0x624).hex(),stride=12,count=131),
  literal=dict(pointer=f'{literal:08x}',raw4_hex=constant.hex(),text=constant[:-1].decode('ascii')),
  format=dict(pointer=f'{format_pointer:08x}',raw8_hex=x.read(format_pointer,8).hex(),text=wide(x,format_pointer)),
  cases=cases,listings=listings)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,default=OUT/'report.json');a=p.parse_args()
 own=sha(__file__);cases=[execute(c,a.output.parent)for c in CONFIG];assert sha(__file__)==own
 report=dict(schema=1,oracle_sha256=own,source_pins=PINS,images=cases,
  authority='Unchanged raw executable instructions and original callback table/string bytes. No attached annotation name/comment is used as a behavioral claim.',
  formula='Callback passes original ASCII literal through original wide formatter %hs; no certificate/version-field arithmetic generates the display text.',
  supplied_inputs=['Zero-filled 512-byte text output; resolver input is one declared source marker E415 followed by NUL, capacity256.',
   'Separate explicit ABI stack/raw address space and original section bytes/zero virtual tails. No full XAPI/CRT/game initializer is claimed.'],
  boundaries=['The callback and complete marker replacement execute without stubs or supplied service/format results.',
   'This proves original build_number text production, not a complete title-screen/menu rendering session, authored widget delivery, latest-ever patch or other gameplay equivalence.',
   'RSA/certificate/section-authentication ownership is separate root evidence. Neither certificate803 nor archive/u5 naming is the numerical-display authority.'],
  instruction_patches=[],production_edits=[])
 save(a.output,encoded(report));print(json.dumps(dict(output=str(a.output),sha256=sha(a.output),texts=[c['literal']['text']for c in cases],cases=sum(len(c['cases'])for c in cases))))
if __name__=='__main__':main()
