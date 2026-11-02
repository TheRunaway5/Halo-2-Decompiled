#!/usr/bin/env python3
"""Owned, frozen-header contextual compile of the isolated304 providers."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage", type=Path, required=True)
    parser.add_argument("--private-stage", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"])
    include = scratch / "include"
    shutil.copytree(ROOT / "include", include)
    overlay = args.private_stage / "staged/include"
    shutil.copytree(overlay, include, dirs_exist_ok=True)
    header = args.stage / "native_contact_producer_native_providers.hpp"
    shutil.copyfile(header, include / "halo2/native_contact_producer_native_providers.hpp")
    source = scratch / "provider.cpp"
    source.write_text('#include "halo2/native_contact_producer_native_providers.hpp"\nint main() { return 0; }\n')
    results = []
    for compiler in ("g++", "clang++"):
        command = [compiler, "-std=c++20", "-O0", "-fno-fast-math", "-ffp-contract=off",
                   "-fsyntax-only", "-I" + str(include), str(source)]
        run = subprocess.run(command, cwd=scratch, text=True, capture_output=True)
        results.append({"compiler": compiler, "command": command,
                        "passed": run.returncode == 0, "returncode": run.returncode,
                        "diagnostics": (run.stdout + run.stderr)[-24000:]})
    report = {"schema": "native-contact-producer-provider-context-v1",
              "passed": all(x["passed"] for x in results),
              "owner": str(scratch), "skeleton_sha256": sha(header),
              "frozen_headers": {str(p.relative_to(include)): sha(p)
                                 for p in sorted(include.rglob("*.hpp"))},
              "source_sha256": sha(source), "compilers": results,
              "scope": "Frozen full-header contextual compile only; not physical/native/live admission."}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"passed": report["passed"], "report": str(args.report),
                      "diagnostics": [x["diagnostics"] for x in results]}))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
