#!/usr/bin/env python3
"""Refresh only the declared combat repairs in an active disposable snapshot."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();source=owner/'source'
 out=ROOT/'analysis/combat-presentation-audit-20261022/repair-candidate-1';out.mkdir(exist_ok=True)
 files=['include/halo2/native_weapon_projectile_scene.hpp','include/halo2/source_dynamic_object.hpp','include/halo2/multiplayer_datagrams.hpp','include/halo2/source_model_build.hpp','src/rendering/models/source_model_build.cpp','src/gameplay/multiplayer/multiplayer.cpp','src/gameplay/objects/source_dynamic_object.cpp','src/gameplay/objects/source_dynamic_object_scene.cpp','src/gameplay/weapons/native_weapon_projectile_scene.cpp','tests/rocket_ghost_projectile_scene_tests.cpp']
 receipt={}
 for relative in files:
  dst=source/relative;receipt[relative]={'before':sha(dst),'after':sha(ROOT/relative)};dst.write_bytes((ROOT/relative).read_bytes())
 # Keep the coherent snapshot's unrelated shield/camouflage implementation.
 # Apply only the two combat call-site changes to its production baseline.
 p=owner/'audit-originals/multiplayer_runtime.cpp';original=p.read_text()
 saved=owner/'audit-originals/pre-combat-repairs-runtime.cpp';assert not saved.exists();saved.write_text(original)
 s=original.replace('#include "halo2/source_lens_query_frame_tu15.hpp"','#include "halo2/source_lens_query_frame_tu15.hpp"\n#include "halo2/source_model_build.hpp"')
 start=s.index('        const auto owner=source_model_shader_collector(object_owner_spheres[id].center,',s.index('Transform primary_camera_adjustment;'))
 end=s.index('\n      }\n      }\n      if(source_particle_world',start)
 block=s[start:end];s=s[:start]+'        if(source_foreground_models_admitted(view_zoom[i])) {\n'+'\n'.join('  '+line if line and not line.startswith('#') else line for line in block.split('\n'))+'\n        }'+s[end:]
 needle=next(line.strip() for line in s.splitlines() if line.strip().startswith('view.light_volumes=weapon_projectile_scene.light_volume_view('))
 current=(ROOT/'src/gameplay/multiplayer/multiplayer_runtime.cpp').read_text();a=current.index('      std::vector<SourceProjectileGlowOwner> attached_projectile_glows;');b=current.index('\n      view.light_volume_observer=',a)
 replacement=current[a:b].strip();assert replacement.endswith('attached_projectile_glows);')
 s=s.replace(needle,replacement);p.write_text(s)
 receipt['runtime_production_baseline']={'before':sha(saved),'after':sha(p),'unrelated_root_runtime_changes_excluded':True}
 (out/'source-refresh.json').write_text(json.dumps(receipt,indent=2)+'\n')
 command=[sys.executable,str(ROOT/'tools/validate_combat_presentation_audit_20261022.py'),'--refresh-existing','--prepare-only','--partition-ai','--mcc-root',str(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b'),'--output',str(out),'--case','full-sniper-1','--case','vehicle-pixels-banshee']
 with (out/'refresh.log').open('w') as log:subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,check=True)
 command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','combat_presentation_audit','--target','vehicle_weapon_hud_runtime_tests','--target','rocket_ghost_projectile_scene_tests','--target','source_model_build_tests','--filter','native_rocket_ghost_presentation|source_model_build','--jobs','8','--test-jobs','1','--output',str(out/'validation')]
 with (out/'command.log').open('w') as log:r=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
 print(json.dumps({'candidate_component_exit':r.returncode,'report':str(out/'validation/report.json')}),flush=True)
 if r.returncode:return r.returncode
 from mcc_content import directory
 map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map'
 env=dict(os.environ,HALO2_COMBAT_AUDIT_CACHE=str(owner/'graphics-cache'))
 report=out/'needler-fire.json'
 with (out/'fire-command.log').open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(owner/'build/combat_presentation_audit'),'fire',str(map_),str(report),'needler','0'],env=env,stdout=log,stderr=subprocess.STDOUT)
 print(json.dumps({'actual_fire_exit':r.returncode,'report':str(report)}),flush=True)
 for label,weapon,presses in [('sniper-1','sniper_rifle',1),('sniper-2','sniper_rifle',2),('beam-1','beam_rifle',1),('beam-2','beam_rifle',2),('br-1','battle_rifle',1),('carbine-1','covenant_carbine',1),('rocket-1','rocket_launcher',1),('smg-binoculars','smg',1),('sniper-unzoomed','sniper_rifle',0)]:
  report=out/(label+'.json')
  with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_presentation_audit_20261022.py'),str(owner/'build/combat_presentation_audit'),'zoom-full-visual',str(map_),str(report),weapon,str(presses)],env=env,stdout=log,stderr=subprocess.STDOUT)
  print(json.dumps({'case':label,'actual_pixel_exit':r.returncode,'report':str(report)}),flush=True)
 return 0
if __name__=='__main__':raise SystemExit(main())
