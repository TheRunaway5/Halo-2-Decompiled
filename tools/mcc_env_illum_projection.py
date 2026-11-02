"""Prove environment/three-channel data conversion against complete paired banks.

This operates on authored metadata, not shader programs. Run the proof through
run_test.py. Only bounded JSON reports and required metadata profiles survive.
The same conversion is used to stage literal component-test expectations; all
114 original pairs are also checked independently against their original banks.
"""
import argparse
import hashlib
import json
import lzma
import math
import os
from pathlib import Path
import struct

FIELDS = ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2), (44, 2),
          (52, 10), (60, 20), (68, 4), (76, 2), (84, 4), (92, 4), (100, 12), (108, 4))
TEMPLATE_FIELDS = ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4))
PIXELS = (0, 1, 2, 3, 4, 5, 6, 7, 9, 8, 10, 11, 12, 13)
TEMPLATE = r"shaders\shader_templates\opaque\tex_bump_env_illum_3_channel"


def need(ok, reason):
    if not ok:
        raise ValueError("Environment/illumination projection: " + reason)


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def sha(data):
    return hashlib.sha256(data).hexdigest()


def u16(data, at):
    return struct.unpack_from("<H", data, at)[0]


def u32(data, at):
    return struct.unpack_from("<I", data, at)[0]


def extent(packed, total):
    first, count = packed & 511, packed >> 9
    need(first <= total and count <= total - first, "packed range escapes its complete bank")
    return range(first, first + count)


def native_graph(directory, record):
    raw = (directory / "payloads" / record["payload"]).read_bytes()
    need(len(raw) == record["bytes"] and sha(raw) + ".bin" == record["payload"], "native payload changed")
    need(u32(raw, 88) == 1, "native template platform count changed")
    pp = u32(raw, 92)
    references = dict(record["references"])
    result = []
    for field, stride in TEMPLATE_FIELDS:
        count, at = u32(raw, pp + field), u32(raw, pp + field + 4)
        need(count <= 65536 and at + count * stride <= len(raw), "native template bank extent changed")
        bank = bytearray(raw[at:at + count * stride])
        typed = []
        if field == 16:
            for i in range(count):
                identity = references[at + i * stride + 4]
                need(identity[0] == "spas" and u32(bank, i * stride) == 0x73706173, "native pass namespace changed")
                typed.append(identity)
                struct.pack_into("<I", bank, i * stride + 4, 0)
        result.append(dict(field=field, count=count, raw_hex=bank.hex(), typed_references=typed))
    return result


def banks(graph):
    return {str(x["field"]): bytes.fromhex(x["raw_hex"]) for x in graph}


