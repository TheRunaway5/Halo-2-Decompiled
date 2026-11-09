#!/usr/bin/env python3
"""Separate full typed304 mutation controls on an immutable source cohort.

Numeric controls require a complete native return and a retained DATA/FP
difference. Named refusals are reported separately and do not satisfy that
requirement. SourcePC-only and native-scope controls have distinct contracts.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

import validate_native_contact_candidates_bound_actual_tu15 as candidate
from native_contact_process_before_input_tu15 import NativeContactProcessBeforeInput, sha
from build_native_tu15_constraint_before_reference import build_constraint_reference, save
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
EXTRA_OWNERS = (0x304350, 0x303ee0, 0x303ba0, 0x303c70, 0x303d00, 0x303d80,
 0x2e4f30, 0x3038c0, 0x2e4fe0, 0x2f8630, 0x30e930, 0x30de60, 0x30d350,
 0x312620, 0x30d3f0, 0x314710, 0x313bb0, 0x32a660, 0x313560, 0x313390,
 0x313290, 0x2e5e20, 0x3070f0, 0x2e8d20, 0x2e4760, 0x30cf40, 0x1f6690,
 0x2e8d40, 0x3079c0, 0x3067d0)
SOLVER = 'src/physics/native_tu15_convex_simplex_solver.cpp'
CANDIDATES = 'src/physics/native_tu15_contact_candidates.cpp'
HELPER = 'include/halo2/native_tu15_contact_producer_binding.hpp'


def main():
 parser = argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--stage', type=Path, required=True)
 parser.add_argument('--output', type=Path, required=True)
 parser.add_argument('--compilers', nargs='+', default=['g++', 'clang++'])
 parser.add_argument('--modes', nargs='+', type=int, choices=range(4), default=list(range(4)))
 args = parser.parse_args()
 args.stage = args.stage.resolve()
 args.output = args.output.resolve()
 args.output.mkdir(parents=True, exist_ok=False)
 scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
 report = dict(passed=False, owned_scratch=str(scratch), retained_large_artifacts=[],
  supplied_original_outcomes=False, captured_gp_used=False)
 try:
  manifest = args.stage / 'source-pins.json'
  stage = json.loads(manifest.read_text())
  stage_pin = sha(manifest)
  stage_pins = {n: row['sha256'] for n, row in stage['files'].items()}
  assert all(sha(args.stage/'staged'/n) == h for n, h in stage_pins.items())
  tools = [Path(__file__), ROOT/'tools/observe_native_contact_producer_mutations_tu15.cpp',
   ROOT/'tools/observe_native_contact_producer_bound_ordered_tu15.cpp',
   ROOT/'tools/observe_native_contact_candidates_bound_actual_tu15.cpp',
   ROOT/'tools/validate_native_contact_candidates_bound_actual_tu15.py',
   ROOT/'tools/native_contact_process_before_input_tu15.py',
   ROOT/'tools/build_native_tu15_constraint_before_reference.py',
   ROOT/'tools/run_tests.py', ROOT/'tools/run_test.py']
  tool_pins = {str(p.relative_to(ROOT)): sha(p) for p in tools}
  before = NativeContactProcessBeforeInput(candidate.ARCHIVE, candidate.ARCHIVE_PIN,
   scratch/'before', 0x304350)
  reference = build_constraint_reference(before, scratch/'reference')
  assert sha(NEW_DEFAULT) == NEW_PIN
  image = Xbe(NEW_DEFAULT)
  candidate.OWNERS = (*candidate.OWNERS, *EXTRA_OWNERS)
  meta = candidate.metadata(image)
  meta.update(input=before.input, regions=before.regions, modes=args.modes)
  proof_path = ROOT/'analysis/physics/native-mopp-aabb-code-dispatch-20261102-fourth/report.json'
  assert sha(proof_path) == '04b72ff2f5e830d6cad907ab2d20680af52f2d8d839c3ab54ce8cf86f37179ad'
  proof = json.loads(proof_path.read_text())
  assert proof['passed']
  for pc, table, width, entries in ((0x30ea30,0x30f70c,1,25), (0x30ea37,0x30f6e0,4,11), (0x313bf5,0x3146f8,4,5)):
   raw = image.read(table, width*entries)
   proof['immutable_dispatch_CODE_tables'].append(dict(address=table, bytes=len(raw), hex=raw.hex(), sha256=hashlib.sha256(raw).hexdigest()))
   proof['dispatch_fetches'].append(dict(instruction=pc, table=table, bytes=width, entries=entries))
  for table in proof['immutable_dispatch_CODE_tables']:
   assert before.read(table['address'], table['bytes']).hex() == table['hex']
  meta['immutable_dispatch_CODE'] = proof
  save(scratch/'metadata.json', meta)
  project = scratch/'project'
  shutil.copytree(args.stage/'staged', project)
  prefix = tools[3].read_text().split('J activity(')[0] + '}\n'
  parent = tools[2].read_text().split('int main(int argc,char**argv)')[0]
  driver = prefix + parent + tools[1].read_text()
  (project/'driver.cpp').write_text(driver)
  variants = {
   'cache_count': (SOLVER,
    'narrow_store(memory,0x30f68fu,input.support_cache+8u,m.word(input.work,1u),1u);',
    'narrow_store(memory,0x30f68fu,input.support_cache+8u,m.word(input.work,1u)^1u,1u);'),
   'candidate_minimum': (CANDIDATES,
    'm.vector(0x2f37e3,self+0x20u,minimum);',
    'm.vector(0x2f37e3,self+0x20u,maximum);'),
   'wrong_store_pc': (SOLVER,
    'narrow_store(memory,0x30f68fu,input.support_cache+8u,',
    'narrow_store(memory,0x30f690u,input.support_cache+8u,'),
   'disabled_scope': (HELPER,
    'NativeTu15PairDeletionNativeScopeGuard all_native(scope,&stats.providers.native_only);',
    '(void)scope;')}
  mutation_pins = {}
  for name, (unit, old, new) in variants.items():
   content = (project/unit).read_text()
   assert content.count(old) == 1, (name, content.count(old))
   if name == 'disabled_scope':
    path = project/'scope-include/halo2'/Path(unit).name
   else:
    path = project/'mutants'/name/Path(unit).name
   path.parent.mkdir(parents=True, exist_ok=True)
   path.write_text(content.replace(old, new))
   mutation_pins[name] = dict(source=unit, before=old, after=new, sha256=sha(path), file=str(path.relative_to(project)))
  cmake = '''cmake_minimum_required(VERSION 3.20)
project(typed304Mutations LANGUAGES CXX)
include(CTest)
find_package(nlohmann_json REQUIRED)
'''
  common = [n for n in stage['units'] if n not in (SOLVER, CANDIDATES)]
  cmake += 'add_library(common OBJECT '+' '.join(common)+')\n'
  cmake += f'add_library(solver OBJECT {SOLVER})\nadd_library(candidates OBJECT {CANDIDATES})\nadd_library(driver OBJECT driver.cpp)\nadd_library(scope_driver OBJECT driver.cpp)\n'
  cmake += 'target_include_directories(scope_driver BEFORE PRIVATE scope-include)\n'
  targets = ['common','solver','candidates','driver','scope_driver']
  for name in ('baseline', *variants):
   pieces = ['$<TARGET_OBJECTS:common>', '$<TARGET_OBJECTS:scope_driver>' if name == 'disabled_scope' else '$<TARGET_OBJECTS:driver>']
   pieces += [mutation_pins[name]['file'] if name in ('cache_count','wrong_store_pc') else '$<TARGET_OBJECTS:solver>']
   pieces += [mutation_pins[name]['file'] if name == 'candidate_minimum' else '$<TARGET_OBJECTS:candidates>']
   cmake += f'add_executable({name} '+' '.join(pieces)+')\n'
   cmake += f'add_test(NAME typed304_{name} COMMAND {name} "{reference["elf"]}" "{scratch}/metadata.json" "${{CMAKE_BINARY_DIR}}/evidence/{name}.json" {name})\n'
   targets.append(name)
  for target in targets:
   cmake += f'target_include_directories({target} PRIVATE include src/physics)\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
  for target in ('baseline',*variants):
   cmake += f'target_link_options({target} PRIVATE -Wl,--gc-sections)\n'
  cmake += 'file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/evidence")\n'
  (project/'CMakeLists.txt').write_text(cmake)
  compiled_pins = {str(p.relative_to(project)):sha(p) for p in project.rglob('*') if p.is_file()}
  results = []
  for compiler in args.compilers:
   output = args.output/compiler
   command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
    '--scratch-root',str(scratch/'builds'),'--output',str(output),
    *sum((['--target',n] for n in ('baseline',*variants)),[]),
    '--filter','^typed304_', '--cmake-arg=-DCMAKE_CXX_COMPILER='+compiler,
    '--jobs','2','--test-jobs','1',
    *sum((['--retain-report','evidence/'+n+'.json'] for n in ('baseline',*variants)),[])]
   run = subprocess.run(command)
   result = dict(compiler=compiler,exit_code=run.returncode,command=command)
   result['evidence'] = {}
   for name in ('baseline',*variants):
    path = output/'evidence/evidence'/f'{name}.json'
    if path.exists():
     payload = json.loads(path.read_text())
     result['evidence'][name] = dict(path=str(path),sha256=sha(path),passed=payload.get('passed'),error=payload.get('error'))
   results.append(result)
   report['results'] = results
   save(args.output/'report.json',report)
  assert all(r['exit_code']==0 for r in results), 'Full typed304 mutation controls differ; named refusals are not numeric rejection proof'
  assert all(sha(project/n)==h for n,h in compiled_pins.items())
  assert all(sha(ROOT/n)==h for n,h in tool_pins.items())
  assert sha(manifest)==stage_pin and all(sha(args.stage/'staged'/n)==h for n,h in stage_pins.items())
  report.update(passed=True,stage_manifest_sha256=stage_pin,stage_source_pins=stage_pins,
   compiled_source_pins=compiled_pins,tool_pins=tool_pins,source_inputs_unchanged=True,
   original_before=before.receipt,original_image_sha256=NEW_PIN,
   original_listing_sha256=meta['source_listing_sha256'],mutations=mutation_pins,
   modes=args.modes,scope=__doc__)
 except BaseException as error:
  report['error']=str(error) or repr(error)
  raise
 finally:
  report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*') if p.is_file())
  save(args.output/'report.json',report)


if __name__=='__main__':
 main()
