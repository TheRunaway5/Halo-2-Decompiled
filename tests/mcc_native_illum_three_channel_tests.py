"""Stage actual paired MCC/TU inputs inside the enclosing test scratch owner."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path, required=True)
    parser.add_argument("--native-root", default="")
    parser.add_argument("--evidence", default="")
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="illum-three-channel-", dir=os.environ["HALO2_TEST_SCRATCH"]) as folder:
        fixture = Path(folder) / "fixture.json"
        if args.native_root and args.evidence:
            native = Path(args.native_root).resolve(strict=True)
            evidence = Path(args.evidence).resolve(strict=True)
            policy = json.loads((root / "tools/data/mcc-native-illum-three-channel-profiles.json").read_bytes())
            expected = policy["paired_source_report"]
            if hashlib.sha256(evidence.read_bytes()).hexdigest() != expected["sha256"]:
                raise ValueError("Complete paired source census differs from the qualified profile proof")
            result = subprocess.run([sys.executable, "-B", str(root / "tools/export_mcc_native_illum_three_channel_fixture.py"),
                "--repo-root", str(root), "--prepared-root", str(native.parent), "--evidence", str(evidence),
                "--output", str(fixture)], check=False)
            if result.returncode:
                return result.returncode
        else:
            directory = root / "tests/fixtures"
            metadata = json.loads((directory / "mcc-native-illum-three-channel-v1.json").read_bytes())
            name = metadata["compressed_file"]
            if (metadata["schema"] != "H2_MCC_NATIVE_ILLUM_THREE_CHANNEL_REGRESSION_V1" or
                    Path(name).name != name or not name or
                    not 0 < metadata["compressed_bytes"] <= 128 * 1024 or
                    not 0 < metadata["decoded_bytes"] <= 2 * 1024 * 1024):
                raise ValueError("Invalid literal cooked-profile regression owner")
            packed = (directory / name).read_bytes()
            if len(packed) != metadata["compressed_bytes"] or hashlib.sha256(packed).hexdigest() != metadata["compressed_sha256"]:
                raise ValueError("Complete literal cooked-profile regression bytes differ")
            decoder = zlib.decompressobj()
            raw = decoder.decompress(packed, metadata["decoded_bytes"] + 1)
            if (len(raw) != metadata["decoded_bytes"] or not decoder.eof or decoder.unused_data or
                    decoder.unconsumed_tail or hashlib.sha256(raw).hexdigest() != metadata["decoded_sha256"]):
                raise ValueError("Literal cooked-profile source fixture has changed or unowned trailing bytes")
            fixture.write_bytes(raw)
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture)], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
