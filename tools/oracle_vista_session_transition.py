#!/usr/bin/env python3
"""Execute the supplied Vista lifecycle pump, preflight, phase writes and Done.

World, network IO and the other handler bodies are explicit controlled services.
This joins actual original instructions; it is not a live native match test.
"""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32

ROOT=Path(__file__).resolve().parents[1]
N=0x7b88;MAN=OBJ+0x8000;HAND=OBJ+0x9000;VT=OBJ+0xa000
OPTIONS=OBJ+0xb000;WORLD=OBJ+0xc000;CB=STOP+0x100
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/transition-joined-original'
def digest(b):return hashlib.sha256(b).hexdigest()
def initial(q):
 b=bytearray((i*37+q['seed']*13+(i>>8)*17)&255 for i in range(N))
 def put(at,v,n=4):b[at:at+n]=(v&0xffffffff).to_bytes(4,'little')[:n]
 for at,v,n in [(8,0x13371337,4),(0x14,0,4),(0x64,0,4),(0x73a0,q['state'],4),(0x73a4,q['transition'],1),(0x4c68,q['phase'],4),
   (0x84,q['count'],4),(0x4c64,0xffffffff,4),(0x4c60,0xfffffffe,4),(0x71c0,0,4),(0x74,0,4),
   (0x4cb0,q['force'],1),(0x4cb8,2,4),(0x4dd8,17 if q['map_valid'] else 0xffffffff,4),(0x4cbc,65 if q['path'] else 0,2)]:put(at,v,n)
 for i in range(17):put(0x71c5+i*0x1c,0x80 if i%3==0 else 0,1)
 return bytes(b)
