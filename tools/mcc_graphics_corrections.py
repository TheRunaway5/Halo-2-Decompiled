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


def apply_selected(root, pack, *, tag_revisions=None):
    from mcc_bitmap_correction_bundle import (BUNDLE_V2, PROFILES_V2, digest,
                                               reconstruct_v2)
    from mcc_bitmap_correction_bundle_v3 import (BUNDLE_V3, MAP_NAME, PROFILES_V3,
                                                SOURCE_MAP as METROPOLIS_SOURCE_MAP,
                                                reconstruct_v3)
    from mcc_bitmap_correction_bundle_v4 import (BUNDLE_V4, MAP_NAME as SACRED_ICON_MAP,
                                                PROFILES_V4, SOURCE_MAP as SACRED_ICON_SOURCE_MAP,
                                                reconstruct_v4)
    from installation_paths import resource_root
    root, pack = resource_root(root), Path(pack)
    # Original/reference profiles are offline maintenance inputs only. Normal
    # import uses the port's qualified deltas and the user's MCC bytes, even
    # when an old development reference registry is present.
    if not (root/BUNDLE_V2).is_file():
        return dict(complete=False, blocker="Verified graphics correction bundle v2 is missing; update this port installation")
    if not (root/BUNDLE_V3).is_file():
        return dict(complete=False, blocker="Verified graphics correction bundle v3 is missing; update this port installation")
    maps = pack/"halo2/h2_maps_win64_dx11"
    sources = [("zanzibar.map", SOURCE_MAP, PROFILES_V2, reconstruct_v2),
               (MAP_NAME, METROPOLIS_SOURCE_MAP, PROFILES_V3, reconstruct_v3)]
    # Older portable tool sets remain valid V3 installations. A complete V4
    # bundle upgrades fresh imports and the onboarding freshness requirement.
    version = 3
    if (root/BUNDLE_V4).is_file():
        sources.append((SACRED_ICON_MAP, SACRED_ICON_SOURCE_MAP, PROFILES_V4, reconstruct_v4))
        version = 4
    map_paths = [maps/name for name,_,_,_ in sources]
    bank_path = maps/"textures.dat"
    for name, expected, _, _ in sources:
        actual = file_sha(maps/name)
        # The importer already projects gameplay tags before this pass. Accept
        # that exact recorded transition from the pinned original MCC source;
        # a stale output or unrelated original still fails before any writes.
        # Bitmap descriptors and every donor/target mip remain checked below.
        revision = tag_revisions.get(name) if isinstance(tag_revisions, dict) else None
        if actual != expected and revision != dict(source_sha256=expected, output_sha256=actual):
            raise FormatError("MCC "+name+" source differs from the verified graphics correction revision")
    before = {p.name:file_sha(p) for p in (*map_paths,bank_path)}
    destination=pack/"graphics-corrections"
    if destination.exists():
        raise FormatError("Graphics correction output already exists")
    caches, staged, bundles = {}, [], []
    try:
        # Validate every selected map, local identity, complete mip chain and
        # delta snapshot before changing any map or resource bank.
        owners, donors = {}, {}
        with bank_path.open("rb") as stream:
            for name,_,profiles,reconstruct in sources:
                cache = caches[name] = MccCache(maps/name)
                donors[name] = {}
                for profile in profiles:
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
                    if descriptor!=(profile.get("donor_width",profile["width"]),
                                    profile.get("donor_height",profile["height"]),1,0,
                                    profile["format"],profile.get("donor_mip_count",profile["mip_count"])-1) or \
                       cache.u32(at+84) or cache.u32(at+88) or (name,at) in owners.values():
                        raise FormatError("Graphics correction bitmap descriptor differs")
                    if ("donor_registration" in profile and
                            list(struct.unpack_from("<hh",cache.data,at+16))!=profile["donor_registration"]):
                        raise FormatError("Graphics correction bitmap registration differs")
                    donor=bytearray(texture_resource(stream,cache.u32(at+56),cache.u32(at+80)))
                    if len(donor)!=profile.get("donor_pixel_bytes",profile["pixel_bytes"]):
                        raise FormatError("Graphics correction donor pixel extent differs")
                    if profile["encoding"]=="rgba8":
                        donor[0::4],donor[2::4]=donor[2::4],donor[0::4]
                    owners[(name,profile["tag"])]=(name,at)
                    donors[name][profile["tag"]]=donor
                images,bundle_path,bundled,bundle_bytes=reconstruct(root,donors[name])
                staged.extend((name,profile,pixels) for profile,pixels in images)
                bundles.append((bundle_path,bundled,bundle_bytes))
        manifest_bytes=(json.dumps(dict(schema=f"H2_MCC_GRAPHICS_CORRECTIONS_V{version}",
                                       bundles=[manifest for _,manifest,_ in bundles],
                                       full_graphics_parity=False),indent=2)+"\n").encode()
        partitions, pointer = [], bank_path.stat().st_size
        for name, profile, pixels in staged:
            native=bytearray(pixels)
            if profile["encoding"]=="rgba8":
                native[0::4],native[2::4]=pixels[2::4],pixels[0::4]
            partition=struct.pack("<Ii",1,-len(native))+native
            partitions.append((name,profile,owners[(name,profile["tag"])][1],pointer,partition,native))
            pointer+=len(partition)
        if pointer>=0x40000000:
            raise FormatError("Corrected MCC texture resource exceeds address capacity")
        # Every identity, input mip, delta, target mip and output address is
        # validated above. The caller publishes this private pack transactionally.
        with bank_path.open("ab") as stream:
            for name,profile,at,pointer,partition,native in partitions:
                stream.write(partition)
                struct.pack_into("<I",caches[name].data,at+56,pointer)
                struct.pack_into("<I",caches[name].data,at+80,len(partition))
                if "donor_width" in profile:
                    # Preserve MCC format, sampling and linear-packing flags;
                    # the original swizzled descriptor is never copied here.
                    struct.pack_into("<H",caches[name].data,at+4,profile["width"])
                    struct.pack_into("<H",caches[name].data,at+6,profile["height"])
                    struct.pack_into("<H",caches[name].data,at+20,profile["mip_count"]-1)
                    struct.pack_into("<hh",caches[name].data,at+16,*profile["registration"])
        from prepare_mcc_gameplay import save_changed_cache
        for cache in caches.values():
            save_changed_cache(cache)
        with bank_path.open("rb") as stream:
            for _,_,_,pointer,partition,native in partitions:
                if texture_resource(stream,pointer,len(partition))!=native:
                    raise FormatError("Corrected MCC texture readback differs")
    finally:
        for cache in caches.values():
            cache.close()
    destination.mkdir()
    (destination/"profile.json").write_bytes(manifest_bytes)
    for bundle_path,bundled,bundle_bytes in bundles:
        for path,data in ((bundle_path,bundle_bytes),
                          (bundle_path.with_suffix(""),(json.dumps(bundled,indent=2)+"\n").encode())):
            (destination/path.name).write_bytes(data)
            if digest(data)!=file_sha(destination/path.name):
                raise FormatError("Preserved bitmap correction delta readback differs")
    report=dict(schema=f"H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V{version}",complete=True,full_graphics_parity=False,
                profile_sha256=file_sha(destination/"profile.json"),tag=TAG,
                restored_mips=sum(p["mip_count"] for _,p,_ in staged),
                source="bundled MCC pixel delta",
                profiles=[dict(map=name,tag=p["tag"],restored_mips=p["mip_count"],format=p["format"],
                               target_sha256=p["target_sha256"]) for name,p,_ in staged],
                changes=[dict(path="halo2/h2_maps_win64_dx11/"+p.name,donor_sha256=before[p.name],converted_sha256=file_sha(p))
                         for p in (*map_paths,bank_path)],
                scope="Authenticated Zanzibar concrete/matte_grey, three Metropolis bitmaps and (when bundled) Sacred Icon gradient; other map references and existing shader programs remain intact")
    (pack/"graphics-conversion.json").write_text(json.dumps(report,indent=2)+"\n")
    return report
