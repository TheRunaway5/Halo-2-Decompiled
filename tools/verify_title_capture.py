#!/usr/bin/env python3
"""Check isolated original Vulkan title stages using exported pre/post surfaces.

These arithmetic/coverage checks do not establish complete native-frame parity.
The GPU tests separately exercise the production combiners on capture samples.
"""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--capture', type=Path, default=ROOT/'analysis/frontend/xemu-capture')
args = parser.parse_args()
p = args.capture


def pixels(event):
    return np.asarray(Image.open(p/f'pass-{event}.png'))[:, :, :3]


def sample(image, coordinates):
    height, width, _ = image.shape
    point = coordinates*np.array([width, height])-.5
    first = np.floor(point).astype(int)
    fraction = point-first
    x, y = np.clip(first[:, 0], 0, width-1), np.clip(first[:, 1], 0, height-1)
    xx = np.clip(first[:, 0]+1, 0, width-1)
    yy = np.clip(first[:, 1]+1, 0, height-1)
    fx, fy = fraction[:, 0, None], fraction[:, 1, None]
    return ((image[y, x]*(1-fx)+image[y, xx]*fx)*(1-fy) +
            (image[yy, x]*(1-fx)+image[yy, xx]*fx)*fy)


def compare(actual, expected):
    byte = np.clip(np.rint(actual*255), 0, 255).astype(np.uint8)
    error = np.abs(byte.astype(int)-expected.astype(int))
    return {'pixels': int(error.size//3), 'maximum_byte_error': int(error.max()),
            'channels_above_one_byte': int(np.count_nonzero(error > 1)),
            'mean_byte_error': float(error.mean())}


results = {}
# Original 26520: ARGB 41201B16 and a maximum-W quad with LEQUAL.
depth = np.frombuffer((p/'eye-fog-depth.bin').read_bytes()[:640*480*4], dtype='<u4').reshape(480, 640)
before, after = pixels(1865), pixels(1872)
mask = depth == 0xffffffff
changed = np.any(before != after, axis=2)
prediction = before.astype(float)/255
prediction[mask] = np.array([32, 27, 22])/255 + prediction[mask]*(1-65/255)
results['eye_fog'] = {**compare(prediction, after), 'background_pixels': int(mask.sum()),
                      'coverage_mismatches': int(np.count_nonzero(mask != changed))}

# 214F0 threshold and four 211A0 filters at the original 160x120 size.
# The threshold constant is 77/255; NV output scaling is four. Each blur
# averages four taps. Every source here is the actual preceding GPU surface.
for before, after, radius in [(8034, 8048, .5), (8048, 8062, 1),
                               (8062, 8076, 1.25), (8076, 8090, 1.5625),
                               (8090, 8104, 1.937500238418579)]:
    source, expected = pixels(before)/255, pixels(after)
    height, width, _ = expected.shape
    x, y = np.meshgrid(np.arange(width)+.5, np.arange(height)+.5)
    uv = np.column_stack([x.ravel()/width, y.ravel()/height])
    taps = [sample(source, uv+np.array([dx, dy])*radius*.5/np.array([width, height]))
            for dx, dy in [(-1, -1), (1, -1), (-1, 1), (1, 1)]]
    prediction = sum(taps)/4
    if after == 8048:
        prediction = np.clip((prediction-77/255)*4, 0, 1)
    results[str(after)] = {**compare(prediction.reshape(expected.shape), expected), 'radius': radius}

# 20F30: source constant 96/255, NV output scaling two, INV_DST_COLOR/ONE.
destination, expected = pixels(8034)/255, pixels(8118)
height, width, _ = expected.shape
x, y = np.meshgrid(np.arange(width)+.5, np.arange(height)+.5)
uv = np.column_stack([x.ravel()/width, y.ravel()/height])
bloom = sample(pixels(8104)/255, uv).reshape(expected.shape)
results['bloom_composite'] = compare(destination+bloom*(192/255)*(1-destination), expected)

# Authored egor effects\screen_effects\desaturate, after bloom and before UI.
source = pixels(8118)/255
brightness = np.clip(source @ (np.array([128, 179, 51])/255), 0, 1)
results['title_color_filter'] = compare(brightness[:, :, None]*(np.array([26, 35, 64])/255), pixels(8132))
failed = any(v['channels_above_one_byte'] for v in results.values()) or results['eye_fog']['coverage_mismatches']
report = {'capture_sha256': hashlib.sha256((p/'title-vulkan_capture.rdc').read_bytes()).hexdigest(),
          'reference': 'Xemu 0.8.136 Vulkan, 1x, 640x480', 'results': results,
          'passed': not failed, 'complete_native_frame_parity_verified': False}
(p/'original-stage-pixel-check.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
raise SystemExit(bool(failed))
