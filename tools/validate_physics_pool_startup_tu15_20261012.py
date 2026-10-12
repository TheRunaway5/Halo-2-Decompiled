#!/usr/bin/env python3
"""Seal source ownership, independent reproductions and native test results."""
import hashlib
import json
import re
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
# A frozen copy still locates the actual workspace evidence, not a new tree.
if ROOT.name in ('source-v1','source-v1a'):ROOT=ROOT.parents[4]
E=ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012'
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def same_directories(a,b):
    left={str(p.relative_to(a)):sha(p)for p in a.rglob('*')if p.is_file()}
    right={str(p.relative_to(b)):sha(p)for p in b.rglob('*')if p.is_file()}
    assert left==right,(a,b)
    return dict(files=len(left),sha256=hashlib.sha256(json.dumps(left,sort_keys=True).encode()).hexdigest())
def main():
    manifest=json.loads((E/'source-manifest-v1a.json').read_text())
    source=E/'source-v1a'
    parent=json.loads((E/'source-manifest-v1.json').read_text())
    assert manifest['parent_manifest_sha256']==sha(E/'source-manifest-v1.json')
    for p,pin in parent['files'].items():
        assert sha(E/'source-v1'/p)==pin,p
        if p not in manifest['metadata_only_overlays']:assert manifest['files'][p]==pin,p
    for p,pin in manifest['files'].items():assert sha(source/p)==pin,p
    for p in manifest['owned']:assert sha(ROOT/p)==manifest['files'][p],p
    assert (ROOT/'CMakeLists.txt').read_text().count('include(cmake/PhysicsPoolStartup.cmake)')==1
    repeats={name:same_directories(E/left,E/right)for name,left,right in (
        ('correspondence','correspondence-v1','correspondence-repeat'),
        ('original','original-v4','original-v4-repeat'))}
    assert sha(ROOT/'src/physics_pool_startup_runtime_generated.cpp')==sha(E/'generated-repeat/runtime.cpp')
    assert sha(E/'generated-v1/manifest.json')==sha(E/'generated-repeat/manifest.json')
    fixture=json.loads((E/'original-v4/fixture.json').read_text())
    correspondence=json.loads((E/'correspondence-v1/complete-report.json').read_text())
    generated=json.loads((E/'generated-v1/manifest.json').read_text())
    assert fixture['tool_sha256']==sha(source/'tools/oracle_physics_pool_startup_tu15_20261012.py')
    assert fixture['correspondence_sha256']==sha(E/'correspondence-v1/complete-report.json')
    assert correspondence['tool_sha256']==sha(source/'tools/research_physics_pool_startup_tu15_20261012.py')
    assert generated['generator_sha256']==sha(source/'tools/generate_physics_pool_startup_tu15_20261012.py')
    assert not fixture['original_instruction_patches'] and not fixture['substituted_game_calls']
    reached=set(fixture['original_instruction_counts'])
    assert all(f'{p:08x}'in reached for p in correspondence['entries'].values())
    assert correspondence['complete_bodies']==15 and len(generated['lowered_instructions'])==561
    logs={};binaries={}
    for kind in ('release','asan'):
        for phase in ('configure','build','test'):
            path=E/f'{kind}-{phase}.log';logs[path.name]=sha(path)
        text=(E/f'{kind}-test.log').read_text()
        assert re.search(r'100% tests passed(?:, 0 tests failed)? out of 1',text) and 'Skipped' not in text,text
        assert '3552 fresh TU1.5 pool lifecycle calls, 122451 ordered writes, 27 actual host allocation/release requests and 258048 final retained bytes match' in text
        binaries[kind]=sha(E/f'build-{kind}/physics_pool_startup_tests')
    report=dict(schema=1,status='passed',source_manifest_sha256=sha(E/'source-manifest-v1a.json'),
        native_build_source_manifest_sha256=sha(E/'source-manifest-v1.json'),
        metadata_only_overlays=manifest['metadata_only_overlays'],
        fixture_sha256=sha(E/'original-v4/fixture.json'),correspondence_sha256=sha(E/'correspondence-v1/complete-report.json'),
        generation_manifest_sha256=sha(E/'generated-v1/manifest.json'),
        independently_identical_reproductions=repeats,source_bodies=15,native_instruction_sites=561,
        dynamically_reached_source_sites=fixture['unique_original_instructions'],
        calls=sum(len(s['steps'])for s in fixture['sequences']),
        stores=sum(len(r['stores'])for s in fixture['sequences']for r in s['steps']),
        selected_tests=dict(release=1,asan_ubsan_leak=1),logs=logs,binaries=binaries,
        current_shared_world_runtime_modified=False,live_physics_integration=False,goal_complete=False,
        scope=manifest['scope'])
    path=E/'validation.json';path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(status=report['status'],sha256=sha(path),calls=report['calls'],stores=report['stores'])))
if __name__=='__main__':main()
