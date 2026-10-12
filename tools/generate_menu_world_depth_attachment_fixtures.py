#!/usr/bin/env python3
"""Prepare immutable ordinary-world depth attachment fixtures, offline.

Uses only the Python standard library. No shader compilation, replay, Vulkan
device, emulator, or production edit is performed. Expected depth/stencil planes
come from the original capture, never from the candidate depth implementation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
AUDIT = ROOT / 'analysis/frontend/title-camera-matrix-audit'
SOURCE_SHA = '05f0af96e923f3efdf1bf0e78fb3eff742769dc9bbe644f76c7984c8b0695c17'
PACKED_SHA = 'e9ec98f370f4ca1b77472dc0109c59025690a0b93f2d62c8a43508ed52d78281'
CAPTURE_SHA = '077524887d852e46af2e3cb8eb85c52dd777b8bda3e86039c29e904a2450b431'
EVENTS = (1335, 1448, 1880, 2409)
MASK_STORE_WORDS = {1335: 3792, 1448: 3883, 1880: 4310, 2409: 4310}
STATE_WORDS = [
    'magic', 'width', 'height', 'record_count', 'vk_cull_mode', 'vk_front_face',
    'depth_clamp_enable', 'depth_test_enable', 'depth_write_enable', 'vk_depth_compare',
    'stencil_test_enable',
    'front_vk_fail', 'front_vk_pass', 'front_vk_depth_fail', 'front_vk_compare',
    'front_compare_mask', 'front_write_mask', 'front_reference',
    'back_vk_fail', 'back_vk_pass', 'back_vk_depth_fail', 'back_vk_compare',
    'back_compare_mask', 'back_write_mask', 'back_reference', 'sampler_cube_mask',
    'depth_bias_enable', 'depth_bias_f32', 'depth_bias_clamp_f32', 'slope_bias_f32',
    'vk_sample_count', 'viewport_x_f32', 'viewport_y_f32', 'viewport_width_f32',
    'viewport_height_f32', 'viewport_min_depth_f32', 'viewport_max_depth_f32',
    'scissor_x_i32', 'scissor_y_i32', 'scissor_width', 'scissor_height',
    'depth_bounds_enable', 'min_depth_bounds_f32', 'max_depth_bounds_f32',
]
assert len(STATE_WORDS) == 44


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def pinned(path, identity):
    raw = path.read_bytes()
    require(sha(raw) == identity, 'source identity differs: ' + str(path))
    return raw


def source_item(directory, item):
    raw = pinned(directory / item['file'], item['sha256'])
    require(len(raw) == item['byte_size'], 'source length differs: ' + item['file'])
    return raw


def save(directory, name, raw):
    path = directory / name
    if path.exists():
        require(path.read_bytes() == raw, 'refusing to replace a different retained fixture: ' + str(path))
    else:
        path.write_bytes(raw)
    return dict(file=name, bytes=len(raw), sha256=sha(raw))


class Spirv:
    def __init__(self, raw):
        require(len(raw) % 4 == 0, 'unaligned SPIR-V')
        self.words = list(struct.unpack('<' + 'I' * (len(raw) // 4), raw))
        require(self.words[0] == 0x07230203 and self.words[1] == 0x00010600,
                'expected retained SPIR-V1.6')
        self.instructions, self.names, self.definitions, self.decorations = [], {}, {}, {}
        at, function = 5, None
        while at < len(self.words):
            size, op = self.words[at] >> 16, self.words[at] & 0xffff
            require(size != 0 and at + size <= len(self.words), 'malformed SPIR-V instruction')
            args = self.words[at + 1:at + size]
            if op == 54:
                function = args[1]
            self.instructions.append((at, op, args, function))
            if op == 5:
                self.names[args[0]] = struct.pack('<' + 'I' * (len(args) - 1), *args[1:]).split(b'\0')[0].decode()
            if op == 71:
                self.decorations.setdefault(args[0], {})[args[1]] = args[2:]
            if op in (19, 20, 21, 22, 23, 25, 27, 28, 30, 32):
                self.definitions[args[0]] = (at, op, args)
            if op in (12, 41, 42, 43, 44, 46, 55, 59, 61, 65, 66, 79, 80, 81,
                      82, 83, 124, 128, 129, 131, 133, 136, 148, 169):
                self.definitions[args[1]] = (at, op, args)
            if op == 56:
                function = None
            at += size
        require(at == len(self.words), 'SPIR-V extent differs')

    def named(self, name):
        found = [i for i, n in self.names.items() if n == name]
        require(len(found) == 1, 'expected one original named value: ' + name)
        return found[0]

    def root(self, pointer):
        _, op, args = self.definitions[pointer]
        return self.root(args[2]) if op in (65, 66) else pointer

    def constant(self, value_id):
        at, op, args = self.definitions[value_id]
        require(op == 43, 'expected scalar constant')
        return at, args[0], args[2:]


def source_fragment(raw, event):
    s = Spirv(raw)
    main, color, depth, zvalue = (s.named(n) for n in ('main', 'fragColor', 'gl_FragDepth', 'zvalue'))
    require(s.decorations[color].get(30) == [0], 'fragColor must be location0')
    require(s.decorations[depth].get(11) == [22], 'depth must be BuiltIn FragDepth')
    require([(op, a[:2]) for _, op, a, _ in s.instructions if op == 15] == [(15, [4, main])],
            'expected one Fragment entry point')
    require(sorted(a[1] for _, op, a, _ in s.instructions if op == 16 and a[0] == main) == [7, 12],
            'expected OriginUpperLeft and DepthReplacing')
    require(sorted(a[0] for _, op, a, _ in s.instructions if op == 17) == [1, 5379],
            'expected Shader and DemoteToHelperInvocation capabilities')
    require(not any(op in (252, 4416) for _, op, _, _ in s.instructions),
            'texture/color rejection cannot be admitted with Kill/TerminateInvocation')

    # Find the red component structurally: one Output location0 vec4, scalar
    # AccessChain with literal integer0, one store to that exact pointer.
    _, op, variable = s.definitions[color]
    require(op == 59 and variable[2] == 3, 'fragColor is not an Output variable')
    _, _, pointer_type = s.definitions[variable[0]]
    _, vector_op, vector = s.definitions[pointer_type[2]]
    require(vector_op == 23 and vector[2] == 4, 'fragColor is not vec4')
    float_type = vector[1]
    require(s.definitions[float_type][1:] == (22, [float_type, 32]), 'expected float32 color')
    one_at, one_type, one_bits = s.constant(199)
    require(one_type == float_type and one_bits == [0x3f800000], 'original float-one ID199 differs')
    red_pointers = []
    for _, op, a, _ in s.instructions:
        if op in (65, 66) and a[2] == color:
            require(len(a) == 4, 'unsupported fragColor access chain')
            _, component_type, component = s.constant(a[3])
            require(s.definitions[component_type][1] == 21, 'color index is not an integer')
            if component == [0]:
                red_pointers.append(a[1])
    require(len(red_pointers) == 1, 'expected one literal red access chain')
    red = [(at, a) for at, op, a, f in s.instructions if op == 62 and a[0] == red_pointers[0] and f == main]
    require(len(red) == 1 and red[0][0] == MASK_STORE_WORDS[event], 'retained red-store location differs')
    red_at, red_args = red[0]
    mask = s.words.copy()
    mask[red_at + 2] = 199
    differences = [i for i, (a, b) in enumerate(zip(s.words, mask)) if a != b]
    require(differences == [red_at + 2], 'mask derivative changed more than one operand')

    # Acceptance/depth proof is deliberately bounded to the three literal FS
    # modules. Color inputs and all sampled-image loads occur only after every
    # W/zvalue store and both demotions; no later code may alias or mutate W.
    variables = {a[1]: a for _, op, a, _ in s.instructions if op == 59}
    color_inputs = {i for i, a in variables.items() if a[2] == 1 and
                    s.decorations.get(i, {}).get(30, [99])[0] <= 8}
    color_loads = [(at, f) for at, op, a, f in s.instructions if op == 61 and s.root(a[2]) in color_inputs]
    require(color_loads and all(f == main for _, f in color_loads), 'unsupported color input reader')
    color_start = min(at for at, _ in color_loads)
    zstores = [(at, a, f) for at, op, a, f in s.instructions if op == 62 and a[0] == zvalue]
    require(len(zstores) == 5 and all(at < color_start and f == main for at, _, f in zstores),
            'all five zvalue stores must precede color inputs')
    require(not any(op in (65, 66) and a[2] == zvalue for _, op, a, _ in s.instructions), 'zvalue may not be aliased')
    require(not any(op == 57 and zvalue in a[3:] for _, op, a, _ in s.instructions), 'zvalue passed to a callee')
    demotions = [(at, f) for at, op, _, f in s.instructions if op == 5380]
    require(len(demotions) == 2 and all(at < color_start and f == main for at, f in demotions),
            'both demotions must precede color inputs')
    sampler_ids = {i for i, a in variables.items() if a[2] == 0}
    sample_loads = [(at, f) for at, op, a, f in s.instructions if op == 61 and s.root(a[2]) in sampler_ids]
    require(sample_loads and all(at > max(x[0] for x in zstores) and f == main for at, f in sample_loads),
            'sampled images must be read after W calculation')
    depth_stores = [(at, a, f) for at, op, a, f in s.instructions if op == 62 and a[0] == depth]
    require(len(depth_stores) == 1 and depth_stores[0][2] == main, 'expected one main FragDepth store')
    depth_at, depth_args, _ = depth_stores[0]
    chain = []
    def dependency(value_id):
        at, op, a = s.definitions[value_id]
        chain.append(dict(word=at, opcode=op, result_id=value_id, operands=a))
        if op == 61:
            require(a[2] == zvalue, 'final depth chain reads something other than earlier W')
        elif op == 43:
            pass
        elif op == 12:
            require(a[3] == 8 and len(a) == 5, 'final depth chain requires GLSL Floor only')
            dependency(a[4])
        elif op == 124:
            dependency(a[2])
        elif op in (128, 136):
            dependency(a[2]); dependency(a[3])
        else:
            raise ValueError('unsupported final depth-chain opcode ' + str(op))
    dependency(depth_args[1])
    require(sorted(x['opcode'] for x in chain) == [12, 43, 43, 61, 124, 124, 128, 136],
            'final depth quantizer structure differs')
    require(any(x['opcode'] == 43 and x['operands'][2:] == [0x4b800000] for x in chain),
            'source quantizer denominator must be float2^24')
    require(any(x['opcode'] == 43 and x['operands'][2:] == [1] for x in chain), 'source next-float add must be integer1')

    # Confirm the sole main return cannot bypass the final depth-store block.
    blocks, block = {}, None
    for at, op, a, f in s.instructions:
        if f != main:
            continue
        if op == 248:
            block = a[0]; blocks[block] = dict(successors=[], returned=False, depth_store=False)
        elif block is not None:
            if at == depth_at:
                blocks[block]['depth_store'] = True
            if op == 249:
                blocks[block]['successors'] = [a[0]]
            elif op == 250:
                blocks[block]['successors'] = a[1:3]
            elif op == 251:
                blocks[block]['successors'] = [a[1]] + a[3::2]
            elif op == 253:
                blocks[block]['returned'] = True
    returns = [i for i, b in blocks.items() if b['returned']]
    require(len(returns) == 1, 'expected one final main return')
    pending, visited = [next(iter(blocks))], set()
    while pending:
        block = pending.pop()
        if block in visited or blocks[block]['depth_store']:
            continue
        visited.add(block)
        require(not blocks[block]['returned'], 'main return bypasses source depth store')
        pending.extend(blocks[block]['successors'])

    resources = []
    for variable_id in sorted(sampler_ids):
        variable = variables[variable_id]
        _, _, pointer = s.definitions[variable[0]]
        _, sampled_op, sampled = s.definitions[pointer[2]]
        require(sampled_op == 27, 'expected combined sampled image')
        _, image_op, image = s.definitions[sampled[1]]
        require(image_op == 25 and image[2] in (1, 3) and image[3:] == [0, 0, 0, 1, 0],
                'only ordinary sampled nonarray/nonMS2D or cube admitted')
        require(s.definitions[image[1]][1:] == (22, [image[1], 32]), 'expected float image sample')
        decorations = s.decorations[variable_id]
        require(decorations.get(34) == [0] and decorations.get(33, [99])[0] in (2, 3, 4, 5), 'unexpected sampler binding')
        resources.append(dict(name=s.names[variable_id], variable_id=variable_id,
                              descriptor_set=0, binding=decorations[33][0],
                              type='COMBINED_IMAGE_SAMPLER', dimension='2D' if image[2] == 1 else 'Cube'))
    cube_mask = sum(1 << (r['binding'] - 2) for r in resources if r['dimension'] == 'Cube')
    ubo = [i for i, a in variables.items() if a[2] == 2]
    require(len(ubo) == 1 and s.decorations[ubo[0]].get(33) == [1] and s.decorations[ubo[0]].get(34) == [0],
            'expected one FS UBO at set0 binding1')
    mask_raw = struct.pack('<' + 'I' * len(mask), *mask)
    proof = dict(original_sha256=sha(raw), mask_sha256=sha(mask_raw),
                 red_store_word=red_at, changed_word=differences[0], original_value_id=red_args[1],
                 mask_value_id=199, original_one_constant_word=one_at,
                 every_other_word_identical=True, color_output_id=color, depth_output_id=depth,
                 zvalue_id=zvalue, main_id=main, first_color_input_word=color_start,
                 zvalue_store_words=[x[0] for x in zstores], demotion_words=[x[0] for x in demotions],
                 sampled_image_load_words=[x[0] for x in sample_loads],
                 no_kill_or_terminate=True, zvalue_not_aliased_or_passed=True,
                 one_main_return_dominated_by_final_depth_store=True,
                 depth_store_word=depth_at, final_depth_dependency_chain=chain,
                 resources=resources, sampler_cube_mask=cube_mask,
                 required_capabilities=['Shader', 'DemoteToHelperInvocation'],
                 required_execution_modes=['OriginUpperLeft', 'DepthReplacing'],
                 scope='Only these pinned original FS modules: texture/color cannot change demotion or final stored W.')
    return mask_raw, proof


def enum_mapping(authority, header):
    conversions = {
        'CullMode': {'NoCull': 'VK_CULL_MODE_NONE', 'Front': 'VK_CULL_MODE_FRONT_BIT',
                     'Back': 'VK_CULL_MODE_BACK_BIT', 'FrontAndBack': 'VK_CULL_MODE_FRONT_AND_BACK'},
        'FillMode': {'Solid': 'VK_POLYGON_MODE_FILL', 'Wireframe': 'VK_POLYGON_MODE_LINE', 'Point': 'VK_POLYGON_MODE_POINT'},
        'CompareFunction': {'Never': 'VK_COMPARE_OP_NEVER', 'AlwaysTrue': 'VK_COMPARE_OP_ALWAYS',
                            'Less': 'VK_COMPARE_OP_LESS', 'LessEqual': 'VK_COMPARE_OP_LESS_OR_EQUAL',
                            'Greater': 'VK_COMPARE_OP_GREATER', 'GreaterEqual': 'VK_COMPARE_OP_GREATER_OR_EQUAL',
                            'Equal': 'VK_COMPARE_OP_EQUAL', 'NotEqual': 'VK_COMPARE_OP_NOT_EQUAL'},
        'StencilOperation': {'Keep': 'VK_STENCIL_OP_KEEP', 'Zero': 'VK_STENCIL_OP_ZERO', 'Replace': 'VK_STENCIL_OP_REPLACE',
                             'IncSat': 'VK_STENCIL_OP_INCREMENT_AND_CLAMP', 'DecSat': 'VK_STENCIL_OP_DECREMENT_AND_CLAMP',
                             'IncWrap': 'VK_STENCIL_OP_INCREMENT_AND_WRAP', 'DecWrap': 'VK_STENCIL_OP_DECREMENT_AND_WRAP',
                             'Invert': 'VK_STENCIL_OP_INVERT'},
    }
    def vk(name):
        found = re.findall(r'\b' + name + r'\s*=\s*(0[xX][0-9a-fA-F]+|[0-9]+)\s*[,\n]', header)
        require(len(found) == 1, 'expected literal Vulkan header value for ' + name)
        return int(found[0], 0)
    mappings, rows = {}, []
    for kind, names in conversions.items():
        require(set(authority['classes'][kind]) == set(names), 'RenderDoc enum authority differs: ' + kind)
        mappings[kind] = {}
        for name, native in names.items():
            source = authority['classes'][kind][name]
            value = vk(native)
            mappings[kind][source] = value
            rows.append(dict(renderdoc_class=kind, renderdoc_name=name, renderdoc_value=source,
                             vulkan_name=native, vulkan_value=value))
    return mappings, rows, vk


def f32_bits(value):
    return struct.unpack('<I', struct.pack('<f', value))[0]


def state(row, width, height, records, cube_mask, mappings, vk):
    r, d, m, v = (row[k] for k in ('rasterizer', 'depth_stencil', 'multisample', 'viewport_scissor'))
    require(mappings['FillMode'][r['fillMode']] == vk('VK_POLYGON_MODE_FILL'), 'nonfill rasterization not supported')
    require(r['depthClipEnable'] is False and r['provokingVertexFirst'] is True and
            r['rasterizerDiscardEnable'] is False and r['conservativeRasterization'] == 0 and
            r['extraPrimitiveOverestimationSize'] == 0 and r['lineWidth'] == 1 and
            r['pipelineShadingRate'] == [1, 1] and r['shadingRateCombiners'] == [0, 0],
            'unsupported nonordinary raster input')
    require(m['rasterSamples'] == 1 and m['sampleMask'] == 0xffffffff and m['sampleShadingEnable'] is False and
            m['sampleLocations']['customLocations'] == [] and
            m['sampleLocations']['gridWidth'] == m['sampleLocations']['gridHeight'] == 1,
            'unsupported multisample state')
    require(v['depthNegativeOneToOne'] is False and v['discardRectangles'] == [] and
            len(v['viewportScissors']) == 1, 'unsupported viewport extensions')
    view = v['viewportScissors'][0]
    require(view['vp']['enabled'] and view['scissor']['enabled'], 'viewport/scissor must be enabled')
    values = [0x57444731, width, height, records, mappings['CullMode'][r['cullMode']],
              vk('VK_FRONT_FACE_COUNTER_CLOCKWISE' if r['frontCCW'] else 'VK_FRONT_FACE_CLOCKWISE'),
              int(r['depthClampEnable']), int(d['depthTestEnable']), int(d['depthWriteEnable']),
              mappings['CompareFunction'][d['depthFunction']], int(d['stencilTestEnable'])]
    for face in ('frontFace', 'backFace'):
        f = d[face]
        values.extend([mappings['StencilOperation'][f[k]] for k in ('failOperation', 'passOperation', 'depthFailOperation')])
        values.extend([mappings['CompareFunction'][f['function']], f['compareMask'], f['writeMask'], f['reference']])
    values += [cube_mask, int(r['depthBiasEnable']), f32_bits(r['depthBias']), f32_bits(r['depthBiasClamp']),
               f32_bits(r['slopeScaledDepthBias']), vk('VK_SAMPLE_COUNT_1_BIT')]
    values += [f32_bits(view['vp'][k]) for k in ('x', 'y', 'width', 'height', 'minDepth', 'maxDepth')]
    values += [view['scissor'][k] & 0xffffffff for k in ('x', 'y', 'width', 'height')]
    values += [int(d['depthBoundsEnable']), f32_bits(d['minDepthBounds']), f32_bits(d['maxDepthBounds'])]
    require(len(values) == 44, 'state schema differs')
    return struct.pack('<44I', *values), dict(zip(STATE_WORDS, values))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=AUDIT / 'original-world-depth')
    parser.add_argument('--output', type=Path, default=AUDIT / 'world-depth-attachment-fixtures')
    parser.add_argument('--enum-authority', type=Path, default=AUDIT / 'renderdoc-depth-enums.json')
    parser.add_argument('--vulkan-header', type=Path, default=Path('/usr/include/vulkan/vulkan_core.h'))
    args = parser.parse_args()
    source_raw = pinned(args.source / 'original-world-depth.json', SOURCE_SHA)
    packed_raw = pinned(args.source / 'original-world-depth-packed-audit.json', PACKED_SHA)
    source, packed = json.loads(source_raw), json.loads(packed_raw)
    require(source['status'] == packed['status'] == 'passed' and not source['failures'], 'source capture extraction failed')
    require(source['capture_sha256'] == packed['capture_sha256'] == CAPTURE_SHA, 'capture identity differs')
    enums_raw, header_raw = args.enum_authority.read_bytes(), args.vulkan_header.read_bytes()
    enums = json.loads(enums_raw)
    mappings, conversions, vk = enum_mapping(enums, header_raw.decode())
    args.output.mkdir(parents=True, exist_ok=True)
    save(args.output, 'renderdoc-depth-enums.json', enums_raw)
    rows, tsv = [], ['H2_WORLD_DEPTH_ATTACHMENT_V1']
    originals = {r['event']: r for r in source['passes']}
    packed_rows = {r['event']: r for r in packed['passes']}
    for event in EVENTS:
        row, packed_row = originals[event], packed_rows[event]
        description = row['framebuffer_images']['depth']['description']
        width, height = description['width'], description['height']
        require((width, height) == (640, 480) and description['format']['type'] == 20,
                'expected retained640x480 D24S8 source')
        records = source_item(args.source, row['post_stage_buffers']['2']['raw'])
        require(len(records) == packed_row['emitted_records'] * 204 and packed_row['gs_record_stride_bytes'] == 204,
                'source GS record stride/count differs')
        require([(f['name'], f['byte_offset']) for f in packed_row['fields']] ==
                [('vtxPos0', 152), ('vtxPos1', 168), ('vtxPos2', 184), ('triMZ', 200)], 'packed field authority differs')
        fs = source_item(args.source, row['stages']['4']['spirv'])
        mask_fs, proof = source_fragment(fs, event)
        blocks = row['stages']['4']['bound_blocks']
        require(len(blocks) == 1 and blocks[0]['index'] == 0, 'expected one captured FS block')
        ubo = source_item(args.source, blocks[0]['raw'])
        require(len(ubo) == 944, 'full captured FS UBO must be944bytes')
        constants = row['stages']['4']['constant_blocks']
        require(len(constants) == 1 and constants[0]['byteSize'] == 944 and
                constants[0]['fixedBindSetOrSpace'] == 0 and constants[0]['fixedBindNumber'] == 1,
                'reflected FS UBO binding differs')
        require(struct.unpack_from('<4f', ubo, 272) == (0.0, 16777215.0, 255.99998474121094, 16777215.0) and
                struct.unpack_from('<2f', ubo, 832) == (0.0, 0.0) and
                struct.unpack_from('<2i', ubo, 864) == (1, 1), 'captured depth uniforms differ')
        require(all(struct.unpack_from('<4i', ubo, 288 + i * 16) == (0, 0, 641, 481) for i in range(8)),
                'captured clip rectangle differs')
        state_raw, readable_state = state(row, width, height, len(records) // 204, proof['sampler_cube_mask'], mappings, vk)
        files = {k: save(args.output, f'{event}-{name}', raw) for k, name, raw in (
            ('state', 'state.bin', state_raw), ('original_fs', 'original.frag.spv', fs),
            ('mask_fs', 'mask.frag.spv', mask_fs), ('records', 'records.bin', records), ('ubo', 'fs-ubo.bin', ubo))}
        statistics = {}
        for phase in ('before', 'after'):
            image = source_item(args.source, row['framebuffer_images']['depth'][phase])
            pixels = width * height
            require(len(image) == pixels * 5, 'RenderDoc D24S8 readback plane size differs')
            raw_depth, stencil = image[:pixels * 4], image[pixels * 4:]
            raw_words = [v[0] for v in struct.iter_unpack('<I', raw_depth)]
            # RenderDoc's depth aspect has an unused high byte: some captured
            # clear words areFFFFFFFF. Only low24 are D24 depth; the separate
            # trailing plane is the stencil authority. Keep those integers
            # exactly and canonicalize unused bits for the transfer fixture.
            integers = [v & 0xffffff for v in raw_words]
            depth = struct.pack('<' + 'I' * pixels, *integers)
            require(all(0 < v <= 0xffffff for v in integers), 'canonical nonzero captured D24 seed boundary exceeded')
            files[phase + '_depth'] = save(args.output, f'{event}-{phase}-depth-u32.bin', depth)
            files[phase + '_stencil'] = save(args.output, f'{event}-{phase}-stencil-u8.bin', stencil)
            statistics[phase] = dict(minimum_depth=min(integers), maximum_depth=max(integers),
                                     stencil_values=sorted(set(stencil)),
                                     source_unused_high_depth_bytes=sorted({v >> 24 for v in raw_words}),
                                     source_raw_readback=row['framebuffer_images']['depth'][phase])
            if phase == 'after':
                alias = bytes(c for n in integers for c in ((n >> 8) & 255, n & 255, 0, (n >> 16) & 255))
                files['expected_alias_rgba'] = save(args.output, f'{event}-expected-alias.rgba', alias)
        ordered = ('state', 'original_fs', 'mask_fs', 'records', 'ubo', 'before_depth', 'before_stencil', 'after_depth', 'after_stencil')
        tsv.append('\t'.join([str(event), row['role'], *(files[k]['file'] for k in ordered)]))
        rows.append(dict(event=event, role=row['role'], width=width, height=height, record_count=len(records) // 204,
                         files=files, source_state={k: row[k] for k in ('rasterizer', 'depth_stencil', 'multisample', 'viewport_scissor')},
                         state_words=readable_state, fragment_binary_proof=proof, captured_depth_statistics=statistics,
                         fs_reflection=row['stages']['4']['constant_blocks']))
    manifest = dict(status='prepared', scope=__doc__, version='H2_WORLD_DEPTH_ATTACHMENT_V1',
                    capture_sha256=CAPTURE_SHA, source_report_sha256=SOURCE_SHA, packed_report_sha256=PACKED_SHA,
                    enum_authority_sha256=sha(enums_raw), enum_authority_binary_hashes=enums['binary_sha256'],
                    vulkan_header_sha256=sha(header_raw), enum_conversions=conversions,
                    state_binary_layout=[dict(word=i, name=name) for i, name in enumerate(STATE_WORDS)],
                    source_conditions_required_but_not_in_state_words=dict(polygon_mode='FILL', depth_clip_enable=False,
                        provoking_vertex='FIRST', rasterizer_discard=False, conservative_rasterization='DISABLED',
                        negative_one_to_one_depth=False, sample_mask=0xffffffff, sample_shading=False,
                        custom_sample_locations=False, discard_rectangles=False, shading_rate=[1, 1]),
                    diagnostic_color_policy=dict(format='R8_UNORM', blend_enable=False, logic_op_enable=False,
                                                 write_mask='R', clear_value=0, accepted_red_value=1),
                    dummy_texture_boundary='Valid finite2D/Cube resources; only pinned module rejection/depth independence proved.',
                    compiled=False, gpu_executed=False, production_edited=False, passes=rows)
    manifest_bytes = (json.dumps(manifest, indent=2, sort_keys=True) + '\n').encode()
    save(args.output, 'manifest.json', manifest_bytes)
    save(args.output, 'manifest.tsv', ('\n'.join(tsv) + '\n').encode())
    print(json.dumps(dict(status='prepared', passes=len(rows), records=sum(r['record_count'] for r in rows),
                         manifest_sha256=sha(manifest_bytes), output=str(args.output))))


if __name__ == '__main__':
    main()
