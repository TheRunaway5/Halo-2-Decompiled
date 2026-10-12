#!/usr/bin/env python3
"""Pin the original GL raw-row point-coordinate bridge, without replay or GPU."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873'
OUT = ROOT / 'analysis/multiplayer-shading/weather-point-scene-prototype/coordinate-bridge.json'
PINNED = ROOT / 'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    draw = json.loads((SOURCE / 'draw.json').read_text())
    assert draw['event'] == 215873 and draw['rasterizer']['state']['pointOriginUpperLeft']
    viewport = draw['viewport']
    assert viewport == dict(enabled=True, x=0., y=0., width=640., height=480., minDepth=0., maxDepth=1.)
    surfaces = json.loads((SOURCE / 'surfaces.json').read_text())
    assert 'no presentation flip' in surfaces['raw_row_order']
    vertex, fragment = [(SOURCE / (name + '.glsl')).read_text() for name in ('vertex', 'fragment')]
    assert 'oPos.xy = (2.0f * oPos.xy - surfaceSize) / surfaceSize;' in vertex
    assert 'vec4 pT3 = vec4(gl_PointCoord, 1.0, 1.0);' in fragment
    assert 'vec4 t3 = textureProj(texSamp3, (pT3.xyw));' in fragment
    native_vertex = (ROOT / 'shaders/source_weather_point.vert').read_text()
    native_fragment = (ROOT / 'shaders/source_weather_point.frag').read_text()
    assert 'clip.xy=(2.0*clip.xy-weather.surface.xy)/weather.surface.xy;' in native_vertex
    assert 'pointCoordinates.y=1.0-pointCoordinates.y;' in native_fragment
    tree = json.loads((PINNED / 'tree.json').read_text())
    assert tree['sha'] == 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee' and not tree['truncated']
    blobs = {row['path']: row['sha'] for row in tree['tree'] if row['type'] == 'blob'}
    primary = []
    for name in ('hw/xbox/nv2a/pgraph/glsl/vsh-prog.c', 'hw/xbox/nv2a/pgraph/glsl/psh.c',
                 'hw/xbox/nv2a/pgraph/gl/draw.c', 'hw/xbox/nv2a/pgraph/gl/renderer.c'):
        p = PINNED / name;raw = p.read_bytes()
        assert hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest() == blobs[name]
        primary.append(p)
    report = dict(event=215873, source_point_origin='upper-left', source_viewport=viewport,
        source_framebuffer_storage=surfaces['raw_row_order'],
        source_sampling='Source GL fragment samples original gl_PointCoord.xy directly; q=1.',
        row_coordinate_inference=dict(GL='t=.5-(numeric_row-center)/diameter',
                                     Vulkan_positive_viewport='t=.5+(numeric_row-center)/diameter',
                                     bridge='t_GL=1-t_Vulkan; u unchanged',
                                     framebuffer='Both wrappers map source screen S to the same numeric row S.'),
        scope='Source fields and literal shader expressions plus API row-coordinate arithmetic. No pixel fit, original instruction replacement, new guest frame or physicalXbox coverage claim.',
        original_execution_cases=0, GPU_jobs=0, guest_IO=False,
        files={str(p.relative_to(ROOT)): sha(p) for p in [Path(__file__).resolve(), SOURCE / 'draw.json',
            SOURCE / 'surfaces.json', SOURCE / 'vertex.glsl', SOURCE / 'fragment.glsl',
            ROOT / 'shaders/source_weather_point.vert', ROOT / 'shaders/source_weather_point.frag',
            ROOT / 'include/halo2/source_weather_point_render.hpp', *primary]})
    OUT.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(path=str(OUT.relative_to(ROOT)), sha256=sha(OUT))))


if __name__ == '__main__':
    main()
