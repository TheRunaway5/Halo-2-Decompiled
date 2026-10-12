#!/usr/bin/env python3
"""Unchanged ordinary Vista constructor, with declared network query boundaries.

No real transport, profile, player list, live session, or readiness is created.
The entire source byte bank is retained to detect unintended native writes.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_ESP,UC_X86_REG_EIP
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from oracle_vista_lobby_countdown import SOURCE_SHA, BASE, STOP, OBJECT, STACK, make_machine, read32, word
SESSION=0x1A00000; SP=STACK+0x8000; SIZE=0x7B88

def put(u,a,v):u.mem_write(a,struct.pack('<I',word(v)))
def ret(u,value=0,pop=0):
 s=u.reg_read(UC_X86_REG_ESP);u.reg_write(UC_X86_REG_EAX,word(value));u.reg_write(UC_X86_REG_ESP,s+4+pop);u.reg_write(UC_X86_REG_EIP,read32(u,s))
def execute(image,label,*,initial_state=0,local_valid=True,create_valid=True,key_valid=True,poison=0xA5,clock=0x12345678,value_seed=0):
 u=make_machine(image);u.mem_map(SESSION,0x8000)
 before=bytearray((i*37+poison)&255 for i in range(SIZE))
 # Explicit quiescent initialized owner context, rather than fake live state.
 struct.pack_into('<I',before,0x73A0,word(initial_state));before[0x48]=0
 struct.pack_into('<I',before,0x5C,0)
 u.mem_write(SESSION,bytes(before));put(u,0x820FE8,SESSION)
 u.mem_write(SP,struct.pack('<3I',STOP,1,0));u.reg_write(UC_X86_REG_ESP,SP)
 for r in (UC_X86_REG_EDX,UC_X86_REG_ESI,UC_X86_REG_EDI):u.reg_write(r,0)
 local=bytes((i*11+7+value_seed)&255 for i in range(36));identity=bytes((i+0x20+value_seed)&255 for i in range(8));address=bytes((i+0x40+value_seed)&255 for i in range(16));random=bytes((i+0x60+value_seed)&255 for i in range(8))
 clocks=[word(clock+i*19) for i in range(3)];calls=[];clock_count=0
 def hook(m,pc,size,_):
  nonlocal clock_count
  s=m.reg_read(UC_X86_REG_ESP)
  if pc==STOP:m.emu_stop();return
  if pc==0x5B5836:
   args=[read32(m,s+4),read32(m,s+8)];calls.append(dict(service='local_machine',pc=f'{pc:08X}',arguments=args))
   assert args[0] and args[1]==0;m.mem_write(args[0],local);ret(m,int(local_valid))
  elif pc==0x5B63C7:
   args=[read32(m,s+4+i*4) for i in range(3)];assert args[:2]==[0,1] and args[2]&255==0, args
   calls.append(dict(service='network_create',pc=f'{pc:08X}',arguments=args));ret(m,int(create_valid))
  elif pc==0x5B56B9:
   args=[read32(m,s+4+i*4) for i in range(3)];assert args[0]==0
   calls.append(dict(service='network_identity',pc=f'{pc:08X}',arguments=args))
   if key_valid:m.mem_write(args[1],identity);m.mem_write(args[2],address)
   ret(m,int(key_valid))
  elif pc==0x5BD629:
   args=[read32(m,s+4+i*4) for i in range(5)];calls.append(dict(service='network_register',pc=f'{pc:08X}',owner=m.reg_read(UC_X86_REG_ECX),arguments=args));ret(m,0,20)
  elif pc==0x5BA2CF:
   calls.append(dict(service='network_publish_identity',pc=f'{pc:08X}',arguments=[read32(m,s+4)]));ret(m)
  elif pc==0x5B3CA0:
   assert clock_count<len(clocks);calls.append(dict(service='network_clock',pc=f'{pc:08X}',arguments=[]))
   put(m,0x91ABCC,clocks[clock_count]);m.mem_write(0x91ABC8,b'\1');clock_count+=1
  elif pc==0x4381FD:calls.append(dict(service='language',pc=f'{pc:08X}',arguments=[]));ret(m,word(6+value_seed))
  elif pc==0x5B5A84:
   args=[read32(m,s+4),read32(m,s+8)];assert args[0]==8
   calls.append(dict(service='random_bytes',pc=f'{pc:08X}',arguments=args));m.mem_write(args[1],random);ret(m)
  elif pc==0x437E5E:calls.append(dict(service='process_clock',pc=f'{pc:08X}',arguments=[]));ret(m,word(0x76543210+value_seed))
  elif pc==0x5B333D:
   assert read32(m,s+4)==SESSION;calls.append(dict(service='publish_peer_payload',pc=f'{pc:08X}',arguments=[SESSION]));ret(m)
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(0x5AD7D0,STOP+1,count=1000000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 after=bytes(u.mem_read(SESSION,SIZE));result=u.reg_read(UC_X86_REG_EAX)&255
 return dict(label=label,inputs=dict(initial_state=word(initial_state),local_valid=local_valid,create_valid=create_valid,key_valid=key_valid,
   local_machine_hex=local.hex(),identity_hex=identity.hex(),address_hex=address.hex(),random_hex=random.hex(),network_clocks=clocks,
   language=word(6+value_seed),process_clock=word(0x76543210+value_seed)),before_hex=before.hex(),after_hex=after.hex(),accepted_al=result,calls=calls,clock_count=clock_count)

def pin(image,name,a,b):
 data=image[a-BASE:b-BASE];decoder=Cs(CS_ARCH_X86,CS_MODE_32)
 return name,dict(begin=f'{a:08X}',end_exclusive=f'{b:08X}',sha256=hashlib.sha256(data).hexdigest(),bytes_hex=data.hex(),
  instructions=[dict(address=f'{i.address:08X}',mnemonic=i.mnemonic,operands=i.op_str) for i in decoder.disasm(data,a)])

def manager(image,label,poison,owners):
 u=make_machine(image);before=bytes((i*37+poison)&255 for i in range(0x44));u.mem_write(OBJECT,before)
 u.mem_write(SP,struct.pack('<6I',STOP,*owners));u.reg_write(UC_X86_REG_ESP,SP);u.reg_write(UC_X86_REG_ECX,OBJECT)
 u.hook_add(UC_HOOK_CODE,lambda m,pc,size,_:m.emu_stop() if pc==STOP else None);u.emu_start(0x5D7591,STOP+1,count=1000)
 assert u.reg_read(UC_X86_REG_EIP)==STOP
 return dict(label=label,before_hex=before.hex(),after_hex=bytes(u.mem_read(OBJECT,0x44)).hex(),owners=owners)

SERVICES={'local_machine':0,'network_create':1,'network_identity':2,'network_register':3,'network_publish_identity':4,
 'network_clock':5,'language':6,'random_bytes':7,'process_clock':8,'publish_peer_payload':9}
def semantic_calls(calls):
 result=[]
 for c in calls:
  a=c['arguments'];service=c['service'];owner=c.get('owner',0)
  if service=='local_machine':args=[int(bool(a[0])),int(bool(a[1]))]
  elif service=='network_create':args=[a[0],a[1],a[2]&255]
  elif service=='network_identity':args=[a[0],int(bool(a[1])),int(bool(a[2]))]
  elif service=='network_register':args=a[:3]
  elif service=='random_bytes':args=[a[0]]
  else:args=[]
  result.append([SERVICES[service],owner,len(args),*args])
 return result

def bytes_words(data):
 b=bytes.fromhex(data);assert len(b)%4==0;return list(struct.unpack('<'+'I'*(len(b)//4),b))
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--exe',type=Path,default=ROOT/'Halo 2 Vista installer/halo2.exe')
 parser.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/vista_session_birth_original.tsv')
 parser.add_argument('--report',type=Path,default=ROOT/'analysis/frontend/vista-session-flow-audit/session-birth-fixture-original.json')
 args=parser.parse_args();image=args.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
 cases=[]
 for state,local,create,key,poison,clock in itertools.product((0,5,0xFFFFFFFF),(False,True),(False,True),(False,True),(0xA5,0x19),(0x12345678,0xFFFFFFF0)):
  cases.append(execute(image,f'birth_{len(cases):03d}',initial_state=state,local_valid=local,create_valid=create,key_valid=key,poison=poison,clock=clock))
 for seed in range(32):cases.append(execute(image,f'value_control_{seed:02d}',value_seed=seed*71,clock=word(seed*0x9E3779B9)))
 managers=[]
 for poison,seed in itertools.product((0,0x19,0xA5,0xFF),(0,1,0xFFFFFFF0)):
  managers.append(manager(image,f'manager_{len(managers):02d}',poison,[word(seed+i*0x1234567) for i in range(5)]))
 lines=['# H2V_SESSION_BIRTH_ORIGINAL_V1','# kind label word_count little_endian_u32_semantic_words_hex']
 for c in cases:
  q=c['inputs'];sc=semantic_calls(c['calls']);c['semantic_calls']=sc
  words=[int(q['local_valid']),int(q['create_valid']),int(q['key_valid']),*bytes_words(q['local_machine_hex']),*bytes_words(q['identity_hex']),*bytes_words(q['address_hex']),
    *q['network_clocks'],q['language'],*bytes_words(q['random_hex']),q['process_clock'],*bytes_words(c['before_hex']),*bytes_words(c['after_hex']),c['accepted_al'],len(sc),
    *itertools.chain.from_iterable(sc)]
  lines.append('\t'.join(['B',c['label'],str(len(words)),*(f'{v:08x}' for v in words)]))
 for c in managers:
  words=[*c['owners'],*bytes_words(c['before_hex']),*bytes_words(c['after_hex'])]
  lines.append('\t'.join(['M',c['label'],str(len(words)),*(f'{v:08x}' for v in words)]))
 args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_text('\n'.join(lines)+'\n')
 report=dict(version='H2V_SESSION_BIRTH_ORIGINAL_V1',source_sha256=SOURCE_SHA,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),birth_cases=cases,manager_cases=managers,
  boundary='Unchanged5AD7D0/5CAF2E constructor with source memory/peer/association/host-entry leaves and literal5D7591 initialization. Network queries/register, language/random/process-clock and final peer publication are explicit external boundaries. Source quiescent primary transport only; no profile/player array, network operation, readiness or live activation.')
 args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(dict(birth=len(cases),manager=len(managers),fixture_sha256=report['fixture_sha256'])))
if __name__=='__main__':main()
