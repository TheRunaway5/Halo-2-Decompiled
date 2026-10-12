#!/usr/bin/env python3
"""Actual Vista 44A82E construction and all 41 real virtual reset operations.

Heap allocation is an explicit zeroed native fixture service. Original child
constructors, vtables, reset methods, root reset and scalar CRT execute. The
named eight-byte pool allocation uses the same explicit fixture heap service;
source memory-manager readiness is not claimed.
"""
from pathlib import Path
import hashlib,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import *
from oracle_native_vista_menu_services import Original,PE,EXE,EXE_SHA,STOP,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/mainmenu-network-original/game-state'
GAME=0x1a00000

class GameState(Original):
 def __init__(self,e):
  super().__init__(e);self.heap_next=0x3000000;self.allocs=[];self.executed={};self.stage='construct'
  w32(self.u,0xe9e9e4,0)
 def hook(self,u,pc,n,unused):
  if pc==0x42ff1d:
   sp=u.reg_read(UC_X86_REG_ESP);name,flags,size=[u32(u,sp+4+i*4)for i in range(3)]
   assert (name,flags,size)==(0x7bcc38,0,8)
   pointer=self.heap_next;u.mem_map(pointer,4096);self.heap_next+=4096
   self.allocs.append(dict(pointer=pointer,size=size,pool_arguments=[name,flags,size]))
   self.ret(pointer);return
  if pc==0x687c86:
   size=u32(u,u.reg_read(UC_X86_REG_ESP)+4);pointer=self.heap_next
   pages=(size+4095)&~4095;u.mem_map(pointer,pages);self.heap_next+=pages
   self.allocs.append(dict(pointer=pointer,size=size));self.ret(pointer);return
  if self.stage=='reset' and pc<0x1800000:self.executed[pc]=bytes(u.mem_read(pc,n))
  super().hook(u,pc,n,unused)

def main():
 e=PE(EXE,EXE_SHA);o=GameState(e);r=o.invoke(0x44a82e,GAME)
 assert r['pc']==STOP and r['eax']==GAME
 assert u32(o.u,0x8d83d0)!=0
 before=[]
 for i in range(41):
  p=u32(o.u,GAME+8+i*4);allocation=next(a for a in o.allocs if a['pointer']==p)
  vtable=u32(o.u,p);reset=u32(o.u,vtable+8)
  before.append(dict(slot=i,pointer=p,size=allocation['size'],vtable=vtable,reset=reset,
    bytes=bytes(o.u.mem_read(p,allocation['size'])).hex()))
 profile=u32(o.u,GAME+0xac);counter=u32(o.u,0x8d83d0)
 profile_before=bytes(o.u.mem_read(profile,0x1c0)).hex()
 o.stage='reset';r=o.invoke(0x44b283,GAME);assert r['pc']==STOP
 after=[dict(slot=r['slot'],bytes=bytes(o.u.mem_read(r['pointer'],r['size'])).hex())for r in before]
 cases=[]
 for seed in range(256):
  root_prefix=bytes((seed*17+i*37+11)&255 for i in range(8))
  root_tail=bytes((seed*23+i*31+7)&255 for i in range(12))
  o.u.mem_write(GAME,root_prefix);o.u.mem_write(GAME+0xb0,root_tail)
  counter_before=bytes((seed*13+i*41+3)&255 for i in range(8));o.u.mem_write(counter,counter_before)
  child_inputs=[]
  for child in before:
   b=bytearray.fromhex(child['bytes'])
   for i in range(4,len(b)):b[i]=(seed*17+child['slot']*23+i*37+19)&255
   o.u.mem_write(child['pointer'],bytes(b));child_inputs.append(b.hex())
  r=o.invoke(0x44b283,GAME);assert r['pc']==STOP
  cases.append(dict(seed=seed,root_prefix_before=root_prefix.hex(),root_tail=root_tail.hex(),
   root_prefix_after=bytes(o.u.mem_read(GAME,8)).hex(),
   counter_before=counter_before.hex(),counter_after=bytes(o.u.mem_read(counter,8)).hex(),
   children_before=child_inputs,children_after=[bytes(o.u.mem_read(c['pointer'],c['size'])).hex()for c in before]))
 OUT.mkdir(parents=True,exist_ok=True);cs=Cs(CS_ARCH_X86,CS_MODE_32)
 (OUT/'executed-reset.asm').write_text('\n'.join(
  f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
  for pc,raw in sorted(o.executed.items()) for i in cs.disasm(raw,pc))+'\n')
 report=dict(source_sha256=EXE_SHA,instructions=o.visits,
  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  allocations=o.allocs,children=before,after=after,
  root=bytes(o.u.mem_read(GAME,0xbc)).hex(),profile=profile_before,
  counter=bytes(o.u.mem_read(counter,8)).hex(),limits=__doc__)
 fixture=ROOT/'tests/fixtures/vista_mainmenu_game_state_original.json'
 fixture.write_text(json.dumps(dict(constructor=before,profile=profile_before,cases=cases),separators=(',',':'))+'\n')
 report['fixture_sha256']=hashlib.sha256(fixture.read_bytes()).hexdigest();report['cases']=len(cases)
 (OUT/'execution.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(instructions=o.visits,children=[{k:r[k]for k in ('slot','size','vtable','reset')}for r in before])))
if __name__=='__main__':main()
