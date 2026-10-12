#!/usr/bin/env python3
"""Execute unchanged original result-stat and match-stat writers in three XBEs.

Raw recording bytes, both banks, selected engine, live-pool membership and
options are declared API inputs. Actual engine registration executes. Engine
replication entity is NONE; no allocator, platform, game or service return is
supplied. Other fields remain nonzero poison. These are whole writer calls,
not whole combat, retained identity capture, report rendering or lobby return.
"""
import argparse,csv,json,random,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_ESI
from oracle_game_over import SPECS as BASE
from oracle_game_variants import Source,ROOT
from oracle_mp_full_scoreboard_1_5 import _Code,literal,complete_tokens,references,sha
from oracle_game_end import EndTrace,fixture as end_fixture,run as end_run,bank as end_bank
from oracle_mp_full_scoreboard_1_5 import HudCache,M as END_M,G as END_G

OUT=ROOT/'analysis/multiplayer/game-variants/result-statistics'
FIXTURE=ROOT/'tests/fixtures/result_statistics_original.json'
SPECS={
 'retail':dict(set=0x1968b0,add=0x1967d0,player=0x1e9ce0,team=0x1e9df0,table=0x46ddc8,recording=0x510ca0,bank=0x511bf4,pool=0x4e8c24),
 'tu11':dict(set=0x1983b0,add=0x1982d0,player=0x1ed6a0,team=0x1ed7b0,table=0x4752b0,recording=0x518490,bank=0x5193e4,pool=0x4f01f4),
 'u5':dict(set=0x19b2b0,add=0x19b1d0,player=0x1f1fc0,team=0x1f20d0,table=0x47a388,recording=0x51dfa0,bank=0x51eef4,pool=0x4f54f4)}
