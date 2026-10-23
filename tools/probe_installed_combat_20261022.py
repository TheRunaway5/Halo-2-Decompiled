#!/usr/bin/env python3
"""Read-only selected-runtime Coagulation startup and projectile resource probe."""
import hashlib,json,os,subprocess
from pathlib import Path
from validate_weapon_runtime_tu15 import private_display
from run_combat_presentation_audit_20261022 import keep_log
ROOT=Path(__file__).resolve().parents[1]
def main():
 scratch=Path(os.environ['HALO2_TEST_SCRATCH']);output=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-startup';output.mkdir(exist_ok=True)
 selection=ROOT/'gameplay-runtime-selection.json';before=selection.read_bytes();record=json.loads(before);exe=ROOT/record['runtime']
 assert hashlib.sha256(exe.read_bytes()).hexdigest()==record['runtime_sha256']
 from mcc_content import directory
 maps=directory(ROOT/record['cache'])/'h2_maps_win64_dx11'
 results=[]
 with private_display(scratch) as display:
  for players in [1,2]:
   command=[str(exe),'--multiplayer','--map',str(maps/'coagulation.map'),'--shared-maps',str(maps),'--local-players',str(players),'--frames','4','--mute','--viewport','640','480']
   log=scratch/(str(players)+'.log');env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(scratch/'cache'))
   with log.open('w') as stream:
    try:code=subprocess.run(command,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=360).returncode
    except subprocess.TimeoutExpired:code=124
   keep_log(log,output/(str(players)+'.log'));lines=log.read_text(errors='replace').splitlines()
   signals=[line for line in lines if any(word in line for word in ['projectile','glow','Multiplayer frames=','Source lens queries:'])]
   results.append({'local_players':players,'exit_code':code,'signals':signals,'command':command});print(json.dumps(results[-1]),flush=True)
 assert selection.read_bytes()==before
 (output/'report.json').write_text(json.dumps({'runtime':str(exe),'runtime_sha256':record['runtime_sha256'],'selection_unchanged':True,'cases':results},indent=2)+'\n')
 return int(any(r['exit_code'] for r in results))
if __name__=='__main__':raise SystemExit(main())
