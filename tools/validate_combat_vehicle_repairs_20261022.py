#!/usr/bin/env python3
"""Actual local Banshee pilot pixels and natural flight, in the active owner."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source'
 out=ROOT/'analysis/combat-presentation-audit-20261022/repair-banshee-local';out.mkdir(exist_ok=True)
 command=[sys.executable,str(ROOT/'tools/validate_combat_presentation_audit_20261022.py'),'--refresh-existing','--prepare-only','--partition-ai','--mcc-root',str(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b'),'--output',str(out),'--case','vehicle-pixels-banshee']
 with (out/'refresh.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
 command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','vehicle_weapon_hud_runtime_tests','--filter','^no_automatic_case$','--jobs','8','--test-jobs','1','--output',str(out/'validation')]
 # The official worker must name a real registered test. Its first GPU run is
 # the grounded pixel cohort; later runs use the exact same binary with only
 # ordinary flight input enabled.
 command[command.index('^no_automatic_case$')]='^combat_audit_vehicle-pixels-banshee$'
 command+=['--retain-report','combat-audit-vehicle-pixels-banshee.json']
 env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'))
 with (out/'command.log').open('w') as log:r=subprocess.run(command,env=env,stdout=log,stderr=subprocess.STDOUT)
 print(json.dumps({'grounded_ctest_exit':r.returncode,'report':str(out/'validation/report.json')}),flush=True)
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map'
 for label in ['grounded','airborne']:
  report=out/('repair-banshee-'+label+'.json');case_env=dict(env,HALO2_COMBAT_AUDIT_REFERENCE_DIR=str(out/'reference'))
  if label=='airborne':case_env['HALO2_COMBAT_AUDIT_FLY']='1'
  # Select an actual source-published birth frame from the complete grounded
  # pilot run. A cool-down/overheat empty final frame is not a missing-effect
  # verdict. Flight gets its own trace first, then selects its own live frame.
  probe=out/(label+'-trace.json')
  with (out/(label+'-trace-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(owner/'build/vehicle_weapon_hud_runtime_tests'),'vehicle',str(map_),str(probe),'banshee'],env=case_env,stdout=log,stderr=subprocess.STDOUT)
  if r.returncode:raise RuntimeError(label+' trace failed')
  samples=json.loads(probe.read_text())['cases'][0]['projectile_frame_samples']
  candidates=[i+1 for i,row in enumerate(samples) if row['owners']]
  if not candidates:raise RuntimeError(label+' never published a live bolt')
  case_env['HALO2_COMBAT_AUDIT_PIXEL_FRAME']=str(candidates[len(candidates)//2])
  with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(owner/'build/vehicle_weapon_hud_runtime_tests'),'vehicle-visual',str(map_),str(report),'banshee'],env=case_env,stdout=log,stderr=subprocess.STDOUT)
  print(json.dumps({'case':label,'actual_pixel_exit':r.returncode,'report':str(report)}),flush=True)
 return 0
if __name__=='__main__':raise SystemExit(main())
