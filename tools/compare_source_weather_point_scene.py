#!/usr/bin/env python3
"""Compare saved original/native weather draw pixels, without changing either.

No guest, replay or GPU use. PNG presentation reverses both saved raw row orders,
which is independently checked against the original RenderDoc SaveTexture PNG.
"""
from pathlib import Path
import hashlib
import json
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'analysis/multiplayer-shading/weather-point-original-reference-event215873'
ORIGINAL = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873'
OUT = ROOT / 'analysis/multiplayer-shading/weather-point-scene-prototype'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def pixels(path):
    raw = path.read_bytes()
    assert len(raw) == 640 * 480 * 4
    return np.frombuffer(raw, np.uint8).reshape(480, 640, 4)


def stats(before, original, native):
    delta = np.abs(original.astype(np.int16) - native.astype(np.int16))
    sm, nm = [np.any(image != before, axis=2) for image in (original, native)]
    different = np.any(original != native, axis=2)
    hist = np.bincount(delta[:, :, :3].max(axis=2).ravel(), minlength=256)
    return dict(source_changed_pixels=int(sm.sum()), native_changed_pixels=int(nm.sum()),
                exact_RGBA_pixels=int((~different).sum()), differing_RGBA_pixels=int(different.sum()),
                effect_mask_intersection=int((sm & nm).sum()), source_only_effect_pixels=int((sm & ~nm).sum()),
                native_only_effect_pixels=int((nm & ~sm).sum()), RGB_absolute_sum=int(delta[:, :, :3].sum()),
                max_channel_difference=int(delta.max()), max_RGB_channel_histogram={str(i): int(n) for i, n in enumerate(hist) if n},
                pixels_exceeding={str(k): int(np.any(delta > k, axis=2).sum()) for k in (0, 1, 2, 4, 8, 16, 32, 64)},
                alpha_equal=bool(np.array_equal(original[:, :, 3], native[:, :, 3])))


