#!/usr/bin/env python3
"""Compare every inventoried original model contract with actual native delivery.

Use tools/run_test.py. Borrow only authenticated frozen Cache archives; all
generated expectations, sources and the CMake build stay in this private owner.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
from refresh_destruction_current_cohort import sha

ROOT=Path(__file__).resolve().parents[1]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--contracts',required=True,type=Path)
    parser.add_argument('--cohort',required=True,type=Path)
    parser.add_argument('--production-refresh',required=True,type=Path)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source';source.mkdir()
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    census=json.loads(args.contracts.read_text())
    if not census['inventory_completed'] or census['schema']!='H2_DESTRUCTION_CONTRACTS_V1':raise ValueError('Require the authenticated original census')
    contracts={}
    for shard in census['contract_shards']:
        path=args.contracts.parent/shard['path']
        if sha(path)!=shard['sha256']:raise ValueError('Original contract shard changed')
        contracts.update(json.loads(path.read_text()))
    expected=dict(schema='H2_DESTRUCTION_NATIVE_MODEL_EXPECTATIONS_V1',maps=[],contracts={})
    reference_owners={}
    for owner in census['source_files']:
        for field,hash_field in (('path','sha256'),('mcc_path','mcc_sha256')):
            if not owner[field] or sha(owner[field])!=owner[hash_field]:raise ValueError('Original/MCC model owner changed')
            reference_owners[owner[field]]=owner[hash_field]
        name=Path(owner['path']).stem;entries=[]
        for definition,row in census['definitions'].items():
            matches=[key for key,maps in row['xbox'].items() if name in maps]
            if len(matches)>1:raise ValueError('Original map-local model contract is ambiguous')
            if matches:
                key=matches[0];expected['contracts'][key]=contracts[key]
                entries.append(dict(definition=definition,original_contract=key))
        expected['maps'].append(dict(map=name,mcc_path=owner['mcc_path'],entries=entries))
    (scratch/'expectations.json').write_text(json.dumps(expected,separators=(',',':'))+'\n')
    cohort=json.loads(args.cohort.read_text());production=json.loads(args.production_refresh.read_text())
    frozen=Path(cohort['scratch']).resolve(strict=True)
    if Path(production['scratch']).resolve()!=frozen:raise ValueError('Native Cache receipt belongs to another owner')
    manifest=json.loads((args.cohort.parent/'source/source-manifest.json').read_text())
    name='src/assets/mcc_cache.cpp';digest=production['patches'].get(name,{}).get('after',manifest['source_files_sha256'][name])
    if sha(frozen/'source'/name)!=digest:raise ValueError('Native Cache source changed')
    link=shlex.split((frozen/'build/CMakeFiles/mcc_gameplay_scalar_tests.dir/link.txt').read_text());libraries=[];library_hashes={}
    for item in link[link.index('-o')+2:]:
        if item.startswith('-'):raise ValueError('Unexpected native Cache link flag')
        path=Path(item) if Path(item).is_absolute() else frozen/'build'/item
        libraries.append(str(path));library_hashes[str(path)]=sha(path)
    headers=frozen/'source/include';files=[p for p in headers.rglob('*') if p.is_file()]
    if any(p.is_symlink() for p in files) or sum(p.stat().st_size for p in files)>16*1024*1024:raise ValueError('Audit headers exceed private budget')
    hashes={}
    for path in files:
        relative=Path('include')/path.relative_to(headers);target=source/relative;target.parent.mkdir(parents=True,exist_ok=True)
        before=sha(path);shutil.copyfile(path,target)
        if sha(path)!=before or sha(target)!=before:raise ValueError('Frozen audit header changed')
        hashes[str(relative)]=before
    relative='tools/audit_destruction_native_contracts.cpp';target=source/relative;target.parent.mkdir(parents=True,exist_ok=True)
    shutil.copyfile(ROOT/relative,target);hashes[relative]=sha(target)
    if any(any(c in x for c in '\n\r";$') for x in libraries):raise ValueError('Unexpected native library path syntax')
    cmake='''cmake_minimum_required(VERSION 3.20)
project(NativeDestructionContracts LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
find_package(nlohmann_json CONFIG REQUIRED)
enable_testing()
add_executable(audit tools/audit_destruction_native_contracts.cpp)
target_include_directories(audit PRIVATE include)
target_link_libraries(audit PRIVATE nlohmann_json::nlohmann_json)
'''
    cmake+='target_link_libraries(audit PRIVATE '+' '.join('"'+x+'"' for x in libraries)+')\n'
    cmake+=f'add_test(NAME native_model_contracts COMMAND audit "{scratch/"expectations.json"}" "{scratch/"build/native-model-contracts.json"}")\n'
    cmake+='set_tests_properties(native_model_contracts PROPERTIES TIMEOUT 600)\n';(source/'CMakeLists.txt').write_text(cmake)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(source),'--target','audit',
        '--filter','^native_model_contracts$','--jobs','2','--test-jobs','1','--output',str(args.output/'validation'),
        '--retain-report','native-model-contracts.json']
    report=dict(passed=False,owned_scratch=str(scratch),source_files_sha256=hashes,read_only_native_library_inputs=library_hashes,
        reference_owners=reference_owners,original_census_sha256=sha(args.contracts),expectations_sha256=sha(scratch/'expectations.json'),
        maps=len(expected['maps']),original_contracts=len(expected['contracts']),required_contexts=sum(len(r['entries']) for r in expected['maps']),
        command=command,tool_sha256=sha(__file__),retained_large_artifacts=[],full_goal_complete=False)
    def save():(args.output/'audit.json').write_text(json.dumps(report,indent=2)+'\n')
    save();code=subprocess.run(command,stdout=subprocess.DEVNULL,stderr=subprocess.STDOUT).returncode
    report.update(passed=code==0,returncode=code,native_library_inputs_unchanged=all(sha(p)==h for p,h in library_hashes.items()),
        reference_owners_unchanged=all(sha(p)==h for p,h in reference_owners.items()))
    report['passed']=report['passed'] and report['native_library_inputs_unchanged'] and report['reference_owners_unchanged'];save()
    print(json.dumps(dict(passed=report['passed'],report=str(args.output/'audit.json'))));return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
