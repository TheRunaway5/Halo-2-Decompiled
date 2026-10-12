#!/usr/bin/env python3
"""Run unchanged supplied Vista PC14 status, button-key and tuple owners.

UI strings/lookups and named session/readiness queries are declared service
boundaries.5B0B79,5AD736,5C2843,5C20C8,61553C,643BE8 and setters execute.
"""
from pathlib import Path
import hashlib,itertools,json,struct,sys
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_pcr_screen import machine,STOP,STACK,OBJ,SHA,u32,w32,wide,string,signed
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/frontend/vista-session-flow-audit/pc14-status-original'
INDICES=(-1,1,2,45,52,53,54,55)
TEXT=OBJ+0x4000;BUFFERS=OBJ+0x9000;SESSION=OBJ+0xe000;MEMBERS=SESSION+0x70
RECORD=SESSION+0x4ae0;SCREEN_VTABLE=OBJ+0x2000;TEXT_VTABLE=OBJ+0x8000;BUFFER_VTABLE=OBJ+0xb000
GET_BUFFER=OBJ+0x8100;CLEAR=OBJ+0xb100;REFRESH=OBJ+0x2100;SCREEN_A0=OBJ+0x2200
VARIANT=OBJ+0xc000;IDENTITY=OBJ+0xc100;RAW=OBJ+0xc200
def original_record(q):
 b=bytearray((i*37+q['seed']*13)&255 for i in range(0xa0))
 struct.pack_into('<i',b,4,q['kind']);struct.pack_into('<i',b,0x88,q['error'])
 ids=[bytes((i*17+j*29+q['seed'])&255 for j in range(8)) for i in range(16)]
 b[8:16]=bytes([255]*8) if q['match0']<0 else ids[q['match0']]
 b[0x8c:0x94]=bytes([254]*8) if q['match1']<0 else ids[q['match1']]
 if q['duplicate']>=0:ids[q['duplicate']]=bytes(b[8:16])
 return bytes(b),ids
