#!/usr/bin/env python3
"""Unchanged PC gameplay black-quad producer and simulation-tick projection.
The47590F tail starts after its unrelated engine/player/event work. Real output
user enumeration, selected engine, player mapping and46C54F are explicit
services. Actual mode/tick-rate/tick/phase/opacity/packing bodies execute.
The47554D draw stops immediately before its separate scoreboard branch.
No original GPU, resource lifetime, DLL body or live native activation claim.
"""
import hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/gameplay-fade-original'
ENGINE=OBJ;WORLD=OBJ+0x2000;TIME=OBJ+0x4000;PLAYERHDR=OBJ+0x5000;PLAYERS=OBJ+0x6000
CONTEXTS=[(0x27f,0x1f80),(0x67f,0x3f80),(0xa7f,0x5f80),(0xe7f,0x7f80),(0x37f,0x1f80),(0x7f,0x1f80),(0x27f,0x9fc0)]
class Original:
 def __init__(self,b,cw,mx):
  self.u=machine(b,cw,mx);self.visits=0;self.executed=set();self.mode='';self.u.hook_add(UC_HOOK_CODE,self.hook)
 def ret(self,value=None,pop=0):
  u=self.u;s=u.reg_read(UC_X86_REG_ESP)
  if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(self,u,pc,size,_):
  self.visits+=1;self.executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x45b15e:self.ret(OBJ+0x8000 if self.q.get('engine',1) else 0);return
  if pc==0x45343f:self.ret(next(iter(self.users),0xffffffff));return
  if pc==0x4513be:
   old=u32(u,s+4);self.ret(next((v for v in self.users if v>old),0xffffffff));return
  if pc==0x45141d:
   user=u32(u,s+4);p=self.users.get(user);self.ret(0x10000+user if p is not None and p.get('player',1) else 0xffffffff);return
  if pc==0x46c54f:
   user=u32(u,s+4)&0xffff;self.calls.append(['active',user]);self.ret(self.users[user].get('active',0));return
  if pc==0x6875d3:self.ret();return
  if self.mode=='render':
   if pc==0x620fbd:self.ret(self.q.get('blocked',0));return
   if pc==0x664f3c:self.ret(self.q.get('windows',1));return
   if pc==0x45ca63:self.ret();return
   if pc==0x45b120:self.ret(0);return
   if pc in (0x673167,0x672de0):self.ret();return
   if pc==0x66eaae:
    assert u32(u,s+4)==6 and u32(u,s+8)==2 and u32(u,s+16)==20
    ptr=u32(u,s+12);colors=[u32(u,ptr+i*20+16)for i in range(4)];assert len(set(colors))==1
    self.colors.append(colors[0]);self.rectangles.append([struct.unpack('<ff',u.mem_read(ptr+i*20,8))for i in range(4)]);self.ret();return
   if pc==0x475709:u.emu_stop();return
 def state(self):
  return dict(counter=struct.unpack('<h',self.u.mem_read(ENGINE+0xfa,2))[0],opacity=[u32(self.u,ENGINE+0xe4+i*4)for i in range(4)],mask=self.u.mem_read(ENGINE+0xf4,1)[0],frames=list(self.u.mem_read(ENGINE+0xf5,4)))
 def setup(self,q,before):
  self.q=q;self.calls=[];self.colors=[];self.rectangles=[];self.users={int(k):v for k,v in q.get('users',{'0':{}}).items()}
  w32(self.u,0x8bf8f8,ENGINE);w32(self.u,0x882d3c,WORLD);w32(self.u,0x8c06e4,TIME);w32(self.u,0x8a8260,PLAYERHDR);w32(self.u,PLAYERHDR+0x44,PLAYERS)
  self.u.mem_write(ENGINE,bytes(0x1000));self.u.mem_write(WORLD,bytes(0x1270));self.u.mem_write(PLAYERS,bytes(0x1000));self.u.mem_write(WORLD+0xc,bytes([q.get('mode',1)]));self.u.mem_write(TIME,struct.pack('<hhII',1,q.get('rate',60),0x3c888889,q.get('tick',1000)&0xffffffff))
  self.u.mem_write(ENGINE+0x6c,struct.pack('<h',q.get('phase',2)));w32(self.u,ENGINE+0xc44,q.get('pending',2));w32(self.u,ENGINE+0x70,q.get('start',0))
  self.u.mem_write(ENGINE+0xfa,struct.pack('<h',before.get('counter',0)));self.u.mem_write(ENGINE+0xf4,bytes([before.get('mask',0)]));self.u.mem_write(ENGINE+0xf5,bytes(before.get('frames',[0]*4)))
  for i,v in enumerate(before.get('opacity',[0]*4)):w32(self.u,ENGINE+0xe4+i*4,v)
  for i,p in self.users.items():w32(self.u,PLAYERS+i*0x204+0x28,0x20000+i if p.get('unit',1) else 0xffffffff);w32(self.u,PLAYERS+i*0x204+0x154,p.get('respawn',0))
 def tick(self,q,before):
  self.mode='tick';self.setup(q,before);s=STACK+0x18000
  if not q.get('engine',1):entry=0x47590f;self.u.mem_write(s,struct.pack('<I',STOP))
  else:entry=0x475b40;self.u.mem_write(s,bytes(40)+struct.pack('<I',STOP));self.u.reg_write(UC_X86_REG_EDI,1)
  self.u.reg_write(UC_X86_REG_ESP,s);self.u.emu_start(entry,STOP+1,count=100000);assert self.u.reg_read(UC_X86_REG_EIP)==STOP
  return dict(kind='tick',input=q,before=before,after=self.state(),calls=self.calls)
 def render(self,q,before):
  self.mode='render';self.setup(q,before);w32(self.u,0x8e6800,q.get('user',0));self.u.mem_write(0x8e66f8,struct.pack('<hhhh',0,0,480,640));s=STACK+0x18000;self.u.mem_write(s,struct.pack('<I',STOP));self.u.reg_write(UC_X86_REG_ESP,s)
  self.u.emu_start(0x47554d,STOP+1,count=100000);assert self.u.reg_read(UC_X86_REG_EIP)in(STOP,0x475709)
  return dict(kind='render',input=q,before=before,after=self.state(),colors=self.colors,rectangles=self.rectangles)
