#!/usr/bin/env python3
"""Literal local PC executable inspection helpers; never modifies game files."""
from pathlib import Path
import hashlib, struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_MODE_64, CS_GRP_JUMP, CS_GRP_RET

ROOT=Path(__file__).resolve().parents[1]
VISTA=ROOT/'Halo 2 Vista installer/halo2.exe'
MCC=Path('/home/eric/.local/share/Steam/steamapps/common/Halo The Master Chief Collection/halo2/halo2.dll')
OUT=ROOT/'analysis/multiplayer/pc-executable-postprocess-20261011'

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
class PE:
 def __init__(self,path):
  self.path=Path(path);self.data=self.path.read_bytes();b=self.data
  pe=struct.unpack_from('<I',b,60)[0];n=struct.unpack_from('<H',b,pe+6)[0];osz=struct.unpack_from('<H',b,pe+20)[0];o=pe+24
  self.bits=32 if struct.unpack_from('<H',b,o)[0]==0x10b else 64
  self.base=struct.unpack_from('<I' if self.bits==32 else '<Q',b,o+(28 if self.bits==32 else 24))[0]
  self.image_size=struct.unpack_from('<I',b,o+56)[0];self.sections=[]
  for i in range(n):
   at=o+osz+40*i;vsize,va,rsize,off=struct.unpack_from('<4I',b,at+8)
   self.sections.append(dict(name=b[at:at+8].split(b'\0')[0].decode(),va=self.base+va,virtual_size=vsize,raw_size=rsize,file_offset=off))
  self.md=Cs(CS_ARCH_X86,CS_MODE_32 if self.bits==32 else CS_MODE_64);self.md.detail=True
  self.funcs=[]
  if self.bits==64:
   s=next(q for q in self.sections if q['name']=='.pdata')
   for a in range(s['file_offset'],s['file_offset']+s['virtual_size'],12):
    start,end,info=struct.unpack_from('<III',b,a)
    if start:self.funcs.append((self.base+start,self.base+end,self.base+info))
 def read(self,address,size):
  s=next(q for q in self.sections if q['va']<=address and address+size<=q['va']+q['raw_size'])
  a=s['file_offset']+address-s['va'];return self.data[a:a+size]
 def owner(self,address):return next((q for q in self.funcs if q[0]<=address<q[1]),None)
 def instruction(self,address):return next(self.md.disasm(self.read(address,16),address,count=1))
 def trace(self,entry):
  todo=[entry];rows={};calls=[]
  while todo:
   p=todo.pop()
   while p not in rows:
    i=self.instruction(p);rows[p]=i
    if i.mnemonic=='call':calls.append((p,i.op_str))
    if i.group(CS_GRP_RET):break
    if i.group(CS_GRP_JUMP):
     assert i.op_str.startswith('0x'),('indirect jump',hex(p),i.op_str)
     todo.append(int(i.op_str,16))
     if i.mnemonic=='jmp':break
    p+=i.size
  return rows,calls
 def listing(self,entry):
  rows,calls=self.trace(entry)
  return ''.join(f'{p:016x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for p,i in sorted(rows.items()))
 def raw_occurrences(self,word):
  st=0;needle=struct.pack('<I',word);out=[]
  while (st:=self.data.find(needle,st))>=0:
   s=next((s for s in self.sections if s['file_offset']<=st<s['file_offset']+s['raw_size']),None)
   out.append(dict(file_offset=st,va=s['va']+st-s['file_offset'] if s else None));st+=1
  return out
