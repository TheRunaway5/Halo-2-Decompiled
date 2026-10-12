#!/usr/bin/env python3
"""Execute unchanged original King traits, retained kill stats and phase dispatch.

Registered engine tables, raw player timers/team bytes, option flags and actual
retained descriptors/banks are original inputs. Trait fallback and retained add
owners execute fully. Phase cases use real Lockout tags, an empty occupied pool
and declared playback mode4. No instruction patch, native output or supplied
game/platform/service return is used. These are isolated callback and playback
transition calls, not live combat, ordinary round reset, HUD or network proof.
"""
import argparse, hashlib, itertools, json, random, struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDX, UC_X86_REG_ESP
from oracle_result_statistics import StatisticsTrace, G, M, P, R
from oracle_round_end import RoundEndTrace
from oracle_king_engine import OWNERS as KING
from oracle_mp_full_scoreboard_1_5 import ROOT, HudCache, CACHE_PIN, literal, complete_tokens, references, sha

OWNERS = {
 'retail': dict(traits=0x2bd9d0, base=0x24dda0, death=0x2bda60, team=0x15eaf0,
               adapter=0x15b930, phase=0x15c550, prepare=0x157ae0),
 'tu11': dict(traits=0x2c3d10, base=0x253f40, death=0x2c3da0, team=0x160150,
             adapter=0x15cf70, phase=0x15dbc0, prepare=0x159180),
 'u5': dict(traits=0x2c7790, base=0x25bcc0, death=0x2c7820, team=0x161530,
           adapter=0x15e350, phase=0x15efa0, prepare=0x15a560),
}
OUT = ROOT/'analysis/multiplayer/game-variants/king-callbacks-20261011'

def trait_inputs():
 return [dict(inside_ticks=t, global_flags=g, hill_flags=h, kind=k)
         for t,g,h,k in itertools.product((-32768,-1,0,1,32767),
           (0,4,0x1000,0x2000,0x3004,0xffffffff),
           (0,4,8,16,28,0xffffffff),(-1,0,1,2,3,4,99))]

def death_inputs():
 rows=[]
 for victim,attacker,credit,betrayal,team,recording in itertools.product(
       (-1,0,1),(-1,0,1),('none','self','other'),(0,1),(-1,7),((0,0),(1,0),(1,1))):
  rows.append(dict(victim_inside=victim,attacker_inside=attacker,credit=credit,
    betrayal=betrayal,team_mode=team!=-1,membership=team,recording=recording,
    fourth=0,initial_word=0x8000))
 rng=random.Random(0x2c7820)
 for n in range(192):
  rows.append(dict(victim_inside=rng.choice((-32768,-1,0,1,32767)),
    attacker_inside=rng.choice((-32768,-1,0,1,32767)),credit=rng.choice(('none','self','other')),
    betrayal=rng.choice((0,1,255,256)),team_mode=bool(n&1),membership=rng.choice((-1,0,7)),
    recording=rng.choice(((0,0),(1,0),(1,1),(255,0))),fourth=rng.getrandbits(32),
    initial_word=rng.choice((0,1,32766,32767,32768,65534,65535))))
 return rows

