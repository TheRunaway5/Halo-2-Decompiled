#!/usr/bin/env python3
"""Freeze actual cache reads and stream CPU prefixes without platform results."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy

BASE=ROOT/'analysis/multiplayer/integration/sound-cache-stream-production-20261009'
FROZEN=BASE/'frozen'
STREAM=ROOT/'analysis/multiplayer/integration/sound-driver-stream-native'
DEPS={
 'analysis/multiplayer/integration/native-worker-threads-windows-20261009/frozen/manifest.json':
 '8da04a75f3ed75ef4f46e101db4b90a5e8ec7a2e00f8def3e5e257c219ecc7a8',
 'analysis/multiplayer/integration/sound-cache-request-native/final-manifest.json':
 '2568f5ded360ee5e8d7fd9967045f7c12a11fbb462b23f0e1beaf7d69b9c07dc',
 'analysis/multiplayer/integration/spawn-source/animation-async-native-worker/manifest.json':
 '507c07480977e9621ffb3b183dba5465db64b43f587c3085a240221357d0afdb',
}
REPORTS={
 'paired/report.json':'781933decd99123c63996cdea1900a527247db159750f9dd74711dbe8d254646',
 'method-abi/report.json':'5d892b7f3c223c6274cbf70ae43c4a3b496c8e2bdd3c8d27117c3992bbe4be9b',
 'measured/report.json':'c1b0a969c0c79797c4e738ebb84e234f4c2cc634dd99abfde222cc1c25e0b724',
 'validated/report.json':'59d736c37d7d627609eb07448fc996c23bb0f38f9d27e80eed9ce8d2b1c14cc2',
 'fixtures.tsv':'40c2ce8f0ace2fca2df05d58b15f73b9ad3a726556a275d5394ce3cc05fa0b41',
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
                'evidence_files','original_inputs','prior_pins','files','dependency_pins','new_source_pins']:
   for path,digest in dependency.get(group,{}).items():add(prior,path,digest)
 verify(prior)
 for name,pin in REPORTS.items():assert sha(STREAM/name)==pin,name
 for left,right in [('paired','paired-repeat'),('method-abi','method-abi-repeat'),('measured','repeated')]:
  a=STREAM/left;b=STREAM/right
  files={p.relative_to(a):sha(p)for p in a.rglob('*')if p.is_file()}
  assert files=={p.relative_to(b):sha(p)for p in b.rglob('*')if p.is_file()},(left,right)
 snapshot=json.loads((STREAM/'validated/report.json').read_text())
 for name,pin in snapshot['source_pins'].items():
  assert sha(actual(name))==sha(STREAM/'validated/source'/name)==pin,name
 for folder in ['paired','paired-repeat','method-abi','method-abi-repeat','measured','repeated','validated']:
  for path in sorted((STREAM/folder).rglob('*')):
   if path.is_file():copy(path,FROZEN/'stream-evidence'/folder/path.relative_to(STREAM/folder))
 for path in sorted(STREAM.glob('fixtures.*')):copy(path,FROZEN/'stream-evidence'/path.name)
 names=['CMakeLists.txt','include/halo2/original_data.hpp','src/assets/original_data.cpp',
 'include/halo2/sound_driver_stream.hpp','src/audio/sound_driver_stream.cpp',
 'tests/sound_driver_stream_tests.cpp','tests/sound_driver_stream_constants_tests.cpp',
 'include/halo2/sound_cache_request.hpp','src/audio/sound_cache_request.cpp','tests/sound_cache_request_tests.cpp',
 'tools/research_sound_driver_stream_u5.py','tools/research_sound_driver_stream_abi.py',
 'tools/oracle_sound_driver_stream_native.py','tools/export_sound_driver_stream_fixtures.py',
 'tools/export_mp_sound_driver_stream_constants.py','tools/validate_sound_driver_stream_native.py',
 'tools/run_owned_fixture_test.py','tools/freeze_mp_native_sound_storage.py',
 'tools/freeze_mp_sound_cache_stream_production.py','docs/MP_SOUND_CACHE_STREAM_PRODUCTION_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 provider=ROOT/'analysis/multiplayer/integration/sound-driver-stream-constants-provider-20261009/measured'
 assert sha(provider/'report.json')=='9d22c1f3985f7f69d68f74217fa584e3f9d5b5bb2d8042780fe62d193fcc0109'
 for path in sorted(provider.rglob('*')):
  if path.is_file():copy(path,FROZEN/'constants'/path.relative_to(provider))
 tests=['sound_driver_stream_tests','sound_driver_stream_constants_tests','sound_cache_request_tests']
 for mode,build_name in [('normal','build-animation-metadata-store-20261008'),
                         ('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/mode
  commands=json.loads((folder/'commands.json').read_text());log=(folder/'test.log').read_text()
  assert set(commands)=={'configure','build','test'}and all(not q['exit_code']for q in commands.values())
  for text in ['100% tests passed out of 3','174 original stream cases, 3860 ordered stores',
               '24969696 full region bytes, 164 exact create requests without results',
               'immutableStreamBytes=56 baseUnknown=absent','18 full phases; 8346 ordered stores',
               '65779560 whole bytes; 3 actual owned shared.map reads']:
   assert text in log,text
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(folder))
  build=ROOT/build_name
  for library in ['halo2_cache','halo2_systems','halo2_native_synchronization']:
   copy(build/f'lib{library}.a',FROZEN/mode/f'lib{library}.a')
   for name in ['flags.make','link.txt']:copy(build/'CMakeFiles'/f'{library}.dir'/name,FROZEN/mode/library/name)
  for test in tests:
   copy(build/test,FROZEN/mode/test)
   for name in ['flags.make','link.txt']:copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/f'{test}-build'/name)
  if mode=='sanitizer':
   for target in ['halo2_cache','halo2_systems','halo2_native_synchronization',*tests]:
    assert '-fsanitize=address,undefined'in(build/'CMakeFiles'/f'{target}.dir/flags.make').read_text(),target
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='sound-cache-stream-production-v1',artifact_pins=artifacts,
 protected_prior_pins=prior,dependency_manifests=DEPS,stream_cases=174,stream_ordered_stores=3860,
 stream_whole_region_bytes=24969696,exact_stream_requests_without_results=164,
 immutable_stream_operand_bytes=56,actual_cache_reads=3,cache_phases=18,
 cache_ordered_stores=8346,cache_whole_region_bytes=65779560,
 production_normal_and_asan_ubsan_passed=True,independent_original_repeats_equal=True,
 concurrent_cache_access=False,returned_stream_creation=False,source_driver_admission=False,
 source_voice_playback=False,audible_playback=False,launcher_changed=False,
 scope='Original stream CPU prefix and serial actual cache/worker reads; post-platform stream code and live source lifecycle remain unverified.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n');verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
