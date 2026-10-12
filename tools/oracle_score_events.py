#!/usr/bin/env python3
"""Whole unchanged score-change event producers with real ordinary dispatch.

Actual Xbox score-limit/options/player pools, selected-engine state and Lockout
matg/mulg response tags are declared inputs. Modes0/1/2/3/4/255 execute real
network guards and local dispatch with original nonlocal-user early returns.
Mode5 external transport and live HUD/audio consumers are outside this corpus.
No original instructions, service returns or native expected outputs are used.
"""
import argparse,hashlib,itertools,json,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EDI
from oracle_king_scoring import ScoringTrace,player,G,R,ROOT
from oracle_mp_full_scoreboard_1_5 import HudCache,CACHE_PIN,literal,complete_tokens,references,sha
OUT=ROOT/'analysis/multiplayer/game-variants/score-events-20261012'
OWNERS={'retail':dict(owner=0x19f470,network=0xa7c50,local=0x19eb30),
 'tu11':dict(owner=0x1a2dd0,network=0xa8a30,local=0x1a2490),
 'u5':dict(owner=0x1a9080,network=0xb1e80,local=0x1a8740)}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'complete/report.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=False);cache=HudCache(ROOT/'assets/disc/maps/lockout.map');assert sha(cache.path)==CACHE_PIN
 rows=[]
 for mode,present,team,membership,slot,remaining in itertools.product((0,1,2,3,4,255),(False,True),(False,True),(-128,-1,0,7,127),(0,15),(0,9,10,11,29,30,31,59,60,61,0x80000000,0xffffffff)):
  # Vary raw updated scores without making a native balancing assumption.
  score=(0,0xffffffff,0x7fffffff,0x80000000)[len(rows)%4]
  rows.append(dict(mode=mode,engine_present=present,team_mode=team,membership=membership,slot=slot,
   player=0x9abc0000|slot,score=score,limit=(score+remaining)&0xffffffff))
 versions={};traces={};listings=[];pairs=[]
 for name,owners in OWNERS.items():
  t=ScoringTrace(name,cache);traces[name]=t;events=[];results=[]
  def observe(g,address,size,user):
   if address not in (owners['network'],owners['local']):return
   pointer=g.reg_read(UC_X86_REG_EDX if address==owners['network']else UC_X86_REG_EDI)
   events.append(dict(kind='network'if address==owners['network']else'local',event34_hex=bytes(g.mem_read(pointer,34)).hex()))
  t.g.hook_add(UC_HOOK_CODE,observe)
  for n,row in enumerate(rows):
   t.setup_score(dict(mode=row['mode'],team_mode=row['team_mode'],engine_present=row['engine_present'],flags=0,
    players=[player(row['slot'],membership=row['membership'])],team_ticks=[0]*8,rate=30,score=0,team_score=0,team_score_kind=0))
   t.w(G+0x18c,row['limit']);before=bytes(t.g.mem_read(R,16*0x21c));options=bytes(t.g.mem_read(G,0x240));events.clear()
   try:t.call(owners['owner'],(row['score'],),((UC_X86_REG_EAX,row['player']),))
   except Exception:print(name,n,json.dumps(row),flush=True);raise
   assert bytes(t.g.mem_read(R,16*0x21c))==before and bytes(t.g.mem_read(G,0x240))==options
   results.append(list(events))
  executed=hashlib.sha256()
  for address in sorted(t.executed):
   instruction=t.code.at(address);raw=bytes(t.g.mem_read(address,instruction.size));assert raw==bytes(instruction.bytes)
   executed.update(struct.pack('<I',address)+raw)
  versions[name]=dict(image_path=t.spec['path'],image_sha256=t.spec['pin'],owners=owners,results=results,observed_instructions=len(t.executed),executed_instruction_sha256=executed.hexdigest())
  for key,entry in owners.items():
   _,ins=literal(t.code,entry,t.starts);p=a.output.parent/f'{name}-{key}-{entry:08x}.asm'
   p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in ins));listings.append(dict(file=p.name,sha256=sha(p),instructions=len(ins)))
 for name in ('tu11','u5'):
  assert versions[name]['results']==versions['retail']['results']
  old,new=traces['retail'],traces[name];_,oi=literal(old.code,OWNERS['retail']['owner'],old.starts);_,ni=literal(new.code,OWNERS[name]['owner'],new.starts)
  assert complete_tokens(oi,old.image)==complete_tokens(ni,new.image)
  pairs.append(dict(image=name,instructions=len(oi),complete_shape_equal=True,relocated_operands=references(oi,ni,old.image,new.image)))
 dependencies={str(Path(m.__file__).resolve().relative_to(ROOT)):sha(m.__file__)for m in tuple(sys.modules.values())if getattr(m,'__file__',None)
  and str(Path(m.__file__).resolve()).startswith(str(ROOT/'tools')+'/')and str(m.__file__).endswith('.py')and Path(m.__file__).resolve()!=Path(__file__).resolve()}
 report=dict(schema='original-score-events-v1',scope=__doc__,tool_sha256=sha(__file__),dependencies=dependencies,lockout_sha256=CACHE_PIN,
  inputs=rows,versions=versions,listings=listings,complete_pairs=pairs,instruction_patches=[],supplied_returns=[],
  summary=dict(whole_calls=len(rows)*3,network_publications=sum(sum(e['kind']=='network'for e in r)for v in versions.values()for r in v['results']),
   local_publications=sum(sum(e['kind']=='local'for e in r)for v in versions.values()for r in v['results'])))
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(report['summary']),flush=True)
if __name__=='__main__':main()
