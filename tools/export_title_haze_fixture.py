#!/usr/bin/env python3
"""Export original title haze GPU inputs, without estimating its RNG history.

Run the two RenderDoc extract-title-haze scripts first. Program operation words
come from unchanged original 26880/26E50 oracle output; constants and UVs come
from the Vulkan capture. This fixture isolates sampled shader passes, not the
native world's geometry, camera history, random initialization, or draw order.
"""
import hashlib
import json
import struct
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
p = ROOT / 'analysis/frontend/xemu-capture'
destination = p / 'haze-fixture'
destination.mkdir(exist_ok=True)
geometry = json.loads((p / 'title-haze-geometry.json').read_text())
oracle = json.loads((ROOT / 'analysis/frontend/patchy-fog-differential.json').read_text())
assert not oracle['failures'] and not oracle['program_failures']
assert hashlib.sha256((p / 'title-vulkan_capture.rdc').read_bytes()).hexdigest() == '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431'

def captured_program(event, template):
    words = list(template)
    raw = (p / f'pass-{event}-4-uniforms.bin').read_bytes()
    for i in range(18):
        values = struct.unpack_from('<4f', raw, 544+i*16)
        rgba = [round(v*255) for v in values]
        index = 10+i//2+(i%2)*8 if i < 16 else 43+i-16
        words[index] = rgba[3]<<24 | rgba[0]<<16 | rgba[1]<<8 | rgba[2]
    return struct.pack('<60I', *words)

corners = geometry['passes'][0]['postvs']['1']['float_rows']
rows = []
maximum_corner_error = 0.
for layer in range(4):
    a, b, c, d = [np.array(v[28+layer*4:30+layer*4], dtype=np.float32) for v in corners]
    horizontal = (d-a)*np.float32(.5)
    vertical = (a-b)*np.float32(.5)
    center = (a+c)*np.float32(.5)
    rows += [[horizontal[0], vertical[0], 0, center[0]],
             [horizontal[1], vertical[1], 0, center[1]]]
    for uv, actual in zip(((0,0),(0,1),(1,1),(1,0)), (a,b,c,d)):
        predicted = (np.float32(uv[0]*2-1)*horizontal-
                     np.float32(uv[1]*2-1)*vertical)+center
        maximum_corner_error = max(maximum_corner_error, float(np.max(np.abs(predicted-actual))))
assert maximum_corner_error <= 2e-6
noise = captured_program(8005, oracle['fixtures'][0]['noise'])
noise += struct.pack('<32f', *(float(v) for r in rows for v in r))
noise += struct.pack('<8f', 1, 0, 0, 0, 0, 0, 0, 0)
noise += (p / 'pass-8005-4-uniforms.bin').read_bytes()[544:832]
assert len(noise)==688
(destination / 'noise-uniform.bin').write_bytes(noise)

# Preserve the actual CPU-completed float4 attributes. The inverse affine
# reconstruction above is retained only for historical comparison; the quad
# shader consumes these captured values without estimating its rows or RNG.
quad = b''.join((p / f'title-haze-8005-input-{stream}.bin').read_bytes()[:64]
                for stream in range(5))
vertex_uniform = (p / 'pass-8005-0-uniforms.bin').read_bytes()
quad += vertex_uniform[3072:3088]
vertex_rows=geometry['passes'][0]['postvs']['1']['float_rows']
assert all(row[11]==1 and row[15]==1 for row in vertex_rows)
quad += vertex_uniform[5336:5344] + struct.pack('<2f',vertex_rows[0][11],vertex_rows[0][15])
assert len(quad) == 352
(destination / 'noise-quad.bin').write_bytes(quad)
direct_quad=b''.join((p / f'title-haze-8034-input-{stream}.bin').read_bytes()[:64]
                     for stream in range(5))
direct_vertex_uniform=(p / 'pass-8034-0-uniforms.bin').read_bytes()
direct_vertex_rows=geometry['passes'][1]['postvs']['1']['float_rows']
assert all(row[11]==1 and row[15]==1 for row in direct_vertex_rows)
direct_quad+=direct_vertex_uniform[3072:3088]+direct_vertex_uniform[5336:5344]+struct.pack('<2f',direct_vertex_rows[0][11],direct_vertex_rows[0][15])
assert len(direct_quad)==352
(destination / 'composite-quad.bin').write_bytes(direct_quad)

direct = captured_program(8034, oracle['fixtures'][0]['composite_direct'])
direct += bytes(128) + struct.pack('<4f', 0, 0, 0, 0)
direct += struct.pack('<4f', *geometry['passes'][1]['postvs']['1']['float_rows'][0][32:36])
direct += (p / 'pass-8034-4-uniforms.bin').read_bytes()[544:832]
assert len(direct)==688
(destination / 'composite-uniform.bin').write_bytes(direct)

surfaces = {}
for label in ('bitmap', 'gradient', 'noise-output', 'sampled-noise', 'before-output', 'composite-output', 'encoded-depth'):
    image = Image.open(p / f'title-haze-{label}.png').convert('RGBA')
    raw = image.tobytes()
    (destination / f'{label}.rgba').write_bytes(raw)
    surfaces[label] = {'width': image.width, 'height': image.height,
                       'sha256': hashlib.sha256(raw).hexdigest()}
assert surfaces['noise-output']['sha256'] == surfaces['sampled-noise']['sha256']

# This captured D24S8 surface returns its depth words first, followed by the
# separate stencil plane. Depth is in the low 24 bits of each u32. Xbox's raw
# color alias exposes byte1, byte0, byte3, byte2 as RGBA (dotmap_hilo_1 uses AR
# for its high word). Keep the integer depths to audit the native decode.
raw_depth = np.frombuffer((p / 'title-haze-world-depth.bin').read_bytes()[:640*480*4], dtype='<u4')
depth = raw_depth & 0xffffff
encoded = np.frombuffer((destination / 'encoded-depth.rgba').read_bytes(), dtype=np.uint8).reshape(-1, 4)
expected = np.column_stack(((raw_depth >> 8)&255, raw_depth&255, raw_depth>>24, (raw_depth>>16)&255)).astype(np.uint8)
assert np.array_equal(expected, encoded), 'original encoded depth differs from the final world depth'
(destination / 'depth-d24.bin').write_bytes(depth.astype('<u4').tobytes())
(destination / 'depth-r32.bin').write_bytes((depth.astype(np.float32)/np.float32(16777215)).astype('<f4').tobytes())
report = {'capture_sha256': '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431',
          'passes': [8005, 8034], 'legacy_inverse_rows_maximum_corner_error': maximum_corner_error,
          'noise_quad_sha256': hashlib.sha256(quad).hexdigest(),
          'noise_quad_uses_exact_captured_attributes': True,
          'composite_quad_sha256': hashlib.sha256(direct_quad).hexdigest(),
          'composite_quad_uses_exact_captured_attributes': True,
          'inline_vertex_alpha_preserved_before_interpolation': True,
          'combiner_float_constants_taken_from_original_upload': True,
          'encoded_depth_matches_world_d24': True, 'surfaces': surfaces,
          'scope': 'captured haze shader inputs; random state and camera history are not reconstructed',
          'complete_native_frame_parity_verified': False}
(destination / 'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
