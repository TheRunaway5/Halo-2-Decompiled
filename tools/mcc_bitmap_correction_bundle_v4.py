"""Exact Sacred Icon gradient restoration from a fingerprinted MCC donor.

V1/V2/V3 stay immutable. MCC's mip1-and-below is the same byte extent as
the original chain, but its pixels differ. A bounded XOR delta restores the
eight original mips; the importer separately restores their descriptor.
"""
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest, mip_hashes, mip_owners

BUNDLE_V4 = Path("tools/data/native-bitmap-corrections-v4.json.zlib")
SCHEMA_V4 = "H2_MCC_BITMAP_DELTA_V4"
MAP_NAME = "06a_sentinelwalls.map"
SOURCE_MAP = "8c992ac2aa7ef60748115c7abdd80062ee8a579cd99bef21f50496556e0bdb68"
REFERENCE_MAP = "710e0944c2e4f081e6a7dd5af83db0356242cd275bdb45d29a38489edb249904"
PROFILE_V4 = dict(
    tag=r"scenarios\skies\solo\sentinelhq\bitmaps\sky_shq_gradient_jungle",
    encoding="rgba8", format=11, width=32, height=128, mip_count=8,
    pixel_bytes=21852,
    donor_width=64, donor_height=256, donor_mip_count=9, donor_pixel_bytes=87388,
    donor_registration=[32, 128], registration=[16, 64],
    donor_sha256="f1b7831f7f362b907b0219a5b441cd5e4ba342d1233cc4a5ac07aa4c6c503699",
    target_sha256="f47a4549233049c382e0679b2d574febf90cbc37388200a98e237eeff4ad3b64")
PROFILES_V4 = (PROFILE_V4,)
DONOR_PROFILE = dict(PROFILE_V4, width=64, height=256, mip_count=9, pixel_bytes=87388)
BASE_OFFSET = 65536


def read_v4(root):
    """Admit the exact profile and both complete mip ownership tables."""
    path = Path(root) / BUNDLE_V4
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 64 * 1024:
        raise FormatError("Excessive bitmap correction v4 manifest")
    try:
        manifest = json.loads(metadata.read_bytes())
        if (not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA_V4 or
                manifest.get("map") != MAP_NAME or
                manifest.get("source_map_sha256") != SOURCE_MAP or
                manifest.get("original_map_sha256") != REFERENCE_MAP or
                manifest.get("full_graphics_parity") is not False or
                manifest.get("profile") != PROFILE_V4 or
                manifest.get("donor_mips") != mip_owners(DONOR_PROFILE) or
                manifest.get("target_mips") != mip_owners(PROFILE_V4) or
                type(manifest.get("base_offset")) is not int or
                manifest["base_offset"] != BASE_OFFSET or
                len(manifest["donor_mip_sha256"]) != 9 or
                len(manifest["target_mip_sha256"]) != 8 or
                type(manifest["delta_bytes"]) is not int or
                not 0 < manifest["delta_bytes"] <= PROFILE_V4["pixel_bytes"] + 65536 or
                path.stat().st_size != manifest["delta_bytes"]):
            raise FormatError("Bitmap correction v4 identity or extent differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"]:
            raise FormatError("Bitmap correction v4 fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError) as error:
        raise FormatError("Invalid bitmap correction v4 manifest") from error
    return path, manifest, packed


def reconstruct_v4(root, donors):
    """Pure MCC-input reconstruction; original maps are never opened."""
    path, manifest, packed = read_v4(root)
    if set(donors) != {PROFILE_V4["tag"]}:
        raise FormatError("Bitmap correction v4 donor set differs")
    donor = donors[PROFILE_V4["tag"]]
    if (len(donor) != DONOR_PROFILE["pixel_bytes"] or
            digest(donor) != PROFILE_V4["donor_sha256"] or
            mip_hashes(donor, DONOR_PROFILE) != manifest["donor_mip_sha256"]):
        raise FormatError("Bitmap correction v4 donor pixels or mips differ")
    base = donor[BASE_OFFSET:]
    if len(base) != PROFILE_V4["pixel_bytes"]:
        raise FormatError("Bitmap correction v4 source slice extent differs")
    result = decode_delta(base, packed, source_sha256=manifest["base_sha256"],
                          target_sha256=PROFILE_V4["target_sha256"],
                          delta_sha256=manifest["delta_sha256"])
    if mip_hashes(result, PROFILE_V4) != manifest["target_mip_sha256"]:
        raise FormatError("Bitmap correction v4 target mips differ")
    return [(PROFILE_V4, result)], path, manifest, packed


def create_v4(donor, target, output):
    """Offline maintenance from two complete authenticated native chains."""
    output = Path(output)
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction v4 output already exists")
    if (len(donor) != DONOR_PROFILE["pixel_bytes"] or
            digest(donor) != PROFILE_V4["donor_sha256"] or
            len(target) != PROFILE_V4["pixel_bytes"] or
            digest(target) != PROFILE_V4["target_sha256"]):
        raise FormatError("Bitmap correction v4 creation images differ")
    base = donor[BASE_OFFSET:]
    packed = zlib.compress(bytes(a ^ b for a, b in zip(base, target, strict=True)), 9)
    decode_delta(base, packed, source_sha256=digest(base),
                 target_sha256=PROFILE_V4["target_sha256"], delta_sha256=digest(packed))
    manifest = dict(schema=SCHEMA_V4, map=MAP_NAME,
                    source_map_sha256=SOURCE_MAP, original_map_sha256=REFERENCE_MAP,
                    profile=PROFILE_V4, donor_mips=mip_owners(DONOR_PROFILE),
                    target_mips=mip_owners(PROFILE_V4), base_offset=BASE_OFFSET,
                    base_sha256=digest(base), donor_mip_sha256=mip_hashes(donor, DONOR_PROFILE),
                    target_mip_sha256=mip_hashes(target, PROFILE_V4),
                    delta_bytes=len(packed), delta_sha256=digest(packed),
                    full_graphics_parity=False,
                    scope="Authenticated Sacred Icon gradient only; original descriptor extent and exact pixels, without resampling or shader changes")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(packed)
    output.with_suffix("").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest
