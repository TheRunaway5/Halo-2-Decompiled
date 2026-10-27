"""Exact original decal mip chains reconstructed from fingerprinted MCC pixels.

The independent decal namespace composes with the immutable V1–V7 bundles.
Only offline maintenance reads original assets; preparation uses MCC and the
small authenticated XOR deltas. This does not claim framebuffer parity.
"""
import json
from pathlib import Path

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest, mip_hashes, mip_owners

BUNDLE = Path("tools/data/native-decal-bitmap-corrections-v1.json.zlib")
SCHEMA = "H2_MCC_DECAL_BITMAP_DELTA_V1"
MANIFEST_SHA256 = "71ef803ba57cfd76f1b08453d3fcbc68f213e01104b4d06bb4121328efd943b1"
PROFILE_DIMENSIONS = {"ice": (256, 256), "impact_scratch": (128, 64), "metal": (128, 128), "stone": (128, 128)}


def read_bundle(root):
    path = Path(root) / BUNDLE
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 256 * 1024:
        raise FormatError("Excessive decal bitmap correction manifest")
    raw = metadata.read_bytes()
    if digest(raw) != MANIFEST_SHA256:
        raise FormatError("Decal bitmap correction complete manifest fingerprint differs")
    try:
        manifest = json.loads(raw)
        if (manifest["schema"] != SCHEMA or manifest["full_graphics_parity"] is not False or
                {e["profile_id"] for e in manifest["profiles"]} != set(PROFILE_DIMENSIONS) or
                len(manifest["profiles"]) != 4 or not manifest["owners"] or
                path.stat().st_size != manifest["delta_bytes"] or manifest["delta_bytes"] > 1024 * 1024):
            raise FormatError("Decal bitmap correction identity or extent differs")
        offset = 0
        for entry in manifest["profiles"]:
            p = entry["profile"]
            w, h = PROFILE_DIMENSIONS[entry["profile_id"]]
            if (p["tag"] != "effects\\decals\\bullet_holes\\bitmaps\\" + entry["profile_id"] or
                    (p["encoding"], p["format"], p["width"], p["height"], p["mip_count"]) != ("rgba8", 11, w, h, 3) or
                    entry["delta_offset"] != offset or entry["mips"] != mip_owners(p) or
                    len(entry["donor_mip_sha256"]) != 3 or len(entry["target_mip_sha256"]) != 3 or
                    not 0 < entry["delta_bytes"] <= p["pixel_bytes"] + 65536):
                raise FormatError("Decal bitmap correction mip or delta ownership differs")
            offset += entry["delta_bytes"]
        if offset != manifest["delta_bytes"]:
            raise FormatError("Decal bitmap correction complete delta extent differs")
        owner_keys = {(o["map"], o["tag"], o["frame"]) for o in manifest["owners"]}
        if len(owner_keys) != len(manifest["owners"]) or any(
                o["profile_id"] not in PROFILE_DIMENSIONS or o["frame"] != 0 or
                Path(o["map"]).name != o["map"] or not o["map"].endswith(".map")
                for o in manifest["owners"]):
            raise FormatError("Decal bitmap correction local owner directory differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"]:
            raise FormatError("Decal bitmap correction complete delta fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        raise FormatError("Invalid authenticated decal bitmap correction manifest") from error
    return path, manifest, packed


def profiles_for_map(root, name):
    _, manifest, _ = read_bundle(root)
    ids = {o["profile_id"] for o in manifest["owners"] if o["map"] == name}
    return [e["profile"] for e in manifest["profiles"] if e["profile_id"] in ids]


def validate_owner(root, name, cache, tag, at):
    _, manifest, _ = read_bundle(root)
    owners = [o for o in manifest["owners"] if o["map"] == name and o["tag"] == tag["name"]]
    if len(owners) != 1:
        raise FormatError("Decal bitmap correction has no unique authenticated owner")
    owner = owners[0]
    if (tag["group"] != "bitm" or tag["datum"] != owner["datum"] or tag["address"] != owner["address"] or
            tag["size"] != owner["tag_size"] or cache.block(tag["offset"] + 68, 168) != [at] or
            digest(cache.data[at:at + 168]) != owner["descriptor_sha256"]):
        raise FormatError("Decal bitmap correction complete local owner differs")
    return owner


def reconstruct(root, donors, *, map_name=None):
    path, manifest, packed = read_bundle(root)
    ids = None if map_name is None else {o["profile_id"] for o in manifest["owners"] if o["map"] == map_name}
    entries = [e for e in manifest["profiles"] if ids is None or e["profile_id"] in ids]
    if not entries or set(donors) != {e["profile"]["tag"] for e in entries}:
        raise FormatError("Decal bitmap correction exact donor set differs")
    images = []
    for entry in entries:
        profile = entry["profile"]
        donor = donors[profile["tag"]]
        if (len(donor) != profile["pixel_bytes"] or digest(donor) != profile["donor_sha256"] or
                mip_hashes(donor, profile) != entry["donor_mip_sha256"]):
            raise FormatError("Decal bitmap correction complete donor mips differ")
        delta = packed[entry["delta_offset"]:entry["delta_offset"] + entry["delta_bytes"]]
        target = decode_delta(donor, delta, source_sha256=profile["donor_sha256"],
            target_sha256=profile["target_sha256"], delta_sha256=entry["delta_sha256"])
        if mip_hashes(target, profile) != entry["target_mip_sha256"]:
            raise FormatError("Decal bitmap correction complete target mips differ")
        images.append((profile, target))
    return images, path, manifest, packed
