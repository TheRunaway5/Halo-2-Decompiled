"""Prepare shipped JSON metadata only, then run the standalone C++ fixtures.

Do not import native_shader_bundle.load_bundled: that decodes program payloads.
The unused base64 fields remain JSON strings and are never decoded or written.
The CTest/run_test owner removes every generated file, including on failure.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib

LIMIT = 192 * 1024 * 1024


def canonical_hash(value):
    encoded = json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False)
    return hashlib.sha256(encoded.encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--test-executable", type=Path, required=True)
    parser.add_argument("--repo-root", type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    repository = args.repo_root.resolve(strict=True)
    bundle_path = repository / "tools/data/native-materials.json.zlib"
    report_path = bundle_path.with_suffix("")
    report_bytes = report_path.read_bytes()
    report = json.loads(report_bytes)
    packed = bundle_path.read_bytes()
    assert report["schema"] == "H2_NATIVE_MATERIAL_BUNDLE_V1"
    assert 0 < report["bytes"] <= LIMIT and 0 < report["decoded_bytes"] <= LIMIT
    assert len(packed) == report["bytes"]
    assert hashlib.sha256(packed).hexdigest() == report["sha256"]
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, LIMIT + 1)
    assert len(raw) == report["decoded_bytes"] and decoder.eof
    assert not decoder.unused_data and not decoder.unconsumed_tail
    bundle = json.loads(raw)
    assert bundle["schema"] == report["schema"] and sorted(bundle["maps"]) == report["maps"]
    closure_path = repository / "tools/data/mcc-native-library-closure.json"
    bootstrap_path = repository / "tools/data/mcc-native-hud-bootstrap.json"
    closure_bytes, bootstrap_bytes = closure_path.read_bytes(), bootstrap_path.read_bytes()
    closure, bootstrap = json.loads(closure_bytes), json.loads(bootstrap_bytes)
    assert closure["schema"] == "H2_MCC_RETAINED_NATIVE_LIBRARY_CLOSURE_V1"
    assert bootstrap["schema"] == "H2_MCC_NATIVE_HUD_BOOTSTRAP_V1"
    recipient_name = "lockout"
    recipient = bundle["maps"][recipient_name]
    source = bundle["maps"][closure["source_map"]]
    recipient_policy = next(row for row in closure["libraries"] if row["map"] == recipient_name)
    assert recipient_policy["missing"] and recipient_name != "00a_introduction"
    assert canonical_hash(recipient) == recipient_policy["canonical_sha256"]
    assert canonical_hash(source) == closure["source_canonical_sha256"]
    fixture = {"recipient": recipient, "source": source,
               "closure_policy": closure, "bootstrap_policy": bootstrap}
    # No payload or scalar-table base64 content is included in the fixture.
    del bundle, raw, packed
    try:
        with tempfile.TemporaryDirectory(prefix="mcc-library-snapshots-", dir=scratch) as name:
            owned = Path(name)
            fixture_path = owned / "metadata.json"
            fixture_path.write_text(json.dumps(fixture, sort_keys=True), encoding="utf-8")
            result = subprocess.run([str(args.test_executable.resolve(strict=True)),
                                     str(fixture_path), str(owned / "cases")], check=False)
            return result.returncode
    finally:
        # Recheck the shipped inputs even when the actual C++ test fails.
        assert hashlib.sha256(bundle_path.read_bytes()).hexdigest() == report["sha256"]
        assert report_path.read_bytes() == report_bytes
        assert closure_path.read_bytes() == closure_bytes
        assert bootstrap_path.read_bytes() == bootstrap_bytes


if __name__ == "__main__":
    raise SystemExit(main())
