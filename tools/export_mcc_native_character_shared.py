#!/usr/bin/env python3
"""Retain the finite actual shared Chief/emblem parameter conversion proof.

Only MCC data metadata is normalized. Original programs remain opaque payloads.
No map, image bank, StringID namespace or native program is modified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from mcc_content import MccCache

ROOT = Path(__file__).resolve().parents[1]
FIELDS = ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2), (44, 2), (52, 10),
          (60, 20), (68, 4), (76, 2), (84, 4), (92, 4), (100, 12), (108, 4))
TEMPLATE_FIELDS = ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4))
SOURCE_LIBRARY = "f523c80325d5fe3f8bff7d4f3768a3b97fb2d9904a9fa455f307757e7436917e"
SOURCE_SHARED = "fafaceef449ec05fcf7a1e35a9164788813a41eb082ff5353932c94a9bbf888f"

def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def u32(raw, at):
    return struct.unpack_from("<I", raw, at)[0]

def put(raw, at, word):
    struct.pack_into("<I", raw, at, word)

def require(value, message):
    if not value:
        raise ValueError("Shared character source: " + message)

def tag_json(tag):
    return {key: tag[key] for key in ("group", "name", "datum", "address", "size")}

def generate(cache):
    cache = Path(cache)
    native = cache / "native-shaders/coagulation"
    library = json.loads((native / "library.json").read_bytes())
    require(digest(canonical(library)) == SOURCE_LIBRARY, "original complete library changed")
    records = {(r["group"], r["name"]): r for r in library["shader_tags"]}
    shared = cache / "halo2/h2_maps_win64_dx11/shared.map"
    require(digest(shared.read_bytes()) == SOURCE_SHARED, "complete actual shared source changed")
    source = MccCache(shared)
    profiles, cases = [], []
    for leaf in ("masterchief", "mc_emblem"):
        name = "objects\\characters\\masterchief\\shaders\\" + leaf
        shader = next(t for t in source.tags.values() if t["group"] == "shad" and t["name"] == name)
        regions = {}
        def read(address, count):
            at = source.pointer(address, source.magic, count)
            raw = bytes(source.data[at:at + count])
            if count:
                previous = regions.get(address, raw)
                require(previous[:min(len(previous), len(raw))] == raw[:min(len(previous), len(raw))],
                        "ambiguous source allocation bytes")
                regions[address] = max((previous, raw), key=len)
            return raw
        root = read(shader["address"], 84)
        stem = source.tags[u32(root, 4)]
        template = bytearray(read(stem["address"], 96))
        definitions = read(u32(template, 92), 120)
        put(template, 92, 0)
        graph = {"root_hex": template.hex(), "definitions": []}
        for platform in range(3):
            definition = bytearray(definitions[platform * 40:(platform + 1) * 40])
            arrays = []
            for field, stride in TEMPLATE_FIELDS:
                count, pointer = u32(definition, field), u32(definition, field + 4)
                raw = bytearray(read(pointer, count * stride)) if count else bytearray()
                references = []
                if field == 16:
                    for i in range(count):
                        target = source.tags[u32(raw, i * 10 + 4)]
                        references.append([target["group"], target["name"]])
                        put(raw, i * 10 + 4, 0)
                arrays.append({"field": field, "count": count, "raw_hex": raw.hex(),
                               "typed_references": references})
                put(definition, field + 4, 0)
            graph["definitions"].append({"definition_hex": definition.hex(), "arrays": arrays})
        definitions = read(u32(root, 36), 372)
        definition = definitions[:124]
        banks = {str(f): bytearray(read(u32(definition, f + 4), u32(definition, f) * stride))
                 if u32(definition, f) else bytearray() for f, stride in FIELDS}
        runtime = read(u32(root, 16), 80)
        predictions = read(u32(root, 48), u32(root, 44) * 8) if u32(root, 44) else b""
        image_roles = []
        for at in range(0, len(banks["4"]), 12):
            image = source.tags[u32(banks["4"], at)]
            image_roles.append([[image["group"], image["name"]], u32(banks["4"], at + 4),
                                u32(banks["4"], at + 8)])
            put(banks["4"], at, 0)
        curves = []
        for at in range(0, len(banks["60"]), 20):
            curve = read(u32(banks["60"], at + 16), u32(banks["60"], at + 12))
            curves.append(curve.hex())
            put(banks["60"], at + 16, 0)
        native_record = records["stem", stem["name"]]
        native_template = (native / "payloads" / native_record["payload"]).read_bytes()
        native_shader_record = records["shad", name]
        original = (native / "payloads" / native_shader_record["payload"]).read_bytes()
        for record, raw in ((native_record, native_template), (native_shader_record, original)):
            require(digest(raw) + ".bin" == record["payload"] and len(raw) == record["bytes"],
                    "complete original payload identity changed")
        pp = u32(original, 36)
        original_banks = {str(f): original[u32(original, pp + f + 4):u32(original, pp + f + 4) + u32(original, pp + f) * stride]
                          if u32(original, pp + f) else b"" for f, stride in FIELDS}
        # All authored pixel, property and noninterpolated values are literal
        # identical. The finite armor setup has one authenticated original UV
        # row and a compiler reorder; no other instance is admitted by it.
        for field in (12, 92, 100, 108):
            require(bytes(banks[str(field)]) == original_banks[str(field)], "authored common bank differs")
        if leaf == "masterchief":
            vectors = bytes(banks["20"])
            mapping = (2, 1, None, 3, 11, 12, 4, 5, 6, 7, 9, 10)
            transformed = b"".join(struct.pack("<4f", 0, 0, 1, 1) if i is None else vectors[i * 16:(i + 1) * 16] for i in mapping)
            require(transformed == original_banks["20"], "complete armor source vector mapping differs")
        else:
            require(bytes(banks["20"]) == original_banks["20"], "emblem authored UV differs")
            for i, curve in enumerate(curves):
                overlay = original_banks["60"]
                size, pointer = u32(overlay, i * 20 + 12), u32(overlay, i * 20 + 16)
                require(original[pointer:pointer + size].hex() == curve, "original complete emblem function differs")
                require(bytes(banks["60"])[i * 20 + 4:i * 20 + 16] == overlay[i * 20 + 4:i * 20 + 16],
                        "emblem range, period or curve extent changed")
        input_names = []
        native_strings = dict(native_shader_record["strings"])
        for i in range(len(curves)):
            identifier = u32(banks["60"], i * 20)
            if not identifier:
                require(u32(original_banks["60"], i * 20) == 0,
                        "source clock became an original object input")
                input_names.append("")
                continue
            index = identifier & 0x00ffffff
            require(index < source.u32(48), "source input StringID escaped actual table")
            offset = source.u32(source.u32(60) + index * 4)
            require(offset < source.u32(56), "source input StringID offset escaped actual table")
            text = source.string(source.u32(52) + offset, source.u32(56) - offset)
            require(native_strings[u32(original, pp + 64) + i * 20] == text,
                    "source/native animation input semantics differ")
            input_names.append(text)
        original_images = []
        references = dict(native_shader_record["references"])
        for i in range(len(original_banks["4"]) // 12):
            image = original_banks["4"][i * 12:(i + 1) * 12]
            ref = references[u32(original, pp + 8) + i * 12]
            original_images.append([ref, u32(image, 4), u32(image, 8)])
            require(image_roles[i][:2] == original_images[i][:2], "original image role/frame differs")
        native_pp = u32(native_template, 92)
        normalized_root = bytearray(root)
        for at in (4, 16, 36, 48):
            put(normalized_root, at, 0)
        normalized_definition = bytearray(definition)
        put(normalized_definition, 0, 0)
        for f, _ in FIELDS:
            put(normalized_definition, f + 4, 0)
        profile = {"template": stem["name"], "source_graph": graph,
            "source_graph_sha256": digest(canonical(graph)), "source_root_hex": normalized_root.hex(),
            "source_definition_hex": normalized_definition.hex(),
            "source_banks": {f: bytes(b).hex() for f, b in banks.items()}, "images": image_roles,
            "runtime_hex": runtime.hex(), "curves": curves, "native_source_map": "coagulation",
            "input_names": input_names,
            "native_record": native_record, "native_shader_record": native_shader_record,
            "native_banks": {f: b.hex() for f, b in original_banks.items() if f not in ("4", "60")},
            "native_shape": {str(f): u32(native_template, native_pp + f) for f, _ in TEMPLATE_FIELDS},
            "original_images": original_images,
            "image_log_words_retained_from_source": image_roles != original_images}
        profile["prediction_roles"] = [
            [[source.tags[u32(predictions, i * 8 + 4)]["group"],
              source.tags[u32(predictions, i * 8 + 4)]["name"]],
             struct.unpack_from("<H", predictions, i * 8 + 2)[0]]
            for i in range(len(predictions) // 8)]
        # Runtime contains actual source handles. The contract compares them
        # with the admitted image role rather than a baked source salt.
        for at in (4, 12, 48, 56):
            value = u32(runtime, at)
            if value != 0xffffffff:
                require(value in [source.u32(source.pointer(u32(definition, 8), source.magic, 12) + i * 12)
                                  for i in range(u32(definition, 4))], "runtime image has no authored role")
        profile["runtime_reference_slots"] = {str(at): next((i for i in range(u32(definition, 4))
            if source.u32(source.pointer(u32(definition, 8), source.magic, 12) + i * 12) == u32(runtime, at)), -1)
            for at in (4, 12, 48, 56)}
        normalized_runtime = bytearray(runtime)
        for at, index in profile["runtime_reference_slots"].items():
            if index >= 0:
                put(normalized_runtime, int(at), 0)
        profile["runtime_hex"] = normalized_runtime.hex()
        profiles.append(profile)
        native_stem = dict(stem, address=0xf0000000, size=len(native_template))
        needed = {shader["datum"], stem["datum"]}
        for p in range(3):
            # Template references keep the complete original typed pass owner
            # even though its native counterpart may later occupy that row.
            raw_definition = read(source.u32(stem["offset"] + 92) + p * 40, 40)
            raw_passes = read(u32(raw_definition, 20), u32(raw_definition, 16) * 10) if u32(raw_definition, 16) else b""
            needed.update(u32(raw_passes, i * 10 + 4) for i in range(len(raw_passes) // 10))
        original_images_bank = read(u32(definition, 8), u32(definition, 4) * 12)
        needed.update(u32(original_images_bank, i * 12) for i in range(u32(definition, 4)))
        needed.update(u32(predictions, i * 8 + 4) for i in range(len(predictions) // 8))
        namespace = [tag_json(t) for t in source.tags.values() if t["datum"] in needed]
        namespace = [tag_json(native_stem) if t["datum"] == stem["datum"] else t for t in namespace]
        counts = {str(t["datum"]): source.u32(t["offset"] + 68) for t in source.tags.values()
                  if t["group"] == "bitm" and t["offset"] is not None and t["size"] >= 76}
        expected = {f: b.hex() for f, b in original_banks.items()}
        expected["4"] = read(u32(definition, 8), u32(definition, 4) * 12).hex()
        expected["60"] = bytes(banks["60"]).hex()
        cases.append({"shader": tag_json(shader), "source_mcc_stem": tag_json(stem),
            "native_stem": tag_json(native_stem), "native_payload_name": native_record["payload"],
            "native_payload_hex": native_template.hex(), "namespace": namespace,
            "bitmap_frame_counts": counts, "regions": [[a, b.hex()] for a, b in sorted(regions.items())],
            "expected_banks": expected, "curves": curves, "runtime_hex": runtime.hex(),
            "predictions_hex": predictions.hex(), "root_hex": root.hex(),
            "string_ids": {str(u32(banks["60"], i * 20)): text for i, text in enumerate(input_names) if text}})
    source.close()
    return {"schema": "H2_MCC_NATIVE_CHARACTER_SHARED_V1", "source_shared_sha256": SOURCE_SHARED,
        "source_library_canonical_sha256": SOURCE_LIBRARY,
        "scope": "Two exact actual shared character setups only; images and original animation input IDs/curves remain authored",
        "profiles": profiles}, {"schema": "H2_MCC_CHARACTER_SHARED_FIXTURE_V1", "cases": cases}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cache", type=Path, required=True)
    parser.add_argument("--fixture", type=Path)
    parser.add_argument("--policy", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    policy, fixture = generate(args.cache)
    if args.policy:
        text = json.dumps(policy, indent=2) + "\n"
        if args.check:
            require(args.policy.read_text() == text, "finite source policy differs from actual complete inputs")
        else:
            args.policy.write_text(text)
    if args.fixture:
        args.fixture.write_bytes(canonical(fixture))
    print("Two complete shared character setups authenticated")

if __name__ == "__main__":
    main()
