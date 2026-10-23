#!/usr/bin/env python3
"""Verify the view-bank repair and replay real Banshee/Needler firing."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-glow-material-banks';changes=[]
 p=source/'src/rendering/renderer/renderer.cpp';before=sha(p);s=p.read_text();old='                for(const auto &phase:transparent_views[i])for(const auto &step:phase)collect(step.geometry);';new='''                for(const auto &phase:transparent_views[i])for(const auto &step:phase) {
                    collect(step.geometry);
                    collect(step.lights);
                }'''
 assert s.count(old)==1;s=s.replace(old,new);p.write_text(s);changes.append(dict(path=str(p),before=before,after=sha(p),scope='Only the actual projected light material ranges join per-view uniform uploads'))
 p=source/'tests/projectile_glow_gpu_tests.cpp';before=sha(p);p.write_bytes((ROOT/'tests/projectile_glow_gpu_tests.cpp').read_bytes());changes.append(dict(path=str(p),before=before,after=sha(p)))
 (out/'green-source-refresh.json').write_text(json.dumps(changes,indent=2)+'\n')
 from validate_weapon_runtime_tu15 import private_display
 display_root=owner/'glow-bank-green-display';display_root.mkdir(exist_ok=True)
 with private_display(display_root) as display:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','projectile_glow_gpu_tests','--target','vehicle_weapon_hud_runtime_tests','--target','combat_presentation_audit','--filter','^projectile_glow_material_banks$','--jobs','8','--test-jobs','1','--output',str(out/'green')]
  with (out/'green-command.log').open('w') as log:r=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
  print(json.dumps(dict(phase='glow-bank-green',exit_code=r.returncode)),flush=True)
  if r.returncode:return r.returncode
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map';rows=[]
 for pilot in [0,1]:
  label=f'banshee-view{pilot}-airborne';env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_PILOT_VIEW=str(pilot),HALO2_COMBAT_AUDIT_FLY='1',HALO2_COMBAT_AUDIT_PIXEL_FRAME='114',HALO2_COMBAT_AUDIT_REFERENCE_DIR=str(out/'reference'))
  report=out/(label+'.json')
  with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(owner/'build/vehicle_weapon_hud_runtime_tests'),'vehicle',str(map_),str(report),'banshee'],env=env,stdout=log,stderr=subprocess.STDOUT)
  rows.append(dict(case=label,exit_code=r.returncode,report=str(report)));print(json.dumps(rows[-1]),flush=True)
 env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_REFERENCE_DIR=str(out/'reference'))
 report=out/'needler.json'
 with (out/'needler-command.log').open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(owner/'build/combat_presentation_audit'),'fire',str(map_),str(report),'needler'],env=env,stdout=log,stderr=subprocess.STDOUT)
 rows.append(dict(case='needler',exit_code=r.returncode,report=str(report)));print(json.dumps(rows[-1]),flush=True)
 (out/'cohort.json').write_text(json.dumps(rows,indent=2)+'\n');return int(any(row['exit_code'] for row in rows))
if __name__=='__main__':raise SystemExit(main())
