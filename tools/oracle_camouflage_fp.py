#!/usr/bin/env python3
"""Whole original camouflage consumers with preserved, observed FP controls.

Reuses the declared player/unit/tag/clock inputs of the earlier trait oracle,
but the call boundary preserves the requested FPCW/MXCSR instead of resetting
them. The original FISTP instruction's actual FPCW, binary32 input and produced
integer are observed. Every reached original owner executes unchanged; actor
and replication entity are NONE. No instruction patch or return is supplied.
This is individual-owner evidence, not a complete world tick or rendering.
"""
import argparse,json,struct,sys
from pathlib import Path
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP,UC_X86_REG_EIP,UC_X86_REG_EAX,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_king_trait_consumers import TraitTrace,camo_inputs
from oracle_game_variants import STACK,STOP
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,literal,complete_tokens,references,sha

OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-state-20261011'
class CamoFP(TraitTrace):
 def __init__(self,name,cache):
  self.preserve_fp=False;super().__init__(name,cache);self.preserve_fp=True;self.fp_observations=[];self.pending_fistp=None
  _,ins=literal(self.code,self.owners['camo_refresh'],self.starts)
  rows=[i for i in ins if i.mnemonic=='fistp'];assert len(rows)==1 and rows[0].op_str=='dword ptr [esp + 8]'
  self.fistp=rows[0].address;self.after_fistp=self.fistp+len(rows[0].bytes)
  self.g.hook_add(UC_HOOK_CODE,self.observe_fp)
 def call(self,address,argv=(),registers=()):
  if not self.preserve_fp:return super().call(address,argv,registers)
  self.g.mem_write(STACK,struct.pack('<I',STOP))
  for n,v in enumerate(argv):self.g.mem_write(STACK+4+n*4,struct.pack('<I',v&0xffffffff))
  self.g.reg_write(UC_X86_REG_ESP,STACK)
  for reg,value in registers:self.g.reg_write(reg,value&0xffffffff)
  self.g.emu_start(address,STOP,count=200000)
  assert self.g.reg_read(UC_X86_REG_EIP)==STOP
  value=self.g.reg_read(UC_X86_REG_EAX);return value if value<0x80000000 else value-0x100000000
 def observe_fp(self,g,pc,n,unused):
  if pc==self.fistp:
   sp=g.reg_read(UC_X86_REG_ESP);row=dict(pc=pc,fpcw=g.reg_read(UC_X86_REG_FPCW),mxcsr=g.reg_read(UC_X86_REG_MXCSR),input_bits=self.u(sp+4))
   self.fp_observations.append(row);self.pending_fistp=(row,sp+8)
  elif pc==self.after_fistp and self.pending_fistp:
   row,at=self.pending_fistp;row['produced_integer']=struct.unpack('<i',g.mem_read(at,4))[0];self.pending_fistp=None
 def camo(self,row,operation):
  self.fp_observations=[];self.pending_fistp=None
  result=super().camo(row,operation)
  assert all(q['fpcw']==row.get('cw',0x27f)and q['mxcsr']==row.get('mxcsr',0x1f80)for q in self.fp_observations)
  result['fp_observations']=self.fp_observations.copy();result['returned_fpcw']=self.g.reg_read(UC_X86_REG_FPCW);result['returned_mxcsr']=self.g.reg_read(UC_X86_REG_MXCSR)
  return result
def inputs():
 rows=camo_inputs()
 for cw in (0x27f,0x67f,0xa7f,0xe7f):
  for rate in (-32768,-31,-1,0,1,32767):
   for timer in (0,15):rows.append(dict(operation='refresh',cw=cw,rate=rate,powerup_ticks=timer,attacker_inside=0,hill_flags=0,global_flags=4))
 return rows
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/camouflage_state_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map');rows=inputs();sources={};versions={};listings=[];pairs=[]
 for name in ('retail','tu11','u5'):
  t=CamoFP(name,cache);sources[name]=t;result=[t.camo(row,row['operation'])for row in rows]
  versions[name]=dict(image_sha256=t.spec['pin'],owners=t.owners,damage=[],camouflage=result)
 def normalized(v):return [{k:q for k,q in r.items()if k!='fp_observations'}for r in v['camouflage']]
 assert normalized(versions['retail'])==normalized(versions['tu11'])==normalized(versions['u5'])
 for name,t in sources.items():
  for key in ('camo_refresh','powerup_grant','powerup_tick','unit_camo_tick','camo_activate','camo_deactivate'):
   _,ins=literal(t.code,t.owners[key],t.starts);listings.append(dict(image=name,owner=key,entry=t.owners[key],instructions=[dict(address=i.address,bytes=i.bytes.hex(),mnemonic=i.mnemonic,operands=i.op_str)for i in ins]))
   if name!='u5':
    u=sources['u5'];_,later=literal(u.code,u.owners[key],u.starts);assert complete_tokens(ins,t.image)==complete_tokens(later,u.image)
    pairs.append(dict(earlier=name,later='u5',owner=key,complete_instructions=len(ins),complete_shape_equal=True,relocated_operands=references(ins,later,t.image,u.image)))
 data=dict(schema='original-camouflage-fp-v1',boundary=__doc__,damage_inputs=[],camouflage_inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 prior=ROOT/'tests/fixtures/king_trait_consumers_original.json'
 report=dict(schema='original-camouflage-fp-report-v1',oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
     whole_camouflage_calls=len(rows)*3,actual_fistp_observations=sum(len(q['fp_observations'])for v in versions.values()for q in v['camouflage']),
     complete_source_pairs=pairs,instruction_listings=listings,dependency_sha256=dict(sorted(deps.items())),boundary=__doc__,instruction_patches=[],supplied_returns=[],
     prior_fixture_sha256=sha(prior),prior_harness_limitation='Earlier TraitTrace.setup requested FPCW, but Source.call reset FPCW027F/MXCSR1F80 before execution. Earlier labels do not prove non-nearest rounding. Original fixture remains immutable.')
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('fixture_sha256','whole_camouflage_calls','actual_fistp_observations')},indent=2))
if __name__=='__main__':main()
