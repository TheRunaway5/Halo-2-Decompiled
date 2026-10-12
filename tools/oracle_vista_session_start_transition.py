#!/usr/bin/env python3
"""Join unchanged supplied Vista0/1/2 handlers with real countdown/prep leaves."""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32
ROOT=Path(__file__).resolve().parents[1]
N=0x7b88;MAN=OBJ+0x8000;HAND=OBJ+0x9000;VT=OBJ+0xa000;WORLD=OBJ+0xc000;CB=STOP+0x100
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/start-transition-joined-original'
def digest(b):return hashlib.sha256(b).hexdigest()
def initial(q):
 b=bytearray((i*37+q['seed']*13+(i>>8)*17)&255 for i in range(N))
 def put(at,v,n=4):b[at:at+n]=(v&0xffffffff).to_bytes(4,'little')[:n]
 for at,v,n in [(8,0x13371337,4),(0x14,0,4),(0x64,0,4),(0x73a0,q['state'],4),(0x73a4,q['transition'],1),(0x4c68,q['phase'],4),
 (0x84,q['count'],4),(0x4c64,0xffffffff,4),(0x4c60,0xfffffffe,4),(0x71c0,q['host'],4),(0x74,0,4),
 (0x4c94,q['running'],1),(0x4c98,q['stage'],4),(0x4c9c,q['flag'],1),(0x4ca0,q['countdown_reason'],4),(0x4ca4,q['reason_stage'],4),
 (0x4c90,q['preflight'],4),(0x4c74,q['local_flag'],1),(0x5101,q['prepared'],1),(0x1258,q['players'],4),(0x5eb8,0xffff,2)]:put(at,v,n)
 for i in range(18):
  put(0x71c5+i*0x1c,0x80 if i%3==0 else 0,1);put(0x110+i*0x10c,q['status'],4)
 for i in range(16):
  at=0x125c+i*0x128;put(at+8,i%max(q['count'],1));put(at+0x14,i);put(at+0x118,i%8,1)
 return bytes(b)