class Callbacks(StatisticsTrace):
 def __init__(self,name):
  super().__init__(name);self.a=OWNERS[name];self.events=[]
  self.g.hook_add(UC_HOOK_CODE,self.observe)
 def observe(self,g,a,n,user):
  if a==self.s['add']:
   sp=g.reg_read(UC_X86_REG_ESP)
   self.events.append(dict(stat=g.reg_read(UC_X86_REG_ECX),
       player=g.reg_read(UC_X86_REG_EAX),team=g.reg_read(UC_X86_REG_EDX),
       delta=struct.unpack('<i',g.mem_read(sp+4,4))[0]))
 def setup_callback(self):
  self.g.mem_write(G,bytes(0x1200));self.g.mem_write(M,bytes(0xc18))
  self.g.mem_write(R,bytes(16*0x21c));self.w(M+0xc14,4)
  self.w(P+0x38,16);self.calls=[];self.events=[]

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_callbacks_original.json')
 a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');rows=dict(traits=trait_inputs(),death=death_inputs())
 versions={};sources={};blobs={};listings=[];pairs=[]
 def blob(raw):
  digest=hashlib.sha256(raw).hexdigest();blobs[digest]=raw.hex();return digest
 for name,owners in OWNERS.items():
  t=Callbacks(name);sources[name]=t;table=t.u(t.u(t.spec['table']+16))
  assert t.u(table+35*4)==owners['traits'] and t.u(table+30*4)==owners['death']
  slots={str(slot):t.u(table+slot*4)for slot in (1,3,26,30,32,33,35,36)}
  traits=[];deaths=[]
  for row in rows['traits']:
   t.setup_callback();t.w(G+0x184,row['global_flags']);t.w(G+0x22c,row['hill_flags'])
   t.g.mem_write(R+3*0x21c+0x1b8,struct.pack('<h',row['inside_ticks']))
   before=bytes(t.g.mem_read(R,16*0x21c))
   result=t.call(owners['traits'],(0x9abc0003,row['kind']))&255
   assert bytes(t.g.mem_read(R,16*0x21c))==before
   traits.append(dict(returned_al=result,base_called=owners['base']in t.calls))
  for row in rows['death']:
   t.setup_callback();t.setup(row['recording']);t.w(G+0x184,int(row['team_mode']))
   for slot in (3,15):
    t.g.mem_write(R+slot*0x21c,struct.pack('<H',0x9abc))
    t.g.mem_write(R+slot*0x21c+0xc0,bytes([row['membership']&255]))
    t.g.mem_write(R+slot*0x21c+0x1b8,struct.pack('<h',row['attacker_inside']if slot==3 else row['victim_inside']))
   for stat in (27,28):
    for player in (True,False):
     for slot in range(16):t.g.mem_write(t.word(player,slot,stat),struct.pack('<H',row['initial_word']))
   attacker={'none':0xffffffff,'self':0x9abc000f,'other':0x9abc0003}[row['credit']]
   before=blob(bytes(t.g.mem_read(t.s['bank'],0x4040)));players=bytes(t.g.mem_read(R,16*0x21c))
   match=bytes(t.g.mem_read(M,0xc18));t.events=[];t.calls=[]
   t.call(owners['death'],(attacker,0x9abc000f,row['betrayal'],row['fourth']))
   assert bytes(t.g.mem_read(R,16*0x21c))==players
   assert bytes(t.g.mem_read(M,0xc18))==match,'King death wrote shared match fields'
   deaths.append(dict(before_bank=before,after_bank=blob(bytes(t.g.mem_read(t.s['bank'],0x4040))),
     retained_add_calls=t.events.copy(),calls=[f'{e:08x}'for e in t.calls]))
  p=RoundEndTrace(name,cache);p.setup_lives(dict(limit=0,mode=4,lifecycle=0,status=1,players=[]))
  p.w(M+0xc14,4);p.w(G+0x180,4);p.w(KING[name]['scenario'],cache.magic+cache.scenario)
  watched={slots['1']:'initialize',slots['3']:'round_initialize',slots['36']:'phase_callback',
           owners['phase']:'phase_transition',owners['prepare']:'round_prepare'};events=[]
  def observe(g,at,n,user):
   if at in watched:events.append(dict(owner=watched[at],address=f'{at:08x}'))
  p.g.hook_add(UC_HOOK_CODE,observe);p.call(slots['1']);phase=[]
  for status in (1,1,2,3,1):
   p.w(M+0xc04,status);events=[];old=struct.unpack('<h',p.g.mem_read(M+0x6c,2))[0]
   before=blob(bytes(p.g.mem_read(M+0xfc,0x1dc)));p.call(owners['phase'])
   phase.append(dict(status=status,lifecycle_before=old,lifecycle_after=struct.unpack('<h',p.g.mem_read(M+0x6c,2))[0],
     events=events.copy(),before_king=before,after_king=blob(bytes(p.g.mem_read(M+0xfc,0x1dc)))))
  versions[name]=dict(image_sha256=t.spec['pin'],vtable=f'{table:08x}',slots=slots,owners=owners,
    descriptors={str(n):t.image.read(t.s['table']+16*n,16).hex()for n in (27,28)},
    traits=traits,death=deaths,phase=phase)
 for name in ('retail','tu11'):
  assert versions[name]['traits']==versions['u5']['traits']
  for old,new in zip(versions[name]['death'],versions['u5']['death']):
   assert {k:v for k,v in old.items()if k!='calls'}=={k:v for k,v in new.items()if k!='calls'}
  def phase_rows(version):
   return [{**r,'events':[e['owner']for e in r['events']]}for r in versions[version]['phase']]
  assert phase_rows(name)==phase_rows('u5')
 for name,t in sources.items():
  for key in ('traits','base','death','team','adapter','phase','prepare'):
   entry=t.a[key];_,ins=literal(t.code,entry,t.starts)
   listings.append(dict(image=name,owner=key,entry=f'{entry:08x}',
     instructions=[f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'for i in ins]))
   if name!='u5':
    u=sources['u5'];_,later=literal(u.code,u.a[key],u.starts)
    assert complete_tokens(ins,t.image)==complete_tokens(later,u.image),(name,key)
    pairs.append(dict(earlier=name,owner=key,complete_shape_equal=True,
      relocated_operands=references(ins,later,t.image,u.image)))
 data=dict(schema='original-king-callbacks-v1',boundary=__doc__,inputs=rows,versions=versions,
   raw_blobs=blobs,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),lockout_sha256=CACHE_PIN,
   image_sha256={name:version['image_sha256']for name,version in versions.items()},
   dependencies={path:sha(ROOT/path)for path in ('tools/oracle_result_statistics.py',
      'tools/oracle_round_end.py','tools/oracle_king_engine.py','tools/oracle_game_over.py',
      'tools/oracle_game_variants.py','tools/oracle_player_lives.py','tools/oracle_mp_full_scoreboard_1_5.py')},
   whole_calls=3*(len(rows['traits'])+len(rows['death'])+6),traits_per_image=len(rows['traits']),
   deaths_per_image=len(rows['death']),source_pairs=pairs,instruction_listings=listings,
   boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps({k:v for k,v in report.items()if k not in ('instruction_listings','source_pairs')},indent=2))

if __name__=='__main__':main()
