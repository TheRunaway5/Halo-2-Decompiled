#!/usr/bin/env python3
"""Select the second genuine local input/view owner in an active owned fixture."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source'
 out=ROOT/'analysis/combat-presentation-audit-20261022/repair-split-pilot';out.mkdir(exist_ok=True)
 p=source/'tests/vehicle_weapon_hud_world_tests.cpp';before=sha(p);s=p.read_text()
 changes=[
 ('std::array<Controls,multiplayer_capacity> controls{};const auto player=world.local_slots[0];',
  'const unsigned pilot_view=std::getenv("HALO2_COMBAT_AUDIT_PILOT_VIEW")?unsigned(std::stoul(std::getenv("HALO2_COMBAT_AUDIT_PILOT_VIEW"))):0u;check(pilot_view<2,"Invalid diagnostic pilot view");if(pilot_view)options.input_instances={-2,-1};row["pilot_view"]=pilot_view;std::array<Controls,multiplayer_capacity> controls{};const auto player=world.local_slots[pilot_view];'),
 ('state.players[world.local_slots[1]].source_vehicle_hud','state.players[world.local_slots[1-pilot_view]].source_vehicle_hud'),
 ('if(view)return;','if(view!=pilot_view)return;'),
 ('options.weapon_light_volume_observer=[&,projectile](unsigned,std::uint32_t,const SourceLightVolumeDraw &draw) {',
  'options.weapon_light_volume_observer=[&,projectile](unsigned view,std::uint32_t,const SourceLightVolumeDraw &draw) {if(view!=pilot_view)return;')]
 if 'const unsigned pilot_view=' not in s:
  for a,b in changes:
   assert a in s,a;s=s.replace(a,b)
  p.write_text(s)
 (out/'source-refresh.json').write_text(json.dumps({'path':str(p),'before':before,'after':sha(p),'scope':'Private declared second-player placement and actual keyboard binding; no fire/physics/render result supplied'},indent=2)+'\n')
 cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','vehicle_weapon_hud_runtime_tests','--target','source_model_build_tests','--filter','^original_model_render_build$','--jobs','8','--test-jobs','1','--output',str(out/'validation')]
 with (out/'command.log').open('w') as log:r=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
 print(json.dumps({'split_pilot_build_exit':r.returncode}),flush=True)
 if r.returncode:return r.returncode
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map';exe=owner/'build/vehicle_weapon_hud_runtime_tests';rows=[]
 for pilot in [1,0]:
  for flight in [False,True]:
   label=f'view{pilot}-'+('airborne' if flight else 'grounded');env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'),HALO2_COMBAT_AUDIT_PILOT_VIEW=str(pilot))
   if flight:env['HALO2_COMBAT_AUDIT_FLY']='1'
   trace=out/(label+'-trace.json')
   with (out/(label+'-trace-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(exe),'vehicle',str(map_),str(trace),'banshee'],env=env,stdout=log,stderr=subprocess.STDOUT)
   if r.returncode:raise RuntimeError(label+' did not complete; see trace log')
   case=json.loads(trace.read_text())['cases'][0];samples=case['projectile_frame_samples'];candidates=[i+1 for i,row in enumerate(samples) if row['owners'] and (not flight or case['flight_samples'][i]['ground_clearance']>3)]
   if not candidates:raise RuntimeError(label+' has no qualified live bolt capture')
   env['HALO2_COMBAT_AUDIT_PIXEL_FRAME']=str(candidates[len(candidates)//2]);env['HALO2_COMBAT_AUDIT_REFERENCE_DIR']=str(out/'reference')
   report=out/(label+'.json')
   with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(exe),'vehicle',str(map_),str(report),'banshee'],env=env,stdout=log,stderr=subprocess.STDOUT)
   rows.append({'case':label,'exit_code':r.returncode,'report':str(report)});(out/'cohort.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows[-1]),flush=True)
 return int(any(r['exit_code'] for r in rows))
if __name__=='__main__':raise SystemExit(main())