def execute(image,q):
 u=machine(image);u.mem_write(OBJ,initial(q));u.mem_write(MAN,bytes(0x100));w32(u,MAN,q['kind']);w32(u,MAN+0x24,OBJ)
 u.mem_write(MAN+0x40,bytes((i*11+7)&255 for i in range(32)));u.mem_write(0x8b00e8,b'\xa5'*0x840)
 for k in range(7):
  h=HAND+k*0x100;v=VT+k*16;w32(u,MAN+4+4*k,h);w32(u,h,v);w32(u,h+4,k);w32(u,h+8,MAN);u.mem_write(h+0xc,b'\1')
  methods=[[0x5d9dae,0x5d9e1c,0x5d9e31],[0x5d9c6b,0x5d9b62,0x5d9ba2],[0x5d9a9f,0x5d98ed,0x5d9741],[0x5d95a7,0x5d9704,0x5d90b5],[0x5d903f,0x5d9070,0x5d909a],[CB,CB+4,CB+8],[CB,CB+4,CB+8]][k]
  u.mem_write(v,struct.pack('<4I',*methods,0))
 w32(u,HAND+0x110,q['last_clock']);w32(u,HAND+0x114,q['last_stage']);w32(u,HAND+0x118,0xfedcba98)
 u.mem_write(HAND+0x210,bytes([q['flag10'],q['flag11']]))
 w32(u,0x882d3c,WORLD);u.mem_write(WORLD+0x1198,b'\0');w32(u,WORLD+0x119c,0)
 u.mem_write(0x9178d0,b'\0');u.mem_write(0x9178d2,b'\0');u.mem_write(0x926cfc,bytes([q['busy']]))
 u.mem_write(0x91abc8,b'\1');w32(u,0x91abcc,q['clock']);events=[];visits=0;clocks=0;clock=q['clock']
 def ret(v=None,pop=0):
  sp=u.reg_read(UC_X86_REG_ESP)
  if v is not None:u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
 def hook(_u,pc,size,_):
  nonlocal visits,clocks,clock
  visits+=1;sp=u.reg_read(UC_X86_REG_ESP)
  if pc==STOP:u.emu_stop();return
  if pc==0x6875d3:ret();return
  if pc==0x5ea803:events.append(['network']);ret();return
  if pc==0x5b07a0:events.append(['external',u32(u,sp+4)]);ret();return
  if pc==0x5b54cf:events.append(['null_close',u32(u,sp+4),u32(u,sp+8)]);ret();return
  if pc in (CB,CB+4,CB+8):
   kind=(u.reg_read(UC_X86_REG_ECX)-HAND)//0x100;events.append(['other_update' if pc==CB else 'other_enter' if pc==CB+4 else 'other_exit',kind]);ret(0,0 if pc==CB else 12 if pc==CB+4 else 4);return
  if pc in (0x5b3ca0,0x5b3cb4):
   clock=(clock+q['clock_step'])&0xffffffff;w32(u,0x91abcc,clock);clocks+=1
   events.append(['clock',digest(bytes(u.mem_read(OBJ,N)))]);return
  if pc==0x45906f:events.append(['seed']);ret(0x12345678);return
  if pc==0x5b5a84:
   assert u32(u,sp+4)==8;events.append(['random']);u.mem_write(u32(u,sp+8),struct.pack('<II',0xabcdef01,0x76543210))
   if q['random_state']!=0xffffffff:w32(u,OBJ+0x73a0,q['random_state'])
   ret();return
  if pc==0x5e0d4f:
   events.append(['busy'])
   if q['busy_state']!=0xffffffff:w32(u,OBJ+0x73a0,q['busy_state']);u.mem_write(OBJ+0x73a4,bytes([q['busy_transition']]))
   if q['busy_local']!=0xffffffff:u.mem_write(OBJ+0x4c74,bytes([q['busy_local']]))
   return
  if pc==0x5bed40:
   args=[u32(u,sp+4+4*i) for i in range(6)];events.append(['send',u.reg_read(UC_X86_REG_ECX),*args[:5],digest(bytes(u.mem_read(args[5],args[4])))]);ret(0,24);return
  if pc==0x5add8e:return
  if pc==0x4499a8:events.append(['complete']);return
  if pc==0x4497ea:events.append(['ready']);ret(0);return
  if pc==0x5d94a1:events.append(['map_change']);u.mem_write(u.reg_read(UC_X86_REG_ECX)+0x10,struct.pack('<II',0xabcdef01,0x76543210));ret(0,4);return
  if pc==0x5c503b:events.append(['termination_reason']);ret(q['termination_reason']);return
  if pc==0x615a7c:events.append(['notify',u32(u,sp+4)]);ret();return
  if pc==0x5cc0a8:events.append(['remote_close',u32(u,sp+4)]);w32(u,OBJ+0x73a0,0);ret(0,4);return
 u.hook_add(UC_HOOK_CODE,hook)
 def run(entry,this,args=()):
  sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*[x&0xffffffff for x in args]));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,this)
  try:u.emu_start(entry,STOP+1,count=300000)
  except Exception as e:raise RuntimeError(f'{q} PC={u.reg_read(UC_X86_REG_EIP):08x}') from e
  assert u.reg_read(UC_X86_REG_EIP)==STOP,q
  return u.reg_read(UC_X86_REG_EAX)&255
 op=q['op'];returned=None
 if op=='cycle':
  run(0x5d7738,MAN);run(0x5c96fb,OBJ,[1,10,0,0,0])
  for _ in range(15):run(0x5d7738,MAN)
 elif op=='pump':run(0x5d7738,MAN)
 elif op.startswith('update'):returned=run([0x5d9dae,0x5d9c6b,0x5d9a9f][int(op[-1])],HAND+int(op[-1])*0x100)
 elif op=='enter1':run(0x5d9b62,HAND+0x100,[HAND,0,0])
 elif op=='enter2':run(0x5d98ed,HAND+0x200,[HAND+0x100,0,0])
 elif op=='clear':returned=run(0x5c969f,OBJ)
 elif op=='write':returned=run(0x5c96fb,OBJ,[q['write_running'],q['write_stage'],q['write_flag'],q['write_reason'],WORLD])
 manager=[u32(u,MAN),*u.mem_read(MAN+0x34,3),u32(u,MAN+0x38),u32(u,MAN+0x3c),bytes(u.mem_read(MAN+0x40,32)).hex()]
 return dict(input=q,events=events,after=digest(bytes(u.mem_read(OBJ,N))),manager=manager,manager_hex=bytes(u.mem_read(MAN,0x100)).hex(),
 task=[u32(u,HAND+0x110),u32(u,HAND+0x114),u32(u,HAND+0x118),*u.mem_read(HAND+0x210,2)],pregame=digest(bytes(u.mem_read(0x8b00e8,0x840))),returned=returned,visits=visits)
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert digest(image)==SHA;rows=[]
 def add(**kw):
  q=dict(op='update1',kind=1,state=5,transition=0,phase=1,count=3,host=0,running=1,stage=0,flag=0,countdown_reason=0,reason_stage=0xffffffff,preflight=0,local_flag=0,prepared=0,players=3,status=3,last_clock=0,last_stage=0xffffffff,flag10=0,flag11=0x80,busy=0,clock=0x12340000,clock_step=1000,random_state=0xffffffff,busy_state=0xffffffff,busy_transition=0,busy_local=0xffffffff,termination_reason=1,write_running=1,write_stage=7,write_flag=0x80,write_reason=0,seed=19);q.update(kw);rows.append(execute(image,q))
 for op,state,phase in itertools.product(('update0','update1','update2','enter2','clear','write'),(*range(12),0xffffffff),(0,1,2,3,4,5,6,9,10,0xffffffff)):add(op=op,state=state,phase=phase)
 for stage,last,running,step in itertools.product((-2,-1,0,1,10),(0xffffffff,0,1,10),(0,0x80),(0,999,1000,1001,0x80000000)):add(stage=stage,last_stage=last,running=running,clock_step=step)
 for status,preflight,count,host in itertools.product((*range(7),0xffffffff),(0,1,2,0xffffffff),(0,1,3),(0,1)):add(status=status,preflight=preflight,count=count,host=host)
 for busy,local,flag10,count,state in itertools.product((0,0x80),(0,0x80),(0,0x80),(0,1,3),(3,5,7,8)):add(op='update2',kind=2,phase=2,busy=busy,local_flag=local,flag10=flag10,count=count,state=state)
 for state,phase in itertools.product((0,3,5,7,8),(1,2,4)):add(op='enter2',state=5,phase=2,random_state=state)
 for busy_state,transition,local in itertools.product((0,3,5,7,8),(0,0x80),(0,0x80)):add(op='update2',kind=2,phase=2,busy=1,busy_state=busy_state,busy_transition=transition,busy_local=local)
 for state,reason,host in itertools.product((3,5,7,8),(0,1,2),(0,1)):add(op='write',state=state,write_reason=reason,host=host)
 for state,stage,flag in itertools.product((3,4),(1,10),(0,0x80)):
  add(state=state,stage=stage,last_stage=stage,flag=flag)
 for count,players,prepared in itertools.product((1,3,16,17,18),(0,1,0xffff),(0,0x80)):
  add(op='update2',kind=2,phase=2,count=count,players=players,prepared=prepared)
 for busy,count,status in itertools.product((0,1),(0,1,3),(0,3,4)):add(op='cycle',kind=0,state=5,phase=1,running=0,busy=busy,count=count,status=status)
 for kind,state in itertools.product((0,1,2),(0,1,3,5,7,8,10)):add(op='pump',kind=kind,state=state,phase=1)
 add(op='enter1')
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_session_start_transition_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 report=dict(source_sha256=SHA,cases=len(rows),instruction_visits=sum(r['visits'] for r in rows),fixture_sha256=digest(fixture.read_bytes()),limits=['Original pump,0/1/2 handlers, countdown getters/commit, zero, parameter setters, player preparation and refresh execute unchanged.','Network IO, OS seed/random outputs, world map-change and network busy callback reentry are controlled services.','Complete session, manager, handler fields and pregame global are compared; no native gameplay or transport acknowledgement is claimed.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
