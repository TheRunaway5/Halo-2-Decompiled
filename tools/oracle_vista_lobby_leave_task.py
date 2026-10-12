#!/usr/bin/env python3
"""Unchanged60C776 binding and63956E/642E8C/6393FC task-frame corridor."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'analysis/frontend/vista-session-flow-audit/lobby-leave-original';SIZE=0xa70

def initial(q):
 b=bytearray((i*37+q['seed']*13)&255 for i in range(SIZE));b[0x42]=q['flags']
 struct.pack_into('<I',b,0xa68,0xffffffff);struct.pack_into('<I',b,0xa5c,q['callback']);return b

def execute(image,q):
 u=machine(image);before=initial(q);u.mem_write(OBJ,bytes(before));events=[];visits=0;ctx=0
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop()
  elif pc==0x63941d:
   d=u32(u,sp+4);events.append(['factory',*[u32(u,d+i*4) for i in range(8)]]);ret(OBJ)
  elif pc==0x6154d8:events.append(['context',ctx]);ret(ctx)
  elif pc==0x639408:events.append(['status',u32(u,sp+4)]);ret(0,4)
  elif pc==0x612604:events.append(['close',u32(u,sp+4)]);ret(0,4)
  elif pc==0x612cd8:events.append(['base']);ret()
 u.hook_add(UC_HOOK_CODE,hook)
 def run(entry,args=()):
  sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
  u.emu_start(entry,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
 hashes=[]
 if q['op']=='bind':run(0x60c776,[0xffffffff,q['controller'],0x642e8c,0,0])
 else:
  for ctx in q['contexts']:
   run(0x63956e);hashes.append(hashlib.sha256(bytes(u.mem_read(OBJ,SIZE))).hexdigest())
 return dict(input=q,events=events,before=hashlib.sha256(before).hexdigest(),after=hashlib.sha256(bytes(u.mem_read(OBJ,SIZE))).hexdigest(),frames=hashes,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA;rows=[]
 def add(**args):
  q=dict(op='frame',seed=len(rows)%17,controller=0,callback=0x642e8c,flags=0,contexts=[1,0,0]);q.update(args);rows.append(execute(image,q))
 for controller,seed in itertools.product(range(4),(0,1,8,16)):add(op='bind',controller=controller,seed=seed)
 for callback,flags,contexts,seed in itertools.product((0,0x642e8c),(0,1,2,3,0x80,0xff),([1,0,0],[0,0],[2,2],[7,0,0],[0xffffffff,0,0]),(0,1,16)):
  add(callback=callback,flags=flags,contexts=contexts,seed=seed)
 OUT.mkdir(parents=True,exist_ok=True);f=ROOT/'tests/fixtures/vista_lobby_leave_task_original.json';f.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 r=dict(source_sha256=SHA,cases=len(rows),instructions=sum(row['visits'] for row in rows),fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),limits=['Complete60C776 and actual63939E/639384/639391 binding execute. Widget construction63941D is an explicit allocation/base/UI boundary and exact descriptor is retained.','Complete63956E/642E8C/6393FC/6393AB execute with actual A68 NONE; complete A70 bank and per-frame hashes are compared. Lifecycle context/status/close/base widgets remain mandatory actual services.','Source63941D disassembly proves screen182 and source-owned model block. No widget allocation/rendering or task-screen parity is claimed.'])
 (OUT/'task-original.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r))
if __name__=='__main__':main()
