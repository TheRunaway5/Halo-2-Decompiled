#!/usr/bin/env python3
"""Capture an explicit Triplicate train inspection, with immutable artifact provenance.

The camera isolates authored train placement0 outside normal world visibility.
It is an inspection camera, not a captured original gameplay camera.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import subprocess

from PIL import Image


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    binary, output = args.binary.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    map_path = root/'assets/map-pack/maps/triplicate.map'
    shader_paths = sorted((binary.parent/'shaders').glob('*.spv'))
    if not shader_paths:
        raise RuntimeError('inspection requires the recorded shader snapshot')
    hashes = {str(path): digest(path) for path in [binary, map_path, *shader_paths]}
    environment = os.environ.copy()
    shader_from = environment.get('HALO2_CAPTURE_SHADER_FROM')
    shader_to = environment.get('HALO2_CAPTURE_SHADER_TO')
    if not shader_from or not shader_to or Path(shader_to).resolve() != binary.parent/'shaders':
        raise RuntimeError('inspection requires explicit routing to the recorded shader snapshot')
    libraries = environment.get('LD_PRELOAD', '').split(':')
    if not any(libraries):
        raise RuntimeError('inspection requires the capture-only shader file routing library')
    routing = {'from': shader_from, 'to': shader_to,
               'preload_libraries': {name: digest(Path(name)) for name in libraries if name}}
    eye = (-63.5, -12.0, 3.0)
    forward = (0.0, .9928768384869, -.1191452206184)
    up = (0.0, 0.0, 1.0)
    if not all(math.isfinite(value) for value in (*eye, *forward, *up)):
        raise RuntimeError('invalid inspection camera')
    capture = output/'train.ppm'
    command = [str(binary), '--multiplayer', '--map', str(map_path),
               '--mp-starting-weapon', '5', '--mp-reference-camera',
               *[format(value, '.12g') for value in (*eye, *forward, *up)],
               '--local-players', '1', '--viewport', '1280', '720', '--hidden',
               '--mute', '--frames', '3', '--validation', '--screenshot', str(capture)]
    with (output/'native.log').open('w') as log:
        subprocess.run(command, cwd=root, env=environment, stdout=log,
                       stderr=subprocess.STDOUT, timeout=120, check=True)
    diagnostics = [line for line in (output/'native.log').read_text().splitlines()
                   if any(marker in line for marker in ['Vulkan validation:',
                          'fallback:', 'error:', 'Scenery:', 'Machine:', 'Prop:',
                          'lighting unresolved:'])]
    if diagnostics:
        raise RuntimeError(diagnostics)
    with Image.open(capture) as frame:
        if frame.size != (1280, 720):
            raise RuntimeError('unexpected inspection extent')
        frame.save(output/'train.png')
    unchanged = all(digest(Path(name)) == value for name, value in hashes.items())
    hashes[str(capture)] = digest(capture)
    report = {'command': command, 'camera': {'eye': eye, 'forward': forward, 'up': up},
              'native_frames': 3, 'original_frame_comparison_performed': False,
              'boundary': 'Explicit native inspection camera, outside normal world visibility. Original camera/controller phase, complete simulation history and framebuffer equality are not established.',
              'sha256': hashes, 'artifacts_unchanged': unchanged,
              'validation_diagnostics': diagnostics,
              'capture_only_shader_file_routing': routing}
    (output/'provenance.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'capture': str(output/'train.png'), 'artifacts_unchanged': unchanged}))
    return 0 if unchanged else 1


if __name__ == '__main__':
    raise SystemExit(main())
