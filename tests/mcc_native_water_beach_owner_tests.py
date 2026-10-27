#!/usr/bin/env python3
"""Source-resource and in-memory writer controls; use tools/run_test.py.

This does not create or publish a prepared derivative. Actual native Cache and
GPU admission are separate checks against the coordinated owned candidate.
"""
import argparse
import copy
from dataclasses import replace
import hashlib
import io
import json
import os
from pathlib import Path
import tempfile
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, texture_resource
from prepare_mcc_water_beach_owner import (profiles, original_resources, validate_original,
    platform_carrier, resource_append, project_metadata, append_owner, MAPS, sha)
from prepare_mcc_world_bitmap_owner import private_candidate


def need(ok, why):
    if not ok:
        raise AssertionError(why)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=ROOT)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    root = args.repo_root.resolve(strict=True)
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    need(owner.is_dir(), "Active owned scratch required")
    source_root = (args.source or root / "assets/imported/mcc-sky-water-tu15-20261025-v7").resolve(strict=True)
    policy, source, _, _ = profiles(root)
    resources = original_resources(root, source)
    source_map = source_root / MAPS / policy["target"]["map"]
    before = file_sha(source_map)
    need(before == policy["target"]["prepared_map_sha256"], "Pinned actual prepared source map differs")
    rejected = 0

    def reject(call):
        nonlocal rejected
        try:
            call()
        except FormatError:
            rejected += 1
            return
        raise AssertionError("Changed source ownership was accepted")

    with tempfile.TemporaryDirectory(prefix="water-writer-guards-", dir=owner) as temporary:
        guard = Path(temporary)
        candidate = guard / "candidate"
        candidate.mkdir()
        original = guard / "original"
        original.mkdir()
        need(private_candidate(original, candidate) == (original, candidate), "Separate unshared owned stage rejected")
        reject(lambda: private_candidate(original, owner))
        reject(lambda: private_candidate(original, original))
        reject(lambda: private_candidate(owner, candidate))
        raw = original / "source-byte"
        raw.write_bytes(b"exact-beforeimage")
        shared = candidate / "shared-byte"
        shared.hardlink_to(raw)
        reject(lambda: private_candidate(original, candidate))
        shared.unlink()
        shared.symlink_to(raw)
        reject(lambda: private_candidate(original, candidate))
        shared.unlink()
        shared.write_bytes(raw.read_bytes())
        need(private_candidate(original, candidate) == (original, candidate), "Independent copied candidate rejected")

    for index in range(len(resources.metadata)):
        changed = bytearray(resources.metadata)
        changed[index] ^= 1
        reject(lambda: validate_original(replace(resources, metadata=bytes(changed)), source))
    for frame in range(17):
        for slot in range(3):
            raw = resources.hardware[frame][slot]
            hardware = [list(parts) for parts in resources.hardware]
            hardware[frame][slot] = (bytes([raw[0] ^ 1]) + raw[1:]) if raw else b"\0"
            reject(lambda: validate_original(replace(resources, hardware=tuple(tuple(p) for p in hardware)), source))
            if raw:
                hardware[frame][slot] = raw[:-1]
                reject(lambda: validate_original(replace(resources, hardware=tuple(tuple(p) for p in hardware)), source))
        low = list(resources.low_detail)
        low[frame] = bytes([low[frame][0] ^ 1]) + low[frame][1:]
        reject(lambda: validate_original(replace(resources, low_detail=tuple(low)), source))
    reject(lambda: validate_original(replace(resources, hardware=resources.hardware[:-1]), source))
    reject(lambda: validate_original(replace(resources, low_detail=resources.low_detail[::-1]), source))
    append, frames = resource_append(resources, source, 4096)
    pool = io.BytesIO(bytes(4096) + append)
    partitions = 0
    for index, rows in enumerate(frames):
        for slot, row in enumerate(rows):
            if row["stored_bytes"]:
                need(texture_resource(pool, row["pointer"], row["stored_bytes"]) == resources.hardware[index][slot],
                     "Native MCC storage decoder changed an original partition byte")
                partitions += 1
    reject(lambda: resource_append(resources, source, 0))
    reject(lambda: resource_append(resources, source, 0x40000000 - len(append)))

    cache = MccCache(source_map)
    try:
        carrier = platform_carrier(cache, policy)
        datum = 0xd0000000 | policy["target"]["directory_count"]
        address = 0x0f000000  # Test-owned translated allocation; no live owner/GPU claim.
        projected = project_metadata(resources, source, policy, carrier, frames, datum, address)
        need(len(projected) == 2936 and struct.unpack_from("<I", projected, 72)[0] == address + 80,
             "Complete target root relocation differs")
        for index in range(17):
            at = 80 + index * 168
            need(projected[at:at + 28] == resources.metadata[76 + index * 116:76 + index * 116 + 28] and
                 struct.unpack_from("<I", projected, at + 28)[0] == datum,
                 "Original author fields or full target datum changed")
            for low, high in ((32, 56), (68, 80), (92, 168)):
                need(projected[at + low:at + high] == carrier[at + low:at + high], "Literal MCC platform bytes changed")
        for index in range(len(carrier)):
            changed = bytearray(carrier)
            changed[index] ^= 1
            reject(lambda: project_metadata(resources, source, policy, bytes(changed), frames, datum, address))
        for frame in range(17):
            for slot in range(3):
                for field in ("frame", "slot", "raw_bytes", "stored_bytes", "pointer"):
                    changed = copy.deepcopy(frames)
                    changed[frame][slot][field] += 1
                    reject(lambda: project_metadata(resources, source, policy, carrier, changed, datum, address))
                if slot < 2:
                    changed = copy.deepcopy(frames)
                    changed[frame][slot]["sha256"] = "0" * 64
                    reject(lambda: project_metadata(resources, source, policy, carrier, changed, datum, address))
        for wrong_datum, wrong_address in ((datum ^ 0x10000000, address), (0xd0008000, address), (datum, 0), (datum, 2**32 - 2936)):
            reject(lambda: project_metadata(resources, source, policy, carrier, frames, wrong_datum, wrong_address))
        changed = copy.deepcopy(policy)
        changed["mcc_platform_carrier"]["tag"]["datum"] ^= 0x10000
        reject(lambda: platform_carrier(cache, changed))
        # Actual decoded source map, complete old-byte and salted-directory proof,
        # no map write and no prepared candidate created.
        result = append_owner(cache, policy, source, resources, carrier, frames)
        at = result["metadata_offset"]
        need(sha(cache.data[at:at + 2936]) == result["metadata_sha256"] and
             struct.unpack_from("<I", cache.data, at + 72)[0] == result["address"] + 80,
             "Actual map append pointer differs")
    finally:
        cache.close()
    need(file_sha(source_map) == before, "Read-only actual source map changed")
    report = dict(schema="H2_MCC_NATIVE_WATER_BEACH_OWNER_COMPONENT_V1", passed=True,
        scope=__doc__, original_frames=17, exact_native_mcc_raw_partitions=partitions,
        exact_source_metadata_bytes=2048, exact_retained_software_low_detail_bytes=2176,
        exact_common_frame_bytes=17 * 28, strict_mutation_rejections=rejected,
        texture_bank_appended_bytes=len(append), component_texture_bank_append_sha256=sha(append),
        source_map_sha256=before, source_map_unmodified=True, prepared_derivative_created=False,
        source_files_sha256={name: file_sha(root / name) for name in (
            "tools/prepare_mcc_water_beach_owner.py", "tools/data/mcc-native-water-beach-projection-v1.json",
            "tools/data/mcc-native-water-beach-owner-v1.json", "tests/mcc_native_water_beach_owner_tests.py")})
    if args.report_output:
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