def main():
    before, original = [pixels(SOURCE / (name + '-scene-color.rgba')) for name in ('before', 'after')]
    native = pixels(OUT / 'native-after-scene-color.rgba')
    assert np.array_equal(np.array(Image.open(ORIGINAL / 'after-source-point.png').convert('RGB')), original[::-1, :, :3])
    report = dict(event=215873, original_backend='Unmodified original title in xemu OpenGL llvmpipe; retained RenderDoc draw.',
                  native_backend='NVIDIA RTX5080 Vulkan isolated SourceWeatherPointRender.',
                  original_packet_sha256=sha(SOURCE / 'packet.json'),
                  scope='Actual1015 points/camera/compatible39780 PS/sprite over original color/D24, one draw only; no runtime/weather publication or physicalXbox claim.',
                  row_storage='Original raw rows unchanged for GPU uploads. Both presentation PNGs reverse rows; exactly matches original SaveTexture PNG RGB.',
                  adaptation='Explicit upper-left GL raw-row PointCoordY complement; no authored bytes or original scene/depth change.',
                  unadapted=stats(before, original, pixels(OUT / 'native-unadapted-after-scene-color.rgba')),
                  adapted=stats(before, original, native),
                  D24=dict(integers=307200, original_stencil_bytes=307200, exact_upload_readback_before_after=True, float_substitution=False),
                  unresolved='Remaining source-only/raster and channel differences are retained. No filter/reciprocal/point-coverage rule is fitted.')
    files = [SOURCE / name for name in ('packet.json', 'before-scene-color.rgba', 'after-scene-color.rgba', 'before-depth-d24-low24.bin')]
    files += [OUT / name for name in ('native-after-scene-color.rgba', 'native-unadapted-after-scene-color.rgba', 'gpu-normal.log', 'gpu-asan.log', 'gpu-normal-unadapted.log', 'unadapted/source_weather_point.frag.spv')]
    report['files'] = {str(p.relative_to(ROOT)): dict(bytes=p.stat().st_size, sha256=sha(p)) for p in files}
    (OUT / 'scene-comparison.json').write_text(json.dumps(report, indent=2) + '\n')

    cpu = np.frombuffer((OUT / 'cpu-program-outputs.bin').read_bytes(), np.float32).reshape(1015, 16, 4)
    pos = cpu[:, 0].copy(); pos[:, :2] = np.trunc(pos[:, :2] * 16) / 16
    sizes = cpu[:, 6, 0]
    depth = np.frombuffer((SOURCE / 'before-depth-d24-low24.bin').read_bytes(), np.uint32).reshape(480, 640)
    metadata = json.loads((ORIGINAL / 'original-software-replay-postvs.json').read_text())
    assert metadata['mesh']['vertexByteStride'] == 204 and metadata['mesh']['numIndices'] == 1015
    layout, cursor = {}, 0
    for item in metadata['output_signature']:
        layout[item['varName']] = (cursor, cursor + item['compCount']); cursor += item['compCount']
    assert cursor == 51
    replay = np.frombuffer((ORIGINAL / 'original-software-replay-postvs.bin').read_bytes(), np.float32).reshape(1015, 51)
    replay_pos = replay[:, slice(*layout['vtxPos0'])]
    replay_size = replay[:, slice(*layout['gl_PointSize'])][:, 0]
    replay_color = replay[:, slice(*layout['vtxD0'])]
    rows = []
    source_only = np.any(original != before, axis=2) & ~np.any(native != before, axis=2)
    for y, x in np.argwhere(source_only):
        distance = np.abs(pos[:, :2] - np.array([x + .5, y + .5])).max(axis=1) / sizes
        near = []
        for index in np.argsort(distance)[:2]:
            size, w = float(sizes[index]), float(pos[index, 3])
            dx = (float(x) + .5 - float(pos[index, 0])) / size
            dy = (float(y) + .5 - float(pos[index, 1])) / size
            near.append(dict(point=int(index), center=pos[index, :2].tolist(), oPts=size, W=w,
                             ideal_fractional_square_covers=bool(distance[index] <= .5),
                             original_GL_PointCoord=[.5 + dx, .5 - dy], distance_in_diameters=float(distance[index]),
                             source_depth_integer=int(depth[y, x]), floor_W=int(np.floor(w)), floor_W_minus_depth=int(np.floor(w)) - int(depth[y, x]),
                             unchanged_original_GL_replay_vtxPos0=replay_pos[index].tolist(),
                             unchanged_original_GL_replay_PointSize=float(replay_size[index]),
                             source_CPU_screenXY_equal_original_replay=bool(np.array_equal(pos[index, :2].view(np.uint32), replay_pos[index, :2].view(np.uint32))),
                             CPU_floor_W_equal_original_replay=int(np.floor(w)) == int(np.floor(replay_pos[index, 3]))))
        rows.append(dict(raw_pixel=[int(x), int(y)], before=before[y, x].tolist(), original=original[y, x].tolist(), native=native[y, x].tolist(), nearest_cpu_source_program_points=near))
    def difference(a, b):
        return dict(different_words=int((a.view(np.uint32) != b.view(np.uint32)).sum()), maximum_absolute=float(np.abs(a - b).max()))
    localization = dict(scope='CPU original-program and unchanged software GL replay output localization; no fitted host raster rule.',
                        source_only_pixels=len(rows), rows=rows,
                        original_postvs=dict(scope=metadata['scope'], stride=204, rows=1015, layout_components=layout,
                            source_point_size_range=[float(replay_size.min()), float(replay_size.max())],
                            CPU_source_program_vs_original_replay=dict(position=difference(pos, replay_pos), W=difference(pos[:, 3], replay_pos[:, 3]), point_size=difference(cpu[:, 6, 0], replay_size), D0=difference(cpu[:, 3], replay_color)),
                            source_only_closest_owners=dict(count=len(rows), screenXY_all_exact=all(r['nearest_cpu_source_program_points'][0]['source_CPU_screenXY_equal_original_replay'] for r in rows), floorW_all_exact=all(r['nearest_cpu_source_program_points'][0]['CPU_floor_W_equal_original_replay'] for r in rows)),
                            files={str((ORIGINAL / name).relative_to(ROOT)): sha(ORIGINAL / name) for name in ('original-software-replay-postvs.json', 'original-software-replay-postvs.bin')}))
    (OUT / 'source-only-point-localization.json').write_text(json.dumps(localization, indent=2) + '\n')

    source_image, native_image = [Image.fromarray(image[::-1, :, :3]) for image in (original, native)]
    source_image.save(OUT / 'source-after-scene.png'); native_image.save(OUT / 'native-after-scene.png')
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    font, small = [ImageFont.truetype(font_path, size) for size in (24, 18)]
    canvas = Image.new('RGB', (1280, 1120), (20, 23, 30)); draw = ImageDraw.Draw(canvas)
    draw.text((18, 12), 'Original snow draw over the same saved scene and depth', font=font, fill='white')
    draw.text((18, 49), 'Original xemu / OpenGL', font=small, fill='white'); draw.text((658, 49), 'Native Vulkan isolated pass', font=small, fill='white')
    canvas.paste(source_image, (0, 78)); canvas.paste(native_image, (640, 78))
    box = (160, 175, 373, 335)
    for x, image in ((0, source_image), (640, native_image)):
        canvas.paste(image.crop(box).resize((639, 480), Image.Resampling.NEAREST), (x, 600))
    draw.text((18, 570), 'Same crop enlarged 3x with nearest pixels', font=small, fill='white')
    draw.text((18, 1088), f"Exact depth/alpha retained; {report['adapted']['differing_RGBA_pixels']} differing pixels, mostly 1 channel step. Runtime integration remains open.", font=small, fill='white')
    canvas.save(OUT / 'phone-source-native-point-scene.png')
    print(json.dumps(report['adapted']))


if __name__ == '__main__':
    main()
