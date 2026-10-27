#!/usr/bin/env python3
"""Append the exact original17-frame beach bitmap to an owned prepared stage.

The default --plan mode is read-only. --candidate mutates only an existing,
unshared stage under HALO2_TEST_SCRATCH. No cache is cloned, selected or
published here. Every source image/partition byte is authenticated; the Xbox
software low-detail payload is retained without inventing runtime selector state.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import struct

from audit_rendering import Cache, FormatError, file_sha
from extract_mcc_native_water_beach_owner import extract, MAP, MAP_SHA
from mcc_content import MccCache, texture_resource
from mcc_native_map_owner_publication import repair_owner_index, verify_owner_index
from prepare_mcc_elongation_glass import atomic_bytes, atomic_json, prefix_digest
from prepare_mcc_world_bitmap_owner import private_candidate

POLICY = "tools/data/mcc-native-water-beach-projection-v1.json"
POLICY_SHA = "af07fb62a8af98ce15f81ded3c3a63f5800e95c5df636f72d129d0c515323449"
SOURCE_PROFILE = "tools/data/mcc-native-water-beach-owner-v1.json"
SOURCE_PROFILE_SHA = "7fbfffd1d4f9749d3614b0609ddd6d62caa3b72e83fe370899de595a90429e6e"
MAPS = Path("halo2/h2_maps_win64_dx11")
SIDECAR = Path("graphics-corrections/water-beach-source-v1")


def require(ok, why):
    if not ok:
        raise FormatError("Native water-beach owner: " + why)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def stamp(path):
    value = path.stat()
    require(not path.is_symlink() and path.is_file(), "input lost regular exact owner")
    return (value.st_dev, value.st_ino, value.st_size, value.st_mtime_ns, value.st_ctime_ns)


def profiles(root):
    policy_raw, source_raw = ((root / name).read_bytes() for name in (POLICY, SOURCE_PROFILE))
    require(len(policy_raw) < 64 * 1024 and sha(policy_raw) == POLICY_SHA and
            len(source_raw) < 64 * 1024 and sha(source_raw) == SOURCE_PROFILE_SHA,
            "complete projection or original source policy differs")
    policy, source = json.loads(policy_raw), json.loads(source_raw)
    require(policy["schema"] == "H2_MCC_NATIVE_WATER_BEACH_PROJECTION_V1" and
            source["schema"] == "H2_MCC_NATIVE_WATER_BEACH_OWNER_V1" and
            policy["target"]["tag"] == source["tag"], "finite typed policy differs")
    return policy, source, policy_raw, source_raw


@dataclass(frozen=True)
class OriginalResources:
    metadata: bytes
    hardware: tuple[tuple[bytes, bytes, bytes], ...]
    low_detail: tuple[bytes, ...]


def validate_original(resources, source):
    expected = source["original"]
    require(len(resources.metadata) == 2048 and
            sha(resources.metadata) == expected["complete_metadata_sha256"] and
            resources.metadata[:76] == bytes.fromhex(expected["root_metadata_hex"]) and
            len(resources.hardware) == len(resources.low_detail) == len(expected["frames"]) == 17,
            "complete original bitmap metadata or frame ownership differs")
    for index, row in enumerate(expected["frames"]):
        require(row["frame"] == index and
                resources.metadata[76 + index * 116:76 + (index + 1) * 116] == bytes.fromhex(row["metadata_hex"]),
                "literal original frame metadata differs")
        parts = resources.hardware[index]
        require(len(parts) == 3, "original complete hardware partition count differs")
        for raw, record in zip(parts, row["complete_hardware_partitions"], strict=True):
            require(len(raw) == record["source_bytes"] and
                    (not raw or sha(raw) == record["sha256"]), "original full hardware partition differs")
        require(len(parts[0]) == 43008 and len(parts[1]) == 10240 and not parts[2] and
                parts[1] == parts[0][32768:], "original primary/secondary/absent partition contract differs")
        low = resources.low_detail[index]
        require(len(low) == row["software_low_detail"]["source_bytes"] == 128 and
                sha(low) == row["software_low_detail"]["sha256"], "original complete software low-detail payload differs")


def original_resources(root, source):
    # Existing recipe also authenticates the actual consuming shader slot and
    # all17 predicted frame indices, not just a same-name bitmap.
    actual, _, _ = extract(root)
    require(actual == source, "complete source extraction policy changed")
    path = root / MAP
    before = stamp(path)
    cache = Cache(path)
    try:
        require(sha(cache.data) == MAP_SHA, "original full map differs")
        tag, at = cache.tag(source["original"]["tag"]["datum"], "bitm")
        metadata = bytes(cache.data[at:at + tag["size"]])
        hardware, low_detail = [], []
        blob = cache.u32(0x158)
        for frame in cache.block(at + 68, 116):
            parts = []
            for index in range(3):
                pointer, size = cache.u32(frame + 28 + index * 4), cache.u32(frame + 52 + index * 4)
                parts.append(bytes(cache.data[pointer:pointer + size]) if size else b"")
            hardware.append(tuple(parts))
            pointer, size = blob + cache.u32(frame + 0x60), cache.u32(frame + 0x64)
            low_detail.append(bytes(cache.data[pointer:pointer + size]))
        resources = OriginalResources(metadata, tuple(hardware), tuple(low_detail))
        validate_original(resources, source)
        require(stamp(path) == before, "original source changed while extracting")
        return resources
    finally:
        cache.close()


def platform_carrier(cache, policy):
    expected = policy["mcc_platform_carrier"]
    tag, at = cache.tag(expected["tag"]["datum"], "bitm")
    require(tag == expected["tag"] and tag["size"] == 80 + 17 * 168,
            "actual MCC platform carrier typed owner differs")
    raw = bytes(cache.data[at:at + tag["size"]])
    require(sha(raw) == expected["complete_metadata_sha256"] and
            raw == bytes.fromhex(expected["root_hex"]) + b"".join(bytes.fromhex(x) for x in expected["frames_hex"]),
            "complete MCC platform carrier metadata differs")
    require(cache.block(at + 68, 168) == [at + 80 + i * 168 for i in range(17)],
            "actual MCC frame bank allocation differs")
    return raw


def resource_append(resources, source, bank_extent):
    validate_original(resources, source)
    require(0 < bank_extent < 0x40000000, "texture bank exceeds complete resource address capacity")
    blob, frames = bytearray(), []
    for index, parts in enumerate(resources.hardware):
        rows = []
        for slot, raw in enumerate(parts):
            row = dict(frame=index, slot=slot, pointer=0, stored_bytes=0, raw_bytes=len(raw))
            if raw:
                row.update(pointer=bank_extent + len(blob), stored_bytes=len(raw) + 8, sha256=sha(raw))
                blob.extend(struct.pack("<Ii", 1, -len(raw)) + raw)
            rows.append(row)
        frames.append(rows)
    require(len(blob) == 905488 and bank_extent + len(blob) < 0x40000000,
            "finite exact appended hardware resource extent differs")
    return bytes(blob), frames


def project_metadata(resources, source, policy, carrier, frames, datum, address):
    validate_original(resources, source)
    expected = policy["mcc_platform_carrier"]
    require(carrier == bytes.fromhex(expected["root_hex"]) + b"".join(bytes.fromhex(x) for x in expected["frames_hex"]),
            "projection lost complete platform carrier identity")
    require(0xd0000000 <= datum < 0xd0008000 and 0 < address < 2**32 - 2936 and
            len(frames) == 17, "relocated typed bitmap/frame ownership differs")
    next_partition = frames[0][0]["pointer"]
    payload = bytearray(carrier)
    payload[:72] = resources.metadata[:72]
    struct.pack_into("<I", payload, 72, address + 80)
    for index, rows in enumerate(frames):
        require(len(rows) == 3, "complete target hardware descriptor count differs")
        start = 80 + index * 168
        payload[start:start + 28] = resources.metadata[76 + index * 116:76 + index * 116 + 28]
        struct.pack_into("<I", payload, start + 28, datum)
        for slot, row in enumerate(rows):
            raw = resources.hardware[index][slot]
            require(row["frame"] == index and row["slot"] == slot and row["raw_bytes"] == len(raw) and
                    ((not raw and row["pointer"] == row["stored_bytes"] == 0) or
                     (raw and row["sha256"] == sha(raw) and row["stored_bytes"] == len(raw) + 8 and
                      0 < row["pointer"] < 0x40000000 - row["stored_bytes"])),
                    "target partition lost its exact source owner")
            if raw:
                require(row["pointer"] == next_partition, "target hardware partition allocation is not complete and contiguous")
                next_partition += row["stored_bytes"]
            struct.pack_into("<I", payload, start + 56 + slot * 4, row["pointer"])
            struct.pack_into("<I", payload, start + 80 + slot * 4, row["stored_bytes"])
    return bytes(payload)


def append_owner(cache, policy, source, resources, carrier, frames):
    """Build the complete in-memory append; never modify an old salted row."""
    data, before = cache.data, bytes(cache.data)
    count = cache.u32(cache.index + 24)
    require(count == policy["target"]["directory_count"] == cache.u32(32) and count < 32768 and
            not any(t["name"] == source["tag"] for t in cache.tags.values()),
            "target directory changed or already declares the exact bitmap")
    header_magic = cache.u32(cache.index) - cache.index - 32
    table = cache.pointer(cache.u32(cache.index + 8), header_magic, count * 16)
    names, names_size, offsets = cache.u32(36), cache.u32(40), cache.u32(44)
    cache.require(names, names_size)
    cache.require(offsets, count * 4)
    old_table, old_names, old_offsets = (bytes(data[at:at + size]) for at, size in
        ((table, count * 16), (names, names_size), (offsets, count * 4)))
    datum = 0xd0000000 | count
    allocation = (len(data) + 15) & ~15
    address = allocation + cache.magic
    payload = project_metadata(resources, source, policy, carrier, frames, datum, address)

    def append(raw, alignment=4):
        data.extend(bytes((-len(data)) % alignment))
        at = len(data)
        data.extend(raw)
        require(len(data) < 1024 * 1024 * 1024, "appended cache exceeds existing bound")
        return at

    require(append(payload, 16) == allocation, "bitmap virtual allocation changed")
    name = source["tag"].encode() + b"\0"
    new_names = append(old_names + name)
    new_offsets = append(old_offsets + struct.pack("<I", names_size))
    new_table = append(old_table + struct.pack("<4I", int.from_bytes(b"bitm", "big"), datum, address, len(payload)))
    updates = {8: len(data), 32: count + 1, 36: new_names, 40: names_size + len(name), 44: new_offsets,
               cache.index + 8: new_table + header_magic, cache.index + 24: count + 1}
    require(0 < updates[cache.index + 8] < 2**32, "new directory pointer overflow")
    for at, value in updates.items():
        struct.pack_into("<I", data, at, value)
    restored = bytearray(data[:len(before)])
    for at in updates:
        restored[at:at + 4] = before[at:at + 4]
    require(restored == before and data[new_table:new_table + count * 16] == old_table and
            data[new_names:new_names + names_size] == old_names and data[new_offsets:new_offsets + count * 4] == old_offsets,
            "append changed an existing source byte or salted owner")
    # Existing MCC writer contract: uncompressedv13, no stale compressed table.
    struct.pack_into("<H", data, 28, cache.u16(28) & ~1)
    struct.pack_into("<4I", data, 776, 0, 0, 0, 0)
    return dict(datum=datum, address=address, metadata_offset=allocation, metadata_bytes=len(payload),
        metadata_sha256=sha(payload), old_directory_count=count, directory_count=count + 1,
        appended_map_bytes=len(data) - len(before), output_map_bytes=len(data), output_map_sha256=sha(data),
        header_updates={str(k): v for k, v in updates.items()},
        existing_body_and_salted_owners_unchanged=True)


def verify_reopened(cache, bank, policy, source, resources, frames, result):
    tag, at = cache.tag(result["datum"], "bitm")
    require(tag["name"] == source["tag"] and tag["size"] == 2936 and tag["address"] == result["address"] and
            sha(cache.data[at:at + tag["size"]]) == result["metadata_sha256"], "reopened typed bitmap metadata differs")
    records = cache.block(at + 68, 168)
    require(records == [at + 80 + i * 168 for i in range(17)], "reopened complete frame allocation differs")
    with bank.open("rb") as stream:
        for index, frame in enumerate(records):
            require(cache.u32(frame + 28) == result["datum"] and
                    cache.data[frame:frame + 28] == resources.metadata[76 + index * 116:76 + index * 116 + 28],
                    "reopened original image authoring or full-salt owner differs")
            for slot, row in enumerate(frames[index]):
                require(cache.u32(frame + 56 + slot * 4) == row["pointer"] and
                        cache.u32(frame + 80 + slot * 4) == row["stored_bytes"], "reopened exact partition descriptor differs")
                if row["stored_bytes"]:
                    require(texture_resource(stream, row["pointer"], row["stored_bytes"]) == resources.hardware[index][slot],
                            "reopened raw hardware partition differs from original")


def prepare_plan(root, source_root):
    root, source_root = Path(root).resolve(strict=True), Path(source_root).resolve(strict=True)
    policy, source, policy_raw, source_raw = profiles(root)
    resources = original_resources(root, source)
    map_name = MAPS / policy["target"]["map"]
    bank_name = MAPS / "textures.dat"
    library_name = Path("native-shaders/03a_oldmombasa/library.json")
    inputs = (map_name, bank_name, library_name, Path("native-shaders/map-owners.json"), Path("provenance.json"))
    stamps = {str(name): stamp(source_root / name) for name in inputs}
    pins = {str(name): file_sha(source_root / name) for name in inputs}
    require(pins[str(map_name)] == policy["target"]["prepared_map_sha256"], "exact prepared target map differs")
    library = json.loads((source_root / library_name).read_bytes())
    matches = [row for row in library["shader_tags"] if row["group"] == "shad" and row["name"] == policy["target"]["shader_name"]]
    require(len(matches) == 1 and matches[0]["payload"] == policy["target"]["shader_payload"],
            "actual retained native consuming shader differs")
    payload_path = source_root / library_name.parent / "payloads" / matches[0]["payload"]
    require(file_sha(payload_path) + ".bin" == matches[0]["payload"], "complete actual native shader payload differs")
    bank_extent = stamps[str(bank_name)][2]
    appended, frames = resource_append(resources, source, bank_extent)
    cache = MccCache(source_root / map_name)
    try:
        carrier = platform_carrier(cache, policy)
        result = append_owner(cache, policy, source, resources, carrier, frames)
        map_raw = bytes(cache.data)
    finally:
        cache.close()
    sidecars = {SIDECAR / "original-metadata.bin": resources.metadata,
                SIDECAR / "software-low-detail.bin": b"".join(resources.low_detail),
                SIDECAR / "source-profile.json": source_raw, SIDECAR / "projection-profile.json": policy_raw}
    report = dict(schema=policy["schema"], scope=__doc__, source=str(source_root), selected=False,
        source_map_sha256=pins, source_map_bytes=stamps[str(map_name)][2], original_map_sha256=MAP_SHA,
        policy_sha256=POLICY_SHA, source_profile_sha256=SOURCE_PROFILE_SHA, native_shader=matches[0],
        native_shader_library_unchanged=True, bank_prefix_bytes=bank_extent, bank_prefix_sha256=pins[str(bank_name)],
        texture_bank_appended_bytes=len(appended), texture_bank_append_sha256=sha(appended),
        frames=17, main_mips=51, exact_hardware_partitions=34, exact_low_detail_payloads=17,
        software_selector_state_synthesized=False, raw_image_bytes_converted=False,
        partitions=frames, retained_source_sidecars={str(k): dict(bytes=len(v), sha256=sha(v)) for k, v in sidecars.items()},
        prepared_derivative_created=False, source_or_prepared_assets_modified=False, **result)
    for name, previous in stamps.items():
        require(stamp(source_root / name) == previous, "read-only source changed during plan")
    return dict(policy=policy, source=source, resources=resources, result=result, map_name=map_name, bank_name=bank_name,
                map_raw=map_raw, appended=appended, frames=frames, sidecars=sidecars, report=report, stamps=stamps, pins=pins)


def prepare(root, source_root, candidate, report_path):
    source_root, candidate = private_candidate(source_root, candidate)
    plan = prepare_plan(root, source_root)
    map_name, bank_name = plan["map_name"], plan["bank_name"]
    policy = plan["policy"]
    metadata_names = [Path("provenance.json"), Path("native-shaders/map-owners.json"),
                      Path("graphics-conversion.json"), Path("graphics-corrections/profile.json")]
    require(not (candidate / SIDECAR).exists(), "source-resource sidecar destination already exists")
    # V8 already owns independent legitimate Zanzibar/library corrections.
    # Only this producer's actual map and hardware-bank inputs must equal the
    # immutable V7 authority. Its global metadata has its own authenticated
    # beforeimage/index, and is extended without copying V7 metadata over it.
    require(all(file_sha(candidate / name) == plan["pins"][str(name)] for name in (map_name, bank_name)),
            "candidate beach map or complete texture-bank prefix differs from the authenticated source")
    library_name = Path("native-shaders/03a_oldmombasa/library.json")
    library = json.loads((candidate / library_name).read_bytes())
    matches = [row for row in library["shader_tags"] if row["group"] == "shad" and
               row["name"] == policy["target"]["shader_name"]]
    require(matches == [plan["report"]["native_shader"]] and
            file_sha(candidate / library_name.parent / "payloads" / matches[0]["payload"]) + ".bin" == matches[0]["payload"],
            "candidate actual native water shader lost its complete original source owner")
    candidate_pins = {str(name): dict(sha256=file_sha(candidate / name), stamp=stamp(candidate / name))
                      for name in (map_name, bank_name, library_name, *metadata_names)}
    provenance = json.loads((candidate / "provenance.json").read_bytes())
    verified = verify_owner_index(candidate, provenance)
    require(verified is not None, "candidate lacks authenticated prepared owner index")
    before = {name: (candidate / name).read_bytes() for name in [map_name, *metadata_names]}
    for name, pin in candidate_pins.items():
        require(stamp(candidate / name) == pin["stamp"], "candidate changed while capturing its own beforeimage")
    atomic_json(Path(report_path).with_suffix(".before.json"), dict(schema=policy["schema"],
        phase="authenticated-owned-candidate-beforeimage", source=str(source_root), candidate=str(candidate),
        source_policy_sha256=SOURCE_PROFILE_SHA, projection_policy_sha256=POLICY_SHA,
        source_pins=plan["pins"], candidate_pins=candidate_pins, owner_index_verified=True,
        original_map_sha256=MAP_SHA, map_beforeimage_held_in_memory_bytes=len(before[map_name]),
        all_other_candidate_json_beforeimages_bytes=sum(len(before[name]) for name in metadata_names),
        source_and_candidate_map_bank_byte_equal=True, candidate_native_water_shader_unchanged=True,
        source_or_prepared_assets_modified=False))
    bank_extent = plan["report"]["bank_prefix_bytes"]
    try:
        with (candidate / bank_name).open("ab") as stream:
            require(stream.tell() == bank_extent, "candidate resource-bank extent changed before append")
            require(stream.write(plan["appended"]) == len(plan["appended"]), "short exact hardware partition append")
        atomic_bytes(candidate / map_name, plan["map_raw"])
        for name, raw in plan["sidecars"].items():
            (candidate / name).parent.mkdir(parents=True, exist_ok=True)
            atomic_bytes(candidate / name, raw)
        require(prefix_digest(candidate / bank_name, bank_extent) == plan["pins"][str(bank_name)] and
                (candidate / bank_name).stat().st_size == bank_extent + len(plan["appended"]),
                "existing complete texture bank prefix changed")
        check = MccCache(candidate / map_name)
        try:
            verify_reopened(check, candidate / bank_name, policy, plan["source"], plan["resources"], plan["frames"], plan["result"])
        finally:
            check.close()
        profile_path = candidate / "graphics-corrections/profile.json"
        profile = json.loads(profile_path.read_bytes())
        profile.setdefault("world_bitmap_owners", []).append(policy)
        atomic_json(profile_path, profile)
        graphics = provenance["graphics_corrections"]
        graphics["profile_sha256"] = file_sha(profile_path)
        graphics["restored_mips"] += 51
        graphics["full_graphics_parity"] = False
        graphics["profiles"].append(dict(map=map_name.name, tag=plan["source"]["tag"], format=23,
            restored_frames=17, restored_mips=51, source_map_sha256=MAP_SHA, source_profile_sha256=SOURCE_PROFILE_SHA))
        for name in (map_name, bank_name):
            graphics["changes"].append(dict(path=name.as_posix(), donor_sha256=plan["pins"][str(name)],
                converted_sha256=file_sha(candidate / name), producer=Path(__file__).name, profile=policy["schema"]))
        graphics["scope"] += "; exact original17-frame OldMombasa water-beach bitmap and all source resource partitions"
        provenance["water_beach_bitmap_owner_v1"] = dict(policy_sha256=POLICY_SHA, source_profile_sha256=SOURCE_PROFILE_SHA,
            source=str(source_root), selected=False, original_reference_runtime_input=False,
            source_resources=plan["report"]["retained_source_sidecars"], software_selector_state_synthesized=False, **plan["result"])
        atomic_json(candidate / "graphics-conversion.json", graphics)
        atomic_json(candidate / "provenance.json", provenance)
        repair_owner_index(candidate, provenance, verified_beforeimage=verified)
        verify_owner_index(candidate, json.loads((candidate / "provenance.json").read_bytes()))
        for name, previous in plan["stamps"].items():
            require(stamp(source_root / name) == previous and file_sha(source_root / name) == plan["pins"][name],
                    "retained read-only prepared source changed")
        for name, raw in plan["sidecars"].items():
            require((candidate / name).read_bytes() == raw, "complete source sidecar readback differs")
        report = dict(plan["report"], candidate=str(candidate), prepared_derivative_created=True,
            source_or_prepared_assets_modified=True, owned_candidate_written=True,
            owner_index_verified=True, original_cache_unchanged=True,
            texture_bank_sha256=file_sha(candidate / bank_name), map_sha256=file_sha(candidate / map_name),
            candidate_beforeimage=candidate_pins,
            native_cache_admission="Required external all17-frame/51-mip check before candidate publication",
            sky_allocation_admission="Required exact before/after source-semantic projection check; no pixel claim")
        atomic_json(Path(report_path), report)
        return report
    except BaseException:
        for name, raw in before.items():
            atomic_bytes(candidate / name, raw)
        with (candidate / bank_name).open("r+b") as stream:
            stream.truncate(bank_extent)
        for name in plan["sidecars"]:
            (candidate / name).unlink(missing_ok=True)
        if (candidate / SIDECAR).exists():
            (candidate / SIDECAR).rmdir()
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    require(bool(args.plan) != bool(args.candidate), "choose read-only --plan or an existing owned --candidate")
    if args.plan:
        report = prepare_plan(args.repo_root, args.source)["report"]
        atomic_json(args.report, report)
    else:
        report = prepare(args.repo_root, args.source, args.candidate, args.report)
    print(json.dumps({key: report[key] for key in ("schema", "frames", "main_mips", "exact_hardware_partitions",
          "exact_low_detail_payloads", "datum", "address", "output_map_sha256", "texture_bank_appended_bytes")}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
