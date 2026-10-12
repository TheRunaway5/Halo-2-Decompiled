#!/usr/bin/env python3
"""Execute complete608690 with declared profile-catalog service results.

The original controller iterator, mask query and43E411 clear execute unchanged.
Catalog persistence/reindexing and dialog registration are explicit services.
"""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EIP,UC_X86_REG_ESP,UC_X86_REG_EAX
from oracle_vista_pcr_screen import machine,STOP,STACK,SHA,u32,w32

ROOT=Path(__file__).resolve().parents[1]
CONTROLLERS=0xd6c858
SIZE=4*0x127c

def execute(image,q):
 u=machine(image);events=[];visits=0
 before=bytearray((i*37+11)&255 for i in range(SIZE))
 for i in range(4):
  struct.pack_into('<I',before,i*0x127c,q['flags'][i])
  struct.pack_into('<I',before,i*0x127c+0x121c,q['indices'][i])
 u.mem_write(CONTROLLERS,bytes(before))
 u.mem_write(0xd718e8,b'\0');u.mem_write(0xd758e0,b'\0')
 u.mem_write(0x882427,bytes([q['pending']]))
 def ret(value=None):
  sp=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x446867:events.append(['refresh_catalog']);ret();return
  if pc==0x442a5d:
   index=u32(u,sp+4);value=q['remap'][index-11]
   events.append(['remap',index,value]);ret(value);return
  if pc==0x60758d:events.append(['mask']) #The original complete leaf executes.
  if pc==0x60e1d8:
   events.append(['dialog',*[u32(u,sp+4+i*4)for i in range(6)]]);ret();return
 u.hook_add(UC_HOOK_CODE,hook)
 sp=STACK+0x18000;w32(u,sp,STOP);u.reg_write(UC_X86_REG_ESP,sp)
 u.emu_start(0x608690,STOP+1,count=10000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 after=bytes(u.mem_read(CONTROLLERS,SIZE))
 return dict(input=q,events=events,pending=u.mem_read(0x882427,1)[0],
  changes=[[i,v]for i,v in enumerate(after)if v!=before[i]],visits=visits)

def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes()
 assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 for mask,indices,remap,pending in itertools.product(range(16),range(3),range(3),(0,1,0x80)):
  q=dict(flags=[0x80000001|(8 if mask&(1<<i)else 0)for i in range(4)],
   indices=[0xffffffff if indices==1 or(indices==2 and i%2)else 11+i for i in range(4)],
   remap=[0xffffffff if remap==1 or(remap==2 and i%2)else 23+i for i in range(4)],pending=pending)
  rows.append(execute(image,q))
 fixture=ROOT/'tests/fixtures/vista_browser_profile_refresh_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 out=ROOT/'analysis/frontend/vista-session-flow-audit/browser-back-original'
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits']for r in rows),
  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),boundaries=__doc__)
 (out/'profile-refresh-original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
if __name__=='__main__':main()
