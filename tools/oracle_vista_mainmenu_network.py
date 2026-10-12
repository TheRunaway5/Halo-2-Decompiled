#!/usr/bin/env python3
"""Execute actual protected PC6 Network action and complete game-state reset.

Original decoder/checksum/dispatcher and the supplied DLL PBuffer getter run
unchanged. Child virtual operations, network readiness, special-UI state,
dialogs, browser factory and deferred admission are explicit external owners.
"""
from pathlib import Path
import hashlib,itertools,json,struct,sys
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import *
from oracle_native_vista_menu_services import Original,PE,EXE,EXE_SHA,STOP,u32,w32
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import DLL,DLL_SHA
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/mainmenu-network-original'
GAME=0x1a01000;CHILD=0x1a03000;VTABLE=0x1a05000;EVENT=0x1a07000;REFERENCE=EVENT+0x100
CHILD_CALL=STOP+0xf00

class Network(Original):
 def __init__(self,e,d,q):
  self.q=q;self.events=[];self.special_index=0;self.bodies={};self.checks=[]
  super().__init__(e)
  u=self.u;end=max(s['rva']+max(s['virtual_size'],s['raw_size'])for s in d.sections)
  u.mem_map(d.base,(end+4095)&~4095)
  for s in d.sections:u.mem_write(d.base+s['rva'],d.data[s['raw']:s['raw']+s['raw_size']])
  w32(u,0x10794f60,0) # Explicit absent optional supplied-DLL logger.
  pbuffer=EVENT+0x200
  w32(u,pbuffer+4,1);w32(u,pbuffer+8,0xdeadc0de);w32(u,pbuffer+12,pbuffer+0x100)
  u.mem_write(pbuffer+0x100,bytes([q['byte']]))
  w32(u,0x879e78,pbuffer if q['pbuffer']=='valid' else 0)
  w32(u,REFERENCE,EVENT);w32(u,EVENT+4,q['controller'])
  w32(u,0xd712d0,q['signin_context'])
  u.mem_write(GAME,bytes([0x83,0x84,0x85,0x86])+bytes(0xa8))
  u.mem_write(CHILD_CALL,b'\xc3');w32(u,VTABLE+8,CHILD_CALL)
  for i in range(41):
   p=CHILD+i*0x20;w32(u,GAME+8+i*4,p);w32(u,p,VTABLE)
   u.mem_write(p+4,bytes([0x80+i,0xf0-i,0x71,0x72]))
  for i,flags in enumerate(q['flags']):w32(u,0xd6c858+i*0x127c,flags)
 def args(self,n):
  sp=self.u.reg_read(UC_X86_REG_ESP)
  return [u32(self.u,sp+4+4*i)for i in range(n)]
 def hook(self,u,pc,n,unused):
  if pc==0x422fe6:self.logical=0x400000+u32(u,u.reg_read(UC_X86_REG_EBP)-0x8c)
  if pc==0x42384b:
   b=u.reg_read(UC_X86_REG_EBP);want=u.mem_read(b-0x82,1)[0];got=u.mem_read(b-0x15,1)[0]
   self.checks.append([want,got]);assert want==got
   self.bodies[self.logical]=(u32(u,b-0x68),bytes(u.mem_read(u32(u,b-0x68),u32(u,b-0x64))))
  if pc in (0x42385a,0x41e153):raise RuntimeError('Original protection failure')
  if pc in self.api and self.api[pc]['name'] in ('EnterCriticalSection','LeaveCriticalSection'):
   self.ret(0,4);return
  if pc==0x40e898:
   args=self.args(3);self.events.append(['pbuffer',args[1],u.mem_read(args[2],1)[0]])
   u.reg_write(UC_X86_REG_EIP,0x10569b60);return
  if pc==0x1056a6d1:
   args=self.args(3);self.events.append(['pbuffer_return',u.reg_read(UC_X86_REG_EAX),u.mem_read(args[2],1)[0]])
  if pc==0x1020b598:self.ret();return # Supplied-DLL security cookie.
  if pc==0x44a619:self.events.append(['game_state']);self.ret(GAME);return
  if pc==0x44b283:self.events.append(['reset']) # Actual complete body executes.
  if pc==CHILD_CALL:
   child=(u.reg_read(UC_X86_REG_ECX)-CHILD)//0x20
   self.events.append(['child_reset',child,*self.args(1)]);self.ret(0,4);return
  if pc==0x5ac10b:self.events.append(['network_ready',self.q['ready']]);self.ret(self.q['ready']);return
  if pc==0x609ed8:
   value=self.q['special'][self.special_index];self.special_index+=1
   self.events.append(['special',value]);self.ret(value);return
  if pc==0x60758d:self.events.append(['controller_mask']) # Actual complete body.
  if pc==0x609674:self.events.append(['ui_context',u32(u,0xd712d0)]) # Actual inline-bank leaf.
  if pc==0x60e1d8:self.events.append(['dialog',*self.args(6)]);self.ret();return
  if pc==0x61a8e3:self.events.append(['admission',u.reg_read(UC_X86_REG_ECX),*self.args(2)]);self.ret(0,8);return
  if pc==0x61a238:
   p=self.args(1)[0];self.events.append(['factory',*[u32(u,p+i*4)for i in range(8)]]);self.ret();return
  super().hook(u,pc,n,unused)

def execute(e,d,q):
 o=Network(e,d,q);r=o.invoke(0x40a978,0x1a00000,[REFERENCE]);assert r['pc']==STOP
 children=[bytes(o.u.mem_read(CHILD+i*0x20+4,4)).hex()for i in range(41)]
 return dict(input=q,events=o.events,root=bytes(o.u.mem_read(GAME,4)).hex(),
  children=children,checks=o.checks,instructions=o.visits),o

def main():
 e=PE(EXE,EXE_SHA);d=PE(DLL,DLL_SHA);rows=[];bodies={}
 for byte,pbuffer,ready,special,context,controller in itertools.product(
  (0,1,0x80),('valid','null'),(0,1,0x80),([0,0],[0,1],[0,0x80],[1,0],[0x80,0]),
  (0,1,2,0xffffffff),(0,3,16,31,32,0xffffffff)):
  q=dict(byte=byte,pbuffer=pbuffer,ready=ready,special=special,signin_context=context,
   controller=controller,flags=[8,0,8,0])
  row,o=execute(e,d,q);rows.append(row);bodies.update(o.bodies)
 OUT.mkdir(parents=True,exist_ok=True)
 fixture=ROOT/'tests/fixtures/vista_mainmenu_network_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 cs=Cs(CS_ARCH_X86,CS_MODE_32)
 for logical,(allocated,body)in bodies.items():
  (OUT/f'{logical:08x}.bin').write_bytes(body)
  (OUT/f'{logical:08x}.asm').write_text('\n'.join(
   f'{i.address:08x} [logical {logical+i.address-allocated:08x}] {i.bytes.hex()} {i.mnemonic} {i.op_str}'
   for i in cs.disasm(body,allocated))+'\n')
 report=dict(source_sha256=EXE_SHA,dll_sha256=DLL_SHA,cases=len(rows),
  instructions=sum(r['instructions']for r in rows),
  checksum_comparisons=sum(len(r['checks'])for r in rows),
  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),limits=__doc__)
 (OUT/'execution.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
