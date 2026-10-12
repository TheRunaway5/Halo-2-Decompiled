#!/usr/bin/env python3
"""Freeze promotion production comparisons and actual native synchronization."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha,actual,relative,add,verify,copy
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-promotion-production-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/sound-submit-listener-production-20261009/frozen/manifest.json':
 'b0ec82e9b3e4c03228da9d9ad5a49bea68f81a0b4ee22fd81dfb0fba6cd0d31b',
 'analysis/multiplayer/integration/sound-promotion-native/final-manifest.json':
 'eb053b6680b4917f9d03eec27b078c725932a9b4e4cb369e13ebbc2a7c11940e',
}
CONTRACTS={
 'analysis/multiplayer/integration/spawn-source/animation-async-construction/contracts/report.json':
 '32fdef27ca343207a873e59fd27c38e0203c2b84a0f623af82fa6363d2df7b49',
 'analysis/multiplayer/integration/spawn-source/animation-async-construction/measured/report.json':
 'e1c8420ce6fc4abf6bcd9626cb0294a66847594a57fd68c72db21bc064509304',
}
def main():
 destination=FROZEN/'manifest.json'
 if destination.exists():
  manifest=json.loads(destination.read_text())
  for key in ['artifact_pins','protected_prior_pins']:verify(manifest[key])
  print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
 prior={**DEPS,**CONTRACTS};verify(prior)
 for name in DEPS:
  dependency=json.loads(actual(name).read_text())
  for group in ['artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
                'evidence_files','original_inputs','prior_pins','files','dependency_pins']:
   for path,digest in dependency.get(group,{}).items():add(prior,path,digest)
 verify(prior)
 names=['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp',
 'include/halo2/sound_promotion.hpp','src/sound_promotion.cpp','tests/sound_promotion_tests.cpp',
 'tests/sound_promotion_constants_tests.cpp','tools/export_mp_sound_promotion_constants.py',
 'include/halo2/native_synchronization.hpp','src/native_synchronization.cpp',
 'tests/native_synchronization_tests.cpp','tools/run_owned_fixture_test.py',
 'tools/freeze_mp_sound_promotion_production.py','tools/freeze_mp_native_sound_storage.py',
 'docs/MP_SOUND_PROMOTION_PRODUCTION_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 constants=ROOT/'analysis/multiplayer/integration/sound-promotion-constants-provider-20261009/measured'
 for path in sorted(constants.rglob('*')):
  if path.is_file():copy(path,FROZEN/'constants'/path.relative_to(constants))
 for name in CONTRACTS:copy(actual(name),FROZEN/'contracts'/actual(name).parent.name/actual(name).name)
 tests=['sound_promotion_tests','sound_promotion_constants_tests','native_synchronization_tests']
 for mode,build_name in [('normal','build-animation-metadata-store-20261008'),
                         ('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/(mode+'-synchronization')
  commands=json.loads((folder/'commands.json').read_text());log=(folder/'test.log').read_text()
  if any(row['exit_code']for row in commands.values()):raise ValueError('Fresh production command failed')
  for text in ['100% tests passed out of 3','74318 calls, 249740 ordered stores',
               '19211104 full region bytes','immutablePromotionBytes=24',
               'concurrentProtectedWrites=800 realPermitConsumptions=151']:
   if text not in log:raise ValueError('Fresh production comparison missing: '+text)
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(folder))
  build=ROOT/build_name
  for archive in ['libhalo2_cache.a','libhalo2_native_synchronization.a']:
   copy(build/archive,FROZEN/mode/archive)
  for test in tests:
   copy(build/test,FROZEN/mode/test)
   for name in ['flags.make','link.txt']:
    copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/f'{test}-build'/name)
  for name in ['flags.make','link.txt']:
   copy(build/'CMakeFiles/halo2_native_synchronization.dir'/name,FROZEN/mode/'synchronization-build'/name)
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='sound-promotion-native-synchronization-production-v1',
 artifact_pins=artifacts,protected_prior_pins=prior,dependency_manifests=DEPS,
 promotion_calls=74318,promotion_ordered_writes=249740,whole_region_bytes=19211104,
 immutable_operand_bytes=24,native_protected_writes=800,native_permit_consumptions=151,
 production_normal_and_asan_ubsan_passed=True,native_objects_actually_owned=True,
 xbox_status_tls_worker_or_io_claimed=False,source_admission=False,
 live_audio=False,live_animation=False,launcher_changed=False,
 limits='CPU comparison and native synchronization only; retained mutable map banks and real worker/file owners remain separate.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                       protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
