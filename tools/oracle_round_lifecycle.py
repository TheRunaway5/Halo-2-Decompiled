#!/usr/bin/env python3
"""Execute unchanged original Slayer round-phase and post-world owners.

Actual Lockout scenario/matg/mulg metadata, registered engine callbacks and
original mainmenu variant buffers are used. Completed first-to-three sequences
explicitly disable parameter11 through the original setter; unchanged authored
Elimination retains its full-map-reset dependency stop. Player pools, NONE current units,
NONE replication entities/local users, an explicitly empty object pool, clocks
and raw banks are declared API inputs. Positive cases execute every reached
instruction without patches, supplied returns or skipped callbacks. Empty-world
round reset is not evidence for live object/map reset, spawn, combat, original
networking, HUD notifications or a normal playable Elimination match. Separate
live-object dependency probes deliberately retain their original stops.
"""
import argparse, hashlib, json, random, struct
from pathlib import Path
from unicorn import UC_HOOK_MEM_INVALID
from unicorn.x86_const import UC_X86_REG_ESI, UC_X86_REG_EIP
from oracle_round_end import RoundEndTrace, setup, player, bank
from oracle_player_lives import G, M, R, C, OWNERS as LIFE
from oracle_round_end import OWNERS as END
from oracle_game_variants import Source, VERSIONS, MENU_PIN, Cache
from oracle_mp_full_scoreboard_1_5 import ROOT, HudCache, literal, complete_tokens, references, sha, CACHE_PIN

OUT=ROOT/'analysis/multiplayer/game-variants/round-lifecycle'
OWNERS={
 'retail':dict(phase=0x15c550,post_world=0x15ca10,initialize_round=0x157ae0,initialize_rows=0x15ace0,
  reset_objects=0x159e20,reset_rows=0x15ad30,reset_players=0x157a80,request=0x15fe50,scenario=0x4e0350,objects=0x4e0300,ai_initialize=0x1c79e0,ai_global=0x4f55d0),
 'tu11':dict(phase=0x15dbc0,post_world=0x15e070,initialize_round=0x159180,initialize_rows=0x15c320,
  reset_objects=0x15b460,reset_rows=0x15c370,reset_players=0x159120,request=0x1614f0,scenario=0x4e7920,objects=0x4e78d0,ai_initialize=0x1cb3d0,ai_global=0x4fcda8),
 'u5':dict(phase=0x15efa0,post_world=0x15f450,initialize_round=0x15a560,initialize_rows=0x15d700,
  reset_objects=0x15c840,reset_rows=0x15d750,reset_players=0x15a500,request=0x1628d0,scenario=0x4ecc20,objects=0x4ecbd0,ai_initialize=0x1cfc90,ai_global=0x5020a8)}
OP, OH, OBJECT, AI=0x1120000,0x1121000,0x1130000,0x1140000

def i16(v):return struct.unpack('<h',struct.pack('<H',v&65535))[0]
def i32(v):return struct.unpack('<i',struct.pack('<I',v&0xffffffff))[0]

