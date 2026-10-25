#!/usr/bin/env python3
"""Export the paired, cooked Elongation glass projection contract.

Run through tools/run_test.py. The only output is the explicitly requested
small JSON fixture/report. Assets are read-only. Synthetic native/template and
nonresident bitmap owner addresses are declared test bindings, never asset
repairs or evidence of raw MCC residency.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_rendering import Cache
from mcc_content import MccCache

ROOT = Path(__file__).resolve().parents[1]
STEM = r"shaders\shader_templates\transparent\two_alpha_env_illum"
SOURCE = r"scenarios\shaders\human\military\glass\glass_inte_dlc"
ORIGINAL = r"scenarios\shaders\human\military\glass\glass_interior"
FIELDS = ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2), (44, 2),
          (52, 10), (60, 20), (68, 4), (76, 2), (84, 4), (92, 4),
          (100, 12), (108, 4))


def digest(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return f"{value:016x}"


def tag_record(tag):
    return {key: tag[key] for key in ("group", "name", "datum", "address", "size")}


def extract(cache, name):
    candidates = [tag for tag in cache.tags.values()
                  if tag["group"] == "shad" and tag["name"] == name]
    if len(candidates) != 1:
        raise ValueError("Expected one exact material identity: " + name)
    tag = candidates[0]
    offset = tag["address"] - cache.magic
    root = bytes(cache.data[offset:offset + 84])
    definitions = cache.block(offset + 32, 124)
    if not definitions:
        raise ValueError("Material lacks its cooked definition")
    pp = definitions[0]
    result = {"shader": tag_record(tag), "root_hex": root.hex(),
              "banks": {}, "regions": [[tag["address"], root.hex()]]}
    for root_field, stride, label in ((12, 80, "runtime"),
                                     (32, 124, "definitions"),
                                     (44, 8, "predictions")):
        count = cache.u32(offset + root_field)
        pointer = cache.u32(offset + root_field + 4)
        data = bytes(cache.data[pointer - cache.magic:
                                pointer - cache.magic + count * stride]) if count else b""
        result[label] = {"count": count, "address": pointer,
                         "hex": data.hex(), "fnv64": fnv(data)}
        if count:
            result["regions"].append([pointer, data.hex()])
    for field, stride in FIELDS:
        count = cache.u32(pp + field)
        pointer = cache.u32(pp + field + 4)
        data = bytes(cache.data[pointer - cache.magic:
                                pointer - cache.magic + count * stride]) if count else b""
        if len(data) != count * stride:
            raise ValueError("Material child bank escapes cache")
        result["banks"][str(field)] = {"count": count, "stride": stride,
                                      "address": pointer, "hex": data.hex(),
                                      "fnv64": fnv(data)}
        if count:
            result["regions"].append([pointer, data.hex()])
    result["bitmaps"] = []
    for at in cache.block(pp + 4, 12):
        handle = cache.u32(at)
        result["bitmaps"].append({"tag": tag_record(cache.tags[handle]),
                                  "frame": cache.u32(at + 4),
                                  "log_dimension_word": cache.u32(at + 8)})
    result["normalized_predictions"] = []
    for at in cache.block(offset + 44, 8):
        bitmap = cache.tags[cache.u32(at + 4)]
        result["normalized_predictions"].append(
            [cache.u16(at), cache.u16(at + 2), bitmap["group"], bitmap["name"]])
    result["normalized_runtime"] = []
    for at in cache.block(offset + 12, 80):
        data = bytearray(cache.data[at:at + 80])
        references = []
        for field in (0, 8, 44, 52):
            handle = cache.u32(at + field + 4)
            bitmap = cache.tags.get(handle)
            references.append(None if handle == 0xffffffff else
                              [bitmap["group"], bitmap["name"]])
            data[field + 4:field + 8] = bytes(4)
        result["normalized_runtime"].append(
            {"bytes_without_datums": data.hex(), "references": references})
    return result


def export(args):
    source_cache = MccCache(args.mcc_map)
    original_cache = Cache(args.original_map)
    try:
        source = extract(source_cache, SOURCE)
        original = extract(original_cache, ORIGINAL)
        library = json.loads(args.native_library.read_text())
        records = [record for record in library["shader_tags"]
                   if record["group"] == "stem" and record["name"] == STEM]
        if len(records) != 1:
            raise ValueError("Native glass template closure is ambiguous")
        record = records[0]
        payload_path = args.native_library.parent / "payloads" / record["payload"]
        payload = payload_path.read_bytes()
        if (len(payload) != record["bytes"] or fnv(payload) != record["fnv64"] or
                hashlib.sha256(payload).hexdigest() + ".bin" != record["payload"]):
            raise ValueError("Native glass template payload identity changed")
        native = next(tag for tag in source_cache.tags.values()
                      if tag["group"] == "stem" and tag["name"] == STEM)
        native = tag_record(native)
        native.update(address=0x90001000, size=len(payload))
        namespace = [source["shader"], native]
        frames = {}
        synthetic = []
        for index, image in enumerate(source["bitmaps"]):
            bitmap = dict(image["tag"])
            if not bitmap["address"] or not bitmap["size"]:
                bitmap.update(address=0x91000000 + index * 0x1000, size=0xac)
                synthetic.append(bitmap["name"])
            namespace.append(bitmap)
            # The fixture tests its explicit selected-frame ownership callback.
            # It makes no claim about raw shared-map residency or mip content.
            frames[str(bitmap["datum"])] = image["frame"] + 1
        source["native_stem"] = native
        source["namespace"] = namespace
        source["bitmap_frame_counts"] = frames
        fixture = {
            "schema": "H2_MCC_ELONGATION_GLASS_TU15_V1",
            "scope": "Paired cooked source/original shader metadata; complete explicit authored tuple, not a suffix alias rule or original framebuffer proof",
            "parameter_name_evidence": "Both cooked shad author parameter banks are empty and the raw MCC stem is nonresident; pixel ordering is proved only by this complete paired tuple",
            "declared_test_bindings": {"native_template_address": native["address"],
                                       "synthetic_bitmap_owners": synthetic,
                                       "bitmap_frame_counts": "Selected-frame ownership callbacks only; independent actual-cache test owns residency/pixels"},
            "inputs": {"mcc_map": str(args.mcc_map.resolve()),
                       "mcc_map_sha256": digest(args.mcc_map),
                       "original_map": str(args.original_map.resolve()),
                       "original_map_sha256": digest(args.original_map),
                       "native_library": str(args.native_library.resolve()),
                       "native_library_sha256": digest(args.native_library),
                       "exporter_sha256": digest(Path(__file__)),
                       "mcc_parser_sha256": digest(ROOT / "tools/mcc_content.py"),
                       "original_parser_sha256": digest(ROOT / "tools/audit_rendering.py")},
            "native_payload_name": record["payload"],
            "native_payload_hex": payload.hex(), "native_template_record": record,
            "source": source, "original": original,
            "runtime_normalized_exact": source["normalized_runtime"] == original["normalized_runtime"],
            "predictions_normalized_exact": source["normalized_predictions"] == original["normalized_predictions"],
        }
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(fixture, indent=2) + "\n")
        print(json.dumps({"output": str(args.output), "bytes": args.output.stat().st_size,
                          "source_pixel_words": list(struct.unpack("<4I", bytes.fromhex(source["banks"]["12"]["hex"]))),
                          "original_pixel_words": list(struct.unpack("<4I", bytes.fromhex(original["banks"]["12"]["hex"]))),
                          "runtime_normalized_exact": fixture["runtime_normalized_exact"],
                          "predictions_normalized_exact": fixture["predictions_normalized_exact"]}))
    finally:
        source_cache.close()
        original_cache.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mcc-map", type=Path, default=ROOT / "assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11/elongation.map")
    parser.add_argument("--original-map", type=Path, default=ROOT / "assets/map-pack/maps/elongation.map")
    parser.add_argument("--native-library", type=Path, default=ROOT / "assets/imported/mcc-dire-valley-polish-20261023/native-shaders/elongation/library.json")
    parser.add_argument("--output", type=Path, required=True)
    export(parser.parse_args())


if __name__ == "__main__":
    main()
