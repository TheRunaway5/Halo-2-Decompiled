#!/usr/bin/env python3
"""Freeze actual Cache-backed mutable promotion comparisons."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha,actual,relative,add,verify,copy
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-promotion-map-owner-20261009'
FROZEN=BASE/'frozen'
DEPS={
 'analysis/multiplayer/integration/sound-promotion-production-20261009/frozen/manifest.json':
 'a7ca3a07efe83429451090080344d26e247eec442c35516ed7192a530bd39df1',
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
  for group in ['artifact_pins','protected_prior_pins']:
   for path,digest in dependency.get(group,{}).items():add(prior,path,digest)
 verify(prior)
 names=['CMakeLists.txt','include/halo2/source_mutable_tag_store.hpp',
 'src/source_mutable_tag_store.cpp','include/halo2/source_tag_metadata.hpp',
 'src/source_tag_metadata_store.cpp','include/halo2/source_tag_metadata_store.hpp',
 'include/halo2/cache.hpp','src/cache.cpp','include/halo2/sound_promotion.hpp',
 'src/sound_promotion.cpp','tests/sound_promotion_map_owner_tests.cpp',
 'tools/run_owned_fixture_test.py','tools/freeze_mp_sound_promotion_map_owner.py',
 'tools/freeze_mp_native_sound_storage.py','docs/MP_SOUND_PROMOTION_MAP_OWNER_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 for path in sorted((BASE/'validation').rglob('*')):
  if path.is_file():copy(path,FROZEN/'validation'/path.relative_to(BASE/'validation'))
 for mode,build_name in [('normal','build-animation-metadata-store-20261008'),
                         ('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/(mode+'-indexed-final')
  commands=json.loads((folder/'commands.json').read_text());log=(folder/'test.log').read_text()
  if any(row['exit_code']for row in commands.values()):raise ValueError('Fresh production command failed')
  for text in ['100% tests passed out of 1','ownedMapBanks=46 sourceCalls=74318 orderedStores=249740',
               'fullRegionBytes=19211104 importedMutableTagBytes=9173672',
               'persistentCountersAndProviderLifetime=passed']:
   if text not in log:raise ValueError('Fresh production comparison missing: '+text)
  build=ROOT/build_name;copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
  copy(build/'sound_promotion_map_owner_tests',FROZEN/mode/'sound_promotion_map_owner_tests')
  for name in ['flags.make','link.txt']:
   copy(build/'CMakeFiles/sound_promotion_map_owner_tests.dir'/name,FROZEN/mode/name)
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='source-mutable-map-sound-promotion-production-v1',
 artifact_pins=artifacts,protected_prior_pins=prior,dependency_manifests=DEPS,
 owned_map_banks=46,source_calls=74318,ordered_stores=249740,whole_region_bytes=19211104,
 actual_cache_import_bytes=9173672,original_cache_unchanged=True,
 persistent_mutable_map_backing=True,production_normal_and_asan_ubsan_passed=True,
 source_admission=False,live_audio=False,launcher_changed=False,
 scope='Actual retained map payloads and complete promotion/clock CPU comparison; original arena/tag table/clock origin remain explicit genuine fixture inputs.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
