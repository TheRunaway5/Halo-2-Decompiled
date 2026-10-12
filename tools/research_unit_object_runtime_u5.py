#!/usr/bin/env python3
"""Check complete object construction bodies against the owned TU1.5 image.

Uses the existing normalized instruction/internal branch checker and verifies
the actual selected jump tables. This is static correspondence, not TU1.5
construction, resource, world or gameplay execution.
"""
import argparse
import json
from pathlib import Path
import research_unit_weapon_routing_u5 as checker
from generate_unit_object_runtime_1_1 import ENTRIES

checker.MANIFEST = checker.ROOT/'tests/fixtures/unit_object_runtime_manifest.json'
checker.TABLES = {0x114ee4: 3, 0x1e8f2c: 6, 0x1e8f44: 6}

if __name__ == '__main__':
    checker.main()
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = json.loads(args.output.read_text())
    report.update(driver_sha256=checker.sha(Path(__file__)),
                  construction_entries=len(set(ENTRIES)),
                  additional_normalization_lookup_entries=5,
                  scope=__doc__)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps(dict(final_report_sha256=checker.sha(args.output))))
