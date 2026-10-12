#!/usr/bin/env python3
"""Verify the complete original sound chunk-size owner against authenticated u5."""
import argparse
import json
from pathlib import Path
import research_animation_selection_u5 as matcher
from oracle_unit_animation_manager_1_1 import sha, save, encoded


def run(output):
    matcher.OWNERS = (0x21abd0,)
    matcher.BOUNDARIES = ()
    matcher.run(output / 'raw')
    raw = output / 'raw/report.json'
    result = json.loads(raw.read_text())
    save(output / 'report.json', encoded(dict(
        schema='original-sound-chunk-size-u5-correspondence-v1',
        tool_sha256=sha(__file__), matcher_sha256=sha(matcher.__file__),
        raw_report_sha256=sha(raw), image_sha256=result['image_sha256'],
        owners=result['owners'],
        scope='Complete static body, internal branch and relocated original chunk-bank operand correspondence only. Original21ABD0 has separately frozen1.1 execution evidence. No new1.5 scheduler, voice, file-registry or hardware audio execution is asserted.')))
    print(json.dumps(dict(report_sha256=sha(output / 'report.json'))))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', required=True, type=Path)
    run(p.parse_args().output)
