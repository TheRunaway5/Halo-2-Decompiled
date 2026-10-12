#!/usr/bin/env python3
"""Execute complete supplied5B333D/5B2F21 owned-bank reconciliation chain.

Real source selectors, insertion/update/removal, team choice, strings, cache,
language cache and clocks execute. Map-backed masks, protected context query,
authored random-name IO, optional admission object and transport IO remain
explicit services with retained exact arguments/results, not live substitutes.
"""
from pathlib import Path
import argparse,hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESP
from oracle_vista_session_owned_inputs import Oracle as BaseOracle,BASE,STOP,SESSION,BUFFER,NETWORK,STACK,SIZE,SHA,pattern,patches,pack,put,channels
USERS=0x91A628;RETAINED=0x91A908;CONTROLLERS=0xD6C858
WINDOWS={'reconciliation':(0x5B333D,0x5B34B8),'parameter_tail':(0x5B2F21,0x5B333D),
 'selector':(0x5B2224,0x5B23C3),'add_user':(0x5B2E34,0x5B2F21),'update_user':(0x5B1B6C,0x5B1BD8),
 'remove_user':(0x5B1B0F,0x5B1B6C),'reset':(0x5B07EC,0x5B0828),
 'join':(0x5CC3C2,0x5CC56B),'update':(0x5C851B,0x5C8693),'remove':(0x5CB280,0x5CB38B),
 'insert':(0x5CBC14,0x5CBD6B),'insert_body':(0x5C7555,0x5C7637),'erase_body':(0x5C7637,0x5C76D1),
 'team_choice':(0x5C31D8,0x5C33C2),'name_collision':(0x5B4D1A,0x5B4DB6),
 'language_cache':(0x4381FD,0x4382DD),'language_publish':(0x5C943F,0x5C9570),
 'final_payload_write':(0x5C5BE6,0x5C5C6B),'controller_team':(0x6068F2,0x606906)}
def name(b,o,text,capacity=32):
 raw=text.encode('utf-16le');raw=raw[:(capacity-1)*2]+b'\0\0';b[o:o+len(raw)]=raw
def input_banks(seed=43,state=5,active=1,membership=0,mode=1,teams=0,team=0xff,
                previous_team=0xff,changed_generation=0,changed_variant=1,limit=16,
                status='absent',name_value='Player',last=0,reserved=False,admission=False,slot=0,associated=False):
 b=pattern(SIZE,seed);users=pattern(0x2e0,seed);retained=pattern(0x278,seed);controllers=pattern(0x49f0,seed)
 for o,v,n in [(0x14,slot,4),(0x18,mode,4),(0x73a0,state,4),(0x73a4,0,1),(0x71c0,0,4),(0x74,0,4),(0x84,1,4),(0x64,0,4),
                (0x1254,membership.bit_count(),4),(0x1258,membership,4),(0x180,membership.bit_count(),4),(0x70,3,4),(0x7930,0xffffffff,4),
                (0x4c7c,16,4),(0x4c80,limit,4),(0x4c70,7,4),(0x4c60,0xffffffff,4),(0x7960,9,4),(0x7b78,BUFFER+0x1000 if admission else 0,4)]:put(b,o,v,n)
 b[0x7978:0x7b78]=bytes(0x200);b[0x4e3c:0x4f6c]=bytes(0x130);put(b,0x4e84,teams,1)
 for i in range(17):put(b,0x71c5+i*0x1c,0,1);put(b,0x71c8+i*0x1c,0xffffffff)
 if associated:put(b,0x71c5,1,1);put(b,0x71c8,2)
 for i in range(4):
  u=i*0xb8;put(users,u,int(bool(active&(1<<i))),1)
  users[u+1:u+9]=bytes([i+1,2,3,4,5,6,7,8]);put(users,u+0xc,i)
  users[u+0x10:u+0x94]=bytes(0x84);name(users,u+0x10,name_value+str(i) if name_value else '')
  put(users,u+0x8c,team,1);put(users,u+0x94,0x11223344+i);put(users,u+0x98,0x55667788+i)
  for field in (0x9c,0xa4,0xac):put(users,u+field,0);put(users,u+field+4,0);put(users,u+field+slot*4,last)
  put(users,u+0xb4,0,2);put(b,0x184+i*4,i if membership&(1<<i) else 0xffffffff)
  p=0x125c+i*0x128
  b[p:p+8]=users[u+1:u+9];put(b,p+8,0);put(b,p+0xc,i);put(b,p+0x14,0xffffffff if status=='pending' else i)
  b[p+0x18:p+0x9c]=users[u+0x10:u+0x94];b[p+0x9c:p+0x120]=bytes(0x84)
  if status=='unchanged':b[p+0x9c:p+0x120]=users[u+0x10:u+0x94]
  put(b,p+0x118,previous_team,1);put(b,p+0x120,0x11223344+i);put(b,p+0x124,0x55667788+i)
  if status=='changed':put(b,p+0x14,0x12345678)
  if reserved and i==0:
   put(b,0x7978,1,1);put(b,0x7979,0,1);b[0x7982:0x798a]=users[1:9]
 put(retained,slot*4,8 if changed_generation else 9);put(retained,8+slot*4,last);put(retained,0x10+slot*4,0 if changed_variant else 3)
 offset=0x18+slot*0x130;retained[offset:offset+0x130]=b[0x4e3c:0x4f6c] if not changed_variant else bytes([0x43])*0x130
 return b,users,retained,controllers
