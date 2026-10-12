#!/usr/bin/env python3
"""Whole original D2310 intrinsic-zero camouflage disruption in three images.

Declared backed actorless/unit records, flags, binary32 primary/speed/amount/
limit and replication entity NONE are explicit ABI inputs. Every reached game
instruction executes unchanged. Main cases use finite positive-zero/0..1 inputs
plus a finite negative primary attainable after an inactive disruption. Authored
Lockout weapon/jpt fields are recorded separately. Intrinsic nonzero delegates
to its actual original controller and is not claimed by this branch corpus.
No instruction patch, supplied return or whole firing/damage/world proof.
"""
import argparse,itertools,json,re,struct,sys
from pathlib import Path
from oracle_camouflage_fp import CamoFP
from oracle_king_trait_consumers import H,T,U,NONE
from oracle_result_statistics import R
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha
from unicorn.x86_const import UC_X86_REG_EAX
OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-state-20261011/disruption'
def bits(value):return struct.unpack('<I',struct.pack('<f',value))[0]
class Disruption(CamoFP):
 def __init__(self,name,cache):
  super().__init__(name,cache);section=next(s for s in self.image.sections if s.name=='.text')
  pattern=rb'\x53\x8b\xc8\x81\xe1\xff\xff\x00\x00\x56\x8b\x35....\x8b\x56\x44\x8d\x0c\x49\xc1\xe1\x02'
  matches=[section.va+m.start()for m in re.finditer(pattern,self.image.section_bytes(section),re.S)]
  assert len(matches)==1;self.d={'disruption':matches[0]}
  _,ins=literal(self.code,matches[0],self.starts);calls=[int(i.op_str,16)for i in ins if i.mnemonic=='call'];assert len(calls)==1;self.d['intrinsic_delegate']=calls[0]
 def run(self,row):
  self.setup(dict(unit_flags=row['flags']));self.w(U+0x13c,0x9abc0000);self.g.mem_write(U+0x2bc,bytes(4))
  self.w(U+0x2b0,row['primary_bits']);self.w(U+0x2b8,row['speed_bits']);unit=bytes(self.g.mem_read(U,0x800));players=bytes(self.g.mem_read(R,16*0x21c))
  self.call(self.d['disruption'],(row['amount_bits'],row['limit_bits']),((UC_X86_REG_EAX,0x8aaa0000),))
  actual=bytearray(self.g.mem_read(U,0x800))
  for off in (0x2b0,0x2b8):actual[off:off+4]=unit[off:off+4]
  assert bytes(actual)==unit and bytes(self.g.mem_read(R,16*0x21c))==players
  return dict(primary_bits=self.u(U+0x2b0),speed_bits=self.u(U+0x2b8),flags=self.u(U+0x134),
      secondary_bits=self.u(U+0x2b4),intrinsic_mode=self.g.mem_read(U+0x2bc,1)[0],intrinsic_ticks=struct.unpack('<h',self.g.mem_read(U+0x2be,2))[0])
def inputs():
 return [dict(primary_bits=bits(primary),speed_bits=bits(speed),flags=flags,amount_bits=bits(amount),limit_bits=bits(limit),intrinsic_mode=0)
     for primary,speed,flags,amount,limit in itertools.product((-.1,0,.049999997,.05,.050000004,.1,.5,1),(0,.1,.25,1),(0x1020,0x1028,0x1038),(0,.05,.1,.2,.75,1),(0,.1,.25,1))]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/camouflage_disruption_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map');rows=inputs();sources={};versions={};listings=[];pairs=[];tags=[]
 for name in ('retail','tu11','u5'):
  t=Disruption(name,cache);sources[name]=t;versions[name]=dict(image_sha256=t.spec['pin'],owners=t.d,results=[t.run(q)for q in rows])
 assert versions['retail']['results']==versions['tu11']['results']==versions['u5']['results']
 for name,t in sources.items():
  for key,entry in t.d.items():
   _,ins=literal(t.code,entry,t.starts);listings.append(dict(image=name,owner=key,entry=entry,instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
   if name!='u5':
    u=sources['u5'];_,later=literal(u.code,u.d[key],u.starts);assert complete_tokens(ins,t.image)==complete_tokens(later,u.image)
    pairs.append(dict(earlier=name,later='u5',owner=key,complete_instructions=len(ins),complete_shape_equal=True,relocated_operands=references(ins,later,t.image,u.image)))
 for tag in cache.tags.values():
  if tag['group']in ('weap','jpt!'):
   offset=0x27c if tag['group']=='weap'else 0x30;size=8 if tag['group']=='weap'else 4;raw=bytes(cache.data[tag['offset']+offset:tag['offset']+offset+size])
   tags.append(dict(**tag,field_offset=offset,raw_hex=raw.hex(),amount_bits=struct.unpack_from('<I',raw)[0],limit_bits=struct.unpack_from('<I',raw,4)[0]if size==8 else 0))
 data=dict(schema='original-camouflage-disruption-v1',boundary=__doc__,inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 report=dict(schema='original-camouflage-disruption-report-v1',oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
     whole_disruption_calls=len(rows)*3,complete_source_pairs=pairs,instruction_listings=listings,authored_tag_fields=tags,dependency_sha256=dict(sorted(deps.items())),
     boundary=__doc__,instruction_patches=[],supplied_returns=[],excluded_inputs='NaN/infinity, negative amount/limit/speed, unbacked invalid unit, actual replicated entity dirty service, intrinsic nonzero activation/deactivation and complete callers are not supplied successful results.')
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('fixture_sha256','whole_disruption_calls')},indent=2))
if __name__=='__main__':main()
