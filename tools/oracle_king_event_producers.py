#!/usr/bin/env python3
"""Whole unchanged Xbox King slot6 start-event producer in three images.

Actual registered callbacks, player pool, options, real Lockout matg/mulg and
ordinary no-local-output bindings are declared inputs. Modes0/1/2/3/4/255 reach
the actual submission guard, dispatch, audience and output-user gates without
supplied returns. Mode5's external network owner is deliberately not covered.
No native audio/HUD/network flow or a populated natural activation is proved.
"""
import argparse,itertools,json,struct
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EDI
from oracle_king_scoring import ScoringTrace,player
from oracle_king_world import OWNERS as WORLD
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/king-presentation-20261011'
OWNERS={'retail':0x2bd260,'tu11':0x2c35f0,'u5':0x2c7070}
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'producer-report.json');a=ap.parse_args()
 a.output.parent.mkdir(parents=True,exist_ok=True);assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN
 cache=HudCache(ROOT/'assets/disc/maps/lockout.map');sources={};versions={}
 matg=next(t for t in cache.tags.values()if t['group']=='matg');mulg=cache.tags[cache.u(matg['offset']+0x16c)]
 runtime=cache.block(mulg['offset']+8,0x568)[0];response_rows=[]
 total,size,indices,strings=struct.unpack_from('<4I',cache.data,matg['offset']+0x190)
 english={}
 for i in range(total):
  sid,offset=struct.unpack_from('<II',cache.data,indices+i*8);english[sid]=(strings+offset,cache.text(strings+offset))
 for ordinal,p in enumerate(cache.block(runtime+0xc8,0xa8)):
  sid=cache.u(p+0xc);entry=english.get(sid)
  response_rows.append(dict(ordinal=ordinal,file_offset=p,record_hex=bytes(cache.data[p:p+0xa8]).hex(),
    subtype=cache.h(p+4),audience=cache.h(p+6),caption_key=sid,caption_name=cache.sid(sid),
    english_file_offset=entry[0]if entry else None,english=entry[1]if entry else None,
    sounds=[dict(datum=cache.u(p+0x48+i*8),tag=cache.tags.get(cache.u(p+0x48+i*8)))for i in range(9)]))
 bank_path=a.output.parent/'xbox-lockout-response-bank.json'
 bank_path.write_text(json.dumps(dict(lockout_sha256=CACHE_PIN,oracle_sha256=sha(__file__),
   matg=matg,mulg=mulg,runtime=runtime,rows=response_rows),sort_keys=True,indent=2)+'\n')
 rows=[dict(mode=mode,recipient=recipient)for mode,recipient in itertools.product((0,1,2,3,4,255),(0xffffffff,0x9abc0000,0x9abc0003))]
 for name,entry in OWNERS.items():
  t=ScoringTrace(name,cache);sources[name]=t;events=[];results=[]
  def observe(g,pc,n,unused):
   if pc==WORLD[name]['dispatch']:
    ptr=g.reg_read(UC_X86_REG_EDI);raw=bytes(g.mem_read(ptr,34));events.append(dict(bytes=raw.hex(),words=list(struct.unpack('<8IH',raw))))
  t.g.hook_add(UC_HOOK_CODE,observe)
  for row in rows:
   t.setup_score(dict(mode=row['mode'],team_mode=False,flags=0,players=[player(0),player(3)],team_ticks=[0]*8,rate=30,score=0,team_score=0,team_score_kind=0))
   v=t.u(t.u(t.spec['table']+16));assert t.u(v+0x18)==entry
   events.clear();t.call(entry,(row['recipient'],));actual=list(events)
   expected=[] if row['mode']==4 else [dict(bytes=struct.pack('<8IH',6,0,row['recipient'],0xffffffff,0xffffffff,0xffffffff,0xffffffff,0,0xffff).hex(),words=[6,0,row['recipient'],0xffffffff,0xffffffff,0xffffffff,0xffffffff,0,0xffff])]
   assert actual==expected,(name,row,actual,expected);results.append(dict(input=row,events=actual))
  versions[name]=dict(image_sha256=t.spec['pin'],owner=entry,results=results)
 assert versions['retail']['results']==versions['tu11']['results']==versions['u5']['results']
 pairs=[];listings=[]
 for name,t in sources.items():
  _,ins=literal(t.code,OWNERS[name],t.starts)
  listings.append(dict(image=name,entry=OWNERS[name],instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
  if name!='u5':
   other=sources['u5'];_,later=literal(other.code,OWNERS['u5'],other.starts)
   assert complete_tokens(ins,t.image)==complete_tokens(later,other.image)
   pairs.append(dict(earlier=name,later='u5',complete_instructions=len(ins),complete_shape_equal=True,relocated_operands=references(ins,later,t.image,other.image)))
 report=dict(schema='original-king-start-event-v1',oracle_sha256=sha(__file__),lockout_sha256=CACHE_PIN,whole_callback_calls=len(rows)*3,
   versions=versions,complete_source_pairs=pairs,instruction_listings=listings,response_bank_sha256=sha(bank_path),
   instruction_patches=[],supplied_returns=[],boundary=__doc__)
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps(dict(calls=len(rows)*3,report_sha256=sha(a.output))))
if __name__=='__main__':main()