def conversion(source_arrays, native_arrays):
    """Derive roles by complete compiler remaps and paired main-slot identity."""
    source, native = banks(source_arrays), banks(native_arrays)
    sp, np = source_arrays[2]["typed_references"], native_arrays[2]["typed_references"]
    need(len(sp) == len(np) == 18, "complete pass count changed")
    implementations, mapping_rules, discarded = [], {}, []
    for p, (source_name, native_name) in enumerate(zip(sp, np, strict=True)):
        source_indices = list(extent(u16(source["16"], p * 10 + 8), len(source["24"]) // 6))
        native_indices = list(extent(u16(native["16"], p * 10 + 8), len(native["24"]) // 6))
        if source_name != native_name:
            need(p in (5, 12) and source_name == ["spas", r"shaders\shader_passes\light_response\default_light_combined_tex"] and
                 native_name == ["spas", r"shaders\shader_passes\light_response\default_light_combined_tex_first_person_detail"],
                 "unpaired source/native pass identity")
        need(len(source_indices) == len(native_indices) or
             (p in (3, 10) and len(source_indices) == 4 and len(native_indices) == 3 and
              source_name[1].endswith(r"\shadow_generate")), "unpaired implementation count")
        discarded.extend(source_indices[len(native_indices):])
        for si, ni in zip(source_indices, native_indices):
            need(ni == len(implementations), "native implementations are not complete and contiguous")
            implementations.append(si)
            if not source_name[1].endswith(r"\multichannel_self_illumination"):
                continue
            sr = extent(u16(source["24"], si * 6 + 2), len(source["32"]) // 4)
            nr = list(extent(u16(native["24"], ni * 6 + 2), len(native["32"]) // 4))
            rules = []
            for sm in sr:
                sd, sc, pixel = struct.unpack_from("<HBB", source["32"], sm * 4)
                matches = [j for j in nr if PIXELS[native["32"][j * 4 + 3]] == pixel]
                need(len(matches) == 1, "illumination pixel role has no unique native counterpart")
                nm = matches[0]
                nd, nc, _ = struct.unpack_from("<HBB", native["32"], nm * 4)
                rules.append([sd, sc, nd, nc, nr.index(nm)])
            need(len({tuple(x[:2]) for x in rules}) == len(rules), "source pixel remaps are ambiguous")
            mapping_rules[str(si)] = rules
    need(len(implementations) == 60 and discarded == [9, 35], "complete native implementation selection changed")
    return dict(implementation_sources=implementations, discarded_sources=discarded, pixel_mapping_rules=mapping_rules)


def project(row, source_arrays, native_arrays, roles):
    source = {k: bytes.fromhex(v["hex"]) for k, v in row["source_banks"].items()}
    native = banks(native_arrays)
    compiled = banks(source_arrays)
    result = dict(source)
    need(len(source["4"]) == 84 and len(source["12"]) == 56 and len(source["20"]) == 240 and
         len(source["52"]) == 620, "main source shape changed")
    need(source["36"] == compiled["8"] and source["44"] ==
         b"".join(compiled["16"][i * 10 + 8:i * 10 + 10] for i in range(18)),
         "source pass scheduling differs from its complete compiler graph")
    need(len(source["28"]) == 18 and all(u16(source["28"], i * 6 + 4) == u16(compiled["0"], i * 10)
         == u16(native["0"], i * 10) for i in range(3)), "authored animation LODs differ from paired templates")
    result["12"] = b"".join(source["12"][i * 4:(i + 1) * 4] for i in PIXELS)
    result["36"] = native["8"]
    result["44"] = b"".join(native["16"][i * 10 + 8:i * 10 + 10] for i in range(18))
    result["52"] = b"".join(source["52"][i * 10:(i + 1) * 10] for i in roles["implementation_sources"])
    # The deleted PC-only shadow rows must carry the same complete animation
    # ranges as their retained siblings. No distinct animation is discarded.
    for removed, first in ((9, 6), (35, 32)):
        need(source["52"][removed * 10:removed * 10 + 10] == source["52"][first * 10:first * 10 + 10],
             "discarded shadow implementation carries distinct authored animation")
    maps = bytearray(source["84"])
    need(len(maps) % 4 == 0, "incomplete animation remap")
    seen = set()
    for si in range(62):
        for f in (0, 2, 4, 8):
            need(not list(extent(u16(source["52"], si * 10 + f), len(maps) // 4)),
                 "animation outside paired pixel-only illumination roles")
        selected = list(extent(u16(source["52"], si * 10 + 6), len(maps) // 4))
        if not selected:
            continue
        need(str(si) in roles["pixel_mapping_rules"], "animated pixel pass has no original paired mapping")
        rules = {tuple(x[:2]): x[2:] for x in roles["pixel_mapping_rules"][str(si)]}
        ordered = []
        for i in selected:
            need(i not in seen, "shared animation-remap ownership requires additional proof")
            seen.add(i)
            destination, component, parameter = struct.unpack_from("<HBB", source["84"], i * 4)
            need((destination, component) in rules, "animated pixel destination has no native counterpart")
            nd, nc, order = rules[destination, component]
            ordered.append((order, struct.pack("<HBB", nd, nc, parameter)))
        need(len({x[0] for x in ordered}) == len(ordered), "duplicate animated constant destination")
        for i, (_, raw) in zip(selected, sorted(ordered), strict=True):
            maps[i * 4:i * 4 + 4] = raw
    need(seen == set(range(len(maps) // 4)), "animation remaps have unowned residual rows")
    result["84"] = bytes(maps)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--authority", type=Path, required=True)
    parser.add_argument("--profile-output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    need(args.authority.stat().st_size <= 1024 * 1024, "authority report exceeds bounded input")
    packed = args.authority.read_bytes()
    data = json.loads(lzma.decompress(packed))
    need(data["template"] == TEMPLATE and len(data["materials"]) == 164 and len(data["stems"]) == 1 and not data["errors"],
         "complete source census changed")
    source_graph = next(iter(data["stems"].values()))["graph"]
    native, records = None, []
    for name, meta in data["native_libraries"].items():
        path = Path(meta["path"])
        raw = path.read_bytes()
        need(sha(raw) == meta["sha256"], "native library changed after paired census")
        library = json.loads(raw)
        record = next(x for x in library["shader_tags"] if x["group"] == "stem" and x["name"] == TEMPLATE)
        graph = native_graph(path.parent, record)
        need(native is None or native == graph, "native template variants require different role mappings")
        native = graph
        records.append(dict(map=name, payload=record["payload"], bytes=record["bytes"], fnv64=record["fnv64"],
                            source_xbe_sha256=meta["source_xbe_sha256"]))
    roles = conversion(source_graph["definitions"][0]["arrays"], native)
    checked, overlays = 0, 0
    for row in data["materials"]:
        need(row["source_root"]["definition_count"] == 3 and row["source_root"]["trailing_definitions_zero"] and
             row["source_root"]["named_parameter_count"] == 0, "additional source authoring requires proof")
        for overlay in row["source_overlays"]:
            period = struct.unpack("<f", struct.pack("<I", overlay["period_word"]))[0]
            need(overlay["input"] == overlay["range"] == 0 and math.isfinite(period) and period > 0 and
                 0 < len(bytes.fromhex(overlay["curve_hex"])) <= 4096, "non-clock overlay requires a native input owner")
        need(len(row["source_overlays"]) == row["source_banks"]["60"]["count"], "incomplete overlay census")
        output = project(row, source_graph["definitions"][0]["arrays"], native, roles)
        if row["native_pair"]:
            need(row["source_images"] == row["native_images"] and row["source_overlays"] == row["native_overlays"],
                 "paired typed images or normalized overlays differ")
            for field, _ in FIELDS:
                if field not in (4, 60):
                    need(output[str(field)].hex() == row["native_banks"][str(field)]["hex"],
                         "complete original bank differs: " + row["map"] + ":" + row["shader"] + ":" + str(field))
            checked += 1
            overlays += len(row["source_overlays"])
    need(checked == 114 and len(records) == 39, "complete original control cohort changed")
    profile = dict(schema="H2_MCC_NATIVE_ENV_ILLUM_THREE_CHANNEL_PROFILES_V1", template=TEMPLATE,
        source_graph=source_graph, native_arrays=native, native_records=records, roles=roles,
        pixel_slots=PIXELS, authority=dict(path=str(args.authority), sha256=sha(packed), paired_controls=checked,
        materials=len(data["materials"]), complete_banks_per_control=14, normalized_overlay_curves=overlays))
    args.profile_output.write_text(json.dumps(profile, indent=2) + "\n")
    report = dict(passed=True, scope=__doc__, materials=164, paired_controls=checked, native_template_variants=len(records),
        source_graphs=1, native_graphs=1, complete_original_banks_checked=checked * 14, paired_overlays_checked=overlays,
        discarded_PC_implementation_indices=roles["discarded_sources"], all_authored_rows_convert=True,
        native_projection_admitted=False, profile_path=str(args.profile_output), profile_sha256=sha(args.profile_output.read_bytes()),
        helper_sha256=sha(Path(__file__).read_bytes()), authority_sha256=sha(packed),
        owned_scratch=str(owner), retained_large_artifacts=[])
    need(args.profile_output.stat().st_size < 1024 * 1024, "metadata profile exceeds report budget")
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("passed", "materials", "paired_controls", "native_template_variants")}))


if __name__ == "__main__":
    main()
