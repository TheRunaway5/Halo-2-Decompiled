#!/usr/bin/env python3
"""Unchanged complete5B1D1B/5B1C39/5AD87C/5C8977 local bank oracle."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-leave-original'
SIZE=0x7b88
def initial(q):
 b=bytearray((i*37+q['seed']*13)&255 for i in range(SIZE))
 for at,key in ((0x73a0,'state'),(0x74,'local'),(0x71c0,'host'),(0x4c68,'phase'),(0x4c60,'revision')):
  struct.pack_into('<I',b,at,q[key]&0xffffffff)
 return b
def execute(image,q):
 u=machine(image);before=initial(q);u.mem_write(OBJ,bytes(before));w32(u,0x820fe8,OBJ)
 events=[];visits=0
 def ret(v):
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x5ad736:
   events.append(['current',q['current']]);w32(u,u32(u,sp+4),OBJ);ret(q['current'])
  elif pc==0x6875d3:ret(u.reg_read(UC_X86_REG_EAX))
  elif pc in (0x687ba9,0x5bed40):raise RuntimeError('remote transport branch reached')
 u.hook_add(UC_HOOK_CODE,hook)
 sp=STACK+0x18000;u.mem_write(sp,struct.pack('<II',STOP,q['value']))
 u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(0x5b1d1b,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 after=bytes(u.mem_read(OBJ,SIZE))
 return dict(input=q,events=events,returned=u.reg_read(UC_X86_REG_EAX)&255,before=hashlib.sha256(before).hexdigest(),after=hashlib.sha256(after).hexdigest(),visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 for state,current,host,phase,value,revision in itertools.product((*range(12),0xffffffff),(0,0x80),(0,1),(0,1,2,0xffffffff),(0,1,2,0x80000000,0xffffffff),(0,0xffffffff)):
  if state in (3,4) and current and host==0 and phase==1:continue
  q=dict(seed=len(rows)%17,state=state,current=current,local=0,host=host,phase=phase,value=value,revision=revision)
  rows.append(execute(image,q))
 fixture=ROOT/'tests/fixtures/vista_browser_preflight_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 from capstone import Cs,CS_ARCH_X86,CS_MODE_32
 cs=Cs(CS_ARCH_X86,CS_MODE_32)
 for name,a,z in (('browser-create-preflight-wrapper',0x5b1d1b,0x5b1d4f),('browser-create-preflight-writer',0x5c8977,0x5c8aa7)):
  raw=image[a-0x400000:z-0x400000];(OUT/(name+'.bin')).write_bytes(raw)
  (OUT/(name+'.asm')).write_text('\n'.join(f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in cs.disasm(raw,a))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),limits=['Complete5B1D1B,5B1C39,5AD87C and local5C8977 execute unchanged; entire7B88 source bank compared.','5AD736 resolves the same actual owner/current flag on both calls; cookie return explicit.','Remote3/4 transport cases rejected by native helper before mutation; no network/device parity claim.'])
 (OUT/'browser-preflight-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
