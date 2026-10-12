#!/usr/bin/env python3
"""Unchanged PC local event→actual response→language→recent hash→queue join.

Actual supplied shared.map mulg records are copied unchanged to declared memory.
Original local iterator, audience matcher, game-over/local-user guard, language
query, response delay, hash suppression and queue bodies execute. Explicit
services are network notification, captions, local-output-user count, standard
memset/cookie and sound playback handles. No network/caption/device/GPU parity.
"""
import hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import ROOT,SHA,BASE,STOP,STACK,OBJ,machine,u32,w32
from export_vista_announcer_cues import PcMap
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/announcer-events-original'
MATG=OBJ;POOL=OBJ+0x2000;PLAYERS=OBJ+0x3000;BITS=OBJ+0x5000
MULG=OBJ+0x8000;WORLD=OBJ+0x17000;TIME=OBJ+0x1a000;EVENT=OBJ+0x1b000;TABLE=0x2000000
CONTEXTS=[(0x27f,0x1f80),(0x67f,0x3f80),(0xa7f,0x5f80),(0xe7f,0x7f80),(0x37f,0x1f80),(0x7f,0x1f80),(0x27f,0x9fc0)]
def event(kind,recipient=0xffffffff):return [kind[0],kind[1],recipient,0xffffffff,0xffffffff,0xffffffff,0xffffffff,0,0xffff]
class Original:
 def __init__(self,b,cache):
  self.u=machine(b);self.u.mem_map(TABLE,0x40000);self.visits=0;self.executed=set();self.u.hook_add(UC_HOOK_CODE,self.hook)
  tag=cache.one('mulg','multiplayer\\multiplayer_globals');payload=cache.data[tag['file_offset']:tag['file_offset']+tag['bytes']]
  self.u.mem_write(MULG,payload);magic=(MULG-tag['address'])&0xffffffff
  for p,v in [(0x879e70,MATG),(MATG+0x16c,int(tag['datum'],16)),(0x87cd50,TABLE),(0x87cd54,magic),(0x882290,magic),
      (TABLE+(int(tag['datum'],16)&65535)*16+8,tag['address']),(0x882d3c,WORLD),(0x8c06e4,TIME),
      (0x8a8260,POOL),(POOL+0x20,16),(POOL+0x24,0x204),(POOL+0x38,16),(POOL+0x44,PLAYERS),(POOL+0x48,BITS),(BITS,1)]:w32(self.u,p,v)
  runtime=cache.block(tag['file_offset']+8,0x568)[0];self.responses={}
  for name,off in [('general',0x98),('slayer',0xa8)]:self.responses[name]=[cache.data[p:p+0xa8].hex()for p in cache.block(runtime+off,0xa8)]
 def ret(self,value=0):
  s=self.u.reg_read(UC_X86_REG_ESP);self.u.reg_write(UC_X86_REG_EAX,value&0xffffffff);self.u.reg_write(UC_X86_REG_EIP,u32(self.u,s));self.u.reg_write(UC_X86_REG_ESP,s+4)
 def hook(self,u,pc,n,_):
  self.visits+=1;self.executed.add(pc);s=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:self.ret();return
  if pc==0x687ba9:u.mem_write(u32(u,s+4),bytes([u32(u,s+8)&255])*u32(u,s+12));self.ret(u32(u,s+4));return
  if pc==0x5b67ea:self.ret();return # Network side effect remains external.
  if pc==0x45ae37:self.ret();return # Separate event side effect after delivery.
  if pc==0x4513b4:self.ret(1);return # Explicit single actual output user.
  if pc==0x43e3ac:u.mem_write(u32(u,s+12),b'\0\0');self.ret();return
  if pc==0x45d7c8:u.mem_write(u32(u,s+16),b'\0\0');self.ret();return
  if pc in (0x62dd6f,0x62de21,0x62cd3c):self.ret();return
  if pc==0x4381fd:self.ret(self.q.get('language',0));return # Source language service input.
  if pc==0x45c1e8:self.selected=u.reg_read(UC_X86_REG_EAX)
  if pc in (0x48836c,0x4883b6):self.calls.append([pc,u32(u,s+4),u32(u,s+8)]);self.ret(self.q.get('voice',0x12340001));return
 def state(self):return dict(queue=(bytes(self.u.mem_read(0x8d8578,60))+bytes(self.u.mem_read(0x8d8614,4))).hex(),recent=bytes(self.u.mem_read(0x8d85b4,96)).hex())
 def call(self,pc,args=()):
  s=STACK+0x10000;self.u.mem_write(s,struct.pack('<I',STOP)+b''.join(struct.pack('<I',x&0xffffffff)for x in args));self.u.reg_write(UC_X86_REG_ESP,s)
  self.u.emu_start(pc,STOP+1,count=100000);assert self.u.reg_read(UC_X86_REG_EIP)==STOP
 def reset(self,rate,cw,mx):
  self.u.reg_write(UC_X86_REG_FPCW,cw);self.u.reg_write(UC_X86_REG_MXCSR,mx);self.u.mem_write(TIME+2,struct.pack('<h',rate));self.calls=[];self.q={};self.call(0x4d0e02)
 def run(self,q,kind='event'):
  self.q=q;self.calls=[];self.selected=None;self.u.mem_write(WORLD+0xc,bytes([q.get('mode',1)]));self.u.mem_write(WORLD+0x1198,bytes([q.get('over',0)]));w32(self.u,TIME+8,q.get('tick',0))
  self.u.mem_write(PLAYERS,struct.pack('<HH',0x1234,q.get('flags',1)));self.u.mem_write(PLAYERS+0xbc,bytes([q.get('member',0)&255]));self.u.mem_write(PLAYERS+0x24,struct.pack('<h',q.get('user',0)))
  before=self.state()
  if kind=='event':
   e=q['event'];self.u.mem_write(EVENT,struct.pack('<8IH',*e)+b'\xa5\xa5');self.call(0x45dc77,(EVENT,))
  elif kind=='update':self.call(0x4d0ff4)
  elif kind=='complete':self.call(0x4d0e62,(0,q['voice']))
  selected=None
  if self.selected:
   p=self.selected;r=bytes(self.u.mem_read(p,0xa8));selected=dict(record=r.hex(),sound=struct.unpack_from('<I',r,0x48+q.get('language',0)*8)[0])
  return dict(kind=kind,input=q,before=before,after=self.state(),calls=self.calls,selected=selected)
