#!/usr/bin/env python3
"""Resume the scoped attachment candidate after the rendered red control."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def write_changed(p,data):
 data=data.encode() if isinstance(data,str) else data
 p.parent.mkdir(parents=True,exist_ok=True)
 if not p.exists() or p.read_bytes()!=data:p.write_bytes(data)
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-attachment-material';changes=[]
 p=source/'src/gameplay/multiplayer/multiplayer.cpp';s=p.read_text();before=sha(p);s=s.replace('Bytes(bank.view(body+0xaau,1)).u8(0)','std::to_integer<std::uint8_t>(bank.view(body+0xaau,1)[0])');write_changed(p,s);changes.append(dict(path=str(p),before=before,after=sha(p)))
 p=source/'tools/audit_combat_presentation_20261022.cpp';before=sha(p);write_changed(p,(ROOT/'tools/audit_combat_presentation_20261022.cpp').read_bytes());changes.append(dict(path=str(p),before=before,after=sha(p)))
 p=source/'include/halo2/multiplayer_datagrams.hpp';before=sha(p);write_changed(p,(ROOT/'include/halo2/multiplayer_datagrams.hpp').read_bytes());changes.append(dict(path=str(p),before=before,after=sha(p)))
 p=source/'src/rendering/renderer/renderer.cpp';before=sha(p);s=p.read_text();s=s.replace('for(const auto &draw:light_views[i]) {\n                    material_pipeline(draw.material,&draw);','for(const auto &draw:light_views[i]) {\n                    material_pipeline(draw.material);').replace('const auto& draw=pstencil_specular_views[i].at(receiver);material_pipeline(draw.material,&draw);','const auto& draw=pstencil_specular_views[i].at(receiver);material_pipeline(draw.material);');write_changed(p,s);changes.append(dict(path=str(p),before=before,after=sha(p)))
 oracle_python=owner/'oracle-python/bin/python'
 if not oracle_python.exists():
  import venv
  venv.EnvBuilder(with_pip=True).create(oracle_python.parent.parent)
 with (out/'oracle-dependencies.log').open('w') as log:
  subprocess.run([str(oracle_python),'-m','pip','install','--quiet','--disable-pip-version-check','capstone>=5,<6','unicorn>=2.1,<3'],env=dict(os.environ,PIP_CACHE_DIR=str(owner/'oracle-package-cache')),stdout=log,stderr=subprocess.STDOUT,check=True)
 versions=subprocess.check_output([str(oracle_python),'-c','import capstone,unicorn,json;print(json.dumps(dict(capstone=capstone.__version__,unicorn=unicorn.__version__)))'],text=True)
 (out/'oracle-dependencies.json').write_text(versions)
 references=owner/'attachment-original-references';references.mkdir(exist_ok=True)
 pass_report=references/'pass/report.json';binder_report=references/'binder/binder-report.json'
 if not pass_report.exists():
  with (out/'original-pass-command.log').open('w') as log:subprocess.run([str(oracle_python),str(ROOT/'tools/inspect_camouflage_pass_contract.py'),'--output',str(pass_report.parent)],stdout=log,stderr=subprocess.STDOUT,check=True)
 if not binder_report.exists():
  with (out/'original-state-command.log').open('w') as log:subprocess.run([str(oracle_python),str(ROOT/'tools/oracle_camouflage_binder_boundaries.py'),'--output',str(binder_report.parent),'--prior',str(pass_report),'--prior-sha256',sha(pass_report)],stdout=log,stderr=subprocess.STDOUT,check=True)
 for relative,reference in [('analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json',binder_report),('analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json',pass_report)]:
  p=source/relative;write_changed(p,reference.read_bytes());changes.append(dict(path=relative,reference_input=True,sha256=sha(p),bytes=p.stat().st_size))
 (out/'source-refresh-2.json').write_text(json.dumps(changes,indent=2)+'\n')
 from validate_weapon_runtime_tu15 import private_display
 display_root=owner/'attachment-material-display-2';display_root.mkdir(exist_ok=True)
 with private_display(display_root) as display:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','camouflage_material_gpu_tests','--target','camouflage_material_tests','--target','camouflage_state_material_tests','--target','combat_presentation_audit','--filter','^camouflage_object_owner_isolation$|^original_camouflage_material_animation$|^original_camouflage_state_material$','--jobs','8','--test-jobs','1','--output',str(out/'green-6')]
  with (out/'green-6-command.log').open('w') as log:r=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
  print(json.dumps(dict(phase='green-6',exit_code=r.returncode)),flush=True)
  if r.returncode:return r.returncode
  from mcc_content import directory
  map_=directory(ROOT/'assets/imported/mcc-056309e12f444a3a8992c94f6fe32f0b')/'h2_maps_win64_dx11/coagulation.map';exe=owner/'build/combat_presentation_audit'
  with (out/'banshee-setup-command.log').open('w') as log:subprocess.run([str(exe),'banshee-setup',str(map_),str(out/'banshee-setup.json')],env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
  env['HALO2_COMBAT_AUDIT_CACHE']=str(owner/'graphics-cache');env['HALO2_COMBAT_AUDIT_REFERENCE_DIR']=str(out/'reference');rows=[]
  for camouflage in [False,True]:
   label='needler-camouflaged' if camouflage else 'needler';e=dict(env)
   if camouflage:e['HALO2_COMBAT_AUDIT_CAMOUFLAGE']='1'
   report=out/(label+'.json')
   with (out/(label+'-command.log')).open('w') as log:r=subprocess.run([sys.executable,str(ROOT/'tools/run_combat_paired_20261022.py'),str(exe),'fire',str(map_),str(report),'needler'],env=e,stdout=log,stderr=subprocess.STDOUT)
   rows.append(dict(case=label,exit_code=r.returncode,report=str(report)));print(json.dumps(rows[-1]),flush=True)
  (out/'cohort.json').write_text(json.dumps(rows,indent=2)+'\n');return int(any(row['exit_code'] for row in rows))
if __name__=='__main__':raise SystemExit(main())
