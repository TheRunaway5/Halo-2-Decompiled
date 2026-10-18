"""Fingerprint-bound offline graphics corrections for the owned MCC cache."""
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, texture_resource

TAG = r"scenarios\bitmaps\multi\zanzibar\massive_concrete_bump"
SOURCE_MAP = "563bef8cc5f8f8cf28112eff44f4e1377e1f012b69f65e3e2e9a04484c497ca7"


def export_profile(reference, reformation, comparison, output):
    from restore_mcc_zanzibar_bump import pixels, REFERENCE_MAP
    output = Path(output)
    if output.exists():
        raise FormatError("Choose a new graphics correction profile directory")
    original, frame, rgba = pixels(reference)
    _, reform_frame, reform_rgba = pixels(reformation)
    report = json.loads(Path(comparison).read_text())
    if file_sha(Path(original["cache"])) != REFERENCE_MAP or report["source_maps"]["xbox"]["sha256"] != REFERENCE_MAP:
        raise FormatError("Original graphics reference fingerprint differs")
    for mip, measured in zip(frame["mips"], report["mips"], strict=True):
        if hashlib.sha256(rgba[mip["offset"]:mip["offset"]+mip["size"]]).hexdigest() != measured["xbox_sha256"]:
            raise FormatError("Original graphics mip fingerprint differs")
    if rgba[:frame["mips"][0]["size"]] != reform_rgba[:reform_frame["mips"][0]["size"]]:
        raise FormatError("Reformation base image does not corroborate this reference")
    output.mkdir(parents=True)
    (output/"zanzibar-concrete.rgba").write_bytes(rgba)
    profile = dict(schema="H2_MCC_GRAPHICS_CORRECTIONS_V1", source_map_sha256=SOURCE_MAP,
                   original_map_sha256=REFERENCE_MAP, tag=TAG, rgba_sha256=hashlib.sha256(rgba).hexdigest(),
                   rgba_bytes=len(rgba), mips=frame["mips"], reference_mips=report["mips"],
                   reformation_base_corroborated=True, full_graphics_parity=False)
    (output/"profile.json").write_text(json.dumps(profile,indent=2)+"\n")
    return profile


