"""Validate literal environment/illumination metadata with owned CMake builds.

The selected runtime and frozen GPU cohort remain unchanged. This is complete
paired bank and malformed-input proof, not live material or GPU acceptance.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preparation", type=Path, required=True)
    parser.add_argument("--base-cache", type=Path, required=True)
    parser.add_argument("--authority", type=Path, required=True)
    parser.add_argument("--compiler", action="append", choices=["g++", "clang++"])
    parser.add_argument("--sanitizers", action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    assert not args.output.resolve().is_relative_to(owner)
    preparation = json.loads(args.preparation.read_text())
    source = Path(preparation["source"]).resolve(strict=True)
    from build_launcher_engine import source_hashes
    inventory = source_hashes(source)
    assert hashlib.sha256(json.dumps(inventory, sort_keys=True).encode()).hexdigest() == preparation["production_inventory_sha256"]
    selectors = {name: (root / name).read_bytes() for name in ("gameplay-runtime-selection.json", "frontend-runtime-selection.json")}
    runtime = root / preparation["selected_runtime"]
    assert digest(runtime) == preparation["runtime_sha256"]
    args.output.mkdir(parents=True, exist_ok=True)
    tool_names = ["tools/export_mcc_native_env_illum_three_channel_fixture.py", "tools/export_mcc_native_world_animated_fixture.py",
        "tools/export_mcc_native_world_cooked_fixture.py", "tools/mcc_env_illum_projection.py",
        "tools/generate_mcc_native_env_illum_three_channel.py", "tools/data/mcc-native-env-illum-three-channel-profiles.json"]
    tool_pins = {str(root / name): digest(root / name) for name in tool_names}
    fixture = owner / "fixture.json"
    command = [sys.executable, "-B", str(root / "tools/export_mcc_native_env_illum_three_channel_fixture.py"),
        "--repo-root", str(root), "--prepared-root", str(args.base_cache), "--authority", str(args.authority), "--output", str(fixture)]
    subprocess.run(command, check=True)
    project = owner / "project"
    base_files = ["src/assets/cache.cpp", "cmake/TestStorage.cmake", "tools/run_test.py"]
    base_files += [p.relative_to(source).as_posix() for p in (source / "include").rglob("*") if p.is_file()]
    overlay_files = ["tests/mcc_native_env_illum_three_channel_tests.cpp", "src/rendering/mcc_native_env_illum_three_channel.cpp",
        "src/rendering/mcc_native_env_illum_three_channel_profiles.inc", "include/halo2/mcc_native_env_illum_three_channel.hpp"]
    pins = {}
    for origin, paths in ((source, base_files), (root, overlay_files)):
        for name in paths:
            target = project / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(origin / name, target)
            pins[str(origin / name)] = digest(origin / name)
    (project / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.25)
project(MccEnvironmentIllumination LANGUAGES CXX)
include(CTest)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
include(cmake/TestStorage.cmake)
add_executable(env_illum_tests tests/mcc_native_env_illum_three_channel_tests.cpp
 src/rendering/mcc_native_env_illum_three_channel.cpp src/assets/cache.cpp)
target_include_directories(env_illum_tests PRIVATE include src)
target_compile_features(env_illum_tests PRIVATE cxx_std_20)
target_compile_definitions(env_illum_tests PRIVATE HALO2_CACHE_BYTES_ONLY=1)
target_link_libraries(env_illum_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
if(HALO2_SANITIZERS)
 target_compile_options(env_illum_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
 target_link_options(env_illum_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME mcc_native_env_illum_three_channel COMMAND env_illum_tests
 "${ENV_ILLUM_FIXTURE}" "${CMAKE_CURRENT_BINARY_DIR}/env-illum.json")
set_tests_properties(mcc_native_env_illum_three_channel PROPERTIES TIMEOUT 180)
''')
    results = []
    matrix = [(c, args.sanitizers) for c in args.compiler] if args.compiler else [("g++", False), ("clang++", False), ("clang++", True)]
    for compiler, sanitizers in matrix:
        name = compiler + ("-sanitizers" if sanitizers else "")
        command = [sys.executable, "-B", str(root / "tools/run_tests.py"), "--source", str(project),
            "--scratch-root", str(owner / "builds"), "--output", str(args.output / name), "--target", "env_illum_tests",
            "--filter", "^mcc_native_env_illum_three_channel$", "--jobs", "2", "--test-jobs", "1",
            "--cmake-arg=-DCMAKE_CXX_COMPILER=" + compiler, "--cmake-arg=-DENV_ILLUM_FIXTURE=" + str(fixture),
            "--retain-report", "env-illum.json", "--retain-report", "Testing/Temporary/LastTest.log"]
        if sanitizers:
            command.append("--sanitizers")
        completed = subprocess.run(command)
        results.append(dict(compiler=compiler, sanitizers=sanitizers, exit_code=completed.returncode))
        if completed.returncode:
            break
    assert source_hashes(source) == inventory
    assert all(digest(Path(path)) == pin for path, pin in pins.items())
    assert all(digest(Path(path)) == pin for path, pin in tool_pins.items())
    assert all((root / name).read_bytes() == raw for name, raw in selectors.items())
    assert digest(runtime) == preparation["runtime_sha256"]
    report = dict(scope=__doc__, passed=all(x["exit_code"] == 0 for x in results), results=results,
        source_pins=pins, tool_pins=tool_pins, helper_sha256=digest(Path(__file__)), literal_fixture_bytes=fixture.stat().st_size,
        literal_fixture_sha256=digest(fixture), authority_sha256=digest(args.authority),
        frozen_source_inventory_sha256=preparation["production_inventory_sha256"],
        selected_runtime_unchanged=True, source_cohort_unchanged=True, source_overlays_applied=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
