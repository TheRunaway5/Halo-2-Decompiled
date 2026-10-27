#!/usr/bin/env python3
"""Bounded native-only selector proof; run through tools/run_test.py.

Compiles the production replica selector and the actual complete253070 native
kernel. The independent original190460 fixture supplies the distance oracle.
No source World, emulator, admission result, driver result or voice is created.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--report-output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise RuntimeError("Run through tools/run_test.py")
    test_path = ROOT / "tests/source_object_audio_views_tu15_tests.cpp"
    test = test_path.read_text()
    # Keep the exact production test's assertions/fixture traversal. The
    # bounded closure runs only its new replica lane; the CMake target still
    # runs all three lanes. The direct native leaf needs no other manager.
    old = "for(unsigned lane=0;lane<3;++lane)"
    if test.count(old) != 1:
        raise RuntimeError("Native replica lane loop changed")
    test = test.replace(old, "for(unsigned lane=2;lane<3;++lane)").replace("results[0].size()", "results[2].size()")
    old = "for(const auto &lane:results)need("
    if test.count(old) != 1:
        raise RuntimeError("Native replica fixture assertion changed")
    test = test.replace(old, "for(const auto &lane:std::array{results[2]})need(")
    test, removed = re.subn(r"source_run_object_loop_audio_tu15\(memory,0x190460,[^;]+;",
                           'throw std::runtime_error("Original source lane outside native-only closure");', test)
    if removed != 1:
        raise RuntimeError("Original source lane boundary changed")
    old = "native_tu15_sound_manager_run(memory,0x253070,abi,{0x27f,0x1f80});"
    if test.count(old) != 1:
        raise RuntimeError("Complete native253070 comparison call changed")
    test = test.replace(old, "native_tu15_sound::cluster_scale_entry(memory,abi,{0x27f,0x1f80},{},{});")
    # These unchanged four checked scalar readers avoid compiling the rest of
    # the asset loader into this small component proof.
    cache_path = ROOT / "src/assets/cache.cpp"
    cache = cache_path.read_text()
    first = cache.index("void Bytes::require(")
    last = cache.index("std::string Bytes::string(", first)
    preamble = '#include "audio/native_tu15_sound_kernels_listener.cpp"\n'
    preamble += "namespace halo2 {\n" + cache[first:last] + "\n}\n"
    source = scratch / "native-loop-replica.cpp"
    source.write_text(preamble + test)
    binary = scratch / "native-loop-replica"
    command = ["c++", "-std=c++20", "-O2", "-ffunction-sections", "-fdata-sections",
               "-Wl,--gc-sections", "-I" + str(ROOT / "include"), "-I" + str(ROOT / "src"),
               "-I" + str(ROOT / "tests"), "-I/usr/include/SDL2", str(source),
               str(ROOT / "src/audio/source_object_loop_emitter_tu15.cpp"), "-lcrypto", "-o", str(binary)]
    compiled = subprocess.run(command, capture_output=True, text=True)
    fixture = ROOT / "tests/fixtures/object_audio_views_tu15.json"
    result = None
    if compiled.returncode == 0:
        result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True)
    paths = [test_path, cache_path, fixture, ROOT / "src/audio/source_object_loop_emitter_tu15.cpp",
             ROOT / "src/audio/native_tu15_sound_kernels_listener.cpp"]
    report = {"passed": compiled.returncode == 0 and result.returncode == 0,
              "scope": "Compiled native253070/all8192 scale inputs plus independent original190460 fixture; no live/client/speaker claim",
              "original_distance_cases": json.loads(fixture.read_text())["cases"],
              "source_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
              "scratch": str(scratch), "retained_large_artifacts": [], "command": command,
              "compile_returncode": compiled.returncode, "compile_stderr": compiled.stderr[-6000:]}
    if result:
        report.update(returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("passed", "compile_returncode", "returncode", "stdout", "stderr") if k in report}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
