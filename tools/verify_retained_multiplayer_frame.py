#!/usr/bin/env python3
"""Capture the finite Zanzibar reference fixture and retain image diagnostics.

This does not resume or modify the original process, restore full original
game state, or establish framebuffer equality. The two regions exclude known
HUD/time and uncaptured object-history differences.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
from PIL import Image, ImageChops, ImageDraw, ImageStat


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, default=Path('build-mp-shading/halo2_native'))
    parser.add_argument('--original', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--label', default='Native retained source fixture')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    binary = args.binary.resolve()
    original_path = args.original.resolve()
    map_path = root/'assets/disc/maps/zanzibar.map'
    fixture = root/'tests/fixtures/retail_zanzibar_player.reference'
    capture = output/'native.ppm'
    command = [str(binary), '--multiplayer', '--map', str(map_path),
               '--mp-starting-weapon', '5', '--mp-reference-player', str(fixture),
               '--local-players', '1', '--viewport', '640', '480', '--hidden',
               '--mute', '--frames', '3', '--validation', '--screenshot', str(capture)]
    environment = os.environ.copy()
    environment.setdefault('DISPLAY', ':0')
    with (output/'native.log').open('w') as log:
        subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                       env=environment, timeout=120, check=True)
    original = Image.open(original_path).convert('RGB')
    native = Image.open(capture).convert('RGB')
    if original.size != (640, 480) or native.size != original.size:
        raise RuntimeError('retained fixture requires matching 640x480 images')
    native.save(output/'native.png')
    comparison = Image.new('RGB', (640, 1000), '#151515')
    comparison.paste(original, (0, 20))
    comparison.paste(native, (0, 520))
    labels = ImageDraw.Draw(comparison)
    labels.text((8, 4), 'Original retained frame20', fill='white')
    labels.text((8, 504), args.label, fill='white')
    comparison.save(output/'comparison.png')
    regions = {}
    for name, box in [('world', (152, 80, 472, 280)),
                      ('first_person', (240, 300, 470, 480))]:
        difference = ImageChops.difference(original.crop(box), native.crop(box))
        histogram = difference.histogram()
        regions[name] = {'box': box,
                         'mean_absolute_channel_difference': sum(ImageStat.Stat(difference).mean)/3,
                         'channels_over_8': sum(histogram[channel*256+value]
                                                for channel in range(3) for value in range(9, 256))}
    files = [binary, map_path, fixture, original_path, capture,
             *sorted((binary.parent/'shaders').glob('*.spv'))]
    report = {'command': command,
              'sha256': {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in files},
              'original_frame': 20, 'original_process_modified': False,
              'framebuffer_equality_established': False,
              'boundary': 'Retained camera/player/device fixture; full simulation, shader clocks, '
                          'runtime admission order, GPU sampling and original startup history are not restored.',
              'diagnostic_image_regions': regions}
    (output/'provenance.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(regions, indent=2))


if __name__ == '__main__':
    main()
