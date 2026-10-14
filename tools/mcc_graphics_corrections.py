"""Fingerprint-bound offline graphics corrections for the owned MCC cache."""
import hashlib
import json
from pathlib import Path
import shutil
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
    root, pack = Path(root), Path(pack)
    registry = root/"assets/graphics-restoration-source.tsv"
    if not registry.is_file():
        return dict(complete=False, blocker="No verified graphics correction profile is selected")
    lines = registry.read_text().splitlines()
    if len(lines)!=2 or lines[0]!="H2_GRAPHICS_CORRECTION_SOURCE_V1" or not lines[1].startswith("profile\t"):
        raise FormatError("Invalid graphics correction selection")
    source = Path(lines[1].split("\t",1)[1]).resolve()
    profile = json.loads((source/"profile.json").read_text())
    rgba = (source/"zanzibar-concrete.rgba").read_bytes()
    if profile.get("schema")!="H2_MCC_GRAPHICS_CORRECTIONS_V1" or profile.get("tag")!=TAG or \
       profile.get("source_map_sha256")!=SOURCE_MAP or len(rgba)!=1398100 or \
       hashlib.sha256(rgba).hexdigest()!=profile.get("rgba_sha256"):
        raise FormatError("Selected graphics correction profile differs")
    maps = pack/"halo2/h2_maps_win64_dx11"
    map_path, bank_path = maps/"zanzibar.map", maps/"textures.dat"
    if file_sha(map_path)!=SOURCE_MAP:
        raise FormatError("MCC Zanzibar source differs from the verified graphics correction revision")
    before = {p.name:file_sha(p) for p in (map_path,bank_path)}
    cache = MccCache(map_path)
    matches = [t for t in cache.tags.values() if t["group"]=="bitm" and t["name"]==TAG and t["address"]]
    if len(matches)!=1:
        raise FormatError("Ambiguous graphics correction bitmap identity")
    rows = cache.block(matches[0]["offset"]+68,168)
    if len(rows)!=1:
        raise FormatError("Graphics correction frame count differs")
    at=rows[0]
    if (cache.u16(at+4),cache.u16(at+6),cache.data[at+8],cache.u16(at+10),cache.u16(at+12),cache.u16(at+20))!=(512,512,1,0,11,9) or cache.u32(at+84) or cache.u32(at+88):
        raise FormatError("Graphics correction bitmap descriptor differs")
    bgra=bytearray(rgba);bgra[0::4],bgra[2::4]=rgba[2::4],rgba[0::4]
    partition=struct.pack("<Ii",1,-len(bgra))+bgra
    with bank_path.open("ab") as stream:
        pointer=stream.tell()
        if pointer+len(partition)>=0x40000000:
            raise FormatError("Corrected MCC texture resource exceeds address capacity")
        stream.write(partition)
    struct.pack_into("<I",cache.data,at+56,pointer);struct.pack_into("<I",cache.data,at+80,len(partition))
    from prepare_mcc_gameplay import save_changed_cache
    save_changed_cache(cache);cache.close()
    with bank_path.open("rb") as stream:
        if texture_resource(stream,pointer,len(partition))!=bgra:
            raise FormatError("Corrected MCC texture readback differs")
    destination=pack/"graphics-corrections"
    destination.mkdir()
    for filename in ("profile.json","zanzibar-concrete.rgba"):
        shutil.copyfile(source/filename,destination/filename)
        if file_sha(source/filename)!=file_sha(destination/filename):
            raise FormatError("Preserved graphics correction profile readback differs")
    report=dict(schema="H2_MCC_APPLIED_GRAPHICS_CORRECTIONS_V1",complete=True,full_graphics_parity=False,
                profile_sha256=file_sha(source/"profile.json"),tag=TAG,restored_mips=10,
                changes=[dict(path="halo2/h2_maps_win64_dx11/"+p.name,donor_sha256=before[p.name],converted_sha256=file_sha(p))
                         for p in (map_path,bank_path)],
                scope="Zanzibar original concrete only; other map references and existing shader programs remain intact")
    (pack/"graphics-conversion.json").write_text(json.dumps(report,indent=2)+"\n")
    return report
