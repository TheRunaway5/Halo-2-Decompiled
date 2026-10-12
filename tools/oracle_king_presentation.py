#!/usr/bin/env python3
"""Execute unchanged Vista King category6 local response/audio delivery.

Actual shared.map mulg King rows are loaded at their original source addresses.
The real45DC77/45DBF0 iterator,45DB5E audience precedence,45BFCC row selector,
45D893 guards/captions and45C1E8 language/hash/queue bodies execute unchanged.
Network, caption layout, output-user count, language, CRT and sound handles are
explicit services inherited from the separately documented Vista event oracle.
No caption pixels, SDL/device, transport, Xbox response equivalence or natural
King producer timeline is established by this local event delivery corpus.
"""
import argparse,hashlib,itertools,json,struct
from pathlib import Path
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EDI,UC_X86_REG_ECX
from oracle_vista_announcer_events import Original,CONTEXTS,event,PLAYERS
from oracle_vista_pcr_screen import ROOT,SHA,BASE,u32
from export_vista_announcer_cues import PcMap,SHARED_SHA

OUT=ROOT/'analysis/multiplayer/game-variants/king-presentation-20261011'
SUBTYPES=(0,1,2,3,4,5,6)
NONE=0xffffffff
PLAYER=0x12340000
OTHER=0x12340003

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def king_event(subtype,profile='cause',recipient=NONE):
 e=event((6,subtype),recipient)
 if profile=='cause':e[3]=PLAYER;e[4]=0
 elif profile=='same_team':e[3]=OTHER;e[4]=0
 elif profile=='other_team':e[3]=OTHER;e[4]=7
 elif profile=='effect':e[5]=PLAYER;e[6]=0
 elif profile=='both':e[3]=PLAYER;e[4]=0;e[5]=PLAYER;e[6]=0
 elif profile=='none_team':e[3]=OTHER;e[4]=NONE
 elif profile!='none':raise ValueError(profile)
 return e

