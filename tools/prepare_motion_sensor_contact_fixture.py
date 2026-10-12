#!/usr/bin/env python3
"""Normalize the retained original-game OpenGL radar capture for the CPU differential.

Input is inspect_motion_sensor_capture.py output, ending at sweep/mask draws.
The plan uses recorded quad geometry and effective vertex colors, independent
of native contact production or its clock. Original screen-space Y maps directly
to OpenGL readback row Y in xemu's passthrough vertex shader. PNG export flips
these rows for display; the native intermediate uses the original source rows.
"""
import json
from pathlib import Path
import struct
import sys


directory = Path(sys.argv[1])
capture = json.loads((directory / 'capture.json').read_text())
draws = capture['draws']
assert len(draws) >= 3
contacts, sweep, mask = draws[:-2], draws[-2], draws[-1]


def vertices(draw, binding):
    raw = (directory / f"{draw['event']}-vertex-buffer-{binding}.bin").read_bytes()
    return [struct.unpack_from('<4f', raw, offset) for offset in range(0, 64, 16)]


def color(draw):
    attribute = next(a for a in draw['vertex_inputs'] if a['name'] == 'v5')
    assert attribute['genericEnabled']
    return attribute['genericValue']['floatValue'][:3]


uv = vertices(sweep, 1)
pulse = struct.unpack('<f', struct.pack('<f', uv[0][0] - uv[1][0]))[0]
lines = [' '.join(map(str, [pulse, *color(sweep), len(contacts)]))]
for draw in contacts:
    assert draw['count'] == 4
    v = vertices(draw, 0)
    assert v[0][0] == v[3][0] and v[1][0] == v[2][0]
    assert v[0][1] == v[1][1] and v[2][1] == v[3][1]
    u = vertices(draw, 1)
    assert [x[:2] for x in u] == [(0, 0), (1, 0), (1, 1), (0, 1)]
    x, y = (v[0][0] + v[1][0]) / 2, (v[0][1] + v[2][1]) / 2
    intensity = (v[1][0] - v[0][0]) / 4
    assert abs((v[0][1] - v[2][1]) / 4 - intensity) < 0.00001
    lines.append(' '.join(map(str, [x, y, 1, intensity, *color(draw), 0])))
(directory / 'original-plan.tsv').write_text('\n'.join(lines) + '\n')
raw = (directory / f"{mask['event']}-after.bin").read_bytes()
assert len(raw) == 64 * 64 * 4
(directory / 'original-target.rgba').write_bytes(raw)
print(f"Prepared {len(contacts)} original contact draws, pulse {pulse}")
