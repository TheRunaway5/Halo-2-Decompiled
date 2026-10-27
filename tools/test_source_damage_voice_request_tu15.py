#!/usr/bin/env python3
"""Compile the production native damage-voice caller under owned test scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    source = args.source or root / "src/gameplay/units/source_damage_delivery_runtime_generated.cpp"
    text = source.read_text()
    start = text.index("Lcb77f: ")
    end = text.index("Lcb7c5: ", start)
    block = text[start:end]
    assert block.count("run_115fa0(s);") == 1
    caller = scratch / "source_damage_voice_caller.inc"
    caller.write_text("void extracted_damage_voice_caller(ObservedDamageState &s) {\n" + block + "Lcb7c5: return;\n}\n")
    binary = scratch / "source-damage-voice-request"
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-fno-fast-math",
               "-ffp-contract=off", "-frounding-math", "-I" + str(root / "include"),
               "-I" + str(root / "src"), "-DHALO2_DAMAGE_VOICE_CALLER=" + json.dumps(str(caller)),
               str(root / "tests/source_damage_voice_request_tu15_tests.cpp"), "-o", str(binary)]
    compiled = subprocess.run(command)
    result = compiled.returncode
    if not result:
        result = subprocess.run([str(binary)]).returncode
    report = {"passed": result == 0, "exit_code": result, "native_caller_sha256": hashlib.sha256(block.encode()).hexdigest(),
              "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
              "observer_header_sha256": hashlib.sha256((root / "include/halo2/source_damage_voice_request_tu15.hpp").read_bytes()).hexdigest(),
              "test_source_sha256": hashlib.sha256((root / "tests/source_damage_voice_request_tu15_tests.cpp").read_bytes()).hexdigest(),
              "scratch": str(scratch), "compile_command": command,
              "runtime_emulation": False, "declared_abi_inputs": True, "callee_returns_supplied": False,
              "scope": "extracted production native caller and typed publication; no live-world or audible-delivery claim"}
    if args.report_output:
        args.report_output.parent.mkdir(parents=True, exist_ok=True)
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    return result


if __name__ == "__main__":
    raise SystemExit(main())
