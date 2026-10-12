#!/usr/bin/env python3
"""Retained Zanzibar camera/player comparison with explicit provenance limits."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--validation', action='store_true')
    parser.add_argument('--fixture',type=Path,help='Explicit captured player/view fixture')
    parser.add_argument('--original',type=Path,help='Explicit actual original image')
    parser.add_argument('--render-fraction',type=float,help='Explicit native diagnostic interpolation fraction')
    parser.add_argument('--gameplay-xbe',type=Path,help='Explicit original executable for runtime data')
    parser.add_argument('--capture-tick',type=int,
                        help='Actual native clock limit; supplies an empty explicit action script')
    args = parser.parse_args()
    binary, output = args.binary.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    map_path = root/'assets/disc/maps/zanzibar.map'
    fixture = (args.fixture or root/'tests/fixtures/retail_zanzibar_player.reference').resolve()
    original = (args.original or root/'analysis/multiplayer-shading/original-xemu/original_capture_20.png').resolve()
    files = [binary, map_path, fixture, original,
             *sorted((binary.parent/'shaders').glob('*.spv'))]
    hashes = {str(path): digest(path) for path in files}
    if args.gameplay_xbe:hashes[str(args.gameplay_xbe.resolve())]=digest(args.gameplay_xbe)
    environment = os.environ.copy()
    routing = None
    if environment.get('HALO2_CAPTURE_SHADER_FROM') or environment.get('HALO2_CAPTURE_SHADER_TO'):
        target = environment.get('HALO2_CAPTURE_SHADER_TO')
        if not environment.get('HALO2_CAPTURE_SHADER_FROM') or not target or Path(target).resolve() != binary.parent/'shaders':
            raise RuntimeError('shader file routing does not target the recorded snapshot')
        routing = {'from': environment['HALO2_CAPTURE_SHADER_FROM'], 'to': target,
                   'preload_libraries': {name: digest(Path(name)) for name in
                                         environment.get('LD_PRELOAD', '').split(':') if name}}
    capture = output/'native.ppm'
    command = [str(binary), '--multiplayer', '--map', str(map_path),
               '--mp-starting-weapon', '5', '--mp-reference-player', str(fixture),
               '--local-players', '1', '--viewport', '640', '480', '--hidden',
               '--mute', '--screenshot', str(capture)]
    if args.render_fraction is not None:
        if args.capture_tick is None or not 0 <= args.render_fraction <= 1:
            raise ValueError('explicit interpolation requires a finite tick capture and fraction0..1')
        command.extend(['--mp-reference-render-fraction',str(args.render_fraction)])
    if args.gameplay_xbe:command.extend(['--gameplay-xbe',str(args.gameplay_xbe.resolve())])
    if args.capture_tick is None:
        command.extend(['--frames','3'])
    else:
        if not 0 < args.capture_tick <= 1000000:
            raise ValueError('capture tick outside native finite clock range')
        idle_script=output/'native-idle-input.txt'
        idle_script.write_text('# No action records: actual native idle controls only\n')
        hashes[str(idle_script)]=digest(idle_script)
        command.extend(['--mp-input-script',str(idle_script),'--mp-capture-tick',str(args.capture_tick)])
    if args.validation:
        command.append('--validation')
    with (output/'native.log').open('w') as log:
        subprocess.run(command, cwd=root, env=environment, stdout=log,
                       stderr=subprocess.STDOUT, timeout=120, check=True)
    diagnostics = [line for line in (output/'native.log').read_text().splitlines()
                   if any(marker in line for marker in ['Vulkan validation:',
                          'fallback:', 'error:', 'Scenery:', 'Machine:', 'Prop:',
                          'lighting unresolved:'])]
    if diagnostics:
        raise RuntimeError(diagnostics)
    with Image.open(capture) as image:
        native = image.convert('RGB')
    with Image.open(original) as image:
        reference = image.convert('RGB')
    if native.size != (640, 480) or reference.size != native.size:
        raise RuntimeError('unexpected retained comparison extent')
    native.save(output/'native.png')
    comparison = Image.new('RGB', (1280, 510), '#151515')
    comparison.paste(reference, (0, 30))
    comparison.paste(native, (640, 30))
    labels = ImageDraw.Draw(comparison)
    labels.text((10, 8), 'Actual original: '+original.name, fill='white')
    labels.text((650, 8), 'Native: explicit retained camera/player/device inputs', fill='white')
    comparison.save(output/'comparison.png')
    regions = {}
    for name, box in {'world': [152, 80, 472, 280],
                      'first_person': [240, 300, 470, 480]}.items():
        differences = np.abs(np.asarray(native.crop(box), dtype=np.int16) -
                             np.asarray(reference.crop(box), dtype=np.int16))
        regions[name] = {'box': box,
                         'mean_absolute_channel_difference': float(differences.mean()),
                         'channels_over_8': int(np.count_nonzero(differences > 8))}
    unchanged = all(digest(Path(name)) == value for name, value in hashes.items())
    hashes[str(capture)] = digest(capture)
    report = {'command': command, 'sha256': hashes, 'artifacts_unchanged': unchanged,
              'original_frame':20 if args.original is None else None,
              'original_image':str(original),'reference_fixture':str(fixture),'original_process_modified': False,
              'native_capture_tick':args.capture_tick,
              'native_render_fraction':args.render_fraction,
              'framebuffer_equality_established': False,
              'boundary': 'Retained camera/player/device fixture. Complete simulation, HUD, controller, visibility, shader clocks, sampling and startup history are not restored.',
              'diagnostic_image_regions': regions, 'validation_diagnostics': diagnostics,
              'capture_only_shader_file_routing': routing}
    (output/'provenance.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'artifacts_unchanged': unchanged, 'regions': regions}))
    return 0 if unchanged else 1


if __name__ == '__main__':
    raise SystemExit(main())
