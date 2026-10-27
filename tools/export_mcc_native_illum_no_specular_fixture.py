#!/usr/bin/env python3
"""Export literal District/Turf illumination inputs into owned test scratch."""
import argparse
import json
import lzma
import os
from pathlib import Path

from export_mcc_native_world_animated_fixture import AnimatedExporter
from export_mcc_native_world_cooked_fixture import NATIVE, require

NATIVE["tex_bump_illum_no_specular"] = "turf"


class IllumExporter(AnimatedExporter):
    def fixture(self, row, ordinal, kind):
        result = super().fixture(row, ordinal, kind)
        reference = next(x for x in self.evidence["materials"] if x["map"] == "turf" and
                         x["template"] == row["template"] and x["native_pair"])
        for field in (28, 52, 84):
            result["expected_banks"][str(field)] = reference["native_banks"][str(field)]["hex"]
        result["expected_banks"]["12"] = row["source_banks"]["12"]["hex"][:8]
        return result

    def export(self):
        rows = [x for x in self.evidence["materials"] if x["map"] in ("street_sweeper", "turf") and
                x["template"].endswith("\\tex_bump_illum_no_specular")]
        require(len(rows) == 3 and sum(x["map"] == "street_sweeper" for x in rows) == 2,
                "complete District/Turf source owner census changed")
        cases = [self.fixture(row, i, "paired-original-main" if row["native_pair"] else "pc-geometry-owner")
                 for i, row in enumerate(rows)]
        return dict(schema="H2_MCC_NATIVE_ILLUM_NO_SPECULAR_FIXTURE_V1",
                    scope="Literal single-profile clock-only illumination data admission; translated native test allocations; no live/GPU draw claim",
                    cases=cases, map_sha256=self.pins, paired_exclusions=[], expected_pc_geometry_owners=2)


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
    require(output.is_relative_to(owner), "raw illumination fixture must stay inside owned scratch")
    evidence = json.loads(lzma.decompress(args.evidence.read_bytes()))
    exporter = IllumExporter(root, prepared, evidence)
    try:
        fixture = exporter.export()
    finally:
        exporter.close()
    output.write_text(json.dumps(fixture, indent=2) + "\n")
    print("Two actual District owners and one actual original Turf paired control exported")


if __name__ == "__main__":
    main()
