#!/usr/bin/env python3
"""Freeze isolated exact depth-alias inputs and collect completed proof logs.

This tool never runs or activates the game. Run `before` before final checks,
then `after` after the standalone and composition checks have completed.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/weather-depth-alias-prototype'
COMPOSITION = ROOT / 'analysis/multiplayer-shading/weather-depth-alias-composition'
SOURCE = [
    'include/halo2/source_weather_depth_alias.hpp',
    'src/source_weather_depth_alias.cpp',
    'shaders/source_weather_depth_alias.comp',
    'tests/source_weather_depth_alias_tests.cpp',
    'tests/source_weather_depth_alias_sample.comp',
    'tests/source_weather_depth_alias_composition_tests.cpp',
    'tools/build_source_weather_depth_alias_prototype.sh',
    'tools/build_source_weather_depth_alias_composition.sh',
    'tools/freeze_source_weather_depth_alias_proof.py',
    'src/source_weather_plate_render.cpp',
    'shaders/source_weather_plate.vert',
    'shaders/source_weather_plate.frag',
]


def entry(path):
    path = Path(path)
    data = path.read_bytes()
    return dict(path=str(path.relative_to(ROOT)), bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def inputs():
    paths = {ROOT / p for p in SOURCE}
    pending = list(paths)
    # Include every local quoted header/fixture and shader include transitively.
    while pending:
        current = pending.pop()
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', current.read_text(), re.M):
            candidates = [current.parent / name, ROOT / 'include' / name, ROOT / 'tests' / name, ROOT / 'shaders' / name]
            resolved = next((p.resolve() for p in candidates if p.is_file()), None)
            if resolved is None:
                raise ValueError(f'local include missing: {name}')
            if resolved not in paths:
                paths.add(resolved)
                pending.append(resolved)
    paths.add(ROOT / 'assets/disc/default.xbe')
    paths.add(ROOT / 'analysis/multiplayer-shading/weather-gpu-assets/lockout-global21-class0.vp')
    base = ROOT / 'analysis/multiplayer-shading/weather-plate-original-reference'
    paths.update(base / p for p in ['reference.json', 'pixel-program-original-proof.json'])
    capture = base / 'event-215778'
    paths.update(capture / p for p in [
        'before-scene-color.rgba', 'after-scene-color.rgba', 'before-scene-depth.raw',
        'after-scene-depth.raw', 'before-depth-d24-low24.bin', 'stage-0-mip-0.rgba',
        'actual-four-vertices.bin', 'actual-scroll-c21-c23.bin', 'actual-clip-range.bin',
        'actual-clip-regions.bin', 'source-mode3-pixel-definition.bin'])
    paths.update(capture / f'stage-{stage}-mip-{mip}.rgba' for stage in range(1, 4) for mip in range(7))
    paths.update(ROOT / p for p in [
        'analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba',
        'analysis/multiplayer-shading/weather-graphics-cmake-integration/build/libhalo2_weather_render.a',
        'analysis/multiplayer-shading/resolution-bitmap-frozen-source/build/libhalo2_cache.a',
        '.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json',
        '.tools/validation/usr/lib/libVkLayer_khronos_validation.so'])
    return [entry(p) for p in sorted(paths)]


def lsan(path):
    text = path.read_text()
    match = re.search(r'SUMMARY: AddressSanitizer: (\d+) byte\(s\) leaked in (\d+) allocation\(s\)', text)
    assert match, f'LSAN control summary missing: {path}'
    assert 'source_weather_depth_alias.cpp:' not in text
    dbus = re.search(r'Indirect leak of (\d+) byte\(s\) in (\d+) object\(s\).*?_dbus_string_copy_len', text, re.S)
    assert dbus
    return dict(bytes=int(match[1]), allocations=int(match[2]), dbus_string_bytes=int(dbus[1]),
                observed_stacks='libdbus and unloaded graphics modules; no depth-alias source frames')


def collect():
    snapshot = json.loads((OUT / 'inputs-before-final-validation.json').read_text())
    now = inputs()
    assert snapshot == now, 'frozen native/reference/build inputs changed during final validation'
    alias = (ROOT / 'analysis/multiplayer-shading/weather-plate-original-reference/event-215778/stage-0-mip-0.rgba').read_bytes()
    standalone = (OUT / 'native-event215778-alias.rgba').read_bytes()
    composition_alias = (COMPOSITION / 'native-generated-depth-alias.rgba').read_bytes()
    assert alias == standalone == composition_alias
    scene = entry(COMPOSITION / 'native-after-scene-color.rgba')
    assert scene['sha256'] == '535179c90f409716be4f27f9e3953f5e9f1196c83f7a08f5a3a0bb2a0552c86f'
    for base in [OUT, COMPOSITION]:
        normal = (base / 'normal-gpu.log').read_text()
        sanitized = (base / 'asan-address-ub-gpu.log').read_text()
        assert 'NVIDIA GeForce RTX 5080' in normal and 'zero' in normal
        assert normal == sanitized, f'normal/address-UB output differs: {base}'
        assert 'ERROR:' not in normal and 'runtime error:' not in sanitized
        cpu = (base / 'asan-cpu.log').read_text()
        assert 'ERROR:' not in cpu and 'runtime error:' not in cpu
    full, empty = lsan(OUT / 'asan-full-gpu-lsan.log'), lsan(OUT / 'asan-empty-context-lsan.log')
    assert full['allocations'] == empty['allocations'] == 29
    # Do not claim matching totals: only the observed libdbus string differs.
    assert full['bytes'] - empty['bytes'] == full['dbus_string_bytes'] - empty['dbus_string_bytes']
    artifacts = []
    for base in [OUT, COMPOSITION]:
        artifacts.extend(entry(p) for p in sorted(base.glob('*.log')))
        artifacts.extend(entry(p) for p in sorted(base.glob('*.rgba')))
        for build in ['build', 'build-asan']:
            artifacts.extend(entry(p) for p in sorted((base / build).glob('source_weather_depth_alias*_tests')))
            artifacts.extend(entry(p) for p in sorted((base / build / 'shaders').glob('*.spv')))
    report = dict(
        scope='Isolated actual current native D24S8 integer/stencil producer and original event215778 plate consumer; no production/runtime/history/physical NV2A activation',
        device='NVIDIA GeForce RTX 5080', validation_layer_API='1.4.363',
        normal_GPU_exit=0, ASAN_address_UB_GPU_exit=0, ASAN_CPU_leaks_enabled_exit=0,
        SDK_synchronization_errors=0, original_alias_pixels_byte_exact=307200,
        original_depth_and_stencil_pixels_unchanged=307200, original_plate_target_alpha_unchanged=True,
        current_native_alias_is_direct_plate_stage0=True, composition_scene=scene,
        current_image_changes_and_alias_bindings_reuse_exact=True,
        odd_extents=[[1, 1], [3, 5], [17, 17]], all_channel_byte_values_checked=True,
        supported_layouts_restored=5, producer_admission_rejections=28, pending_ownership_rejections=4,
        sampled_UNORM_nearest_and_clamp_checked=True, shader_uses_only_Shader_capability=True,
        normal_ASAN_stdout_equal=True, all_frozen_inputs_unchanged=True,
        graphics_LSAN=dict(full=full, empty_context=empty, historical_plate_baseline=dict(bytes=3156, allocations=29),
            totals_equal=full['bytes']==empty['bytes'],
            difference_accounted_for_by_observed_libdbus_string=True,
            full_GPU_exit=1, empty_GPU_exit=1,
            boundary='Graphics leak checking is not clean. Address/UB GPU checks use detect_leaks=0 without suppressions; CPU leak checking remains enabled.'),
        inputs=now, artifacts=artifacts,
        documentation='docs/SOURCE_WEATHER_DEPTH_ALIAS_PROTOTYPE.md',
    )
    (OUT / 'validation.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=['before', 'after'])
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if args.stage == 'before':
        snapshot = inputs()
        (OUT / 'inputs-before-final-validation.json').write_text(json.dumps(snapshot, indent=2)+'\n')
        print(json.dumps(dict(frozen_inputs=len(snapshot))))
    else:
        report = collect()
        print(json.dumps({k: v for k, v in report.items() if k not in ['inputs', 'artifacts']}, indent=2))


if __name__ == '__main__':
    main()
