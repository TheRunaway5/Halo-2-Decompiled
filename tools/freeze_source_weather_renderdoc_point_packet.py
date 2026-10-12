#!/usr/bin/env python3
"""Freeze a native comparison packet from exported original GL event215873.

Reads already saved RenderDoc exports; does not access a guest, display, replay
controller or GPU. Captured float colors must be exact float32(byte/255) words.
The old source-proved39780 definition is retained only after the captured pixel
body matches its modulate/final-combiner semantics. This does not recover unique
retail pixel-definition words from a compiled GLSL shader.
"""
from pathlib import Path
import argparse
import collections
import ctypes
import hashlib
import json
import math
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CAPTURE = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1'
SOURCE = CAPTURE / 'source-snow-event215873'
OLD = ROOT / 'analysis/multiplayer-shading/weather-particle-packet'
OUT = ROOT / 'analysis/multiplayer-shading/weather-point-original-reference-event215873'
PINNED_XEMU = ROOT / 'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'
COMMIT = 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee'


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def record(path):
    raw = path.read_bytes()
    return {'path': str(path.relative_to(ROOT)), 'bytes': len(raw), 'sha256': digest(raw)}


def freeze(path, raw):
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        assert path.read_bytes() == raw, f'immutable output differs: {path}'
    else:
        path.write_bytes(raw)


def words(variable):
    assert variable['rows'] == 1 and not variable['members']
    return variable['value']['u32v'][:variable['columns']]