def main():
 b=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(b).hexdigest()==SHA
 cache=PcMap(ROOT/'Halo 2 Vista installer/maps/shared.map');o=Original(b,cache);rows=[]
 for cw,mx in CONTEXTS:
  for rate,kind,language,mode,over,flags,user in itertools.product([30,60],[(2,0),(0,19),(0,31)],[0,1,7,8],[1,4],[0,1],[1,2],[-1,0]):
   o.reset(rate,cw,mx);q=dict(event=event(kind,0x12340000 if kind==(2,0) else 0xffffffff),language=language,mode=mode,over=over,flags=flags,user=user)
   for tick in [0,1,26,27,28,53,54]:
    q['tick']=tick;r=o.run(dict(q));r.update(cw=cw,mxcsr=mx,rate=rate);rows.append(r)
  # Genuine queue sequence: sentinel, start event, playback/completion, end.
  o.reset(30,cw,mx)
  operations=[('event',dict(event=event((2,0),0x12340000),tick=0))]
  operations +=[('update',dict(tick=t))for t in range(1,70)]
  operations +=[('complete',dict(tick=70,voice=0x12340001)),('update',dict(tick=71)),('event',dict(event=event((0,19)),tick=72,over=1)),('update',dict(tick=73)),('complete',dict(tick=74,voice=0x12340001)),('update',dict(tick=75))]
  for kind,q in operations:r=o.run(q,kind);r.update(cw=cw,mxcsr=mx,rate=30);rows.append(r)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_announcer_events_original.json';fixture.write_text(json.dumps(dict(responses=o.responses,cases=rows),separators=(',',':'))+'\n')
 data=dict(image_sha256=SHA,shared_sha256=hashlib.sha256(cache.data).hexdigest(),cases=len(rows),instructions=o.visits,fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),scope=__doc__)
 (OUT/'execution.json').write_text(json.dumps(data,indent=2)+'\n');c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
 for pc in sorted(o.executed):
  if BASE<=pc<BASE+len(b):i=next(c.disasm(b[pc-BASE:pc-BASE+15],pc));asm.append(f'{pc:08X}: {i.mnemonic} {i.op_str}')
 (OUT/'executed-original.asm').write_text('\n'.join(asm)+'\n');cache.close();print(json.dumps(data))
if __name__=='__main__':main()
