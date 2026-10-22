#!/usr/bin/env python3
"""Freeze, measure and display connected source weather without replay/guest IO."""
from pathlib import Path
import argparse
import hashlib
import json
import re

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/weather-connected-scene-prototype'
PLATE = ROOT / 'analysis/multiplayer-shading/weather-plate-original-reference/event-215778'
POINT = ROOT / 'analysis/multiplayer-shading/weather-point-original-reference-event215873'
RAW_POINT = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873'
SOURCES = [
    'tests/source_weather_connected_scene_tests.cpp',
    'tools/build_source_weather_connected_scene.sh',
    'tools/freeze_source_weather_connected_scene.py',
    'src/rendering/weather/source_weather_depth_alias.cpp', 'src/rendering/weather/source_weather_plate_render.cpp',
    'src/rendering/weather/source_weather_point_render.cpp', 'shaders/source_weather_depth_alias.comp',
    'shaders/source_weather_plate.vert', 'shaders/source_weather_plate.frag',
    'shaders/source_weather_point.vert', 'shaders/source_weather_point.frag']


def artifact(path):
    path = Path(path)
    data = path.read_bytes()
    return dict(path=str(path.relative_to(ROOT)), bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def frozen_inputs():
    files = {ROOT / p for p in SOURCES}
    pending = list(files)
    while pending:
        current = pending.pop()
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', current.read_text(), re.M):
            candidates = [current.parent / name, ROOT / 'include' / name, ROOT / 'tests' / name, ROOT / 'shaders' / name]
            found = next((p.resolve() for p in candidates if p.is_file()), None)
            if found is None:
                raise ValueError(f'missing local include {name}')
            if found not in files:
                files.add(found)
                pending.append(found)
    files.update(PLATE / p for p in ['before-scene-color.rgba', 'after-scene-color.rgba', 'before-scene-depth.raw', 'after-scene-depth.raw', 'before-depth-d24-low24.bin', 'stage-0-mip-0.rgba', 'actual-four-vertices.bin', 'actual-scroll-c21-c23.bin', 'actual-clip-range.bin', 'actual-clip-regions.bin', 'source-mode3-pixel-definition.bin'])
    files.update(PLATE / f'stage-{stage}-mip-{mip}.rgba' for stage in range(1, 4) for mip in range(7))
    files.update(POINT / p for p in ['packet.json', 'before-scene-color.rgba', 'after-scene-color.rgba', 'before-depth-d24-low24.bin', 'source-vs22-class0.vp', 'points-v0-logical-argb.bin', 'vertex_constants_c0_c11.bin', 'source-compatible-pixel-definition-39780.bin', 'clip_range.bin', 'sprite-resident-physical-partition.bin'])
    files.update(RAW_POINT / p for p in ['before-scene-color.rgba', 'after-scene-color.rgba', 'before-scene-depth.raw', 'after-scene-depth.raw', 'draw.json', 'vertex.glsl', 'fragment.glsl'])
    files.update(ROOT / p for p in [
        'assets/disc/default.xbe', 'docs/SOURCE_WEATHER_DRAW_SUBMISSION_AUDIT.md',
        'analysis/multiplayer-shading/weather-gpu-assets/lockout-global21-class0.vp',
        'analysis/multiplayer-shading/weather-plate-original-reference/reference.json',
        'analysis/multiplayer-shading/weather-plate-original-reference/pixel-program-original-proof.json',
        'analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba',
        'analysis/multiplayer-shading/weather-point-scene-prototype/native-after-scene-color.rgba',
        'analysis/multiplayer-shading/weather-point-scene-prototype/cpu-program-outputs.bin',
        'analysis/multiplayer-shading/weather-point-scene-prototype/scene-comparison.json',
        'analysis/multiplayer-shading/resolution-bitmap-frozen-source/build/libhalo2_cache.a',
        '.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json',
        '.tools/validation/usr/lib/libVkLayer_khronos_validation.so'])
    return [artifact(p) for p in sorted(files)]


def compare(source_before, source_after, native_before, native_after):
    assert len(source_before) == len(source_after) == len(native_before) == len(native_after) == 640 * 480 * 4
    histogram = {}
    source_mask, native_mask = set(), set()
    different = absolute = 0
    for i in range(307200):
        j = i * 4
        a, b = source_after[j:j+4], native_after[j:j+4]
        assert a[3] == source_before[j+3] and b[3] == native_before[j+3]
        if a != source_before[j:j+4]:
            source_mask.add(i)
        if b != native_before[j:j+4]:
            native_mask.add(i)
        if a != b:
            different += 1
        diffs = [abs(a[c] - b[c]) for c in range(3)]
        maximum = max(diffs)
        absolute += sum(diffs)
        histogram[str(maximum)] = histogram.get(str(maximum), 0) + 1
    metrics = dict(source_changed_pixels=len(source_mask), native_changed_pixels=len(native_mask),
        exact_RGBA_pixels=307200-different, differing_RGBA_pixels=different,
        effect_mask_intersection=len(source_mask & native_mask), source_only_effect_pixels=len(source_mask-native_mask),
        native_only_effect_pixels=len(native_mask-source_mask), RGB_absolute_sum=absolute,
        maximum_RGB_channel_difference=max(map(int, histogram)), max_channel_histogram=histogram,
        pixels_exceeding={str(t): sum(count for value, count in histogram.items() if int(value)>t) for t in [0, 1, 2, 4, 8, 16, 32, 64]},
        target_alpha_unchanged=True)
    return metrics, source_mask-native_mask, native_mask-source_mask


def phone(original, native, metrics):
    from PIL import Image, ImageDraw, ImageFont
    import subprocess
    font_path = subprocess.check_output(['fc-match', '-f', '%{file}', 'DejaVu Sans'], text=True)
    title = ImageFont.truetype(font_path, 18)
    text = ImageFont.truetype(font_path, 14)
    canvas = Image.new('RGB', (672, 1330), '#11151c')
    draw = ImageDraw.Draw(canvas)
    draw.text((16, 14), 'Lockout: connected native clouds and snow', font=title, fill='#eef3fa')
    draw.text((16, 45), 'Original weather after plate + snow, xemu software capture', font=text, fill='#b9cbe2')
    # Identical presentation adapter; GPU input and every metric use raw rows.
    source_image = Image.frombytes('RGBA', (640, 480), original).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    native_image = Image.frombytes('RGBA', (640, 480), native).transpose(Image.Transpose.FLIP_TOP_BOTTOM)
    canvas.paste(source_image.convert('RGB'), (16, 72))
    draw.text((16, 574), 'Native Vulkan: current D24 -> packed alias -> clouds -> snow', font=text, fill='#b9cbe2')
    canvas.paste(native_image.convert('RGB'), (16, 601))
    lines = [
        ('All 307,200 packed depth pixels exact; depth/stencil/alpha preserved.', '#b9e8c4'),
        (f"Combined: {metrics['differing_RGBA_pixels']:,} pixels differ; max RGB byte {metrics['maximum_RGB_channel_difference']}.", '#ffd093'),
        ('Cloud byte differences and original snow edge coverage remain open.', '#ffd093'),
        ('Same captured background; both effects execute in original order.', '#b9cbe2'),
        ('Full native world, live admission and physical Xbox parity remain open.', '#b9cbe2')]
    for i, (line, color) in enumerate(lines):
        draw.text((16, 1103+i*33), line, font=text, fill=color)
    canvas.save(OUT / 'phone-source-native-connected-weather.png')
    source_image.save(OUT / 'source-after-connected-weather.png')
    native_image.save(OUT / 'native-after-connected-weather.png')


def collect():
    before = json.loads((OUT / 'inputs-before-validation.json').read_text())
    current = frozen_inputs()
    assert before == current, 'connected native/reference inputs changed during validation'
    original_before = (PLATE / 'before-scene-color.rgba').read_bytes()
    original_plate = (PLATE / 'after-scene-color.rgba').read_bytes()
    original_point_before = (POINT / 'before-scene-color.rgba').read_bytes()
    original_final = (POINT / 'after-scene-color.rgba').read_bytes()
    assert original_plate == original_point_before
    native_plate = (OUT / 'native-after-plate-color.rgba').read_bytes()
    native_final = (OUT / 'native-after-connected-weather-color.rgba').read_bytes()
    native_alias = (OUT / 'native-generated-depth-alias.rgba').read_bytes()
    assert native_plate == (ROOT / 'analysis/multiplayer-shading/weather-plate-scene-prototype/native-after-scene-color.rgba').read_bytes()
    assert native_alias == (PLATE / 'stage-0-mip-0.rgba').read_bytes()
    for mode in ['normal', 'asan-address-ub']:
        log = (OUT / f'{mode}-gpu.log').read_text()
        assert 'zero SDK synchronization errors' in log and '13 guards and reuse exact' in log
        assert 'ERROR:' not in log and 'runtime error:' not in log
    assert (OUT / 'normal-gpu.log').read_text() == (OUT / 'asan-address-ub-gpu.log').read_text()
    assert 'all 1228800 continuity bytes exact' in (OUT / 'asan-cpu.log').read_text()
    cloud, _, _ = compare(original_before, original_plate, original_before, native_plate)
    combined, _, _ = compare(original_before, original_final, original_before, native_final)
    snow, missing_snow, extra_snow = compare(original_point_before, original_final, native_plate, native_final)
    prior_native_snow = (ROOT / 'analysis/multiplayer-shading/weather-point-scene-prototype/native-after-scene-color.rgba').read_bytes()
    prior_snow, prior_missing, _ = compare(original_point_before, original_final, original_point_before, prior_native_snow)
    assert cloud['maximum_RGB_channel_difference'] == 1 and cloud['differing_RGBA_pixels'] == 14477
    assert prior_snow['source_only_effect_pixels'] == 13
    phone(original_final, native_final, combined)
    outputs = [artifact(p) for p in sorted(OUT.glob('*.rgba'))]
    outputs += [artifact(p) for p in sorted(OUT.glob('*.png'))]
    outputs += [artifact(p) for p in sorted(OUT.glob('*.log'))]
    outputs += [artifact(OUT / 'cpu-point-program-outputs.bin')]
    for build in ['build', 'build-asan']:
        outputs += [artifact(OUT / build / 'source_weather_connected_scene_tests')]
        outputs += [artifact(p) for p in sorted((OUT / build / 'shaders').glob('*.spv'))]
    report = dict(
        scope='Isolated current native D24 -> exact alias -> original plate215778 -> original snow215873 over captured scene background; no live world/weather/admission activation',
        original_backend='Unmodified original xemu OpenGL llvmpipe software capture, not physical Xbox',
        native_backend='NVIDIA GeForce RTX 5080 Vulkan with actual SDK1.4.363 and synchronization validation',
        source_order_authority='Source-inspected unchanged36B50 calls all37A60 plates before all391F0 particle draws; exact captured plate output equals point input',
        original_color_continuity=artifact(PLATE / 'after-scene-color.rgba'),
        original_depth_continuity=artifact(PLATE / 'before-scene-depth.raw'),
        all_original_alias_pixels_exact=307200, all_original_D24_stencil_alpha_pixels_preserved=307200,
        one_caller_fence_covers_producer_plate_snow_and_readbacks=True,
        generated_stage0_is_current_native_D24=True, intermediate_plate_equals_previous_native_bytes=True,
        guarded_inputs_rejected=13, retained_bindings_reuse_exact=True,
        normal_GPU_exit=0, ASAN_address_UB_GPU_exit=0, ASAN_CPU_leaks_enabled_exit=0,
        SDK_synchronization_errors=0, normal_ASAN_stdout_equal=True,
        CPU_point_program_outputs_equal_existing_source_fixture=(OUT / 'cpu-point-program-outputs.bin').read_bytes()==(ROOT / 'analysis/multiplayer-shading/weather-point-scene-prototype/cpu-program-outputs.bin').read_bytes(),
        original_point_sampler_LOD_bounds=[-1000, 1000], point_coordinates='OriginalGlRawRowsUpperLeft',
        raw_rows_preserved=True, display_adapter='Both outputs identically reverse rows to original SaveTexture display convention; no GPU input or metric flip',
        cloud_stage=cloud, connected_snow_stage=snow, combined_weather=combined,
        known_point_edge_boundary=dict(prior_source_only_edge_pixels=13,
            connected_source_only_pixels=len(missing_snow), connected_prior_missing_intersection=len(missing_snow & prior_missing),
            connected_additional_source_only_pixels=len(missing_snow-prior_missing), connected_native_only_pixels=len(extra_snow)),
        unresolved=['Original/native cloud one-byte response boundary remains.',
            'Original point-square edge coverage and filtering/arithmetic differences remain; no rounding or texture fit.',
            'Captured background geometry/lighting are not produced by this fixture.',
            'Native frame admission/simulation/timing/residency/full world path and physical Xbox precision remain open.'],
        sanitizer_boundary='CPU leak checking enabled and clean; graphics address/UB uses detect_leaks=0 without suppressions, with separately documented external graphics LSAN limitation.',
        all_inputs_unchanged=current==before, inputs=current, outputs=outputs,
        documentation='docs/SOURCE_WEATHER_CONNECTED_SCENE_PROTOTYPE.md')
    (OUT / 'validation.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=['before', 'after'])
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    if args.stage == 'before':
        files = frozen_inputs()
        (OUT / 'inputs-before-validation.json').write_text(json.dumps(files, indent=2)+'\n')
        print(json.dumps(dict(frozen_inputs=len(files))))
    else:
        report = collect()
        print(json.dumps({k:v for k,v in report.items() if k not in ['inputs', 'outputs']}, indent=2))


if __name__ == '__main__':
    main()
