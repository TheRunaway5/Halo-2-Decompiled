#!/usr/bin/env python3
"""Unchanged Vista fade guard, geometry-cache count and player admission owners.

All reached PC instructions execute, with no supplied function return or patch.
Player records/options are explicit inputs. Native eligibility expectations are
the separately frozen whole-original Xbox player-life corpus, not this port.
The named excluded flag moves from Xbox4000 to PC8000; other queried fields are
mapped by meaning. This is a query equivalence, not whole-bank layout parity.
Geometry preload completion tail is entered with its actual preceding branch
inputs; async cache/D3D/screenshot bodies are not executed here.
"""
import hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/gameplay-fade-original'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 b=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(b).hexdigest()==SHA
 u=machine(b);visits=0;executed=set()
 def hook(u,pc,n,_):
  nonlocal visits
  visits+=1;executed.add(pc)
  if pc==STOP:u.emu_stop()
 u.hook_add(UC_HOOK_CODE,hook)
 def call(pc,args=(),regs=(),prefix=b''):
  s=STACK+0x10000;u.mem_write(s,prefix+struct.pack('<I',STOP)+b''.join(struct.pack('<I',a&0xffffffff)for a in args));u.reg_write(UC_X86_REG_ESP,s)
  for r,v in regs:u.reg_write(r,v)
  u.emu_start(pc,STOP+1,count=10000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  return u.reg_read(UC_X86_REG_EAX)&0xffffffff
 # Base executable has no active screenshot/cubemap, before any capture.
 assert bytes(u.mem_read(0xe55630,2))==b'\0\0';assert call(0x620fbd)==0
 guards=[]
 for taking,cubemap in itertools.product([0,1,2,255],repeat=2):
  u.mem_write(0xe55630,bytes([taking,cubemap]));guards.append(dict(taking=taking,cubemap=cubemap,result=call(0x620fbd)))
 counts=[]
 for before,world,dl,needs in itertools.product([0,1,0x7fffffff,0xffffffff],[0,1],[0,1],[0,1]):
  w32(u,0xe3f5bc,before);u.mem_write(0xe3f5c4,b'\0')
  #6654D4 entry: stack has saved ESI/EDI/EBX and16 local bytes;
  #sp+E is second preload flag,sp+20 says group sbsp/ltmp.
  frame=bytearray(28);frame[0xe]=needs
  prefix=bytes(frame)+struct.pack('<II',STOP,world)
  s=STACK+0x10000;u.mem_write(s,prefix);u.reg_write(UC_X86_REG_ESP,s);u.reg_write(UC_X86_REG_EDX,dl)
  u.emu_start(0x6654d4,STOP+1,count=1000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  after=call(0x664f3c);blocking=u.mem_read(0xe3f5c4,1)[0];call(0x664f31);assert call(0x664f3c)==0
  counts.append(dict(before=before,world=world,dl=dl,needs=needs,after=after,blocking=blocking))
 native_path=ROOT/'tests/fixtures/player_lives_original.json';native=json.loads(native_path.read_text());players=[]
 w32(u,0x882d3c,OBJ);w32(u,0x8a8260,OBJ+0x2000);w32(u,OBJ+0x2044,OBJ+0x3000)
 for row,expected in zip(native['inputs']['queries'],native['versions']['u5']['results']['queries'],strict=True):
  p=row['players'][0];at=OBJ+0x3000+p['slot']*0x204
  flags=(p['flags']&~0xc000)|((p['flags']&0x4000)<<1)
  w32(u,OBJ+8+0x2a0+0x7c,row['limit']);u.mem_write(at+2,struct.pack('<H',flags));u.mem_write(at+0xbc,bytes([p['membership']&255]));w32(u,at+0x28,p['unit']);u.mem_write(at+0x190,struct.pack('<h',p['lives']))
  before=bytes(u.mem_read(at,0x204));result=call(0x46c54f,(0x9abc0000+p['slot'],))&255
  assert result==expected['allowed'] and bytes(u.mem_read(at,0x204))==before,(row,flags,result,expected)
  players.append(dict(limit=row['limit'],native=p,pc_flags=flags,result=result))
 fixture=ROOT/'tests/fixtures/vista_gameplay_fade_owners_original.json'
 fixture.write_text(json.dumps(dict(guards=guards,counts=counts,players=players),separators=(',',':'))+'\n')
 data=dict(image_sha256=SHA,cases=len(guards)+len(counts)+len(players),instructions=visits,fixture_sha256=sha(fixture),native_fixture_sha256=sha(native_path),native_image_sha256=native['versions']['u5']['image_sha256'],scope=__doc__)
 (OUT/'owners-execution.json').write_text(json.dumps(data,indent=2)+'\n')
 c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
 for pc in sorted(executed):
  if BASE<=pc<BASE+len(b):
   i=next(c.disasm(b[pc-BASE:pc-BASE+15],pc));asm.append(f'{pc:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'owners-executed.asm').write_text('\n'.join(asm)+'\n')
 # Static whole owner/caller windows are separate from executed instructions.
 ranges=[(0x44a4af,0x44a5d0),(0x45b171,0x45b17c),(0x46ad5c,0x46ad98),
  (0x46c54f,0x46c5ac),(0x47554d,0x47570e),(0x47590f,0x475d11),
  (0x620fbd,0x620fc2),(0x664f31,0x664f42),(0x6652bc,0x665504),
  (0x6736a4,0x6736de),(0x673795,0x673849),(0x6741c2,0x674227)]
 listing=[]
 for lo,hi in ranges:
  block=b[lo-BASE:hi-BASE];listing.append(f'\nSTATIC {lo:08X}..{hi:08X} sha256={hashlib.sha256(block).hexdigest()}')
  listing.extend(f'{i.address:08X}: {i.mnemonic} {i.op_str}'for i in c.disasm(block,lo))
 (OUT/'source-owners.asm').write_text('\n'.join(listing)+'\n');print(json.dumps(data))
if __name__=='__main__':main()
