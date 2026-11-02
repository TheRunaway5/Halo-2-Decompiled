#!/usr/bin/env python3
"""Admit qualified BC720 into its still-owned destruction source/build only."""
import difflib
import hashlib
import json
import os
from pathlib import Path
from native_region_transition_kernel_tu15 import install_region_transition_kernel

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    base=ROOT/'build/test-results/vehicle-destruction-phantom-live-20261101'
    prior_path=base/'region-transition-capture-refresh.json'
    report=json.loads(prior_path.read_text())
    prior=json.loads((base/'region-transition-capture-state-build/refresh.json').read_text())
    assert Path(report['scratch'])==scratch
    qualification_base=ROOT/'build/test-results/vehicle-destruction-region-transition-20261101'
    qualifications=[qualification_base/name/'report.json'for name in ('gcc-matrix-v2','clang-matrix','power-actual')]
    native=['include/halo2/native_tu15_region_transition.hpp',
        'src/gameplay/objects/source_region_transition_native_tu15_internal.hpp']
    verified={};qualification_receipts=[]
    for path in qualifications:
        q=json.loads(path.read_text());assert q['passed'] and q['scratch_work_removed'],str(path)
        for name in native:assert sha(ROOT/name)==q['source_hashes'][name],name
        for name,digest in q['compiled_source_hashes'].items():
            if name in native or name=='tools/observe_native_region_transition_tu15.cpp':continue
            assert sha(source/name)==digest,name
            assert name not in verified or verified[name]==digest,name
            verified[name]=digest
        qualification_receipts.append(dict(path=str(path),sha256=sha(path)))
    manifest=json.loads((base/'cohort/source/source-manifest.json').read_text())
    expected=dict(manifest['source_files_sha256']);expected.update({name:row['after']for name,row in report['patches'].items()})
    expected['CMakeLists.txt']=prior['registered_cmake_sha256']
    expected['tests/destruction_runtime_tu15_tests.cpp']=prior['fixture_refresh']['after']
    changes={name:(ROOT/name).read_text()for name in native+['tools/native_region_transition_kernel_tu15.py']}
    module='src/gameplay/units/death_cycle_runtime_tu15_generated.cpp'
    changes[module]=install_region_transition_kernel((source/module).read_text())
    generator='tools/generate_death_cycle_runtime_tu15.py'
    before=(source/generator).read_text()
    old='from native_region_transition_kernel_tu15 import install_region_transition_observation\n    source=install_region_transition_observation(source)'
    new='from native_region_transition_kernel_tu15 import install_region_transition_kernel\n    source=install_region_transition_kernel(source)'
    if old in before:
        assert before.count(old)==1
        changes[generator]=before.replace(old,new)
    else:
        anchor='    args.output.write_text(source)\n'
        assert before.count(anchor)==1
        changes[generator]=before.replace(anchor,'    '+new+'\n'+anchor)
    for name in changes:
        path=source/name;assert not path.is_symlink() and path.resolve().is_relative_to(source),name
        if name in expected:assert sha(path)==expected[name],name
        else:assert not path.exists(),name
    patches=report['patches']
    for name,content in changes.items():
        path=source/name;before=path.read_text()if path.exists()else ''
        path.parent.mkdir(parents=True,exist_ok=True);path.write_text(content)
        patches[name]=dict(before=hashlib.sha256(before.encode()).hexdigest()if before else None,
            after=sha(path),refreshed_mtime_ns=path.stat().st_mtime_ns,
            diff='\n'.join(difflib.unified_diff(before.splitlines(),content.splitlines(),fromfile='before/'+name,tofile='after/'+name)))
    fixture='tests/destruction_runtime_tu15_tests.cpp'
    assert sha(source/fixture)==prior['fixture_refresh']['after']
    patches[fixture]=dict(before=report['patches'][fixture]['after'],after=sha(source/fixture),
        authenticated_existing_fixture=True,source_fixture_refresh=str(base/'region-transition-capture-state-build/refresh.json'))
    report.update(previous_production_refresh=str(prior_path),cmake_sha256=sha(source/'CMakeLists.txt'),
        qualification=dict(path=str(qualifications[0]),sha256=sha(qualifications[0]),
            scope='BC720: GCC and Clang actual Warthog plus 11 complete component returns and 3 explicit type-branch prefixes each, all83 parent sites; actual power-module full return with Clang; all current callees execute original CPU',
            reports=qualification_receipts),verified_frozen_transitive_headers=verified,
        unrelated_root_world_changes_not_imported=True)
    report['original_qualified_dependency_closure'].update({name:sha(ROOT/name)for name in native})
    target=base/'region-transition-production-refresh.json';target.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(applied=True,verified_frozen_headers=len(verified),receipt=str(target))))


if __name__=='__main__':main()
