#!/usr/bin/env python3
"""Unchanged supplied Vista phase writer/request and PCR continuation owner.

The actual two network-clock results and transport delivery are controlled
external services. Whole-bank hashes are original outputs, not native fixtures.
"""
from pathlib import Path
import hashlib,json,struct,itertools
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1];N=0x7b88;OUT=ROOT/'analysis/frontend/vista-session-flow-audit/phase-owner-original'
def initial(q):
 b=bytearray((i*37+q['seed']*13+(i>>8)*17)&255 for i in range(N))
 def put(o,v,n=4):b[o:o+n]=(v&0xffffffff).to_bytes(4,'little')[:n]
 for o,v in ((0x73a0,q['state']),(0x84,q['count']),(0x4c68,q['before']),(0x4c64,0xffffffff),(0x4c60,0xfffffffe),(0x71c0,7),(0x74,7 if q['host'] else 8),(0x64,0),(8,0x13371337),(0x14,2)):
  put(o,v)
 for i in range(17):put(0x71c5+i*0x1c,(i%3==0)*q['present'],1)
 return bytes(b)
def digest(v):return hashlib.sha256(v).hexdigest()
def execute(image,q):
 u=machine(image);u.mem_write(OBJ,initial(q));w32(u,0x820fe8,OBJ);w32(u,0x820fc4,q['kind']);u.mem_write(0x820fc0,bytes([q['enabled']]));events=[];visits=0;clocks=0
 def ret(v=None,pop=0):
  s=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(_u,pc,size,_):
  nonlocal visits,clocks
  visits+=1;s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc==0x687ba9:
   dst,val,n=[u32(u,s+4+i*4) for i in range(3)];u.mem_write(dst,bytes([val&255])*n);ret(dst);return
  if pc==0x5b3ca0:
   events.append(['clock',digest(bytes(u.mem_read(OBJ,N)))]);clocks+=1
   if q['mutate']:w32(u,OBJ+0x4c64,0x55555555+clocks);u.mem_write(OBJ+0x796c,b'\x80')
   w32(u,0x91abcc,[0x12345678,0xfedcba98][clocks-1]);return
  if pc==0x5bed40:
   owner=u.reg_read(UC_X86_REG_ECX);args=[u32(u,s+4+i*4) for i in range(6)];events.append(['send',owner,*args[:5],digest(bytes(u.mem_read(args[5],args[4]))) ]);ret(0,24);return
 u.mem_write(0x91abc8,b'\x01');u.hook_add(UC_HOOK_CODE,hook)
 entry={'write':0x5c596e,'request':0x5c884a,'continue':0x5ad9e3,'host':0x5ad940}[q['op']];args=[q['requested']] if q['op'] in ('write','request') else []
 esp=STACK+0x18000;u.mem_write(esp,struct.pack('<'+'I'*(len(args)+1),STOP,*args));u.reg_write(UC_X86_REG_ESP,esp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(input=q,events=events,after=digest(bytes(u.mem_read(OBJ,N))),returned=None if q['op']=='write' else u.reg_read(UC_X86_REG_EAX)&255,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert digest(image)==SHA;rows=[]
 def add(**kw):
  q=dict(op='request',state=5,kind=4,enabled=1,host=1,count=3,present=1,before=5,requested=1,seed=23,mutate=0);q.update(kw);rows.append(execute(image,q))
 for op,state,host,present,before in itertools.product(('request','continue','host'),(*range(12),0xffffffff),(0,1),(0,0x80),(1,5)):
  add(op=op,state=state,host=host,present=present,before=before)
 for kind,enabled in itertools.product((0,1,2,3,4,5,0xffffffff),(0,1,0x80)):add(op='continue',kind=kind,enabled=enabled);add(op='host',kind=kind,enabled=enabled)
 for count,present,before,requested,mutate in itertools.product((0,1,16,17,0xffffffff),(0,1,0x80),(0,1,5),(0,1,5),(0,1)):
  add(op='write',count=count,present=present,before=before,requested=requested,mutate=mutate)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_session_phase_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),visits=sum(r['visits'] for r in rows),fixture_sha256=digest(fixture.read_bytes()),limits=['Clock values and delivery are explicit controlled source services.','Every complete7B88-byte bank and both intermediate callback-visible banks are compared.','This does not initialize the session, advance the lifecycle pump, allocate a new lobby or prove live network delivery.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
