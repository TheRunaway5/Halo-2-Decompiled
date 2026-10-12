#!/usr/bin/env python3
"""Original initialized achievement guards and decorated-soldier threshold.
The full manager constructor stops immediately before its first allocation;
only the consumed five initialized flags are claimed. Admitted dispatch calls
are recorded services, not Live submission or41-child-manager emulation.
"""
import hashlib,itertools,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-achievement-original'
class Original:
 def __init__(self,b):
  self.u=machine(b);self.u.mem_map(0,0x1000);self.visits=0;self.executed=set();self.events=[];self.mode='';self.u.hook_add(UC_HOOK_CODE,self.hook)
 def ret(self,value=None,pop=0):
  u=self.u;s=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(self,u,pc,size,_):
  self.visits+=1;self.executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x44a891 and self.mode=='constructor':u.emu_stop();return
  if pc in (0x44a66e,0x44b2e6) and self.mode=='dispatch':
   self.events.append(['children' if pc==0x44a66e else 'special',u32(u,u32(u,s+4)),u32(u,u32(u,s+4)+4)])
   self.ret(0,4);return
  if pc==0x48c8e4 and self.mode=='signin':self.events.append(['live_signin',u32(u,s+4)]);self.ret(0,4);return
  if pc==0x609674 and self.mode=='state':self.ret(OBJ+0x1000);return
 def call(self,pc,args=(),this=OBJ):
  self.u.mem_write(STACK+0x18000,struct.pack('<'+'I'*(len(args)+1),STOP,*args));self.u.reg_write(UC_X86_REG_ESP,STACK+0x18000);self.u.reg_write(UC_X86_REG_ECX,this)
  self.u.emu_start(pc,STOP+1,count=100000)
  if self.mode!='constructor':assert self.u.reg_read(UC_X86_REG_EIP)==STOP
  return self.u.reg_read(UC_X86_REG_EAX)
 def constructor(self):
  self.mode='constructor';self.u.mem_write(OBJ,b'\xa5'*0xbc);self.call(0x44a82e)
  return dict(kind='constructor',flags=bytes(self.u.mem_read(OBJ,5)).hex(),stop=hex(self.u.reg_read(UC_X86_REG_EIP)))
 def dispatch(self,flags,count):
  self.mode='dispatch';self.events=[];self.u.mem_write(OBJ,flags+bytes(0xbc-5));self.u.mem_write(OBJ+0x1000,struct.pack('<II',12,count&0xffffffff));w32(self.u,0x882d48,OBJ)
  assert self.call(0x44a619)==OBJ;self.call(0x44b553,(OBJ+0x1000,))
  assert bytes(self.u.mem_read(OBJ,5))==flags
  return dict(kind='dispatch',flags=flags.hex(),count=count,events=self.events)
 def signin(self,flags,value):
  self.mode='signin';self.events=[];self.u.mem_write(OBJ,flags+bytes(0xbc-5));w32(self.u,OBJ+0xac,OBJ+0x2000);self.call(0x44a7b5,(value,))
  return dict(kind='signin',initial=flags.hex(),value=value,flags=bytes(self.u.mem_read(OBJ,5)).hex(),events=self.events)
 def state(self,value):
  self.mode='state';w32(self.u,OBJ+0x1008,value);self.call(0x5ac4a3)
  return dict(kind='state',state=value,online=self.u.reg_read(UC_X86_REG_EAX)&255)
 def threshold(self,count,enabled):
  self.mode='threshold';self.u.mem_write(OBJ,bytes(16));self.call(0x4c1599,(8,));self.u.mem_write(OBJ+4,bytes([enabled]));self.u.mem_write(OBJ+0x1000,struct.pack('<II',12,count&0xffffffff));self.call(0x4c15be,(OBJ+0x1000,))
  return dict(kind='threshold',count=count,enabled=enabled,flags=bytes(self.u.mem_read(OBJ+4,3)).hex(),threshold=u32(self.u,OBJ+8))
def main():
 b=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(b).hexdigest()==SHA;o=Original(b);rows=[o.constructor()]
 for count in [-2147483648,-1,0,1,7,8,9,24,2147483647]:
  for online,earning in itertools.product([0,1,2,255],repeat=2):rows.append(o.dispatch(bytes([0x7e,earning,0x92,0xab,online]),count))
  for enabled in [0,1,255]:rows.append(o.threshold(count,enabled))
 for value in [0,1,2,255]:rows.append(o.signin(bytes.fromhex('fefdfcfbfa'),value))
 for value in [-1,0,1,2,3,255]:rows.append(o.state(value&0xffffffff))
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_pcr_achievement_original.jsonl';fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in rows))
 (OUT/'original.json').write_text(json.dumps(dict(image_sha256=SHA,cases=len(rows),instructions=o.visits,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),rows=rows),indent=2)+'\n')
 c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
 for pc in sorted(o.executed):
  if BASE<=pc<BASE+len(b):
   i=next(c.disasm(b[pc-BASE:pc-BASE+15],pc));asm.append(f'{pc:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'executed-original.asm').write_text('\n'.join(asm)+'\n')
 # Original constructor passes8 to the child stored at manager+9C(index37).
 asm=[]
 for at,size in [(0x44af2d,0x1b),(0x40cb70,0x4c),(0x44a619,0x10),(0x44a82e,0x69)]:
  asm.append(f'\n{at:08X}:')
  for i in c.disasm(b[at-BASE:at-BASE+size],at):asm.append(f'{i.address:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'source-owners.asm').write_text('\n'.join(asm)+'\n');print(json.dumps(dict(cases=len(rows),instructions=o.visits,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest())))
if __name__=='__main__':main()
