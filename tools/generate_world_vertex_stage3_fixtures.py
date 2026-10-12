#!/usr/bin/env python3
"""Read-only source mapping and uncompiled Stage3 VS replacement adapters.

No original instructions, Vulkan work, compilation or replay is performed.
The adapters execute the current native interpreter using literal resident
class1 tokens, original Vulkan input locations/formats and the original bound
uniform layout. They isolate arithmetic from CPU-decoding/class0 adaptation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_rendering import Cache

BASE = ROOT / 'analysis/frontend/title-camera-matrix-audit/original-world-depth'
DEFAULT_OUT = ROOT / 'analysis/frontend/title-camera-matrix-audit/world-vertex-stage3'
CAPTURE_SHA = '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431'
PROGRAMS = {1335: 0xe1cc0057, 1448: 0xe1dd0068,
            1880: 0xe1ba0045, 2409: 0xe1ba0045}
MAC = ('NOP', 'MOV', 'MUL', 'ADD', 'MAD', 'DP3', 'DPH', 'DP4',
       'DST', 'MIN', 'MAX', 'SLT', 'SGE', 'ARL')
ILU = ('NOP', 'MOV', 'RCP', 'RCC', 'RSQ', 'EXP', 'LOG', 'LIT')


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def save(out, name, raw):
    (out / name).write_bytes(raw)
    return {'file': name, 'bytes': len(raw), 'sha256': sha(raw)}


def checked_original(item):
    raw = (BASE / item['file']).read_bytes()
    if len(raw) != item['byte_size'] or sha(raw) != item['sha256']:
        raise ValueError('original extracted file differs: ' + item['file'])
    return raw


def program(cache, datum, classification):
    tag, root = cache.tag(datum, 'vrtx')
    at = cache.block(root + 4, 28)[classification]
    size = cache.u32(at + 12)
    start = cache.pointer(cache.u32(at + 16), cache.magic, size)
    raw = bytes(cache.data[start:start + size])
    header, count = struct.unpack_from('<2H', raw)
    if header != 0x2078 or count > 136 or size != 4 + count * 16:
        raise ValueError('unexpected original compiled token record')
    dcount = cache.u32(at + 4)
    dstart = cache.pointer(cache.u32(at + 8), cache.magic, dcount * 2)
    declaration = list(struct.unpack_from('<' + 'H' * dcount, cache.data, dstart))
    sequence, inputs, constants = [], set(), set()
    words = list(struct.iter_unpack('<4I', raw[4:]))
    for _, y, z, w in words:
        m, i = (y >> 21) & 15, (y >> 25) & 7
        if m >= len(MAC):
            raise ValueError('unsupported original MAC')
        if m not in (0, 1):
            sequence.append('_' + MAC[m])
            # Xemu lowers a simultaneous temp/output MAC as two calls to its
            # same helper. Native calculates m once then writes both targets.
            if (w >> 24) & 15 and (w >> 12) & 15 and not w & 4:
                sequence.append('_' + MAC[m])
        if i not in (0, 1):
            sequence.append('_' + ILU[i])
        active = (m != 0, m in (2, 4, 5, 6, 7, 8, 9, 10, 11, 12),
                  i != 0 or m in (3, 4))
        muxes = ((z >> 26) & 3, (z >> 11) & 3, (w >> 28) & 3)
        for used, mux in zip(active, muxes):
            if used and mux == 2:
                inputs.add((y >> 9) & 15)
            if used and mux == 3:
                if w & 2:
                    raise ValueError('relative constants outside bounded adapter')
                constants.add((y >> 13) & 255)
    return raw, {'datum': f'{datum:08x}', 'name': tag['name'],
                 'classification': classification, 'instructions': count,
                 'declaration': declaration, 'read_inputs': sorted(inputs),
                 'read_constants': sorted(constants), 'expanded_helpers': sequence}, words


def adapter(words, reads, mode, interpreter):
    if not set(reads) <= {0, 1, 2, 3, 4}:
        raise ValueError('adapter cannot supply a consumed constant attribute')
    text = '''#version 450
// Diagnostic only; original inputs/constants, current native interpreter.
layout(location=0) in vec4 v0;
'''
    if 1 in reads:
        text += 'layout(location=1) in vec4 v1;\n'
    for i in (2, 3, 4):
        if i in reads:
            text += f'layout(location={i}) in int v{i}_cmp;\n'
    text += '''layout(set=0,binding=0,std140) uniform VshUniforms {
    vec4 c[192];
    vec4 clipRange;
    vec2 fogParam;
    layout(offset=5336) vec2 surfaceSize;
} source;
layout(location=0) out vec4 v_vtxD0;
layout(location=1) out vec4 v_vtxD1;
layout(location=2) out vec4 v_vtxB0;
layout(location=3) out vec4 v_vtxB1;
layout(location=4) out float v_vtxFog;
layout(location=5) out vec4 v_vtxT0;
layout(location=6) out vec4 v_vtxT1;
layout(location=7) out vec4 v_vtxT2;
layout(location=8) out vec4 v_vtxT3;
layout(location=9) flat out vec4 v_vtxPos0;
layout(location=10) flat out vec4 v_vtxPos1;
layout(location=11) flat out vec4 v_vtxPos2;
layout(location=12) flat out float v_triMZ;
struct ReferenceMaterial {
    uvec4 flags; uvec4 vertexInstructions[136]; vec4 vertexAttributes[16];
};
ReferenceMaterial material;
vec4 vpConstant(uint index) { return source.c[index]; }
#define H2_VERTEX_REFERENCE_INPUTS
#define H2_VERTEX_REFERENCE_CONSTANTS
#define H2_EXTENDED_VERTEX_BANK
'''
    if mode != 'precise-source-epilogue':
        text += '#define H2_VERTEX_XEMU_ARITHMETIC\n'
    # Inline the unchanged includes, so owner compilation needs no search paths.
    includes = {'render_view.glsl': (ROOT / 'shaders/render_view.glsl').read_text(),
                'source_vertex_inputs.glsl': (ROOT / 'shaders/source_vertex_inputs.glsl').read_text()}
    for name, body in includes.items():
        interpreter = interpreter.replace(f'#include "{name}"', body)
    text += interpreter + '''
float sourceClampW(float t) {
    return t>0.0 || floatBitsToUint(t)==0u
        ? clamp(t,uintBitsToFloat(528482304u),uintBitsToFloat(1602224128u))
        : clamp(t,uintBitsToFloat(3749707776u),uintBitsToFloat(2675965952u));
}
vec4 sourcePackedNormal(int p) {
    return vec4(float(bitfieldExtract(p,0,11))/1023.0,
                float(bitfieldExtract(p,11,11))/1023.0,
                float(bitfieldExtract(p,22,10))/511.0,1.0);
}
vec4 sourceColor(vec4 value) {
    return clamp(mix(value,vec4(1.0),isnan(value)),0.0,1.0);
}
void main() {
'''
    text += f'    material.flags=uvec4(0,0,1,{len(words)});\n'
    text += '    for(int i=0;i<16;++i)material.vertexAttributes[i]=vec4(0,0,0,1);\n'
    for i in reads:
        value = f'sourcePackedNormal(v{i}_cmp)' if i >= 2 else f'v{i}'
        text += f'    material.vertexAttributes[{i}]={value};\n'
    for i, values in enumerate(words):
        text += f'    material.vertexInstructions[{i}]=uvec4(' + ','.join(f'0x{v:08x}u' for v in values) + ');\n'
    text += '''    runVertexProgram();
    vec4 screen=vpOut[0];
    screen.xy=trunc(screen.xy*16.0)/16.0;
'''
    if mode == 'xemu-native-epilogue':
        text += '''    screen.w=(screen.w<0.0?-1.0:1.0)*clamp(abs(screen.w),5.421010862427522e-20,1.8446744073709552e19);
    vec2 ndc=screen.xy*2.0/source.surfaceSize-1.0;
    gl_Position=vec4(ndc*screen.w,(screen.z/16777215.0)*screen.w,screen.w);
'''
    else:
        text += '''    screen.w=sourceClampW(screen.w);
    vec4 clip=screen;
    clip.xy=(2.0*clip.xy-source.surfaceSize)/source.surfaceSize;
    clip.z=clip.z/source.clipRange.y;
    clip.xyz=clip.xyz*clip.w;
    gl_Position=clip;
'''
    text += '''    gl_PointSize=1.0;
    v_vtxD0=sourceColor(vpOut[3]);v_vtxD1=sourceColor(vpOut[4]);
    v_vtxB0=sourceColor(vpOut[7]);v_vtxB1=sourceColor(vpOut[8]);
    float distance=vpOut[5].x;
    float fog=source.fogParam.x+distance*source.fogParam.y;
    fog=fog-1.0;
    v_vtxFog=isinf(distance)?1.0:clamp(isnan(fog)?1.0:fog,-uintBitsToFloat(2139095039u),uintBitsToFloat(2139095039u));
    v_vtxT0=vpOut[9];v_vtxT1=vpOut[10];v_vtxT2=vpOut[11];v_vtxT3=vpOut[12];
    v_vtxPos0=screen;v_vtxPos1=screen;v_vtxPos2=screen;v_triMZ=0.0;
}
'''
    return text.encode()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    original = json.loads((BASE / 'original-world-depth.json').read_text())
    if original['status'] != 'passed' or original['failures'] or original['capture_sha256'] != CAPTURE_SHA:
        raise ValueError('original stage authority invalid')
    interpreter = (ROOT / 'shaders/vertex_program.glsl').read_text()
    report = {'status': 'passed', 'scope': __doc__, 'capture_sha256': CAPTURE_SHA,
              'gpu_validation_run': False, 'native_production_input_adaptation_verified': False,
              'adapters_compiled': False,
              'native_shader_sha256': sha(interpreter.encode()), 'passes': []}
    cache = Cache(ROOT / 'assets/disc/maps/mainmenu.map')
    report['cache'] = {'file': str(cache.path.relative_to(ROOT)),
                       'bytes': len(cache.data), 'sha256': sha(cache.data)}
    try:
        for p in original['passes']:
            event = p['event']
            raw, info, words = program(cache, PROGRAMS[event], 1)
            world_raw, world_info, _ = program(cache, PROGRAMS[event], 0)
            source_shader = checked_original(p['stages']['0']['spirv'])
            source_text = (BASE / p['stages']['0']['disassembly']).read_text()
            main_text = source_text[source_text.index('void main()'):]
            main_text = main_text[:main_text.index('\nfloat4 decompress') if '\nfloat4 decompress' in main_text else main_text.index('\nfloat clampAway')]
            original_helpers = re.findall(r'= (_[A-Z0-9]+)\(', main_text)
            if original_helpers != info['expanded_helpers']:
                raise ValueError(f'event{event}: token sequence does not match original helper lowering')
            source_constants = sorted(set(map(int, re.findall(r'VshUniforms_var.c\[(\d+)\]', main_text))))
            if source_constants != info['read_constants']:
                raise ValueError(f'event{event}: consumed token constants do not match original shader')
            bound = checked_original(p['stages']['0']['bound_blocks'][0]['raw'])
            row = {'event': event, 'role': p['role'], 'original_vertex_spirv_sha256': sha(source_shader),
                   'original_program': info, 'native_world0_program': world_info,
                   'helper_sequence_match': True, 'NoContraction_decorations': source_text.count('NoContraction'),
                   'original_Dot_instructions': len(re.findall(r'Dot\(', source_text)),
                   'source_shader_consumed_constants': source_constants,
                   'source_uniforms': save(out, f'event-{event}-uniforms.bin', bound),
                   'constants192': save(out, f'event-{event}-constants192.bin', bound[:3072]),
                   'program_source1': save(out, f'event-{event}-source1-program.bin', raw),
                   'program_world0': save(out, f'event-{event}-world0-program.bin', world_raw),
                   'source_attributes': p['vertex_attributes'], 'source_vertex_input': p['vertex_input'],
                   'source_topology': p['input_assembly']['topology'], 'streams': [], 'adapters': {}}
            post_gs = checked_original(p['post_stage_buffers']['2']['raw'])
            w_values = [struct.unpack_from('<f', post_gs, i + 12)[0]
                        for i in range(0, len(post_gs), 204)]
            row['rendered_gl_Position_W'] = {
                'minimum': min(w_values), 'maximum': max(w_values),
                'zero_count': sum(w == 0 for w in w_values),
                'negative_count': sum(w < 0 for w in w_values),
                'scope': 'Actual post-GS rendered records, not an independently instrumented VSOut stream'}
            n = p['action']['numIndices']
            index_item = next(s for s in p['source_buffers'] if s['label'] == 'index')
            index_raw = checked_original(index_item['complete_bound_tail'])[:n * 4]
            indices = struct.unpack('<' + 'I' * n, index_raw)
            maximum = max(indices) + p['action']['baseVertex']
            row['indices'] = save(out, f'event-{event}-indices.bin', index_raw)
            row['index_count'] = n
            row['unique_vertex_count'] = len(set(indices))
            row['maximum_source_vertex'] = maximum
            for attr in p['vertex_attributes']:
                binding = attr['vertexBuffer']
                item = next(s for s in p['source_buffers'] if s['label'] == f'vertex-{binding}')
                # Preserve Vulkan fetch conversion (especially SNORM16) rather
                # than substituting a Python/CPU normalization as GPU authority.
                stream = checked_original(item['complete_bound_tail'])
                needed = maximum * item['binding']['byteStride'] + attr['byteOffset'] + attr['format']['compByteWidth'] * attr['format']['compCount']
                if needed > len(stream):
                    raise ValueError('source stream too short')
                row['streams'].append({'attribute': attr['name'], 'binding': binding,
                                       'original_binding': item['binding'],
                                       'raw': save(out, f'event-{event}-vertex-{binding}.bin', stream[:needed])})
            for mode in ('precise-source-epilogue', 'xemu-source-epilogue', 'xemu-native-epilogue'):
                row['adapters'][mode] = save(out, f'event-{event}-{mode}.vert', adapter(words, info['read_inputs'], mode, interpreter))
            row['boundary'] = ('Matching helper sequence plus exact resident tokens is a reproducible program mapping, not GPU execution proof. '
                               'Adapters keep class1/actual source streams. Production class0 CPU-decoded vertices/object transforms remain a separate comparison.')
            report['passes'].append(row)
    finally:
        cache.close()
    report['limits'] = [
        'No compiler, GPU replay, source instruction execution or production mutation occurred.',
        'Adapters use the native interpreter RCC and RSQ, including retained differences at zero/NaN boundaries.',
        'Native epilogue adapter is bounded to captured viewport origin(0,0)/D24 maximum; it is not an aspect extension proof.',
        'RenderDoc post-VS instrumentation differs from the values consumed by GS; compare actual post-GS fields and attachment bytes.',
        'Original shader resource replacement affects earlier uses; owner replay must report cumulative-history boundaries.'
    ]
    (out / 'world-vertex-stage3.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'Four original programs mapped; twelve uncompiled adapters retained in {out}')


if __name__ == '__main__':
    main()