class LifecycleTrace(RoundEndTrace):
 def __init__(self,name,cache):
  super().__init__(name,cache);self.phase_owners=OWNERS[name]
  self.scenario=cache.magic+cache.scenario
  self.w(self.phase_owners['scenario'],self.scenario)
  self.g.mem_map(OP,0x2000);self.g.mem_map(OBJECT,0x2000);self.g.mem_map(AI,0x1000)
  self.starts.add(self.phase_owners['ai_initialize'])
  self.w(self.phase_owners['objects'],OP);self.w(OP+0x44,OH);self.w(OP+0x38,0)
  self.faults=[];self.g.hook_add(UC_HOOK_MEM_INVALID,self.fault)
  self.whole_calls=0
 def fault(self,g,access,address,size,value,user):
  self.faults.append(dict(access=access,address=f'{address:08x}',size=size,eip=f'{g.reg_read(UC_X86_REG_EIP):08x}'))
  return False
 def invoke(self,key,args=(),regs=()):
  self.calls=[];self.call(self.phase_owners[key] if isinstance(key,str)else key,args,regs)
  self.whole_calls+=1
  return [f'{e:08x}'for e in self.calls]
 def prepare(self,row):
  setup(self,dict(limit=row.get('limit',1),team_mode=row.get('team_mode',False),team_mask=row.get('team_mask',0),
   source_engine=2,lifecycle=row.get('lifecycle',0),status=row.get('status',1),mode=row.get('mode',1),
   round_kind=5,engine_present=row.get('engine_present',True),players=row.get('players',[player(0,0,0xffffffff),player(3,0,0xffffffff)])))
  self.g.mem_write(M+0x6e,struct.pack('<h',row.get('index',0)));self.w(M+0xc00,row.get('countdown',0))
  self.g.mem_write(C+2,struct.pack('<h',row.get('rate',30)));self.w(C+8,row.get('tick',12345))
  self.w(G+0x190,row.get('time_limit',120));self.w(G+0x184,int(row.get('team_mode',False)))
  self.g.mem_write(self.s['recording'],bytes([int(row.get('recording',False)),row.get('frozen',0)]))
  self.w(OP+0x38,0);self.g.mem_write(OH,bytes(0x1000));self.faults=[]
  self.invoke('initialize_rows',regs=((UC_X86_REG_ESI,M+0x7dc),))
  self.w(M,row.get('flags',0xa5a50000));self.w(M+0x70,0x12345678)
  self.g.mem_write(M+0x74,b'\xa5'*108);self.g.mem_write(M+0xe0,b'\xa5\xa5')
  rng=random.Random(row.get('seed',0x15efa0));self.g.mem_write(M+0x304,rng.randbytes(0x254))
  # Only the raw signed-short row clocks are poisoned. Original initialization
  # supplied all NONE object bindings, avoiding an invented delete-service return.
  for n in range(self.u(self.scenario+0x120)):
   self.g.mem_write(M+0x7dc+n*8+4,struct.pack('<hh',37,-19))
  for p in row.get('players',[player(0,0,0xffffffff),player(3,0,0xffffffff)]):
   at=R+p['slot']*0x21c;self.w(at+0x164,987);self.g.mem_write(at+0x1a7,b'\x71\x52')
   self.g.mem_write(M+0x558+p['slot']*24,b'\x63')
 def record(self):
  return dict(flags=self.u(M),status=i32(self.u(M+0xc04)),lifecycle=i16(self.u(M+0x6c)&65535),
   index=i16((self.u(M+0x6c)>>16)&65535),start_tick=self.u(M+0x70),round_countdown=i32(self.u(M+0xc00)),
   clock_seconds=i16(self.u(M+0xe0)&65535),game_over=self.u(G+0x1128)&255,
   game_over_countdown=i32(self.u(G+0x112c)),recording=list(self.g.mem_read(self.s['recording'],2)),
   bank_hex=bank(self),ranking_hex=bytes(self.g.mem_read(M+0x74,108)).hex(),
   players_hex=bytes(self.g.mem_read(R,16*0x21c)).hex(),
   player_respawn_rows_hex=bytes(self.g.mem_read(M+0x558,16*24)).hex(),
   netgame_rows_hex=bytes(self.g.mem_read(M+0x7dc,self.u(self.scenario+0x120)*8)).hex())

