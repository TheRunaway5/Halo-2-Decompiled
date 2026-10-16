#!/usr/bin/env python3
"""Compare actual native GP value-copy owners with frozen original values."""
from pathlib import Path
import argparse
import gzip
import hashlib
import json
import struct
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--probe", type=Path, required=True)
parser.add_argument("--vectors", type=Path, required=True)
parser.add_argument("--report", type=Path, required=True)
args = parser.parse_args()
compressed = args.vectors.read_bytes()
raw = gzip.decompress(compressed)
assert struct.unpack_from("<2I", raw) == (0x47504F31, 36)
assert len(raw) == 8 + 36 * (1 + 6 + 3712 + 1024) * 4
run = subprocess.run([str(args.probe.resolve())], input=raw, capture_output=True, timeout=60)
assert run.returncode == 0 and not run.stderr, (run.returncode, run.stderr.decode())
result = json.loads(run.stdout)
assert result == {
    "passed": True, "original_frames": 33, "phase_snapshots": 3,
    "no_mutation_guards": 4762, "receipt_lifetime_controls": 12,
    "retained_receipt_after_owner_destruction": True, "native64_allocation": True,
    "install_ack": False, "graph_frame_receipt": False, "dma_packing_or_pcm": False,
}, result
result.update({"fixture_sha256": hashlib.sha256(compressed).hexdigest(),
               "uncompressed_fixture_sha256": hashlib.sha256(raw).hexdigest(),
               "probe_sha256": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
               "initializer45_complete": False, "whole_DSP_graph_ready": False,
               "runtime_original_program_or_core_loaded": False})
args.report.parent.mkdir(parents=True, exist_ok=True)
args.report.write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result))
