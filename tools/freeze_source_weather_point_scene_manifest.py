#!/usr/bin/env python3
"""Freeze completed matched-source snow draw checks; no GPU or guest access."""
from pathlib import Path
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/weather-point-scene-prototype'
SOURCE = ROOT / 'analysis/multiplayer-shading/weather-point-original-reference-event215873'
ORIGINAL = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873'


def record(path):
    raw = path.read_bytes()
    return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())


def main():
    p = argparse.ArgumentParser()
    p.add_argument('normal_build', type=Path)
    p.add_argument('asan_build', type=Path)
    args = p.parse_args()
    for name in ('gpu-normal.log', 'gpu-asan.log'):
        text = (OUT / name).read_text()
        assert 'source changed 1255, native changed 1242, differing pixels 729, source-only 13, native-only 0, max channel 18, total RGB absolute 1900' in text
        assert 'All307200 source D24 integers/stencil bytes exact before and after' in text
        assert not any(token in text for token in ('Validation Error', 'Validation Warning', 'AddressSanitizer', 'runtime error:'))
    for name in ('cpu-normal.log', 'cpu-asan.log'):
        assert 'Frozen actual1015 points, all positive W' in (OUT / name).read_text()
    comparison = json.loads((OUT / 'scene-comparison.json').read_text())
    assert comparison['adapted']['alpha_equal'] and comparison['adapted']['differing_RGBA_pixels'] == 729
    paths = [ROOT / path for path in (
        'include/halo2/source_weather_point_render.hpp', 'src/source_weather_point_render.cpp',
        'shaders/source_weather_point.vert', 'shaders/source_weather_point.frag',
        'shaders/source_weather_point_program.glsl', 'shaders/vertex_program.glsl',
        'shaders/combiner.glsl', 'shaders/particle_depth.glsl',
        'tests/source_weather_point_gpu_fixture.hpp', 'tests/source_weather_point_scene_tests.cpp',
        'tools/build_source_weather_point_scene_prototype.sh', 'tools/compare_source_weather_point_scene.py',
        'tools/audit_source_weather_point_coordinate_bridge.py',
        'tools/freeze_source_weather_renderdoc_point_packet.py',
        'docs/SOURCE_WEATHER_POINT_SCENE_PARITY.md', 'docs/SOURCE_WEATHER_POINT_GPU_PROTOTYPE.md',
        'build-mp-shading/libhalo2_cache.a', 'build-mp-shading-asan/libhalo2_cache.a')]
    paths += [Path(__file__).resolve()]
    paths += [SOURCE / name for name in ('packet.json', 'points-v0-logical-argb.bin', 'vertex_constants_c0_c11.bin',
        'source-vs22-class0.vp', 'source-compatible-pixel-definition-39780.bin', 'clip_range.bin',
        'surface_size.bin', 'window_clip_regions.bin', 'sprite-resident-physical-partition.bin',
        'before-scene-color.rgba', 'after-scene-color.rgba', 'before-depth-d24-low24.bin')]
    paths += [ORIGINAL / name for name in ('before-scene-depth.raw', 'depth-upload.json',
        'original-software-replay-postvs.json', 'original-software-replay-postvs.bin')]
    paths += [OUT / name for name in ('build-normal.log', 'build-asan.log', 'cpu-normal.log', 'cpu-asan.log',
        'gpu-normal.log', 'gpu-asan.log', 'gpu-normal-unadapted.log', 'cpu-program-outputs.bin',
        'native-after-scene-color.rgba', 'native-unadapted-after-scene-color.rgba', 'scene-comparison.json',
        'source-only-point-localization.json', 'coordinate-bridge.json', 'source-after-scene.png', 'native-after-scene.png',
        'phone-source-native-point-scene.png', 'unadapted/source_weather_point.frag.spv')]
    for build in (args.normal_build.resolve(), args.asan_build.resolve()):
        paths += [build / 'source_weather_point_scene_tests', build / 'shaders/source_weather_point.vert.spv', build / 'shaders/source_weather_point.frag.spv']
    result = dict(version=1, authority='Immutable original GL event215873 and saved pre/post color/D24; isolated native point pass, no live activation.',
        event=215873, point_count=1015, comparison=comparison['adapted'],
        depth_stencil_integers_exact=307200, alpha_bytes_exact=307200, normal_GPU=True,
        ASAN_UBSAN_GPU=True, CPU_sanitizer_detect_leaks=True, GPU_sanitizer_detect_leaks=False,
        SDK_sync_diagnostics=0, production_renderer_runtime_frontend_edits=False, guest_IO=False,
        original_postVS_scope='Replay-generated unchanged original GL shader outputs; no physicalNV2A precision claim.',
        residuals='13 source-only edge pixels; remaining channel differences retained with no fitted filtering/point-size/reciprocal rule.',
        files={str(path.relative_to(ROOT)): record(path) for path in paths})
    path = OUT / 'manifest.json';path.write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(dict(manifest=str(path.relative_to(ROOT)), files=len(result['files']), **record(path))))


if __name__ == '__main__':
    main()