class KingOriginal(Original):
 def __init__(self,b,cache):
  self.matches=[];self.attempts=[];self.recipient_bindings=[]
  super().__init__(b,cache)
  tag=cache.one('mulg','multiplayer\\multiplayer_globals')
  runtime=cache.block(tag['file_offset']+8,0x568)[0]
  self.bank=[dict(ordinal=i,file_offset=p,address=p-cache.index,
       record=cache.data[p:p+0xa8].hex(),subtype=cache.u16(p+4),audience=cache.u16(p+6),
       caption_key=cache.u32(p+0xc),display_key=cache.u32(p+0x14),
       flags=cache.u16(p+0x40),weighted_count=cache.u32(p+0xa0),
       sounds=[dict(datum=cache.u32(p+0x48+j*8),
           name=cache.tags.get(cache.u32(p+0x48+j*8),{}).get('name'))for j in range(9)])
       for i,p in enumerate(cache.block(runtime+0xc8,0xa8))]
  self.responses['king']=[x['record'] for x in self.bank]
 def hook(self,u,pc,n,user):
  s=u.reg_read(UC_X86_REG_ESP)
  if pc==0x45db5e:
   datum=u.reg_read(UC_X86_REG_ECX);p=PLAYERS+(datum&0xffff)*0x204
   raw=bytes(u.mem_read(p,0x204))
   self.recipient_bindings.append(dict(datum=datum,slot=datum&0xffff,
      flags=struct.unpack_from('<H',raw,2)[0],membership=struct.unpack_from('<b',raw,0xbc)[0],
      output_user=struct.unpack_from('<h',raw,0x24)[0],record_hex=raw.hex(),
      declared_binding='configured_slot0'if(datum&0xffff)==0 else 'zero_backed_unused_slot'))
  if pc==0x45bfcc:self.attempts.append(dict(player=u32(u,s+4),audience=u32(u,s+8)))
  if pc==0x45d893:
   p=u.reg_read(UC_X86_REG_EDI);raw=bytes(u.mem_read(p,0xa8)).hex()
   matching=[x for x in self.bank if x['record']==raw]
   self.matches.append(dict(player=u32(u,s+4),response_ordinal=matching[0]['ordinal'] if len(matching)==1 else None,
       record=raw))
  super().hook(u,pc,n,user)
 def run(self,q,kind='event'):
  self.matches=[];self.attempts=[];self.recipient_bindings=[];r=super().run(q,kind)
  r.update(matches=self.matches,attempts=self.attempts,recipient_bindings=self.recipient_bindings)
  return r

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--output',type=Path,default=OUT/'report.json')
 ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/king_presentation_original.json')
 a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 exe=ROOT/'Halo 2 Vista installer/halo2.exe';shared=ROOT/'Halo 2 Vista installer/maps/shared.map'
 assert sha(exe)==SHA and sha(shared)==SHARED_SHA
 b=exe.read_bytes();cache=PcMap(shared);o=KingOriginal(b,cache);rows=[]
 assert len(o.bank)==8 and all(x['weighted_count']==0 and x['flags']==1 for x in o.bank)
 assert struct.unpack_from('<I',b,0x45c198-BASE+6*4)[0]==0x45c092
 def isolated(q,cw=0x27f,mx=0x1f80,rate=30):
  o.reset(rate,cw,mx);r=o.run(q);r.update(cw=cw,mxcsr=mx,rate=rate,reset=True);rows.append(r)
 # All authored languages, all source subtypes, exact audience precedence.
 for subtype,profile,language in itertools.product(SUBTYPES,
     ('none','cause','same_team','other_team','effect','both','none_team'),range(9)):
  isolated(dict(event=king_event(subtype,profile),language=language,member=-1 if profile=='none_team' else 0))
 # Full original mode/game-over/local-player/user/explicit-recipient guards.
 for subtype,mode,over,flags,user,recipient in itertools.product(SUBTYPES,
     (0,1,2,3,4,5,255),(0,1),(1,2),(-1,0),(NONE,PLAYER,OTHER)):
  isolated(dict(event=king_event(subtype,'cause',recipient),mode=mode,over=over,flags=flags,user=user))
 # Team audiences and negative NONE membership use actual signed-byte lookup.
 for subtype,member,profile in itertools.product(SUBTYPES,(-128,-1,0,1,7,127),
     ('cause','same_team','other_team','effect','both','none_team')):
  isolated(dict(event=king_event(subtype,profile),member=member))
 # Original suppression boundary and shared queue state, including languageNONE.
 for cw,mx in CONTEXTS:
  for subtype,rate,language in itertools.product((0,1,2,4,5,6),(30,60),(0,8)):
   o.reset(rate,cw,mx)
   for tick in (0,1,26,27,28,53,54):
    r=o.run(dict(event=king_event(subtype,'cause'),tick=tick,language=language))
    r.update(cw=cw,mxcsr=mx,rate=rate,reset=tick==0);rows.append(r)
 # Real queue progression through source start/moved/control/contest/game-over.
 for cw,mx in CONTEXTS:
  o.reset(30,cw,mx)
  operations=[('event',dict(event=king_event(0,'none'),tick=0))]
  operations += [('update',dict(tick=t))for t in range(1,70)]
  operations += [('complete',dict(tick=70,voice=0x12340001)),('event',dict(event=king_event(4,'none'),tick=71)),
      ('update',dict(tick=72)),('event',dict(event=king_event(5,'cause'),tick=73)),
      ('event',dict(event=king_event(6,'same_team'),tick=74)),
      ('event',dict(event=event((0,19)),tick=75,over=1)),('update',dict(tick=76))]
  for index,(kind,q) in enumerate(operations):
   r=o.run(q,kind);r.update(cw=cw,mxcsr=mx,rate=30,reset=index==0);rows.append(r)
 data=dict(schema='original-vista-king-presentation-v1',responses=o.responses,bank=o.bank,cases=rows,boundary=__doc__)
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 cs=Cs(CS_ARCH_X86,CS_MODE_32);assembly=[]
 for pc in sorted(o.executed):
  if BASE<=pc<BASE+len(b):
   ins=next(cs.disasm(b[pc-BASE:pc-BASE+15],pc));assembly.append(f'{pc:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}')
 asm=a.output.parent/'executed-original.asm';asm.write_text('\n'.join(assembly)+'\n')
 deps=('oracle_vista_announcer_events.py','oracle_vista_pcr_screen.py','export_vista_announcer_cues.py')
 report=dict(schema='original-vista-king-presentation-report-v1',image_sha256=SHA,shared_sha256=SHARED_SHA,
   oracle_sha256=sha(__file__),dependencies={p:sha(ROOT/'tools'/p)for p in deps},fixture_sha256=sha(a.fixture),
   instruction_listing_sha256=sha(asm),whole_delivery_and_queue_calls=len(rows),instructions=o.visits,
   response_matches=sum(len(r['matches'])for r in rows),matched_ordinals=sorted(set(x['response_ordinal'] for r in rows for x in r['matches'] if x['response_ordinal'] is not None)),
   bank=o.bank,instruction_patches=[],boundary=__doc__,supplied_services=Original.__module__)
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');cache.close()
 print(json.dumps({k:v for k,v in report.items()if k not in ('bank','boundary')},indent=2))
if __name__=='__main__':main()
