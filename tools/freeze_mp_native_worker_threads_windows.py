#!/usr/bin/env python3
"""Append the actual Windows C++/Wine worker check without rewriting the native freeze."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha,actual,relative,verify,copy
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/native-worker-threads-windows-20261009'
FROZEN=BASE/'frozen'
PARENT='analysis/multiplayer/integration/native-worker-threads-20261009/frozen/manifest.json'
PIN='64040b6f32814cb750c96ff5250dd43671a8848c35defd90321f3b7bc5ef010f'
def main():
 destination=FROZEN/'manifest.json'
 if destination.exists():
  d=json.loads(destination.read_text());verify(d['artifact_pins']);verify(d['protected_prior_pins'])
  print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
 verify({PARENT:PIN});parent=json.loads(actual(PARENT).read_text())
 prior={PARENT:PIN,**parent['artifact_pins'],**parent['protected_prior_pins']};verify(prior)
 commands=json.loads((BASE/'validation/commands.json').read_text())
 assert len(commands)==2 and all(row['exit_code']==0 for row in commands)
 assert 'realWorkers=6 realPermitConsumptions=2 postWaitStores=2'in(BASE/'validation/test.log').read_text()
 for path in sorted((BASE/'validation').rglob('*')):
  if path.is_file():copy(path,FROZEN/'validation'/path.relative_to(BASE/'validation'))
 for name in ['include/halo2/native_threads.hpp','src/native_threads.cpp','tests/native_threads_tests.cpp',
              'include/halo2/native_synchronization.hpp','src/native_synchronization.cpp',
              'tools/freeze_mp_native_worker_threads_windows.py']:
  copy(actual(name),FROZEN/'source'/name)
  if name!='tools/freeze_mp_native_worker_threads_windows.py':
   assert sha(actual(name))==sha(actual(PARENT).parent/'source'/name)
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='native-worker-windows-wine-v1',artifact_pins=artifacts,protected_prior_pins=prior,
 actual_windows_binary_and_wine_passed=True,native_windows_hardware=False,source_worker_execution=False,
 live_audio=False,launcher_changed=False,scope='Same frozen real native thread/synchronization implementation compiled for Windows and executed under Wine; original kernel/worker integration remains separate.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
