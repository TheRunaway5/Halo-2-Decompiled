#!/usr/bin/env python3
"""Whole unchanged King scoring and shared award calls in three original XBEs.

Occupied player pools/current-unit fields, original score banks/options/phase,
clock, King timers and disabled recording are declared API inputs. Actual
engine registration, iterator, score getter/writers, recorder gate and reached
notification owners execute. Lockout matg/mulg tags back notification lookups;
replication entities/local users are NONE. No game or service return is supplied.
This is not natural objective play, full update/HUD or network session proof.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP
from oracle_king_engine import OWNERS as KING
from oracle_round_end import RoundEndTrace,G,M,R,bank
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha
from oracle_player_lives import C

OUT=ROOT/'analysis/multiplayer/game-variants/king-engine-20261011/scoring'
OWNERS={
 'retail':dict(scoring=0x2beee0,getter=0x19fc70,award=0x15b7c0,changed=0x19f470,record=0x1972a0),
 'tu11':dict(scoring=0x2c5220,getter=0x1a35d0,award=0x15ce00,changed=0x1a2dd0,record=0x198da0),
 'u5':dict(scoring=0x2c8ca0,getter=0x1a9880,award=0x15e1e0,changed=0x1a9080,record=0x19bca0)}
K=M+0xfc
def player(slot,inside=30,membership=0,unit=True,flags=1):
 return dict(slot=slot,inside_ticks=inside,membership=membership,unit=0x8aaa0000+slot if unit else 0xffffffff,lives=-1,flags=flags)
def inputs():
 rows=[]
 rosters=[[],[player(0)],[player(0,unit=False)],
  [player(0,30),player(3,30)],[player(0,29),player(3,30)],
  [player(0,31),player(3,30),player(15,30)],
  [player(0,-1),player(3,-32768),player(15,32767)],
  [player(0,30,0),player(3,30,7),player(15,0,7)],
  [player(0,30,-1),player(3,30,7)],
  [player(0,30,0,flags=2),player(3,30,0,flags=0x4002),player(15,30,0)],
  [player(0,30,0),player(3,30,0,unit=False),player(15,30,0)]]
 for team in (False,True):
  for flags in (0,1,2,3,0xffffffff):
   for roster in rosters:
    for clock in (0,28,29,30,-1,-2,2147483647,-2147483648):
     rows.append(dict(team_mode=team,flags=flags,players=roster,team_ticks=[clock]*8,rate=30,
       score=0,team_score=0,team_score_kind=0))
 for lifecycle,status in ((-1,1),(0,1),(1,-1),(1,0),(1,1),(1,2),(2,1)):
  for mode in (0,1,2,4,255):
   for present in (False,True):
    rows.append(dict(team_mode=True,flags=0,players=[player(0),player(3)],team_ticks=[29]*8,rate=30,
      lifecycle=lifecycle,status=status,mode=mode,engine_present=present,score=0,team_score=0,team_score_kind=0))
 rng=random.Random(0x2c8ca0)
 for n in range(512):
  slots=sorted(rng.sample(range(16),rng.randrange(17)))
  roster=[player(slot,rng.choice((-32768,-32760,-60,-30,-1,0,1,29,30,60,32760,32767)),
    rng.choice((-1,0,1,7)),rng.choice((True,True,False)),rng.choice((0,1,2,3,0x4002)))for slot in slots]
  rate=rng.choice((-32768,-30,-1,1,30,60,32767))
  ticks=[rng.choice((-2147483648,-2147483647,-61,-31,-2,-1,0,28,29,59,2147483647))for _ in range(8)]
  # Avoid only the literal x86 signed division trap, not a native result.
  if rate==-1:ticks=[0 if v==2147483647 else v for v in ticks]
  rows.append(dict(team_mode=bool(n&1),flags=rng.randrange(32),players=roster,team_ticks=ticks,rate=rate,
    score=rng.choice((-30000,-1,0,1,29999,30000)),team_score=rng.choice((-30000,-1,0,1,29999,30000)),team_score_kind=rng.randrange(3)))
 return rows
class ScoringTrace(RoundEndTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.ka=KING[name];self.so=OWNERS[name];self.events=[]
  self.g.hook_add(UC_HOOK_CODE,self.event)
 def event(self,g,a,n,user):
  kind=next((k for k,v in self.so.items()if a==v and k!='scoring'),None)
  if kind:
   sp=g.reg_read(UC_X86_REG_ESP)
   if kind=='getter':row=dict(kind=kind,player=g.reg_read(UC_X86_REG_EDX))
   elif kind=='award':row=dict(kind=kind,player=self.u(sp+4),delta=g.reg_read(UC_X86_REG_EAX))
   elif kind=='changed':row=dict(kind=kind,player=g.reg_read(UC_X86_REG_EAX),score=struct.unpack('<i',g.mem_read(sp+4,4))[0])
   else:row=dict(kind=kind,player=g.reg_read(UC_X86_REG_EBX),request=self.u(sp+4),other=self.u(sp+8))
   self.events.append(row)
 def setup_score(self,row):
  self.setup_lives(dict(limit=0,**row));self.w(M+0xc14,4 if row.get('engine_present',True)else 0)
  self.w(G+0x180,4);self.w(G+0x22c,row['flags']);self.w(G+0x1e0,row['team_score_kind']);self.w(G+0x18c,0)
  self.w(self.ka['state'],K);self.g.mem_write(K,b'\x5a'*0x1dc)
  self.g.mem_write(K+0x180,struct.pack('<8i',*row['team_ticks']));self.g.mem_write(C+2,struct.pack('<h',row['rate']))
  for p in row['players']:self.g.mem_write(R+p['slot']*0x21c+0x1b8,struct.pack('<h',p['inside_ticks']))
  self.g.mem_write(M+0x304,bytes(0x254))
  for i in range(16):self.g.mem_write(M+0x308+i*28,struct.pack('<h',row['score']))
  for i in range(8):self.g.mem_write(M+0x4c8+i*18,struct.pack('<h',row['team_score']))
  self.events=[]
 def state(self):
  return dict(king_hex=bytes(self.g.mem_read(K,0x1dc)).hex(),bank_hex=bank(self),engine_flags=self.u(M),status=self.u(M+0xc04),
    timers=[struct.unpack('<h',self.g.mem_read(R+i*0x21c+0x1b8,2))[0]for i in range(16)])
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_scoring_original.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');rows=inputs();versions={};sources={}
 for name in OWNERS:
  t=ScoringTrace(name,cache);sources[name]=t;results=[]
  for index,row in enumerate(rows):
   t.setup_score(row);before=t.state();players=bytes(t.g.mem_read(R,16*0x21c))
   try:t.call(t.so['scoring'])
   except Exception:
    print(name,index,json.dumps(row),flush=True);raise
   after=t.state();actual=bytearray(t.g.mem_read(R,16*0x21c))
   for i in range(16):actual[i*0x21c+0x1b8:i*0x21c+0x1ba]=players[i*0x21c+0x1b8:i*0x21c+0x1ba]
   assert bytes(actual)==players,'Scorer mutated an unowned player field'
   results.append(dict(before=before,after=after,events=t.events))
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.so,results=results,observed_instructions=len(t.executed))
 assert versions['retail']['results']==versions['tu11']['results']==versions['u5']['results']
 listings=[];pairs=[];directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for earlier in ('retail','tu11'):
  old=sources[earlier];later=sources['u5']
  for key in ('scoring','getter','award','changed','record'):
   oe,ne=old.so[key],later.so[key];_,oi=literal(old.code,oe,old.starts);_,ni=literal(later.code,ne,later.starts)
   assert complete_tokens(oi,old.image)==complete_tokens(ni,later.image),(earlier,key)
   pairs.append(dict(earlier=earlier,owner=key,instructions=len(oi),complete_shape_equal=True,relocated_operands=references(oi,ni,old.image,later.image)))
   for version,e,ins in((earlier,oe,oi),('u5',ne,ni)):
    p=directory/f'{version}-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
 data=dict(schema='original-king-scoring-v1',boundary=__doc__,inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),lockout_sha256=CACHE_PIN,complete_source_pairs=pairs,instruction_listings=listings,
  summary=dict(whole_scoring_calls=len(rows)*3,awards=sum(sum(e['kind']=='award'for e in r['events'])for r in versions['u5']['results'])),
  boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary'],indent=2))
if __name__=='__main__':main()
