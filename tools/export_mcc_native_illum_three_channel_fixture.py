#!/usr/bin/env python3
"""Export the literal Needle light plus all nine paired originals to owned scratch."""
import argparse
import json
import lzma
import os
from pathlib import Path

from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import NATIVE, require

NATIVE["tex_bump_illum_3_channel"] = "04a_gasgiant"


class LightExporter(AnimatedExporter):
    def fixture(self, row, ordinal, kind):
        result = super().fixture(row, ordinal, kind)
        reference = next(x for x in self.evidence["materials"] if x["map"] == "04a_gasgiant" and
                         x["shader"] == row["shader"] and x["native_pair"])
        for field in (28, 52, 84):
            result["expected_banks"][str(field)] = reference["native_banks"][str(field)]["hex"]
        for field, stride, slots in ((4, 12, (0, 1, 2, 3, 5)), (12, 4, (0, 1, 2, 3, 4, 6, 5, 7)), (20, 16, (0, 1, 2, 3, 4, 6, 7, 5))):
            raw = bytes.fromhex(row["source_banks"][str(field)]["hex"])
            result["expected_banks"][str(field)] = b"".join(raw[i * stride:(i + 1) * stride] for i in slots).hex()
        return result

    def export(self):
        rows = [x for x in self.evidence["materials"] if x["template"].endswith("\\tex_bump_illum_3_channel") and
                x["shader"] == "scenarios\\shaders\\forerunner\\industrial\\lights\\light_05_white"]
        require(len(rows) == 10 and sum(x["native_pair"] for x in rows) == 9 and
                [x["map"] for x in rows if not x["native_pair"]] == ["needle"], "complete one-PC/nine-pair source census differs")
        return dict(schema="H2_MCC_NATIVE_ILLUM_THREE_CHANNEL_FIXTURE_V1",
            scope="Literal paired shared light setup; translated native test allocations; no live/GPU draw claim",
            cases=[self.fixture(row, i, "paired-original-main" if row["native_pair"] else "pc-geometry-owner") for i, row in enumerate(rows)],
            map_sha256=self.pins, paired_exclusions=[], expected_pc_geometry_owners=1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--prepared-root", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    root, prepared = args.repo_root.resolve(strict=True), args.prepared_root.resolve(strict=True)
    owner = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = args.output.resolve()
    require(output.is_relative_to(owner), "raw light fixture escaped the owned scratch")
    evidence = json.loads(lzma.decompress(args.evidence.read_bytes()))
    exporter = LightExporter(root, prepared, evidence)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    output.write_text(json.dumps(fixture, indent=2) + "\n")
    print("One actual Needle owner and nine complete original paired controls exported")


if __name__ == "__main__":
    main()
