#!/usr/bin/env python3
"""Export literal MCC world-material admission inputs into owned test scratch.

This does not alter a map, decode a shader, or claim a live/GPU draw. The native
template is placed at an explicitly translated test allocation; source MCC
addresses, names, salted handles, root bytes and child banks remain literal.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct

from audit_rendering import FormatError
from mcc_content import MccCache

FIELDS = ((4, 12), (12, 4), (20, 16), (28, 6), (36, 2), (44, 2),
          (52, 10), (60, 20), (68, 4), (76, 2), (84, 4), (92, 4),
          (100, 12), (108, 4))
NATIVE = {
    "overlay": "01b_spacestation", "tex_alpha_test": "01b_spacestation",
    "tex_bump_alpha_test": "01b_spacestation",
    "tex_bump_alpha_test_single_pass": "01b_spacestation",
    "tex_bump_env_alpha_test_combined": "01b_spacestation",
    "tex_bump_env_combined": "01b_spacestation",
    "tex_bump_env_illum_combined": "01b_spacestation",
    "tex_bump_illum": "01b_spacestation", "one_alpha_env": "03a_oldmombasa",
    "one_alpha_env_illum": "dune", "water_edge_blend": "dune",
    "decal_simple": "03b_newmombasa", "tex_bump_detail_keep": "05a_deltaapproach",
}


def require(value, message):
    if not value:
        raise FormatError("World cooked fixture: " + message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=False).encode()


def u32(data, at):
    return struct.unpack_from("<I", data, at)[0]


def tag_json(tag):
    return {key: tag[key] for key in ("group", "name", "datum", "address", "size")}


def transformed_main(family, banks):
    result = {str(field): bytes.fromhex(banks[str(field)]["hex"]) for field in (4, 12, 20)}
    if family == "tex_alpha_test":
        # The authenticated native template has no pixel slots. These two
        # PC-only words are not constrained to zero and are not TU inputs.
        result["12"] = b""
    elif family == "tex_bump_env_combined":
        result["12"] = bytes.fromhex("ffffff00") + result["12"]
        vectors = result["20"]
        result["20"] = vectors[32:48] + vectors[16:32] + vectors[:16] + vectors[48:]
    elif family == "tex_bump_env_alpha_test_combined":
        vectors = result["20"]
        result["20"] = (vectors[32:48] + vectors[16:32] + vectors[:16] +
                        bytes.fromhex("00000000000000000000803f0000803f") + vectors[48:])
    elif family in ("one_alpha_env", "one_alpha_env_illum"):
        pixels = result["12"]
        result["12"] = pixels[:4] + pixels[8:12] + pixels[4:8] + pixels[12:]
    return result


class Exporter:
    def __init__(self, root, prepared, evidence):
        self.root, self.prepared, self.evidence = root, prepared, evidence
        self.maps = prepared / "halo2/h2_maps_win64_dx11"
        self.shared = MccCache(self.maps / "shared.map")
        self.cache = None
        self.pins = {}
        closure = json.loads((root / "tools/data/mcc-native-library-closure.json").read_bytes())
        self.authorities = {row["map"]: row["canonical_sha256"] for row in closure["libraries"]}
        self.native = {}
        self.exclusions = []

    def close(self):
        if self.cache:
            self.cache.close()
        self.shared.close()

    def select(self, name):
        if self.cache and self.cache.path.stem == name:
            return self.cache
        if self.cache:
            self.cache.close()
        path = self.maps / (name + ".map")
        self.cache = MccCache(path)
        actual = sha(path.read_bytes())
        require(actual == self.evidence["map_sha256"][name], "complete raw MCC input changed: " + name)
        self.pins[name] = actual
        return self.cache

    def resolve(self, cache, datum):
        tag = cache.tags.get(datum)
        require(tag is not None, "source full-salt handle is absent")
        if tag["offset"] is not None:
            return cache, tag
        other = self.shared.tags.get(datum)
        if (other and other["offset"] is not None and
                (other["name"], other["group"]) == (tag["name"], tag["group"])):
            return self.shared, other
        return cache, tag

    def native_template(self, template):
        if template in self.native:
            return self.native[template]
        family = template.rsplit("\\", 1)[-1]
        name = NATIVE[family]
        directory = self.prepared / "native-shaders" / name
        library = json.loads((directory / "library.json").read_bytes())
        require(sha(canonical(library)) == self.authorities[name], "complete native library changed")
        selected = [r for r in library["shader_tags"] if r["group"] == "stem" and r["name"] == template]
        require(len(selected) == 1, "ambiguous or missing native template")
        record = selected[0]
        payload = (directory / "payloads" / record["payload"]).read_bytes()
        require(len(payload) == record["bytes"] and sha(payload) + ".bin" == record["payload"],
                "complete native template payload changed")
        require(u32(payload, 88) == 1 and u32(payload, 92) == 96, "native template shape changed")
        self.native[template] = (name, record, payload)
        return self.native[template]

    def fixture(self, row, ordinal, kind):
        cache = self.select(row["map"])
        shader = cache.tags[row["datum"]]
        require(shader["name"] == row["shader"] and shader["group"] == "shad", "source shader owner changed")
        root = shader["offset"]
        source_owner, source_stem = self.resolve(cache, cache.u32(root + 4))
        require(source_stem["offset"] is not None and source_stem["name"] == row["template"],
                "source stem is not resident in its exact shared owner")
        native_map, native_record, native_payload = self.native_template(row["template"])
        regions, names, frames, mutation_regions = {}, {}, {}, []

        def namespace(datum):
            owner, tag = self.resolve(cache, datum)
            names[datum] = tag_json(tag)
            if tag["group"] == "bitm":
                require(tag["offset"] is not None and tag["size"] >= 76, "actual source bitmap is nonresident")
                frames[datum] = len(owner.block(tag["offset"] + 68, 168))
            return owner, tag

        def region(owner, address, count, label=None):
            if not count:
                return
            at = owner.pointer(address, owner.magic, count)
            raw = bytes(owner.data[at:at + count])
            if address in regions:
                require(regions[address] == raw or regions[address].startswith(raw) or raw.startswith(regions[address]),
                        "source address has inconsistent owners")
                if len(raw) <= len(regions[address]):
                    return
            for first, old in regions.items():
                low, high = max(first, address), min(first + len(old), address + len(raw))
                require(low >= high or old[low - first:high - first] == raw[low - address:high - address],
                        "source address allocations overlap inconsistently")
            regions[address] = raw
            if label:
                mutation_regions.append(dict(address=address, bytes=count, label=label))

        namespace(shader["datum"])
        names[source_stem["datum"]] = tag_json(source_stem)
        region(cache, shader["address"], shader["size"])
        runtime_count, runtime_pointer = cache.u32(root + 12), cache.u32(root + 16)
        require(runtime_count == 1, "source runtime property count changed")
        region(cache, runtime_pointer, 80)
        runtime_at = cache.pointer(runtime_pointer, cache.magic, 80)
        for field in (4, 12, 48, 56):
            value = cache.u32(runtime_at + field)
            if value != 0xffffffff:
                namespace(value)
        definition_count, definition_pointer = cache.u32(root + 32), cache.u32(root + 36)
        require(definition_count == 3, "source platform definition count changed")
        region(cache, definition_pointer, 3 * 124)
        pp = cache.pointer(definition_pointer, cache.magic, 3 * 124)
        expected_banks = {}
        for field, stride in FIELDS:
            count, pointer = cache.u32(pp + field), cache.u32(pp + field + 4)
            region(cache, pointer, count * stride)
            raw = bytes(cache.data[cache.pointer(pointer, cache.magic, count * stride):][:count * stride]) if count else b""
            require(raw.hex() == row["source_banks"][str(field)]["hex"], "literal source cooked bank changed")
            expected_banks[str(field)] = raw
        images = expected_banks["4"]
        for at in range(0, len(images), 12):
            datum, frame = u32(images, at), u32(images, at + 4)
            require(datum != 0xffffffff, "paired source requires an authored NONE slot outside this PC world contract")
            namespace(datum)
            require(frame < frames[datum], "source bitmap frame escapes actual metadata")
        predictions = cache.u32(root + 44)
        prediction_pointer = cache.u32(root + 48)
        region(cache, prediction_pointer, predictions * 8)
        if predictions:
            at = cache.pointer(prediction_pointer, cache.magic, predictions * 8)
            for i in range(predictions):
                namespace(cache.u32(at + i * 8 + 4))

        stem_at = source_stem["offset"]
        region(source_owner, source_stem["address"], 96, "source-stem-root")
        definitions = source_owner.block(stem_at + 88, 40)
        require(len(definitions) == 3, "literal source stem definition count changed")
        region(source_owner, source_owner.u32(stem_at + 92), 120, "source-stem-definitions")
        for definition in definitions:
            for field, stride in ((0, 10), (8, 2), (16, 10), (24, 6), (32, 4)):
                count, pointer = source_owner.u32(definition + field), source_owner.u32(definition + field + 4)
                region(source_owner, pointer, count * stride, "source-stem-bank-" + str(field))
                if field == 16 and count:
                    at = source_owner.pointer(pointer, source_owner.magic, count * stride)
                    for i in range(count):
                        namespace(source_owner.u32(at + i * stride + 4))
        translated_native = dict(group="stem", name=row["template"], datum=source_stem["datum"],
                                 address=0x70000000 + ordinal * 0x10000, size=len(native_payload))
        names[translated_native["datum"]] = translated_native
        main = transformed_main(row["template"].rsplit("\\", 1)[-1], row["source_banks"])
        expected_banks.update(main)
        native_pp = 96

        def native_bank(field, stride):
            count, pointer = u32(native_payload, native_pp + field), u32(native_payload, native_pp + field + 4)
            require(pointer + count * stride <= len(native_payload), "native template bank extent changed")
            return count, native_payload[pointer:pointer + count * stride]

        count, lods = native_bank(0, 10)
        expected_banks["28"] = b"".join(struct.pack("<IH", 0, struct.unpack_from("<H", lods, i * 10)[0])
                                          for i in range(count))
        _, expected_banks["36"] = native_bank(8, 2)
        count, passes = native_bank(16, 10)
        expected_banks["44"] = b"".join(passes[i * 10 + 8:i * 10 + 10] for i in range(count))
        count, _ = native_bank(24, 6)
        expected_banks["52"] = bytes(count * 10)
        result = dict(kind=kind, map=row["map"], template=row["template"],
            shader=tag_json(shader), source_mcc_stem=tag_json(source_stem), native_stem=translated_native,
            native_source_map=native_map, native_payload_name=native_record["payload"],
            native_payload_hex=native_payload.hex(), namespace=list(names.values()),
            bitmap_frame_counts={str(k): n for k, n in frames.items()},
            regions=[[a, b.hex()] for a, b in sorted(regions.items())],
            stem_mutation_regions=mutation_regions,
            expected_banks={k: b.hex() for k, b in expected_banks.items()},
            source_stem_graph_sha256=row["source_stem_graph_sha256"],
            source_root_hex=bytes(cache.data[root:root + 84]).hex(),
            expected_runtime_hex=bytes(cache.data[runtime_at:runtime_at + 80]).hex(),
            expected_predictions_hex=(bytes(cache.data[cache.pointer(prediction_pointer, cache.magic, predictions * 8):]
                                           [:predictions * 8]).hex() if predictions else ""))
        if row["native_pair"]:
            result["original_main"] = dict(images=row["native_images"],
                pixels_hex=row["native_banks"]["12"]["hex"], vectors_hex=row["native_banks"]["20"]["hex"],
                source_shader_payload=row["native_shader_payload"])
        return result

    def export(self):
        materials = self.evidence["materials"]
        cases = []
        for row in materials:
            family = row["template"].rsplit("\\", 1)[-1]
            if row["map"] in ("needle", "street_sweeper") and family in NATIVE:
                require(all(row["source_banks"][str(f)]["count"] == 0 for f in (60, 68, 76, 84)),
                        "requested PC source is animated")
                cases.append(self.fixture(row, len(cases), "pc-geometry-owner"))
        require(len(cases) == 31, "complete additional PC geometry owner census changed")
        for family in NATIVE:
            candidates = []
            for row in materials:
                if not row["native_pair"] or row["template"].rsplit("\\", 1)[-1] != family:
                    continue
                if any(row["source_banks"][str(f)]["count"] for f in (60, 68, 76, 84)):
                    continue
                mapped = transformed_main(family, row["source_banks"])
                exact = (row["normalized_images_exact"] and mapped["12"].hex() == row["native_banks"]["12"]["hex"] and
                         mapped["20"].hex() == row["native_banks"]["20"]["hex"])
                if not exact:
                    self.exclusions.append(dict(map=row["map"], shader=row["shader"], template=row["template"],
                        reason="Literal MCC authored main banks differ from the original; not fitted or used as a paired positive"))
                    continue
                candidates.append(row)
            candidates.sort(key=lambda row: (row["map"] != NATIVE[family], row["map"], row["shader"]))
            admitted = 0
            for row in candidates:
                try:
                    case = self.fixture(row, len(cases), "paired-original-main")
                except FormatError as error:
                    self.exclusions.append(dict(map=row["map"], shader=row["shader"], template=row["template"],
                                                reason=str(error)))
                    continue
                cases.append(case)
                admitted += 1
                if admitted == 2:
                    break
            require(admitted, "no complete real paired source fixture for family: " + family)
        return dict(schema="H2_MCC_NATIVE_WORLD_COOKED_FIXTURE_V1",
            scope="Literal data admission component fixture; translated native test allocations; no live/GPU draw claim",
            map_sha256=self.pins, cases=cases, paired_exclusions=self.exclusions,
            expected_pc_geometry_owners=31)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--prepared-root", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    root, prepared = args.repo_root.resolve(strict=True), args.prepared_root.resolve(strict=True)
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = args.output.resolve()
    require(output.is_relative_to(scratch), "fixture must stay inside owned scratch")
    evidence = json.loads(lzma.decompress(args.evidence.read_bytes()))
    exporter = Exporter(root, prepared, evidence)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    raw = (json.dumps(fixture, indent=2) + "\n").encode()
    output.write_bytes(raw)
    report = dict(scope=fixture["scope"], fixture_bytes=len(raw), fixture_sha256=sha(raw),
        pc_geometry_owners=sum(c["kind"] == "pc-geometry-owner" for c in fixture["cases"]),
        paired_original_fixtures=sum(c["kind"] == "paired-original-main" for c in fixture["cases"]),
        paired_exclusions=fixture["paired_exclusions"], source_map_sha256=fixture["map_sha256"],
        fixture=str(output))
    if args.report_output:
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "paired_exclusions"}, indent=2))


if __name__ == "__main__":
    main()
