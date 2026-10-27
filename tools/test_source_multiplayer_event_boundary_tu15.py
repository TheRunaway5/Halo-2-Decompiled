#!/usr/bin/env python3
"""Compile complete native Slayer activation/B1E80 callers under owned scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--report-output", type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    if not scratch.is_absolute() or not scratch.is_dir():
        raise RuntimeError("Run through tools/run_test.py")
    path = args.source or root / "src/physics/physics_live_support_tu15_generated.cpp"
    source = path.read_text()
    first = source.index("std::uint32_t sign_extend(")
    last = source.index("\nstd::uint32_t selected(", first)
    bodies = [source[first:last]]
    for entry in ("b1e80", "2c5d70"):
        first = source.index("static void run_" + entry + "(WorldState &s) {")
        last = source.index("\nstatic void ", first + 1)
        bodies.append(source[first:last])
    caller = scratch / "native-announcer-activation.inc"
    caller.write_text("\n".join(bodies))
    binary = scratch / "native-announcer-activation"
    test = root / "tests/source_multiplayer_event_boundary_tu15_tests.cpp"
    command = ["c++", "-std=c++20", "-O1", "-fno-fast-math", "-ffp-contract=off", "-frounding-math",
               "-I" + str(root / "include"), "-I" + str(root / "src"),
               "-DHALO2_MULTIPLAYER_EVENT_CALLER=" + json.dumps(str(caller)), str(test), "-o", str(binary)]
    compiled = subprocess.run(command, capture_output=True, text=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True) if compiled.returncode == 0 else None
    report = {"passed": compiled.returncode == 0 and result.returncode == 0,
              "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
              "native_callers_sha256": hashlib.sha256(caller.read_bytes()).hexdigest(),
              "test_sha256": hashlib.sha256(test.read_bytes()).hexdigest(), "scratch": str(scratch),
              "compile_command": command, "compile_returncode": compiled.returncode, "compile_stderr": compiled.stderr,
              "runtime_emulation": False, "callee_returns_supplied": False,
              "scope": "Complete compiled2C5D70/B1E80, actual native event CALL boundary; live journal/PCM acceptance remains separate"}
    if result:
        report.update(returncode=result.returncode, stdout=result.stdout, stderr=result.stderr)
    args.report_output.parent.mkdir(parents=True, exist_ok=True)
    args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("passed", "compile_returncode", "returncode", "stdout", "stderr", "compile_stderr") if k in report}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
