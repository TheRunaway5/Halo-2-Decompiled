#!/usr/bin/env python3
"""Unchanged Vista storage registration and full local-peer publication.

All source initialization, connectivity/statistic, strings, publisher and clock
instructions execute. Only literal memory services and outbound transport IO
are intercepted. All session/channel/name/clock/owner inputs are explicit.
"""
from pathlib import Path
import argparse,hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDI,UC_X86_REG_EIP,UC_X86_REG_ESP
SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BASE=0x400000;STOP=0x1800000;SESSION=0x1A00000;BUFFER=0x1A10000;NETWORK=0x1A20000;STACK=0x1900000;SIZE=0x7b88
WINDOWS={'initialize':(0x5C1E65,0x5C2054),'register6':(0x5E5996,0x5E59A3),'register5':(0x5BD60C,0x5BD629),
 'payload':(0x5B2C51,0x5B2E34),'connectivity':(0x5B20EB,0x5B2224),'publisher':(0x5C83F3,0x5C851B),
 'statistic':(0x5E0D55,0x5E0E62),'channel_getter':(0x5C5B4D,0x5C5B82),
 'membership_getter':(0x5C20C8,0x5C20EB),'network_owner':(0x5AD7B7,0x5AD7D0),
 'bandwidth':(0x5E0D39,0x5E0D4F),'elapsed_clock':(0x5B3CB4,0x5B3CD1),
 'clock':(0x5B3CA0,0x5B3CB4),'string':(0x44BD27,0x44BD45),'cookie':(0x6875D3,0x6875DD)}
def word(v):return v&0xffffffff
def pack(*v):return struct.pack('<'+'I'*len(v),*map(word,v))
def put(b,o,v,n=4):b[o:o+n]=pack(v)[:n]
def pattern(n,seed):return bytearray((i*37+(i>>8)*17+seed)&255 for i in range(n))
def patches(before,after):
 out=[];i=0
 while i<len(before):
  if before[i]==after[i]:i+=1;continue
  lo=i;i+=1
  while i<len(before) and before[i]!=after[i]:i+=1
  out.append([lo,after[lo:i].hex()])
 return out
def bank(seed=19,state=5,count=1,local=0,present=0):
 b=pattern(SIZE,seed)
 for o,v,n in [(0x73a0,state,4),(0x84,count,4),(0x71c0,local,4),(0x14,0,4),(0x64,0,4),(0x70,0xfffffffe,4),(0x7930,0xffffffff,4)]:put(b,o,v,n)
 for i in range(17):
  put(b,0xac+i*0x10c,present,1);put(b,0x71c5+i*0x1c,0,1);put(b,0x71c8+i*0x1c,0xffffffff)
 return b
def channels(b,count,local,flags=0x10,status=7,samples=None):
 c=bytearray(4*0x740)
 for i in range(max(0,count)):
  if i==local:continue
  index=i%4;put(b,0x71c5+i*0x1c,1,1);put(b,0x71c8+i*0x1c,index)
  put(c,index*0x740+0x80,status);put(c,index*0x740+0x88,flags,1)
  put(c,index*0x740+0x100,(samples or [25,300,2001,-1])[index])
 return c