class Oracle(BaseOracle):
 def hook(self,uc,pc,size,_):
  special=pc in (0x5B3CB4,0x5B3CA0,0x4381FD,0x5B087B,0x5B0A25,0x61A995,0x5B4C3B,BUFFER+0x1200,0x5BED40)
  if not special:return super().hook(uc,pc,size,_)
  self.visits+=1;s=uc.reg_read(UC_X86_REG_ESP)
  if pc in (0x5B3CB4,0x5B3CA0):
   value=(self.clock_base+self.clock_step*self.clock_count)&0xffffffff;self.clock_count+=1
   self.events.append(dict(kind='clock',value=value));uc.mem_write(0x91ABCC,pack(value));return
  if pc==0x4381FD:self.events.append(dict(kind='language',value=self.language));return
  if pc in (0x5B087B,0x5B0A25):
   key='allowed' if pc==0x5B087B else 'primary_allowed';self.events.append(dict(kind=key,value=self.mask));self.ret(self.mask);return
  if pc==0x61A995:self.events.append(dict(kind='context',value=self.context));self.ret(self.context);return
  if pc==BUFFER+0x1200:
   id=bytes(uc.mem_read(self.get(s+4),8)).hex();self.events.append(dict(kind='admission',owner=uc.reg_read(UC_X86_REG_ECX),identity=id,value=self.admission));self.ret(self.admission,4);return
  if pc==0x5B4C3B:
   dest,n=self.get(s+4),self.get(s+8);assert n==32
   text=self.random_names[min(self.random_count,len(self.random_names)-1)];self.random_count+=1
   self.events.append(dict(kind='random_name',value=text))
   if text is not None:uc.mem_write(dest,text.encode('utf-16le')[:62]+b'\0\0')
   self.ret(text is not None);return
  args=[self.get(s+i*4) for i in range(1,7)];assert args[2]==0
  self.events.append(dict(kind='message',owner=uc.reg_read(UC_X86_REG_ECX),slot=args[0],channel=args[1],id=args[3],payload=bytes(uc.mem_read(args[5],args[4])).hex()));self.ret(None,24)
 def run(self,kind,banks,*,mask=0xff,context=0,language=3,clock_base=1000,clock_step=113,admission=True,random_names=None):
  b,users,retained,controllers=banks;uc=self.uc
  for address,data in ((SESSION,b),(USERS,users),(RETAINED,retained),(CONTROLLERS,controllers)):uc.mem_write(address,bytes(data))
  uc.mem_write(SESSION+SIZE,b'\xa5'*32)
  uc.mem_write(STACK,b'\xa5'*0x10000);esp=STACK+0x8000
  uc.mem_write(esp,pack(STOP,*([SESSION] if kind=='R' else [])));uc.reg_write(UC_X86_REG_ESP,esp);uc.reg_write(UC_X86_REG_ECX,SESSION)
  uc.mem_write(0x820FC0,b'\1');uc.mem_write(0x820FE8,pack(SESSION));uc.mem_write(0x820FF4,pack(NETWORK));uc.mem_write(NETWORK,bytes(0x10000))
  uc.mem_write(0x91ABC8,b'\1');uc.mem_write(0x881908,b'\1');uc.mem_write(0x812818,pack(language))
  uc.mem_write(0x926C88,pack(BUFFER+0x100));uc.mem_write(BUFFER+0x100,pack(0));uc.mem_write(0x926D08,pack(0xdeadbeef));uc.mem_write(0x926D04,b'\x37')
  uc.mem_write(0x91A592,bytes(96));uc.mem_write(0x91A592,'Actual Machine'.encode('utf-16le'));uc.mem_write(0x91A5B2,'Actual Party'.encode('utf-16le'))
  uc.mem_write(0x91A5F2,b'\1');uc.mem_write(0x91A608,pack(0x11223344));uc.mem_write(0x91A614,pack(0x99aabbcc));uc.mem_write(0x91A620,pack(2));uc.mem_write(0x91A624,pack(100))
  for a in (0x8F8DCC,0x8F8DD0,0x8F8DD4,0x8F8DD8):uc.mem_write(a,pack(100))
  uc.mem_write(BUFFER+0x1000,pack(BUFFER+0x1100));uc.mem_write(BUFFER+0x110c,pack(BUFFER+0x1200))
  self.events=[];self.mask=mask;self.context=context;self.language=language;self.clock_base=clock_base;self.clock_step=clock_step;self.clock_count=0;self.admission=admission;self.random_names=random_names or ['Authored Name'];self.random_count=0
  before=self.visits;uc.emu_start(0x5B333D if kind=='R' else 0x5B2F21,STOP+1,count=500000)
  assert uc.reg_read(UC_X86_REG_EIP)==STOP
  assert uc.mem_read(SESSION+SIZE,32)==b'\xa5'*32
  return dict(after_patches={name:patches(before,bytes(uc.mem_read(address,len(before)))) for name,address,before in [('session',SESSION,b),('users',USERS,users),('retained',RETAINED,retained),('controllers',CONTROLLERS,controllers)]},events=self.events,instruction_visits=self.visits-before)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'));p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_session_reconciliation_original.jsonl'));p.add_argument('--directory',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/reconciliation-original'));a=p.parse_args();image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
 a.directory.mkdir(parents=True,exist_ok=True);o=Oracle(image);records=[]
 def add(kind,label,banks,seed=43,**services):
  result=o.run(kind,banks,**services);before={name:patches(pattern(len(data),seed),data) for name,data in zip(('session','users','retained','controllers'),banks)}
  records.append(dict(kind=kind,label=label,seed=seed,before_patches=before,services=services,**result))
 for state,active,membership,generation in itertools.product((0,2,3,4,5,6,7,8,9,10,0xffffffff),(0,1,0xf),(0,1),(0,1)):
  add('R',f'flow_{len(records):04d}',input_banks(state=state,active=active,membership=membership,changed_generation=generation))
 for status,active,team,previous,mode,mask in itertools.product(('pending','changed','unchanged'),(0,1,0xf),(0xff,0,7),(0xff,0),(1,2),(0,3,0xff)):
  add('R',f'users_{len(records):04d}',input_banks(active=active,membership=0xf,status=status,teams=1,team=team,previous_team=previous,mode=mode),mask=mask,context=0x80)
 for limit,reserved,admission,result,last in itertools.product((0,1,16),(False,True),(False,True),(False,True),(0,950)):
  add('R',f'join_{len(records):04d}',input_banks(limit=limit,reserved=reserved,admission=admission,last=last),admission=result,clock_base=1000,clock_step=1)
 for mode,teams,team,previous,changed in itertools.product((1,2),(0,1),(0xff,0,7,0x80),(0xff,0),(0,1)):
  add('F',f'final_{len(records):04d}',input_banks(membership=0xf,mode=mode,teams=teams,team=team,previous_team=previous,changed_variant=changed),mask=3)
 for value,random in [('',[None]),('Duplicate',['Duplicate0','Alternate']),('Same',['Same0'])]:
  add('R',f'names_{len(records):04d}',input_banks(active=0xf,name_value=value),random_names=random)
 for slot,state,active,membership,status in itertools.product((0,1),(3,5,7,8),(0,1,0xf),(0,0xf),('pending','changed','unchanged')):
  add('R',f'owned_tail_{len(records):04d}',input_banks(slot=slot,state=state,active=active,membership=membership,status=status,associated=True,changed_generation=1))
 for slot,mask,clock,last in itertools.product((0,1),(0,1,3,0x80,0xff),(0,0xfffffff0,0x80000000),(0,950,0xfffffff0)):
  add('R',f'join_team_clock_{len(records):04d}',input_banks(slot=slot,active=0xf,teams=1,last=last),mask=mask,clock_base=clock,clock_step=17)
 for random in ([None],['Duplicate','Alternate'],['Duplicate']):
  banks=input_banks(active=0,membership=0xf)
  for i in range(4):name(banks[0],0x125c+i*0x128+0x18,'Duplicate')
  add('F',f'collision_{len(records):04d}',banks,random_names=random)
 for pending_match in (False,True):
  banks=input_banks(last=950)
  put(banks[0],0x793c,1,1);banks[0][0x793d:0x7945]=banks[1][1:9] if pending_match else bytes(8)
  add('R',f'pending_retry_{len(records):04d}',banks,clock_base=1000,clock_step=1)
 for slot,state in itertools.product((0,1),(7,8)):
  banks=input_banks(slot=slot,state=state,changed_generation=1);put(banks[0],0x73a4,1,1)
  add('R',f'parameter_gate_{len(records):04d}',banks)
 for slot,state in itertools.product((0,1),(0,3,4,6,7,8,10,0xffffffff)):
  add('F',f'final_state_{len(records):04d}',input_banks(slot=slot,state=state,membership=0xf))
 a.fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in records));m=Cs(CS_ARCH_X86,CS_MODE_32);windows={};asm=[]
 for window_name,(lo,hi) in WINDOWS.items():
  raw=image[lo-BASE:hi-BASE];windows[window_name]=dict(begin=hex(lo),end_exclusive=hex(hi),sha256=hashlib.sha256(raw).hexdigest(),bytes=raw.hex());asm.append(f';{window_name} {lo:08X}..{hi:08X}');asm.extend(f'{x.address:08X} {x.mnemonic} {x.op_str}' for x in m.disasm(raw,lo))
 (a.directory/'source.asm').write_text('\n'.join(asm)+'\n');report=dict(source_sha256=SHA,case_count=len(records),instruction_visits=o.visits,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),windows=windows,
 boundaries=['Unchanged full5B333D and5B2F21 owned-bank execution including real insertion/update/removal/selectors/cache/string/team choice/controller setter/language cache/clock instructions.',
 'Declared actual-service results only:map-dependent team masks,protected61A995 context query,authored random-name output,optional admission object,transport IO.',
 'Full source banks and exact ordered service operands/results are captured;no native runtime/account/readiness/network delivery is fabricated.'])
 (a.directory/'execution.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('case_count','instruction_visits','fixture_sha256')}))
if __name__=='__main__':main()
