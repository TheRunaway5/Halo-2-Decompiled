#!/usr/bin/env python3
"""Reproduce the actual two-view MGS2 material-upload/black-sprite failure."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 owner=Path(os.environ['HALO2_TEST_SCRATCH']);source=owner/'source';out=ROOT/'analysis/combat-presentation-audit-20261022/repair-glow-material-banks';out.mkdir(exist_ok=True);receipt=[]
 relative='tests/projectile_glow_gpu_tests.cpp';p=source/relative;p.write_bytes((ROOT/relative).read_bytes());receipt.append(dict(path=relative,sha256=sha(p)))
 p=source/'CMakeLists.txt';s=p.read_text();needle='        add_executable(camouflage_material_gpu_tests tests/camouflage_material_gpu_tests.cpp)';assert s.count(needle)==1
 if 'add_executable(projectile_glow_gpu_tests' not in s:
  s=s.replace(needle,needle+'''\n        add_executable(projectile_glow_gpu_tests tests/projectile_glow_gpu_tests.cpp)
        target_link_libraries(projectile_glow_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME projectile_glow_material_banks COMMAND projectile_glow_gpu_tests "${HALO2_ROCKET_GHOST_MCC_MAP}")
        set_tests_properties(projectile_glow_material_banks PROPERTIES SKIP_RETURN_CODE 77)''');p.write_text(s)
 receipt.append(dict(path='CMakeLists.txt',sha256=sha(p)));(out/'red-source-refresh.json').write_text(json.dumps(receipt,indent=2)+'\n')
 from validate_weapon_runtime_tu15 import private_display
 display_root=owner/'glow-bank-display';display_root.mkdir(exist_ok=True)
 with private_display(display_root) as display:
  env=dict(os.environ,DISPLAY=display,SDL_VIDEODRIVER='x11',SDL_AUDIODRIVER='dummy',XDG_CACHE_HOME=str(owner/'graphics-cache'))
  cmd=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','projectile_glow_gpu_tests','--filter','^projectile_glow_material_banks$','--jobs','8','--test-jobs','1','--output',str(out/'red')]
  with (out/'red-command.log').open('w') as log:r=subprocess.run(cmd,env=env,stdout=log,stderr=subprocess.STDOUT)
  result=dict(exit_code=r.returncode,expected_failure=False)
  log=out/'red/tests.log'
  if log.exists():result['expected_failure']=r.returncode!=0 and 'Projectile glow material skipped the actual per-view uniform upload' in log.read_text()
  (out/'red-result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result),flush=True);return 0 if result['expected_failure'] else 1
if __name__=='__main__':raise SystemExit(main())
