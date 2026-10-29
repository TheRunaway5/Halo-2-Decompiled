#!/usr/bin/env python3
"""Prepare an unselected COW candidate with the complete V4/V5/V6 corrections.

Use tools/run_test.py --scratch-root .local/test-scratch. The complete staged
copy stays in HALO2_TEST_SCRATCH until qualification; only a passing candidate
is renamed to the explicit new --output directory. Existing caches, original
references and asset registries are read-only. Normal import uses bundled deltas
through mcc_graphics_corrections instead of this incremental candidate tool.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import uuid

from audit_rendering import FormatError, file_sha
from mcc_bitmap_correction_bundle import digest, mip_hashes
from mcc_bitmap_correction_bundle_v5 import (BUNDLE_V5, MAP_NAME, PROFILE_V5,
                                            SOURCE_MAP, reconstruct_v5)
from mcc_bitmap_correction_bundle_v4 import (MAP_NAME as V4_MAP_NAME,
    PROFILE_V4, SOURCE_MAP as V4_SOURCE_MAP, reconstruct_v4)
from mcc_bitmap_correction_bundle_v6 import (OWNERS_V6, PROFILE_V6,
    reconstruct_v6, validate_owner_v6)
from mcc_content import MccCache, texture_resource
from mcc_native_map_owner_publication import verify_owner_index, repair_owner_index
from prepare_mcc_gameplay import save_changed_cache

MAP = "halo2/h2_maps_win64_dx11/" + MAP_NAME
V4_MAP = "halo2/h2_maps_win64_dx11/" + V4_MAP_NAME
BANK = "halo2/h2_maps_win64_dx11/textures.dat"


def atomic_bytes(path, data):
    temporary = path.with_name("." + path.name + ".v5-" + uuid.uuid4().hex)
    try:
        temporary.write_bytes(data)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def atomic_json(path, value):
    atomic_bytes(path, (json.dumps(value, indent=2, sort_keys=True) + "\n").encode())


def unshare(path):
    """Detach the output inode before direct cache writes or bank appends."""
    temporary = path.with_name("." + path.name + ".v5-" + uuid.uuid4().hex)
    try:
        subprocess.run(["cp", "--reflink=always", "--preserve=mode,timestamps", "--",
                        str(path), str(temporary)], check=True, capture_output=True, text=True)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def prefix_digest(path, extent):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        while extent:
            block = stream.read(min(extent, 4 * 1024 * 1024))
            if not block:
                raise FormatError("COW bank prefix was truncated")
            result.update(block)
            extent -= len(block)
    return result.hexdigest()


def bitmap(cache, profile=PROFILE_V5, *, donor=True):
    tags = [tag for tag in cache.tags.values() if tag["group"] == "bitm" and
            tag["name"] == profile["tag"] and tag["address"]]
    if len(tags) != 1:
        raise FormatError("Elongation glass bitmap identity is ambiguous")
    rows = cache.block(tags[0]["offset"] + 68, 168)
    if len(rows) != 1:
        raise FormatError("Elongation glass frame count differs")
    at = rows[0]
    descriptor = (cache.u16(at + 4), cache.u16(at + 6), cache.data[at + 8],
                  cache.u16(at + 10), cache.u16(at + 12), cache.u16(at + 20))
    width = profile.get("donor_width", profile["width"]) if donor else profile["width"]
    height = profile.get("donor_height", profile["height"]) if donor else profile["height"]
    mips = profile.get("donor_mip_count", profile["mip_count"]) if donor else profile["mip_count"]
    registration = profile.get("donor_registration") if donor else profile.get("registration", profile.get("donor_registration"))
    image_format = profile.get("donor_format", profile["format"]) if donor else profile["format"]
    if (descriptor != (width, height, 1, 0, image_format, mips - 1) or
            (registration is not None and list(struct.unpack_from("<hh", cache.data, at + 16)) != registration) or
            cache.u32(at + 84) or cache.u32(at + 88)):
        raise FormatError("Elongation glass descriptor differs")
    return at


def rgba(cache, bank, profile=PROFILE_V5, *, donor=True):
    at = bitmap(cache, profile, donor=donor)
    maximum = profile.get("donor_pixel_bytes", profile["pixel_bytes"]) if donor else profile["pixel_bytes"]
    with Path(bank).open("rb") as stream:
        pixels = bytearray(texture_resource(stream, cache.u32(at + 56), cache.u32(at + 80),
                                           decoded_limit=maximum))
    encoding = profile.get("donor_encoding", profile["encoding"]) if donor else profile["encoding"]
    if encoding == "rgba8":
        pixels[0::4], pixels[2::4] = pixels[2::4], pixels[0::4]
    return pixels


def allocated_unshared(path):
    """Linux FIEMAP extents; None means this filesystem cannot report sharing."""
    try:
        import fcntl
    except ImportError:
        return None
    try:
        position = total = 0
        with path.open("rb") as stream:
            while True:
                request = bytearray(struct.pack("<QQIIII", position, 0xffffffffffffffff - position,
                                                1, 0, 256, 0) + bytes(256 * 56))
                fcntl.ioctl(stream.fileno(), 0xc020660b, request, True)
                count = struct.unpack_from("<I", request, 20)[0]
                if not count:
                    return total
                last = False
                for index in range(count):
                    logical, _, length = struct.unpack_from("<QQQ", request, 32 + index * 56)
                    flags = struct.unpack_from("<I", request, 32 + index * 56 + 40)[0]
                    if not flags & 0x2000:
                        total += length
                    position = logical + length
                    last = bool(flags & 1)
                if last:
                    return total
    except (OSError, ValueError):
        return None


def sizes(root):
    logical = allocated = private = 0
    physical_complete = True
    inodes = set()
    files = 0
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        stat = path.stat();logical += stat.st_size;files += 1
        key = stat.st_dev, stat.st_ino
        if key in inodes:
            continue
        inodes.add(key);allocated += stat.st_blocks * 512
        if stat.st_nlink == 1:
            extent = allocated_unshared(path)
            if extent is None:
                physical_complete = False
            else:
                private += extent
    return dict(files=files, logical_bytes=logical,
                allocated_bytes_including_shared_extents=allocated,
                incremental_unshared_extent_bytes=private,
                incremental_physical_measurement_complete=physical_complete)


def prepare(root, source, output, report_path):
    root, source = Path(root).resolve(strict=True), Path(source).resolve(strict=True)
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = Path(output).resolve()
    if output.exists() or output == source or output.is_relative_to(source) or source.is_relative_to(output):
        raise FormatError("Choose a new candidate output outside the source cache")
    if scratch.stat().st_dev != source.stat().st_dev or output.parent.stat().st_dev != source.stat().st_dev:
        raise FormatError("COW source, owned scratch and candidate must share one filesystem")
    registry = root / "assets/mcc-source.tsv"
    registry_before = registry.read_bytes() if registry.exists() else None
    cohort = [(V4_MAP, V4_SOURCE_MAP, PROFILE_V4, reconstruct_v4),
              (MAP, SOURCE_MAP, PROFILE_V5, reconstruct_v5)]
    cohort.extend(("halo2/h2_maps_win64_dx11/" + owner["map"],
                   owner["source_map_sha256"], PROFILE_V6, reconstruct_v6)
                  for owner in OWNERS_V6)
    map_names = list(dict.fromkeys(name for name, _, _, _ in cohort))
    names = (*map_names, BANK, "provenance.json", "native-shaders/map-owners.json")
    before = {name: file_sha(source / name) for name in names}
    for name, pin, _, _ in cohort:
        if before[name] != pin:
            raise FormatError("COW source map differs from the authenticated correction donor: " + name)
    provenance = json.loads((source / "provenance.json").read_text())
    if verify_owner_index(source, provenance) is None:
        raise FormatError("Existing prepared cache needs its authenticated owner index")
    corrections = []
    for name, _, profile, reconstruct in cohort:
        cache = MccCache(source / name)
        try:
            donor = rgba(cache, source / BANK, profile)
            at = bitmap(cache, profile)
            if reconstruct is reconstruct_v6:
                tag = next(tag for tag in cache.tags.values() if tag["name"] == profile["tag"] and tag["address"])
                validate_owner_v6(Path(name).name, cache, tag, at)
            staged, bundle_path, manifest, packed = reconstruct(root, {profile["tag"]: donor})
            corrections.append(dict(name=name, profile=profile, donor=donor,
                target=staged[0][1], bundle_path=bundle_path, manifest=manifest,
                packed=packed, descriptor_before=bytes(cache.data[at:at + 168])))
        finally:
            cache.close()
    stage = scratch / "elongation-glass-candidate"
    try:
        shutil.copytree(source, stage, copy_function=os.link, symlinks=True)
        stage_provenance = json.loads((stage / "provenance.json").read_text())
        verified = verify_owner_index(stage, stage_provenance)
        for name in (*map_names, BANK):
            unshare(stage / name)
        if any((stage / name).stat().st_ino == (source / name).stat().st_ino for name in (*map_names, BANK)):
            raise FormatError("COW bitmap/map inode remained shared with the selected cache")
        bank_extent = (stage / BANK).stat().st_size
        appended_bytes, resources = 0, {}
        # Shared original Earth pixels own one appended resource, while each
        # local map retains its independently authenticated tag/descriptor.
        with (stage / BANK).open("ab") as stream:
            for correction in corrections:
                profile, target = correction["profile"], correction["target"]
                native = bytearray(target)
                if profile["encoding"] == "rgba8":
                    native[0::4], native[2::4] = target[2::4], target[0::4]
                identity = (len(native), digest(native))
                if identity not in resources:
                    partition = struct.pack("<Ii", 1, -len(native)) + native
                    pointer = bank_extent + appended_bytes
                    if pointer + len(partition) >= 0x40000000:
                        raise FormatError("Corrected texture bank exceeds resource address capacity")
                    stream.write(partition)
                    resources[identity] = (pointer, len(partition))
                    appended_bytes += len(partition)
                pointer, resource_bytes = resources[identity]
                fields = [(56, struct.pack("<I", pointer)), (80, struct.pack("<I", resource_bytes))]
                if "donor_width" in profile:
                    fields += [(4, struct.pack("<H", profile["width"])),
                        (6, struct.pack("<H", profile["height"])),
                        (12, struct.pack("<H", profile["format"])),
                        (16, struct.pack("<hh", *profile["registration"])),
                        (20, struct.pack("<H", profile["mip_count"] - 1))]
                correction["fields"] = fields
        # Both Elongation profiles mutate a single current cache and commit
        # once; reopening a stale donor for the second profile loses the first.
        for name in map_names:
            changed = MccCache(stage / name)
            patches = []
            try:
                for correction in corrections:
                    if correction["name"] != name:
                        continue
                    at = bitmap(changed, correction["profile"])
                    for field, raw in correction["fields"]:
                        changed.data[at + field:at + field + len(raw)] = raw
                        patches.append((at + field, raw))
                if changed.u16(28) & 1:
                    save_changed_cache(changed)
                else:
                    with changed.path.open("r+b") as stream:
                        for at, raw in patches:
                            stream.seek(at);stream.write(raw)
                check = MccCache(changed.path)
                try:
                    if check.data != changed.data:
                        raise FormatError("COW metadata patch changed an unrelated map byte")
                finally:
                    check.close()
            finally:
                changed.close()
        for correction in corrections:
            name, profile, target = correction["name"], correction["profile"], correction["target"]
            actual = MccCache(stage / name)
            try:
                pixels = rgba(actual, stage / BANK, profile, donor=False)
                at = bitmap(actual, profile, donor=False)
                unchanged = bytearray(actual.data[at:at + 168])
                for field, raw in correction["fields"]:
                    unchanged[field:field + len(raw)] = correction["descriptor_before"][field:field + len(raw)]
                if bytes(unchanged) != correction["descriptor_before"] or pixels != target:
                    raise FormatError("Correction changed an unowned descriptor field or target pixel")
                correction["target_mip_sha256"] = mip_hashes(pixels, profile)
                correction["descriptor_changed_fields"] = sorted(field for field, _ in correction["fields"])
            finally:
                actual.close()
        if prefix_digest(stage / BANK, bank_extent) != before[BANK]:
            raise FormatError("COW correction changed the existing texture bank prefix")
        correction_dir = stage / "graphics-corrections"
        current_manifest = json.loads((correction_dir / "profile.json").read_text())
        current_manifest["schema"] = "H2_MCC_GRAPHICS_CORRECTIONS_V6"
        bundled = {correction["bundle_path"]: correction for correction in corrections}
        current_manifest.setdefault("bundles", []).extend(correction["manifest"] for correction in bundled.values())
        atomic_json(correction_dir / "profile.json", current_manifest)
        for correction in bundled.values():
            atomic_bytes(correction_dir / correction["bundle_path"].name, correction["packed"])
            atomic_json(correction_dir / correction["bundle_path"].with_suffix("").name, correction["manifest"])
        graphics = stage_provenance["graphics_corrections"]
        restored_mips = graphics["restored_mips"] + sum(correction["profile"]["mip_count"] for correction in corrections)
        graphics.update(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V6",
                        profile_sha256=file_sha(correction_dir / "profile.json"),
                        restored_mips=restored_mips, full_graphics_parity=False,
                        scope=graphics.get("scope", "") + "; exact Sacred Icon V4 gradient, Elongation V5 glass and five V6 Earth owners")
        for correction in corrections:
            profile = correction["profile"]
            graphics["profiles"].append(dict(map=Path(correction["name"]).name, tag=profile["tag"],
                restored_mips=profile["mip_count"], format=profile["format"], target_sha256=profile["target_sha256"]))
        for name in (*map_names, BANK):
            graphics["changes"].append(dict(path=name, donor_sha256=before[name],
                converted_sha256=file_sha(stage / name), producer=Path(__file__).name,
                profile="H2_MCC_ELONGATION_GLASS_COW_CANDIDATE_V3"))
        stage_provenance["graphics_v6_candidate"] = dict(source=str(source),
            candidate=str(output), selected=False, original_reference_runtime_input=False)
        atomic_json(stage / "graphics-conversion.json", graphics)
        atomic_json(stage / "provenance.json", stage_provenance)
        repair_owner_index(stage, stage_provenance, verified_beforeimage=verified)
        final_provenance = json.loads((stage / "provenance.json").read_text())
        verify_owner_index(stage, final_provenance)
        from mcc_onboarding import _needs_graphics_reimport, private_cache
        if _needs_graphics_reimport(root, final_provenance) or restored_mips != 97:
            raise FormatError("COW candidate lacks the complete current97-mip correction cohort")
        if any(file_sha(source / name) != pin for name, pin in before.items()) or (
                registry.read_bytes() if registry.exists() else None) != registry_before:
            raise FormatError("Selected cache or asset registry changed during preparation")
        retained = sizes(stage)
        output.parent.mkdir(parents=True, exist_ok=True)
        stage.rename(output)
        private_cache(root, output, verify=True)
        if _needs_graphics_reimport(root, json.loads((output / "provenance.json").read_text())):
            raise FormatError("Published candidate failed latest graphics freshness")
        report = dict(schema="H2_MCC_ELONGATION_GLASS_COW_CANDIDATE_V3", passed=True,
            source=str(source), candidate=str(output), candidate_selected=False,
            corrections=[dict(map=correction["name"], tag=correction["profile"]["tag"],
                donor_pixels_sha256=digest(correction["donor"]), target_pixels_sha256=digest(correction["target"]),
                target_mip_sha256=correction["target_mip_sha256"],
                descriptor_changed_fields=correction["descriptor_changed_fields"]) for correction in corrections],
            restored_mips=restored_mips, latest_graphics_freshness_passed=True, private_cache_verified=True, retained=retained,
            source_sha256_before=before,
            source_unchanged=True, registry_unchanged=True, original_assets_unchanged=True,
            separate_map_and_bank_inodes=True, existing_bank_prefix_unchanged=True,
            appended_resource_bytes=appended_bytes, appended_resource_count=len(resources),
            map_commit_count=len(map_names), output_sha256={name: file_sha(output / name) for name in names},
            owner_index_verified=True, scratch_stage_removed=not stage.exists(),
            full_graphics_parity=False)
        atomic_json(Path(report_path), report)
        print(json.dumps(report, indent=2))
        return report
    finally:
        if stage.exists():
            shutil.rmtree(stage)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.root, args.source, args.output, args.report)
