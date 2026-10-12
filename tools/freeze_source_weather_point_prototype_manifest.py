#!/usr/bin/env python3
"""Freeze the isolated weather prototype after its guarded CPU/GPU runs."""
import argparse
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/weather-point-prototype'


def digest(path):
    raw = path.read_bytes()
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def main():
    p = argparse.ArgumentParser()
    p.add_argument('normal_build', type=Path)
    p.add_argument('asan_build', type=Path)
    args = p.parse_args()
    builds = [args.normal_build.resolve(), args.asan_build.resolve()]
    previous = OUT / 'manifest-before-output-guards.json'
    current = OUT / 'manifest.json'
    if not previous.exists():
        previous.write_bytes(current.read_bytes())
    m = json.loads(previous.read_text())
    for name in ('cpu-guard-normal.log', 'cpu-guard-asan.log'):
        log = (OUT / name).read_text()
        assert 'CPU analytic952 cases/60928 words: 0 differ, maxULP0' in log
    graphics_cases = None
    for name in ('gpu-guard-normal.log', 'gpu-guard-asan.log'):
        log = (OUT / name).read_text()
        assert 'GPU analytic952 cases/60928 words: 376 differ, maxULP32' in log
        assert not re.search(r'Validation (?:Error|Warning)|runtime error:|AddressSanitizer', log)
        actual = [list(map(int, row)) for row in re.findall(r'(\d+)x(\d+): (\d+) changed pixels', log)]
        assert [row[:2] for row in actual] == [[640,480],[641,481],[1920,1080],[3840,2160]]
        if graphics_cases is None:
            graphics_cases = actual
        else:
            assert actual == graphics_cases
    m['graphics']['target_cases'] = graphics_cases
    m['version'] = 3
    m['graphics']['alpha_contract'] = 'Original method033C=0x206 GEQUAL, method0340=0; int(round(alpha*255)) >= 0. Alpha0 is neutral blending, not discard.'
    controls = m['graphics']['controls']
    controls[controls.index('alpha0')] = 'alpha0 neutral blending'
    controls[controls.index('authored diameter half')] = 'authored diameter doubled'
    controls.extend(['zero/negative point diameter', 'zero/negative/nonfinite program W',
                     'physical pointSizeRange bounds', 'full-surface viewport and scissor subset',
                     'valid prepare, rejected prepare/view, retained valid readback',
                     'missing original raw-row/upper-left sprite coordinates'])
    m['graphics']['point_coordinate_adapter'] = 'Explicit original GL raw-row upper-left source mode; preserve raw framebuffer/sprite rows, sample PointCoordY=1-VulkanPointCoordY.'
    m['admission'] = dict(before_binding_replacement=True,
                         output='Each original VS22 CPU output is checked for finite position, positive W/oPts and actual physical pointSizeRange.',
                         precision_limit='CPU admission does not establish GPU rounding at point-size range boundaries.',
                         record='Prepared integer full surface; viewport origin0, matching width/height, depth0..1; explicit subset scissor. Split viewport conversion is not admitted.')
    files = {}
    for name in m['files']:
        if name.startswith('build-source-weather-point-final-asan-20261009/'):
            path = builds[1] / Path(name).relative_to('build-source-weather-point-final-asan-20261009')
        elif name.startswith('build-source-weather-point-final-20261009/'):
            path = builds[0] / Path(name).relative_to('build-source-weather-point-final-20261009')
        elif name.endswith('-final.log'):
            path = ROOT / name.replace('-final.log', '-guard.log')
            # Logs are named by mode, e.g. gpu-guard-normal.log.
            path = OUT / Path(name).name.replace('-normal-final', '-guard-normal').replace('-asan-final', '-guard-asan')
        else:
            path = ROOT / name
        files[str(path.relative_to(ROOT))] = digest(path)
    for path in [Path(__file__).resolve(), OUT / 'build-guard-normal.log', OUT / 'build-guard-asan.log',
                 ROOT / 'shaders/vertex_program.glsl', ROOT / 'shaders/combiner.glsl', ROOT / 'shaders/particle_depth.glsl']:
        files[str(path.relative_to(ROOT))] = digest(path)
    m['files'] = files
    current.write_text(json.dumps(m, indent=2) + '\n')
    print(json.dumps(dict(manifest=str(current.relative_to(ROOT)), files=len(files), **digest(current))))


if __name__ == '__main__':
    main()
