#!/usr/bin/env python3
"""Replay frozen typed controller values; original executable is never loaded."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--probe", type=Path, required=True)
parser.add_argument("--vectors", type=Path, required=True)
parser.add_argument("--report", type=Path, required=True)
args = parser.parse_args()
compressed = args.vectors.read_bytes()
raw = gzip.decompress(compressed)
vectors = json.loads(raw)
assert vectors["schema"] == "H2_TYPED_CONTROLLER_FULLBODY_SOURCE_V1"
assert (len(vectors["producer_cases"]), vectors["producer_calls"], vectors["emissions"],
        vectors["coupled_CPU_calls"], len(vectors["copy_controls"]),
        len(vectors["CPU_extension_controls"])) == (20, 208, 1712, 208, 72, 324)
run = subprocess.run([str(args.probe.resolve())], input=raw, capture_output=True, timeout=60)
assert run.returncode == 0 and not run.stderr, (run.returncode, run.stderr.decode())
result = json.loads(run.stdout)
expected = {
    "passed": True, "retained_producer_calls": 208, "source_emissions": 1712,
    "individual_additions": 13696, "coupled_original_CPU_callbacks": 208,
    "original_copy_controls": 72, "original_reset_gate_error_controls": 324,
    "all39_source_fields": True, "all640_ring_words_per_call": True,
    "stable_owners_above4GiB": True, "physical_hardware_or_endpoint_receipt_proven": False,
}
assert result == expected, result
result.update({
    "fixture_sha256": hashlib.sha256(compressed).hexdigest(),
    "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
    "whole_DSP_graph_ready": False, "initializer45_complete": False,
    "runtime_original_program_or_core_loaded": False,
})
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result))
