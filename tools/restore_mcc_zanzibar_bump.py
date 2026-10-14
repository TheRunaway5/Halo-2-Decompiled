#!/usr/bin/env python3
"""Apply the verified Zanzibar bump restoration to a new private MCC cache.

Reformation confirms the original base image; its regenerated smaller mips
differ from Xbox. Use the independently decoded original complete mip chain.
No installed game, selected runtime, shader source or donor copy is modified.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct

from audit_rendering import FormatError, file_sha
from mcc_content import MccCache, texture_resource
from prepare_mcc_gameplay import save_changed_cache, write_json

TAG = r"scenarios\bitmaps\multi\zanzibar\massive_concrete_bump"
SOURCE_MAP = "563bef8cc5f8f8cf28112eff44f4e1377e1f012b69f65e3e2e9a04484c497ca7"
REFERENCE_MAP = "2000bfc26fa38929e3ae8792618465ca5a60cc6bbeb753426e9d3a99954bdafb"


def pixels(directory):
    directory = Path(directory)
    manifest = json.loads((directory / "bitmap.json").read_text())
    if manifest["tag"] != TAG or len(manifest["frames"]) != 1:
        raise FormatError("Zanzibar correction reference identity differs")
    frame = manifest["frames"][0]
    if (frame["width"], frame["height"], frame["format"], frame["layers"], frame["depth"]) != (512,512,11,1,0):
        raise FormatError("Zanzibar correction reference dimensions/format differ")
    filename = frame["pixels"]
    if Path(filename).name != filename:
        raise FormatError("Invalid reference pixels path")
    data = (directory / filename).read_bytes()
    if len(data) != 1398100 or len(frame["mips"]) != 10:
        raise FormatError("Incomplete original Zanzibar mip chain")
    return manifest, frame, data


def restore(source, output, reference, reformation, comparison):
    source, output = Path(source).resolve(), Path(output).resolve()
    if output.exists() or source == output or source in output.parents or output in source.parents:
        raise FormatError("Choose a new non-overlapping private cache directory")
    provenance = json.loads((source / "provenance.json").read_text())
    if provenance.get("schema") != "H2_MCC_TU15_PREPARATION_V1" or provenance.get("runtime_active") is not False:
        raise FormatError("Restoration requires an inactive privately imported MCC cache")
    report = json.loads(Path(comparison).read_text())
    ref, frame, rgba = pixels(reference)
    _, reform_frame, reform_rgba = pixels(reformation)
    if file_sha(Path(ref["cache"])) != REFERENCE_MAP or report["source_maps"]["xbox"]["sha256"] != REFERENCE_MAP:
        raise FormatError("Original Xbox reference map fingerprint differs")
    for mip, measured in zip(frame["mips"], report["mips"], strict=True):
        raw = rgba[mip["offset"]:mip["offset"]+mip["size"]]
        if hashlib.sha256(raw).hexdigest() != measured["xbox_sha256"]:
            raise FormatError("Original decoded mip fingerprint differs")
    base = frame["mips"][0]; reform_base = reform_frame["mips"][0]
    if rgba[:base["size"]] != reform_rgba[:reform_base["size"]]:
        raise FormatError("Reformation's base image does not corroborate the Xbox reference")
    maps_relative = Path("halo2/h2_maps_win64_dx11")
    if file_sha(source / maps_relative / "zanzibar.map") != SOURCE_MAP:
        raise FormatError("MCC source revision differs from the measured Zanzibar correction")
    # Copy ownership transactionally. Untouched private imported files may
    # share immutable inodes; detach every modified file before writing it.
    try:
        shutil.copytree(source, output, copy_function=os.link, symlinks=False)
        for relative in [maps_relative / "textures.dat", maps_relative / "zanzibar.map", Path("provenance.json")]:
            destination = output / relative
            destination.unlink(); shutil.copy2(source / relative, destination)
        maps = output / maps_relative
        cache = MccCache(maps / "zanzibar.map")
        tags = [t for t in cache.tags.values() if t["group"] == "bitm" and t["name"] == TAG and t["address"]]
        if len(tags) != 1:
            raise FormatError("MCC Zanzibar bitmap owner is ambiguous")
        rows = cache.block(tags[0]["offset"] + 68, 168)
        if len(rows) != 1:
            raise FormatError("MCC Zanzibar frame count changed")
        at = rows[0]
        if (cache.u16(at+4), cache.u16(at+6), cache.data[at+8], cache.u16(at+10), cache.u16(at+12), cache.u16(at+20)) != (512,512,1,0,11,9):
            raise FormatError("MCC Zanzibar descriptor differs from the measured correction")
        if cache.u32(at+84) or cache.u32(at+88):
            raise FormatError("Unexpected lower-detail Zanzibar resource partitions")
        # Preserve each original mip verbatim, transposing RGBA decoder output
        # back to this MCC A8R8G8B8 resource's serialized BGRA channel order.
        bgra = bytearray(rgba)
        bgra[0::4], bgra[2::4] = rgba[2::4], rgba[0::4]
        partition = struct.pack("<Ii", 1, -len(bgra)) + bgra
        with (maps / "textures.dat").open("ab") as stream:
            pointer = stream.tell()
            if pointer + len(partition) >= 0x40000000:
                raise FormatError("Restored texture partition exceeds MCC resource addressing")
            stream.write(partition)
        struct.pack_into("<I", cache.data, at+56, pointer)
        struct.pack_into("<I", cache.data, at+80, len(partition))
        save_changed_cache(cache)
        cache.close()
        with (maps / "textures.dat").open("rb") as stream:
            if texture_resource(stream, pointer, len(partition)) != bgra:
                raise FormatError("Restored MCC texture readback differs")
        changes = [dict(path=str(relative), donor_sha256=file_sha(source/relative), converted_sha256=file_sha(output/relative))
                   for relative in [maps_relative/"textures.dat", maps_relative/"zanzibar.map"]]
        correction = dict(schema="H2_MCC_ZANZIBAR_BUMP_RESTORATION_V1", tag=TAG,
                          runtime_active=False, full_tu15_parity=False, changes=changes,
                          original_reference_sha256=REFERENCE_MAP,
                          reformation_base_corroborated=True, restored_mips=report["mips"],
                          shader_policy="Keep the preserved existing native shader definitions and Vulkan programs",
                          scope="Zanzibar only. Terminal and every other map retain their own original MCC resource references.")
        write_json(output/"zanzibar-bump-restoration.json", correction)
        provenance["derived_from"] = str(source)
        provenance["output_changes"] = changes
        provenance["graphics_restoration"] = "zanzibar-bump-restoration.json"
        write_json(output/"provenance.json", provenance)
        return correction
    except BaseException:
        if output.exists():
            shutil.rmtree(output)
        raise


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "output", "reference", "reformation", "comparison"):
        parser.add_argument(name, type=Path)
    args = parser.parse_args()
    restore(**vars(args))
    print(args.output.resolve())
