"""Exact original Earth BC3 chain from authenticated MCC bytes only.

The binary prefix XOR is a storage delta, never an image conversion. All five
local owners were inventoried and their independent original native chains
corroborated. Original files are offline bundle-maintenance inputs only.
"""
import argparse
import json
from pathlib import Path
import zlib

from audit_rendering import FormatError
from mcc_bitmap_correction_bundle import decode_delta, digest, mip_hashes, mip_owners

BUNDLE_V6 = Path("tools/data/native-bitmap-corrections-v6.json.zlib")
SCHEMA_V6 = "H2_MCC_BITMAP_DELTA_V6"
PROFILE_V6 = dict(
    tag=r"scenarios\skies\solo\spacestation\bitmaps\earth_matte",
    encoding="bc3", format=16, width=512, height=512, mip_count=8,
    pixel_bytes=349520, registration=[256, 256],
    donor_encoding="rgba8", donor_format=11, donor_width=1024, donor_height=1024,
    donor_mip_count=11, donor_pixel_bytes=5592404, donor_registration=[512, 512],
    donor_sha256="481c96bdcbff17ef7abbc9915f26afac8e7e2a1fc40a164bd2a82768d380f05c",
    target_sha256="b80a32d6384cbfa338297df337f8e66ebaf63c33c02e2f8283d0ff37aa5e80c8")
PROFILES_V6 = (PROFILE_V6,)
DONOR_PROFILE = dict(PROFILE_V6, encoding="rgba8", format=11, width=1024, height=1024,
                     mip_count=11, pixel_bytes=5592404)
BASE_SHA256 = "174983aabfba15e25d3417f71d05537ef582e0f198d2c538e33b1a5eaa70fd7f"
# Authenticated actual resident owners; shared maps and aliases are excluded.
OWNERS_V6 = (
    dict(map="00a_introduction.map", source_map_sha256="f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8",
         original_map_sha256="5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28", datum=3786866753, address=456668),
    dict(map="01a_tutorial.map", source_map_sha256="f71460e42eb43ea1d74a67af2589e8bccea645f62d921a430f94e946ac4bf08e",
         original_map_sha256="623598449e34a51da6fb131ad66aaf2bb0de57113abe616d4c4009e877398faf", datum=3783720977, address=375448),
    dict(map="01b_spacestation.map", source_map_sha256="06fe3555d4301c18a5b0f7379f5ddd6714626a3134462a14ce80a609bc7bf2bb",
         original_map_sha256="5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0", datum=3783720977, address=1317360),
    dict(map="07a_highcharity.map", source_map_sha256="f883bc4592839329f5e12618413e202298f139d01a6758764d28ded95e4b0569",
         original_map_sha256="770426d5182d47d31ce870fcbc9ed346988175690a3dc1c58ca8a7c1ef1dbd45", datum=3786866753, address=1584460),
    dict(map="elongation.map", source_map_sha256="3780ab18cb5c65a0f23d99ab827671bc85c53c22c5e9a3826e5ab01425cff729",
         original_map_sha256="8aa190e424003ce7efaea9e57be9556194b3aefdeccd8a86919f2534f7cf15ea", datum=3783655433, address=52896872),
)


def validate_owner_v6(name, cache, tag, at):
    owner = next((row for row in OWNERS_V6 if row["map"] == name), None)
    if (owner is None or tag["datum"] != owner["datum"] or tag["address"] != owner["address"] or
            tag["size"] != 248 or cache.u16(at + 14) != 513 or
            cache.u32(at + 56) != 119835648 or cache.u32(at + 80) != 2210131):
        raise FormatError("Earth correction actual local owner differs")


