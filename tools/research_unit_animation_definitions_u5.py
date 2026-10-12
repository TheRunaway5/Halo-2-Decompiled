#!/usr/bin/env python3
"""Reuse the independently byte-checked complete-body matcher for metadata owners.

Only 1CA5A0 is executed by the companion metadata oracle. The other owners
establish source layout/call correspondence, not successful resource or events.
"""
import argparse
import json
from pathlib import Path
import research_unit_animation_manager_u5 as matcher
from oracle_unit_animation_manager_1_1 import sha, save, encoded


def run(out):
    matcher.OWNERS = (0x1CA5A0, 0x1DE890, 0x1CAE40, 0x1E13C0)
    matcher.BOUNDARIES = ()
    matcher.run(out / 'raw')
    raw = out / 'raw/report.json'
    original = json.loads(raw.read_text())
    save(out / 'report.json', encoded(dict(
        schema='source-unit-animation-definition-u5-correspondence-v1',
        driver_sha256=sha(__file__), matcher_sha256=sha(matcher.__file__),
        raw_report_sha256=sha(raw), image_sha256=original['image_sha256'],
        owners=original['owners'],
        scope='Static unique full-body/internal-branch correspondence for graph resolution, definition notification and event traversal. The companion TU1.1 oracle dynamically executes only1CA5A0. No TU1.5 runtime, resource notification, codec, event/audio/effect success or pose is asserted.')))
    print(json.dumps(dict(report_sha256=sha(out / 'report.json'))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    run(p.parse_args().output)
