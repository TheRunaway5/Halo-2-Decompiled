#!/usr/bin/env python3
"""Whole original player-trait damage and camouflage consumers in three XBEs.

Original registrations, player/options records, current unit, object headers,
unit fields and clock are declared API inputs. Damage15F520 and the reached
King/base trait and membership callbacks execute unchanged. Camouflage refresh,
grant/activation, timer expiry and unit-fade owners execute with actual unit
replication entityNONE and actorNONE; no game/platform return is supplied.
Individual-owner timelines are declared composition, not a whole native/Xbox
world update, renderer, authenticated session, powerup pickup or firing flow.
"""
import argparse,itertools,json,random,re,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_ESI,UC_X86_REG_ESP,UC_X86_REG_XMM0,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_king_scoring import ScoringTrace,player,G,M,R
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha
from oracle_player_lives import C

OUT=ROOT/'analysis/multiplayer/game-variants/king-traits-20261011'
H,T,U=0x4000000,0x4001000,0x4002000
NONE=0xffffffff
PATTERNS={
 'damage':rb'\x51\xf3\x0f\x10\x05....\x57\x8b\xf8\xa1....\x8b\x90\x14\x0c\x00\x00',
 'camo_refresh':rb'\xa1....\x8b\x88\x14\x0c\x00\x00\x8b\x0c\x8d....\x83\xec\x08\x85\xc9',
 'powerup_grant':rb'\x8b\xc8\x81\xe1\xff\xff\x00\x00\x69\xc9\x1c\x02\x00\x00\x53\x56',
 'powerup_tick':rb'\xf3\x0f\x10\x0d....\x53\x55\x25\xff\xff\x00\x00\x56\x57\x8b\xf8',
 'unit_camo_tick':rb'\x53\x55\x8b\x2d....\x25\xff\xff\x00\x00\x56\x57\x8d\x3c\x40\x8b\x45\x44'}
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
def signed(value):return struct.unpack('<h',struct.pack('<H',value&65535))[0]
class TraitTrace(ScoringTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.g.mem_map(H,0x10000);self.owners={};self.queries=[]
  section=next(x for x in self.image.sections if x.name=='.text');raw=self.image.section_bytes(section)
  for kind,pattern in PATTERNS.items():
   matches=[section.va+m.start()for m in re.finditer(pattern,raw,re.S)]
   assert len(matches)==1,(name,kind,matches);self.owners[kind]=matches[0]
  for owner,key in [('powerup_grant','camo_activate'),('powerup_tick','camo_deactivate')]:
   _,ins=literal(self.code,self.owners[owner],self.starts)
   calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'and i.op_str.startswith('0x')]
   assert len(calls)==1;self.owners[key]=calls[0]
  _,tick=literal(self.code,self.owners['unit_camo_tick'],self.starts)
  self.object_global=int(tick[2].op_str.split('[')[1].split(']')[0],16)
  self.w(self.object_global,H);self.w(H+0x44,T);self.w(T+8,U)
  self.trait={'retail':0x2bd9d0,'tu11':0x2c3d10,'u5':0x2c7790}[name]
  self.g.hook_add(UC_HOOK_CODE,self.observe)
 def observe(self,g,pc,n,unused):
  if pc==self.trait:
   s=g.reg_read(UC_X86_REG_ESP);self.queries.append([self.u(s+4),self.u(s+8)])
 def setup(self,row):
  self.setup_score(dict(team_mode=bool(row.get('global_flags',0)&1),flags=row.get('hill_flags',0),players=[player(0),player(3)],
      team_ticks=[0]*8,rate=row.get('rate',30),score=0,team_score=0,team_score_kind=0,engine_present=row.get('engine_present',True)))
  self.w(G+0x184,row.get('global_flags',0));self.g.mem_write(R+0xc1,bytes([row.get('strength',0)]))
  for slot,inside in((0,row.get('attacker_inside',0)),(3,row.get('victim_inside',0))):
   self.g.mem_write(R+slot*0x21c+0x1b8,struct.pack('<h',inside))
  self.g.mem_write(R+0xc0,bytes([row.get('attacker_team',0)&255]));self.g.mem_write(R+3*0x21c+0xc0,bytes([row.get('victim_team',7)&255]))
  self.w(R+3*0x21c+0x194,row.get('protection',0))
  self.g.mem_write(U,bytes(0x800));self.w(U+0xd4,NONE);self.w(U+0x12c,NONE)
  self.w(R+0x2c,NONE if row.get('unit_none',False)else 0x8aaa0000)
  self.w(U+0x134,row.get('unit_flags',0x1020))
  for off,value in((0x2b0,row.get('camo',.5)),(0x2b4,row.get('secondary_camo',.25)),(0x2b8,row.get('fade_speed',.25))):self.w(U+off,bits(value))
  self.g.mem_write(R+0x198,struct.pack('<hh',row.get('powerup_ticks',0),row.get('secondary_ticks',0)))
  self.g.mem_write(C+2,struct.pack('<h',row.get('rate',30)));self.w(C+4,bits(row.get('dt',1/30)))
  self.g.reg_write(UC_X86_REG_FPCW,row.get('cw',0x27f));self.g.reg_write(UC_X86_REG_MXCSR,row.get('mxcsr',0x1f80));self.queries=[]
 def state(self):
  return dict(powerup_ticks=list(struct.unpack('<hh',self.g.mem_read(R+0x198,4))),unit_flags=self.u(U+0x134),
      camo_bits=self.u(U+0x2b0),secondary_camo_bits=self.u(U+0x2b4),fade_speed_bits=self.u(U+0x2b8))
 def damage(self,row):
  self.setup(row);before=bytes(self.g.mem_read(R,16*0x21c));options=bytes(self.g.mem_read(G,0x1200))
  victim={'none':NONE,'self':0x9abc0000}.get(row.get('victim'),0x9abc0003)
  attacker=NONE if row.get('attacker')=='none'else 0x9abc0000
  self.call(self.owners['damage'],registers=((UC_X86_REG_EAX,victim),(UC_X86_REG_ECX,attacker)))
  assert bytes(self.g.mem_read(R,16*0x21c))==before and bytes(self.g.mem_read(G,0x1200))==options
  return dict(multiplier_bits=self.g.reg_read(UC_X86_REG_XMM0)&NONE,queries=self.queries)
 def camo(self,row,operation):
  self.setup(row);before=self.state();players=bytes(self.g.mem_read(R,16*0x21c));unit=bytes(self.g.mem_read(U,0x800))
  if operation=='refresh':self.call(self.owners['camo_refresh'],registers=((UC_X86_REG_ESI,0x9abc0000),))
  elif operation=='timer':self.call(self.owners['powerup_tick'],registers=((UC_X86_REG_EAX,0x9abc0000),))
  else:self.call(self.owners['unit_camo_tick'],registers=((UC_X86_REG_EAX,0x8aaa0000),))
  actual=bytearray(self.g.mem_read(R,16*0x21c));actual[0x198:0x19c]=players[0x198:0x19c];assert bytes(actual)==players
  actual=bytearray(self.g.mem_read(U,0x800))
  for off,size in((0x134,4),(0x2b0,12)):actual[off:off+size]=unit[off:off+size]
  assert bytes(actual)==unit
  return dict(before=before,after=self.state(),queries=self.queries)

def damage_inputs():
 rows=[]
 for strength,timers,hill,global_flags,protection in itertools.product((0,1,2,3,4,127,128,255),
    ((0,0),(1,1),(0,1),(1,0)),(0,4,8,12),(0,1<<12,1<<13,3<<12),(0,1)):
  rows.append(dict(strength=strength,attacker_inside=timers[0],victim_inside=timers[1],hill_flags=hill,global_flags=global_flags,protection=protection))
 for present,attacker,victim,strength in itertools.product((False,True),('none','player'),('none','self','other'),(0,1,2,3,255)):
  rows.append(dict(engine_present=present,attacker=attacker,victim=victim,strength=strength,hill_flags=12,global_flags=3<<12,attacker_inside=1,victim_inside=1,protection=1))
 # Actual variant48 team bit0 and friendly-fire bit7, with independently
 # declared NONE/same/different signed memberships and trait branches.
 for team,friendly,attacker_member,victim_member,traits in itertools.product((False,True),(False,True),(-1,0,7),(-1,0,7),(False,True)):
  rows.append(dict(global_flags=int(team)|(int(friendly)<<7)|((3<<12)if traits else 0),
      attacker_team=attacker_member,victim_team=victim_member))
 rng=random.Random(0x15f520)
 for n in range(512):
  rows.append(dict(strength=rng.randrange(256),attacker_inside=rng.choice((-32768,-1,0,1,32767)),victim_inside=rng.choice((-32768,-1,0,1,32767)),
      hill_flags=rng.randrange(32),global_flags=rng.choice((0,1<<7,1<<12,1<<13,3<<12,(3<<12)|(1<<7))),
      attacker_team=rng.choice((-1,0,7)),victim_team=rng.choice((-1,0,7)),protection=rng.choice((-1,0,1,2147483647))))
 return rows
def camo_inputs():
 rows=[]
 for cw,rate,ticks,inside,hill,global_flags,none in itertools.product((0x27f,0x67f,0xa7f,0xe7f),(30,31,60),(-1,0,1,15,32767),
      (0,1),(0,16),(0,4),(False,True)):
  rows.append(dict(operation='refresh',cw=cw,rate=rate,powerup_ticks=ticks,attacker_inside=inside,hill_flags=hill,global_flags=global_flags,unit_none=none))
 for ticks,secondary,flags in itertools.product((-32768,-1,0,1,2,32767),(-1,0,1,2),(0x1020,0x1028,0x1038)):
  rows.append(dict(operation='timer',powerup_ticks=ticks,secondary_ticks=secondary,unit_flags=flags))
 for flags,camo,secondary,speed,dt in itertools.product((0x1020,0x1028,0x1030,0x1038),(-.1,0,.25,.99,1,1.1),(0,.5,1),(0,.1,.25,1),(1/30,1/60)):
  rows.append(dict(operation='fade',unit_flags=flags,camo=camo,secondary_camo=secondary,fade_speed=speed,dt=dt))
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_trait_consumers_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
 damage=damage_inputs();camo=camo_inputs();sources={};versions={}
 for name in ('retail','tu11','u5'):
  t=TraitTrace(name,cache);sources[name]=t
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.owners,damage=[t.damage(row)for row in damage],
      camouflage=[t.camo(row,row['operation'])for row in camo])
 assert versions['retail']['damage']==versions['tu11']['damage']==versions['u5']['damage']
 assert versions['retail']['camouflage']==versions['tu11']['camouflage']==versions['u5']['camouflage']
 listings=[];pairs=[]
 for name,t in sources.items():
  for key,entry in t.owners.items():
   _,ins=literal(t.code,entry,t.starts);listings.append(dict(image=name,owner=key,entry=entry,
      instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
   if name!='u5':
    other=sources['u5'];_,later=literal(other.code,other.owners[key],other.starts)
    assert complete_tokens(ins,t.image)==complete_tokens(later,other.image),(name,key)
    pairs.append(dict(earlier=name,later='u5',owner=key,complete_instructions=len(ins),complete_shape_equal=True,relocated_operands=references(ins,later,t.image,other.image)))
 data=dict(schema='original-king-trait-consumers-v1',damage_inputs=damage,camouflage_inputs=camo,versions=versions,boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(schema='original-king-trait-consumers-report-v1',oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
   versions={name:dict(image_sha256=row['image_sha256'],owners=row['owners'])for name,row in versions.items()},
   whole_damage_calls=len(damage)*3,whole_camouflage_calls=len(camo)*3,complete_source_pairs=pairs,instruction_listings=listings,
   boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
 print(json.dumps(dict(whole_damage_calls=len(damage)*3,whole_camouflage_calls=len(camo)*3,fixture_sha256=sha(a.fixture),complete_source_pairs=len(pairs)),indent=2))
if __name__=='__main__':main()
