#!/usr/bin/env python3
"""Both actual Host/Live/runtime/World TUs over exact1480+retained stage."""
from pathlib import Path
import hashlib,json,os,shutil,subprocess
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-moving-shape-retained-world-stage-20261102/staged'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 base=Path(os.environ['HALO2_FROZEN_SOURCE']);owned=Path(os.environ['HALO2_TEST_SCRATCH']);out=ROOT/'analysis/physics/native-moving-retained-full-context-20261102-first';out.mkdir(exist_ok=False);r={'passed':False,'owned_scratch':str(owned),'retained_large_artifacts':[],'results':[]}
 try:
  tests=owned/'tests';shutil.copytree(base/'tests',tests)
  # Runtime checkpoints omit native component-only helpers; borrow only their
  # exact staged copies where needed, never an unrelated current Root version.
  if(STAGE/'tests/helpers').exists():shutil.copytree(STAGE/'tests/helpers',tests/'helpers',dirs_exist_ok=True)
  host='tests/physics_multiplayer_acceptance.cpp';shutil.copyfile(STAGE/host,tests/'physics_multiplayer_acceptance.cpp')
  old_flags=json.loads((ROOT/'analysis/physics/native-seated-look-input-capture-host-context-20261102-second/report.json').read_text())['compiler_results'][0]['command'][1:-2]
  host_flags=[('-I'+str(base/'include')if p.startswith('-I')else p)for p in old_flags]
  other=['-std=c++20','-O0','-fno-fast-math','-ffp-contract=off','-frounding-math','-isystem','/usr/include/SDL2']
  pins={str(p.relative_to(STAGE)):sha(p)for p in STAGE.rglob('*')if p.is_file()}
  for cc in ('g++','clang++'):
   for name,path,flags in [('Host',tests/'physics_multiplayer_acceptance.cpp',host_flags),('LiveWorld',STAGE/'src/physics_live_world_tu15.cpp',other),('ObjectRuntime',STAGE/'src/multiplayer_object_runtime.cpp',other),('WorldAdapter',STAGE/'src/physics/native_tu15_world_integration_residuals.cpp',other)]:
    command=[cc,'-I'+str(STAGE/'include'),'-I'+str(base/'include'),'-I'+str(STAGE/'src/physics'),'-I'+str(base/'src'),'-I'+str(base/'src/physics'),*flags,'-fsyntax-only',str(path)];run=subprocess.run(command,capture_output=True,text=True);(out/(cc+'-'+name+'.log')).write_text(run.stdout+run.stderr);r['results'].append({'compiler':cc,'translation_unit':name,'exit_code':run.returncode,'command':command});assert run.returncode==0,run.stderr[-5000:]
  r.update(passed=all(sha(STAGE/n)==v for n,v in pins.items()),stage_source_sha256=pins,scope='Exact frozen1480+retained stage full fourTU syntax contexts; no linked/runtime/visible claim.')
 except BaseException as error:r['error']=str(error)or repr(error);raise
 finally:r['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owned.rglob('*')if p.is_file());(out/'report.json').write_text(json.dumps(r,indent=2)+'\n')
 return 0 if r['passed']else 1
if __name__=='__main__':raise SystemExit(main())
