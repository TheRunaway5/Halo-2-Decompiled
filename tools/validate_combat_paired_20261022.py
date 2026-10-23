#!/usr/bin/env python3
"""Refresh diagnostic controls and verify repaired combat with paired frames."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source'
 out=ROOT/'analysis/combat-presentation-audit-20261022/repair-paired-1';out.mkdir(exist_ok=True)
 baseline=owner/'audit-originals/multiplayer_runtime.cpp';before=sha(baseline);s=baseline.read_text()
 needle='for(const auto &model:source_models)if(model.mode!=1) {'
 assert s.count(needle)==1;s=s.replace(needle,'for(const auto &model:source_models)if(model.mode!=1 && source_foreground_models_admitted(view_zoom[i])) {');baseline.write_text(s)
 (out/'source-refresh.json').write_text(json.dumps({'runtime_baseline':{'before':before,'after':sha(baseline)},'scope':'Complete original zoom gate also suppresses the looking-down FP body'},indent=2)+'\n')
 cmd=[sys.executable,str(ROOT/'tools/validate_combat_presentation_audit_20261022.py'),'--refresh-existing','--prepare-only','--partition-ai','--mcc-root',str(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b'),'--output',str(out),'--case','full-sniper-1']
 with (out/'refresh.log').open('w') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
 cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','combat_presentation_audit','--target','vehicle_weapon_hud_runtime_tests','--target','source_model_build_tests','--filter','^original_model_render_build$','--jobs','8','--test-jobs','1','--output',str(out/'validation')]
 with (out/'command.log').open('w') as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
 print(json.dumps({'paired_build_exit':r.returncode}),flush=True)
 if r.returncode:return r.returncode
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map'
 env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'))
 exe=owner/'build/combat_presentation_audit';rows=[]
 def run(label,mode,subject,presses=0,reference=False,env_=None,executable=exe):
  report=out/(label+'.json');e=dict(env if env_ is None else env_)
  if reference:e['HALO2_COMBAT_AUDIT_REFERENCE_DIR']=str(out/'reference')
  command=[sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(executable),mode,str(map_),str(report),subject,str(presses)]
  with (out/(label+'-command.log')).open('w') as log:r=subprocess.run(command,env=e,stdout=log,stderr=subprocess.STDOUT)
  rows.append({'case':label,'exit_code':r.returncode,'report':str(report)})
  (out/'cohort.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows[-1]),flush=True)
 for label,weapon,presses in [('sniper-1','sniper_rifle',1),('sniper-2','sniper_rifle',2),('beam-1','beam_rifle',1),('beam-2','beam_rifle',2),('br-1','battle_rifle',1),('carbine-1','covenant_carbine',1),('rocket-1','rocket_launcher',1),('binoculars','smg',1),('unzoomed','sniper_rifle',0),('sniper-exit','sniper_rifle',3),('beam-exit','beam_rifle',3)]:
  run(label,'zoom',weapon,presses,label=='sniper-1')
 run('needler','fire','needler',reference=True)
 with (out/'projectiles-command.log').open('w') as log:subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(exe),'projectiles',str(map_),str(out/'projectiles.json')],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 for subject in ['frag','plasma']:
  report=out/('grenade-'+subject+'.json')
  with (out/('grenade-'+subject+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(exe),'grenade',str(map_),str(report),subject,'0'],env=env,stdout=log,stderr=subprocess.STDOUT)
  print(json.dumps({'grenade':subject,'exit_code':r.returncode,'report':str(report)}),flush=True)
 vehicle_exe=owner/'build/vehicle_weapon_hud_runtime_tests'
 for label in ['grounded','airborne']:
  e=dict(env)
  if label=='airborne':e['HALO2_COMBAT_AUDIT_FLY']='1'
  report=out/(label+'-trace.json')
  with (out/(label+'-trace-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(vehicle_exe),'vehicle',str(map_),str(report),'banshee'],env=e,stdout=log,stderr=subprocess.STDOUT)
  if r.returncode:raise RuntimeError(label+' trace did not complete')
  case=json.loads(report.read_text())['cases'][0];samples=case['projectile_frame_samples'];candidates=[i+1 for i,row in enumerate(samples) if row['owners']]
  if label=='airborne':
   candidates=[i+1 for i,row in enumerate(samples) if row['owners'] and case['flight_samples'][i]['ground_clearance']>3]
  if not candidates:raise RuntimeError(label+' has no qualified live bolt capture frame')
  e['HALO2_COMBAT_AUDIT_PIXEL_FRAME']=str(candidates[len(candidates)//2]);run('banshee-'+label,'vehicle','banshee',reference=True,env_=e,executable=vehicle_exe)
 return int(any(r['exit_code'] for r in rows))
if __name__=='__main__':raise SystemExit(main())
