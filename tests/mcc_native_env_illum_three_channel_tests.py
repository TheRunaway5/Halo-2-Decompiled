"""Run complete literal environment material regression in owned scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path, required=True)
    parser.add_argument("--native-root", default="")
    parser.add_argument("--authority", default="")
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="env-illum-", dir=os.environ["HALO2_TEST_SCRATCH"]) as folder:
        fixture = Path(folder) / "fixture.json"
        if args.native_root and args.authority:
            result = subprocess.run([sys.executable, "-B", str(root / "tools/export_mcc_native_env_illum_three_channel_fixture.py"),
                "--repo-root", str(root), "--prepared-root", str(Path(args.native_root).resolve(strict=True).parent),
                "--authority", str(Path(args.authority).resolve(strict=True)), "--output", str(fixture)])
            if result.returncode:
                return result.returncode
        else:
            directory = root / "tests/fixtures"
            metadata = json.loads((directory / "mcc-native-env-illum-three-channel-v1.json").read_bytes())
            name = metadata["compressed_file"]
            if (metadata["schema"] != "H2_MCC_NATIVE_ENV_ILLUM_THREE_CHANNEL_REGRESSION_V1" or
                    Path(name).name != name or not name or not 0 < metadata["compressed_bytes"] <= 256 * 1024 or
                    not 0 < metadata["decoded_bytes"] <= 32 * 1024 * 1024 or
                    metadata["profile_sha256"] != sha((root / "tools/data/mcc-native-env-illum-three-channel-profiles.json").read_bytes())):
                raise ValueError("Invalid complete literal environment/illumination fixture contract")
            packed = (directory / name).read_bytes()
            if len(packed) != metadata["compressed_bytes"] or sha(packed) != metadata["compressed_sha256"]:
                raise ValueError("Complete literal environment fixture bytes changed")
            decoder = zlib.decompressobj()
            raw = decoder.decompress(packed, metadata["decoded_bytes"] + 1)
            if (len(raw) != metadata["decoded_bytes"] or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or
                    sha(raw) != metadata["decoded_sha256"]):
                raise ValueError("Literal environment fixture has changed, oversized or trailing data")
            fixture.write_bytes(raw)
        output = args.report_output or Path(folder) / "result.json"
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(fixture), str(output)]).returncode


if __name__ == "__main__":
    raise SystemExit(main())
