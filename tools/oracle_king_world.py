#!/usr/bin/env python3
"""Whole original King slot18, including notifications, scorer and moving hills.

All reached instructions execute unchanged in retail/TU1.1/TU1.5. Real authored
netgame rows and Lockout matg/mulg notification tags, occupied player pools,
cached slot15 timers, clock/options/phase and retained RNG are declared inputs.
Recording is disabled, local users and replication entities are NONE. Source
initialization and round start execute before each update. No callback return
is supplied. This is not natural movement/combat, live HUD/audio or transport.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDI
from oracle_king_scoring import ScoringTrace,OWNERS as SCORING,player,G,M,K
from oracle_king_engine import OWNERS as KING,source_maps
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/king-engine-20261011/world'
OWNERS={
 'retail':dict(world=0x2bd680,dispatch=0x19eb30),
 'tu11':dict(world=0x2c3a10,dispatch=0x1a2490),
 'u5':dict(world=0x2c7490,dispatch=0x1a8740)}
W,F,Q=0x4000000,0x4001000,0x4010000
def inputs():
 rng=random.Random(0x2c7490);rows=[]
 rosters=[[],[player(0,29,0)],[player(0,30,0),player(3,30,7)],
  [player(0,30,0),player(3,30,0),player(15,0,7)],
  [player(0,30,0,flags=2),player(3,30,7,flags=0x4002)],
  [player(0,30,-1),player(3,30,7)],
  [player(0,30,0,unit=False),player(3,30,7)]]
 for n in range(672):
  rows.append(dict(map=n%12,team_mode=bool(n&1),flags=(n//2)%4,players=rosters[(n//8)%len(rosters)],
   team_ticks=[(-1,0,28,29,2147483647,-2147483648)[n%6]]*8,rate=30,
   score=(-30000,0,29999,30000)[(n//6)%4],team_score=0,team_score_kind=(n//24)%3,
   seed=rng.getrandbits(32),move_seconds=(0,1,30,-1)[(n//4)%4],
   move_ticks=(0,1,2,-1,2147483647,-2147483648)[(n//16)%6],
   previous_occupants=(0,1,8,9,0x8000,0xffff)[(n//12)%6],
   game_over=n%19==0,mode=4 if n%23==0 else 1,engine_present=n%29!=0,
   lifecycle=0 if n%31==0 else 1,status=0 if n%37==0 else 1))
 return rows
class WorldTrace(ScoringTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.wo=OWNERS[name];self.g.mem_map(W,0x20000)
  self.g.hook_add(UC_HOOK_CODE,self.notice)
 def notice(self,g,a,n,user):
  if a==self.wo['dispatch']:
   ptr=g.reg_read(UC_X86_REG_EDI);event=struct.unpack('<8i',g.mem_read(ptr,32))
   if event[0]==6:
    assert event[2]==event[5]==event[6]==-1 and event[7]==0
    assert bytes(g.mem_read(ptr+32,2))==b'\xff\xff'
    self.events.append(dict(kind='notification',type=event[1],player=event[3]&0xffffffff,membership=event[4]))
 def setup_world(self,row,raw):
  self.setup_score(row);self.w(self.ka['scenario'],W);self.w(W+0x118,len(raw)//32);self.w(W+0x11c,F);self.g.mem_write(F,raw)
  self.w(self.ka['rng'],Q);self.w(Q,row['seed']);self.g.mem_write(G+0x1128,bytes([int(row['game_over'])]))
  self.g.mem_write(G+0x230,struct.pack('<h',row['move_seconds']))
  assert self.call(self.ka['initialize'])&255==1
  assert self.call(self.ka['start'])&255==1
  self.g.mem_write(K+0x180,struct.pack('<8i',*row['team_ticks']));self.w(K+0x1a4,row['move_ticks'])
  self.g.mem_write(K+0x1a8,struct.pack('<H',row['previous_occupants']));self.events=[]
 def state(self):
  out=super().state();out.update(seed=self.u(Q),groups_hex=bytes(self.g.mem_read(self.ka['count'],2)+self.g.mem_read(self.ka['groups'],16)).hex());return out
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_world_original.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);maps=source_maps();rows=inputs();versions={};sources={}
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
 for name in OWNERS:
  t=WorldTrace(name,cache);sources[name]=t;results=[]
  for n,row in enumerate(rows):
   t.setup_world(row,bytes.fromhex(maps[row['map']]['rows_hex']));before=t.state()
   try:t.call(t.wo['world'])
   except Exception:print(name,n,json.dumps(row),flush=True);raise
   results.append(dict(before=before,after=t.state(),events=t.events))
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.wo,results=results,observed_instructions=len(t.executed))
 assert versions['retail']['results']==versions['tu11']['results']==versions['u5']['results']
 directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True);pairs=[];listings=[]
 for earlier in ('retail','tu11'):
  old=sources[earlier];later=sources['u5'];oe,ne=old.wo['world'],later.wo['world']
  _,oi=literal(old.code,oe,old.starts);_,ni=literal(later.code,ne,later.starts)
  assert complete_tokens(oi,old.image)==complete_tokens(ni,later.image)
  pairs.append(dict(earlier=earlier,owner='world',instructions=len(oi),complete_shape_equal=True,relocated_operands=references(oi,ni,old.image,later.image)))
  for version,e,ins in((earlier,oe,oi),('u5',ne,ni)):
   p=directory/f'{version}-{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
 data=dict(schema='original-king-world-v1',boundary=__doc__,maps=maps,inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),lockout_sha256=CACHE_PIN,complete_source_pairs=pairs,instruction_listings=listings,
  summary=dict(whole_world_calls=len(rows)*3,whole_initialization_start_calls=len(rows)*6,
   notifications=sum(sum(e['kind']=='notification'for e in r['events'])for r in versions['u5']['results']),
   moves=sum(r['before']['king_hex'][0x1a0*2:0x1a4*2]!=r['after']['king_hex'][0x1a0*2:0x1a4*2]for r in versions['u5']['results'])),
  boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary'],indent=2))
if __name__=='__main__':main()