def main():
 b=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(b).hexdigest()==SHA;rows=[];visits=0;executed=set()
 for cw,mx in CONTEXTS:
  o=Original(b,cw,mx);local=[]
  for rate,phase,pending,mode,counter in itertools.product([29,30,59,60],[1,2],[1,2],[1,4],[0,119,120,239,240,32767]):
   local.append(o.tick(dict(rate=rate,phase=phase,pending=pending,mode=mode),dict(counter=counter,opacity=[0x3e800000]*4)))
  for rate,tick,unit,respawn,active,mask in itertools.product([30,60],[0,45,46,90,91,1000],[0,1],[-1,1],[0,1],[0,1]):
   local.append(o.tick(dict(rate=rate,tick=tick,start=30,phase=1,pending=1,users={'0':dict(unit=unit,respawn=respawn,active=active)}),dict(counter=9,mask=mask,opacity=[0x3f800000,0,0,0])))
  for opacity,mask,frames,user,windows in itertools.product([0,0x3c888889,0x3f000000,0x3f800000,0x7fc12345],[0,1],[0,179,180,255],[0,0xffffffff],[0,1]):
   local.append(o.render(dict(rate=60,user=user,windows=windows),dict(opacity=[opacity,0,0,0],mask=mask,frames=[frames,0,0,0])))
  for query in [dict(engine=0),dict(blocked=1)]:local.append(o.render(query,dict(opacity=[0x3f800000]*4,mask=15,frames=[10]*4)))
  local.append(o.tick(dict(engine=0),dict(counter=123,opacity=[0x3f800000]*4)))
  # Full retained postgame sequence: exact strict4s hold,1s ramp,final2s hold.
  state=dict(counter=0,opacity=[0]*4,mask=0,frames=[0]*4)
  for t in range(1,422):
   r=o.tick(dict(rate=60,tick=10000+t),state);local.append(r);state=r['after']
  for r in local:r['cw']=cw;r['mxcsr']=mx
  rows+=local;visits+=o.visits;executed|=o.executed
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_gameplay_fade_original.jsonl';fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n'for r in rows))
 (OUT/'execution.json').write_text(json.dumps(dict(image_sha256=SHA,cases=len(rows),instructions=visits,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
 c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
 for pc in sorted(executed):
  if BASE<=pc<BASE+len(b):
   i=next(c.disasm(b[pc-BASE:pc-BASE+15],pc));asm.append(f'{pc:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'executed-original.asm').write_text('\n'.join(asm)+'\n');print(json.dumps(dict(cases=len(rows),instructions=visits,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest())))
if __name__=='__main__':main()
