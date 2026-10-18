"""Verified pixel deltas applied to an owned MCC bitmap during onboarding.

The bundle contains XOR differences rather than an original bitmap bank. An
exact MCC input and exact reconstructed output are both required. Creation is
offline maintenance; importing needs only this bundle and the MCC texture bank.
"""
import argparse
import hashlib
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError, file_sha

BUNDLE = Path("tools/data/native-bitmap-corrections.json.zlib")
SCHEMA = "H2_MCC_BITMAP_DELTA_V1"
PIXEL_BYTES = 1398100
DONOR_SHA = "e3bf56db0a97306a9a6e7e5efa48a6e050e808464d23e65db3e443bcfdbae915"
TARGET_SHA = "e73e664e6b6fa60d5f245ca7b3edac4d00200f38bc8a67ee12330666e30e063c"
REFERENCE_MAP = "2000bfc26fa38929e3ae8792618465ca5a60cc6bbeb753426e9d3a99954bdafb"

# Keep V1 immutable: published builds already carry its concrete-only delta.
BUNDLE_V2 = Path("tools/data/native-bitmap-corrections-v2.json.zlib")
SCHEMA_V2 = "H2_MCC_BITMAP_DELTA_V2"
GREY_TAG = r"scenarios\bitmaps\human\urban\metals\matte_grey"
PROFILES_V2 = (
    dict(tag=r"scenarios\bitmaps\multi\zanzibar\massive_concrete_bump",
         encoding="rgba8", format=11, width=512, height=512, mip_count=10,
         pixel_bytes=PIXEL_BYTES, donor_sha256=DONOR_SHA, target_sha256=TARGET_SHA),
    dict(tag=GREY_TAG, encoding="bc3", format=16, width=256, height=256, mip_count=7,
         pixel_bytes=87376,
         donor_sha256="cb0d67d51093c58594944262febc2abc2dab789de244f8cb4de2cc24495fa334",
         target_sha256="7221dd1f5eac5b5e65c40a5dfcef4942a9455e94c9afa55e181aab9ff6b807ee"),
)