def floats(raw):
    return struct.unpack('<' + 'f' * (len(raw) // 4), raw)


def rounded(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def make_decoder(directory):
    tree = json.loads((PINNED_XEMU / 'tree.json').read_text())
    assert tree['sha'] == COMMIT and not tree['truncated']
    blobs = {row['path']: row['sha'] for row in tree['tree'] if row['type'] == 'blob'}
    sources = []
    for name in ('s3tc.c', 's3tc.h'):
        path = PINNED_XEMU / 'hw/xbox/nv2a/pgraph' / name
        raw = path.read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        assert blob == blobs[str(path.relative_to(PINNED_XEMU))]
        sources.append(record(path))
    include = directory / 'decoder/qemu/osdep.h'
    freeze(include, b'#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n'
                   b'#include <stdlib.h>\n#define g_malloc malloc\n'
                   b'#define MIN(a,b) ((a)<(b)?(a):(b))\n')
    library = directory / 'decoder/original-s3tc.so'
    if not library.exists():
        subprocess.run(['cc', '-std=c11', '-O2', '-shared', '-fPIC', '-I', str(include.parent.parent),
                        sources[0]['path'], '-o', str(library)], cwd=ROOT, check=True)
    lib = ctypes.CDLL(str(library))
    lib.s3tc_decompress_2d.argtypes = [ctypes.c_int, ctypes.c_void_p, ctypes.c_uint, ctypes.c_uint]
    lib.s3tc_decompress_2d.restype = ctypes.c_void_p
    free = ctypes.CDLL(None).free
    free.argtypes = [ctypes.c_void_p]

    def decode(raw, width):
        block = ctypes.create_string_buffer(raw)
        pointer = lib.s3tc_decompress_2d(2, block, width, width)
        assert pointer
        try:
            return ctypes.string_at(pointer, width * width * 4)
        finally:
            free(pointer)
    return decode, sources + [record(include), record(library)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=SOURCE)
    parser.add_argument('--output', type=Path, default=OUT)
    parser.add_argument('--range-proof', type=Path, required=True)
    args = parser.parse_args()
    source, output = args.source.resolve(), args.output.resolve()
    original = json.loads((source / 'draw.json').read_text())
    assert original['event'] == 215873 and original['topology'] == 1
    draw_range = json.loads(args.range_proof.read_text())
    assert draw_range['eventId'] == 215873 and draw_range['numIndices'] == 1015
    assert draw_range['vertexOffset'] == draw_range['indexOffset'] == draw_range['instanceOffset'] == 0
    assert draw_range['numInstances'] == 1 and draw_range['baseVertex'] == 0 and draw_range['flags'] == 2
    assert original['vertex_input']['topology'] == 1
    enabled = [(i, item) for i, item in enumerate(original['vertex_input']['attributes']) if item['enabled']]
    assert [i for i, item in enabled] == [0, 1]
    for index, item in enabled:
        assert item['boundShaderInput'] == item['vertexBufferSlot'] == index and item['byteOffset'] == 0
        assert item['format'] == {'compByteWidth': 4, 'compCount': 4, 'compType': 1, 'type': 0}
        buffer = original['vertex_input']['vertexBuffers'][index]
        assert buffer['byteOffset'] == buffer['instanceDivisor'] == 0 and buffer['byteStride'] == 16
    v0, v1 = [(source / f'vertex-buffer-{i}.bin').read_bytes() for i in range(2)]
    assert len(v0) == len(v1) == 1015 * 16
    lookup = {struct.unpack('<I', struct.pack('<f', byte / 255))[0]: byte for byte in range(256)}
    assert len(lookup) == 256
    typed = bytearray()
    colors = collections.Counter()
    for index, color in enumerate(struct.iter_unpack('<4I', v1)):
        assert all(component in lookup for component in color), f'non-exact byte color at row{index}'
        r, g, b, a = [lookup[component] for component in color]
        packed = a << 24 | r << 16 | g << 8 | b
        colors[f'{packed:08X}'] += 1
        typed += v0[index * 16:index * 16 + 16] + struct.pack('<I', packed)
    assert all(math.isfinite(value) for value in floats(v0))

    vertex_text, fragment_text = [(source / (label + '.glsl')).read_text() for label in ('vertex', 'fragment')]
    slots = re.findall(r'/\* Slot (\d+): ((?:0x[0-9A-Fa-f]{8} ?){4}) \*/', vertex_text)
    assert [int(index) for index, row in slots] == list(range(9))
    tokens = b''.join(struct.pack('<4I', *(int(word, 16) for word in row.split())) for index, row in slots)
    old_program = (OLD / 'source-vs22-class0.vp').read_bytes()
    assert len(old_program) == 148 and old_program[4:] == tokens
    assert digest(old_program) == '3f9e4e6030626339db80d3bb5bf9e4103936041f9bdcfd75626f18ca2ff1a1ee'
    # Exact executed combiner body corresponds to the source-proved39780
    # definition. The copied words retain their separate original-call proof.
    combiner = fragment_text.split('vec4 t0 =', 1)[1].split('int fragAlpha', 1)[0]
    known = ''' vec4(0.0, 0.0, 0.0, 1.0); /* PS_TEXTUREMODES_NONE */
vec4 t1 = vec4(0.0, 0.0, 0.0, 1.0); /* PS_TEXTUREMODES_NONE */
vec4 t2 = vec4(0.0, 0.0, 0.0, 1.0); /* PS_TEXTUREMODES_NONE */
vec4 t3 = textureProj(texSamp3, (pT3.xyw));
vec4 r0 = vec4(0);
r0.a = 1.0;
// Stage 0
ab.rgb = clamp(vec3((v0.rgb * t3.rgb)), -1.0, 1.0);
ab.a = clamp(((v0.a * t3.a)), -1.0, 1.0);
r0.rgb = ab.rgb;
r0.a = ab.a;
// Final Combiner
fragColor.rgb = max(vec4(0.0).rgb, 0.0) + mix(vec3(max(vec4(0.0).rgb, 0.0)), vec3((1.0 - clamp(vec4(0.0).rgb, 0.0, 1.0))), vec3(max(r0.rgb, 0.0)));
fragColor.a = max(r0.a, 0.0);
'''
    assert combiner == known
    assert 'int fragAlpha = int(round(fragColor.a * 255.0));\nif (!(fragAlpha >= alphaRef)) discard;' in fragment_text
    assert 'vec4 pT3 = vec4(gl_PointCoord, 1.0, 1.0);' in fragment_text
    assert '/*  Window-clip (Inclusive) */' in fragment_text
    pixel = (OLD / 'pixel_definition_39780.bin').read_bytes()
    assert len(pixel) == 240 and digest(pixel) == 'ea5ef59b00ca319f27866926ecdef28b61c919296a2ee28910c1173f9768549b'

    vertex_values = {item['name']: item for item in original['vertex_constant_values_0']}
    fragment_values = {item['name']: item for item in original['fragment_constant_values_0']}
    constants = vertex_values['c']['members'][:12]
    assert [item['name'] for item in constants] == [f'c[{i}]' for i in range(12)]
    camera = struct.pack('<48I', *(word for item in constants for word in words(item)))
    clip_words = words(vertex_values['clipRange'])
    assert clip_words == words(fragment_values['clipRange']) == [0, 0x4B7FFFFF, 0x437FFFFF, 0x4B7FFFFF]
    clip = struct.pack('<4I', *clip_words)
    surface = struct.pack('<2I', *words(vertex_values['surfaceSize']))
    assert floats(surface) == (640., 480.)
    assert words(fragment_values['alphaRef']) == [0]
    assert words(fragment_values['depthFactor']) == words(fragment_values['depthOffset']) == [0]
    assert words(fragment_values['surfaceScale']) == [1, 1]
    regions = [item['value']['s32v'][:4] for item in fragment_values['clipRegion']['members']]
    assert regions == [[0, 0, 641, 481]] * 8
    assert original['viewport'] == {'enabled': True, 'height': 480., 'maxDepth': 1., 'minDepth': 0., 'width': 640., 'x': 0., 'y': 0.}
    assert original['scissor'] == {'enabled': True, 'height': 480, 'width': 640, 'x': 0, 'y': 0}
    assert original['depth']['depthEnable'] and original['depth']['depthFunction'] == 3 and not original['depth']['depthWrites']
    assert not original['stencil']['stencilEnable']
    raster = original['rasterizer']['state']
    assert raster['depthClamp'] and raster['pointOriginUpperLeft'] and raster['programmablePointSize']
    assert raster['depthBias'] == raster['slopeScaledDepthBias'] == 0
    blend = original['framebuffer']['blendState']['blends'][0]
    assert blend['enabled'] and not blend['logicOperationEnabled'] and blend['writeMask'] == 7
    assert blend['colorBlend'] == blend['alphaBlend'] == {'destination': 7, 'operation': 0, 'source': 6}
    sprite_binding = original['fragment_resources'][0]
    assert sprite_binding['descriptor']['numMips'] == 4 and sprite_binding['descriptor']['resource'] == 'ResourceId::57768'
    sampler = sprite_binding['sampler']
    assert sampler['addressU'] == sampler['addressV'] == sampler['addressW'] == 0
    assert sampler['filter']['magnify'] == sampler['filter']['minify'] == sampler['filter']['mip'] == 2
    assert sampler['mipBias'] == 0 and sampler['minLOD'] == -1000 and sampler['maxLOD'] == 1000

    raw_sprite = (OLD / 'sprite-resident-physical-partition.bin').read_bytes()
    assert len(raw_sprite) == 1408 and digest(raw_sprite) == '6f5c36ca1c5f8a8ab02b1472e546d1070ef8b069e58122beba4ead51bc5a1133'
    decode, decoder_records = make_decoder(output)
    mip_checks = []
    for index, offset in enumerate((0, 1024, 1280, 1344)):
        width = 32 >> index
        rgba = decode(raw_sprite[offset:], width)
        assert rgba == (source / f'sprite-mip-{index}.rgba').read_bytes()
        mip_checks.append({'level': index, 'width': width, 'height': width,
                           'source_BC3_offset': offset, 'RGBA_bytes_exact': len(rgba), 'sha256': digest(rgba)})
    before, after = [(source / (label + '-scene-color.rgba')).read_bytes() for label in ('before', 'after')]
    assert len(before) == len(after) == 640 * 480 * 4
    assert before[3::4] == after[3::4]
    changed = [(index // 4 % 640, index // 4 // 640) for index in range(0, len(before), 4)
               if before[index:index+4] != after[index:index+4]]
    assert len(changed) == 1255
    assert (source / 'before-scene-depth.raw').read_bytes() == (source / 'after-scene-depth.raw').read_bytes()
    depth_proof = json.loads((source / 'depth-upload.json').read_text())
    source_depth = (source / 'before-scene-depth.raw').read_bytes()
    native_depth = (source / depth_proof['native_file']).read_bytes()
    assert depth_proof['event'] == 215873 and depth_proof['raw_rows_preserved']
    assert not depth_proof['float_depth_substitution'] and depth_proof['before_after_depth_identical']
    assert digest(source_depth) == depth_proof['original_depth_sha256']
    assert digest(native_depth) == depth_proof['native_depth_sha256']
    assert digest((source / 'surfaces.json').read_bytes()) == depth_proof['surface_metadata_sha256']
    assert len(source_depth) == len(native_depth) == 640 * 480 * 4
    assert all(original_word >> 8 == native_word for (original_word,), (native_word,)
               in zip(struct.iter_unpack('<I', source_depth), struct.iter_unpack('<I', native_depth)))
    artifacts = {'points-v0-logical-argb.bin': bytes(typed), 'vertex_constants_c0_c11.bin': camera,
                 'source-vs22-class0.vp': old_program, 'source-compatible-pixel-definition-39780.bin': pixel,
                 'clip_range.bin': clip, 'surface_size.bin': surface,
                 'window_clip_regions.bin': struct.pack('<32i', *(x for region in regions for x in region)),
                 'sprite-resident-physical-partition.bin': raw_sprite,
                 'before-scene-color.rgba': before, 'after-scene-color.rgba': after,
                 'before-depth-d24-low24.bin': native_depth}
    for name, raw in artifacts.items():
        freeze(output / name, raw)
    dependencies = [source / name for name in ('draw.json', 'vertex.glsl', 'fragment.glsl', 'vertex-buffer-0.bin',
                    'vertex-buffer-1.bin', 'surfaces.json', 'depth-upload.json', 'before-depth-d24-low24.bin',
                    'before-scene-color.rgba', 'after-scene-color.rgba',
                    'before-scene-depth.raw', 'after-scene-depth.raw', *(f'sprite-mip-{i}.rgba' for i in range(4)))]
    dependencies += [args.range_proof.resolve(), OLD / 'manifest.json', OLD / 'packet.json',
                     OLD / 'source-vs22-class0.vp', OLD / 'pixel_definition_39780.bin',
                     OLD / 'sprite-resident-physical-partition.bin', CAPTURE / 'original-lockout-gl-source_capture.rdc',
                     ROOT / 'tools/freeze_source_weather_depth_reference.py']
    metadata = {'authority': __doc__, 'event': 215873, 'point_count': 1015, 'draw_range': draw_range,
        'typed_input_layout': '20-byte little-endian row: four original v0 float32 words then logical ARGB uint32',
        'color_recovery': 'All4060 component words exactly match float32(byte/255); zero tolerance; position bytes unchanged.',
        'logical_color_counts': dict(colors), 'source_program_match': 'Allnine literal token rows equal source-proved VS22/class0 payload.',
        'pixel_definition_scope': 'Source-proved39780 words reused after exact executed combiner-body match; GLSL is not a unique reconstruction of title-side60words.',
        'raster': {'clip_range_words': [f'{word:08X}' for word in clip_words], 'surface_size': [640, 480],
            'alpha_function': 'rounded8bit GEQUAL', 'alpha_reference': 0, 'depth_clipping': True,
            'depth_clamp': True, 'depth_compare': 'LEQUAL', 'depth_writes': False, 'depth_factor': 0,
            'depth_offset': 0, 'stencil_enabled': False, 'RGB_write_mask': 7, 'point_origin_upper_left': True,
            'programmable_point_size': True, 'viewport': original['viewport'], 'scissor': original['scissor'],
            'window_clip': {'inclusive': True, 'regions': regions}, 'blend': blend, 'sampler': sampler},
        'sprite': {'raw_original_BC3_partition': record(OLD / 'sprite-resident-physical-partition.bin'),
                   'source_decoder_commit': COMMIT, 'mips': mip_checks, 'all5440_RGBA_bytes_exact': True},
        'observed_draw_effect': {'changed_color_pixels': len(changed), 'bounds': [min(x for x,y in changed), min(y for x,y in changed),
                max(x for x,y in changed), max(y for x,y in changed)], 'alpha_bytes_unchanged': True, 'raw_depth_bytes_unchanged': True},
        'depth_upload': depth_proof,
        'presentation_limits': ['Original reference is xemuOpenGL llvmpipe, not physicalXbox.',
            'Scene and sprite exports retain RenderDoc GL raw row order; no presentation flip applied.',
            'Point-coordinate/raster precision and hardware BC3 filtering require measured native comparison.',
            'Only the captured640x480 event is a matched scene reference; no4K or split-view reference is inferred.',
            'D24 depth integers are repacked using the separately proved nine originalPickPixel controls; raw row order is unchanged. NoD32 substitution.',
            'No live native weather producer, timing, residency, simulation or map-wide scene parity is activated.'],
        'GPU_operations': 0, 'guest_access': False, 'production_activation': False,
        'artifacts': {name: record(output / name) for name in artifacts},
        'dependencies': [record(path) for path in dependencies] + decoder_records,
        'tool': record(Path(__file__).resolve())}
    freeze(output / 'packet.json', (json.dumps(metadata, indent=2, allow_nan=False) + '\n').encode())
    freeze(output / 'manifest.json', (json.dumps({'packet': record(output / 'packet.json'),
        'artifacts': metadata['artifacts'], 'dependencies': metadata['dependencies'], 'tool': metadata['tool']}, indent=2) + '\n').encode())
    print(json.dumps({'packet': record(output / 'packet.json'), 'points': 1015, 'color_components_exact': 4060,
                      'sprite_RGBA_bytes_exact': 5440, 'original_changed_pixels': len(changed)}, indent=2))


if __name__ == '__main__':
    main()
