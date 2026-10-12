#!/usr/bin/env python3
"""Unchanged lobby guards/settings setter and complete5B11E2 argument join."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-input-original'
SIZE=0x7b88
def initial(q):
 b=bytearray((i*37+q['seed']*13)&255 for i in range(SIZE))
 for at,key in ((0x73a0,'state'),(0x74,'local'),(0x71c0,'host'),(0x4c68,'phase'),(0x84,'count'),(0x4c60,'revision')):
  struct.pack_into('<I',b,at,q[key]&0xffffffff)
 b[0x73a4]=q['closing'];b[0x4c74]=q['local_flag'];return b
def execute(image,q):
 u=machine(image);before=initial(q);u.mem_write(OBJ,bytes(before));w32(u,0x820fe8,OBJ);u.mem_write(0x820fc0,bytes([q['enabled']]))
 for i in range(4):
  w32(u,0xd6c858+i*0x127c,q['controller_flags']);u.mem_write(0x91a628+i*0xb8,bytes([q['present']])+struct.pack('<II',0x12345678+i,0xfedcba98-i))
 events=[];visits=0
 def ret(v=0,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,v&0xffffffff);u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x5ad736:w32(u,u32(u,sp+4),OBJ);ret(q['current'])
  elif pc==0x6875d3:ret(u.reg_read(UC_X86_REG_EAX))
  elif pc==0x5c96fb:
   a=[u32(u,sp+4+i*4) for i in range(5)];assert a[4]==0x91a629+q['user']*0xb8
   events.append(['write',a[0]&255,a[1],a[2]&255,a[3],u32(u,a[4]),u32(u,a[4]+4)]);ret(q['write'],20)
 u.hook_add(UC_HOOK_CODE,hook)
 entry=dict(event=0x5ad686,ready=0x5b1c39,controller=0x606b50,count=0x615285,settings=0x615219,wrapper=0x5b11e2)[q['op']]
 args=[q['controller']] if q['op']=='controller' else [q['value']] if q['op']=='settings' else [q['running'],q['stage']&0xffffffff,q['reason'],q['user'],q['flag']] if q['op']=='wrapper' else []
 sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 after=bytes(u.mem_read(OBJ,SIZE));returned=u.reg_read(UC_X86_REG_EAX)
 if q['op']!='count':returned &=255
 return dict(input=q,events=events,returned=returned,before=hashlib.sha256(before).hexdigest(),after=hashlib.sha256(after).hexdigest(),visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA;rows=[]
 def add(**q):
  v=dict(op='event',seed=len(rows)%17,state=5,enabled=0x80,current=0x80,local=0,host=0,phase=1,count=1,closing=0,local_flag=0,controller=0,controller_flags=8,present=1,revision=0xffffffff,value=1,running=1,stage=10,reason=0,user=0,flag=0,write=0x80);v.update(q);rows.append(execute(image,v))
 for state,enabled,closing in itertools.product((*range(12),0xffffffff),(0,0x80),(0,1,0x80)):
  add(state=state,enabled=enabled,closing=closing)
 for state,current,host,phase in itertools.product((*range(12),0xffffffff),(0,0x80),(0,1),(0,1,2,0xffffffff)):
  add(op='ready',state=state,current=current,host=host,phase=phase)
 for controller,flags in itertools.product(range(4),(0,1,7,8,15,0x80000000,0x80000008,0xffffffff)):
  add(op='controller',controller=controller,controller_flags=flags)
 for state,current,count in itertools.product((*range(12),0xffffffff),(0,0x80),(0,1,2,0x80000000,0xffffffff)):
  add(op='count',state=state,current=current,count=count)
 for state,current,host,value in itertools.product((0,1,2,5,6,7,8,9,10,11,0xffffffff),(0,0x80),(0,1),(0,1,0x80,0xff)):
  add(op='settings',state=state,current=current,host=host,value=value)
 for state,current,present,localflag,user,reason,flag,write in itertools.product((0,3,4,5,6,7,8,9),(0,0x80),(0,0x80),(0,0x80),(0,3),(0,1,2),(0,1,0x80),(0,0x80)):
  add(op='wrapper',state=state,current=current,present=present,local_flag=localflag,user=user,reason=reason,flag=flag,write=write,stage=-11)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_lobby_bank_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),limits=['Complete5AD686/5AD457,5B1C39/5AD87C,606B50,615285/5B1BDE/5B1A37/5C20C8,615219/5B0FB3/5B0F53/5C91CB and5B11E2 execute unchanged.','Current-session resolution5AD736 and external5C96FB write are explicit service boundaries; stack-cookie return is explicit. Complete7B88 banks are compared.','Settings corpus excludes remote3/4 transport; native controls reject it before mutation. No actual Leave/UI/device/transport parity claim.'])
 (OUT/'bank-original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
