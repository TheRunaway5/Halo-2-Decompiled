#!/usr/bin/env python3
"""Execute actual PC13 Create selection,616345 and40B8B9 unchanged.

Disconnect, local birth and617138 selected-controller/settings publication
remain explicit owner services. This does not invent a lobby factory.
"""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-leave-original'
def execute(image,q):
 u=machine(image);events=[];visits=0
 w32(u,OBJ+0x70,OBJ+0x3000);w32(u,OBJ+0x3000+0x44,OBJ+0x4000)
 u.mem_write(OBJ+0x4008,b'\1');u.mem_write(OBJ+0x149d,b'\0')
 w32(u,OBJ+0x5000,0);w32(u,0x86eee8,q['players']);w32(u,0x86eef0,q['teams'])
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x5ad6af:events.append(['disconnect',u32(u,sp+4)]);ret()
  elif pc==0x40b8b9:events.append(['clear_report']) #Actual two stores execute.
  elif pc==0x5ad8fc:events.append(['birth',q['birth']]);ret(q['birth'])
  elif pc==0x617138:events.append(['configure',u32(u,sp+4)]);ret()
 u.hook_add(UC_HOOK_CODE,hook)
 sp=STACK+0x18000;u.mem_write(sp,struct.pack('<III',STOP,OBJ+0x5100,OBJ+0x5000))
 u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(0x619d1c,STOP+1,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(input=q,events=events,players=u32(u,0x86eee8),teams=u32(u,0x86eef0),visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[execute(image,dict(birth=birth,players=p,teams=t))for birth,p,t in itertools.product((0,1,0x80),(0,1,16,0xffffffff),(0,1,16,0x80000000))]
 f=ROOT/'tests/fixtures/vista_network_browser_original.json';f.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 r=dict(source_sha256=SHA,cases=len(rows),instructions=sum(q['visits']for q in rows),fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),limits=['Full619D1C Create branch and616345 local branch execute unchanged, including complete40B8B9 count stores. Actual619F5D149D=0 and618998+8Create row construction are pinned separately.','5AD6AF disconnect,5AD8FC actual local birth and617138 selected-controller/settings publication remain mandatory owner services. Birth AL high-bit booleans are admitted. No new lobby factory is supplied by the harness.'])
 (OUT/'browser-create-original.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
