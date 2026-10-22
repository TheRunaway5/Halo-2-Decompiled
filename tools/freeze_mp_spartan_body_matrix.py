#!/usr/bin/env python3
"""Freeze only the new source/current-body-matrix consumer experiment."""
import json
from pathlib import Path
import freeze_mp_spartan_death_physical as archive
ROOT=archive.ROOT
BASE=ROOT/'analysis/multiplayer/integration/spartan-body-matrix-20261010'
def main():
 assert (BASE/'audit.json').read_bytes()==(BASE/'audit-repeat.json').read_bytes()
 assert (BASE/'fixture.txt').read_bytes()==(BASE/'fixture-repeat.txt').read_bytes()
 assert (BASE/'fixture.json').read_bytes()==(BASE/'fixture-repeat.json').read_bytes()
 for p in (BASE/'measured').rglob('*'):
  if p.is_file()and p.name not in ('raw-host-context.json','runtime-stderr.txt'):
   assert p.read_bytes()==(BASE/'repeat'/p.relative_to(BASE/'measured')).read_bytes(),p
 for name in ('normal-test-final.log','asan-test-final.log'):
  assert '170 logical stores / 1160 ordered source bytes' in (BASE/'native'/name).read_text()
 assert archive.sha(ROOT/'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/frozen/manifest.json')=='c8a78b096ce8f8a22c487fdab55da339c4d3cf09f5c492fffe26fe22cbf8f3a7'
 paths=[ROOT/p for p in ('tools/freeze_mp_spartan_body_matrix.py','tools/oracle_mp_spartan_body_matrix_1_1.py',
  'tools/export_mp_spartan_body_matrix_1_1.py','tools/audit_mp_spartan_body_matrix_1_1.py',
  'include/halo2/physics_body_matrix.hpp','src/physics/physics_body_matrix.cpp','tests/physics_body_matrix_tests.cpp',
  'docs/MP_SPARTAN_BODY_MATRIX_RESEARCH.md',
  'analysis/multiplayer/integration/spartan-ragdoll-constructor-20261010/frozen/manifest.json')]
 for p in BASE.rglob('*'):
  if p.is_file()and 'frozen'not in p.relative_to(BASE).parts:paths.append(p)
 archive.freeze(BASE/'frozen',paths,dict(
  report_sha256=archive.sha(BASE/'measured/report.json'),source_complete_calls=10,
  independently_repeated_source_report_and_listings=True,source_reads=270,source_raw_stores=220,
  native_full_spans_equal=True,native_logical_stores=170,native_ordered_store_bytes=1160,
  normal_and_actual_asan_ubsan_module_tests_passed=True,new_supplied_services=0,
  explicit_unmeasured_bit6_correction_required=True,
  world_solver_natural_ragdoll_admission_or_render_proven=False,
  shared_runtime_headers_render_CMake_or_release_edits_from_leaf=False))
if __name__=='__main__':main()
