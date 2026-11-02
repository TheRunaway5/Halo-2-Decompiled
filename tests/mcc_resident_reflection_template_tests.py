"""Stage literal PC banks and existing resident reference inputs in owned scratch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zlib


def literal_cases(root):
    directory = root / "tests/fixtures"
    metadata = json.loads((directory / "mcc-native-world-cooked-v1.json").read_bytes())
    name = metadata["compressed_file"]
    if (metadata["schema"] != "H2_MCC_NATIVE_WORLD_COOKED_REGRESSION_V1" or
            Path(name).name != name or not name or
            not 0 < metadata["compressed_bytes"] <= 128 * 1024 or
            not 0 < metadata["decoded_bytes"] <= 2 * 1024 * 1024):
        raise ValueError("Invalid literal cooked regression owner")
    packed = (directory / name).read_bytes()
    if (len(packed) != metadata["compressed_bytes"] or
            hashlib.sha256(packed).hexdigest() != metadata["compressed_sha256"]):
        raise ValueError("Literal compressed PC banks changed")
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, metadata["decoded_bytes"] + 1)
    if (len(raw) != metadata["decoded_bytes"] or not decoder.eof or decoder.unused_data or
            decoder.unconsumed_tail or hashlib.sha256(raw).hexdigest() != metadata["decoded_sha256"]):
        raise ValueError("Literal decoded PC banks changed or have trailing data")
    decoded = json.loads(raw)
    if decoded["schema"] != "H2_MCC_NATIVE_WORLD_COOKED_FIXTURE_V1":
        raise ValueError("Unsupported literal cooked fixture")
    cases = [case for case in decoded["cases"]
             if case["template"] == r"shaders\shader_templates\transparent\one_alpha_env_illum"]
    if (len(cases) != 3 or
            sorted(case["map"] for case in cases) != ["00a_introduction", "01a_tutorial", "needle"] or
            sum(case["shader"]["name"].endswith(r"\pg_decal") for case in cases) != 2):
        raise ValueError("Incomplete literal resident-reflection source cases")
    return cases, metadata["decoded_sha256"]


def existing_record(native, map_name, group, name, payload, size):
    directory = native / map_name
    library = json.loads((directory / "library.json").read_bytes())
    records = [record for record in library["shader_tags"]
               if record["group"] == group and record["name"] == name]
    if len(records) != 1 or records[0]["payload"] != payload or records[0]["bytes"] != size:
        raise ValueError("Existing resident reference identity changed")
    record = records[0]
    raw = (directory / "payloads" / payload).read_bytes()
    if len(raw) != size or hashlib.sha256(raw).hexdigest() + ".bin" != payload:
        raise ValueError("Complete existing resident reference bytes changed")
    return {"record": record, "raw_hex": raw.hex()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path, required=True)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    native = root / "assets/imported/mcc-sky-water-tu15-20261025-v7/native-shaders"
    if not (native / "03a_oldmombasa/library.json").is_file() or not (native / "dune/library.json").is_file():
        print("Existing resident pass/vertex and Dune template references are unavailable")
        return 77
    cases, source_hash = literal_cases(root)
    fixture = {
        "schema": "H2_MCC_RESIDENT_REFLECTION_COMPONENT_V1",
        "scope": "Literal PC metadata and complete resident-program authentication; CPU only; no live/GPU draw claim",
        "source_fixture_sha256": source_hash,
        "cases": cases,
        "pass": existing_record(native, "03a_oldmombasa", "spas",
            r"shaders\shader_passes\transparent\default_transparent_one_sided",
            "bc87fed938e4d10f84ae3b66dfb6082199f2898d6c0d42406276e2690217ffaa.bin", 576),
        "vertex": existing_record(native, "03a_oldmombasa", "vrtx",
            r"rasterizer\vertex_shaders_xbox\transparent_generic_reflection",
            "9b8a3c3c0e1d8e21eef32be5a16dfd02cc362b33825d2e2665a290dcf61fefe2.bin", 5608),
        "reference_template": existing_record(native, "dune", "stem",
            r"shaders\shader_templates\transparent\one_alpha_env_illum",
            "ffbc8f0460d8beaf3e5349ba896d63d42fb7bbbccd7b18b9735ba19ccce06bec.bin", 296),
    }
    with tempfile.TemporaryDirectory(prefix="resident-reflection-", dir=os.environ["HALO2_TEST_SCRATCH"]) as folder:
        path = Path(folder) / "fixture.json"
        path.write_text(json.dumps(fixture, separators=(",", ":")))
        return subprocess.run([str(args.test_executable.resolve(strict=True)), str(path)], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
