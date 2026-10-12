#!/usr/bin/env python3
"""Check haze spans in actual native inputs against original captured geometry.

Slice phase/rotation/offsets require matched camera and RNG history. The span
ratio is independent of those values and exposes the viewport aspect directly.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / 'analysis/frontend/xemu-capture'
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native-uniform', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
geometry = json.loads((CAPTURE / 'title-haze-geometry.json').read_text())
original = geometry['passes'][0]['postvs']['1']['float_rows']
uniform = args.native_uniform.read_bytes()
assert len(uniform) == 400
native = struct.unpack('<100f', uniform)
vertical = native[92]
checks = []
for layer in range(4):
    offset = 28 + layer*4
    a, b, _, d = [v[offset:offset+2] for v in original]
    x_span, y_span = math.dist(a, d), math.dist(a, b)
    r, s = native[60+layer*8:64+layer*8], native[64+layer*8:68+layer*8]
    native_x = 2*math.hypot(r[0], s[0])
    native_y = 2*abs(vertical)*math.hypot(r[1], s[1])
    actual, reference = native_y/native_x, y_span/x_span
    checks.append({'layer':layer, 'original_span_ratio':reference,
                   'native_span_ratio':actual, 'absolute_error':abs(actual-reference)})
report = {'original_capture_sha256':'077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431',
          'native_uniform':str(args.native_uniform.resolve()),
          'native_uniform_sha256':hashlib.sha256(uniform).hexdigest(),
          'original_viewport':[640,480], 'height_over_width':.75,
          'native_vertical_factor':vertical, 'checks':checks,
          'passed':all(c['absolute_error'] <= 2e-6 for c in checks),
          'scope':'actual captured span ratios; slice phase, rotation, offsets, RNG history and whole-frame parity are not compared'}
args.output.write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
raise SystemExit(not report['passed'])
