#!/usr/bin/env python3
"""Authenticate and apply only the qualified list-agent delta to its live owner."""
import difflib
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source=scratch/'source'
    base=ROOT/'build/test-results/vehicle-destruction-phantom-live-20261101'
    previous_path=base/'factory-capture-refresh.json'
    placement=json.loads((base/'factory-capture-placement/refresh.json').read_text())
    previous=json.loads(previous_path.read_text())
    assert Path(previous['scratch'])==scratch
    qualification_path=ROOT/'build/test-results/vehicle-destruction-list-contact-agent-20261101/v2/report.json'
    qualification=json.loads(qualification_path.read_text())
    assert qualification['passed'] and qualification['current_sources_unchanged']
    assert all(sha(ROOT/name)==pin for name,pin in qualification['source_hashes'].items())
    expected=dict(previous['patches'])
    expected['tests/destruction_runtime_tu15_tests.cpp']={'after':placement['fixture_refresh']['after']}
    for name,change in expected.items():
        assert sha(source/name)==change['after'],name
    assert sha(source/'CMakeLists.txt')==placement['registered_cmake_sha256']
    patches=dict(previous['patches'])
    patches['tests/destruction_runtime_tu15_tests.cpp']=dict(before=placement['fixture_refresh']['before'],after=placement['fixture_refresh']['after'])
    def install(name,content):
        path=source/name
        assert not path.is_symlink(),name
        before=path.read_text() if path.exists() else ''
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(content)
        patches[name]={'before':hashlib.sha256(before.encode()).hexdigest() if before else None,
                       'after':sha(path),'refreshed_mtime_ns':path.stat().st_mtime_ns,
                       'diff':'\n'.join(difflib.unified_diff(before.splitlines(),content.splitlines(),fromfile='before/'+name,tofile='after/'+name))}
    for name in ['include/halo2/native_tu15_list_contact_agent.hpp','src/physics/native_tu15_list_contact_agent.cpp',
                 'include/halo2/native_tu15_collection_filter.hpp','src/physics/native_tu15_collection_filter.cpp']:
        assert not (source/name).exists(),name
        install(name,(ROOT/name).read_text())
    world='src/physics/native_tu15_world_integration_residuals.cpp'
    frozen=(source/world).read_text();root=(ROOT/world).read_text()
    marker='      if(target.identity==0x2f8030u){'
    start=root.index(marker);end=root.index('      if(target.identity==0x2f1cc0u||',start)
    delta=root[start:end]
    includes='#include "halo2/native_tu15_list_contact_agent.hpp"\n#include "halo2/native_tu15_collection_filter.hpp"\n'
    anchor='#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"\n'
    assert frozen.count(anchor)==1
    frozen=frozen.replace(anchor,anchor+includes)
    anchor='    create.agents.create_agent=[&](auto target,auto first,auto second,auto settings,auto manager,auto&current){'
    assert frozen.count(anchor)==1
    frozen=frozen.replace(anchor,'    unsigned factory_depth{};\n'+anchor)
    anchor='      if(target.identity!=0x2e4eb0u)unqualified_pair_service('
    assert frozen.count(anchor)==1
    frozen=frozen.replace(anchor,delta+anchor)
    install(world,frozen)
    cmake='cmake/PhysicsLiveWorldTu15.cmake';text=(source/cmake).read_text()
    anchor='    src/physics/native_tu15_phantom_overlap_add.cpp\n'
    assert text.count(anchor)==1
    text=text.replace(anchor,anchor+'    src/physics/native_tu15_list_contact_agent.cpp\n    src/physics/native_tu15_collection_filter.cpp\n')
    install(cmake,text)
    report=dict(scratch=str(scratch),patches=patches,cmake_sha256=sha(source/'CMakeLists.txt'),
                previous_production_refresh=str(previous_path),
                qualification={'path':str(qualification_path),'sha256':sha(qualification_path),
                    'scope':'Actual native BEFORE and declared zero/negative/one/rejected variants; native ordinary collection branch; complete persistent memory and ordered parent operands/services'},
                original_qualified_dependency_closure={**previous['original_qualified_dependency_closure'],**qualification['source_hashes']},
                unrelated_root_world_changes_not_imported=True,
                unsupported=['BSP-special collection filter callbacks','unqualified nested factories'],retained_large_artifacts=[])
    (base/'list-contact-production-refresh.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Applied authenticated native list-agent delta; unrelated root wrapper/type additions remain outside frozen cohort')


if __name__=='__main__':main()
