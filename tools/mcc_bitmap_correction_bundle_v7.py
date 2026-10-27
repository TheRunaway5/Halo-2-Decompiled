"""Authenticated original native sky chains reconstructed from exact MCC bytes.

The manifest pins every local owner and every mip. Prefix XOR is storage only:
there is no image decoder, resampling, compression encoder or shader execution.
Original caches are offline maintenance references and never runtime inputs.
"""
from pathlib import Path
import json

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest

BUNDLE_V7 = Path("tools/data/native-bitmap-corrections-v7.json.zlib")
SCHEMA_V7 = "H2_MCC_BITMAP_DELTA_V7"
MANIFEST_SHA256 = "c3f23db58c268f5f4d0b81f46e62fdbba010c36aaaff964405465342401a6fe2"


def mip_owners_v7(profile):
    result, offset = [], 0
    if profile["encoding"] not in ("rgba8", "bc1", "bc3"):
        raise FormatError("Unsupported authenticated sky storage format")
    for level in range(profile["mip_count"]):
        width, height = max(1, profile["width"] >> level), max(1, profile["height"] >> level)
        size = width * height * 4 if profile["encoding"] == "rgba8" else (
            ((width + 3) // 4) * ((height + 3) // 4) * (8 if profile["encoding"] == "bc1" else 16))
        result.append(dict(width=width, height=height, depth=1, offset=offset, size=size))
        offset += size
    if offset != profile["pixel_bytes"]:
        raise FormatError("Sky correction complete mip extent differs")
    return result


def donor_profile(profile):
    return {key: profile.get("donor_" + key, profile[key])
            for key in ("encoding", "width", "height", "mip_count", "pixel_bytes")}


def hashes(pixels, profile):
    return [digest(pixels[m["offset"]:m["offset"] + m["size"]]) for m in mip_owners_v7(profile)]


def read_v7(root):
    path = Path(root) / BUNDLE_V7
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 128 * 1024:
        raise FormatError("Excessive sky bitmap correction manifest")
    raw = metadata.read_bytes()
    if digest(raw) != MANIFEST_SHA256:
        raise FormatError("Sky bitmap correction complete manifest fingerprint differs")
    try:
        manifest = json.loads(raw)
        if (manifest["schema"] != SCHEMA_V7 or manifest["full_graphics_parity"] is not False or
                len(manifest["profiles"]) != 5 or len(manifest["owners"]) != 11 or
                path.stat().st_size != manifest["delta_bytes"]):
            raise FormatError("Sky bitmap correction identity or extent differs")
        offset = 0
        for entry in manifest["profiles"]:
            profile = entry["profile"]
            if (entry["delta_offset"] != offset or entry["mips"] != mip_owners_v7(profile) or
                    entry["donor_mips"] != mip_owners_v7(donor_profile(profile)) or
                    entry["base_bytes"] != profile["pixel_bytes"] or entry["base_offset"] != 0 or
                    profile["pixel_bytes"] > profile["donor_pixel_bytes"] or
                    len(entry["target_mip_sha256"]) != profile["mip_count"] or
                    len(entry["donor_mip_sha256"]) != profile["donor_mip_count"]):
                raise FormatError("Sky bitmap correction mip ownership differs")
            offset += entry["delta_bytes"]
        if offset != manifest["delta_bytes"]:
            raise FormatError("Sky bitmap correction delta ownership differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"]:
            raise FormatError("Sky bitmap correction complete delta fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        raise FormatError("Invalid authenticated sky bitmap correction manifest") from error
    return path, manifest, packed


def profiles_for_map_v7(root, name):
    _, manifest, _ = read_v7(root)
    ids = {owner["profile_id"] for owner in manifest["owners"] if owner["map"] == name}
    return [entry["profile"] for entry in manifest["profiles"] if entry["profile_id"] in ids]


def validate_owner_v7(root, name, cache, tag, at):
    _, manifest, _ = read_v7(root)
    owners = [o for o in manifest["owners"] if o["map"] == name and o["tag"] == tag["name"]]
    if len(owners) != 1:
        raise FormatError("Sky bitmap correction has no unique authenticated owner")
    owner = owners[0]
    if (tag["datum"] != owner["datum"] or tag["address"] != owner["address"] or
            tag["size"] != owner["tag_size"] or
            digest(cache.data[at:at + 168]) != owner["descriptor_sha256"]):
        raise FormatError("Sky bitmap correction complete local owner differs")
    return owner


def reconstruct_v7(root, donors, *, map_name=None):
    path, manifest, packed = read_v7(root)
    selected_ids = None if map_name is None else {
        owner["profile_id"] for owner in manifest["owners"] if owner["map"] == map_name}
    entries = [entry for entry in manifest["profiles"]
               if selected_ids is None or entry["profile_id"] in selected_ids]
    if not entries or set(donors) != {entry["profile"]["tag"] for entry in entries}:
        raise FormatError("Sky bitmap correction exact donor set differs")
    images = []
    for entry in entries:
        profile = entry["profile"]
        donor = donors[profile["tag"]]
        if (len(donor) != profile["donor_pixel_bytes"] or digest(donor) != profile["donor_sha256"] or
                hashes(donor, donor_profile(profile)) != entry["donor_mip_sha256"]):
            raise FormatError("Sky bitmap correction complete donor mips differ")
        delta = packed[entry["delta_offset"]:entry["delta_offset"] + entry["delta_bytes"]]
        result = decode_delta(donor[:profile["pixel_bytes"]], delta,
            source_sha256=entry["base_sha256"], target_sha256=profile["target_sha256"],
            delta_sha256=entry["delta_sha256"])
        if hashes(result, profile) != entry["target_mip_sha256"]:
            raise FormatError("Sky bitmap correction complete target mips differ")
        images.append((profile, result))
    return images, path, manifest, packed
