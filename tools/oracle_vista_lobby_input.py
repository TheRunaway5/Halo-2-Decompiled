#!/usr/bin/env python3
"""Unchanged PC14 event/Setup and61694B cancellation with explicit services."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-input-original'
EVENT=OBJ+0x8000
REF=EVENT+0x100
ITEM=EVENT+0x200
def signed(v):return struct.unpack('<i',struct.pack('<I',v&0xffffffff))[0]
def execute(image,q):
 u=machine(image);events=[];visits=0;route=None
 u.mem_write(EVENT,struct.pack('<3I',5,q['controller']&0xffffffff,dict(start=0,back=1,auxiliary=2).get(q['op'],0)))
 w32(u,REF,EVENT);u.mem_write(ITEM,struct.pack('<H',1));u.mem_write(OBJ+0x1c6c+q['controller']*4,bytes([q['edit'],0,0xff,0xff]))
 def ret(v=0,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def end(r):
  nonlocal route
  route=r;u.emu_stop()
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x61520f:events.append(['pregame',q['pregame']]);ret(q['pregame'])
  elif pc==0x60a540:events.append(['channel',u32(u,sp+4),q['channel']]);ret(q['channel'])
  elif pc==0x606b50:events.append(['signed',signed(u32(u,sp+4)),q['signed']]);ret(q['signed'])
  elif pc==0x6152b0:events.append(['host',q['host']]);ret(q['host'])
  elif pc==0x6153c9:events.append(['ready',q['ready']]);ret(q['ready'])
  elif pc==0x616ba2:events.append(['active',q['active']]);ret(q['active'])
  elif pc==0x643a40:events.append(['start',signed(u32(u,sp+4))]);ret(0)
  elif pc==0x643c25 and q['op'] not in ('cancel','request'):
   events.append(['cancel',signed(u32(u,sp+4)),q['cancel']]);ret(q['cancel'])
  elif pc==0x615a4b and q['op']=='auxiliary':ret(10)
  elif pc==0x61694b and q['op']=='auxiliary':
   assert u32(u,sp+8)==10
   events.append(['cancel',signed(u32(u,sp+4)),q['cancel']]);ret(q['cancel'])
  elif pc==0x615219:events.append(['settings',u32(u,sp+4)&255]);ret(0)
  elif pc==0x64534c:events.append(['finish',signed(u32(u,sp+4))]);ret(0,8)
  elif pc==0x60b0bc:ret(0)
  elif pc==0x60b0c6:
   assert [u32(u,sp+4+i*4) for i in range(6)]==[4,1<<q['controller'],4,4,0,0x64fe89]
   ret(0,24)
  elif pc==0x60b11e:end('setup')
  elif pc==0x60eb2b:end('start_and_base_input' if any(e[0]=='start' for e in events) else 'base_input')
  elif pc==0x6454f2:end('leave')
  elif pc in (0x6435b5,0x6460d1):end('team_edit' if pc==0x6435b5 else 'unsigned_controller')
  elif pc in (0x645c6c,0x645cdd,0x645d69,0x6462d0):events.append(['team',q['edit']])
  elif pc==0x5b1bde:events.append(['duration']);ret(0)
  elif pc==0x60687f:events.append(['user',signed(u32(u,sp+4)),q['user']]);ret(q['user'])
  elif pc==0x5b13c7:events.append(['stage',q['stage']]);ret(q['stage'])
  elif pc==0x5ad940:events.append(['host',q['host']]);ret(q['host'])
  elif pc==0x616490:assert u32(u,sp+4)==0;events.append(['ui',q['ui']]);ret(q['ui'])
  elif pc==0x5b11e2:
   a=[u32(u,sp+4+i*4) for i in range(5)];assert a[4]==0
   events.append(['write',a[0]&255,signed(a[1]),a[2]&255,signed(a[3])]);ret(q['write'])
 u.hook_add(UC_HOOK_CODE,hook)
 args=[REF,ITEM] if q['op']=='setup' else [q['controller']&0xffffffff,q['requested']&0xffffffff] if q['op']=='request' else [q['controller']&0xffffffff] if q['op']=='cancel' else [EVENT]
 entry=0x64620f if q['op']=='setup' else 0x61694b if q['op']=='request' else 0x643c25 if q['op']=='cancel' else 0x645bb1
 sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=100000)
 if route is None:
  assert u.reg_read(UC_X86_REG_EIP)==STOP
  route='consumed' if q['op'] not in ('cancel','request') else 'cancel_result'
 return dict(input=q,events=events,route=route,returned=u.reg_read(UC_X86_REG_EAX)&255 if route=='cancel_result' else None,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 def add(**q):
  v=dict(op='start',controller=0,pregame=0x80,channel=0,signed=0x80,host=0x80,edit=0,ready=0x80,active=0x80,cancel=0x80,user=0,stage=5,ui=11,write=0x80,requested=10);v.update(q);rows.append(execute(image,v))
 for op,pregame,channel,signin,host,active,cancel in itertools.product(('start','back','auxiliary'),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80)):
  add(op=op,pregame=pregame,channel=channel,signed=signin,host=host,active=active,cancel=cancel)
 for signin,host,edit,ready,cancel,controller in itertools.product((0,0x80),(0,0x80),(0,1),(0,0x80),(0,0x80),(0,3)):
  add(op='setup',signed=signin,host=host,edit=edit,ready=ready,cancel=cancel,controller=controller)
 for op,user,stage,host,ui,write,requested,controller in itertools.product(('cancel','request'),(-1,0,3),(-2147483648,-1,0,3,10,2147483647),(0,0x80),(0,10,11,12),(0,0x80),(-2147483648,-1,0,10,2147483647),(0,3)):
  add(op=op,user=user,stage=stage,host=host,ui=ui,write=write,requested=requested,controller=controller)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_lobby_input_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instructions=sum(r['visits'] for r in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),limits=['Original complete645BB1 event and64620F Setup instructions execute; external base/Leave/team-edit/factory calls are explicit stop boundaries.','Start/cancel callbacks are explicit supplied effects for input coverage. Full643C25/61694B execute separately with source-user, stage, host, UI and5B11E2 as controlled service boundaries.','Unrelated team-edit and unsigned-controller join policies are not implemented. No real input/audio/GPU or complete Vista parity claim.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
