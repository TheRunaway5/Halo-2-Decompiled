#!/usr/bin/env python3
"""Remove declared test registrations/observers from a qualified owned snapshot.

Every restored byte must match the canonical source frozen before diagnostics.
This prepares the ordinary production build; launcher qualification is separate.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re

from weapon_test_cache import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('prepared','cohort','refresh','validation','output'):
        parser.add_argument('--'+name,type=Path,required=True)
    args=parser.parse_args();owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=owner/'source';prepared=json.loads(args.prepared.read_text())
    cohort=json.loads(args.cohort.read_text());refresh=json.loads(args.refresh.read_text())
    validation=json.loads(args.validation.read_text())
    if (Path(prepared['runner_scratch'])!=owner or Path(cohort['owner'])!=owner or
            Path(refresh['owner'])!=owner or source.is_symlink()):
        raise ValueError('Delivery requires the same authenticated private source owner')
    journal=ROOT/'analysis/vehicles-tu15-goal-20261023/session-v1/session.json'
    for command in json.loads(journal.read_text())['commands']:
        if 'finished' not in command and not any(
                '\n' not in arg and Path(arg).name==Path(__file__).name for arg in command['argv']):
            raise ValueError('Another source/build command is still running')
    expected=dict(prepared['registered_snapshot_source_files_sha256'])
    for change in cohort['changes']:
        if expected[change['path']]!=change['before']:raise ValueError('Cohort preimage differs')
        expected[change['path']]=change['after']
    expected.update({'data/'+name:pin for name,pin in cohort['private_data_sha256'].items()})
    expected.update(refresh['refreshed_source_sha256'])
    if any(sha(source/name)!=pin for name,pin in expected.items()):
        raise ValueError('Frozen source changed after native qualification')
    binding=validation.get('vehicle_goal_source_binding',{})
    routes=binding.get('native_model_lookup_routes',{}).get('routes',[])
    if ({row.get('route') for row in routes}!={'delivery','direct','vitality','live_support'} or
            len(routes)!=4 or any(not row.get('native_route_installed') or
                not row.get('complete_authenticated_literal_body_preserved') or
                sha(source/row['path'])!=row['sha256'] for row in routes)):
        raise ValueError('Delivery requires all four authenticated TU1.5 native model lookup routes')
    receipts={str(path.resolve()):sha(path) for path in (args.prepared,args.cohort,args.refresh)}
    digest=hashlib.sha256(json.dumps(expected,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    if (binding.get('source_receipts_sha256')!=receipts or
            binding.get('frozen_source_manifest_sha256')!=digest or
            binding.get('source_unchanged_after_run') is not True or
            binding.get('declared_case_count_passed') is not True or
            binding.get('ownership_tests')!=['native_tu15_world_memory_ownership','native_tu15_physics_branch_ownership',
                'original_tu15_contrail_snapshot_inputs','original_tu15_light_fade_curves']):
        raise ValueError('Delivery needs complete validation bound to these exact current-source receipts')
    count=len(refresh['registered_cases'])+len(cohort['destruction_cases'])+17
    tests=(args.validation.parent/'tests.log').read_text()
    completed=re.search(r'^100% tests passed(?:, 0 tests failed)? out of (\d+)\s*$',tests,re.MULTILINE)
    if not validation['passed'] or not completed or int(completed[1])!=count:
        raise ValueError('The complete declared current-source native cohort must pass first')
    origin=Path(prepared['original_preparation_report']).parent
    manifest=json.loads((origin/'source-manifest.json').read_text())['source_files_sha256']
    observation=json.loads((origin/'vehicle-projectile-birth-retirement-observation.json').read_text())
    replacements={};restored_native_hashes={}
    # Preparation only appends CMake registrations. Locate the authenticated
    # canonical prefix by its original hash, without reading a drifting checkout.
    cmake=(source/'CMakeLists.txt').read_bytes();digest=hashlib.sha256();end=0
    for line in cmake.splitlines(keepends=True):
        digest.update(line);end+=len(line)
        if digest.hexdigest()==manifest['CMakeLists.txt']:
            replacements['CMakeLists.txt']=cmake[:end];break
    if 'CMakeLists.txt' not in replacements:raise ValueError('Canonical CMake prefix is unavailable')
    for row in observation['private_generated_owner_instrumentation']:
        name=row['path'];text=(source/name).read_text()
        include='#include "halo2/source_physics_entry_observation.hpp"\n'
        if not text.startswith(include):raise ValueError('Diagnostic include prefix changed')
        text=text[len(include):]
        for entry in row['entries']:
            needle=f'\nSourcePhysicsEntryObservation diagnostic(s.memory,0x{int(entry,16):x}u,[&]{{return s.save();}});'
            if text.count(needle)!=1:raise ValueError('Diagnostic observer changed: '+entry)
            text=text.replace(needle,'',1)
        data=text.encode()
        if row['original_sha256']!=manifest[name]:
            raise ValueError('Diagnostic source preimage differs from the canonical freeze: '+name)
        restored_pin=hashlib.sha256(data).hexdigest()
        if restored_pin!=row['original_sha256']:
            # This owner also received the independently qualified native
            # lookup route. Remove only the declared diagnostic hooks; keep
            # that production route, and prove every other byte is canonical.
            if name!='src/physics/physics_live_support_tu15_generated.cpp' or name not in refresh['refreshed_source_sha256']:
                raise ValueError('Restored native owner differs from canonical original: '+name)
            installer=source/'tools/native_damage_model_kernel_tu15.py'
            if sha(installer)!=binding['native_model_lookup_routes']['installer_sha256']:
                raise ValueError('Qualified native lookup installer changed')
            spec=importlib.util.spec_from_file_location('delivery_native_model_routes',installer)
            module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
            if (module.install_actual_model_lookup(text)!=text or
                    hashlib.sha256(module.strip_actual_model_lookup(text).encode()).hexdigest()!=row['original_sha256']):
                raise ValueError('Native route restoration changes another canonical body: '+name)
            restored_native_hashes[name]=restored_pin
        replacements[name]=data
    production=lambda name:name=='CMakeLists.txt' or name.startswith(('src/','include/','cmake/','shaders/','rust/'))
    canonical=dict(manifest)
    canonical.update(restored_native_hashes)
    # Later declared native fixes retain their authenticated canonical source;
    # only the two private diagnostic transformations above are undone.
    canonical.update({name:pin for name,pin in refresh['refreshed_source_sha256'].items()
                      if production(name) and name not in replacements})
    for name,pin in canonical.items():
        if production(name):
            actual=hashlib.sha256(replacements[name]).hexdigest() if name in replacements else sha(source/name)
            if actual!=pin:raise ValueError('Production source differs from the coherent canonical freeze: '+name)
    args.output.mkdir(parents=True,exist_ok=False);changes=[]
    for name,data in replacements.items():
        path=source/name;before=sha(path);path.write_bytes(data)
        changes.append(dict(path=name,before=before,after=sha(path)))
    if any(sha(source/name)!=pin for name,pin in canonical.items() if production(name)):
        raise ValueError('Canonical production source changed during restoration')
    report=dict(status='prepared',passed=False,validation_not_run=True,owner=str(owner),
        qualified_cohort_cases=count,qualified_validation_sha256=sha(args.validation),
        canonical_manifest_sha256=sha(origin/'source-manifest.json'),changes=changes,
        original_instruction_changes=[],production_source_matches_canonical_freeze=True,
        retained_qualified_native_routes=restored_native_hashes,
        launcher_runtime_rebuilt=False,full_vehicle_parity=False,tool_sha256=sha(Path(__file__)))
    (args.output/'prepared.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(status='prepared',restored_files=len(changes),report=str(args.output/'prepared.json'))),flush=True)


if __name__=='__main__':main()
