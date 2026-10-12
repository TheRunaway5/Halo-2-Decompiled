#!/usr/bin/env python3
"""Unchanged617400,6073AE/60758D and61A10F source screen argument."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-leave-original'
def execute(image,q):
 u=machine(image);events=[];visits=0;w32(u,0x863b40,q['cached'])
 for i in range(4):w32(u,0xd6c858+i*0x127c,q['flags'][i])
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x615ea9:events.append(['protocol',q['protocol']]);ret(q['protocol'])
  elif pc==0x4080ba:events.append(['pending',q['pending']]);ret(q['pending'])
  elif pc==0x60758d and q['op']=='suffix':events.append(['mask',q['mask']]);ret(q['mask'])
  elif pc==0x609850:
   args=[u32(u,sp+4+i*4) for i in range(3)];v=q['present13'] if args[2]==13 else q['present6'];events.append(['present',*args,v]);ret(v)
  elif pc==0x60b11e:
   d=u.reg_read(UC_X86_REG_ECX);events.append(['factory',*[u32(u,d+i*4) for i in range(8)]]);ret()
 u.hook_add(UC_HOOK_CODE,hook)
 sp=STACK+0x18000;w32(u,sp,STOP);w32(u,sp+4,q['context']);u.reg_write(UC_X86_REG_ESP,sp)
 u.reg_write(UC_X86_REG_EAX,q['screen']);u.reg_write(UC_X86_REG_ECX,q['old'])
 u.emu_start(dict(suffix=0x617400,count=0x6073ae,mask=0x60758d)[q['op']],STOP+1,count=100000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(input=q,events=events,cached=u32(u,0x863b40),returned=u.reg_read(UC_X86_REG_EAX) if q['op']!='suffix' else None,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA;rows=[]
 def add(**args):
  q=dict(op='suffix',cached=3,protocol=3,pending=0,mask=1,screen=14,old=2,context=0,present13=0,present6=0,flags=[8,0,0,0]);q.update(args);rows.append(execute(image,q))
 for context,protocol,cached in itertools.product((1,2,3,5,0xffffffff),(0,1,2,3,6,0xffffffff),(0,3,0xffffffff)):add(context=context,protocol=protocol,cached=cached)
 for old,screen,cached,pending,mask,p13,p6 in itertools.product((0,1,2),(14,),(2,3,4,5),(0,0x80),(0,1,8,15,0x80010001),(0,0x80),(0,0x80)):
  add(old=old,screen=screen,cached=cached,pending=pending,mask=mask,present13=p13,present6=p6)
 for mask,high,op in itertools.product(range(16),(0,1,7,0x80000000,0xfffffff7),('count','mask')):
  add(op=op,flags=[high|(8 if mask&(1<<i) else 0) for i in range(4)])
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_lobby_leave_return_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),limits=['Complete617400 source code and literal jump tables execute unchanged; nonzero context cache and zero-context oldclass0/1 or PC14/class2/protocol2..5 are admitted.','6073AE and60758D execute actual selected-profile flags; explicit protocol/pending/mask/screen lookup/screen factory callbacks are external. Exact eight-word descriptor is compared.','61A238/61A10F disassembly proves source constructor screen13. Native semantic mapping and native UI allocation are not claimed. Unknown suffix routes reject explicitly.'])
 (OUT/'return-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