def apply_selected(root, pack):
    from mcc_bitmap_correction_bundle import (BUNDLE_V2, PROFILES_V2, digest,
                                               reconstruct_v2, validate_profile)
    root, pack = Path(root), Path(pack)
    registry = root/"assets/graphics-restoration-source.tsv"
    source = None
    if registry.is_file():
        lines = registry.read_text().splitlines()
        if len(lines)!=2 or lines[0]!="H2_GRAPHICS_CORRECTION_SOURCE_V1" or not lines[1].startswith("profile\t"):
            raise FormatError("Invalid graphics correction selection")
        source = Path(lines[1].split("\t",1)[1]).resolve()
    if not (root/BUNDLE_V2).is_file():
        return dict(complete=False, blocker="Verified graphics correction bundle v2 is missing; update this port installation")
    maps = pack/"halo2/h2_maps_win64_dx11"
    map_path, bank_path = maps/"zanzibar.map", maps/"textures.dat"
    if file_sha(map_path)!=SOURCE_MAP:
        raise FormatError("MCC Zanzibar source differs from the verified graphics correction revision")
    before = {p.name:file_sha(p) for p in (map_path,bank_path)}
    destination=pack/"graphics-corrections"
    if destination.exists():
        raise FormatError("Graphics correction output already exists")
    cache = MccCache(map_path)
    try:
        # Resolve exact local salted identities and complete descriptors for all
        # entries before reconstructing or changing either cache file.
        owners, donors = {}, {}
        with bank_path.open("rb") as stream:
            for profile in PROFILES_V2:
                matches = [t for t in cache.tags.values() if t["group"]=="bitm" and
                           t["name"]==profile["tag"] and t["address"]]
                if len(matches)!=1:
                    raise FormatError("Ambiguous graphics correction bitmap identity")
                rows=cache.block(matches[0]["offset"]+68,168)
                if len(rows)!=1:
                    raise FormatError("Graphics correction frame count differs")
                at=rows[0]
                descriptor=(cache.u16(at+4),cache.u16(at+6),cache.data[at+8],
                            cache.u16(at+10),cache.u16(at+12),cache.u16(at+20))
                if descriptor!=(profile["width"],profile["height"],1,0,profile["format"],profile["mip_count"]-1) or \
                   cache.u32(at+84) or cache.u32(at+88) or at in owners.values():
                    raise FormatError("Graphics correction bitmap descriptor differs")
                donor=bytearray(texture_resource(stream,cache.u32(at+56),cache.u32(at+80)))
                if len(donor)!=profile["pixel_bytes"]:
                    raise FormatError("Graphics correction donor pixel extent differs")
                if profile["encoding"]=="rgba8":
                    donor[0::4],donor[2::4]=donor[2::4],donor[0::4]
                owners[profile["tag"]]=at
                donors[profile["tag"]]=donor
        staged,bundle_path,bundled,bundle_bytes=reconstruct_v2(root,donors)
        manifest_bytes=(json.dumps(bundled,indent=2)+"\n").encode()
        if source is not None:
            # The explicit development selection can corroborate concrete, but
            # never substitutes an unverified texture or removes the grey fix.
            selected=json.loads((source/"profile.json").read_text())
            validate_profile(selected)
            rgba=(source/"zanzibar-concrete.rgba").read_bytes()
            if len(rgba)!=PROFILES_V2[0]["pixel_bytes"] or digest(rgba)!=PROFILES_V2[0]["target_sha256"]:
                raise FormatError("Selected graphics correction profile differs")
        partitions, pointer = [], bank_path.stat().st_size
        for profile, pixels in staged:
            native=bytearray(pixels)
            if profile["encoding"]=="rgba8":
                native[0::4],native[2::4]=pixels[2::4],pixels[0::4]
            partition=struct.pack("<Ii",1,-len(native))+native
            partitions.append((profile,owners[profile["tag"]],pointer,partition,native))
            pointer+=len(partition)
        if pointer>=0x40000000:
            raise FormatError("Corrected MCC texture resource exceeds address capacity")
        # Every identity, input mip, delta, target mip and output address is
        # validated above. The caller publishes this private pack transactionally.
        with bank_path.open("ab") as stream:
            for _,at,pointer,partition,_ in partitions:
                stream.write(partition)
                struct.pack_into("<I",cache.data,at+56,pointer)
                struct.pack_into("<I",cache.data,at+80,len(partition))
        from prepare_mcc_gameplay import save_changed_cache
        save_changed_cache(cache)
        with bank_path.open("rb") as stream:
            for _,_,pointer,partition,native in partitions:
                if texture_resource(stream,pointer,len(partition))!=native:
                    raise FormatError("Corrected MCC texture readback differs")
    finally:
        cache.close()
    destination.mkdir()
    (destination/"profile.json").write_bytes(manifest_bytes)
    for path,data in ((bundle_path,bundle_bytes),(bundle_path.with_suffix(""),manifest_bytes)):
        (destination/path.name).write_bytes(data)
        if digest(data)!=file_sha(destination/path.name):
            raise FormatError("Preserved bitmap correction delta readback differs")
    report=dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V2",complete=True,full_graphics_parity=False,
                profile_sha256=file_sha(destination/"profile.json"),tag=TAG,
                restored_mips=sum(p["mip_count"] for p in PROFILES_V2),
                source="bundled MCC pixel delta" if source is None else "selected correction profile and bundled MCC pixel delta",
                profiles=[dict(tag=p["tag"],restored_mips=p["mip_count"],format=p["format"],
                               target_sha256=p["target_sha256"]) for p in PROFILES_V2],
                changes=[dict(path="halo2/h2_maps_win64_dx11/"+p.name,donor_sha256=before[p.name],converted_sha256=file_sha(p))
                         for p in (map_path,bank_path)],
                scope="Authenticated Zanzibar concrete and matte_grey only; other map references and existing shader programs remain intact")
    (pack/"graphics-conversion.json").write_text(json.dumps(report,indent=2)+"\n")
    return report
