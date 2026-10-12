#!/usr/bin/env python3
"""Whole unchanged original offline identities, profiles and sanitizers.

Six machine-address bytes, local-user indices, offline retained profile fields,
raw configuration bytes, local input-member records and literal game/tag
context are declared API inputs. No online account service is supplied.
Actual registered engine objects and all reached original callees execute.
This does not produce a native machine address, select a frontend profile,
create an original player, or bind retained results to a native match.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX
from oracle_result_statistics import StatisticsTrace,G,M
from oracle_mp_full_scoreboard_1_5 import ROOT,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/player-profile'
FIXTURE=ROOT/'tests/fixtures/player_profile_original.json'
H=0x1016000
OWNERS={
 'retail':dict(identity=0x14b410,full=0x1a0750,configuration=0x1537f0,
  convert=0x18fd94,sanitize=0x14bc80,local=0x54e8e0,stride=0xc70,
  matg=0x4e034c,scenario=0x4e0350,members=0x4cd904),
 'tu11':dict(identity=0x14cb60,full=0x1a40b0,configuration=0x154f40,
  convert=0x1917c6,sanitize=0x14d3d0,local=0x556430,stride=0xc74,
  matg=0x4e791c,scenario=0x4e7920,members=0x4d4e24),
 'u5':dict(identity=0x14df40,full=0x1aa360,configuration=0x156320,
  convert=0x192baa,sanitize=0x14e7b0,local=0x55d770,stride=0xc74,
  matg=0x4ecc1c,scenario=0x4ecc20,members=0x4da128)}

def input_cases():
 rng=random.Random(0x14df40);identity=[];conversion=[];sanitizers=[]
 indices=[-2147483648,-65537,-65536,-1,0,1,2,3,4,255,65535,65536,2147483647]
 for n in range(640):
  identity.append(dict(machine_hex=(bytes(6)if n==0 else bytes([255])*6 if n==1 else rng.randbytes(6)).hex(),
   user=indices[n%len(indices)]if n<128 else rng.randrange(-2147483648,2147483648)))
 for n in range(512):
  name=[rng.randrange(1,65536)for _ in range(32)]
  if n%4!=3:name[(n//4)%32]=0
  account=bytearray(rng.randbytes(12))
  if n%4==0:account[:8]=bytes(8)
  if n%4==1:account[:4]=bytes(4)
  if n%4==2:account[4:8]=bytes(4)
  conversion.append(dict(slot=n%4,identity_hex=rng.randbytes(12).hex(),name=name,
   appearance_hex=rng.randbytes(16).hex(),account_hex=account.hex(),account_data_hex=rng.randbytes(32).hex(),
   committed_team=indices[n%len(indices)]if n<128 else rng.randrange(-2147483648,2147483648),
   field_200=n%256,field_46c=(n*53)%256,field_204=rng.randrange(1<<32),override_four=bool(n&1),
   input_member=(-2147483648,-1,0,1,2,3,4,2147483647)[(n//16)%8],
   input_member_present=(0,1,255)[(n//128)%3],input_configuration_hex=rng.randbytes(144).hex(),
   identity_output=bool((n//4)%4 not in(1,3)),configuration_output=bool((n//4)%4 not in(2,3))))
 for n in range(2048):
  raw=bytearray(rng.randbytes(144))
  raw[0x44]=n%256;raw[0x7c]=(n*3)%256;raw[0x7d]=(n*7)%256
  raw[0x7e]=(n*11)%256;raw[0x7f]=(n*13)%256;raw[0x81]=(n*17)%256
  struct.pack_into('<i',raw,0x84,(-2147483648,-1,0,1,15,16,2147483647)[n%7])
  struct.pack_into('<i',raw,0x8c,(-2147483648,-1,0,1,0x3fffffff,0x40000000,2147483647)[(n//7)%7])
  for at,k in ((0x88,n//11),(0x8a,n//13)):
   struct.pack_into('<h',raw,at,(-32768,-1,0,1,127,128,32767)[k%7])
  scenario=([-1]*((n//19)%4)) + ([]if n%3==0 else [(-32768,-2,0,1,3,127,128,255,256,32767)[n%10],2])
  sanitizers.append(dict(raw_hex=raw.hex(),game_type=(-1,0,1,2,3)[(n//256)%5],
   representation_count=(-2147483648,-1,0,1,2,4,127,128,255,256,2147483647)[(n//17)%11],
   scenario=scenario,selected_engine=bool(n&1),team_mode=bool(n&2),team_mask=(0,1,2,255,65535)[(n//4)%5]))
 return dict(identity=identity,conversion=conversion,sanitize=sanitizers)

class ProfileTrace(StatisticsTrace):
 def __init__(self,name):
  super().__init__(name);self.o=OWNERS[name];self.g.mem_map(H,0x3000)
  _,ins=literal(self.code,self.o['identity'],self.starts)
  addresses=[i.operands[1].mem.disp for i in ins if i.mnemonic=='movzx'and 'byte ptr [0x'in i.op_str]
  assert len(addresses)==6 and sorted(addresses)==list(range(min(addresses),min(addresses)+6))
  self.machine=min(addresses)
 def default(self,op,size):
  self.g.mem_write(H,b'\xa5'*size);self.calls=[]
  self.call(self.o[op],registers=((UC_X86_REG_EDX,H),))
  return dict(output_hex=bytes(self.g.mem_read(H,size)).hex(),calls=[f'{e:08x}'for e in self.calls])
 def identity(self,row):
  self.g.mem_write(self.machine,bytes.fromhex(row['machine_hex']));self.g.mem_write(H,b'\xa5'*12);self.calls=[]
  self.call(self.o['identity'],(row['user'],),((UC_X86_REG_ECX,H),))
  return dict(output_hex=bytes(self.g.mem_read(H,12)).hex(),calls=[f'{e:08x}'for e in self.calls])
 def convert(self,row):
  local=self.o['local']+row['slot']*self.o['stride']
  self.g.mem_write(local,bytes(self.o['stride']));self.w(local+4,row['input_member'])
  self.g.mem_write(self.o['members'],bytes(4*0xd0))
  for slot in range(4):
   member=self.o['members']+slot*0xd0
   self.g.mem_write(member,bytes([row['input_member_present']]))
   self.g.mem_write(member+0x14,bytes.fromhex(row['input_configuration_hex']))
  self.g.mem_write(local+8,bytes.fromhex(row['identity_hex']))
  self.g.mem_write(local+0xc1c,struct.pack('<32H',*row['name']))
  for off,key in ((0x130,'appearance_hex'),(0x4e0,'account_hex'),(0x4ec,'account_data_hex')):
   self.g.mem_write(local+off,bytes.fromhex(row[key]))
  self.w(local+0x1fc,row['committed_team']);self.g.mem_write(local+0x200,bytes([row['field_200']]))
  self.g.mem_write(local+0x46c,bytes([row['field_46c']]));self.w(local+0x204,row['field_204'])
  if self.name=='u5':self.g.mem_write(0x51d2c8,bytes([int(row['override_four'])]))
  self.g.mem_write(H,b'\xa5'*144);self.g.mem_write(H+0x400,b'\xa5'*12);self.calls=[]
  buffer=H if row['configuration_output']else 0;identity=H+0x400 if row['identity_output']else 0
  args=(row['slot'],buffer)if self.name=='u5'else (buffer,)
  self.call(self.o['convert'],args,((UC_X86_REG_EAX,identity),(UC_X86_REG_ECX,row['slot'])))
  return dict(output_hex=bytes(self.g.mem_read(H,144)).hex(),identity_hex=bytes(self.g.mem_read(H+0x400,12)).hex(),
   calls=[f'{e:08x}'for e in self.calls])
 def sanitize(self,row):
  self.g.mem_write(H,bytes.fromhex(row['raw_hex']));self.w(self.o['matg'],H+0x1000)
  self.w(H+0x1138,row['representation_count']);self.w(self.o['scenario'],H+0x1400)
  self.w(H+0x1500,len(row['scenario']));self.w(H+0x1504,H+0x1800)
  for n,value in enumerate(row['scenario']):self.g.mem_write(H+0x1800+n*0x34+0x2c,struct.pack('<h',value))
  self.w(G+8,row['game_type']);self.w(G+0x184,int(row['team_mode']))
  self.w(M+0xc14,2 if row['selected_engine']else 0)
  self.g.mem_write(M+0xa,struct.pack('<H',row['team_mask']));self.calls=[]
  self.call(self.o['sanitize'],registers=((UC_X86_REG_EAX if self.name=='retail'else UC_X86_REG_EDX,H),))
  return dict(output_hex=bytes(self.g.mem_read(H,144)).hex(),calls=[f'{e:08x}'for e in self.calls])

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=FIXTURE);a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 inputs=input_cases();sources={};versions={};entries={};listings=[];pairs=[]
 for name in OWNERS:
  t=ProfileTrace(name);sources[name]=t;entries[name]=set()
  result=dict(image_sha256=t.spec['pin'],owners=t.o,machine_address=f'{t.machine:08x}',
   full=t.default('full',0x1e0),configuration=t.default('configuration',0x90))
  for operation,key in (('identity','identity'),('conversion','convert'),('sanitize','sanitize')):
   result[operation]=[]
   for row in inputs[operation]:
    output=getattr(t,key)(row);result[operation].append(output);entries[name]|={int(e,16)for e in output['calls']}
  entries[name]|={t.o['full'],t.o['configuration']};versions[name]=result
 directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for name,t in sources.items():
  for entry in sorted(entries[name]):
   _,ins=literal(t.code,entry,t.starts);p=directory/f'{name}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p),kind='executed_owner'))
  if name=='u5':continue
  u=sources['u5']
  for key in ('identity','full','configuration')+ (('sanitize',)if name=='tu11'else ()):
   _,oi=literal(t.code,t.o[key],t.starts);_,ni=literal(u.code,u.o[key],u.starts)
   assert complete_tokens(oi,t.image)==complete_tokens(ni,u.image),(name,key)
   pairs.append(dict(earlier=name,later='u5',owner=key,instructions=len(oi),
    complete_shape_equal=True,relocated_operands=references(oi,ni,t.image,u.image)))
 t=sources['u5']
 for entry in (0x70ce0,0x192931,0x14e9e0):
  _,ins=literal(t.code,entry,t.starts);p=directory/f'u5-static-{entry:08x}.asm'
  p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p),kind='static_caller'))
 data=dict(schema='original-player-profile-v1',boundary=__doc__,instruction_patches=[],supplied_returns=[],inputs=inputs,versions=versions)
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),instruction_listings=listings,
  complete_source_pairs=pairs,dependencies={p:sha(ROOT/p)for p in ('tools/oracle_result_statistics.py',
   'tools/oracle_game_variants.py','tools/oracle_game_over.py','tools/oracle_mp_full_scoreboard_1_5.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(calls=3*(2+sum(map(len,inputs.values()))),fixture_sha256=sha(a.fixture),
  listings=len(listings),pairs=len(pairs)),indent=2))
if __name__=='__main__':main()
