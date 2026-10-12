#!/usr/bin/env python3
"""Unchanged sole-local disposal/browser reset and PC14 Leave task corridor."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-leave-original'
SIZE=0x7b88;REG=OBJ+0x10000;MAN=OBJ+0x11000;HAND=OBJ+0x12000;VT=OBJ+0x13000;TASK=OBJ+0x14000
BROWSER=0x822568;BSIZE=0x4045c;KEYS=0x91c4cc
def digest(b):return hashlib.sha256(b).hexdigest()
def initial(q):
 b=bytearray((i*37+q['seed']*13)&255 for i in range(SIZE))
 for at,v in ((8,REG),(0x14,q['slot']),(0x5c,q['key_slot']),(0x74,0),(0x84,1),(0x71c0,0),(0x7b78,0),(0x73a0,q['state'])):struct.pack_into('<I',b,at,v)
 b[0x48]=q['has_key'];b[0x6c]=0;b[0x73a4]=q['closing']
 for i in range(17):b[0x71c4+i*0x1c]=0
 b[0x71c4]=q['association'];b[0x71c5]=0;struct.pack_into('<I',b,0x71c8,0xffffffff)
 k=bytearray((i*11+q['seed']*7)&255 for i in range(64));k[q['key_slot']*32]=q['key_active'];k[q['key_slot']*32+2]=q['key_exempt']
 r=bytes((i*17+q['seed']*19)&255 for i in range(0x14+2*36))
 browser=bytearray((i*3+q['seed']*5)&255 for i in range(BSIZE))
 for at,v in ((0x428,q['handle']),(0xc,q['child'])):struct.pack_into('<I',browser,at,v)
 browser[0x4044c]=q['query']
 return b,k,r,browser
def execute(image,q):
 u=machine(image);b,k,r,br=initial(q);u.mem_write(OBJ,bytes(b));u.mem_write(KEYS,bytes(k));u.mem_write(REG,r);u.mem_write(BROWSER,bytes(br))
 w32(u,0x820fe8,OBJ);u.mem_write(0x8fad99,bytes([q['network']]));u.mem_write(0x926bf0,bytes([q['qos']]));u.mem_write(0x91c45c,b'\1');u.mem_write(0x820fc0,b'\1');w32(u,0x820fc4,q['kind'])
 u.mem_write(MAN,bytes(0x100));w32(u,MAN,q['kind']);w32(u,MAN+0x24,OBJ)
 for i in range(7):
  h=HAND+i*0x100;v=VT+i*16;w32(u,MAN+4+i*4,h);w32(u,h,v);w32(u,h+4,i);w32(u,h+8,MAN);u.mem_write(h+0xc,b'\1')
  methods=[0x5d9dae,0x5d9e1c,0x5d9e31] if i==0 else [0x5d9c6b,0x5d9b62,0x5d9ba2]
  u.mem_write(v,struct.pack('<3I',*methods))
 events=[];visits=0;completed=False
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits,completed
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x6875d3:ret()
  elif pc==0x687ba9:
   dst,val,n=[u32(u,sp+4+i*4) for i in range(3)];u.mem_write(dst,bytes([val&255])*n);ret(dst)
  elif pc==0x5ad736:w32(u,u32(u,sp+4),OBJ);ret(1)
  elif pc==0x6a8673:
   assert [u32(u,sp+8+i*4) for i in range(4)]==[0,0,0,0x10]
   p=u32(u,sp+4);events.append(['qos',u32(u,p),u32(u,p+4)]);ret(0,20)
  elif pc==0x6a8607:
   p=u32(u,sp+4);events.append(['key',u32(u,p),u32(u,p+4)]);ret(0,4)
  elif pc==0x5bed40:raise RuntimeError('sole-local disposal tried remote send')
  elif pc==u32(u,0x79b2b0):events.append(['handle',u32(u,sp+4)]);ret(1,4)
  elif pc==0x601a9e:events.append(['child',u.reg_read(UC_X86_REG_ECX)]);ret()
  elif pc==u32(u,0x86d76c):
   p=u32(u,sp+4);events.append(['query',digest(bytes(u.mem_read(p,0x208)))]);ret(0,4)
  elif pc==0x60c776:
   a=[u32(u,sp+4+i*4) for i in range(5)];events.append(['task',*a]);ret()
  elif pc==0x639408:events.append(['status',u32(u,sp+4)]);ret(0,4)
  elif pc==0x6393fc:events.append(['complete']);completed=True;ret()
  elif pc==0x5ea803:events.append(['network_update']);ret()
  elif pc==0x5b54cf:events.append(['mode',u32(u,sp+4),u32(u,sp+8)]);ret()
 u.hook_add(UC_HOOK_CODE,hook)
 def run(entry,this=OBJ,args=()):
  sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*[v&0xffffffff for v in args]));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,this)
  u.emu_start(entry,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP,hex(u.reg_read(UC_X86_REG_EIP))
 if q['op']=='dispose':run(0x5cc258)
 elif q['op']=='browser':run(0x5e46e4,BROWSER,[q['mode']])
 elif q['op']=='task':run(0x642e8c,TASK,[TASK])
 elif q['op']=='leave':
  u.reg_write(UC_X86_REG_EDI,q['controller']);run(0x6454f2)
  run(0x642e8c,TASK,[TASK])
  if q['pump']:
   # Actual source manager consumes state0, then source null handler calls the
   # declared5B54CF boundary. No kind0 assignment is made by this harness.
   run(0x5d7738,MAN);w32(u,0x820fc4,u32(u,MAN));run(0x642e8c,TASK,[TASK])
 else:raise RuntimeError(q['op'])
 return dict(input=q,events=events,completed=completed,kind=u32(u,MAN),before=[digest(b),digest(k),digest(r),digest(br)],after=[digest(bytes(u.mem_read(a,n))) for a,n in ((OBJ,SIZE),(KEYS,64),(REG,len(r)),(BROWSER,BSIZE))],visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert digest(image)==SHA;rows=[]
 def add(**q):
  v=dict(op='dispose',seed=len(rows)%13,state=5,slot=0,key_slot=0,closing=0,has_key=1,key_active=1,key_exempt=0,association=1,network=1,qos=0,handle=0xffffffff,child=0,query=0,mode=6,kind=1,controller=0,pump=0);v.update(q);rows.append(execute(image,v))
 for state,closing,key,active,exempt,network,qos,slot in itertools.product((0,5,6,7,8),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,1)):
  add(state=state,closing=closing,has_key=key,key_active=active,key_exempt=exempt,network=network,qos=qos,slot=slot,key_slot=slot)
 for mode,handle,child,query in itertools.product((0,6,0xffffffff),(0xffffffff,0,0x12345678),(0,0x87654321),(0,0x80)):
  add(op='browser',mode=mode,handle=handle,child=child,query=query)
 for kind in range(9):add(op='task',kind=kind)
 for state,controller,pump in itertools.product((5,7,8),(0,3),(0,1)):
  add(op='leave',state=state,controller=controller,pump=pump)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_session_disposal_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits'] for r in rows),fixture_sha256=digest(fixture.read_bytes()),limits=['Complete5CC258/5CC0A8/5CA1CD/5C7FA2/5C7F0C/5C73A3/5B56F9/5BD629 execute unchanged for the sole-local source bank. QoS/key OS imports and standard CRT memset are explicit boundaries.','Complete5E46E4 browser reset,6454F2 sole-peer Leave,616388,5AD6AF,642E8C and5D7738→actualnull handler execute. Browser OS/child,taskallocation/status/completion,networkframe and5B54CF are explicit services.','No remote peer, Windows XNet teardown, complete browser constructor, live loading/task/GPU or full Vista parity claim.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