G,M,P,R,K=0x1000000,0x1010000,0x1011000,0x1012000,0x1015000
def seed():return b''.join(struct.pack('<H',(i*73+0x3141)&65535) for i in range(0x2020))
def short(value):return struct.unpack('<h',struct.pack('<H',value&65535))[0]
class StatisticsTrace(Source):
 def __init__(self,name):
  super().__init__(BASE[name]);self.name=name;self.s=SPECS[name]
  self.g.mem_map(M,0x6000)
  self.code=_Code(self.image,'.text')
  self.starts={int(r['address'],16)for r in csv.DictReader((ROOT/self.spec['tsv']).open(),delimiter='\t')}
  self.calls=[];self.g.hook_add(UC_HOOK_CODE,self.hook)
  self.call(self.spec['ctor']);self.constructor_calls=self.calls.copy()
  self.w(self.spec['globals'],G);self.w(self.spec['engine'],M);self.w(self.s['pool'],P)
  self.w(P+0x24,0x21c);self.w(P+0x44,R)
 def hook(self,g,a,n,user):
  if a in self.starts:self.calls.append(a)
 def w(self,a,v):self.g.mem_write(a,struct.pack('<I',v&0xffffffff))
 def setup(self,flags):
  self.g.mem_write(self.s['recording'],bytes(flags));self.g.mem_write(self.s['bank'],seed());self.calls=[]
 def word(self,player,slot,stat):return self.s['bank']+(slot*437+stat)*2 if player else self.s['bank']+0x3aa0+(slot*45+stat)*2
 def destinations(self,stat,player,team):
  return [dict(player=is_player,slot=slot,stat=stat,value=int.from_bytes(self.g.mem_read(self.word(is_player,slot,stat),2),'little'))
   for is_player,slot in ((True,player),(False,team)) if slot!=-1]
 def check_other(self,before,targets):
  after=bytearray(self.g.mem_read(self.s['bank'],0x4040))
  for row in targets:
   off=self.word(row['player'],row['slot'],row['stat'])-self.s['bank'];after[off:off+2]=before[off:off+2]
  assert bytes(after)==before,'Unclassified retained field changed'
  assert set(self.calls)<=set(self.s[k]for k in ('set','add','player','team')),(self.name,self.calls)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=FIXTURE);a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);a.fixture.parent.mkdir(parents=True,exist_ok=True)
 versions={};sources={};values=(-2147483648,-32769,-30001,-1,0,1,32,255,1023,32767,2147483647)
 for name in BASE:
  t=StatisticsTrace(name);sources[name]=t;direct=[];writers=[];rng=random.Random(0x19b2b0)
  raw=t.image.read(t.s['table'],45*16)
  descriptors=[dict(minimum=struct.unpack_from('<H',raw,i*16)[0],maximum=struct.unpack_from('<H',raw,i*16+2)[0],remaining_hex=raw[i*16+4:(i+1)*16].hex())for i in range(45)]
  for op in ('set','add'):
   for stat in range(45):
    for value in values:
     for player,team in ((0,-1),(-1,15),(15,7)):
      flags=(1,0);t.setup(flags);before=bytes(t.g.mem_read(t.s['bank'],0x4040));targets=t.destinations(stat,player,team)
      if op=='set':t.call(t.s[op],registers=((UC_X86_REG_ECX,player),(UC_X86_REG_EAX,team),(UC_X86_REG_ESI,stat),(UC_X86_REG_EBX,value)))
      else:t.call(t.s[op],(value,),((UC_X86_REG_ECX,stat),(UC_X86_REG_EDX,team),(UC_X86_REG_EAX,player)))
      t.check_other(before,targets)
      direct.append(dict(op=op,flags=flags,stat=stat,player=player,team=team,value=value,before=targets,after=t.destinations(stat,player,team)))
   for flags in ((0,0),(0,255),(1,1),(255,0)):
    for player,team in ((-1,-1),(3,-1),(-1,0),(15,15)):
     stat=7;value=-1;t.setup(flags);before=bytes(t.g.mem_read(t.s['bank'],0x4040));targets=t.destinations(stat,player,team)
     if op=='set':t.call(t.s[op],registers=((UC_X86_REG_ECX,player),(UC_X86_REG_EAX,team),(UC_X86_REG_ESI,stat),(UC_X86_REG_EBX,value)))
     else:t.call(t.s[op],(value,),((UC_X86_REG_ECX,stat),(UC_X86_REG_EDX,team),(UC_X86_REG_EAX,player)))
     t.check_other(before,targets);direct.append(dict(op=op,flags=flags,stat=stat,player=player,team=team,value=value,before=targets,after=t.destinations(stat,player,team)))
  for n in range(640):
   is_player=n<512;slot=rng.choice((0,3,15)) if is_player else rng.choice((0,2,7));field=n%(12 if is_player else 9)
   delta=short(rng.choice((-32768,-30001,-1,0,1,30000,32767,65535)))
   stat=rng.choice((-1,n%45));flags=rng.choice(((0,0),(1,0),(255,0),(1,255)))
   t.setup(flags);t.g.mem_write(M,bytes(0xc18));t.w(M+0x28,-1);present=bool(n&1);team_mode=bool(n&2)
   t.w(M+0xc14,2 if present else 0);t.w(G+0x184,int(team_mode));t.w(P+0x38,rng.choice((0,4,16)))
   membership=rng.choice((-1,0,2,7));occupied=rng.choice((0,1,0x8000));t.g.mem_write(R+slot*0x21c,struct.pack('<H',occupied));t.g.mem_write(R+slot*0x21c+0xc0,bytes([membership&255]))
   team_destination=membership if is_player and field<9 and present and team_mode and slot<t.u(P+0x38) and occupied and membership!=-1 else -1
   publish_team=is_player and field<9
   initial=bytearray(rng.randbytes(0x254));off=4+slot*28+field*2 if is_player else 0x1c4+slot*18+field*2
   struct.pack_into('<h',initial,off,rng.choice((-32768,-30001,-30000,-1,0,29999,30000,32767)))
   t.g.mem_write(K,bytes(initial));targets=t.destinations(stat,slot if is_player else -1,team_destination if is_player else slot) if stat!=-1 else []
   before=bytes(t.g.mem_read(t.s['bank'],0x4040))
   if is_player:t.call(t.s['player'],(K,field,delta,int(publish_team)),((UC_X86_REG_EAX,0x9abc0000+slot),(UC_X86_REG_ECX,stat)))
   else:t.call(t.s['team'],(slot,delta),((UC_X86_REG_EAX,field),(UC_X86_REG_ECX,stat),(UC_X86_REG_EDX,K)))
   t.check_other(before,targets)
   writers.append(dict(player=is_player,slot=slot,field=field,delta=delta,stat=stat,flags=flags,team_destination=team_destination,
    selected_engine=present,team_mode=team_mode,membership=membership,pool_high_water=t.u(P+0x38),occupied=occupied,
    before_bank_hex=bytes(initial).hex(),after_bank_hex=bytes(t.g.mem_read(K,0x254)).hex(),before=targets,
    after=t.destinations(stat,slot if is_player else -1,team_destination if is_player else slot)if stat!=-1 else [],calls=[f'{e:08x}'for e in t.calls]))
  versions[name]=dict(image_sha256=BASE[name]['pin'],owners={k:f'{v:08x}'for k,v in t.s.items()},descriptors=descriptors,
   table_raw_hex=raw.hex(),table_sha256=__import__('hashlib').sha256(raw).hexdigest(),direct=direct,writers=writers,
   constructor_calls=[f'{e:08x}'for e in t.constructor_calls],executed_instructions=len(t.executed))
 pairs=[];listings=[];listingdir=a.output.parent/'instructions';listingdir.mkdir(exist_ok=True)
 for name,t in sources.items():
  for key in ('set','add','player','team'):
   f,ins=literal(t.code,t.s[key],t.starts);p=listingdir/f'{name}-{t.s[key]:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
   if name!='u5':
    later=sources['u5'];lf,li=literal(later.code,later.s[key],later.starts);assert complete_tokens(ins,t.image)==complete_tokens(li,later.image)
    mapping={t.s[k]:later.s[k]for k in ('set','add','player','team')}
    for oi,ni in zip(ins,li):
     if oi.mnemonic=='call' and oi.op_str.startswith('0x') and int(oi.op_str,16) in mapping:assert int(ni.op_str,16)==mapping[int(oi.op_str,16)]
    pairs.append(dict(earlier=name,owner=key,instructions=len(ins),complete_instruction_shape_equal=True,relocated_operands=references(ins,li,t.image,later.image)))
 # The actual round-end owner includes an inlined player-win publication.
 # Exercise it with recording enabled, rather than infer it from a leaf call.
 et=EndTrace(HudCache(ROOT/'assets/disc/maps/lockout.map'));et.fixture();end_run(et,'registration',0x3825d0)
 awards=[]
 for team in (False,True):
  roster=[dict(slot=i,datum=0x80000000+i,membership=(7 if i in(0,3,15)else 0),flags=(0x4002 if i==3 else 1))for i in (0,3,15)]
  for winner in ((0xffffffff,0,7) if team else (0xffffffff,0x80000000,0x8000000f,0x11110003)):
   for wins in (-32768,-30001,-1,0,1,31,32,29999,30000,32767):
    initial=bytearray((i*17+0x3f)&255 for i in range(0x254))
    for i in range(16):struct.pack_into('<h',initial,0x12+i*28,wins)
    for i in range(8):struct.pack_into('<h',initial,0x1d2+i*18,wins)
    end_fixture(et,initial,roster,team);et.g.mem_write(0x51dfa0,b'\x01\x00');et.g.mem_write(0x51eef4,seed())
    before=end_bank(et);call=end_run(et,f'recorded-award-{team}-{winner:08x}-{wins}',0x15ddc0,(0,),((UC_X86_REG_EBX,winner),))
    targets=[dict(player=p,slot=s,stat=6,value=int.from_bytes(et.g.mem_read(0x51eef4+((s*437+6)*2 if p else 0x3aa0+(s*45+6)*2),2),'little'))for p,s in [(True,i)for i in range(16)]+[(False,i)for i in range(16)]]
    retained=bytearray(et.g.mem_read(0x51eef4,0x4040))
    for row in targets:
     off=(row['slot']*437+6)*2 if row['player'] else 0x3aa0+(row['slot']*45+6)*2;retained[off:off+2]=seed()[off:off+2]
    assert bytes(retained)==seed() and not call['services']
    awards.append(dict(team=team,winner=f'{winner:08x}',roster=roster,before_bank_hex=before,after_bank_hex=end_bank(et),after=targets))
 callers=[]
 for e in (0x19a430,0x15efa0,0x15ddc0,0x1f2890,0x1f2a50,0x1f2280):
  f,ins=literal(et.code,e,et.starts);p=listingdir/f'u5-caller-{e:08x}.asm'
  p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins))
  callers.append(dict(file=p.name,sha256=sha(p),instructions=len(ins),kind='complete_static_owner_listing'))
 data=dict(schema='original-result-statistics-v1',boundary=__doc__,instruction_patches=[],supplied_returns=[],versions=versions,recorded_awards=awards)
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),complete_source_pairs=pairs,instruction_listings=listings,caller_instruction_listings=callers,
  dependencies={p:sha(ROOT/p)for p in ('tools/oracle_game_variants.py','tools/oracle_game_over.py','tools/oracle_game_end.py','tools/oracle_mp_full_scoreboard_1_5.py','tools/reference/halo2-decompiled/tools/xbe.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(fixture=str(a.fixture),fixture_sha256=sha(a.fixture),cases=sum(len(v['direct'])+len(v['writers'])for v in versions.values())+len(awards),pairs=len(pairs)),indent=2))
if __name__=='__main__':main()
