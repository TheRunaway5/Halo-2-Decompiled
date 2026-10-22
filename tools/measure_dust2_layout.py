#!/usr/bin/env python3
"""Make a measured Dust II layout guide for the clean District rebuild.

Navigation polygons are measurement references, not finished render geometry.
Run through tools/run_test.py; source units are inches, Halo units inches/120.
"""
import argparse
import collections
import json
import os
from pathlib import Path
import xml.sax.saxutils as xml

import numpy as np
from build_dust2_mombasa import sha


def measure(layout, entities, output):
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert output.resolve().is_relative_to(scratch)
    output.mkdir(exist_ok=True)
    data = json.loads(layout.read_text())
    areas = data['areas']
    points = np.concatenate([np.array(a['corners'])/120 for a in areas])
    low, high = points.min(axis=0), points.max(axis=0)
    width, height = 1400, 1300
    padding = 70
    scale = min((width-2*padding)/(high[0]-low[0]), (height-2*padding)/(high[1]-low[1]))

    def screen(p):
        return padding+(p[0]-low[0])*scale, height-padding-(p[1]-low[1])*scale

    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#0c1620"/>',
        '<g fill="#b9cad4" stroke="#304351" stroke-width=".35">']
    rows = []
    for area in sorted(areas, key=lambda a: np.mean(np.array(a['corners'])[:, 2])):
        p = np.array(area['corners'])/120
        flat = [screen(v) for v in p]
        z = float(p[:, 2].mean())
        value = int(130+90*(z-low[2])/max(.1, high[2]-low[2]))
        color = f'#{value-25:02x}{value:02x}{min(value+15,255):02x}'
        coords = ' '.join(f'{x:.2f},{y:.2f}' for x, y in flat)
        svg.append(f'<polygon points="{coords}" fill="{color}"><title>Area {area["id"]}; elevation {z:.3f} WU</title></polygon>')
        rows.append(dict(id=area['id'], corners_halo_world=p.tolist(), elevation=z,
                         connections=area['connections']))
    svg.append('</g>')
    spawns = collections.defaultdict(list)
    for entity in json.loads(entities.read_text()):
        kind = entity['classname']
        if kind not in ('info_player_counterterrorist', 'info_player_terrorist'):
            continue
        p = entity['halo_world_position']
        spawns[kind].append(p)
        x, y = screen(p)
        color = '#48bde8' if kind == 'info_player_counterterrorist' else '#e7a26c'
        svg.append(f'<circle cx="{x:.2f}" cy="{y:.2f}" r="3" fill="{color}"/>')
    for name, locations in spawns.items():
        center = np.mean(locations, axis=0);x, y = screen(center)
        label = 'CT spawn' if name == 'info_player_counterterrorist' else 'T spawn'
        svg.append(f'<text x="{x+10:.2f}" y="{y-10:.2f}" fill="white" font-family="sans-serif" font-size="18">{label}</text>')
    svg.extend(['<text x="55" y="34" fill="white" font-family="sans-serif" font-size="23">Dust II — measured route reference</text>',
        '<text x="55" y="57" fill="#8ba5b5" font-family="sans-serif" font-size="14">Navigation clearance areas; compare to source collision before authoring final boundaries.</text>',
        '</svg>'])
    (output/'route-reference.svg').write_text('\n'.join(svg)+'\n')
    report = dict(schema='H2_DUST2_REBUILD_LAYOUT_GUIDE_V1', source_sha256=sha(layout),
        source_units='inches', halo_units='source inches / 120', bounds= [low.tolist(), high.tolist()],
        areas=rows, spawns=dict(spawns), geometry_reused=False,
        scope='measurement guide; navigation clearance is not a wall or floor boundary')
    (output/'measured-layout.json').write_text(json.dumps(report, indent=2)+'\n')
    print(dict(areas=len(rows),bounds=report['bounds'],spawns={k:len(v) for k,v in spawns.items()}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('layout', 'entities', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    measure(**vars(parser.parse_args()))
