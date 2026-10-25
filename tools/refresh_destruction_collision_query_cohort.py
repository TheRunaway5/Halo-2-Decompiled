#!/usr/bin/env python3
"""Admit the two original collision owners exposed by actual Flood Lab input.

Use the active tools/run_test.py finite session. Preserve all32 earlier bodies,
authenticate the missing-owner regression and rebuild through the CMake worker.
This is complete compiled original code; semantic-kernel proof remains separate.
"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def bodies(path):
    return {m.group(1):m.group(0) for m in re.finditer(
        r'^static void run_([0-9a-f]+)\(WorldState &s\) \{.*?^\}',path.read_text(),re.M|re.S)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--previous-refresh',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--jobs',type=int,default=8)
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use the active finite test owner')
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source = scratch/'source'
    cohort = json.loads(args.cohort.read_text());prior = json.loads(args.previous_refresh.read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,prior)):
        raise ValueError('Collision refresh has another source/build owner')
    validation = ROOT/'analysis/destruction-goal-20261022/flood-native-live-v2/validation'
    worker = json.loads((validation/'report.json').read_text())
    signature = 'TU1.5 live world owner missing at 2f7038 -> 304c90 return 2f703f'
    if not all(s['exit_code']==0 for s in worker['steps'][:2]) or signature not in (validation/'tests.log').read_text():
        raise ValueError('Missing direct original collision-owner regression')
    args.output = args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    receipt = args.output/'admission.json'
    report = dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),
        previous_refresh=str(args.previous_refresh.resolve()),patches=dict(prior['patches']),
        tool_sha256=sha(__file__),scope='Complete compiled original query owners and bounded native regressions; full and semantic-kernel gates remain open',
        retained_large_artifacts=[])
    def save():receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest = json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({p:change['after'] for p,change in prior['patches'].items()})
    expected['CMakeLists.txt'] = prior['registered_cmake_sha256']
    if cohort.get('private_build_partition'):
        part = cohort['private_build_partition'];expected[part['path']] = part['after']
    report['frozen_source_drift_before_refresh'] = [p for p,h in expected.items() if sha(source/p)!=h];save()
    if report['frozen_source_drift_before_refresh']:raise ValueError('Frozen source changed before query admission')
    relative = 'src/physics/collision/source_collision_query_tu15_generated.cpp'
    old,new = bodies(source/relative),bodies(ROOT/relative)
    if len(new)!=34 or any(old[k]!=new[k] for k in old) or not (
        (len(old)==32 and set(new)-set(old)=={'304c90','2f4de0'}) or (len(old)==34 and old==new)):
        raise ValueError('Earlier complete query bodies changed')
    generation_path = ROOT/'analysis/destruction-goal-20261022/flood-query-generation-v3/generation.json'
    generation = json.loads(generation_path.read_text())
    if generation['generated_cpp_sha256']!=sha(ROOT/relative):raise ValueError('Query generation output changed')
    report['original_generation'] = dict(path=str(generation_path),sha256=sha(generation_path),
        all_prior_32_bodies_byte_identical=True,new_entries=['304c90','2f4de0'])
    header='tests/helpers/live_physics_entry_capture_tu15.hpp'
    before_header=(source/header).read_text();after_header=before_header
    replacements=[
        ('  bool active_{},complete_{};','  bool active_{},complete_{};\n  bool retain_entry_record_{};'),
        ('  explicit LivePhysicsEntryCaptureTu15(std::filesystem::path path):directory_(std::move(path)){}',
         '  explicit LivePhysicsEntryCaptureTu15(std::filesystem::path path,bool retain_entry_record=false)\n      :directory_(std::move(path)),retain_entry_record_(retain_entry_record){}'),
        ('      entry_=entry;active_=true;',
         '      if(retain_entry_record_) {\n        std::ofstream out(directory_/"capture-entry.json");out<<record_.dump(2)<<\'\\n\';\n        require(bool(out),"Cannot retain actual original physics entry ABI");\n      }\n      entry_=entry;active_=true;')]
    if 'bool retain_entry_record_' not in before_header:
        for old_text,new_text in replacements:
            if after_header.count(old_text)!=1:raise ValueError('Entry diagnostic header seam changed')
            after_header=after_header.replace(old_text,new_text)
    if after_header!=(ROOT/header).read_text():raise ValueError('Entry diagnostic header has unrelated changes')
    for name in (relative,'tools/generate_collision_query_tu15.py','tests/destruction_runtime_tu15_tests.cpp',header):
        target = source/name;before = sha(target);target.write_bytes((ROOT/name).read_bytes())
        report['patches'][name] = dict(before=before,after=sha(target))
    report['fixture_refresh'] = dict(report['patches']['tests/destruction_runtime_tu15_tests.cpp'],path='tests/destruction_runtime_tu15_tests.cpp')
    report['registered_cmake_sha256'] = report['cmake_sha256'] = sha(source/'CMakeLists.txt')
    report['registered_cases'] = prior['registered_cases'];report['cases'] = ['ghost','campaign_scorpion_cannon','campaign_flood_clump']
    for case in report['cases']:(scratch/'build'/f'destruction-{case}.json').unlink(missing_ok=True)
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
        '--target','destruction_runtime_tu15_tests','--target','halo2_native','--target','physics_multiplayer_acceptance',
        '--filter','^destruction_mcc_(ghost|campaign_scorpion_cannon|campaign_flood_clump)$',
        '--jobs',str(args.jobs),'--test-jobs','1','--output',str(args.output/'validation')]
    for case in report['cases']:command += ['--retain-report',f'destruction-{case}.json']
    report['command'] = command;save();print(json.dumps(dict(status='validating',report=str(receipt))),flush=True)
    environment = dict(os.environ,HALO2_DESTRUCTION_FIXED_CLOCK='1',HALO2_DESTRUCTION_CONTINUATION='1')
    code = subprocess.run(command,env=environment,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save()
    print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True);return code


if __name__=='__main__':raise SystemExit(main())