def execute(image,q):
 u=machine(image);events=[];visits=0;sp=STACK+0x18000
 record,ids=original_record(q);u.mem_write(RECORD,record)
 w32(u,0x820fe8,SESSION);u.mem_write(0x820fc0,bytes([q['available']]));w32(u,SESSION+0x73a0,q['session_nonzero'])
 w32(u,MEMBERS+0x11e8,q['mask'])
 for i,id in enumerate(ids):u.mem_write(MEMBERS+0x11ec+i*0x128,id)
 w32(u,OBJ,SCREEN_VTABLE);w32(u,SCREEN_VTABLE+0x60,REFRESH);w32(u,SCREEN_VTABLE+0xa0,SCREEN_A0)
 w32(u,TEXT_VTABLE+0x4c,GET_BUFFER);w32(u,BUFFER_VTABLE+4,CLEAR)
 for i in range(len(INDICES)):
  a=TEXT+i*0x100;b=BUFFERS+i*0x80
  w32(u,a,TEXT_VTABLE);w32(u,b,BUFFER_VTABLE)
  u.mem_write(a+0x6e,bytes([q['visible'][i]]));u.mem_write(a+0x20,struct.pack('<4h',*q['bounds'][i]));u.mem_write(b+0x14,struct.pack('<H',q['buffer_flags'][i]))
 u.mem_write(OBJ+0x3484,struct.pack('<h',q['pane']));u.mem_write(OBJ+0x3579,bytes([q['dialog']]))
 w32(u,0xd78e00,q['previous_kind']);u.mem_write(0xd73abc,bytes([q['factory']]))
 u.mem_write(VARIANT+0x48,bytes([q['variant48']]));wide(u,RAW,'original controlled custom string')
 #P+50 is not initialized by644E0F; pin that exact external stack seed.
 w32(u,sp-12,q['stack_seed'])
 def ret(v=0,pop=0):
  s=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,v&0xffffffff)
  u.reg_write(UC_X86_REG_EIP,u32(u,s));u.reg_write(UC_X86_REG_ESP,s+4+pop)
 def text_index(a):
  i=(a-TEXT)//0x100;assert 0<=i<len(INDICES) and a==TEXT+i*0x100,hex(a);return INDICES[i]
 def event(k,*args):events.append([k,*args])
 def byte_service(pc,name,key):
  if pc==address[name]:event(name,q[key]);ret(q[key]);return True
  return False
 address={'active':0x616ba2,'favorite':0x6156b9,'multiplayer':0x615fd9,'reset_dialog':0x61553f,
  'campaign':0x615ff6,'state2':0x6156a1,'state9':0x615246,'team_change':0x6168fc,'factory':0x60b365}
 def hook(_u,pc,_n,_):
  nonlocal visits
  visits+=1;s=u.reg_read(UC_X86_REG_ESP);c=u.reg_read(UC_X86_REG_ECX)
  if pc==STOP:u.emu_stop()
  elif pc==0x6875d3:ret()
  elif pc==0x5b0b79:event('query') #Full original query executes.
  elif pc==0x5ad736:event('record',q['available'],q['session_nonzero']) #Actual session wrapper executes.
  elif pc==0x5c20c8:
   assert c==SESSION and [u32(u,s+4),u32(u,s+8)]==[0,0]
   event('peers') #Actual session+70 getter executes unchanged.
  elif pc==0x611973:
   i=signed(u32(u,s+4));assert i in INDICES
   exists=q['text_exists'][INDICES.index(i)];event('text',i,exists);ret(TEXT+INDICES.index(i)*0x100 if exists else 0,4)
  elif pc==GET_BUFFER:ret(BUFFERS+INDICES.index(text_index(c))*0x80)
  elif pc==CLEAR:
   assert string(u,u32(u,s+4))=='';i=(c-BUFFERS)//0x80;event('clear',INDICES[i]);ret(0,4)
  elif pc in (0x61bf85,0x61bff6):event('set' if pc==0x61bf85 else 'append',text_index(c),u32(u,s+4));ret(0,4)
  elif pc==0x61baa9:event('raw',text_index(c),string(u,u32(u,s+4)));ret(0,4)
  elif pc==REFRESH:event('refresh');ret()
  elif pc==SCREEN_A0:event('screen_a0',signed(u32(u,s+4)));ret(0,4)
  elif pc==0x6116d2:event('bounds',text_index(c),list(struct.unpack('<4h',u.mem_read(u32(u,s+4),8))))
  elif pc==0x6152b5:
   dst=u32(u,s+4);event('identity',u32(u,dst),u32(u,dst+4),q['identity']);u.mem_write(dst,struct.pack('<II',0x12345678,0x9abcdef0));ret(q['identity'])
  elif pc==0x5b3e98:
   #60967F ->5B0359 executes its actual global917970 getter. The identity
   #argument is consumed by this thiscall, not by that zero-argument getter.
   assert c==0x917970 and list(struct.unpack('<II',u.mem_read(u32(u,s+4),8)))==[0x12345678,0x9abcdef0]
   event('favorite_identity',q['favorite_identity']);ret(q['favorite_identity'],4)
  elif pc==0x616490:assert u32(u,s+4)==0;event('readiness',q['readiness']);ret(q['readiness'])
  elif pc==0x64499f:event('status7_dialog');u.mem_write(OBJ+0x3579,bytes([q['dialog_result']]));ret()
  elif pc==0x61678c:event('status_string',signed(u32(u,s+4)),q['status_sid']);ret(q['status_sid'])
  elif pc==0x4d108f:event('custom_map',u32(u,s+4),q['custom_map']);ret(q['custom_map'])
  elif pc==0x4d10bb:
   event('custom_text',u32(u,s+4)&255,u32(u,s+8)&255,q['custom_text']);ret(RAW if q['custom_text'] else 0)
  elif pc==0x61525a:event('count',q['count']);ret(q['count'])
  elif pc==0x615214:event('maximum',q['maximum']);ret(q['maximum'])
  elif pc==0x6101e9:
   assert [u32(u,s+4),u32(u,s+8)]==[0,3];event('overflow_string',q['overflow_sid']);ret(q['overflow_sid'],8)
  elif pc==0x61569c:event('user',q['user']);ret(q['user'])
  elif pc==0x615885:event('user_ready',signed(u32(u,s+4)),q['user_ready']);ret(q['user_ready'])
  elif pc==0x643b35:event('map_team_limit',q['map_team_limit']);ret(q['map_team_limit'])
  else:
   for name,key in [('active','active'),('favorite','favorite'),('multiplayer','multiplayer'),('reset_dialog','reset_dialog'),
    ('campaign','campaign'),('state2','state2'),('state9','state9'),('team_change','team_change'),('factory','factory')]:
    if byte_service(pc,name,key):break
 u.hook_add(UC_HOOK_CODE,hook)
 if q['op']=='tuple':
  outputs=OBJ+0xd000;initial=[-7,-8,0,0,-9,-10,0x1111,0x2222,0x3333,0x44444444]
  for i,v in enumerate(initial):w32(u,outputs+i*8,v)
  args=[outputs+i*8 for i in range(10)];entry=0x5b0b79
 else:
  args=[3,VARIANT if q['variant_present'] else 0,0,0,1,q['host']];entry=0x644d0d if q['op']=='caption' else 0x644e0f
 u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,OBJ)
 u.emu_start(entry,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP,(q,hex(u.reg_read(UC_X86_REG_EIP)))
 tuple_out=None;success=None
 if q['op']=='tuple':
  tuple_out=[u32(u,outputs+i*8) for i in range(10)]
  for i in [0,1,4,5]:tuple_out[i]=signed(tuple_out[i])
  for i in [2,3]:tuple_out[i]=None if not tuple_out[i] else tuple_out[i]-RECORD
  for i in [6,7,8]:tuple_out[i]&=65535
  success=u.reg_read(UC_X86_REG_EAX)&255
 return dict(input=q,events=events,tuple=tuple_out,success=success,visits=visits,
  state=dict(visible=[u.mem_read(TEXT+i*0x100+0x6e,1)[0] for i in range(len(INDICES))],
   bounds=[list(struct.unpack('<4h',u.mem_read(TEXT+i*0x100+0x20,8))) for i in range(len(INDICES))],
   buffer_flags=[struct.unpack('<H',u.mem_read(BUFFERS+i*0x80+0x14,2))[0] for i in range(len(INDICES))],
   dialog=u.mem_read(OBJ+0x3579,1)[0],previous_kind=signed(u32(u,0xd78e00))))
def main():
 image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 rows=[]
 def add(**args):
  q=dict(op='status',seed=3,kind=0,error=0,available=0x80,session_nonzero=1,mask=0xffff,match0=7,match1=12,duplicate=-1,
   pane=0,visible=[1]*8,bounds=[[50+i,100+i,-30-i,400+i] for i in range(8)],buffer_flags=[0xa5a4]*8,text_exists=[1]*8,
   dialog=0x80,previous_kind=3,factory=0x80,stack_seed=0x89abcdef,variant48=1,variant_present=1,host=0x80,active=0,favorite=0,
   identity=0x80,favorite_identity=0,multiplayer=0x80,readiness=9,reset_dialog=0,dialog_result=1,status_sid=0x240001f0,
   custom_map=0x80,custom_text=1,count=1,maximum=16,overflow_sid=0x12345678,user=-1,user_ready=0,state2=0,campaign=0,state9=0,
   team_change=0,map_team_limit=4)
  q.update(args)
  try:rows.append(execute(image,q))
  except Exception:
   print('failed_input',json.dumps(q),file=sys.stderr);raise
 for available,nonzero,kind,match0,match1 in itertools.product((0,0x80),(0,1),(-1,0,3,4,2147483647),(-1,0,7,15),(-1,0,12,15)):
  add(op='tuple',available=available,session_nonzero=nonzero,kind=kind,match0=match0,match1=match1)
 for mask,duplicate in itertools.product((0,1,1<<7,1<<15,0xffff),(0,3,15)):
  add(op='tuple',kind=4,mask=mask,duplicate=duplicate)
 for exists,active,host,favorite,identity,favorite_identity,variant,flags,multiplayer in itertools.product(
  (0,1),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,1),(0,1),(0,0x80)):
  e=[1]*8;e[1]=exists
  add(op='caption',text_exists=e,active=active,host=host,favorite=favorite,identity=identity,favorite_identity=favorite_identity,
   variant_present=variant,variant48=flags,multiplayer=multiplayer)
 for pane,available,nonzero in itertools.product((-1,0,1,2,3),(0,0x80),(0,1)):add(pane=pane,available=available,session_nonzero=nonzero)
 for code in (-2147483648,-1,*range(28),2147483647):
  for flags,reset in itertools.product((0xa5a4,0xa5a5),(0,0x80)):add(readiness=code,buffer_flags=[flags]*8,reset_dialog=reset)
 for code,campaign,host,team_change,state9,state2,user,user_ready,limit in itertools.product(
  (8,9,11),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(0,0x80),(-1,0,3),(0,0x80),(1,3,8,9)):
  add(readiness=code,campaign=campaign,host=host,team_change=team_change,state9=state9,state2=state2,user=user,user_ready=user_ready,map_team_limit=limit)
 for kind,error,previous,team,factory in itertools.product((-1,0,1,2,3,4,5),(-1,0,1,2,3,4),(-1,0,1,2,3,4,5),(0,0x80),(0,0x80)):
  add(kind=kind,error=error,previous_kind=previous,team_change=team,factory=factory)
 for code,count,max,custom,raw,seed in itertools.product((10,22),(-1,1,17),(-1,16),(0,0x80),(0,1),(0,0x89abcdef)):
  add(readiness=code,count=count,maximum=max,status_sid=0x0f0001fd,custom_map=custom,custom_text=raw,stack_seed=seed)
 #Null status bypasses tuple, but the final team-change/button caption owner runs.
 for pane,team,factory in itertools.product((-1,0,1,2,3),(0,0x80),(0,0x80)):
  e=[1]*8;e[0 if pane not in (0,1,2) else 2]=0;add(pane=pane,text_exists=e,team_change=team,factory=factory)
 OUT.mkdir(parents=True,exist_ok=True);fixture=ROOT/'tests/fixtures/vista_lobby_status_original.json'
 fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
 sys.path.insert(0,str(ROOT/'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
 from audit import PE,EXE,EXE_SHA
 p=PE(EXE,EXE_SHA);spans={k:p.span(a,b) for k,a,b in [('tuple',0x5b0b79,0x5b0e43),('session',0x5ad736,0x5ad75c),
  ('record',0x5c2843,0x5c284a),('membership',0x5c20c8,0x5c20eb),('caption',0x644d0d,0x644e0f),('status',0x644e0f,0x64534c),('style',0x643be8,0x643c25),
  ('map_identity_stub',0x61553c,0x61553f),('custom_text_consumer',0x4d10bb,0x4d110b)]}
 report=dict(source_sha256=SHA,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),cases=len(rows),instructions=sum(r['visits'] for r in rows),
  fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),source_spans=spans,
  limits=['Full5B0B79/5AD736/5C2843/5C20C8 execute, including actual session+70 membership and exact (0,0) arguments.',
   'Full644D0D/644E0F and643BE8 execute. UI string setters/lookups, readiness and named session queries remain controlled services.',
   '61553C executes its actual false/no-write stub. Its caller stack seed is explicit;4D10BB(false,...) ignores the second boolean for string selection.',
   'No genuine account/session owner binding, native rendering, roster publication, pixel match or complete Vista transition is established.'])
 (OUT/'original.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='source_spans'}))
if __name__=='__main__':main()
