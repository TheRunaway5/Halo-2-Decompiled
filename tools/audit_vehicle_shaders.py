#!/usr/bin/env python3
"""Inventory every original Xbox vehicle model, shader LOD and dependency.

Campaign and multiplayer caches are read independently of native preparation.
The assets are used with the unchanged OG Xbox TU1.5 gameplay executable; this
census alone does not prove vehicle appearance, effects, poses or live lighting.
"""
import argparse
import json
from pathlib import Path
from audit_campaign import MAPS as CAMPAIGN
from audit_dynamic_model_shader_assets import MAPS as MULTIPLAYER
from audit_campaign_character_shaders import inventory
from audit_rendering import file_sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--maps', type=Path, default=ROOT/'assets/disc/maps')
    p.add_argument('--map-pack', type=Path, default=ROOT/'assets/map-pack/maps')
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=False)
    rows = []
    for name in (*CAMPAIGN, *MULTIPLAYER):
        directory = a.map_pack if name in MULTIPLAYER[12:] else a.maps
        report = inventory(directory/(name+'.map'), vehicles_only=True)
        report['cache_path'] = str((directory/(name+'.map')).resolve())
        destination = a.output/(name+'.json')
        destination.write_text(json.dumps(report, indent=2)+'\n')
        row = dict(map=name, cache_sha256=report['cache_sha256'], report_sha256=file_sha(destination),
                   **{k:len(report[k]) for k in ('units','models','shaders','passes','vertex_shaders')})
        rows.append(row)
        print(json.dumps(row), flush=True)
    (a.output/'manifest.json').write_text(json.dumps(dict(scope=__doc__, maps=rows,
        tool_sha256=file_sha(Path(__file__)), dependency_tool_sha256=file_sha(Path(__file__).with_name('audit_campaign_character_shaders.py')),
        full_visual_parity_verified=False), indent=2)+'\n')


if __name__ == '__main__':
    main()
