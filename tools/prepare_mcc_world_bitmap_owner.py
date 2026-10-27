#!/usr/bin/env python3
"""Restore the finite missing Zanzibar bitmap in an owned private MCC stage.

Run through tools/run_test.py. The source cache is read-only. This reuses a
complete existing MCC texture partition whose nine decoded mips are byte-exact
to the original Xbox bitmap. No original map is opened during preparation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import stat
import struct

from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, texture_resource
from mcc_native_map_owner_publication import verify_owner_index, repair_owner_index
from prepare_mcc_gameplay import save_changed_cache
from prepare_mcc_elongation_glass import atomic_bytes, atomic_json

POLICY = "tools/data/mcc-native-world-bitmap-owner-v1.json"
POLICY_SHA256 = "1c12854d01de1a347917b3e728b4ee52c609c9c868f65add81a9b8104d870e66"
MAPS = Path("halo2/h2_maps_win64_dx11")


def require(value, message):
    if not value:
        raise FormatError("Native world bitmap owner: " + message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_policy(root):
    raw = (Path(root) / POLICY).read_bytes()
    require(len(raw) < 64 * 1024 and digest(raw) == POLICY_SHA256,
            "complete source-authenticated policy differs")
    policy = json.loads(raw)
    require(policy["schema"] == "H2_MCC_NATIVE_WORLD_BITMAP_OWNER_V1",
            "unsupported policy")
    return policy, raw


def image(cache, tag, bank, policy):
    """Decode storage only; no resampling, mip synthesis or pixel tolerance."""
    rows = cache.block(tag["offset"] + 68, 168)
    require(len(rows) == 1, "source bitmap frame bank differs")
    at = rows[0]
    donor, pixels = policy["donor"], policy["pixels"]
    require(cache.u32(at + 56) == donor["partition_pointer"] and
            cache.u32(at + 80) == donor["partition_bytes"] and
            cache.u32(at + 60) == cache.u32(at + 64) == 0 and
            cache.u32(at + 84) == cache.u32(at + 88) == 0,
            "complete hardware partition ownership differs")
    require((cache.u16(at + 4), cache.u16(at + 6), cache.data[at + 8],
             cache.u16(at + 10), cache.u16(at + 12), cache.u16(at + 20)) ==
            (pixels["width"], pixels["height"], 1, 0, 11, len(pixels["mips"]) - 1),
            "source frame dimensions, format or complete mip count differ")
    with bank.open("rb") as stream:
        raw = texture_resource(stream, donor["partition_pointer"], donor["partition_bytes"])
    require(digest(raw) == donor["raw_resource_sha256"] and len(raw) == pixels["bytes"],
            "complete MCC source resource differs")
    rgba = bytearray(raw)
    rgba[0::4], rgba[2::4] = raw[2::4], raw[0::4]
    require(digest(rgba) == pixels["sha256"], "original full mip chain differs")
    offset = 0
    for mip in pixels["mips"]:
        require(mip["offset"] == offset and
                digest(rgba[offset:offset + mip["size"]]) == mip["sha256"],
                "original mip owner differs")
        offset += mip["size"]
    require(offset == len(rgba), "mip chain has unowned trailing bytes")
    return bytes(rgba)


def append_bitmap(cache, metadata, name, source_datum, expected_count):
    """Append one typed owner while preserving every old salted directory row."""
    data = cache.data
    before = bytes(data)
    count = cache.u32(cache.index + 24)
    require(count == expected_count and count < 32768 and count == cache.u32(32),
            "target directory identity or capacity differs")
    require(not any(tag["name"] == name for tag in cache.tags.values()),
            "target already declares this bitmap identity")
    require(len(metadata) == 248 and struct.unpack_from("<2I", metadata, 60) == (0, 0) and
            struct.unpack_from("<I", metadata, 68)[0] == 1 and
            struct.unpack_from("<I", metadata, 108)[0] == source_datum,
            "source bitmap payload is not the complete single-frame owner")
    header_magic = cache.u32(cache.index) - cache.index - 32
    table = cache.pointer(cache.u32(cache.index + 8), header_magic, count * 16)
    names, names_size, offsets = cache.u32(36), cache.u32(40), cache.u32(44)
    cache.require(names, names_size)
    cache.require(offsets, count * 4)
    old_table = bytes(data[table:table + count * 16])
    old_names = bytes(data[names:names + names_size])
    old_offsets = bytes(data[offsets:offsets + count * 4])
    datum = 0xd0000000 | count

    def append(raw, alignment=4):
        data.extend(bytes((-len(data)) % alignment))
        at = len(data)
        data.extend(raw)
        require(len(data) < 1024 * 1024 * 1024, "appended cache exceeds existing bound")
        return at

    payload = bytearray(metadata)
    allocation = (len(data) + 15) & ~15
    address = allocation + cache.magic
    require(0 < address < 2**32 - len(payload), "new bitmap virtual allocation overflow")
    struct.pack_into("<I", payload, 72, address + 80)
    struct.pack_into("<I", payload, 108, datum)
    require(append(payload, 16) == allocation, "bitmap allocation differs")
    new_names = append(old_names + name.encode("utf-8") + b"\0")
    new_offsets = append(old_offsets + struct.pack("<I", names_size))
    entry = struct.pack("<4I", int.from_bytes(b"bitm", "big"), datum, address, len(payload))
    new_table = append(old_table + entry)
    pointer = new_table + header_magic
    require(0 < pointer < 2**32, "new tag table pointer overflow")
    updates = {8: len(data), 32: count + 1, 36: new_names,
               40: names_size + len(name.encode("utf-8")) + 1, 44: new_offsets,
               cache.index + 8: pointer, cache.index + 24: count + 1}
    for at, value in updates.items():
        struct.pack_into("<I", data, at, value)
    restored = bytearray(data[:len(before)])
    for at in updates:
        restored[at:at + 4] = before[at:at + 4]
    require(restored == before and data[new_table:new_table + count * 16] == old_table and
            data[new_names:new_names + names_size] == old_names and
            data[new_offsets:new_offsets + count * 4] == old_offsets,
            "append changed an existing cache byte or salted owner")
    return dict(datum=datum, address=address, tag_bytes=len(payload),
                old_directory_count=count, directory_count=count + 1,
                header_updates={str(k): v for k, v in updates.items()},
                appended_bytes=len(data) - len(before),
                metadata_sha256=digest(payload), existing_body_and_salted_owners_unchanged=True)


def private_candidate(source, candidate):
    source, candidate = (Path(p).resolve(strict=True) for p in (source, candidate))
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    require(candidate.is_relative_to(scratch) and candidate != scratch and
            not candidate.is_relative_to(source) and not source.is_relative_to(candidate),
            "candidate must be a separate existing stage inside the active owned scratch")
    for path in candidate.rglob("*"):
        info = path.lstat()
        require(stat.S_ISDIR(info.st_mode) or
                (stat.S_ISREG(info.st_mode) and info.st_nlink == 1),
                "candidate has a shared, symbolic or nonregular writer owner")
    return source, candidate


def _prepare(root, source, candidate, report_path):
    root = Path(root).resolve(strict=True)
    source, candidate = private_candidate(source, candidate)
    policy, policy_raw = read_policy(root)
    target = MAPS / policy["target"]["map"]
    donor_path = MAPS / policy["donor"]["map"]
    bank = MAPS / "textures.dat"
    names = [target, donor_path, Path("provenance.json"), Path("native-shaders/map-owners.json"), bank]
    before = {str(p): file_sha(source / p) for p in names if p != bank}
    source_stamps = {str(p): (source / p).stat() for p in names}
    for name in (target, donor_path):
        require(file_sha(candidate / name) == before[str(name)], "candidate source map differs")
    require(before[str(target)] == policy["target"]["prepared_map_sha256"] and
            before[str(donor_path)] == policy["donor"]["map_sha256"],
            "complete prepared target or donor input differs")
    provenance = json.loads((candidate / "provenance.json").read_bytes())
    verified = verify_owner_index(candidate, provenance)
    require(verified is not None, "candidate lacks authenticated prepared owners")
    donor = MccCache(candidate / donor_path)
    target_cache = MccCache(candidate / target)
    try:
        expected = policy["donor"]
        tag, at = donor.tag(expected["datum"], "bitm")
        metadata = bytes(donor.data[at:at + tag["size"]])
        require(tag["name"] == policy["tag"] and tag["address"] == expected["address"] and
                tag["size"] == expected["tag_size"] and digest(metadata) == expected["metadata_sha256"] and
                metadata == bytes.fromhex(expected["metadata_hex"]),
                "complete MCC donor metadata differs")
        pixels = image(donor, tag, candidate / bank, policy)
        library = json.loads((candidate / "native-shaders/zanzibar/library.json").read_bytes())
        require([r for r in library["shader_tags"] if r["group"] == "shad" and
                 r["name"] == policy["target"]["shader"]["name"]] == [policy["target"]["shader"]],
                "original consuming shader owner differs")
        result = append_bitmap(target_cache, metadata, policy["tag"], tag["datum"],
                               policy["target"]["directory_count"])
        save_changed_cache(target_cache)
        check = MccCache(candidate / target)
        try:
            actual, _ = check.tag(result["datum"], "bitm")
            require(actual["name"] == policy["tag"] and
                    image(check, actual, candidate / bank, policy) == pixels,
                    "independently reopened owner or original complete pixels differ")
        finally:
            check.close()
    finally:
        donor.close()
        target_cache.close()
    after = file_sha(candidate / target)
    directory = candidate / "graphics-corrections"
    profile = json.loads((directory / "profile.json").read_bytes())
    profile.setdefault("world_bitmap_owners", []).append(policy)
    atomic_json(directory / "profile.json", profile)
    atomic_bytes(directory / "native-world-bitmap-owner-v1.json", policy_raw)
    graphics = provenance["graphics_corrections"]
    graphics["profile_sha256"] = file_sha(directory / "profile.json")
    graphics["restored_mips"] += len(policy["pixels"]["mips"])
    graphics["full_graphics_parity"] = False
    graphics["profiles"].append(dict(map=target.name, tag=policy["tag"],
        restored_mips=len(policy["pixels"]["mips"]), format=11,
        target_sha256=policy["pixels"]["sha256"]))
    graphics["changes"].append(dict(path=target.as_posix(), donor_sha256=before[str(target)],
        converted_sha256=after, producer=Path(__file__).name, profile=policy["schema"]))
    graphics["scope"] += "; exact source-authenticated missing Zanzibar granite bump owner"
    provenance["world_bitmap_owner_v8"] = dict(policy_sha256=POLICY_SHA256,
        source=str(source), selected=False, original_reference_runtime_input=False, **result)
    atomic_json(candidate / "graphics-conversion.json", graphics)
    atomic_json(candidate / "provenance.json", provenance)
    repair_owner_index(candidate, provenance, verified_beforeimage=verified)
    verify_owner_index(candidate, json.loads((candidate / "provenance.json").read_bytes()))
    for name, previous in source_stamps.items():
        current = (source / name).stat()
        require((current.st_dev, current.st_ino, current.st_size, current.st_mtime_ns, current.st_ctime_ns) ==
                (previous.st_dev, previous.st_ino, previous.st_size, previous.st_mtime_ns, previous.st_ctime_ns),
                "read-only retained source owner changed")
        if name in before:
            require(file_sha(source / name) == before[name], "retained source bytes changed")
    require((candidate / bank).stat().st_size == source_stamps[str(bank)].st_size,
            "existing shared resource-bank extent changed")
    report = dict(schema=policy["schema"], source=str(source), candidate=str(candidate),
        selected=False, original_cache_unchanged=True, source_map_sha256=before,
        map_sha256=after, original_pixels_sha256=policy["pixels"]["sha256"],
        complete_mips=len(policy["pixels"]["mips"]), pixel_bytes=len(pixels),
        texture_bank_bytes_written=0, owner_index_verified=True, **result)
    atomic_json(Path(report_path), report)
    return report


def prepare(root, source, candidate, report_path):
    source, candidate = private_candidate(source, candidate)
    policy, _ = read_policy(root)
    names = [MAPS / policy["target"]["map"], Path("provenance.json"),
             Path("native-shaders/map-owners.json"), Path("graphics-conversion.json"),
             Path("graphics-corrections/profile.json"),
             Path("graphics-corrections/native-world-bitmap-owner-v1.json")]
    before = {name: (candidate / name).read_bytes() if (candidate / name).exists() else None
              for name in names}
    try:
        return _prepare(root, source, candidate, report_path)
    except BaseException:
        for name, raw in before.items():
            if raw is None:
                (candidate / name).unlink(missing_ok=True)
            else:
                atomic_bytes(candidate / name, raw)
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--candidate", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    result = prepare(Path(__file__).resolve().parents[1], args.source, args.candidate, args.report)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
