#!/usr/bin/env python3
"""Validate semantic platform operations without claiming an original kernel ABI."""
import json
import re
from freeze_physics_platform_locks_20261011 import ROOT, BASE, OWNED, APPEND, PARENT_PIN, sha

E = ROOT / 'analysis/multiplayer/sandbox-source/physics-platform-lock-runtime-20261011'
FIXTURE_PIN = '2ac25d909493a09c05b5c20391cd5021dec16355acb1b2e07a9c8382ac500975'


def main():
    parent_path = BASE / 'source-manifest-v11.json'
    assert sha(parent_path) == PARENT_PIN
    parent = json.loads(parent_path.read_text())
    manifest_path = BASE / 'source-manifest-v12.json'
    manifest = json.loads(manifest_path.read_text())
    assert manifest['parent_manifest_sha256'] == PARENT_PIN
    assert manifest['owned_overlays'] == list(OWNED)
    for relative, pin in parent['files'].items():
        assert sha(BASE / 'source-v11' / relative) == pin, relative
        if relative not in OWNED and relative != 'CMakeLists.txt':
            assert manifest['files'][relative] == pin, relative
    for relative, pin in manifest['files'].items():
        assert sha(BASE / 'source-v12' / relative) == pin, relative
    for relative in OWNED:
        assert sha(ROOT / relative) == manifest['files'][relative], relative
    assert (BASE / 'source-v12/CMakeLists.txt').read_text() == (BASE / 'source-v11/CMakeLists.txt').read_text() + APPEND
    fixture_path = E / 'compatibility-verified/fixture.json'
    assert sha(fixture_path) == FIXTURE_PIN == sha(E / 'compatibility-repeat/fixture.json')
    fixture = json.loads(fixture_path.read_text())
    assert fixture['tool_sha256'] == sha(ROOT / 'tools/oracle_physics_platform_locks_20261011.py')
    assert fixture['real_distinct_host_threads_verified'] and len(set(fixture['opaque_equality_tokens'])) == 2
    assert not any(fixture[k] for k in ('original_kernel_execution', 'original_kernel_register_clobbers_proven', 'wait_wake_results_supplied', 'live_gameplay_proof'))
    rows = fixture['cases']
    assert len(rows) == 1595 and sum(r['boundary'] == 'wait' for r in rows) == sum(r['boundary'] == 'wake' for r in rows) == 9
    assert all(len(bytes.fromhex(r['before_hex'])) == len(bytes.fromhex(r['after_hex'])) == 28 for r in rows)
    assert {r['ordinal'] for r in rows} == {277, 291, 294}
    boundary_path = E / 'boundary-v1/report.json'
    assert sha(boundary_path) == fixture['actual_boundary_sha256'] == 'ddbf22c6cf09518f34f55e4f0a1628e9a9804a0ec62bc12b312c984ef24973f7'
    boundary = json.loads(boundary_path.read_text())
    assert boundary['tool_sha256'] == sha(ROOT / 'tools/research_physics_platform_lock_boundary_20261011.py')
    assert rows[0]['address'] == boundary['actual_argument_pointer'] and rows[0]['before_hex'] == boundary['actual_argument28_hex']
    predecessor = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-bootstrap-runtime-20261011/validation.json'
    assert sha(predecessor) == '0c6983353f0939acb05162958c9107d6fc2c76dba012b19c5af167c36ea3e728'
    logs, binaries = {}, {}
    for kind, directory in (('production', 'build-v12'), ('asan', 'asan-v12')):
        for stage in ('configure', 'build', 'rebuild', 'test'):
            path = E / f'{kind}-{stage}.log'
            logs[path.name] = sha(path)
        output = (E / f'{kind}-test.log').read_text()
        assert '100% tests passed out of 1' in output and 'Skipped' not in output
        assert re.search(r'\bnative_physics_platform_locks\s+\.+\s+Passed', output)
        binary = BASE / directory / 'physics_platform_locks_tests'
        binaries[kind] = sha(binary)
        build = (BASE / directory / 'build.ninja').read_text()
        if kind == 'asan':
            block = build.split('CMakeFiles/halo2_physics_platform_locks.dir/src/physics_platform_locks.cpp.o:')[1].split('\n\n')[0]
            assert '-fsanitize=address,undefined' in block
    result = dict(schema=1, validator_sha256=sha(__file__), source_manifest_sha256=sha(manifest_path),
        unchanged_initialization_manifest_sha256=PARENT_PIN, unchanged_initialization_validation_sha256=sha(predecessor),
        fixture_sha256=FIXTURE_PIN, identical_independent_fixture_repeat=True,
        actual_original_pre_import_input_sha256=sha(boundary_path), compatibility_transitions=1595,
        missing_wait_boundaries=9, missing_wake_boundaries=9, native_component_checks_passed=1,
        sanitizer_component_checks_passed=1, logs=logs, binaries=binaries,
        original_kernel_execution=False, original_kernel_abi_proven=False,
        native_x86_callee_adapter_implemented=False, original_world_initialization_completed=False,
        live_match_integrated=False, whole_physics_engine_complete=False, published_launcher_replaced=False,
        scope='Native semantic platform operations independently match pinned compatibility field transitions. Supplied real native event handoff is a separate contract test; no original kernel wait lists, scheduling, register clobbers or successful original kernel result are provided.')
    path = E / 'validation.json'
    path.write_text(json.dumps(result, sort_keys=True, indent=2) + '\n')
    print(json.dumps(dict(transitions=1595, native_checks=1, sanitizer_checks=1, validation_sha256=sha(path))))


if __name__ == '__main__':
    main()
