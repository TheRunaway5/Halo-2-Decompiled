#!/usr/bin/env python3
"""Validate the campaign query module in a small, disposable source cohort.

Uses the production CMake module and storage wrappers. This avoids rebuilding
unrelated engine modules; it does not certify a complete client or co-op game.
Invoke through tools/run_test.py; only bounded runner logs and input hashes
survive in --output.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    'cmake/CampaignPlayerQueries.cmake', 'cmake/SnapshotStorage.cmake',
    'cmake/TestStorage.cmake', 'tools/run_test.py',
    'src/campaign/source_campaign_player_queries_tu15.cpp', 'src/campaign/scripting/source_script_lists.cpp',
    'src/core/datum_pool.cpp', 'src/gameplay/objects/raw_datum_pool.cpp', 'src/gameplay/objects/raw_datum_pool_identity.cpp',
    'src/gameplay/objects/raw_datum_pool_startup.cpp', 'src/runtime/source_runtime_memory.cpp',
    'src/assets/cache_file_snapshot.cpp', 'src/runtime/source_runtime_snapshot_bytes.cpp',
    'tests/campaign_player_queries_tu15_tests.cpp',
    'tests/fixtures/campaign_player_queries_tu15.json', 'src/gameplay/simulation.cpp',
)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--sanitizers', action='store_true')
    args = ap.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    project = scratch/'campaign-player-queries'
    project.mkdir()
    args.output.mkdir(parents=True, exist_ok=True)
    sources = [ROOT/p for p in FILES]
    sources += sorted(p for p in (ROOT/'include').rglob('*') if p.is_file())
    sources += sorted((ROOT/'src').rglob('*.hpp'))
    sources += sorted((ROOT/'src').rglob('*.inc'))
    hashes = {}
    for source in sources:
        relative = source.relative_to(ROOT)
        data = source.read_bytes()
        target = project/relative
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        hashes[str(relative)] = hashlib.sha256(data).hexdigest()
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.24)
project(CampaignPlayerQueries LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SDL2 REQUIRED CONFIG)
include(CTest)
include(cmake/TestStorage.cmake)
include(cmake/SnapshotStorage.cmake)
add_library(halo2_systems STATIC)
target_include_directories(halo2_systems PRIVATE include)
include(cmake/CampaignPlayerQueries.cmake)
add_library(campaign_query_simulation_compile OBJECT src/gameplay/simulation.cpp)
target_include_directories(campaign_query_simulation_compile PRIVATE include src)
target_link_libraries(campaign_query_simulation_compile PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
''')
    command = [sys.executable, str(ROOT/'tools/run_tests.py'),
        '--source', str(project), '--scratch-root', str(scratch/'builds'),
        '--output', str(args.output), '--target', 'campaign_player_queries_tu15_tests',
        '--filter', '^original_campaign_player_queries_tu15$', '--jobs', '2']
    if args.sanitizers:
        command.append('--sanitizers')
    else:
        command += ['--target', 'campaign_query_simulation_compile']
    status = subprocess.run(command).returncode
    changed = [name for name, digest in hashes.items()
               if hashlib.sha256((project/name).read_bytes()).hexdigest() != digest]
    drift = [name for name, digest in hashes.items()
             if not (ROOT/name).exists() or hashlib.sha256((ROOT/name).read_bytes()).hexdigest() != digest]
    report = dict(scope=__doc__, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        inputs_sha256=hashes, command=command,
        snapshot_changed=changed, checkout_drift=drift,
        passed=status == 0 and not changed, complete_client_validated=False,
        live_campaign_or_coop_validated=False)
    (args.output/'source-cohort.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    return status or bool(changed)


if __name__ == '__main__':
    raise SystemExit(main())
