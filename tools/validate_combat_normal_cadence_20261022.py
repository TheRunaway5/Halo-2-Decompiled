#!/usr/bin/env python3
"""Exercise Banshee publication under the ordinary wall-clock game loop."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-normal-cadence';out.mkdir(exist_ok=True)
 p=source/'tests/vehicle_weapon_hud_world_tests.cpp';before=sha(p);s=p.read_text()
 a='options.reference_tick_cadence=true;options.reference_render_fraction=.5f;'
 b='if(!std::getenv("HALO2_COMBAT_AUDIT_NORMAL_CADENCE")){options.reference_tick_cadence=true;options.reference_render_fraction=.5f;}if(const auto *width=std::getenv("HALO2_COMBAT_AUDIT_WIDTH")){options.width=unsigned(std::stoul(width));options.height=options.width*9/16;}'
 if a in s:s=s.replace(a,b);p.write_text(s)
 (out/'source-refresh.json').write_text(json.dumps({'path':str(p),'before':before,'after':sha(p),'scope':'Ordinary production wall-clock accumulator and render interpolation, actual SDL input, two local views'},indent=2)+'\n')
 cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','vehicle_weapon_hud_runtime_tests','--target','source_model_build_tests','--filter','^original_model_render_build$','--jobs','8','--test-jobs','1','--output',str(out/'validation')]
 with (out/'command.log').open('w') as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
 if r.returncode:return r.returncode
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map';exe=owner/'build/vehicle_weapon_hud_runtime_tests';rows=[]
 for pilot,width,flight in [(0,800,False),(0,1280,True),(0,1920,True),(1,1280,True)]:
  label=f'view{pilot}-{width}-'+('airborne' if flight else 'grounded');env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_PILOT_VIEW=str(pilot),HALO2_COMBAT_AUDIT_NORMAL_CADENCE='1',HALO2_COMBAT_AUDIT_WIDTH=str(width))
  if flight:env['HALO2_COMBAT_AUDIT_FLY']='1'
  report=out/(label+'.json')
  with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(exe),'vehicle',str(map_),str(report),'banshee'],env=env,stdout=log,stderr=subprocess.STDOUT)
  row={'case':label,'exit_code':r.returncode,'report':str(report)}
  if report.exists():
   data=json.loads(report.read_text());case=data.get('cases',[{}])[0]
   samples=case.get('projectile_frame_samples',[]);ticks=[s['tick'] for s in samples]
   row.update(render_frames=len(ticks),source_ticks=ticks[-1]-ticks[0] if ticks else 0,live_bolt_frames=sum(bool(s['owners']) for s in samples),projectile_render=case.get('projectile_render'),source_births=len(case.get('source_projectile_trace',{}).get('constructor_calls',[])))
  rows.append(row);(out/'cohort.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(row),flush=True)
 return int(any(row['exit_code'] for row in rows))
if __name__=='__main__':raise SystemExit(main())