def execute(image,q):
 u=machine(image);u.mem_write(OBJ,initial(q));u.mem_write(MAN,bytes(0x100));w32(u,MAN,q['kind']);w32(u,MAN+0x24,OBJ)
 u.mem_write(MAN+0x40,bytes((i*11+7)&255 for i in range(32)))
 original_identity=[0x12345678,0x87654321]
 for k in range(7):
  h=HAND+k*0x100;v=VT+k*16;w32(u,MAN+4+4*k,h);w32(u,h,v);w32(u,h+4,k);w32(u,h+8,MAN);u.mem_write(h+0xc,b'\1');u.mem_write(h+0x10,struct.pack('<II',*original_identity))
  methods=[CB,CB+4,CB+8,0]
  if k==3:methods=[0x5d95a7,0x5d9704,0x5d90b5,0]
  if k==4:methods=[0x5d903f,0x5d9070,0x5d909a,0]
  u.mem_write(v,struct.pack('<4I',*methods))
 w32(u,0x882d3c,WORLD);u.mem_write(WORLD+0x1198,bytes([q['complete']]));w32(u,WORLD+0x119c,q['remaining'])
 w32(u,OPTIONS+0x10,original_identity[0]^(0 if q['identity_match'] else 1));w32(u,OPTIONS+0x14,original_identity[1])
 w32(u,OPTIONS+0x80,2);w32(u,OPTIONS+0x84,17 if q['map_match'] else 18)
 u.mem_write(0x9178d0,bytes([q['closing_a']]));u.mem_write(0x9178d2,bytes([q['closing_b']]))
 u.mem_write(0x91abc8,b'\1');w32(u,0x820fe8,OBJ);w32(u,0x820fc4,q['kind']);u.mem_write(0x820fc0,b'\1')
 events=[];visits=0;clocks=0
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,size,_):
  nonlocal visits,clocks
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc in (0x6875d3,0x5b07a0):
   if pc==0x5b07a0:events.append(['postgame',u32(u,sp+4)])
   ret();return
  if pc==0x5ea803:events.append(['network']);ret();return
  if pc in (CB,CB+4,CB+8):
   kind=(u.reg_read(UC_X86_REG_ECX)-HAND)//0x100
   events.append(['other_update' if pc==CB else 'other_enter' if pc==CB+4 else 'other_exit',kind])
   ret(0,0 if pc==CB else 12 if pc==CB+4 else 4);return
  if pc==0x5b3ca0:
   events.append(['clock',digest(bytes(u.mem_read(OBJ,N)))]);clocks+=1;w32(u,0x91abcc,0x12340000+clocks);return
  if pc==0x5bed40:
   args=[u32(u,sp+4+4*i) for i in range(6)]
   events.append(['send',u.reg_read(UC_X86_REG_ECX),*args[:5],digest(bytes(u.mem_read(args[5],args[4])))])
   ret(0,24);return
  if pc==0x5add8e:events.append(['closing']);return
  if pc==0x4499a8:events.append(['complete']);return
  if pc==0x4497ea:events.append(['ready']);ret(q['ready']);return
  if pc==0x449801:events.append(['options']);ret(OPTIONS);return
  if pc==0x449fd7:events.append(['force_end']);ret();return
  if pc==0x5ae8d1:events.append(['network_close']);ret();return
  if pc==0x5d94a1:
   events.append(['map_change']);u.mem_write(u.reg_read(UC_X86_REG_ECX)+0x10,struct.pack('<II',0xabcdef01,0x76543210));ret(0,4);return
  if pc==0x5c503b:events.append(['termination_reason']);ret(q['reason']);return
  if pc==0x615a7c:events.append(['notify',u32(u,sp+4)]);ret();return
  if pc==0x5cc0a8:events.append(['remote_close',u32(u,sp+4)]);w32(u,OBJ+0x73a0,0);ret(0,4);return
 u.hook_add(UC_HOOK_CODE,hook)
 def run(entry,this,args=()):
  sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,this)
  u.emu_start(entry,STOP+1,count=300000);assert u.reg_read(UC_X86_REG_EIP)==STOP
  return u.reg_read(UC_X86_REG_EAX)&255
 op=q['op'];returned=None
 if op=='done_cycle':
  run(0x5d7738,MAN);w32(u,0x820fc4,u32(u,MAN));returned=run(0x5ad9e3,OBJ);run(0x5d7738,MAN)
 elif op=='pump':run(0x5d7738,MAN)
 elif op=='preflight':returned=run(0x5d7644,HAND+0x300)
 elif op=='update3':returned=run(0x5d95a7,HAND+0x300)
 elif op=='update4':returned=run(0x5d903f,HAND+0x400)
 elif op=='exit':run(0x5d90b5,HAND+0x300,[HAND+0x400])
 elif op=='enter3':run(0x5d9704,HAND+0x300,[HAND+0x200,0,0])
 else:run(0x5d9070,HAND+0x400,[HAND+0x300,0,0])
 manager=[u32(u,MAN),*u.mem_read(MAN+0x34,3),u32(u,MAN+0x38),u32(u,MAN+0x3c),bytes(u.mem_read(MAN+0x40,32)).hex()]
 return dict(input=q,events=events,after=digest(bytes(u.mem_read(OBJ,N))),manager=manager,manager_hex=bytes(u.mem_read(MAN,0x100)).hex(),identity=list(struct.unpack('<II',u.mem_read(HAND+0x310,8))),returned=returned,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert digest(image)==SHA;rows=[]
 def add(**kw):
  q=dict(op='pump',kind=3,state=5,transition=0,phase=4,count=3,force=0,complete=1,remaining=0,ready=1,identity_match=1,map_valid=1,path=1,map_match=1,closing_a=0,closing_b=0,reason=1,seed=19);q.update(kw);rows.append(execute(image,q))
 for op,state,phase in itertools.product(('pump','update3','update4','preflight'),(*range(12),0xffffffff),(1,4,5)):
  add(op=op,state=state,phase=phase,kind=4 if op=='update4' else 3)
 for op,ready,identity,valid,path,match in itertools.product(('pump','update3','exit'),(0,0x80),(0,1),(0,1),(0,1),(0,1)):
  add(op=op,ready=ready,identity_match=identity,map_valid=valid,path=path,map_match=match)
 for state,complete,remaining,force,a,b in itertools.product((3,5,7),(0,0x80),(0,1,0xffffffff),(0,0x80),(0,0x80),(0,0x80)):
  add(state=state,complete=complete,remaining=remaining,force=force,closing_a=a,closing_b=b)
 for state,count,ready in itertools.product((3,5,7,8),(0,1,16,17),(0,1)):add(op='done_cycle',state=state,count=count,ready=ready)
 for reason,op in itertools.product((0,2,0xffffffff),('pump','preflight','update3','update4')):add(op=op,state=10,reason=reason,kind=4 if op=='update4' else 3)
 for state,count,ready in itertools.product((3,5,7,8),(0,1,16,17),(0,1)):add(op='done_cycle',kind=4,phase=5,state=state,count=count,ready=ready)
 for op in ('enter3','enter4'):add(op=op)
 for state,transition in itertools.product((7,8),(1,0x80)):add(state=state,transition=transition)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_session_transition_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instruction_visits=sum(r['visits'] for r in rows),fixture_sha256=digest(fixture.read_bytes()),limits=['World options/readiness, map-change, network IO, termination and non3/4 handler bodies are controlled services.','Full original pump, preflight, admission getters, game-over predicate, selected-map getter, phase writer and Done execute unchanged.','Every full retained session bank and ordered external event is compared. This is not a complete live native transition or fade.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
