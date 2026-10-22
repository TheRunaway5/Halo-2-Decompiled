#!/usr/bin/env python3
"""Immutable archive of the independently verified moving-death and constructor slices."""
import hashlib
import json
import shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'analysis/multiplayer/integration'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def encoded(v):return (json.dumps(v,indent=2,sort_keys=True)+'\n').encode()
def write(p,raw):
 p.parent.mkdir(parents=True,exist_ok=True)
 if p.exists():assert p.read_bytes()==raw,p
 else:p.write_bytes(raw)
def freeze(out,paths,summary):
 rows=[]
 for p in sorted(set(paths)):
  relative=p.relative_to(ROOT);target=out/'files'/relative
  target.parent.mkdir(parents=True,exist_ok=True)
  if target.exists():assert target.read_bytes()==p.read_bytes(),p
  else:shutil.copy2(p,target)
  rows.append(dict(path=str(relative),archive=str(target.relative_to(out)),bytes=p.stat().st_size,sha256=sha(p)))
 manifest=dict(schema=1,tool_sha256=sha(Path(__file__)),summary=summary,files=rows)
 write(out/'manifest.json',encoded(manifest))
 for row in rows:assert sha(out/row['archive'])==row['sha256']
 print(json.dumps(dict(manifest=str((out/'manifest.json').relative_to(ROOT)),sha256=sha(out/'manifest.json'),files=len(rows))))
def descendants(p):return [q for q in p.rglob('*')if q.is_file()]
def main():
 moving=BASE/'spartan-death-physical-20261010'
 constructor=BASE/'spartan-ragdoll-constructor-20261010'
 for a,b in ((moving/'moving-effect-fixed',moving/'moving-effect-fixed-repeat'),(constructor/'measured',constructor/'repeat')):
  for p in a.rglob('*'):
   if p.is_file()and p.name not in ('raw-host-context.json','runtime-stderr.txt'):
    assert p.read_bytes()==(b/p.relative_to(a)).read_bytes(),p
 for base in (moving,constructor):assert (base/'audit.json').read_bytes()==(base/'audit-repeat.json').read_bytes()
 asan=moving/'native/production-asan-current-headers-test.log'
 normal=moving/'native/production-normal-current-headers-test.log'
 assert '100% tests passed' in asan.read_text() and '100% tests passed'in normal.read_text()
 assert 'Ordinary fatal damage discarded the victim' in (moving/'native/old-clear-test.log').read_text()
 common=[ROOT/'tools/freeze_mp_spartan_death_physical.py']
 paths=common+[ROOT/p for p in (
  'tools/oracle_mp_spartan_death_physical_1_1.py','tools/audit_mp_spartan_death_physical_1_1.py',
  'docs/MP_SPARTAN_MOVING_DEATH_RESEARCH.md','tests/multiplayer_death_momentum_tests.cpp',
  'src/gameplay/simulation.cpp','include/halo2/simulation.hpp','include/halo2/multiplayer.hpp','include/halo2/content.hpp','CMakeLists.txt')]
 for folder in ('moving-effect-fixed','moving-effect-fixed-repeat','unfixed','native'):paths+=descendants(moving/folder)
 paths +=[moving/'audit.json',moving/'audit-repeat.json']
 freeze(moving/'frozen',paths,dict(
  original_report_sha256=sha(moving/'moving-effect-fixed/report.json'),
  moving_source_report_listing_files_identical=326,ordinary_fatal_velocity_preserved=True,
  native_change='Remove only ordinary Simulation::damage velocity clear; kill-trigger unchanged',
  normal_production_target_passed=True,asan_ubsan_production_target_passed=True,
  actual_new_linked_binaries_archived=True,old_clear_negative_control_rejected=True,
  full_live_device_fatal_admission_or_ten_body_ragdoll_or_continuous_solver_proven=False))
 paths=common+[ROOT/p for p in ('tools/oracle_mp_spartan_ragdoll_constructor_1_1.py',
  'tools/audit_mp_spartan_ragdoll_constructor_1_1.py','docs/MP_SPARTAN_RAGDOLL_CONSTRUCTOR_RESEARCH.md')]
 paths += descendants(constructor/'measured')+descendants(constructor/'repeat')
 paths +=[constructor/'audit.json',constructor/'audit-repeat.json',moving/'frozen/manifest.json']
 freeze(constructor/'frozen',paths,dict(
  original_report_sha256=sha(constructor/'measured/report.json'),
  original_report_listing_files_identical=153,source_body_count=10,source_constraint_count=9,
  complete_prior_moving_death_repeated_identically=True,new_supplied_services=0,
  explicit_original_roots=['1D4C50','1D3BB0','1D9330(component,0,0,1)'],
  natural_ragdoll_admission_or_world_insertion_or_solver_or_presentation_proven=False))
if __name__=='__main__':main()