def assert_phase(before,after,row):
 status=before['status'];expected=dict(before)
 if status==before['lifecycle']:
  assert after==before;return
 expected['lifecycle']=i16(status)
 if status==2:
  expected['round_countdown']=row['rate']*5
  raw=bytearray.fromhex(before['bank_hex']);raw[0]=0;expected['bank_hex']=raw.hex()
  if row.get('mode',1)!=4 and row.get('recording',False):expected['recording']=[1,1]
 elif status==3:
  expected['index']=i16(before['index']+1);expected['round_countdown']=row['rate']
 if status!=1:assert after==expected,(row,after,expected)
 else:
  expected['start_tick']=row.get('tick',12345);expected['ranking_hex']='00'*108
  expected['clock_seconds']=i16(row.get('time_limit',120))
  raw=bytearray.fromhex(before['players_hex']);respawn=bytearray.fromhex(before['player_respawn_rows_hex'])
  for p in row.get('players',[player(0,0,0xffffffff),player(3,0,0xffffffff)]):
   at=p['slot']*0x21c
   if not p['flags']&2:
    raw[at+0x1a7:at+0x1a9]=b'\0\xff'
    if row.get('mode',1)!=4:
     struct.pack_into('<h',raw,at+0x1ac,i16(row.get('limit',1)or -1))
     struct.pack_into('<I',raw,at+0x164,0);struct.pack_into('<H',raw,at+2,p['flags']|8)
     respawn[p['slot']*24]=0
  expected['players_hex']=raw.hex();expected['player_respawn_rows_hex']=respawn.hex()
  stats=bytearray.fromhex(before['bank_hex']);stats[0]=1
  if row.get('mode',1)!=4:
   expected['flags']=before['flags']|0x20
   if row.get('recording',False):expected['recording']=[1,0]
   for at in [4+n*28 for n in range(16)]+[0x1c4+n*18 for n in range(8)]:
    score,total=struct.unpack_from('<hh',stats,at)
    if score:struct.pack_into('<hh',stats,at,0,i16(score+total))
   for n in range(16):
    stats[4+n*28+18:4+n*28+22]=bytes(4)
    stats[4+n*28+24:4+n*28+28]=bytes(4)
   if before['index']==0:
    rows=bytearray.fromhex(before['netgame_rows_hex'])
    for n in range(len(rows)//8):struct.pack_into('<hh',rows,n*8+4,1,0)
    expected['netgame_rows_hex']=rows.hex()
  expected['bank_hex']=stats.hex()
  assert after==expected,dict(row=row,differing_fields=[k for k in expected if after[k]!=expected[k]])

def variants():
 cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');assert sha(cache.path)==MENU_PIN
 rows=cache.inspect_variants()['variants'];source=Source(VERSIONS['retail']);out={}
 assert len(rows)==39
 for n in (7,):
  row=rows[n];source.init(row['engine'])
  for parameter,value in row['overrides'].items():source.set(parameter,value)
  out[str(n)]=dict(index=n,**row,raw_hex=source.raw())
  source.set(91,1)
  out['7_team']=dict(index=n,**row,raw_hex=source.raw(),explicit_edits={'91':1})
  source.set(11,0)
  out['7_team_reset_map_disabled']=dict(index=n,**row,raw_hex=source.raw(),explicit_edits={'11':0,'91':1})
  source.set(91,0)
  out['7_reset_map_disabled']=dict(index=n,**row,raw_hex=source.raw(),explicit_edits={'11':0})
 return out

def sequence(t,variant,team=False,expect_map_stop=False):
 roster=[player(0,0,0xffffffff,membership=0),player(3,0,0xffffffff,membership=7 if team else 3)]
 t.prepare(dict(players=roster,team_mode=team,team_mask=0x81 if team else 0))
 t.g.mem_write(G+0x13c,bytes.fromhex(variant['raw_hex']))
 assert bool(t.u(G+0x184)&1)==team
 t.g.mem_write(M+0x304,bytes(0x254));events=[]
 def capture(name,calls):events.append(dict(event=name,tick=t.u(C+8),record=t.record(),calls=calls))
 capture('first_active',t.invoke('phase'))
 for round_index in range(3):
  # Original life/death owner, with current units NONE and notification cause41.
  capture('life_exhausted',t.invoke(LIFE[t.name]['death'],(0xffffffff,0x9abc0003,0,41)))
  assert t.life(3)==0
  capture('round_award',t.invoke(END[t.name]['tick']))
  capture('after_world',t.invoke('post_world'))
  assert t.record()['lifecycle']==2
  if round_index==2:
   assert t.record()['game_over']==1 and t.record()['game_over_countdown']==210;break
  assert t.record()['game_over']==0 and t.record()['round_countdown']==150
  for n in range(180):
   t.w(C+8,t.u(C+8)+1)
   try:calls=t.invoke('post_world')
   except Exception as error:
    assert expect_map_stop and round_index==0 and n==179
    return dict(team_mode=team,variant=variant,events=events,completion='stopped_original_instruction',
     stop=dict(error=str(error),faults=t.faults,eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',calls=[f'{e:08x}'for e in t.calls]))
   if n in(148,149,178,179):capture(f'wait_{n+1}',calls)
  assert t.record()['lifecycle']==1 and t.record()['index']==round_index+1
  assert t.life(0)==t.life(3)==1
 assert struct.unpack_from('<h',bytes.fromhex(t.record()['bank_hex']),0x12)[0]==3
 assert not expect_map_stop,'Authored reset-map sequence unexpectedly completed; revalidate its declared boundary'
 return dict(team_mode=team,variant=variant,events=events,completion='complete_original_return')

def live_probe(t,cache,unit,initialize_ai=False):
 t.prepare(dict(lifecycle=3,status=1,index=1));smg=next(tag for tag in cache.tags.values()if tag['group']=='weap'and tag['name'].endswith('\\smg\\smg'))
 t.g.mem_write(OBJECT,bytes(0x2000))
 for off in(0xc,0x10,0x14,0xd4):t.w(OBJECT+off,-1)
 matg=next(tag for tag in cache.tags.values()if tag['group']=='matg')
 definitions=[cache.u(p+0xb4)for p in cache.block(matg['offset']+0x138,0xbc)]
 definition=next(d for d in definitions if d!=0xffffffff) if unit else int(smg['datum'],16)
 assert cache.reference(definition)['group']==('bipd'if unit else'weap')
 t.w(OBJECT,definition);t.g.mem_write(OBJECT+0xaa,bytes([0 if unit else 2]));t.g.mem_write(OBJECT+0xaf,b'\xff')
 t.g.mem_write(OH,struct.pack('<HBBII',0x9abc,0,0 if unit else 2,0,OBJECT));t.w(OP+0x38,1)
 if unit:t.w(R+0x2c,0x9abc0000)
 t.w(t.phase_owners['ai_global'],0);initialization=None
 if initialize_ai:
  t.w(t.phase_owners['ai_global'],AI);t.g.mem_write(AI,b'\xa5'*0x374)
  calls=t.invoke('ai_initialize');after=bytes(t.g.mem_read(AI,0x374))
  assert after==b'\xa5\0'+b'\xa5'*0x372
  initialization=dict(whole_original_return=True,calls=calls,after_hex=after.hex())
 t.calls=[];t.faults=[]
 try:
  t.call(t.phase_owners['phase']);completion='complete_original_return';error=None
 except Exception as e:completion='stopped_original_instruction';error=str(e)
 return dict(boundary='Declared object records; construction/allocation not executed',current_unit=unit,
  original_ai_initialization=initialization,definition=cache.reference(definition),completion=completion,error=error,eip=f'{t.g.reg_read(UC_X86_REG_EIP):08x}',faults=t.faults,
  calls=[f'{e:08x}'for e in t.calls])

def compact_raw_records(data):
 """Retain every raw byte once; records refer to SHA-256-keyed hex blobs."""
 blobs={}
 def walk(value,key=''):
  if isinstance(value,dict):return {k:walk(v,k)for k,v in value.items()}
  if isinstance(value,list):return [walk(v)for v in value]
  if isinstance(value,str)and key.endswith('_hex'):
   digest=hashlib.sha256(bytes.fromhex(value)).hexdigest()
   assert digest not in blobs or blobs[digest]==value
   blobs[digest]=value;return dict(raw_blob_sha256=digest)
  return value
 result=walk(data);result['raw_blobs']=blobs;return result

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/round_lifecycle_original.json')
 ap.add_argument('--verify-fixture',type=Path,help='Compare independently executed complete output with this existing fixture')
 a=ap.parse_args()
 if a.verify_fixture:assert a.fixture.resolve()!=a.verify_fixture.resolve(),'Verification must not overwrite its expected fixture'
 a.output.parent.mkdir(parents=True,exist_ok=True);assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');authored=variants();sources={};versions={};listings=[];pairs=[]
 inputs=[]
 for mode in(0,1,4,255):
  for status in(-1,0,1,2,3,32768):
   for rate in(-32768,-1,0,1,30,32767):
    inputs.append(dict(mode=mode,status=status,rate=rate,recording=True,frozen=1,index=32767 if status==3 else 0,
     limit=(-32768,-1,0,1,32768,65536)[len(inputs)%6],seed=len(inputs)))
 # A sparse occupied pool exercises the original flag2 exclusion and slots15/3.
 inputs.append(dict(status=1,rate=30,players=[player(0,17,0xffffffff,2),player(3,17,0xffffffff,0x4001),player(15,17,0xffffffff,1)],seed=999))
 ticks=[]
 for phase in(2,3):
  for mode in(0,1,4):
   for present in(False,True):
    for index in(-32768,-1,0,30,31,32767):
     for count in(-2147483648,-1,0,1,2,2147483647):
      ticks.append(dict(lifecycle=phase,status=phase,index=index,countdown=count,mode=mode,rate=30,engine_present=present))
 for name in OWNERS:
  t=LifecycleTrace(name,cache);sources[name]=t;entries=set();phase_rows=[];tick_rows=[]
  for row in inputs:
   t.prepare(row);before=t.record();calls=t.invoke('phase');after=t.record();entries|={int(e,16)for e in calls}
   assert_phase(before,after,row);phase_rows.append(dict(before=before,after=after,calls=calls))
  for row in ticks:
   t.prepare(row);before=t.record();calls=t.invoke('post_world');after=t.record();entries|={int(e,16)for e in calls}
   if row['mode']==4 or not row['engine_present']:assert before==after
   else:
    expected=i32(row['countdown']-1)
    if expected>0:assert after['round_countdown']==expected and after['lifecycle']==row['lifecycle']
    elif row['lifecycle']==2 and row['index']<31:assert after['lifecycle']==3 and after['index']==i16(row['index']+1)and after['round_countdown']==30
    elif row['lifecycle']==2:assert after['game_over']==1
    else:assert after['lifecycle']==1 and after['start_tick']==12345
   tick_rows.append(dict(before=before,after=after,calls=calls))
  chains=[sequence(t,authored['7_team_reset_map_disabled'if team else'7_reset_map_disabled'],team)for team in(False,True)]
  authored_stops=[sequence(t,authored['7_team'if team else'7'],team,True)for team in(False,True)]
  for chain in chains+authored_stops:
   for event in chain['events']:entries|={int(e,16)for e in event['calls']}
   if 'stop'in chain:entries|={int(e,16)for e in chain['stop']['calls']}
  probes=[live_probe(t,cache,unit,initialized)for initialized in(False,True)for unit in(False,True)]
  for probe in probes:
   entries|={int(e,16)for e in probe['calls']}
   if probe['original_ai_initialization']:entries|={int(e,16)for e in probe['original_ai_initialization']['calls']}
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.phase_owners,phase_rows=phase_rows,tick_rows=tick_rows,
   first_to_three=chains,reset_map_enabled_dependency_stops=authored_stops,live_object_dependency_probes=probes,whole_calls=t.whole_calls,scenario_pointer=f'{t.scenario:08x}',
   netgame_rows=t.u(t.scenario+0x120))
  directory=a.output.parent/'instructions';directory.mkdir(exist_ok=True)
  for e in sorted(entries|{t.phase_owners[k]for k in('phase','post_world','initialize_round','initialize_rows','reset_objects','reset_rows','reset_players','request')}):
   _,ins=literal(t.code,e,t.starts);p=directory/f'{name}-{e:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
 for name in('retail','tu11'):
  for key in('phase_rows','tick_rows'):
   assert [(r['before'],r['after'])for r in versions[name][key]]==[(r['before'],r['after'])for r in versions['u5'][key]],(name,key)
  for left,right in zip(versions[name]['first_to_three'],versions['u5']['first_to_three']):
   assert [(r['event'],r['tick'],r['record'])for r in left['events']]==[(r['event'],r['tick'],r['record'])for r in right['events']],name
 for name in('retail','tu11'):
  old=sources[name];new=sources['u5']
  for key in('phase','post_world','initialize_round','initialize_rows','reset_objects','reset_rows','reset_players','request'):
   _,oi=literal(old.code,old.phase_owners[key],old.starts);_,ni=literal(new.code,new.phase_owners[key],new.starts)
   assert complete_tokens(oi,old.image)==complete_tokens(ni,new.image),(name,key)
   pairs.append(dict(earlier=name,owner=key,complete_shape_equal=True,instructions=len(oi),relocated_operands=references(oi,ni,old.image,new.image)))
 data=dict(schema='original-round-lifecycle-v1',boundary=__doc__,inputs=inputs,tick_inputs=ticks,versions=versions,
  lockout_sha256=CACHE_PIN,mainmenu_sha256=MENU_PIN,instruction_patches=[],supplied_returns=[],whole_calls=sum(t.whole_calls for t in sources.values()))
 data=compact_raw_records(data)
 serialized=json.dumps(data,sort_keys=True,separators=(',',':'))+'\n'
 if a.verify_fixture:assert serialized==a.verify_fixture.read_text(),'Complete independently executed original records differ'
 a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text(serialized)
 report=dict(**data,fixture_sha256=sha(a.fixture),oracle_sha256=sha(__file__),instruction_listings=listings,complete_source_pairs=pairs,
  dependencies={p:sha(ROOT/p)for p in('tools/oracle_round_end.py','tools/oracle_player_lives.py','tools/oracle_result_statistics.py','tools/oracle_game_variants.py','tools/oracle_game_over.py','tools/oracle_mp_full_scoreboard_1_5.py','tools/inspect_multiplayer.py','tools/inspect_multiplayer_hud.py','tools/reference/halo2-decompiled/tools/xbe.py')})
 a.output.write_text(json.dumps(report,sort_keys=True,separators=(',',':'))+'\n')
 print(json.dumps(dict(whole_calls=data['whole_calls'],phase_cases_per_image=len(inputs),post_world_cases_per_image=len(ticks),
  edited_empty_world_first_to_three_sequences=6,reset_map_enabled_dependency_stops=6,complete_source_pairs=len(pairs),fixture_sha256=sha(a.fixture),live_object_probes={n:[p['eip']for p in v['live_object_dependency_probes']]for n,v in versions.items()}),indent=2))
if __name__=='__main__':main()
