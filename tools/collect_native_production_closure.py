#!/usr/bin/env python3
"""Collect bounded configured-production evidence inside a run_test owner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tarfile

ROOT = Path(__file__).resolve().parents[1]
PEER_COMMIT = '6b1e2e620f1e37f0eafeee702eaea0043cb305fd'


def cmake_hashes(root):
    return {str(path.relative_to(root)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in [root / 'CMakeLists.txt', *sorted((root / 'cmake').rglob('*.cmake'))]}


def preparation_tool_hashes():
    return {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest() for path in (
        Path(__file__), ROOT / 'tools/audit_native_production_closure.py', ROOT / 'tools/native_inventory_fileapi.cmake',
        ROOT / 'tools/specialize_native_operands.py', ROOT / 'tools/split_native_translation_unit.py',
        ROOT / 'tools/instrument_native_execution.py', ROOT / 'tools/generated_routine_ownership.py',
        ROOT / 'tools/native_service_observations.py', ROOT / 'tools/native_source_prefixes.py')}


def prepare_peer_source(source, scratch):
    """Archive pinned commits; CMake may patch only these disposable copies."""
    source = source.resolve()
    subprocess.run(['git', '-C', str(source), 'cat-file', '-e', PEER_COMMIT + '^{commit}'], check=True)
    destination = scratch / 'datachannel'
    inputs = []
    for relative in ('', 'deps/libjuice', 'deps/plog', 'deps/usrsctp'):
        repository = source / relative
        commit = PEER_COMMIT
        if relative:
            line = subprocess.check_output(['git', '-C', str(source), 'ls-tree', PEER_COMMIT, relative], text=True)
            commit = line.split()[2]
        archive = scratch / ('dependency-' + str(len(inputs)) + '.tar')
        with archive.open('wb') as stream:
            subprocess.run(['git', '-C', str(repository), 'archive', commit], check=True, stdout=stream)
        target = destination / relative
        target.mkdir(parents=True, exist_ok=True)
        with tarfile.open(archive) as stream:
            stream.extractall(target, filter='data')
        inputs.append({'repository': str(repository), 'commit': commit,
            'archive_sha256': hashlib.sha256(archive.read_bytes()).hexdigest(), 'archive_bytes': archive.stat().st_size})
        archive.unlink()
    return destination, inputs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--peer-source', type=Path, default=ROOT / 'build-perf-4k60/_seed/halo2_datachannel-src')
    parser.add_argument('--cmake-arg', action='append', default=[])
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py to own configure/source-alias outputs')
    source, output = args.source.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    dependency, dependency_inputs = prepare_peer_source(args.peer_source, Path(os.environ['HALO2_TEST_SCRATCH']))
    before = cmake_hashes(source)
    tool_before = preparation_tool_hashes()
    context_path = Path(os.environ['HALO2_TEST_SCRATCH']) / 'collection-context.json'
    context_path.write_text(json.dumps({'cmake_source_hashes': before,
        'dependency_committed_inputs': dependency_inputs, 'dependency_working_trees_modified': False,
        'tool_hashes': tool_before}))
    command = [sys.executable, str(ROOT / 'tools/run_tests.py'), '--build-worker',
        '--source', str(source), '--output', str(output / 'cmake'), '--build-only',
        '--target', 'halo2_native_inventory_closure', '--jobs', '1',
        '--retain-report', 'native-production-closure.json',
        '--cmake-arg=-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=' + str(ROOT / 'tools/native_inventory_fileapi.cmake'),
        '--cmake-arg=-DHALO2_NATIVE_INVENTORY_PYTHON=' + sys.executable,
        '--cmake-arg=-DHALO2_NATIVE_INVENTORY_TOOL=' + str(ROOT / 'tools/audit_native_production_closure.py'),
        '--cmake-arg=-DHALO2_NATIVE_INVENTORY_COLLECTION_CONTEXT=' + str(context_path),
        '--cmake-arg=-DFETCHCONTENT_SOURCE_DIR_HALO2_DATACHANNEL=' + str(dependency),
        '--cmake-arg=-DFETCHCONTENT_FULLY_DISCONNECTED=ON']
    command.extend('--cmake-arg=' + value for value in args.cmake_arg)
    result = subprocess.run(command)
    after = cmake_hashes(source)
    tool_after = preparation_tool_hashes()
    changed = sorted(name for name in set(before) | set(after) if before.get(name) != after.get(name))
    tools_changed = sorted(name for name in set(tool_before) | set(tool_after) if tool_before.get(name) != tool_after.get(name))
    evidence = output / 'cmake/evidence/native-production-closure.json'
    report = {'schema': 'native-production-cmake-collection-v1', 'command': command,
        'exit_code': result.returncode, 'configured_closure_collected': result.returncode == 0 and not changed and not tools_changed and evidence.is_file(),
        'source_configuration_changed_during_collection': changed, 'cmake_source_hashes': after,
        'source_preparation_tools_changed_during_collection': tools_changed, 'preparation_tool_hashes_at_completion': tool_after,
        'evidence': str(evidence) if evidence.is_file() else None,
        'evidence_sha256': hashlib.sha256(evidence.read_bytes()).hexdigest() if evidence.is_file() else None,
        'owned_scratch': os.environ['HALO2_TEST_SCRATCH'], 'retained_large_artifacts': [],
        'dependency_committed_inputs': dependency_inputs, 'dependency_working_trees_modified': False,
        'game_compiled': False, 'runtime_execution_proved': False}
    (output / 'collection.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({key: report[key] for key in ['configured_closure_collected', 'source_configuration_changed_during_collection', 'source_preparation_tools_changed_during_collection', 'evidence']}))
    return result.returncode or (1 if not report['configured_closure_collected'] else 0)


if __name__ == '__main__':
    raise SystemExit(main())
