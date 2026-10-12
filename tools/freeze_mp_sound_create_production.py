#!/usr/bin/env python3
"""Freeze sound-create production/import comparisons without supplying cache success."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha,actual,relative,add,verify,copy
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-create-production-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/sound-promotion-map-owner-20261009/frozen/manifest.json':
 'e4e88de7a815ceba57f2958ce6e3e9b047f72f97bfbbff83f0c6d9c7697449ca',
 'analysis/multiplayer/integration/sound-create-native/final-manifest.json':
 '6483852f56ef577383d3fd80edb7edec16a942f0067d654742e9370c95026318',
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
 names=['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp',
 'include/halo2/sound_create.hpp','src/sound_create.cpp','tests/sound_create_tests.cpp',
 'tests/sound_create_constants_tests.cpp','tools/export_mp_sound_create_constants.py',
 'tools/run_owned_fixture_test.py','tools/freeze_mp_sound_create_production.py',
 'tools/freeze_mp_native_sound_storage.py','docs/MP_SOUND_CREATE_PRODUCTION_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 constants=ROOT/'analysis/multiplayer/integration/sound-create-constants-provider-20261009/measured'
 for path in sorted(constants.rglob('*')):
  if path.is_file():copy(path,FROZEN/'constants'/path.relative_to(constants))
 tests=['sound_create_tests','sound_create_constants_tests']
 for mode,build_name in [('normal','build-animation-metadata-store-20261008'),
                         ('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/mode
  commands=json.loads((folder/'commands.json').read_text());log=(folder/'test.log').read_text()
  if set(commands)!={'configure','build','test'}or any(row['exit_code']for row in commands.values()):
   raise ValueError('Fresh production command failed or missing')
  for text in ['100% tests passed out of 2','138 original creation prefixes, 14858 ordered stores',
               '65947824 full region bytes','138 exact cache requests stopped without results',
               'immutableSoundCreateBytes=88 baseUnknown=absent']:
   if text not in log:raise ValueError('Fresh production comparison missing: '+text)
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(folder))
  build=ROOT/build_name
  copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
  for test in tests:
   copy(build/test,FROZEN/mode/test)
   for name in ['flags.make','link.txt']:
    copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/f'{test}-build'/name)
  for name in ['flags.make','link.txt']:
   copy(build/'CMakeFiles/halo2_cache.dir'/name,FROZEN/mode/'cache-build'/name)
  flags=(build/'CMakeFiles/halo2_cache.dir/flags.make').read_text()
  if '-frounding-math'not in flags or '-ffp-contract=off'not in flags:
   raise ValueError('Verified source floating point compilation unavailable')
  if mode=='sanitizer'and '-fsanitize=address,undefined'not in flags:
   raise ValueError('Actual sanitizer compilation unavailable')
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='sound-create-production-immutable-import-v1',
 artifact_pins=artifacts,protected_prior_pins=prior,dependency_manifests=DEPS,
 creation_prefixes=138,ordered_stores=14858,whole_region_bytes=65947824,
 immutable_operand_bytes=88,exact_cache_requests_without_results=138,
 production_normal_and_asan_ubsan_passed=True,source_admission=False,
 returned_creation=False,live_audio=False,launcher_changed=False,
 limits='Complete creation CPU prefix to mandatory cache service; no cache result, admitted sound or playback.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                       protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
