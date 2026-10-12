#!/usr/bin/env python3
"""Execute original47156A end guard and45B05D event constructor unchanged.

Rank, phase application, replication and auxiliary game-over publication are
explicit supplied services; their bodies and side effects are not this proof.
The actual engine query45B15E, mode449F8E, game-over44999C, C48 marker guard/store
and event bytes execute.45DC77 is an observed service boundary here; the
separate event oracle executes its body. No executable bytes are patched.
"""
import hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/announcer-events-original'
WORLD=OBJ;ENGINE=OBJ+0x3000
def main():
 b=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(b).hexdigest()==SHA
 u=machine(b);executed=set();instructions=0;calls=[];events=[]
 def ret(value=0):
  s=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4)
 def hook(_,pc,n,unused):
  nonlocal instructions
  instructions+=1;executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc in (0x5b65b7,0x4709e0,0x470a6f,0x471157,0x46de37):
   calls.append([pc,int(u.mem_read(ENGINE+0xc48,1)[0])]);ret(0xffffffff if pc==0x4709e0 else 0);return
  if pc==0x45dc77:
   events.append(dict(bytes=bytes(u.mem_read(u32(u,s+4),34)).hex(),marker=int(u.mem_read(ENGINE+0xc48,1)[0])));ret();return
  if pc==0x466b5d:ret(0);return # Auxiliary network/UI game-over publication remains external.
 u.hook_add(UC_HOOK_CODE,hook);w32(u,0x882d3c,WORLD);w32(u,0x8bf8f8,ENGINE)
 # Declared selected-engine bank, queried by the actual45B15E body.
 w32(u,0x8d8548,0);w32(u,0x8d8550,OBJ+0x6000)
 rows=[]
 for present,mode,over,published,phase,pending in itertools.product([0,1],[0,1,2,3,4,5,255],[0,1,2,255],[0,1,2,255],[0,1,2,3],[0,1,2]):
  calls.clear();events.clear();u.mem_write(WORLD+0xc,bytes([mode]));u.mem_write(WORLD+0x1198,bytes([over]))
  w32(u,ENGINE+0xc54,2 if present else 0);u.mem_write(ENGINE+0xc48,bytes([published]))
  u.mem_write(ENGINE+0x6c,struct.pack('<h',phase));w32(u,ENGINE+0xc44,pending);w32(u,ENGINE+0xc40,10)
  s=STACK+0x10000;w32(u,s,STOP);u.reg_write(UC_X86_REG_ESP,s);u.emu_start(0x47156a,STOP+1,count=10000)
  assert u.reg_read(UC_X86_REG_EIP)==STOP
  rows.append(dict(engine=present,mode=mode,over=over,published=published,phase=phase,pending=pending,
      marker=int(u.mem_read(ENGINE+0xc48,1)[0]),events=list(events),services=list(calls)))
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_announcer_publication_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 data=dict(image_sha256=SHA,cases=len(rows),instructions=instructions,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),scope=__doc__)
 (OUT/'publication-execution.json').write_text(json.dumps(data,indent=2)+'\n')
 c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
 for pc in sorted(executed):
  if BASE<=pc<BASE+len(b):i=next(c.disasm(b[pc-BASE:pc-BASE+15],pc));asm.append(f'{pc:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'publication-executed-original.asm').write_text('\n'.join(asm)+'\n');print(json.dumps(data))
 # These caller/activation windows are static executable evidence, deliberately
 # separate from the instruction coverage above. No claim they all executed.
 caller=[]
 for start,end in [(0x44a4af,0x44a5c2),(0x47590f,0x475932),(0x45339b,0x45343f),
     (0x5ae0cb,0x5ae151),(0x46a29e,0x46a2bc),(0x510c2b,0x510c75),
     (0x4883b6,0x488405),(0x4d0e62,0x4d0e75)]:
  caller.append(f'; Static original caller window {start:08X}..{end:08X}; not execution coverage')
  for i in c.disasm(b[start-BASE:end-BASE],start):caller.append(f'{i.address:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'static-callers-original.asm').write_text('\n'.join(caller)+'\n')
if __name__=='__main__':main()
