"""Stage complete literal environment/illumination component inputs in scratch.

All164 authored rows,114 original counterparts and39 exact native template
variants are retained as metadata. No map or shader program is modified.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct

from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import canonical, require, sha, tag_json
from mcc_content import MccCache
from mcc_env_illum_projection import project
import steam_workshop as workshop


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


class EnvironmentExporter(AnimatedExporter):
    def __init__(self, root, prepared, evidence, profile):
        super().__init__(root, prepared, evidence)
        self.profile = profile
        self.selected_name = None
        self.active_native = None
        self.forced_native = None
        self.shared_pins = {}
        mod = next(x for x in workshop.scan()["mods"] if x["id"] == "3360515088")
        item = Path(mod["directory"])
        inputs = workshop.resource_inputs(item, prepared, workshop.read_mod(item))
        self.workshop_shared = Path(inputs["maps"]["shared.map"]).resolve(strict=True)

    def select(self, name):
        if self.selected_name == name:
            return self.cache
        if self.cache:
            self.cache.close()
        entry = self.evidence["maps"][name]
        path = Path(entry["path"]).resolve(strict=True)
        require(digest(path) == entry["sha256"], "complete source map changed: " + name)
        shared = self.workshop_shared if name.startswith("workshop:") else (self.maps / "shared.map").resolve(strict=True)
        expected = self.evidence["maps"]["shared:" + str(shared)]
        if str(shared) not in self.shared_pins:
            require(digest(shared) == expected, "complete shared source map changed")
        else:
            require(self.shared_pins[str(shared)] == expected, "shared owner pins disagree")
        self.shared_pins[str(shared)] = expected
        if self.shared.path.resolve() != shared:
            self.shared.close()
            self.shared = MccCache(shared)
        self.cache = MccCache(path)
        self.selected_name = name
        self.pins[str(path)] = entry["sha256"]
        return self.cache

    def native_template(self, template):
        name = self.forced_native or self.active_native
        key = (name, template)
        if key in self.native:
            return self.native[key]
        directory = self.prepared / "native-shaders" / name
        library = json.loads((directory / "library.json").read_bytes())
        require(sha(canonical(library)) == self.authorities[name], "complete native library changed")
        record = next(x for x in library["shader_tags"] if x["group"] == "stem" and x["name"] == template)
        payload = (directory / "payloads" / record["payload"]).read_bytes()
        require(len(payload) == record["bytes"] and sha(payload) + ".bin" == record["payload"], "native template changed")
        self.native[key] = name, record, payload
        return self.native[key]

    def fixture(self, row, ordinal, kind):
        self.active_native = row["map"] if row["map"] in self.evidence["native_libraries"] else "03a_oldmombasa"
        result = super().fixture(row, ordinal, kind)
        if result["shader"]["size"] == 0:
            shader = self.cache.tags[row["datum"]]
            owned = [x for x in self.cache.tags.values() if x["address"] == shader["address"] and
                     x["group"] == shader["group"] and x["size"]]
            require(len(owned) == 1 and owned[0]["size"] >= 84, "shader alias lacks its exact typed payload owner")
            owner = owned[0]
            result["namespace"].append(tag_json(owner))
            at = self.cache.pointer(owner["address"], self.cache.magic, owner["size"])
            result["regions"].append([owner["address"], bytes(self.cache.data[at:at + owner["size"]]).hex()])
            result["source_alias_owner"] = tag_json(owner)
        expected = project(row, self.profile["source_graph"]["definitions"][0]["arrays"],
                           self.profile["native_arrays"], self.profile["roles"])
        overlays = bytearray(expected["60"])
        for i in range(len(overlays) // 20):
            struct.pack_into("<I", overlays, i * 20 + 16, 0)
        expected["60"] = bytes(overlays)
        result["expected_banks"] = {k: v.hex() for k, v in expected.items()}
        require(result["expected_overlays"] == row["source_overlays"], "complete nested source curves changed")
        if row["native_pair"]:
            result["original_banks"] = {k: v["hex"] for k, v in row["native_banks"].items() if k not in ("4", "60")}
        return result

    def export(self):
        rows = self.evidence["materials"]
        require(len(rows) == 164 and sum(x["native_pair"] for x in rows) == 114, "complete authored source cohort changed")
        cases = [self.fixture(row, i, "paired-original-main" if row["native_pair"] else "authored-mcc-owner")
                 for i, row in enumerate(rows)]
        seen = {x["native_payload_name"] for x in cases}
        for record in self.profile["native_records"]:
            if record["payload"] in seen:
                continue
            self.forced_native = record["map"]
            cases.append(self.fixture(rows[0], len(cases), "native-template-variant"))
            seen.add(cases[-1]["native_payload_name"])
        self.forced_native = None
        require(len(seen) == 39, "complete native template variants not exercised")
        pins = dict(self.pins, **self.shared_pins)
        require(all(digest(Path(path)) == pin for path, pin in pins.items()), "a source input changed during fixture export")
        return dict(schema="H2_MCC_NATIVE_ENV_ILLUM_THREE_CHANNEL_FIXTURE_V1", cases=cases,
            scope=__doc__, source_map_sha256=pins, authored_rows=164, paired_original_rows=114,
            native_template_variants=39, native_allocations_explicitly_translated=True,
            native_projection_admitted=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--prepared-root", type=Path, required=True)
    parser.add_argument("--authority", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    require(args.output.resolve().is_relative_to(owner), "literal fixture escaped its enclosing owner")
    root, prepared = args.repo_root.resolve(strict=True), args.prepared_root.resolve(strict=True)
    profile = json.loads((root / "tools/data/mcc-native-env-illum-three-channel-profiles.json").read_bytes())
    packed = args.authority.read_bytes()
    require(sha(packed) == profile["authority"]["sha256"], "complete paired authority changed")
    exporter = EnvironmentExporter(root, prepared, json.loads(lzma.decompress(packed)), profile)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    raw = (json.dumps(fixture, indent=2) + "\n").encode()
    require(len(raw) <= 32 * 1024 * 1024, "literal metadata fixture exceeds owned input budget")
    args.output.write_bytes(raw)
    print(json.dumps(dict(cases=len(fixture["cases"]), authored_rows=164, paired_original_rows=114,
                         native_template_variants=39, literal_fixture_bytes=len(raw))))


if __name__ == "__main__":
    main()
