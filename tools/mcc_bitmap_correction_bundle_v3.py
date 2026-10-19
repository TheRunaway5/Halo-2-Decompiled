"""Exact Metropolis pixel deltas; MCC is the only import-time asset source.

V1/V2 stay immutable. This separate bundle adds three descriptor-identical
original bitmap restorations without resampling or changing shader programs.
"""
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest, mip_hashes, mip_owners

BUNDLE_V3 = Path("tools/data/native-bitmap-corrections-v3.json.zlib")
SCHEMA_V3 = "H2_MCC_BITMAP_DELTA_V3"
MAP_NAME = "03b_newmombasa.map"
SOURCE_MAP = "9ddd648b3c80aeb0b8823f07d91d88f71c62b53fcbcd93376af0c200075068c2"
REFERENCE_MAP = "d04e3445bafffd621b279482c23d21d91b37db098af0e1b977a04ce0f65ecfa7"
PROFILES_V3 = (
    dict(tag=r"scenarios\bitmaps\human\urban\metals\matte_grey_edge_dif",
         encoding="bc3", format=16, width=256, height=256, mip_count=7,
         pixel_bytes=87376,
         donor_sha256="7082f9e1467b1fb63566ec7e2465a411c9e68af7ef7e6f6e403da94d9162d75d",
         target_sha256="4dbd000ab52bd4cfc71ffa13d6a90769b5b9a5ef94f939bb04a81a86003e24be"),
    dict(tag=r"scenarios\bitmaps\human\urban\metals\metal_slats_bump",
         encoding="rgba8", format=11, width=256, height=256, mip_count=9,
         pixel_bytes=349524,
         donor_sha256="58c37f307c7bb977d664021b6debd1cf9fe3a4fa50e58fda259ca7dd65bfcb4b",
         target_sha256="0484cd3c3777f946f8c236b7a02c88073e6b19fccd8b520e5ee60b8d546fc53a"),
    dict(tag=r"scenarios\bitmaps\human\urban\metals\metal_slats_dif",
         encoding="bc3", format=16, width=256, height=256, mip_count=7,
         pixel_bytes=87376,
         donor_sha256="1236a6b05ae67a984a4d853db95c4b772e07688c7be89577d909b5bb4a55032e",
         target_sha256="c76e0016c69f1053710d600ec9f5fb32e9537b819c62cd5360dd846705628bab"),
)


def read_v3(root):
    """Validate every profile, mip and nonoverlapping delta byte owner."""
    path = Path(root) / BUNDLE_V3
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 64 * 1024:
        raise FormatError("Excessive bitmap correction manifest")
    try:
        manifest = json.loads(metadata.read_bytes())
        if (not isinstance(manifest, dict) or manifest.get("schema") != SCHEMA_V3 or
                manifest.get("map") != MAP_NAME or
                manifest.get("source_map_sha256") != SOURCE_MAP or
                manifest.get("original_map_sha256") != REFERENCE_MAP or
                manifest.get("full_graphics_parity") is not False or
                len(manifest["profiles"]) != len(PROFILES_V3)):
            raise FormatError("Bitmap correction bundle v3 identity differs")
        offset = 0
        for entry, expected in zip(manifest["profiles"], PROFILES_V3, strict=True):
            if (entry["profile"] != expected or entry["mips"] != mip_owners(expected) or
                    type(entry["delta_offset"]) is not int or entry["delta_offset"] != offset or
                    type(entry["delta_bytes"]) is not int or
                    not 0 < entry["delta_bytes"] <= expected["pixel_bytes"] + 65536 or
                    any(len(entry[k]) != expected["mip_count"]
                        for k in ("donor_mip_sha256", "target_mip_sha256"))):
                raise FormatError("Bitmap correction v3 profile or delta owner differs")
            offset += entry["delta_bytes"]
        if manifest["delta_bytes"] != offset or path.stat().st_size != offset:
            raise FormatError("Bitmap correction bundle v3 extent differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"]:
            raise FormatError("Bitmap correction bundle v3 fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError) as error:
        raise FormatError("Invalid bitmap correction v3 manifest") from error
    return path, manifest, packed


def reconstruct_v3(root, donors):
    """Stage all three exact images without writing any cache bytes."""
    path, manifest, packed = read_v3(root)
    if set(donors) != {p["tag"] for p in PROFILES_V3}:
        raise FormatError("Bitmap correction v3 donor set differs")
    staged = []
    for entry in manifest["profiles"]:
        profile = entry["profile"]
        donor = donors[profile["tag"]]
        if (len(donor) != profile["pixel_bytes"] or
                mip_hashes(donor, profile) != entry["donor_mip_sha256"]):
            raise FormatError("Bitmap correction v3 donor mip differs")
        offset, size = entry["delta_offset"], entry["delta_bytes"]
        result = decode_delta(donor, packed[offset:offset + size],
                              source_sha256=profile["donor_sha256"],
                              target_sha256=profile["target_sha256"],
                              delta_sha256=entry["delta_sha256"])
        if mip_hashes(result, profile) != entry["target_mip_sha256"]:
            raise FormatError("Bitmap correction v3 target mip differs")
        staged.append((profile, result))
    return staged, path, manifest, packed


def create_v3(donors, targets, output):
    """Offline maintenance from the three fully fingerprinted native images."""
    output = Path(output)
    names = {p["tag"] for p in PROFILES_V3}
    if set(donors) != names or set(targets) != names:
        raise FormatError("Bitmap correction v3 creation input set differs")
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction bundle v3 output already exists")
    payload, entries = bytearray(), []
    for profile in PROFILES_V3:
        donor, target = donors[profile["tag"]], targets[profile["tag"]]
        if len(donor) != profile["pixel_bytes"] or len(target) != profile["pixel_bytes"]:
            raise FormatError("Bitmap correction v3 creation extent differs")
        packed = zlib.compress(bytes(a ^ b for a, b in zip(donor, target, strict=True)), 9)
        decode_delta(donor, packed, source_sha256=profile["donor_sha256"],
                     target_sha256=profile["target_sha256"], delta_sha256=digest(packed))
        entries.append(dict(profile=profile, mips=mip_owners(profile),
                            donor_mip_sha256=mip_hashes(donor, profile),
                            target_mip_sha256=mip_hashes(target, profile),
                            delta_offset=len(payload), delta_bytes=len(packed),
                            delta_sha256=digest(packed)))
        payload.extend(packed)
    manifest = dict(schema=SCHEMA_V3, map=MAP_NAME, source_map_sha256=SOURCE_MAP,
                    original_map_sha256=REFERENCE_MAP, profiles=entries,
                    delta_bytes=len(payload), delta_sha256=digest(payload),
                    full_graphics_parity=False,
                    scope="Three exact original Metropolis bitmap mip chains; composed with immutable V2 Zanzibar corrections")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(payload)
    output.with_suffix("").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest
