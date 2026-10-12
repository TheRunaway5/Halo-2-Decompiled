#!/usr/bin/env python3
"""Whole unchanged original participant/life round-end query and active wrapper.

Occupied pools, life fields, flags, memberships, retained teams, options,
lifecycle/status, clock and stat banks are declared API inputs. Original engine
registration and every reached game routine execute. Replicated entities and
local users are NONE; recording is disabled. The active wrapper uses the real
Slayer dispatch and literal Lockout matg/mulg tag backing for notifications.
Direct queries also use registered CTF engine bindings for
the literal options+240/engine+2F0 branch. No instruction patch or supplied
game/platform/service return is used. This is not a complete engine/world tick,
round reset, live player allocation, original transport or frontend session.
"""
import argparse,json,random,struct
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EDI,UC_X86_REG_EIP
from oracle_player_lives import LivesTrace,G,M,R
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,literal,complete_tokens,references,sha,CACHE_PIN

OUT=ROOT/'analysis/multiplayer/game-variants/round-end'
OWNERS={
 'retail':dict(count=0x158ab0,collect=0x158c00,query=0x158d20,tick=0x15ba20),
 'tu11':dict(count=0x15a0f0,collect=0x15a240,query=0x15a360,tick=0x15d060),
 'u5':dict(count=0x15b4d0,collect=0x15b620,query=0x15b740,tick=0x15e440)}
O=0x1017800
class RoundEndTrace(LivesTrace):
 def __init__(self,name,cache):
  super().__init__(name)
  page=cache.magic&~4095;self.g.mem_map(page,((len(cache.data)+cache.magic-page+4095)//4096)*4096)
  self.g.mem_write(cache.magic,bytes(cache.data))
  table=0x1100000;size=(max(d&65535 for d in cache.tags)+1)*16;self.g.mem_map(table,(size+4095)//4096*4096)
  for datum,tag in cache.tags.items():
   if tag['offset'] is not None:self.w(table+(datum&65535)*16+8,cache.magic+tag['offset'])
  tag_globals={'retail':(0x4e034c,0x4e3b44),'tu11':(0x4e791c,0x4eb114),'u5':(0x4ecc1c,0x4f0414)}
  matg=next(t for t in cache.tags.values()if t['group']=='matg')
  mg,tg=tag_globals[name];self.w(mg,cache.magic+matg['offset']);self.w(tg,table)
def player(slot,life=1,unit=0x8aaa0000,flags=1,membership=None):
 return dict(slot=slot,datum=0x9abc0000+slot,lives=life,unit=unit,flags=flags,membership=slot if membership is None else membership)
def inputs():
 rng=random.Random(0x15b740);none=0xffffffff
 rosters=[[],[player(0)],[player(0,0,none)],[player(0,-1,none)],
  [player(0,0,none,0)],[player(0),player(3)],[player(0),player(3,0,none)],
  [player(0,0,none),player(3,0,none)],[player(0,membership=7),player(3,membership=7)],
  [player(0,0,none,2),player(15)],[player(0,membership=-1),player(15)],
  [player(0,membership=16),player(15)],[player(0,0,0x8aaa0000),player(15,0,none)],
  [player(0,0,none,0x4001),player(3,1,none),player(15,0,none)]]
 direct=[];wrapped=[]
 for limit in (-1,0,1,2):
  for team in (False,True):
   for mask in (0,1,0x81,0xffff):
    for players in rosters:
     row=dict(limit=limit,team_mode=team,team_mask=mask,source_engine=2,players=players)
     direct.append(row)
     active={**row,'players':[{**p,'membership':7 if team and 8<=p['membership']<16 else p['membership']}for p in players]}
     wrapped.append(dict(**active,round_kind=len(wrapped)%2*5,wins=(-1,0,1,2,29999,30000)[len(wrapped)%6]))
 for n in range(768):
  slots=sorted(rng.sample(range(16),rng.randrange(17)));players=[]
  for slot in slots:
   players.append(player(slot,rng.choice((-32768,-1,0,1,2,32767)),none if rng.randrange(2)else 0x8aaa0000+slot,
    rng.choice((0,1,1,1,2,3,0x4000,0x4001,0x4002,0xffff)),rng.choice((-128,-1,0,1,7,15,16,127,slot))))
  direct.append(dict(limit=rng.choice((-1,0,1,2,32767)),team_mode=bool(n&1),team_mask=rng.randrange(65536),
   source_engine=rng.choice((1,2,9)),variant_240=rng.choice((0,1,2)),engine_2f0=rng.choice((none,0,7,15)),
   engine_present=n%8!=0,players=players))
 for engine in (1,2,9):
  for flag in (-1,0,1,2):
   for designated in (none,0,7,15):
    for force in (False,True):
     direct.append(dict(limit=1,team_mode=True,team_mask=0x81,source_engine=engine,variant_240=flag,engine_2f0=designated,
      players=[player(3,membership=7)] if force else [player(0,0,none),player(3,membership=7)]))
 for lifecycle,status in ((-1,1),(0,1),(1,-1),(1,0),(1,1),(1,2),(2,1),(3,1)):
  for mode in (0,1,2,3,4,255):
   for present in (False,True):
    for team in (False,True):
     wrapped.append(dict(limit=1,team_mode=team,team_mask=0x81,source_engine=2,lifecycle=lifecycle,status=status,mode=mode,
      engine_present=present,round_kind=0,wins=0,players=[player(0),player(3,0,none)]))
 # Exact native regression: team0 retains its lead after both members leave;
 # team1 remains, including an unlimited-life member waiting to respawn.
 departed=dict(limit=0,team_mode=True,team_mask=3,source_engine=2,team_score0=1,
  players=[player(2,-1,none,membership=1),player(3,membership=1)])
 direct.append(departed);wrapped.append(dict(**departed,round_kind=0,wins=0))
 return dict(queries=direct,ticks=wrapped)
def setup(t,row):
 t.setup_lives(row);t.w(G+0x180,row['source_engine']);t.w(G+0x188,row.get('round_kind',0))
 t.w(G+0x240,row.get('variant_240',0));t.w(M+0x2f0,row.get('engine_2f0',0xffffffff))
 t.w(M+0xc14,row['source_engine'] if row.get('engine_present',True)else 0)
 t.g.mem_write(M+0xe,struct.pack('<H',row['team_mask']));t.g.mem_write(M+0x6e,struct.pack('<h',0))
 for i in range(16):t.g.mem_write(M+0x316+i*28,struct.pack('<h',row.get('wins',0)))
 for i in range(8):t.g.mem_write(M+0x4d6+i*18,struct.pack('<h',row.get('wins',0)))
 t.g.mem_write(M+0x4c8,struct.pack('<h',row.get('team_score0',0)))
def bank(t):return bytes(t.g.mem_read(M+0x304,0x254)).hex()
def record(t):
 return dict(flags=t.u(M),status=t.u(M+0xc04),lifecycle=struct.unpack('<h',t.g.mem_read(M+0x6c,2))[0],
  game_over=t.u(G+0x1128)&255,countdown=t.u(G+0x112c),round_countdown=t.u(M+0xc00))
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/round_end_original.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);rows=inputs();versions={};sources={};entries={};calls=0
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
 for name,owners in OWNERS.items():
  t=RoundEndTrace(name,cache);sources[name]=t;entries[name]=set();queries=[];ticks=[]
  for n,row in enumerate(rows['queries']):
   setup(t,row);before=t.state();before_bank=bank(t);seed=0xffffffff if n%2 else 0x1234000f
   t.w(O,seed);t.w(O+4,0xa5a5a5a5)
   end=t.call(owners['query'],(O+4,),((UC_X86_REG_EDI,O),))&255;winner=t.u(O);force=t.u(O+4)&255
   trace=t.calls.copy();entries[name]|=set(t.calls);t.calls=[]
   t.w(O,seed);end_null=t.call(owners['query'],(0,),((UC_X86_REG_EDI,O),))&255
   assert end_null==end and t.u(O)==winner
   assert t.state()==before and bank(t)==before_bank
   entries[name]|=set(t.calls);queries.append(dict(initial_winner=seed,end=end,winner=winner,force=force,
    calls=[f'{e:08x}'for e in trace],null_force_calls=[f'{e:08x}'for e in t.calls]));calls+=2
  for row in rows['ticks']:
   setup(t,row);before_bank=bank(t);before_players=t.state();before=record(t)
   try:t.call(owners['tick'])
   except Exception as error:
    failure=dict(image=name,row=row,eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',error=str(error),calls=[f'{e:08x}'for e in t.calls])
    (a.output.parent/'stopped-wrapper.json').write_text(json.dumps(failure,indent=2)+'\n');raise
   calls+=1;entries[name]|=set(t.calls)
   assert t.state()==before_players,'Round-end wrapper changed player records'
   ticks.append(dict(before_bank=before_bank,after_bank=bank(t),before=before,after=record(t),calls=[f'{e:08x}'for e in t.calls]))
  versions[name]=dict(image_sha256=t.spec['pin'],owners=owners,queries=queries,ticks=ticks)
 listings=[];pairs=[];directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
 for name,t in sources.items():
  for e in sorted(entries[name]):
   _,ins=literal(t.code,e,t.starts);p=directory/f'{name}-{e:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
   listings.append(dict(file=p.name,instructions=len(ins),sha256=sha(p)))
  if name=='u5':continue
  u=sources['u5']
  for key in OWNERS[name]:
   _,oi=literal(t.code,OWNERS[name][key],t.starts);_,ni=literal(u.code,OWNERS['u5'][key],u.starts)
   assert complete_tokens(oi,t.image)==complete_tokens(ni,u.image),(name,key)
   pairs.append(dict(earlier=name,later='u5',owner=key,instructions=len(oi),complete_shape_equal=True,
    relocated_operands=references(oi,ni,t.image,u.image)))
 data=dict(schema='original-round-end-v1',boundaries=__doc__,whole_calls=calls,lockout_sha256=CACHE_PIN,
  inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),instruction_listings=listings,complete_source_pairs=pairs,
  dependencies={p:sha(ROOT/p)for p in ('tools/oracle_player_lives.py','tools/oracle_result_statistics.py','tools/oracle_game_over.py',
   'tools/oracle_game_variants.py','tools/oracle_mp_full_scoreboard_1_5.py','tools/inspect_multiplayer_hud.py',
   'tools/inspect_multiplayer.py','tools/reference/halo2-decompiled/tools/xbe.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(whole_calls=calls,queries_per_image=len(rows['queries']),wrappers_per_image=len(rows['ticks']),
  fixture_sha256=sha(a.fixture),source_pairs=len(pairs),instruction_listings=len(listings)),indent=2))
if __name__=='__main__':main()
