#!/usr/bin/env python3
"""Compile actual World/caller objects from a frozen owned include closure.

This is compilation evidence, not a linked client or World execution. All
local transitive quoted headers and the Boost.PFR header owner are frozen.
"""
import argparse,hashlib,json,os,re,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SOURCES=['src/physics/physics_live_world_tu15.cpp',
    'src/campaign/source_campaign_screen_runtime_tu15.cpp','src/runtime/source_execution_calls.cpp',
    'src/campaign/ai/native_campaign_ai_path_distance_tu15_binding.cpp',
    'src/campaign/ai/native_campaign_ai_path_distance_tu15.cpp',
    'src/campaign/ai/native_campaign_ai_point_geometry_tu15.cpp']


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']);project=owner/'project';project.mkdir()
    pending=[*SOURCES,*[str(path.relative_to(ROOT))for path in(ROOT/'third_party/boost-pfr/include').rglob('*')if path.is_file()]]
    inputs={}
    while pending:
        name=pending.pop()
        if name in inputs:continue
        original=ROOT/name;data=original.read_bytes();inputs[name]=hashlib.sha256(data).hexdigest()
        target=project/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"',data.decode(errors='replace'),re.M):
            for base in(original.parent,ROOT/'include',ROOT/'src',ROOT/'third_party/boost-pfr/include'):
                candidate=base/include
                if candidate.is_file():pending.append(str(candidate.relative_to(ROOT)));break
    (project/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(NativeDistanceWorldCallers LANGUAGES CXX)
include(CTest)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
add_library(native_distance_world_callers OBJECT '''+' '.join(SOURCES)+''')
target_compile_features(native_distance_world_callers PRIVATE cxx_std_23)
target_compile_options(native_distance_world_callers PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
target_include_directories(native_distance_world_callers PRIVATE include src third_party/boost-pfr/include)
target_link_libraries(native_distance_world_callers PRIVATE SDL2::SDL2 Vulkan::Vulkan)
''')
    args.output=args.output.resolve();args.output.mkdir(parents=True,exist_ok=False)
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),
        '--scratch-root',str(owner/'builds'),'--output',str(args.output/'cmake'),
        '--jobs','2','--test-jobs','1','--target','native_distance_world_callers','--build-only',
        '--cmake-arg=-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','--retain-report','compile_commands.json']
    result=subprocess.run(command);unchanged=all(sha(ROOT/name)==pin for name,pin in inputs.items())
    report=dict(passed=result.returncode==0 and unchanged,caller_compile_only=True,validation_not_run=True,
        source_sha256=inputs,source_unchanged=unchanged,command=command,exit_code=result.returncode,
        owned_scratch=str(owner),retained_large_artifacts=[],actual_world_route_qualified=False,
        linked_client_qualified=False,natural_armory_frontier_advanced=False)
    (args.output/'validation.json').write_text(json.dumps(report,indent=2)+'\n');return int(not report['passed'])


if __name__=='__main__':raise SystemExit(main())