def mip_owners(profile):
    """Serialized byte owners, including BC3 blocks; no resampling or encoding."""
    result, offset = [], 0
    for level in range(profile["mip_count"]):
        width, height = max(1, profile["width"] >> level), max(1, profile["height"] >> level)
        size = (((width + 3) // 4) * ((height + 3) // 4) * 16
                if profile["encoding"] == "bc3" else width * height * 4)
        result.append(dict(width=width, height=height, depth=1, offset=offset, size=size))
        offset += size
    if offset != profile["pixel_bytes"]:
        raise FormatError("Bitmap correction mip extent differs")
    return result


def mip_hashes(pixels, profile):
    return [digest(pixels[mip["offset"]:mip["offset"] + mip["size"]])
            for mip in mip_owners(profile)]


def read_v2(root):
    """Validate the entire bundle directory and byte ownership before use."""
    from mcc_graphics_corrections import SOURCE_MAP
    path = Path(root) / BUNDLE_V2
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 64 * 1024:
        raise FormatError("Excessive bitmap correction manifest")
    try:
        manifest = json.loads(metadata.read_bytes())
        if (not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA_V2 or
                manifest.get("source_map_sha256") != SOURCE_MAP or
                manifest.get("original_map_sha256") != REFERENCE_MAP or
                manifest.get("full_graphics_parity") is not False or
                len(manifest["profiles"]) != len(PROFILES_V2)):
            raise FormatError("Bitmap correction bundle identity differs")
        offset = 0
        for entry, expected in zip(manifest["profiles"], PROFILES_V2, strict=True):
            if (entry["profile"] != expected or entry["mips"] != mip_owners(expected) or
                    entry["delta_offset"] != offset or
                    type(entry["delta_bytes"]) is not int or
                    not 0 < entry["delta_bytes"] <= expected["pixel_bytes"] + 65536 or
                    any(len(entry[k]) != expected["mip_count"]
                        for k in ("donor_mip_sha256", "target_mip_sha256"))):
                raise FormatError("Bitmap correction profile or delta owner differs")
            offset += entry["delta_bytes"]
        if manifest["delta_bytes"] != offset or path.stat().st_size != offset:
            raise FormatError("Bitmap correction bundle extent differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"]:
            raise FormatError("Bitmap correction bundle fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError) as error:
        raise FormatError("Invalid bitmap correction manifest") from error
    return path, manifest, packed


def reconstruct_v2(root, donors):
    """Stage both exact corrections. This function never writes cache bytes."""
    path, manifest, packed = read_v2(root)
    if set(donors) != {p["tag"] for p in PROFILES_V2}:
        raise FormatError("Bitmap correction donor set differs")
    staged = []
    for entry in manifest["profiles"]:
        profile = entry["profile"]
        donor = donors[profile["tag"]]
        if (len(donor) != profile["pixel_bytes"] or
                mip_hashes(donor, profile) != entry["donor_mip_sha256"]):
            raise FormatError("Bitmap correction donor mip differs")
        offset, size = entry["delta_offset"], entry["delta_bytes"]
        result = decode_delta(donor, packed[offset:offset + size],
                              source_sha256=profile["donor_sha256"],
                              target_sha256=profile["target_sha256"],
                              delta_sha256=entry["delta_sha256"])
        if mip_hashes(result, profile) != entry["target_mip_sha256"]:
            raise FormatError("Bitmap correction target mip differs")
        staged.append((profile, result))
    return staged, path, manifest, packed


def create_v2(donors, targets, output):
    """Offline generation from the two pinned donor/target tuples only.

    The raw BC3 original matte_grey evidence and all ten concrete RGBA mips
    are authenticated by their complete hashes above. Neither an original map
    nor an original bitmap bank is embedded or needed by reconstruct_v2().
    """
    from mcc_graphics_corrections import SOURCE_MAP
    output = Path(output)
    names = {p["tag"] for p in PROFILES_V2}
    if set(donors) != names or set(targets) != names:
        raise FormatError("Bitmap correction creation input set differs")
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction bundle output already exists")
    payload, entries = bytearray(), []
    for profile in PROFILES_V2:
        donor, target = donors[profile["tag"]], targets[profile["tag"]]
        if len(donor) != profile["pixel_bytes"] or len(target) != profile["pixel_bytes"]:
            raise FormatError("Bitmap correction creation extent differs")
        packed = zlib.compress(bytes(a ^ b for a, b in zip(donor, target, strict=True)), 9)
        decode_delta(donor, packed, source_sha256=profile["donor_sha256"],
                     target_sha256=profile["target_sha256"], delta_sha256=digest(packed))
        entries.append(dict(profile=profile, mips=mip_owners(profile),
                            donor_mip_sha256=mip_hashes(donor, profile),
                            target_mip_sha256=mip_hashes(target, profile),
                            delta_offset=len(payload), delta_bytes=len(packed), delta_sha256=digest(packed)))
        payload.extend(packed)
    manifest = dict(schema=SCHEMA_V2, source_map_sha256=SOURCE_MAP,
                    original_map_sha256=REFERENCE_MAP, profiles=entries,
                    delta_bytes=len(payload), delta_sha256=digest(payload),
                    full_graphics_parity=False,
                    scope="Authenticated Zanzibar concrete and matte_grey only; other maps and shaders unchanged")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    output.with_suffix("").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def create_v2_from_inspections(concrete_donor, concrete_reference, grey_donor, grey_reference, output):
    """Maintenance CLI inputs retain authenticated map and mip provenance."""
    from mcc_graphics_corrections import SOURCE_MAP
    def inspected(path, expected, map_sha):
        path = Path(path)
        inspection = json.loads((path / "bitmap.json").read_bytes())
        if (inspection.get("tag") != expected["tag"] or len(inspection["frames"]) != 1 or
                file_sha(Path(inspection["cache"])) != map_sha):
            raise FormatError("Bitmap correction inspection identity differs")
        frame = inspection["frames"][0]
        if (frame["width"], frame["height"], frame["format"], frame["layers"], frame["depth"]) != (
                expected["width"], expected["height"], expected["format"], 1, 0) or frame["mips"] != mip_owners(expected):
            raise FormatError("Bitmap correction inspection mip ownership differs")
        return (path / "frame-0.pixels").read_bytes()
    concrete, grey = PROFILES_V2
    profile_path = Path(concrete_reference)
    validate_profile(json.loads((profile_path / "profile.json").read_bytes()))
    donors = {concrete["tag"]: inspected(concrete_donor, concrete, SOURCE_MAP),
              grey["tag"]: inspected(grey_donor, grey, SOURCE_MAP)}
    targets = {concrete["tag"]: (profile_path / "zanzibar-concrete.rgba").read_bytes(),
               grey["tag"]: inspected(grey_reference, grey, REFERENCE_MAP)}
    return create_v2(donors, targets, output)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def decode_delta(donor, packed, *, source_sha256, target_sha256, delta_sha256):
    """Bound decompression to the caller's already validated donor extent."""
    if not donor or len(donor) > 8 * 1024 * 1024 or digest(donor) != source_sha256:
        raise FormatError("Bitmap correction donor pixels differ")
    if digest(packed) != delta_sha256:
        raise FormatError("Bitmap correction delta fingerprint differs")
    decoder = zlib.decompressobj()
    try:
        delta = decoder.decompress(packed, len(donor) + 1)
    except zlib.error as error:
        raise FormatError("Invalid bitmap correction compression") from error
    if len(delta) != len(donor) or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
        raise FormatError("Bitmap correction delta extent differs")
    result = bytes(a ^ b for a, b in zip(donor, delta, strict=True))
    if digest(result) != target_sha256:
        raise FormatError("Bitmap correction reconstructed pixels differ")
    return result


def validate_profile(profile):
    from mcc_graphics_corrections import SOURCE_MAP, TAG
    if (profile.get("schema") != "H2_MCC_GRAPHICS_CORRECTIONS_V1" or
            profile.get("tag") != TAG or profile.get("source_map_sha256") != SOURCE_MAP or
            profile.get("original_map_sha256") != REFERENCE_MAP or
            profile.get("rgba_sha256") != TARGET_SHA or profile.get("rgba_bytes") != PIXEL_BYTES or
            profile.get("reformation_base_corroborated") is not True):
        raise FormatError("Bundled bitmap correction reference differs")
    expected, offset = [], 0
    for level in range(10):
        width = 512 >> level
        size = width * width * 4
        expected.append(dict(width=width, height=width, depth=1, offset=offset, size=size))
        offset += size
    if profile.get("mips") != expected or offset != PIXEL_BYTES:
        raise FormatError("Bundled bitmap correction mip owners differ")


def reconstruct(root, donor):
    path = Path(root) / BUNDLE
    manifest = json.loads(path.with_suffix("").read_bytes())
    if (manifest.get("schema") != SCHEMA or manifest.get("donor_rgba_sha256") != DONOR_SHA or
            manifest.get("delta_bytes", 0) <= 0 or manifest["delta_bytes"] > PIXEL_BYTES + 65536):
        raise FormatError("Invalid bundled bitmap correction")
    profile = manifest["profile"]
    validate_profile(profile)
    if len(donor) != PIXEL_BYTES or path.stat().st_size != manifest["delta_bytes"]:
        raise FormatError("Bundled bitmap correction extent differs")
    packed = path.read_bytes()
    result = decode_delta(donor, packed, source_sha256=DONOR_SHA, target_sha256=TARGET_SHA,
                          delta_sha256=manifest["delta_sha256"])
    for mip, reference in zip(profile["mips"], profile["reference_mips"], strict=True):
        if digest(result[mip["offset"]:mip["offset"] + mip["size"]]) != reference["xbox_sha256"]:
            raise FormatError("Bundled bitmap correction reference mip differs")
    return profile, result, path, manifest


def create(donor_inspection, reference_profile, output):
    """Consume fingerprinted prior decoder evidence, never open an ISO/XBE."""
    from mcc_graphics_corrections import SOURCE_MAP, TAG
    donor_inspection, reference_profile, output = map(Path, (donor_inspection, reference_profile, output))
    inspection = json.loads((donor_inspection / "bitmap.json").read_bytes())
    if inspection.get("tag") != TAG or len(inspection["frames"]) != 1 or file_sha(Path(inspection["cache"])) != SOURCE_MAP:
        raise FormatError("Bitmap correction donor inspection identity differs")
    frame = inspection["frames"][0]
    if (frame["width"], frame["height"], frame["format"], frame["layers"], frame["depth"]) != (512, 512, 11, 1, 0):
        raise FormatError("Bitmap correction donor layout differs")
    donor = (donor_inspection / "frame-0.pixels").read_bytes()
    profile = json.loads((reference_profile / "profile.json").read_bytes())
    validate_profile(profile)
    target = (reference_profile / "zanzibar-concrete.rgba").read_bytes()
    if frame["mips"] != profile["mips"] or len(donor) != PIXEL_BYTES or len(target) != PIXEL_BYTES or digest(target) != TARGET_SHA:
        raise FormatError("Bitmap correction decoded extent or reference differs")
    delta = bytes(a ^ b for a, b in zip(donor, target, strict=True))
    packed = zlib.compress(delta, 9)
    # Verify the actual bounded importer path before publishing either file.
    decode_delta(donor, packed, source_sha256=DONOR_SHA, target_sha256=TARGET_SHA, delta_sha256=digest(packed))
    manifest = dict(schema=SCHEMA, donor_rgba_sha256=DONOR_SHA, delta_sha256=digest(packed),
                    delta_bytes=len(packed), profile=profile, full_graphics_parity=False,
                    scope="Zanzibar massive concrete: exact ten original mips; other map and shader bindings unchanged")
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction bundle output already exists")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packed)
    output.with_suffix("").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("donor_inspection", type=Path)
    parser.add_argument("reference_profile", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--matte-grey-donor", type=Path)
    parser.add_argument("--matte-grey-reference", type=Path)
    args = parser.parse_args()
    if bool(args.matte_grey_donor) != bool(args.matte_grey_reference):
        parser.error("Both matte-grey inspection directories are required for v2")
    if args.matte_grey_donor:
        report = create_v2_from_inspections(args.donor_inspection, args.reference_profile,
                                            args.matte_grey_donor, args.matte_grey_reference, args.output)
    else:
        report = create(args.donor_inspection, args.reference_profile, args.output)
    print(json.dumps(dict(bytes=report["delta_bytes"], sha256=report["delta_sha256"])))


if __name__ == "__main__":
    main()
