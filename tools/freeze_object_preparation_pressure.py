#!/usr/bin/env python3
"""Freeze preparation/pressure source owners and audit prior immutable pins."""
import argparse,json
from pathlib import Path
from freeze_mp_native_sound_storage import ROOT,sha,actual,relative,add,verify,copy
from oracle_mp_title_update_display import save,encoded
from validate_object_preparation_pressure import OWN,DEP

BASE=ROOT/'analysis/multiplayer/integration/spawn-source/object-preparation-pressure'
DEPENDENCIES={
 'analysis/multiplayer/integration/spawn-source/generic-object-constructor/manifest.json':'eca49b6594615fff1fc2a96e206dd5371a548c04fbc0bb100a2e9f95fc16e0aa',
 'analysis/multiplayer/integration/looping-construction-operands-20261009/frozen/manifest.json':'bcb5fa772eded922b4398da43cf34e6a19df1d9e63cedc4dc3e17119a26be0e5',
}
NAMES=OWN+('tools/oracle_mp_object_preparation_pressure_1_1.py',
 'tools/research_mp_object_preparation_pressure_u5.py','tools/measure_object_preparation_pressure_controls.py',
 'tools/validate_object_preparation_pressure.py','tools/validate_object_preparation_pressure_production.py',
 'tools/freeze_object_preparation_pressure.py','docs/MP_OBJECT_PREPARATION_PRESSURE_RESEARCH.md')

def audit(report):
 for group in ('artifact_pins','protected_prior_pins','new_source_pins','dependency_pins'):verify(report[group])
 files={relative(path)for path in BASE.rglob('*')if path.is_file()and path.name not in('manifest.json','audit-final.json','audit-repeat.json')}
 assert files==set(report['artifact_pins']),'Frozen evidence inventory changed'
 text=(ROOT/'CMakeLists.txt').read_text()
 for fragment in report['cmake_owned_fragments']:assert fragment in text,'Owned registration changed'
 for left,right in report['exact_repeat_pairs']:assert actual(left).read_bytes()==actual(right).read_bytes(),(left,right)
 return dict(manifest_sha256=sha(BASE/'manifest.json'),owned=len(report['new_source_pins']),artifacts=len(report['artifact_pins']),protected=len(report['protected_prior_pins']),dependencies=len(report['dependency_pins']),repeat_pairs=len(report['exact_repeat_pairs']),mismatches=0)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--audit-output',type=Path);a=p.parse_args();destination=BASE/'manifest.json'
 if not destination.exists():
  prior=dict(DEPENDENCIES);verify(prior)
  for name in DEPENDENCIES:
   dependency=json.loads(actual(name).read_text())
   for group in ('artifact_pins','protected_prior_pins','new_source_pins','owned_source_pins','dependency_pins'):
    for name,pin in dependency.get(group,{}).items():add(prior,name,pin)
  verify(prior)
  assert sha(BASE/'measured/report.json')==sha(BASE/'repeated/report.json')=='81976d9769e4e05df5b031eb50af97963e03aacb6564b63393726522c45291ca'
  assert sha(BASE/'controls-measured-v2/report.json')==sha(BASE/'controls-repeated-v2/report.json')=='e7fa44c6dec80ef563d8fd6e1e60aefac6079198abaad24cbe2c70b157a9eea0'
  controls=json.loads((BASE/'controls-measured-v2/report.json').read_text());assert len(controls['natural'])==55 and len(controls['cases'])==262
  assert controls['paired_retained_outputs_equal']and not controls['instruction_patches']and not controls['production_unit_copies']
  assert sum(len(c['lower_calls'])for c in controls['cases'])==4608
  validation=json.loads((BASE/'validated/report.json').read_text());assert validation['normal_and_asan_ubsan_pass']
  for name,pin in validation['owned_source_pins'].items():assert sha(actual(name))==sha(BASE/'validated/source'/name)==pin
  for mode in ('normal','sanitizer'):
   folder=BASE/f'production-{mode}';report=json.loads((folder/'report.json').read_text());assert report['owned_object_equals_observed_archive_member']
   assert all(not c['exit_code']for c in json.loads((folder/'commands.json').read_text()).values())
   if mode=='sanitizer':assert report['actual_sanitizer_flags_checked']
   for name in OWN+DEP:assert sha(actual(name))==sha(folder/'source-before'/name)==report['observed_source_pins'][name]
  for name in NAMES+DEP:copy(actual(name),BASE/'source'/name)
  copy(ROOT/'CMakeLists.txt',BASE/'cmake-observed.txt')
  text=(ROOT/'CMakeLists.txt').read_text();start=text.index('    find_path(HALO2_TEST_JSON_INCLUDE',text.index('    add_executable(object_construction_tests'))
  end=text.index('    endif()',start)+len('    endif()')
  fragments=['target_sources(halo2_systems PRIVATE src/gameplay/objects/object_preparation_pressure.cpp)',text[start:end]]
  save(BASE/'cmake-owned-fragments.json',encoded(fragments))
  pairs=[(relative(BASE/'measured/report.json'),relative(BASE/'repeated/report.json')),
   (relative(BASE/'controls-measured-v2/report.json'),relative(BASE/'controls-repeated-v2/report.json'))]
  for path in sorted((BASE/'u5').rglob('*')):
   if path.is_file():pairs.append((relative(path),relative(BASE/'u5-repeat'/path.relative_to(BASE/'u5'))))
  for path in sorted((BASE/'measured/instructions').glob('*')):pairs.append((relative(path),relative(BASE/'repeated/instructions'/path.name)))
  artifacts={relative(path):sha(path)for path in sorted(BASE.rglob('*'))if path.is_file()and path.name not in('manifest.json','audit-final.json','audit-repeat.json')}
  dependencies={name:sha(actual(name))for name in DEP}
  result=dict(schema='object-preparation-pressure-frozen-v1',artifact_pins=artifacts,protected_prior_pins=prior,new_source_pins={name:sha(actual(name))for name in NAMES},dependency_pins=dependencies,dependency_manifests=DEPENDENCIES,cmake_owned_fragments=fragments,exact_repeat_pairs=pairs,
   complete_instruction_count=241,natural_entries=40,declared_preparation_entries=15,paired_original_controls=262,explicit_test_only_lower_callbacks=4608,ordered_retained_stores=831,retained_output_bytes=71690,
   independent_original_and_pair_repeats=True,copied_and_production_normal_asan_ubsan_pass=True,emulator_sticky_status_authority=False,native_actual_sse_context_and_host_restoration=True,lower_reclamation_world_success_provider=False,final_unit_injection=False,runtime_or_launcher_changes=False,gpu_runs=0,
   next_live_owner='Real raw header/body heap allocation and growth, physical/animation/world/resource owners; no ready/offset/body substitutes.')
  save(destination,encoded(result))
 report=json.loads(destination.read_text());result=audit(report)
 if a.audit_output:save(a.audit_output,encoded(result))
 print(json.dumps(result))
if __name__=='__main__':main()
