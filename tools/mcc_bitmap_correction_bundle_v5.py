"""Exact Elongation glass pixels, applied from an authenticated MCC-only delta.

Older bundles stay immutable. The authored original and MCC chains have the
same descriptor and nine mip owners; no image resampling or shader change is
part of this correction. Original bytes are offline maintenance inputs only.
"""
import argparse
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest, mip_hashes, mip_owners

BUNDLE_V5 = Path("tools/data/native-bitmap-corrections-v5.json.zlib")
SCHEMA_V5 = "H2_MCC_BITMAP_DELTA_V5"
MAP_NAME = "elongation.map"
SOURCE_MAP = "3780ab18cb5c65a0f23d99ab827671bc85c53c22c5e9a3826e5ab01425cff729"
REFERENCE_MAP = "8aa190e424003ce7efaea9e57be9556194b3aefdeccd8a86919f2534f7cf15ea"
PROFILE_V5 = dict(
    tag=r"scenarios\bitmaps\human\military\glass\glass_smudged_grey",
    encoding="rgba8", format=11, width=256, height=256, mip_count=9,
    pixel_bytes=349524, donor_registration=[128, 128],
    donor_sha256="1b811f460bc482850465cc6fe069328b03d5c98df8ee2ad1fca28bacf3a94e6e",
    target_sha256="9c11e22435fa1139de981cdbbe33f08076ed4f92ebe3373acdebd9136e350c70")
PROFILES_V5 = (PROFILE_V5,)


def read_v5(root):
    path = Path(root) / BUNDLE_V5
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 64 * 1024:
        raise FormatError("Excessive bitmap correction v5 manifest")
    try:
        manifest = json.loads(metadata.read_bytes())
        if (manifest.get("schema") != SCHEMA_V5 or manifest.get("map") != MAP_NAME or
                manifest.get("source_map_sha256") != SOURCE_MAP or
                manifest.get("original_map_sha256") != REFERENCE_MAP or
                manifest.get("full_graphics_parity") is not False or
                len(manifest["profiles"]) != 1):
            raise FormatError("Bitmap correction v5 identity differs")
        entry = manifest["profiles"][0]
        if (entry["profile"] != PROFILE_V5 or entry["mips"] != mip_owners(PROFILE_V5) or
                entry["delta_offset"] != 0 or type(entry["delta_bytes"]) is not int or
                not 0 < entry["delta_bytes"] <= PROFILE_V5["pixel_bytes"] + 65536 or
                manifest["delta_bytes"] != entry["delta_bytes"] or
                any(len(entry[key]) != 9 for key in ("donor_mip_sha256", "target_mip_sha256")) or
                path.stat().st_size != manifest["delta_bytes"]):
            raise FormatError("Bitmap correction v5 mip or delta ownership differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"] or digest(packed) != entry["delta_sha256"]:
            raise FormatError("Bitmap correction v5 fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        raise FormatError("Invalid bitmap correction v5 manifest") from error
    return path, manifest, packed


def reconstruct_v5(root, donors):
    """Validate and reconstruct all nine mip owners without writing assets."""
    path, manifest, packed = read_v5(root)
    if set(donors) != {PROFILE_V5["tag"]}:
        raise FormatError("Bitmap correction v5 donor set differs")
    donor = donors[PROFILE_V5["tag"]]
    entry = manifest["profiles"][0]
    if (len(donor) != PROFILE_V5["pixel_bytes"] or
            mip_hashes(donor, PROFILE_V5) != entry["donor_mip_sha256"]):
        raise FormatError("Bitmap correction v5 donor mips differ")
    result = decode_delta(donor, packed, source_sha256=PROFILE_V5["donor_sha256"],
                          target_sha256=PROFILE_V5["target_sha256"],
                          delta_sha256=manifest["delta_sha256"])
    if mip_hashes(result, PROFILE_V5) != entry["target_mip_sha256"]:
        raise FormatError("Bitmap correction v5 target mips differ")
    return [(PROFILE_V5, result)], path, manifest, packed


def create_v5(donors, targets, output):
    """Offline creation accepts only the fully authenticated paired chains."""
    output = Path(output)
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction v5 output already exists")
    if set(donors) != {PROFILE_V5["tag"]} or set(targets) != {PROFILE_V5["tag"]}:
        raise FormatError("Bitmap correction v5 creation set differs")
    donor, target = donors[PROFILE_V5["tag"]], targets[PROFILE_V5["tag"]]
    if (len(donor) != PROFILE_V5["pixel_bytes"] or len(target) != PROFILE_V5["pixel_bytes"] or
            digest(donor) != PROFILE_V5["donor_sha256"] or digest(target) != PROFILE_V5["target_sha256"]):
        raise FormatError("Bitmap correction v5 creation images differ")
    packed = zlib.compress(bytes(a ^ b for a, b in zip(donor, target, strict=True)), 9)
    decode_delta(donor, packed, source_sha256=PROFILE_V5["donor_sha256"],
                 target_sha256=PROFILE_V5["target_sha256"], delta_sha256=digest(packed))
    entry = dict(profile=PROFILE_V5, mips=mip_owners(PROFILE_V5),
                 donor_mip_sha256=mip_hashes(donor, PROFILE_V5),
                 target_mip_sha256=mip_hashes(target, PROFILE_V5),
                 delta_offset=0, delta_bytes=len(packed), delta_sha256=digest(packed))
    manifest = dict(schema=SCHEMA_V5, map=MAP_NAME, source_map_sha256=SOURCE_MAP,
                    original_map_sha256=REFERENCE_MAP, profiles=[entry],
                    delta_bytes=len(packed), delta_sha256=digest(packed),
                    full_graphics_parity=False,
                    scope="Authenticated original Elongation glass_smudged_grey nine-mip chain; no resampling, shader edits, or other bitmap changes")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packed)
    output.with_suffix("").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--donor", type=Path, required=True)
    parser.add_argument("--target", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    create_v5({PROFILE_V5["tag"]: args.donor.read_bytes()},
              {PROFILE_V5["tag"]: args.target.read_bytes()}, args.output)
