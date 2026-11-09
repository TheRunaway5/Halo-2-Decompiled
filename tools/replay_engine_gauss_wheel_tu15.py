#!/usr/bin/env python3
"""Owned ordinary-input Gauss driving/wheel or separate turret regression.

Run under tools/run_test.py. The acceptance executable owns real SDL controls,
native motion/collision and the reached-wheel verdict. No physics answer is
supplied, and an attempted drive is never a successful wheel reproduction.
"""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
from weapon_test_cache import require_mcc_map

ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.file_digest(p.open('rb'),'sha256').hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--mcc-root',type=Path,required=True);p.add_argument('--mode',choices=('drive-wheel','fire-turret'),default='drive-wheel');p.add_argument('--report',type=Path,required=True);p.add_argument('--timeout',type=int,default=420);a=p.parse_args()
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);runtime=a.runtime.resolve(strict=True);cache=a.mcc_root.resolve(strict=True);map_path=cache/'halo2/h2_maps_win64_dx11/zanzibar.map';require_mcc_map(map_path);assert 30<=a.timeout<=900
 a.report=a.report.resolve();a.report.mkdir(parents=True,exist_ok=False)
 env={k:v for k,v in os.environ.items()if not k.startswith('HALO2_')}
 env.update(HALO2_TEST_SCRATCH=str(owner),HALO2_TU15_VEHICLE_NAME='warthog_gauss',HALO2_TU15_VEHICLE_SEAT='0')
 if a.mode=='drive-wheel':
  env.update(HALO2_TU15_VEHICLE_DRIVE_TICKS='1200',HALO2_TU15_VEHICLE_SINGLE_VIEW_INPUT='1',HALO2_TU15_VEHICLE_WHEEL_FULL_DRIVE='1',HALO2_VERIFY_DEPTH_GEOMETRY_RUNS='1')
 else:env.update(HALO2_TU15_VEHICLE_DRIVE_TICKS='180',HALO2_TU15_VEHICLE_CHILD='1',HALO2_TU15_GAUSS_FIRE_INPUT='1')
 command=['xvfb-run','-a','-s','-screen 0 1280x720x24 -nolisten tcp',sys.executable,'-B',str(ROOT/'tools/test_physics_multiplayer_visible_tu15.py'),'--runtime',str(runtime),'--mcc-root',str(cache),'--map',str(map_path),'--output',str(owner/a.mode),'--report-output',str(a.report/'visible'),'--scenario','vehicle','--no-record','--timeout-seconds',str(a.timeout)]
 report=dict(passed=False,scope=__doc__,mode=a.mode,runtime=str(runtime),runtime_sha256=sha(runtime),map_sha256=sha(map_path),helper_sha256=sha(Path(__file__)),visible_wrapper_sha256=sha(ROOT/'tools/test_physics_multiplayer_visible_tu15.py'),command=command,stimulus={k:env[k]for k in env if k.startswith(('HALO2_TU15_VEHICLE_','HALO2_TU15_GAUSS_','HALO2_VERIFY_DEPTH_'))},owned_scratch=str(owner),retained_large_artifacts=[],supplied_physics_outcomes=False)
 log=owner/'runner.log'
 try:
  with log.open('wb')as stream:r=subprocess.run(command,env=env,stdout=stream,stderr=subprocess.STDOUT)
  report['exit_code']=r.returncode;report['runtime_unchanged']=sha(runtime)==report['runtime_sha256']
  case=a.report/'visible/vehicle/report.json';execution=a.report/'visible/vehicle/execution.json'
  if case.exists():
   d=json.loads(case.read_text());report['acceptance']={k:d[k]for k in ('acceptance_passed','error','authority_error','observed_input_ticks','selected_vehicle','selected_seat_controls_vehicle','single_view_vehicle_input','single_view_minimum_wheel_distance','single_view_wheel_receiver_vertices','single_view_mouse_steering_commands','single_view_route_waypoint','location_authority','declared_initial_location')if k in d}
  if execution.exists():report['execution']=json.loads(execution.read_text())
  native_logs=[]
  for path in sorted(p for p in (owner/a.mode).rglob('*.log')if p.name in ('native.log','runtime.log')):
   with path.open('rb')as stream:stream.seek(max(0,path.stat().st_size-8000));tail=stream.read().decode('utf-8','replace')
   native_logs.append(dict(path=str(path.relative_to(owner)),bytes=path.stat().st_size,
       tail=''.join(c if c in '\n\t' or c.isprintable()else'?'for c in tail)))
  report['native_diagnostics']=native_logs
  tail=log.read_text(errors='replace')[-6000:];report['diagnostic_tail']=''.join(c if c in '\n\t' or c.isprintable()else'?'for c in tail)
  report['passed']=r.returncode==0 and report['runtime_unchanged'] and report.get('acceptance',{}).get('acceptance_passed',False)
  if a.mode=='drive-wheel':report['passed']=report['passed']and report['acceptance'].get('single_view_vehicle_input',False)and report['acceptance'].get('single_view_minimum_wheel_distance',float('inf'))<8
 finally:(a.report/'report.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps({k:report.get(k)for k in ('passed','mode','exit_code','acceptance')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
