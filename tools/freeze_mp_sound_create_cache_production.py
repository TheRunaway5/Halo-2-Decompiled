#!/usr/bin/env python3
"""Freeze whole source creation with actual pending cache ownership and IO."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy
BASE=ROOT/'analysis/multiplayer/integration/sound-create-cache-production-20261009'
FROZEN=BASE/'frozen'
EVIDENCE=ROOT/'analysis/multiplayer/integration/sound-create-cache-native'
DEPS={
 'analysis/multiplayer/integration/sound-cache-stream-production-20261009/frozen/manifest.json':
 '9b1624b1ddcfe43dc1e78e9855211ef09618d219d86e633f370921f82ece8bf4',
}
REPORT='45f02efccebea71086c277526dab443b9fb52191f2b37d788069fc8060b55b9f'
FIXTURE='ccd0d64396100d4a88c521c306a2afc70e238bf81ca0fb05320ac94dd7e1f77f'

def main():
 destination=FROZEN/'manifest.json'
 if destination.exists():
  d=json.loads(destination.read_text());verify(d['artifact_pins']);verify(d['protected_prior_pins'])
  print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
 prior=dict(DEPS);verify(prior)
 for name in DEPS:
  dependency=json.loads(actual(name).read_text())
  for group in ['artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
                'evidence_files','original_inputs','prior_pins','files','dependency_pins','new_source_pins']:
   for path,pin in dependency.get(group,{}).items():add(prior,path,pin)
 verify(prior)
 assert sha(EVIDENCE/'measured/report.json')==REPORT
 assert sha(EVIDENCE/'fixtures.tsv')==FIXTURE
 def files(folder):return {str(p.relative_to(folder)):sha(p)for p in folder.rglob('*')if p.is_file()}
 assert files(EVIDENCE/'measured')==files(EVIDENCE/'repeated')
 r=json.loads((EVIDENCE/'measured/report.json').read_text())
 assert r['source_creation_returned']and not r['source_admitted']and not r['voice_or_playback']
 assert not r['instruction_patches']and not r['supplied_ready_or_completion']
 assert len(r['cases'])==3
 for case in r['cases']:
  phase=case['stages'][1]
  assert phase['name']=='whole_source_create_publishes_and_pins_pending'
  assert phase['returned']and phase['eax']=='ef730000'and phase['stop']is None
 validation=json.loads((EVIDENCE/'validated/native/commands.json').read_text())
 assert validation['fixture_sha256']==FIXTURE
 assert all(q['exit_code']==0 for q in validation['commands'])
 for name,pin in validation['observed_source_files'].items():
  assert sha(actual(name))==sha(EVIDENCE/'validated/native/source'/name)==pin,name
 for name in ['measured','repeated','validated']:
  for path in sorted((EVIDENCE/name).rglob('*')):
   if path.is_file():copy(path,FROZEN/'evidence'/name/path.relative_to(EVIDENCE/name))
 for name in ['fixtures.tsv','fixtures.json']:copy(EVIDENCE/name,FROZEN/'evidence'/name)
 for name in ['CMakeLists.txt','tests/sound_create_cache_tests.cpp',
              'tools/oracle_sound_create_cache_native.py','tools/export_sound_create_cache_fixtures.py',
              'tools/validate_sound_create_cache_native.py','tools/freeze_mp_sound_create_cache_production.py',
              'docs/MP_SOUND_CREATE_CACHE_PRODUCTION_RESEARCH.md']:
  copy(actual(name),FROZEN/'source'/name)
 expected='PASS 3 whole original sound creations; 18 full phases; 8678 ordered stores; 65779560 whole bytes; 3 actual owned shared.map reads; actual pending source/cache pins; no admission/voice/thread claim.'
 for mode,directory in [('normal','build-animation-metadata-store-20261008'),('sanitizer','build-animation-event-asan-20261008')]:
  folder=BASE/'validation'/mode;commands=json.loads((folder/'commands.json').read_text())
  assert set(commands)=={'configure','build','test'}and all(not q['exit_code']for q in commands.values())
  text=(folder/'test.log').read_text();assert expected in text and '100% tests passed out of 1'in text
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(folder))
  build=ROOT/directory
  for library in ['halo2_cache','halo2_systems','halo2_native_synchronization']:
   copy(build/f'lib{library}.a',FROZEN/mode/f'lib{library}.a')
   for name in ['flags.make','link.txt']:copy(build/'CMakeFiles'/f'{library}.dir'/name,FROZEN/mode/library/name)
  copy(build/'sound_create_cache_tests',FROZEN/mode/'sound_create_cache_tests')
  for name in ['flags.make','link.txt']:copy(build/'CMakeFiles/sound_create_cache_tests.dir'/name,FROZEN/mode/'test-build'/name)
  for target in ['halo2_cache','halo2_systems','halo2_native_synchronization','sound_create_cache_tests']:
   flags=(build/'CMakeFiles'/f'{target}.dir/flags.make').read_text()
   if mode=='sanitizer':assert '-fsanitize=address,undefined'in flags,target
  assert '-frounding-math'in(build/'CMakeFiles/halo2_cache.dir/flags.make').read_text()
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 d=dict(schema='sound-create-cache-production-v1',artifact_pins=artifacts,protected_prior_pins=prior,
 dependency_manifests=DEPS,whole_creation_cases=3,full_phases=18,ordered_stores=8678,
 full_output_bytes=65779560,actual_owned_shared_reads=3,actual_pending_source_and_cache_pin=True,
 original_repeat_byte_identical=True,snapshot_and_production_normal_asan_ubsan_passed=True,
 source_creation_returned=True,executed_update='1.1',concurrent_creation=False,source_admission=False,
 live_first_person_route=False,audible_playback=False,all_map_sound_parity=False,launcher_changed=False,
 scope='Whole creation/cache/queue/worker serial actual ABI composition; no original thread/driver/map admission or live audio claim.')
 destination.write_text(json.dumps(d,sort_keys=True,indent=2)+'\n');verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),protected_prior=len(prior),mismatches=0)))
if __name__=='__main__':main()
