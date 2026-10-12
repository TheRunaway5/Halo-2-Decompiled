#!/usr/bin/env python3
"""Original protected connection-task queries and PCR capability/tab owners.

Original decryptors execute unchanged. The harness resolves only platform
Get/SetLastError and VirtualAlloc/VirtualFree, plus security-cookie checks.
The bounded PCR registration probe stops at its real asset/layout owner; the
tab selection probe stops before the inherited widget initializer.
"""
import hashlib,json,struct,sys
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,u32,w32
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import PE,EXE,EXE_SHA
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/native-menu-services'
class Original:
 def __init__(self,e):
  self.u=machine(e.data);self.u.mem_map(0,0x1000);self.api={};self.next_alloc=0x1c00000
  self.visits=0;self.body=[];self.stops=set();self.calls=[]
  for i,imp in enumerate(e.imports()):
   a=STOP+0x100+i;w32(self.u,int(imp['iat_va'],16),a);self.api[a]=imp
  self.u.hook_add(UC_HOOK_CODE,self.hook)
 def ret(self,value=None,pop=0):
  u=self.u;sp=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(self,u,pc,n,_):
  self.visits+=1
  if pc==STOP or pc in self.stops:u.emu_stop();return
  if pc==0x6875d3:self.ret();return
  if 0x1c00000<=pc<self.next_alloc:
   raw=bytes(u.mem_read(pc,n));i=next(Cs(CS_ARCH_X86,CS_MODE_32).disasm(raw,pc))
   self.body.append(dict(address=hex(pc),bytes=raw.hex(),mnemonic=i.mnemonic,operands=i.op_str))
  if pc==0x61208e:self.calls.append(['append_child',u.reg_read(UC_X86_REG_ECX),u32(u,u.reg_read(UC_X86_REG_ESP)+4)])
  if pc not in self.api:return
  imp=self.api[pc];name=imp['name'];sp=u.reg_read(UC_X86_REG_ESP);args=[u32(u,sp+4+4*i) for i in range(4)]
  if name=='GetLastError':self.ret(0);return
  if name=='SetLastError':self.ret(0,4);return
  if name=='VirtualAlloc':
   size=(args[1]+4095)&~4095;out=args[0] or self.next_alloc;self.next_alloc=out+size
   u.mem_map(out,size);self.ret(out,16);return
  if name=='VirtualFree':self.ret(1,12);return
  raise RuntimeError(f'Unexpected external import {imp}')
 def invoke(self,entry,ecx=0,args=()):
  u=self.u;sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),STOP,*args))
  u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,ecx)
  u.emu_start(entry,STOP+1,count=3000000)
  return dict(eax=u.reg_read(UC_X86_REG_EAX),pc=u.reg_read(UC_X86_REG_EIP))
def connection_task(e,state):
 o=Original(e);w32(o.u,0x822114,state);values=[]
 for entry in (0x5ad704,0x5ad70e,0x5ad718,0x5ad722):
  result=o.invoke(entry);assert result['pc']==STOP;values.append(result['eax'])
 return dict(kind='connection_task',state=state,returned_state=values[0],blocks=values[1],gate_a=values[2],gate_b=values[3],instruction_visits=o.visits,body=o.body)
def connection_initialize(e):
 o=Original(e);before=bytes(o.u.mem_read(0x822100,0x120)).hex()
 result=o.invoke(0x5d7810,0x822100,[0x820fc4]);assert result['pc']==STOP
 after=bytes(o.u.mem_read(0x822100,0x120)).hex()
 for _ in range(8):
  result=o.invoke(0x5d7a19,0x822100);assert result['pc']==STOP
  assert bytes(o.u.mem_read(0x822100,0x120)).hex()==after
 return dict(before=before,after=after,update_iterations=8,state=u32(o.u,0x822114),slot=u32(o.u,0x822104),manager=u32(o.u,0x822108),flag=bytes(o.u.mem_read(0x82210c,1))[0],instruction_visits=o.visits,body=o.body)
def details(e):
 o=Original(e);rows=[];session=0x1a00000;w32(o.u,0x820fe8,session)
 for enabled in (0,1):
  o.u.mem_write(0x820fc0,bytes([enabled]))
  for state in list(range(12))+[0xffffffff]:
   for kind in (0,1,2,3,0xffffffff):
    w32(o.u,session+0x73a0,state);w32(o.u,session+0x1c,kind)
    result=o.invoke(0x6156b9);assert result['pc']==STOP
    rows.append(dict(kind='pcr_details',enabled=enabled,state=state,session_kind=kind,available=result['eax']))
 return rows
def default_pane(e):
 o=Original(e);screen=0x1a00000;w32(o.u,screen+0x70,16);o.stops={0x610a44}
 result=o.invoke(0x40b94b,screen,[0]);assert result['pc']==0x610a44
 offsets=[call[2]-screen for call in o.calls];assert offsets==[0xad0,0x1e40,0x31b0,0x4520,0x5894,0x6c04]
 assert u32(o.u,screen+0xa5c+0x14)==screen+0xad0
 prefix_body=o.body.copy();o.stops={0x611e23};result=o.invoke(0x620a5f,screen+0xa5c)
 assert result['pc']==0x611e23
 return dict(screen=16,pane_offsets=offsets,selected_offset=u32(o.u,screen+0xacc)-screen,body=prefix_body,instruction_visits=o.visits,boundaries=['610A44 asset/layout initialization','611E23 inherited widget initialization'])
def main():
 e=PE(EXE,EXE_SHA);init=connection_initialize(e);assert init['state']==0 and init['slot']==6 and init['manager']==0x820fc4 and init['flag']==1
 tasks=[connection_task(e,n&0xffffffff)for n in [-2,-1,*range(14),0x7fffffff,0x80000000]]
 rows=[{key:row[key]for key in ('kind','state','returned_state','blocks','gate_a','gate_b')}for row in tasks]+details(e)
 pane=default_pane(e);assert pane['selected_offset']==0xad0
 fixture=ROOT/'tests/fixtures/native_vista_menu_services_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(exe_sha256=EXE_SHA,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),initialization=init,connection_tasks=tasks,default_pane=pane,cases=len(rows),spans={'queries':e.span(0x5ad704,0x5ad72c),'shared_queries':e.span(0x5f69ba,0x5f69f4),'details':e.span(0x5b1643,0x5b16a6),'details_wrapper':e.span(0x6156b9,0x6156c8),'current_session':e.span(0x5ad736,0x5ad75c),'tab_constructor':e.span(0x620a3a,0x620a5c),'tab_initialize':e.span(0x620a5f,0x620a6e),'append_child':e.span(0x61208e,0x6120d2),'close':e.span(0x5ad6af,0x5ad6d2),'clock_freeze':e.span(0x5b3c3a,0x5b3c4e),'clock_queries':e.span(0x5b3ca0,0x5b3cd4),'network_frame':e.span(0x5b53c7,0x5b5456),'ordinary_frame':e.span(0x439c8d,0x439ca1),'lifecycle_frame':e.span(0x5ad83f,0x5ad85d),'seed_raw_clock':e.span(0x45906f,0x459087)},boundary=__doc__)
 OUT.mkdir(parents=True,exist_ok=True);(OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(cases=len(rows),fixture_sha256=report['fixture_sha256'],initial_task_state=init['state'],default_pane=pane['selected_offset'])))
if __name__=='__main__':main()
