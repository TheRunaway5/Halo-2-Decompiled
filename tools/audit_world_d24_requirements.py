#!/usr/bin/env python3
"""Read immutable title/capture evidence and current native depth consumers.

CPU only: no Vulkan calls, capture replay controller, guest I/O or shared build.
Expression results are binary32 diagnostics, not GPU fixed-point observations.
"""
from pathlib import Path
import collections
import hashlib
import json
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'analysis/multiplayer-shading/world-d24-requirements'
PINNED = ROOT / 'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'
POINT = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1/source-snow-event215873'
SAVED = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/source'


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    names = ['src/rendering/renderer/renderer.cpp', 'include/halo2/renderer.hpp', 'src/rendering/world/bsp_postprocess.cpp',
             'include/halo2/bsp_postprocess.hpp', 'src/rendering/effects/particle_renderer.cpp', 'src/rendering/scope_render.cpp',
             'src/rendering/shadows/source_shadow_draw_storage.cpp', 'shaders/world_vertex.glsl', 'shaders/world.frag',
             'shaders/particle_depth.glsl', 'shaders/fog.frag', 'shaders/fog_color.glsl',
             'include/halo2/source_weather_depth_alias.hpp', 'src/rendering/weather/source_weather_depth_alias.cpp',
             'shaders/source_weather_depth_alias.comp', 'tools/audit_world_depth_encoding.cpp',
             'tools/audit_world_d24_requirements.py',
             'tools/reference/halo2-decompiled/src/unknown_01cf50.cpp',
             'analysis/multiplayer-shading/original-xemu/original_capture_20.json',
             'docs/SOURCE_WEATHER_DEPTH_ALIAS_PROTOTYPE.md', 'docs/SOURCE_WEATHER_POINT_SCENE_PARITY.md']
    paths = [ROOT / p for p in names]
    paths += [ROOT / 'shaders/light_volume.frag', ROOT / 'shaders/patchy_composite.frag',
              ROOT / 'shaders/bsp_patchy_composite.frag', ROOT / 'build-mp-shading/shaders/world.frag.spv',
              ROOT / 'tools/reference/halo2-decompiled/src/unknown_020560.cpp',
              ROOT / 'docs/APPEARANCE_RESIDUAL_DIAGNOSIS.md',
              ROOT / 'docs/SOURCE_WORLD_D24_MIGRATION_AUDIT.md']
    paths += [POINT / name for name in ('surfaces.json', 'depth-upload.json', 'before-scene-depth.raw',
                                      'after-scene-depth.raw', 'fragment.glsl')]
    tree = json.loads((PINNED / 'tree.json').read_text())
    assert tree['sha'] == 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee' and not tree['truncated']
    blobs = {r['path']: r['sha'] for r in tree['tree'] if r['type'] == 'blob'}
    primary = ['hw/xbox/nv2a/pgraph/glsl/psh.c', 'hw/xbox/nv2a/pgraph/vk/surface.c',
               'hw/xbox/nv2a/pgraph/vk/surface-compute.c']
    for name in primary:
        p = PINNED / name; raw = p.read_bytes()
        assert hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest() == blobs[name]
        paths.append(p)
    snapshots = {}
    sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference/halo2-decompiled/tools')]
    from differential_source_object_admission_snapshot import Snapshot
    from xbe import Xbe, FUNCTIONS_CSV
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32
    import csv
    xbe_path = ROOT / 'assets/disc/default.xbe'
    xbe = Xbe(xbe_path)
    functions = {int(r['va'], 16): r for r in csv.DictReader(Path(FUNCTIONS_CSV).open())}
    blocks, disassembly = {}, []
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    for address in (0x12490, 0x128c0, 0x15370):
        raw = xbe.read(address, int(functions[address]['size']))
        instructions = list(decoder.disasm(raw, address))
        assert sum(i.size for i in instructions) == len(raw)
        blocks[f'{address:08X}'] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
        disassembly += [f'{i.address:08X} {i.bytes.hex():24s} {i.mnemonic} {i.op_str}' for i in instructions]
    for label in ('lockout-weather-full-entry', 'lockout-weather-full-return'):
        directory = POINT.parent / label
        snapshot = Snapshot(directory)
        settings = struct.unpack('<H', snapshot.read(0x485ac8, 2))[0]
        snapshots[label] = dict(RAM_sha256=hashlib.sha256(snapshot.ram).hexdigest(),
                                source_485AC8=settings, depth_format_setting_byte=settings & 255)
        paths.append(directory / 'export.json')
    raw = (POINT / 'before-scene-depth.raw').read_bytes()
    assert len(raw) == 640 * 480 * 4 and raw == (POINT / 'after-scene-depth.raw').read_bytes()
    packed = struct.unpack('<307200I', raw)
    stencil = collections.Counter(n & 255 for n in packed)
    depth = [n >> 8 for n in packed]
    capture = json.loads((ROOT / names[18]).read_text())
    texture = [r for r in capture['textures'] if r.get('id') == 'ResourceId::8601']
    assert len(texture) == 1 and texture[0]['format'] == 'D24S8'
    captured_passes = {}
    for event in (8878, 8787, 8102, 8097):
        p = SAVED / f'{event}-pipeline.json'
        data = json.loads(p.read_text())
        captured_passes[str(event)] = dict(depth_resource=data['depth']['resource'],
            state=data['vulkan']['depthStencil'], raster=data['vulkan']['rasterizer'],
            blend=data['vulkan']['colorBlend'], viewport=data['viewport'],
            index_count=data['action']['numIndices'], indices=data['indices'],
            postVS=data['postvs'], postVS_stride=data['postvs_description']['vertexByteStride'],
            identity='retained Zanzibar sky A/B' if event in (8097, 8102) else 'retained selected original20 draw',
            evidence_limit='No per-event before/after stencil image in this exporter; no Lockout ownership attribution.')
        paths.append(p)
        if event in (8097, 8102):
            paths += [p for p in sorted(SAVED.glob(f'{event}-*')) if p.is_file() and p not in paths]
    p = SAVED / '8878-fragment.spvasm'
    assert 'GLSL.std.450::Floor' in p.read_text() and '/ 16777216.0000' in p.read_text()
    paths.append(p)
    initial_hashes = {str(p.relative_to(ROOT)): digest(p) for p in paths}
    native_disassembly = OUT / 'native-world-fragment.spvasm'
    subprocess.run(['spirv-dis', str(ROOT / 'build-mp-shading/shaders/world.frag.spv'),
                    '-o', str(native_disassembly)], check=True)
    native_lines = [line.strip() for line in native_disassembly.read_text().splitlines()
                    if 'OpFDiv' in line and '%float_16777215' in line]
    assert len(native_lines) == 1
    binary = OUT / 'depth-expression-cpu'
    subprocess.run(['c++', '-std=c++20', '-O2', '-ffp-contract=off', '-fno-fast-math',
                    str(ROOT / 'tools/audit_world_depth_encoding.cpp'), '-o', str(binary)], check=True)
    expression = json.loads(subprocess.check_output([str(binary), str(POINT / 'before-scene-depth.raw')]))
    (OUT / 'depth-expression.json').write_text(json.dumps(expression, indent=2) + '\n')
    report = dict(scope='Read-only original title/static renderer audit plus CPU encoding diagnostics. No GPU, replay controller, guest IO, source replacement or production change.',
        source_xbe_sha256=digest(xbe_path), source_blocks=blocks, actual_settings=snapshots,
        pinned_xemu_commit=tree['sha'], original20_depth=texture[0], original20_passes=captured_passes,
        original_lockout_depth=dict(pixels=len(packed), raw_bytes=len(raw), before_after_identical=True,
            minimum=min(depth), maximum=max(depth), unique_depth_integers=len(set(depth)),
            stencil_histogram=dict(sorted(stencil.items())),
            limit='These are observed stored D24 integers, not recovered pre-fragment W. The stencil histogram is this saved scene only, not an all-map or live producer.'),
        native_SPIRV=dict(path='build-mp-shading/shaders/world.frag.spv',
            sha256=initial_hashes['build-mp-shading/shaders/world.frag.spv'], W_encoding_division=native_lines,
            limit='CPU disassembly of this retained build artifact. SPIR-V retains FDiv; final driver lowering/execution is not measured here, and no binary/capture identity is inferred.'),
        expression=expression,
        expression_limit='Complete finite integer-domain binary32 encodings and ideal binary64 real-product floor/nearest diagnostics. Does not establish compiled driver lowering, attachment quantization, noninteger/negative W, triangle interpolation or physical Xbox precision.',
        source_capture_limits='Original20 Vulkan xemu and Lockout OpenGL llvmpipe are distinct frames/backends. No physical Xbox comparison and no live weather activation.',
        native_facts=dict(depth_candidates=['D32_SFLOAT', 'D24_UNORM_S8_UINT', 'D16_UNORM'],
            current_format_requirements=['DEPTH_STENCIL_ATTACHMENT', 'SAMPLED_IMAGE'],
            main_depth_usage=['DEPTH_STENCIL_ATTACHMENT', 'SAMPLED'], transfer_source=False,
            sampled_view_aspect='DEPTH', stencil_load='DONT_CARE', stencil_store='DONT_CARE',
            queue_requirements=['GRAPHICS', 'PRESENT'], frame_fence_reset='after command recording, immediately before submission',
            main_and_water_single_sample=True, water_extent=[512, 256], scope_extent=[64, 64],
            screenshot_copies_color_only=True),
        GPU_jobs=0, guest_IO=False, original_execution_cases=0, files=initial_hashes)
    for p in paths:
        assert digest(p) == initial_hashes[str(p.relative_to(ROOT))], f'input changed during audit: {p}'
    (OUT / 'source-depth-settings-disasm.txt').write_text('\n'.join(disassembly) + '\n')
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(report=str((OUT / 'report.json').relative_to(ROOT)),
                          sha256=digest(OUT / 'report.json'), expression=expression)))


if __name__ == '__main__':
    main()
