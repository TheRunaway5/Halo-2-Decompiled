#!/usr/bin/env python3
"""Stage eight literal clock-only MCC owners and original paired controls.

Data admission component inputs only. Native templates use translated owned
test allocations; this exporter does not modify caches or claim GPU draws.
"""
import argparse
import hashlib
import json
import lzma
import os
from pathlib import Path
import struct

from export_mcc_native_world_cooked_fixture import Exporter, NATIVE, require, sha, u32

NATIVE.update(plasma_alpha="dune", two_alpha_clouds="dune", water_static="zanzibar")
FAMILIES = {"plasma_alpha", "two_alpha_clouds", "water_static"}


class AnimatedExporter(Exporter):
    def fixture(self, row, ordinal, kind):
        result = super().fixture(row, ordinal, kind)
        cache = self.select(row["map"])
        tag = cache.tags[row["datum"]]
        pp = cache.block(tag["offset"] + 32, 124)[0]
        overlays = cache.block(pp + 60, 20)
        regions = {address: bytes.fromhex(raw) for address, raw in result["regions"]}
        expected = bytearray.fromhex(result["expected_banks"]["60"])
        curves = []
        for i, at in enumerate(overlays):
            count, pointer = cache.u32(at + 12), cache.u32(at + 16)
            require(0 < count <= 4096, "finite literal clock curve extent changed")
            offset = cache.pointer(pointer, cache.magic, count)
            raw = bytes(cache.data[offset:offset + count])
            regions[pointer] = raw
            curves.append(dict(input=cache.u32(at), range=cache.u32(at + 4),
                               period_word=cache.u32(at + 8), curve_hex=raw.hex()))
            struct.pack_into("<I", expected, i * 20 + 16, 0)
        result["regions"] = [[address, raw.hex()] for address, raw in sorted(regions.items())]
        result["expected_banks"]["60"] = expected.hex()
        # These native implementation roles are nonzero and exactly paired.
        result["expected_banks"]["52"] = row["source_banks"]["52"]["hex"]
        native = bytes.fromhex(result["native_payload_hex"])
        template_pp = u32(native, 92)
        count, at = u32(native, template_pp), u32(native, template_pp + 4)
        result["expected_banks"]["28"] = b"".join(
            struct.pack("<IH", u32(native, at + i * 10 + 2),
                        struct.unpack_from("<H", native, at + i * 10)[0]) for i in range(count)).hex()
        result["expected_overlays"] = curves
        return result

    def export(self, water_evidence):
        wanted = [row for row in self.evidence["materials"] if row["map"] == "needle" and
                  row["template"].rsplit("\\", 1)[-1] in FAMILIES]
        require(len(wanted) == 7, "complete preceding animated owner census changed")
        cases = [self.fixture(row, i, "pc-geometry-owner") for i, row in enumerate(wanted)]
        controls = (("lockout", "scenarios\\shaders\\multi\\lockout\\grav_lift"),
                    ("coagulation", "scenarios\\shaders\\multi\\lockout\\grav_lift"),
                    ("dune", "scenarios\\shaders\\multi\\dune\\dune_waves"))
        for name, shader in controls:
            row = next(row for row in self.evidence["materials"] if row["map"] == name and row["shader"] == shader)
            require(row["native_pair"] and row["normalized_images_exact"] and
                    all(row["source_banks"][str(field)]["hex"] == row["native_banks"][str(field)]["hex"]
                        for field in (12, 20, 52, 68, 76, 84, 92)),
                    "complete original paired source roles or main constants changed")
            cases.append(self.fixture(row, len(cases), "paired-original-main"))
        # The broad clock census predates the owned V8 Zanzibar bitmap append.
        # Keep that source context intact; import water cases through their
        # separately hash-pinned current-map proof instead of replacing a SHA.
        water = AnimatedExporter(self.root, self.prepared, water_evidence)
        try:
            required = [(row, "pc-geometry-owner" if row["map"] == "needle" else "paired-original-main")
                        for row in water_evidence["materials"]]
            require(len(required) == 2 and {row["map"] for row, _ in required} == {"needle", "zanzibar"}
                    and all(row["template"] == "shaders\\shader_templates\\water\\water_static"
                            for row, _ in required), "complete finite water owner census changed")
            for row, kind in required:
                if kind == "paired-original-main":
                    require(row["native_pair"] and row["normalized_images_exact"] and
                            all(row["source_banks"][str(field)]["hex"] == row["native_banks"][str(field)]["hex"]
                                for field in (12, 20, 52, 68, 76, 84, 92)),
                            "complete paired water roles/main banks changed")
                cases.append(water.fixture(row, len(cases), kind))
            for name, digest in water.pins.items():
                require(name not in self.pins or self.pins[name] == digest,
                        "executed fixture input owners conflict")
                self.pins[name] = digest
        finally:
            water.close()
        return dict(schema="H2_MCC_NATIVE_WORLD_ANIMATED_FIXTURE_V1",
                    scope="Literal clock-only data admission component; translated native test allocations; no live/GPU draw claim",
                    map_sha256=self.pins, cases=cases, expected_pc_geometry_owners=8,
                    paired_exclusions=[], source_clock_authority="Original Lockout grav_lift, Dune dune_waves and Zanzibar water_static roles; exact authored MCC water periods/curve variants retained")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--prepared-root", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--water-evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report-output", type=Path)
    args = parser.parse_args()
    root, prepared = args.repo_root.resolve(strict=True), args.prepared_root.resolve(strict=True)
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = args.output.resolve()
    require(output.is_relative_to(owner), "literal fixture must stay inside owned scratch")
    evidence = json.loads(lzma.decompress(args.evidence.read_bytes()))
    policy = json.loads((root / "tools/data/mcc-native-world-animated-profiles.json").read_bytes())
    water_raw = args.water_evidence.read_bytes()
    require(sha(water_raw) == policy["water_source_report"]["sha256"],
            "complete water source proof differs from the qualified profile")
    water = json.loads(lzma.decompress(water_raw))
    exporter = AnimatedExporter(root, prepared, evidence)
    try:
        fixture = exporter.export(water)
    finally:
        exporter.close()
    raw = (json.dumps(fixture, indent=2) + "\n").encode()
    output.write_bytes(raw)
    report = dict(scope=fixture["scope"], fixture_bytes=len(raw), fixture_sha256=sha(raw),
                  pc_geometry_owners=8, paired_original_fixtures=4, source_map_sha256=fixture["map_sha256"])
    if args.report_output:
        args.report_output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
