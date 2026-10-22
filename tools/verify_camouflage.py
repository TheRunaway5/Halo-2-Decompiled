#!/usr/bin/env python3
"""Freeze and compare source camouflage kernels; normal gameplay remains gated."""
import argparse,datetime,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer/game-variants/camouflage-native-20261011'
OWNERS=('src/rendering/source_camouflage.cpp','src/gameplay/multiplayer/source_king.cpp')
FIXTURE='tests/fixtures/camouflage_state_original.json'
DISRUPTION='tests/fixtures/camouflage_disruption_original.json'
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'verification.json');a=ap.parse_args()
 run=a.output.parent/('verify-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ'))
 source=run/'frozen-source';build=run/'build';temp=run/'compiler-temp';source.mkdir(parents=True);temp.mkdir();pins={};commands=[]
 for folder in ('include','src','tests','tools','cmake','external','resources','shaders'):
  for path in sorted((ROOT/folder).rglob('*')):
   if not path.is_file()or '__pycache__'in path.parts or path.suffix in('.pyc','.pyo'):continue
   name=path.relative_to(ROOT);target=source/name;target.parent.mkdir(parents=True,exist_ok=True)
   target.write_bytes(path.read_bytes());pins[str(name)]=sha(target)
 for name in ('CMakeLists.txt','README.md'):
  shutil.copy2(ROOT/name,source/name);pins[name]=sha(source/name)
 for name in ('assets','analysis'):(source/name).symlink_to(ROOT/name,target_is_directory=True)
 (run/'source-hashes.json').write_text(json.dumps(pins,sort_keys=True,indent=2)+'\n')
 # The whole-owner oracle uses these three original executable images and
 # actual Lockout tag backing. No other game asset is a kernel input.
 asset_paths=('assets/disc/default.xbe','assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
     '/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',
     'assets/disc/maps/lockout.map')
 assets={name:sha(source/name)for name in asset_paths}
 (run/'asset-hashes.json').write_text(json.dumps(assets,sort_keys=True,indent=2)+'\n')
 env={**os.environ,'TMPDIR':str(temp),'PYTHONDONTWRITEBYTECODE':'1'}
 def execute(label,cmd,cwd=source,extra=None):
  log=run/(label+'.log')
  with log.open('w')as output:r=subprocess.run(list(map(str,cmd)),cwd=cwd,env={**env,**(extra or {})},stdout=output,stderr=subprocess.STDOUT)
  commands.append(dict(label=label,command=list(map(str,cmd)),cwd=str(cwd),exit=r.returncode,
      log=str(log.relative_to(a.output.parent)),log_sha256=sha(log)))
  print(label,'exit',r.returncode,flush=True)
  if r.returncode:raise RuntimeError(f'{label} failed; see {log}')
 execute('configure',['cmake','-S',source,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DBUILD_TESTING=ON'])
 execute('build',['cmake','--build',build,'--target','camouflage_tests','-j2'])
 execute('ctest',['ctest','--test-dir',build,'-R','^original_camouflage$','--output-on-failure'])
 original=run/'original'
 execute('original-camouflage',[sys.executable,source/'tools/oracle_camouflage_fp.py','--output',original/'report.json','--fixture',original/'fixture.json'])
 assert sha(original/'fixture.json')==pins[FIXTURE],'Original camouflage fixture did not reproduce'
 report=json.loads((original/'report.json').read_text())
 execute('original-disruption',[sys.executable,source/'tools/oracle_camouflage_disruption.py','--output',original/'disruption-report.json','--fixture',original/'disruption-fixture.json'])
 assert sha(original/'disruption-fixture.json')==pins[DISRUPTION]
 binary=run/'camouflage_tests_asan'
 execute('sanitizer-build',['c++','-std=c++20','-O1','-g','-fno-fast-math','-ffp-contract=off','-fsanitize=address,undefined','-fno-omit-frame-pointer',
     '-I'+str(source/'include'), '-I'+str(source/'src'),source/'tests/camouflage_tests.cpp',*(source/name for name in OWNERS),'-o',binary])
 execute('sanitizer-test',[binary,source/FIXTURE,source/DISRUPTION],extra={'ASAN_OPTIONS':'detect_leaks=1:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'})
 assert all(sha(source/name)==digest for name,digest in pins.items())
 assert all(sha(source/name)==digest for name,digest in assets.items())
 result=dict(schema='source-camouflage-kernel-verification-v1',scope=__doc__,frozen_source=str(source.relative_to(a.output.parent)),
     source_hashes=str((run/'source-hashes.json').relative_to(a.output.parent)),source_hashes_sha256=sha(run/'source-hashes.json'),
     asset_hashes=str((run/'asset-hashes.json').relative_to(a.output.parent)),asset_hashes_sha256=sha(run/'asset-hashes.json'),
     original_report=str((original/'report.json').relative_to(a.output.parent)),original_report_sha256=sha(original/'report.json'),
     fixture_sha256=sha(original/'fixture.json'),disruption_fixture_sha256=sha(original/'disruption-fixture.json'),commands=commands,
     binaries={str(p.relative_to(a.output.parent)):sha(p)for p in(build/'camouflage_tests',binary)},
     sanitizer_scope='Complete standalone camouflage test and both source owners; no ordinary native library linkage.',
     native_live_binding=False,production_king_admission=False,king_transport_admission=False,goal_complete=False,
     live_checkout_drift=[name for name,digest in pins.items()if not(ROOT/name).exists()or sha(ROOT/name)!=digest])
 a.output.write_text(json.dumps(result,sort_keys=True,indent=2)+'\n');print(a.output,flush=True)
if __name__=='__main__':main()