class Oracle:
 def __init__(self,image):
  self.uc=Uc(UC_ARCH_X86,UC_MODE_32);self.uc.mem_map(BASE,(len(image)+4095)&~4095);self.uc.mem_write(BASE,image)
  for a,n in ((STOP,0x1000),(SESSION,0x10000),(BUFFER,0x10000),(NETWORK,0x10000),(STACK,0x10000)):self.uc.mem_map(a,n)
  self.uc.hook_add(UC_HOOK_CODE,self.hook);self.visits=0
 def get(self,a):return struct.unpack('<I',self.uc.mem_read(a,4))[0]
 def ret(self,value=None,pop=0):
  s=self.uc.reg_read(UC_X86_REG_ESP)
  if value is not None:self.uc.reg_write(UC_X86_REG_EAX,word(value))
  self.uc.reg_write(UC_X86_REG_EIP,self.get(s));self.uc.reg_write(UC_X86_REG_ESP,s+4+pop)
 def hook(self,uc,pc,size,_):
  self.visits+=1;s=uc.reg_read(UC_X86_REG_ESP)
  if pc==STOP:uc.emu_stop();return
  if pc==0x687BA9:
   dst,v,n=(self.get(s+i) for i in (4,8,12));uc.mem_write(dst,bytes([v&255])*n);self.ret(dst);return
  if pc in (0x687E18,0x689C3C):
   dst,src,n=(self.get(s+i) for i in (4,8,12));uc.mem_write(dst,bytes(uc.mem_read(src,n)));self.ret(dst);return
  if pc==0x5B2C98:self.built=True
  if pc==0x5B3CB4:self.elapsed_calls+=1;uc.mem_write(0x91ABCC,pack(self.elapsed))
  if pc==0x5B3CA0:self.publication_calls+=1;uc.mem_write(0x91ABCC,pack(self.publication))
  if pc==0x5C83F3:self.attempted=True
  if pc==0x5B2E0C:self.accepted=bool(uc.reg_read(UC_X86_REG_EAX)&255)
  if pc==0x5BED40:
   args=[self.get(s+i*4) for i in range(1,7)]
   assert args[2:5]==[0,0x1a,0xd0]
   self.message=dict(transport_owner=uc.reg_read(UC_X86_REG_ECX),session_slot=args[0],channel=args[1],payload_hex=bytes(uc.mem_read(args[5],0xd0)).hex())
   self.ret(None,24)
 def execute(self,kind,before,**kw):
  uc=self.uc;uc.mem_write(SESSION,bytes(before));uc.mem_write(SESSION+SIZE,b'\xa5'*32)
  owners=pattern(0x1000,kw.get('seed',19));uc.mem_write(NETWORK,bytes(owners))
  self.built=False;self.attempted=False;self.accepted=False;self.elapsed_calls=0;self.publication_calls=0;self.message=None
  self.elapsed=kw.get('elapsed',1000);self.publication=kw.get('publication',2000)
  uc.mem_write(STACK,b'\xa5'*0x10000);esp=STACK+0x8000
  if kind=='S':
   args=[kw['slot'],kw['mode'],kw['network_slot'],BUFFER+0x200,NETWORK,NETWORK+0x400,NETWORK+0x800]
   entry=0x5C1E65
  else:
   uc.mem_write(NETWORK,bytes(kw['channels'])+bytes(0x10000-len(kw['channels'])))
   uc.mem_write(0x820FC0,b'\1');uc.mem_write(0x820FF4,pack(NETWORK));uc.mem_write(0x926C88,pack(BUFFER+0x100));uc.mem_write(BUFFER+0x100,pack(kw.get('scale',0)))
   if kind=='C':entry=0x5B20EB;args=[SESSION,BUFFER]
   else:
    entry=0x5B2C51;args=[]
    uc.mem_write(0x91A592,bytes(96));uc.mem_write(0x91A592,kw['name'].encode('utf-16le')+b'\0\0');uc.mem_write(0x91A5B2,kw['party'].encode('utf-16le')+b'\0\0')
    uc.mem_write(0x91A5F2,bytes([kw['optional']]));
    for a,v in [(0x91A608,kw['metadata68']),(0x91A604,0x12345678),(0x91A614,kw['metadata70']),(0x91A620,kw['map_status']),(0x91A624,kw['map_progress']),(0x926D08,kw['bandwidth']),(0x91A910,kw['last']),(0x8F8DCC,kw['threshold'])]:uc.mem_write(a,pack(v))
    uc.mem_write(0x926D04,b'\x37');uc.mem_write(0x91ABC8,b'\1')
  uc.mem_write(esp,pack(STOP,*args));uc.reg_write(UC_X86_REG_ESP,esp);uc.reg_write(UC_X86_REG_ECX,SESSION)
  if kind=='P':uc.reg_write(UC_X86_REG_EDI,SESSION);uc.reg_write(UC_X86_REG_ECX,SESSION+0x88+self.get(SESSION+0x71C0)*0x10c)
  beforevisits=self.visits;uc.emu_start(entry,STOP+1,count=200000)
  assert uc.reg_read(UC_X86_REG_EIP)==STOP
  assert uc.mem_read(SESSION+SIZE,32)==b'\xa5'*32
  result=dict(after_patches=patches(before,bytes(uc.mem_read(SESSION,SIZE))),instruction_visits=self.visits-beforevisits)
  if kind=='S':result.update(al=uc.reg_read(UC_X86_REG_EAX)&255,owner_patches=patches(owners,bytes(uc.mem_read(NETWORK,0x1000))))
  if kind=='C':result.update(connectivity=[self.get(BUFFER+i*4) for i in range(4)])
  if kind=='P':result.update(built=self.built,attempted=self.attempted,accepted=self.accepted,elapsed_calls=self.elapsed_calls,publication_calls=self.publication_calls,last_clock=self.get(0x91A910),payload_hex=bytes(uc.mem_read(esp-0xcc,200)).hex() if self.built else None,message=self.message)
  return result
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'));p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_session_owned_inputs_original.jsonl'));p.add_argument('--directory',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/owned-inputs-original'));a=p.parse_args()
 image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SHA;a.directory.mkdir(parents=True,exist_ok=True);o=Oracle(image);records=[]
 def add(kind,label,b,seed=19,**kw):
  original=o.execute(kind,b,seed=seed,**kw)
  inputs={k:(v.hex() if isinstance(v,(bytes,bytearray)) else v) for k,v in kw.items()}
  for k in ('name','party'):
   if k in kw:inputs[k+'_utf16_hex']=kw[k].encode('utf-16le').hex()
  record=dict(kind=kind,label=label,seed=seed,before_patches=patches(pattern(SIZE,seed),b),inputs=inputs,**original);records.append(record);return original
 for slot,mode,networkslot,seed in itertools.product((0,1,7),(0,1,0xffffffff),(0,1),(19,0xa5)):
  add('S',f'storage_{len(records):04d}',pattern(SIZE,seed),seed,slot=slot,mode=mode,network_slot=networkslot)
 for count,local,flags,status,scale in itertools.product((-1,0,1,2,5,17),(0,1),(0,0x10),(0,7),(0,0x3dcccccd,0xbf000000)):
  b=bank(count=count,local=local);c=channels(b,count,local,flags,status)
  add('C',f'connectivity_{len(records):04d}',b,channels=c,scale=scale)
 defaults=dict(optional=1,metadata68=0x11223344,metadata70=0x99aabbcc,map_status=2,map_progress=100,bandwidth=0xdeadbeef,name='Local Machine',party='Local Party',last=0,threshold=100,elapsed=1000,publication=2000,scale=0)
 for state,local,optional in itertools.product((0,2,3,4,5,6,7,8,9,10,0xffffffff),(0,1),(0,1,0x80)):
  b=bank(state=state,count=2,local=local);c=channels(b,2,local);put(b,0x71c5,1,1);put(b,0x71c8,0x12345678)
  # Target association0 is also a connectivity row unless it is local.
  if local!=0:put(b,0x71c8,0)
  add('P',f'publication_state_{len(records):04d}',b,channels=c,**(defaults|dict(optional=optional)))
 b=bank();c=channels(b,1,0);base=add('P','normal_payload',b,channels=c,**defaults)
 for last,threshold,elapsed,pub in itertools.product((0,950,900,0xfffffff0),(100,-1,0x7fffffff),(1000,0x10,0x800003e8),(2000,0xfffffff0)):
  add('P',f'clocks_{len(records):04d}',bank(),channels=c,**(defaults|dict(last=last,threshold=threshold,elapsed=elapsed,publication=pub)))
 for present,stored in itertools.product((0,1,0x80),(bytes.fromhex(base['payload_hex']),bytes(200))):
  b=bank(present=present);b[0xb0:0xb0+200]=stored
  add('P',f'stored_{len(records):04d}',b,channels=c,**defaults)
 for name,party,optional,scale in itertools.product(('', 'ABCDEFGHIJKLMNOPQRSTUV', 'ABC\0DE','Élite🎮'),('', 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789','XY\0ZZ'),(0,0xff),(0,0x3e800000)):
  b=bank(count=5);c5=channels(b,5,0)
  add('P',f'names_metadata_{len(records):04d}',b,channels=c5,**(defaults|dict(name=name,party=party,optional=optional,scale=scale,map_status=0x12345678,map_progress=0xffffffff,bandwidth=0)))
 a.fixture.write_text(''.join(json.dumps(r,separators=(',',':'))+'\n' for r in records));m=Cs(CS_ARCH_X86,CS_MODE_32);windows={};asm=[]
 for name,(lo,hi) in WINDOWS.items():
  b=image[lo-BASE:hi-BASE];windows[name]=dict(begin=hex(lo),end_exclusive=hex(hi),sha256=hashlib.sha256(b).hexdigest(),bytes=b.hex());asm.append(f';{name} {lo:08X}..{hi:08X}');asm.extend(f'{x.address:08X} {x.mnemonic} {x.op_str}' for x in m.disasm(b,lo))
 (a.directory/'source.asm').write_text('\n'.join(asm)+'\n');report=dict(source_sha256=SHA,fixture_sha256=hashlib.sha256(a.fixture.read_bytes()).hexdigest(),case_count=len(records),instruction_visits=o.visits,windows=windows,boundary='Actual owned storage, string, channel, scale/statistic, local publisher and clock instructions. Literal byte memory services and outgoing5BED40 IO are explicit boundaries. No identities, readiness, network acknowledgment or live native account/session are fabricated.')
 (a.directory/'execution.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ('case_count','instruction_visits','fixture_sha256')}))
if __name__=='__main__':main()
