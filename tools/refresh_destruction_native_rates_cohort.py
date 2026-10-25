#!/usr/bin/env python3
"""Admit the original-qualified rate kernels into an owned destruction cohort.

Run inside its finite tools/run_test.py session. Only exact native dispatch,
named PC-adapter and read-only diagnostic fixture deltas are admitted. Validate
the integrated native game and every registered destruction case through the
official CMake worker. Retain reports/hashes, never a second build or raw bank.
"""
import argparse
import difflib
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

from refresh_destruction_current_cohort import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('cohort','previous-refresh','production-refresh','qualification','fixture-patch','output'):
        parser.add_argument('--'+name,required=True,type=Path)
    parser.add_argument('--jobs',type=int,default=5)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = scratch/'source'
    cohort = json.loads(args.cohort.read_text())
    previous = json.loads(args.previous_refresh.read_text())
    production = json.loads(args.production_refresh.read_text())
    qualified = json.loads(args.qualification.read_text())
    pins = json.loads((args.qualification.parent/'source-hashes.json').read_text())
    if any(Path(row['scratch']).resolve()!=scratch for row in (cohort,previous,production)):
        raise ValueError('Native rate admission has a different source/build owner')
    if previous['prior_cohort']!=str(args.cohort.resolve()):
        raise ValueError('Previous fixture belongs to a different source cohort')
    if not qualified['passed'] or not qualified['frozen_source_unchanged'] or qualified['instruction_patches']:
        raise ValueError('Require the passing unchanged-original rate qualification')
    if qualified['native_expected_data_used']:
        raise ValueError('Native-generated expected data cannot qualify a replacement')
    args.output = args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    receipt = args.output/'admission.json'
    report = dict(passed=False,scratch=str(scratch),prior_cohort=str(args.cohort.resolve()),
                  previous_refresh=str(args.previous_refresh.resolve()),
                  inherited_production_refresh=str(args.production_refresh.resolve()),
                  qualification=dict(path=str(args.qualification.resolve()),sha256=sha(args.qualification),
                                     source_hashes_sha256=sha(args.qualification.parent/'source-hashes.json')),
                  fixture_patch_sha256=sha(args.fixture_patch),patches=dict(production['patches']),
                  tool_sha256=sha(__file__),full_goal_complete=False,retained_large_artifacts=[])
    def save():
        receipt.write_text(json.dumps(report,indent=2)+'\n')
    manifest = json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    expected = dict(manifest['source_files_sha256']);expected.update(cohort['additional_frozen_inputs'])
    expected.update({name:row['after'] for name,row in production['patches'].items()})
    expected['CMakeLists.txt'] = previous['registered_cmake_sha256']
    expected['tests/destruction_runtime_tu15_tests.cpp'] = previous['fixture_refresh']['after']
    if cohort.get('private_build_partition'):
        row = cohort['private_build_partition'];expected[row['path']] = row['after']
    report['frozen_source_drift_before_refresh'] = [name for name,digest in expected.items() if sha(source/name)!=digest]
    save()
    if report['frozen_source_drift_before_refresh']:
        raise ValueError('Frozen source changed before native rate admission')

    # Authenticate the complete local quoted-header dependency closure against
    # the unchanged-original component qualification and this frozen baseline.
    native = 'src/physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp'
    vehicle = 'src/physics/vehicles/physics_vehicle_runtime_generated.cpp'
    remap = 'src/physics/runtime/physics_instruction_remap_tu15.hpp'
    public = 'include/halo2/source_vehicle_native_rates.hpp'
    changed = {native,vehicle,remap,public}
    seen = set();pending = [vehicle,native,remap]
    while pending:
        name = pending.pop()
        if name in seen:continue
        seen.add(name)
        if sha(ROOT/name)!=pins.get(name):
            raise ValueError('Native rate dependency changed after original qualification: '+name)
        if name not in changed and sha(source/name)!=pins[name]:
            raise ValueError('Frozen native rate dependency differs from original qualification: '+name)
        for include in re.findall(r'^#include "([^"]+)"',(ROOT/name).read_text(),re.M):
            candidates = [ROOT/Path(name).parent/include,ROOT/'include'/include,ROOT/'src'/include]
            match = next((path for path in candidates if path.is_file()),None)
            if match:pending.append(str(match.relative_to(ROOT)))
    report['original_qualified_dependency_closure'] = {name:pins[name] for name in sorted(seen)}
    overlay = {name:(ROOT/name).read_text() for name in (native,remap,public)}
    def substitutions(name,replacements):
        text = (source/name).read_text()
        for before,after in replacements:
            if text.count(before)!=1:raise ValueError('Native admission seam changed: '+name)
            text = text.replace(before,after)
        if text!=(ROOT/name).read_text():
            raise ValueError('Workspace has additional unqualified native changes: '+name)
        overlay[name] = text
    substitutions(vehicle,[
        ('#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"',
         '#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"\n#include "physics/vehicles/source_vehicle_rate_native_tu15_internal.hpp"'),
        ('static void run_208d80(VehicleState &s) {\n',
         'static void run_208d80(VehicleState &s) {\nif(vehicle_rate_native_eligible(s,selected,false)){vehicle_rate_accelerate_native(s,selected);return;}\n'),
        ('static void run_208e70(VehicleState &s) {\n',
         'static void run_208e70(VehicleState &s) {\nif(vehicle_rate_native_eligible(s,selected,true)){vehicle_rate_approach_native(s,selected);return;}\n'),
        ('} // namespace\nvoid source_run_physics_vehicle_runtime(',
         '} // namespace\nSourceVehicleNativeRateActivity source_vehicle_native_rate_activity() {\n'
         '    return {rate_acceleration_calls.load(std::memory_order_relaxed),\n'
         '        rate_approach_calls.load(std::memory_order_relaxed),rate_cursor_calls.load(std::memory_order_relaxed)};\n'
         '}\nvoid source_run_physics_vehicle_runtime(')])
    substitutions('src/physics/physics_phantom_world_tu15_generated.cpp',[
        ('#include "halo2/physics_world_runtime.hpp"',
         '#include "halo2/physics_world_runtime.hpp"\n#include "physics/runtime/physics_instruction_remap_tu15.hpp"'),
        ('struct Binding { std::uint32_t key,value; };','using Binding=PhysicsInstructionRemapTu15::Binding;'),
        ('result.instruction=[&,bank=&sites](auto p){if(services.instruction)services.instruction(site(*bank,p));};',
         'result.instruction=PhysicsInstructionRemapTu15{&services,sites};')])
    for patch in json.loads(args.fixture_patch.read_text()):
        name = patch['path']
        if name not in ('tests/destruction_runtime_tu15_tests.cpp','tests/physics_multiplayer_acceptance.cpp'):
            raise ValueError('Native diagnostics changed an unexpected fixture')
        if sha(source/name)!=patch['before_sha256'] or sha(ROOT/name)!=patch['after_sha256']:
            raise ValueError('Native diagnostics fixture delta changed before admission')
        substitutions(name,[(row['old'],row['new']) for row in patch['replacements']])
    report['admitted_diffs'] = {}
    for name,text in overlay.items():
        path = source/name;before = sha(path) if path.exists() else None
        prior = path.read_text() if path.exists() else ''
        report['admitted_diffs'][name] = ''.join(difflib.unified_diff(prior.splitlines(True),text.splitlines(True),n=2))
        path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text)
        report['patches'][name] = dict(before=before,after=sha(path))
    cmake = source/'CMakeLists.txt'
    cmake.write_text(cmake.read_text()+'\nset_tests_properties(destruction_mcc_ghost PROPERTIES ENVIRONMENT "HALO2_DESTRUCTION_REQUIRE_NATIVE_RATES=1")\n')
    report['cmake_sha256'] = sha(cmake)
    cases = previous['registered_cases']
    report['removed_previous_case_outputs'] = {}
    for case in cases:
        path = scratch/'build'/f'destruction-{case}.json'
        if path.exists():
            report['removed_previous_case_outputs'][path.name] = sha(path)
            path.unlink()
    command = [sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),
               '--target','halo2_native','--target','physics_multiplayer_acceptance',
               '--target','destruction_runtime_tu15_tests','--filter','^destruction_mcc_',
               '--jobs',str(args.jobs),'--test-jobs','1','--output',str(args.output/'validation')]
    for case in cases:command += ['--retain-report',f'destruction-{case}.json']
    report['cases'] = cases;report['command'] = command;save()
    environment = dict(os.environ,HALO2_DESTRUCTION_CONTINUATION='1',HALO2_DESTRUCTION_FIXED_CLOCK='1',
                       HALO2_DESTRUCTION_PENDING_CONTINUATION='1')
    print(json.dumps(dict(status='validating',dependency_files=len(seen),report=str(receipt))),flush=True)
    code = subprocess.run(command,env=environment,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code);save()
    print(json.dumps(dict(passed=code==0,returncode=code,report=str(receipt))),flush=True)
    return code


if __name__=='__main__':
    raise SystemExit(main())
