#!/usr/bin/env python3
"""Declared composition of whole original camouflage owners in three XBEs.

Cold unit camouflage fields and two player timers are explicit zero inputs,
with backed current unit, actor/entity NONE, actual registered King engine,
no global/hill trait, 30 Hz clock and observed source-frame nearest control.
An explicit grant is followed by refresh, unit fade, player timer in original
wrapper order for 30 declared steps. Every reached game instruction executes
unchanged, without supplied returns. This is not a whole original world tick,
authored powerup pickup, natural clock admission, renderer or normal play proof.
"""
import argparse,json,struct,sys
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EDX,UC_X86_REG_EDI,UC_X86_REG_ESI,UC_X86_REG_FPCW,UC_X86_REG_MXCSR
from oracle_camouflage_fp import CamoFP
from oracle_king_trait_consumers import U,NONE
from oracle_mp_full_scoreboard_1_5 import ROOT,HudCache,CACHE_PIN,sha

OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-state-20261011/timeline'
def run(t,row):
 t.setup(dict(camo=0,secondary_camo=0,fade_speed=0,unit_flags=0,powerup_ticks=0,secondary_ticks=0,
     global_flags=0,hill_flags=0,attacker_inside=0,rate=30,dt=1/30,cw=0x23f))
 t.w(U+0x13c,0x9abc0000);t.g.mem_write(U+0x2bc,bytes(4))
 cold=t.state();t.call(t.owners['powerup_grant'],registers=((UC_X86_REG_EAX,0x9abc0000),(UC_X86_REG_EDX,row['index']),(UC_X86_REG_EDI,row['ticks'])))
 grant=t.state();frames=[]
 for frame in range(1,row['steps']+1):
  t.queries=[];t.fp_observations=[]
  t.call(t.owners['camo_refresh'],registers=((UC_X86_REG_ESI,0x9abc0000),));refresh=t.state()
  assert t.queries==[[0x9abc0000,1]] and not t.fp_observations
  t.call(t.owners['unit_camo_tick'],registers=((UC_X86_REG_EAX,0x8aaa0000),));fade=t.state()
  t.call(t.owners['powerup_tick'],registers=((UC_X86_REG_EAX,0x9abc0000),));timer=t.state()
  frames.append(dict(step=frame,after_refresh=refresh,after_fade=fade,after_timer=timer,queries=t.queries.copy(),
      fpcw=t.g.reg_read(UC_X86_REG_FPCW),mxcsr=t.g.reg_read(UC_X86_REG_MXCSR)))
 return dict(cold=cold,after_grant=grant,steps=frames)
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'report.json');ap.add_argument('--fixture',type=Path,default=ROOT/'tests/fixtures/camouflage_timeline_original.json');a=ap.parse_args();a.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(ROOT/'assets/disc/maps/lockout.map')==CACHE_PIN;cache=HudCache(ROOT/'assets/disc/maps/lockout.map')
 rows=[dict(index=0,ticks=15,steps=30),dict(index=1,ticks=2,steps=30)];versions={}
 for name in ('retail','tu11','u5'):
  t=CamoFP(name,cache);versions[name]=dict(image_sha256=t.spec['pin'],owners=t.owners,results=[run(t,row)for row in rows])
 assert versions['retail']['results']==versions['tu11']['results']==versions['u5']['results']
 data=dict(schema='original-camouflage-timeline-v1',boundary=__doc__,inputs=rows,versions=versions,instruction_patches=[],supplied_returns=[])
 a.fixture.write_text(json.dumps(data,sort_keys=True,separators=(',',':'))+'\n')
 deps={}
 for mod in list(sys.modules.values()):
  path=getattr(mod,'__file__',None)
  if path:
   p=Path(path).resolve()
   if p.is_relative_to(ROOT/'tools')and p.suffix=='.py':deps[str(p.relative_to(ROOT))]=sha(p)
 report=dict(schema='original-camouflage-timeline-report-v1',oracle_sha256=sha(__file__),fixture_sha256=sha(a.fixture),lockout_sha256=CACHE_PIN,
     whole_principal_owner_calls=3*sum(1+3*q['steps']for q in rows),dependency_sha256=dict(sorted(deps.items())),
     boundary=__doc__,instruction_patches=[],supplied_returns=[])
 a.output.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('fixture_sha256','whole_principal_owner_calls')},indent=2))
if __name__=='__main__':main()