def read_v6(root):
    path = Path(root) / BUNDLE_V6
    metadata = path.with_suffix("")
    if metadata.stat().st_size > 64 * 1024:
        raise FormatError("Excessive bitmap correction v6 manifest")
    try:
        manifest = json.loads(metadata.read_bytes())
        if (manifest.get("schema") != SCHEMA_V6 or manifest.get("owners") != list(OWNERS_V6) or
                manifest.get("full_graphics_parity") is not False or len(manifest["profiles"]) != 1):
            raise FormatError("Bitmap correction v6 identity differs")
        entry = manifest["profiles"][0]
        if (entry["profile"] != PROFILE_V6 or entry["mips"] != mip_owners(PROFILE_V6) or
                entry["donor_mips"] != mip_owners(DONOR_PROFILE) or
                entry["base_offset"] != 0 or entry["base_bytes"] != PROFILE_V6["pixel_bytes"] or
                entry["base_sha256"] != BASE_SHA256 or entry["delta_offset"] != 0 or
                type(entry["delta_bytes"]) is not int or
                not 0 < entry["delta_bytes"] <= PROFILE_V6["pixel_bytes"] + 65536 or
                manifest["delta_bytes"] != entry["delta_bytes"] or
                len(entry["donor_mip_sha256"]) != 11 or len(entry["target_mip_sha256"]) != 8 or
                path.stat().st_size != manifest["delta_bytes"]):
            raise FormatError("Bitmap correction v6 mip or delta ownership differs")
        packed = path.read_bytes()
        if digest(packed) != manifest["delta_sha256"] or digest(packed) != entry["delta_sha256"]:
            raise FormatError("Bitmap correction v6 fingerprint differs")
    except FormatError:
        raise
    except (KeyError, TypeError, ValueError, AttributeError) as error:
        raise FormatError("Invalid bitmap correction v6 manifest") from error
    return path, manifest, packed


def reconstruct_v6(root, donors):
    path, manifest, packed = read_v6(root)
    if set(donors) != {PROFILE_V6["tag"]}:
        raise FormatError("Bitmap correction v6 donor set differs")
    donor = donors[PROFILE_V6["tag"]]
    entry = manifest["profiles"][0]
    if (len(donor) != DONOR_PROFILE["pixel_bytes"] or digest(donor) != PROFILE_V6["donor_sha256"] or
            mip_hashes(donor, DONOR_PROFILE) != entry["donor_mip_sha256"]):
        raise FormatError("Bitmap correction v6 complete donor mips differ")
    target = decode_delta(donor[:PROFILE_V6["pixel_bytes"]], packed, source_sha256=BASE_SHA256,
                          target_sha256=PROFILE_V6["target_sha256"], delta_sha256=manifest["delta_sha256"])
    if mip_hashes(target, PROFILE_V6) != entry["target_mip_sha256"]:
        raise FormatError("Bitmap correction v6 target mips differ")
    return [(PROFILE_V6, target)], path, manifest, packed


def create_v6(donors, targets, output):
    output = Path(output)
    if output.exists() or output.with_suffix("").exists():
        raise FormatError("Bitmap correction v6 output already exists")
    if set(donors) != {PROFILE_V6["tag"]} or set(targets) != {PROFILE_V6["tag"]}:
        raise FormatError("Bitmap correction v6 creation set differs")
    donor, target = donors[PROFILE_V6["tag"]], targets[PROFILE_V6["tag"]]
    base = donor[:PROFILE_V6["pixel_bytes"]]
    if (len(donor) != DONOR_PROFILE["pixel_bytes"] or len(target) != PROFILE_V6["pixel_bytes"] or
            digest(donor) != PROFILE_V6["donor_sha256"] or digest(base) != BASE_SHA256 or
            digest(target) != PROFILE_V6["target_sha256"]):
        raise FormatError("Bitmap correction v6 creation images differ")
    packed = zlib.compress(bytes(a ^ b for a, b in zip(base, target, strict=True)), 9)
    decode_delta(base, packed, source_sha256=BASE_SHA256, target_sha256=PROFILE_V6["target_sha256"],
                 delta_sha256=digest(packed))
    entry = dict(profile=PROFILE_V6, mips=mip_owners(PROFILE_V6), donor_mips=mip_owners(DONOR_PROFILE),
        donor_mip_sha256=mip_hashes(donor, DONOR_PROFILE), target_mip_sha256=mip_hashes(target, PROFILE_V6),
        base_offset=0, base_bytes=len(base), base_sha256=BASE_SHA256,
        delta_offset=0, delta_bytes=len(packed), delta_sha256=digest(packed))
    manifest = dict(schema=SCHEMA_V6, owners=list(OWNERS_V6), profiles=[entry],
        delta_bytes=len(packed), delta_sha256=digest(packed), full_graphics_parity=False,
        scope="Exact delivered original512 BC3 eight-mip chain for five authenticated local Earth owners; all source/native owners corroborated, no 2x2/1x1 tails, image resampling, recompression or geometry changes")
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
    create_v6({PROFILE_V6["tag"]: args.donor.read_bytes()},
              {PROFILE_V6["tag"]: args.target.read_bytes()}, args.output)
