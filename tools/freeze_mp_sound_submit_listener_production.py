#!/usr/bin/env python3
"""Freeze exact production submit/listener comparisons and owned operands."""
import json
from pathlib import Path
from freeze_mp_native_sound_storage import sha, actual, relative, add, verify, copy

ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration/sound-submit-listener-production-20261009'
FROZEN=BASE/'frozen'
LISTENER=ROOT/'analysis/multiplayer/integration/sound-driver-listener-native'
REMAINING=ROOT/'analysis/multiplayer/integration/sound-driver-remaining-native'
OPERANDS=ROOT/'analysis/multiplayer/integration/sound-driver-owner-operands-provider-20261009/measured'
DEPS={
 'analysis/multiplayer/integration/sound-driver-cleanup-production-20261009/frozen/manifest.json':
  '0bec100e4009d5c9bd0e47a02795e38977ff5f9bff6543a7608aa65b091dc85a',
 'analysis/multiplayer/integration/sound-submit-native/final-manifest.json':
  'b7d3de47f405ec45ea5d682ab4b7106ab4f65da83fd92a8d834a1d6efbc81ca3',
 'analysis/multiplayer/integration/sound-submit-promotion-output-erratum/manifest.json':
  '993cd08aa36edcafd94f4b232caf227e362e09031b53c5dcd3405e19dd13360a',
}


def repeats(first,second):
 paths=[path for path in sorted(first.rglob('*'))if path.is_file()]
 if [p.relative_to(first)for p in paths]!=[p.relative_to(second)for p in sorted(second.rglob('*'))if p.is_file()]:
  raise ValueError('Independent repeat inventories differ')
 for path in paths:
  if sha(path)!=sha(second/path.relative_to(first)):raise ValueError('Independent repeated evidence differs: '+str(path))
 return len(paths)


def main():
 destination=FROZEN/'manifest.json'
 if destination.exists():
  manifest=json.loads(destination.read_text())
  for key in ['artifact_pins','protected_prior_pins']:verify(manifest[key])
  print(json.dumps(dict(manifest_sha256=sha(destination),audit='passed')));return
 prior=dict(DEPS);verify(prior)
 for name in DEPS:
  dependency=json.loads(actual(name).read_text())
  for key in ['artifact_pins','protected_prior_pins','owned_input_pins','owned_current_files',
              'evidence_files','original_inputs','prior_pins','files','dependency_pins']:
   for path,digest in dependency.get(key,{}).items():add(prior,path,digest)
 verify(prior)
 repeated=repeats(REMAINING/'paired',REMAINING/'paired-repeat')
 repeated+=repeats(LISTENER/'measured',LISTENER/'repeated')
 for name in ['fixtures.tsv','fixtures.json']:
  if sha(LISTENER/name)!=sha(LISTENER/'export-repeat'/name):raise ValueError('Independent fixture export differs')
  repeated+=1
 names=['CMakeLists.txt','include/halo2/original_data.hpp','src/original_data.cpp',
        'include/halo2/cache.hpp','include/halo2/sound_driver_listener.hpp','src/sound_driver_listener.cpp',
        'include/halo2/sound_driver_owners.hpp','include/halo2/sound_driver_startup.hpp',
        'include/halo2/sound_map_activation.hpp','include/halo2/sound_startup.hpp',
        'include/halo2/sound_submit.hpp','src/sound_submit.cpp','include/halo2/sound_submit_operands.hpp',
        'include/halo2/native_sound_storage.hpp','src/native_sound_storage.cpp',
        'tests/sound_driver_listener_tests.cpp','tests/sound_submit_tests.cpp',
        'tests/sound_driver_owner_operands_tests.cpp','tools/run_owned_fixture_test.py',
        'tools/research_sound_driver_remaining_u5.py','tools/research_animation_selection_u5.py',
        'tools/oracle_sound_driver_listener.py','tools/export_sound_driver_listener_fixtures.py',
        'tools/export_mp_sound_driver_owner_operands.py','tools/freeze_mp_sound_submit_listener_production.py',
        'tools/freeze_mp_native_sound_storage.py','docs/MP_SOUND_SUBMIT_LISTENER_PRODUCTION_RESEARCH.md']
 for name in names:copy(actual(name),FROZEN/'source'/name)
 for label,folder in [('remaining',REMAINING),('listener',LISTENER),('operands',OPERANDS)]:
  for path in sorted(folder.rglob('*')):
   if path.is_file():copy(path,FROZEN/label/path.relative_to(folder))
 source=json.loads((LISTENER/'fixtures.json').read_text())
 if source['cases']!=128 or source['stores']!=2286 or source['complete_output_bytes']!=18382336:
  raise ValueError('Original listener coverage changed')
 for mode,directory in [('normal','build-animation-metadata-store-20261008'),
                        ('sanitizer','build-animation-event-asan-20261008')]:
  validation=BASE/'validation'/mode;log=(validation/'test.log').read_text()
  commands=json.loads((validation/'commands.json').read_text())
  if any(row['exit_code']for row in commands.values()):raise ValueError('Production command failed')
  for required in ['100% tests passed out of 3','listenerCases=128 orderedStores=2286',
                   'immutableSoundOwnerBytes=232','2574']:
   if required not in log:raise ValueError('Production submit/listener comparison failed: '+required)
  for path in sorted(validation.rglob('*')):
   if path.is_file():copy(path,FROZEN/'validation'/mode/path.relative_to(validation))
  build=ROOT/directory;copy(build/'libhalo2_cache.a',FROZEN/mode/'libhalo2_cache.a')
  for test in ['sound_driver_listener_tests','sound_submit_tests','sound_driver_owner_operands_tests']:
   copy(build/test,FROZEN/mode/test)
   for name in ['flags.make','link.txt']:
    copy(build/'CMakeFiles'/f'{test}.dir'/name,FROZEN/mode/f'{test}-build'/name)
 artifacts={relative(path):sha(path)for path in sorted(FROZEN.rglob('*'))if path.is_file()}
 manifest=dict(schema='sound-submit-listener-production-v1',artifact_pins=artifacts,protected_prior_pins=prior,
               dependency_manifests=DEPS,listener_cases=128,listener_ordered_stores=2286,
               listener_complete_output_bytes=18382336,listener_raw_control_returns=60,
               listener_platform_request_stops=66,listener_unavailable_stops=2,
               submit_cases=2574,submit_ordered_stores=196,submit_genuine_effect_cleanup_cycles=8,
               immutable_operand_bytes=232,independently_repeated_files=repeated,
               normal_and_asan_ubsan_passed=True,source_platform_returns=0,source_voice_admission=False,
               live_audio=False,launcher_changed=False,source_assets_external=True,
               archive_claim='Observed Cache archives; only named listener/submit/operand comparisons claimed.')
 destination.write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
 verify(artifacts);verify(prior)
 print(json.dumps(dict(manifest_sha256=sha(destination),artifacts=len(artifacts),
                       protected_prior=len(prior),mismatches=0,independently_repeated_files=repeated)))


if __name__=='__main__':main()
