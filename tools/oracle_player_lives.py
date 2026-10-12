#!/usr/bin/env python3
"""Whole unchanged original player-life initialization, death and admission.

Occupied player pools, options, lifecycle/status, clock and life counts are
declared API inputs. Engine registration and every reached game routine execute.
Replication entities/local users/current units are NONE for initialization and
death calls; death cause41 avoids the separate notification branch. Query calls
also cover present current units. Recording is disabled. No instruction patch
or supplied game/platform/service return is used. This is not a complete round,
world combat, live player allocation or original transport.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI
from oracle_result_statistics import StatisticsTrace,G,M,P,R
from oracle_mp_full_scoreboard_1_5 import ROOT,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/player-lives'
OWNERS={
 'retail':dict(initialize=0x157be0,death=0x15ce70,exhausted=0x15db30,allowed=0x158a50,team_exhausted=0x15db80),
 'tu11':dict(initialize=0x159280,death=0x15e4d0,exhausted=0x15f190,allowed=0x15a090,team_exhausted=0x15f1e0),
 'u5':dict(initialize=0x15a660,death=0x15f8b0,exhausted=0x160570,allowed=0x15b470,team_exhausted=0x1605c0)}
H,B,C=0x1016000,0x1016000,0x1017000
def short(value):return struct.unpack('<h',struct.pack('<H',value&65535))[0]
class LivesTrace(StatisticsTrace):
 def __init__(self,name):
  super().__init__(name);self.o=OWNERS[name];self.g.mem_map(H,0x2000)
  self.w(self.spec['clock'],C);self.w(P+0x48,B);self.w(P+0x20,16)
 def setup_lives(self,row):
  self.g.mem_write(G,bytes(0x1200));self.g.mem_write(M,bytes(0xc18));self.g.mem_write(R,bytes(16*0x21c))
  self.g.mem_write(self.s['recording'],bytes(0x12));self.w(P+0x38,16)
  self.w(G+8,row.get('game_type',2));self.g.mem_write(G+0xc,bytes([row.get('mode',1)&255]))
  self.w(G+0x184,int(row.get('team_mode',False)));self.w(G+0x1b8,row['limit']);self.w(G+0x1bc,5)
  self.w(M+0xc14,2 if row.get('engine_present',True)else 0)
  self.g.mem_write(M+0x6c,struct.pack('<h',row.get('lifecycle',1)));self.w(M+0xc04,row.get('status',1))
  self.w(M+0x24,-1);self.w(M+0x28,-1)
  for i in range(16):self.w(M+0x2c+i*4,-1)
  self.g.mem_write(C+2,struct.pack('<h',30));self.g.mem_write(C+4,struct.pack('<f',1/30));self.w(C+8,0)
  mask=0
  for player in row['players']:
   slot=player['slot'];mask|=1<<slot;at=R+slot*0x21c
   self.g.mem_write(at,struct.pack('<HH',0x9abc,player['flags']))
   self.w(at+0x24,-1);self.g.mem_write(at+0x28,b'\xff\xff');self.w(at+0x2c,player['unit'])
   self.g.mem_write(at+0xc0,bytes([player['membership']&255]));self.w(at+0x1b0,-1)
   self.g.mem_write(at+0x1ac,struct.pack('<h',player['lives']))
  self.w(B,mask);self.calls=[]
 def state(self):return bytes(self.g.mem_read(R,16*0x21c)).hex()
 def life(self,slot):return struct.unpack('<h',self.g.mem_read(R+slot*0x21c+0x1ac,2))[0]

def inputs():
 rng=random.Random(0x160570);values=(-2147483648,-65537,-32769,-32768,-1,0,1,2,3,32767,32768,65535,65536,2147483647)
 initial=[];deaths=[];queries=[];teams=[]
 for n in range(128):
  slot=(0,3,15)[n%3];initial.append(dict(slot=slot,limit=values[n%len(values)],mode=(0,1,2,4,255)[n//26],
   engine_present=bool(n&1),players=[dict(slot=slot,flags=1,unit=0xffffffff,membership=slot,lives=short(n*793))]))
 for n in range(512):
  slot=(0,3,15)[n%3];deaths.append(dict(slot=slot,limit=values[n%len(values)],game_type=(-1,0,1,2,3)[n%5],
   mode=(0,1,4,255)[(n//5)%4],engine_present=bool(n&1),lifecycle=(-1,0,1,2)[(n//4)%4],status=(-1,0,1,2)[(n//16)%4],
   players=[dict(slot=slot,flags=1,unit=0xffffffff,membership=slot,
    lives=(-32768,-1,0,1,2,32767)[n%6] if n<384 else rng.randrange(-32768,32768))]))
 # Explicit active gates for every signed-short life boundary, including
 # zero, NONE, and other negative values. The configured limit does not gate
 # the original death decrement; it gates exhaustion queries instead.
 for mode in (0,1,2,4,255):
  for limit in (-1,0,1,32767):
   for life in (-32768,-2,-1,0,1,2,32767):
    deaths.append(dict(slot=3,limit=limit,mode=mode,players=[dict(slot=3,flags=1,unit=0xffffffff,membership=3,lives=life)]))
 for n in range(1024):
  slot=(0,3,15)[n%3];queries.append(dict(slot=slot,limit=values[(n//8)%len(values)],
   players=[dict(slot=slot,flags=(0,1,2,3,0x4000,0x4001,0x4002,0xffff)[n%8],
    unit=0xffffffff if n&8 else 0x8aaa0000,membership=(-128,-1,0,7,15,127)[(n//16)%6],
    lives=(-32768,-2,-1,0,1,2,32767)[(n//96)%7])]))
 for n in range(256):
  players=[dict(slot=slot,flags=(0,1,2,3)[(n//(1+slot))%4],unit=0xffffffff if n&(1<<j)else 0x8aaa0000+slot,
   membership=(-1,0,7)[(n+j)%3],lives=(-1,0,1)[(n//8+j)%3])for j,slot in enumerate((0,3,15))]
  teams.append(dict(team=(-1,0,7,15)[n%4],limit=values[(n//4)%len(values)],team_mode=bool(n&16),engine_present=bool(n&32),players=players))
 return dict(initialize=initial,death=deaths,queries=queries,teams=teams)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/player_lives_original.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);rows=inputs();sources={};versions={};entries={};listings=[];pairs=[]
 for name in OWNERS:
  t=LivesTrace(name);sources[name]=t;result={};entries[name]=set()
  for operation,cases in rows.items():
   result[operation]=[]
   for row in cases:
    t.setup_lives(row);before=t.state();slot=row.get('slot',0);datum=0x9abc0000+slot;out={}
    if operation=='initialize':t.call(t.o[operation],registers=((UC_X86_REG_EAX,datum),));out['lives']=t.life(slot)
    elif operation=='death':t.call(t.o[operation],(0xffffffff,datum,0,41));out['lives']=t.life(slot)
    elif operation=='queries':
     for query,reg in (('exhausted',UC_X86_REG_ECX),('allowed',UC_X86_REG_ESI)):
      out[query]=t.call(t.o[query],registers=((reg,datum),))&255
     assert t.state()==before,'Read-only life query changed a player'
    else:
     out['team_exhausted']=t.call(t.o['team_exhausted'],(row['team'],))&255
     assert t.state()==before,'Read-only team life query changed a player'
    out['calls']=[f'{e:08x}'for e in t.calls];entries[name]|=set(t.calls)
    result[operation].append(out)
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.o,results=result)
 directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for name,t in sources.items():
  for entry in sorted(entries[name]):
   _,ins=literal(t.code,entry,t.starts);p=directory/f'{name}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p)))
  if name=='u5':continue
  for key in ('exhausted','allowed','team_exhausted'):
   u=sources['u5'];_,oi=literal(t.code,t.o[key],t.starts);_,ni=literal(u.code,u.o[key],u.starts)
   assert complete_tokens(oi,t.image)==complete_tokens(ni,u.image),(name,key)
   pairs.append(dict(earlier=name,later='u5',owner=key,instructions=len(oi),complete_shape_equal=True,
    relocated_operands=references(oi,ni,t.image,u.image)))
 data=dict(schema='original-player-lives-v1',boundaries=__doc__,inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),instruction_listings=listings,complete_source_pairs=pairs,
  dependencies={p:sha(ROOT/p)for p in ('tools/oracle_result_statistics.py','tools/oracle_game_over.py','tools/oracle_game_variants.py','tools/oracle_mp_full_scoreboard_1_5.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(whole_calls=3*(len(rows['initialize'])+len(rows['death'])+2*len(rows['queries'])+len(rows['teams'])),
  fixture_sha256=sha(a.fixture),source_pairs=len(pairs),instruction_listings=len(listings)),indent=2))
if __name__=='__main__':main()
