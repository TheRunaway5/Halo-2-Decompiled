#!/usr/bin/env python3
"""Expand a retained strict native-frame mismatch using unchanged TU1.5 replay.

All extracted banks, replay binaries and raw outputs belong to run_test.py.
Only bounded diagnostics and source/input hashes survive. No compared byte,
original instruction or physics result is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    value = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--archive-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(diagnostic_completed=False, owned_scratch=str(scratch),
                  retained_large_artifacts=[], original_or_operands_modified=False)
    sources = [Path(__file__), ROOT / "tools/replay_live_phantom_hardware_tu15.py",
               ROOT / "tools/live_physics_hardware_return_tu15.cpp"]
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sources}
    try:
        assert sha(args.archive) == args.archive_sha256, "Reference archive changed"
        names = subprocess.run(["tar", "--zstd", "-tf", str(args.archive.resolve())],
                               check=True, capture_output=True, text=True).stdout.splitlines()
        assert names and all(not Path(n).is_absolute() and ".." not in Path(n).parts for n in names)
        inputs = scratch / "inputs"
        inputs.mkdir()
        subprocess.run(["tar", "--zstd", "-xf", str(args.archive.resolve()),
                        "--no-same-owner", "-C", str(inputs)], check=True)
        captures = list(inputs.rglob("capture.json"))
        assert len(captures) == 1, "Require exactly one authenticated capture"
        capture = captures[0].parent
        data = json.loads(captures[0].read_text())
        assert data["entry"] == 0x1cc8d0 and data["reference"] == "English TU1.5 only"
        replay = scratch / "replay"
        command = [sys.executable, str(sources[1]), "--native-frame-semantics",
                   "--return-breakpoints", "--capture", str(capture), "--output", str(replay)]
        result = subprocess.run(command, check=False, timeout=180, capture_output=True, text=True)
        (args.output / "replay.log").write_text((result.stdout + result.stderr)[-65536:])
        original = json.loads((replay / "report.json").read_text())
        (args.output / "original-report.json").write_text(json.dumps(original, indent=2) + "\n")
        memory = [d for d in original["differences"] if d["field"] == "memory"]
        banks = []
        with (replay / "output.bin").open("rb") as stream:
            stream.seek(276)  # Actual i386 ABI/FP output prefix in the qualified replay.
            for region in sorted(data["regions"], key=lambda row: row["address"]):
                size, address = region["size"], region["address"]
                actual = stream.read(size)
                assert len(actual) == size
                if not any(address <= row["address"] < address + size for row in memory):
                    continue
                expected_path = capture / region.get("after_file", region["before_file"])
                expected = expected_path.read_bytes()
                before = (capture / region["before_file"]).read_bytes()
                assert len(expected) == len(before) == size
                positions = [n for n, (a, b) in enumerate(zip(actual, expected)) if a != b
                             and not 0x600e0000 <= address + n < 0x600f0000]
                runs = []
                for position in positions:
                    if runs and runs[-1][1] == position:
                        runs[-1][1] += 1
                    else:
                        runs.append([position, position + 1])
                groups = []
                for first, end in runs[:256]:
                    sample_end = min(end, first + 256)
                    groups.append(dict(address=hex(address + first), bytes=end-first,
                                       original=actual[first:sample_end].hex(),
                                       native=expected[first:sample_end].hex(),
                                       before=before[first:sample_end].hex(),
                                       hex_truncated=sample_end != end))
                banks.append(dict(first=hex(address), bytes=size, differing_bytes=len(positions),
                                  differing_runs=len(runs), runs=groups,
                                  runs_truncated=len(runs) > 256,
                                  original_sha256=hashlib.sha256(actual).hexdigest(),
                                  native_sha256=sha(expected_path)))
        assert all(sha(ROOT / path) == digest for path, digest in pins.items())
        report.update(diagnostic_completed=True, physics_parity_passed=original["passed"],
                      reference_replay_exit=result.returncode, original_differences=original["differences"],
                      compared_memory_bytes=original["compared_memory_bytes"], banks=banks,
                      differing_bytes=sum(bank["differing_bytes"] for bank in banks),
                      archive=str(args.archive.resolve()), archive_sha256=args.archive_sha256,
                      source_hashes=pins, command=command,
                      scope="Strict mismatch expansion only; no source-writer attribution or byte waiver")
    finally:
        report["temporary_allocated_bytes_before_cleanup"] = sum(
            path.stat().st_blocks * 512 for path in scratch.rglob("*") if path.is_file())
        (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report[key] for key in
                      ("diagnostic_completed", "physics_parity_passed", "differing_bytes")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
