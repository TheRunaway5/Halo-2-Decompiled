#!/usr/bin/env python3
"""Freeze actual native worker ownership and its production checks."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha,actual,relative,add,verify,copy
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/native-worker-threads-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/sound-create-production-20261009/frozen/manifest.json':
 '3d726a46ea59ac086b5650c47b4ef338c9ff1bd93c1fd1d3298c10b7695c3de3',
 'analysis/multiplayer/integration/spawn-source/animation-async-construction/manifest.json':
 '9260f28be79900f3f683926a4cbb54de0bc847909626a66049ac00272c966e61',
}
def main():
 destination=FROZEN/'manifest.json'
 if destination.exists():
  manifest=json.loads(destination.read_text())
  for key in ['artifact_pins','protected_prior_pins']:verify(manifest[key])
  print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
 prior=dict(DEPS);verify(prior)
 for name in DEPS:
  dependency=json.loads(actual(name).read_text())
  for group in ['artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
                'evidence_files','original_inputs','prior_pins','files','dependency_pins']:
   for path,digest in dependency.get(group,{}).items():add(prior,path,digest)
 verify(prior)
 names=['CMakeLists.txt','include/halo2/native_threads.hpp','src/native_threads.cpp',
 'tests/native_threads_tests.cpp','include/halo2/native_synchronization.hpp',
 'src/native_synchronization.cpp','tests/native_synchronization_tests.cpp',
 'tools/freeze_mp_native_worker_threads.py','tools/freeze_mp_native_sound_storage.py',
 'docs/MP_NATIVE_WORKER_THREADS_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 for path in sorted((BASE/'standalone').rglob('*')):
  if path.is_file():copy(path,FROZEN/'standalone'/path.relative_to(BASE/'standalone'))
 for mode,build_name in [('normal','build-animation-metadata-store-20261008'),
                         ('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/mode
  commands=json.loads((folder/'commands.json').read_text());log=(folder/'test.log').read_text()
  if set(commands)!={'configure','build','test'}or any(row['exit_code']for row in commands.values()):
   raise ValueError('Fresh production command failed or missing')
  for text in ['100% tests passed out of 2','realWorkers=6 realPermitConsumptions=2 postWaitStores=2',
               'blockedStopAndFailure=noSourceStores','finalOwner=stopAndJoin',
               'concurrentProtectedWrites=800 realPermitConsumptions=151']:
   if text not in log:raise ValueError('Fresh production comparison missing: '+text)
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(folder))
  build=ROOT/build_name
  copy(build/'libhalo2_native_synchronization.a',FROZEN/mode/'libhalo2_native_synchronization.a')
  for test in ['native_threads_tests','native_synchronization_tests']:
   copy(build/test,FROZEN/mode/test)
   for name in ['flags.make','link.txt']:
    copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/f'{test}-build'/name)
  for name in ['flags.make','link.txt']:
   copy(build/'CMakeFiles/halo2_native_synchronization.dir'/name,FROZEN/mode/'native-build'/name)
  if mode=='sanitizer'and '-fsanitize=address,undefined'not in (build/'CMakeFiles/halo2_native_synchronization.dir/flags.make').read_text():
   raise ValueError('Actual native sanitizer compilation unavailable')
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='native-worker-threads-production-v1',artifact_pins=artifacts,
 protected_prior_pins=prior,dependency_manifests=DEPS,real_workers_per_run=6,
 real_permit_consumptions_per_run=2,post_wait_stores_per_run=2,
 production_and_standalone_normal_and_asan_ubsan_passed=True,
 original_thread_tls_priority_ntstatus_or_close_claimed=False,
 source_worker_execution_claimed=False,live_audio=False,launcher_changed=False,
 limits='Real native worker/stop/join ownership only; original async CPU/file integration remains separate.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                       protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
