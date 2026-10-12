#!/usr/bin/env python3
"""Inspect immutable actual weather packet, SDK caches and pinned xemu source.

No title instruction, GPU operation or live guest access is performed. Derived
sampler words have literal SDK/cache authority; no live PGRAPH state is assumed.
The explicit float32 VS diagnostic is not a captured post-VS or raster result.
"""
from pathlib import Path
import collections
import csv
import hashlib
import json
import math
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from differential_source_object_admission_snapshot import Snapshot, PIN, uint, words
from xbe import Xbe, FUNCTIONS_CSV
from capstone import Cs, CS_ARCH_X86, CS_MODE_32

COMMIT = 'fc24584ce88f0915ad7f04775bb7712c2e3f49ee'
ENTRY = 'f76d0569dedcd2e94d3f2ee4c67a97db47cefea848105a8b24fe08342daf24cd'
RETURN = '952562fe625ce266ca3c2d121b10177866e83235ceb8b8ca99df19764abfd7d9'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def main():
    directory = ROOT / 'analysis/multiplayer-shading/weather-particle-packet'
    manifest = json.loads((directory / 'manifest.json').read_text())
    assert sha((directory / 'packet.json').read_bytes()) == manifest['packet']['sha256']
    for path, digest in manifest['dependencies'].items():
        assert sha((ROOT / path).read_bytes()) == digest, path
    for item in manifest['artifacts'].values():
        raw = (ROOT / item['path']).read_bytes()
        assert len(raw) == item['bytes'] and sha(raw) == item['sha256'], item['path']
    packet = json.loads((directory / 'packet.json').read_text())
    capture = ROOT / 'analysis/multiplayer-shading/original-weather-20261009/session1'
    entry, returned = [Snapshot(capture / ('lockout-weather-particle-' + label))
                       for label in ('entry', 'return')]
    assert sha(entry.ram) == ENTRY == packet['entry_RAM_SHA256']
    assert sha(returned.ram) == RETURN == packet['return_RAM_SHA256']
    image = Xbe(ROOT / 'assets/disc/default.xbe')
    assert sha(image.data) == PIN == packet['source_xbe_sha256']
    functions = {int(row['va'], 16): row for row in csv.DictReader(Path(FUNCTIONS_CSV).open())}
    disassembler = Cs(CS_ARCH_X86, CS_MODE_32)
    regions, disassembly = {}, []
    for address in (0x391f0, 0x3ffe00, 0x3f6390, 0x3f5f30,
                    0x3f5e30, 0x3f54f0, 0x3f6190, 0x1e930, 0x3f59b0, 0x3f5590):
        raw = image.read(address, int(functions[address]['size']))
        assert entry.read(address, len(raw)) == returned.read(address, len(raw)) == raw
        instructions = list(disassembler.disasm(raw, address))
        assert sum(i.size for i in instructions) == len(raw)
        regions[f'{address:08X}'] = dict(bytes=len(raw), sha256=sha(raw),
                                       statically_decoded_instructions=len(instructions))
        disassembly.append(f'{address:08X} {len(raw)} bytes SHA256 {sha(raw)}\n')
        disassembly.extend(f'{i.address:08X} {i.bytes.hex():24s} {i.mnemonic} {i.op_str}\n'
                           for i in instructions)
    primary = ROOT / 'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained'
    tree = json.loads((primary / 'tree.json').read_text())
    assert tree['sha'] == COMMIT and not tree['truncated']
    blobs = {item['path']: item['sha'] for item in tree['tree'] if item['type'] == 'blob'}
    primary_files = {}
    for suffix in ('nv2a_regs.h', 'pgraph/pgraph.c', 'pgraph/gl/texture.c',
                   'pgraph/gl/constants.h', 'pgraph/gl/draw.c', 'pgraph/texture.c',
                   'pgraph/glsl/vsh.c', 'pgraph/glsl/vsh-prog.c', 'pgraph/glsl/psh.c',
                   'pgraph/glsl/common.c', 'pgraph/glsl/geom.c',
                   'pgraph/vk/instance.c', 'pgraph/vk/draw.c'):
        relative = 'hw/xbox/nv2a/' + suffix
        path = primary / relative
        raw = path.read_bytes()
        blob = hashlib.sha1(b'blob ' + str(len(raw)).encode() + b'\0' + raw).hexdigest()
        assert blob == blobs[relative], relative
        primary_files[relative] = dict(path=str(path.relative_to(ROOT)), git_blob=blob,
                                      bytes=len(raw), sha256=sha(raw))
    device = uint(entry.read(0x407488, 4))
    assert device == uint(returned.read(0x407488, 4)) == 0x404fe0
    specs = [('stage3', 0x404f50, 128), ('render_cache', 0x40757c, 0x64),
             ('title_color_mask', 0x4b8308, 4), ('stage3_control0', device + 0x780, 4),
             ('point_globals', 0x407668, 32), ('depth_mode', 0x4076d4, 4),
             ('stencil_enable', 0x4076d8, 4), ('title_stencil_enable', 0x4b843c, 4),
             ('integer_depth_bias_request', 0x4076ec, 4), ('title_integer_depth_bias', 0x4b8450, 4),
             ('zeta_surface_pointer', device + 0x1a08, 4),
             ('control0_extra', 0x407718, 4), ('depth_clip_bank', device + 0x1c08, 20),
             ('raster_viewport', device + 0xee0, 48), ('resolution_bank', device + 0x93c, 64),
             ('stage3_texture_pointer', device + 0xf94, 4)]
    banks = {}
    for label, address, size in specs:
        before, after = entry.read(address, size), returned.read(address, size)
        banks[label] = dict(address=f'{address:08X}', bytes=size,
            entry_hex=before.hex(), return_hex=after.hex(), entry_sha256=sha(before),
            return_sha256=sha(after), unchanged=before == after)
    stage = words(entry.read(0x404f50, 128))
    assert banks['stage3']['unchanged'] and stage[:12] == [1, 1, 1, 2, 2, 2, 0, 0, 0, 0, 0, 0]
    table_raw = image.read(0x4039c4, 12 * 4)
    lookup_raw = entry.read(0x407620, 4 * 4)
    assert table_raw == entry.read(0x4039c4, len(table_raw)) == returned.read(0x4039c4, len(table_raw))
    assert lookup_raw == returned.read(0x407620, len(lookup_raw))
    table, lookup = words(table_raw), words(lookup_raw)
    address_word = (((stage[2] << 8 | stage[1]) << 8) | lookup[stage[28] & 0xffff] | stage[0])
    # Actual admitted branch3FFE82..3FFEEB: MIN/MAG both2<3; no anisotropy rewrite.
    lod_bias = struct.unpack('<f', struct.pack('<I', stage[6]))[0]
    lookup_bias = struct.unpack('<f', entry.read(device + 0x968, 4))[0]
    assert lod_bias == lookup_bias == 0.0
    lod_fixed = int(f32((lod_bias + lookup_bias) * 256.0 + 0.5))
    filter_word = table[stage[4] * 3 + stage[5]] | stage[3] << 24 | stage[10] | 0x2000 | lod_fixed & 0x1fff
    control_word = stage[7] << 26 | stage[11] | stage[9] | 0x3ffc0
    if uint(entry.read(device + 0xf94, 4)):
        control_word |= 0x40000000
    assert (address_word, control_word, filter_word) == (0x10101, 0x4003ffc0, 0x2062000)
    assert uint(entry.read(device + 0x780, 4)) == control_word
    mask = uint(entry.read(0x4075a4, 4))
    assert mask == uint(returned.read(0x4075a4, 4)) == uint(entry.read(0x4b8308, 4)) == 0x10101
    map_offset = (0x4075a4 - 0x407498) // 4
    assert uint(image.read(0x43e6f0 + map_offset * 4, 4)) == 0x40358
    original = json.loads((ROOT / 'analysis/multiplayer-shading/weather-particle-entry-original.json').read_text())
    packets = original['original_draw_entry']['appended_commands']['packets']
    methods = collections.defaultdict(list)
    for item in packets:
        for index, value in enumerate(item['words']):
            methods[f'{int(item["method"], 16) + index * 4:04X}'].append(value)
    for method in ('1BC8', '1BCC', '1BD4', '0358', '02B4', '032C', '0384', '0388', '0330', '0334', '0338'):
        assert method not in methods
    bias_words = words(entry.read(0x4075cc, 20))
    assert bias_words == words(returned.read(0x4075cc, 20)) == [0x80000000, 0x80000000, 0, 0, 0]
    stencil = uint(entry.read(0x4076d8, 4))
    assert stencil == uint(returned.read(0x4076d8, 4)) == uint(entry.read(0x4b843c, 4)) == 0
    explicit = {'030C': '00000001', '035C': '00000000', '0354': '00000203',
                '0290': '00110001', '0394': '437FFFFF', '0398': '4B7FFFFF',
                '1D78': '00000000', '0318': '00000001', '031C': '00000001', '043C': '00000008'}
    assert all(methods[key] and all(word == value for word in methods[key])
               for key, value in explicit.items())
    constants_raw = (directory / 'vertex_constants_c0_c11.bin').read_bytes()
    constants = struct.unpack('<48f', constants_raw)
    constants = [constants[i:i + 4] for i in range(0, 48, 4)]
    diagnostic = []
    for vertex in packet['emitted']:
        v = struct.unpack('<4f', struct.pack('<4I', *vertex['source_v0_words']))
        def dph(c):
            return f32(f32(f32(f32(v[0] * c[0]) + f32(v[1] * c[1])) + f32(v[2] * c[2])) + c[3])
        pos = [dph(c) for c in constants[:4]]
        reciprocal = f32(1.0 / pos[3])
        diameter = f32(reciprocal * v[3])
        x, y = [f32(f32(f32(pos[i] * constants[10][i]) * reciprocal) + constants[11][i]) for i in range(2)]
        assert all(math.isfinite(t) for t in (pos[3], diameter, x, y))
        diagnostic.append(dict(source_particle_index=vertex['source_particle_index'],
            DPH_W=pos[3], authored_oPts=diameter, source_screen_center=[x, y],
            center_inside_rectangle=0 <= x < 640 and 0 <= y < 480))
    inside = [v for v in diagnostic if v['center_inside_rectangle']]
    def range_of(rows, key):
        return [min(v[key] for v in rows), max(v[key] for v in rows)]
    def center_ranges(rows):
        return [[min(v['source_screen_center'][i] for v in rows),
                 max(v['source_screen_center'][i] for v in rows)] for i in range(2)]
    stats = dict(points=len(diagnostic), positive_W=sum(v['DPH_W'] > 0 for v in diagnostic),
        W_range=range_of(diagnostic, 'DPH_W'), oPts_range=range_of(diagnostic, 'authored_oPts'),
        diameter_over64=sum(v['authored_oPts'] > 64 for v in diagnostic),
        diameter_at_most1=sum(v['authored_oPts'] <= 1 for v in diagnostic),
        all_center_ranges=center_ranges(diagnostic), centers_inside_rectangle=len(inside),
        rectangle_center_ranges=center_ranges(inside), rectangle_oPts_range=range_of(inside, 'authored_oPts'))
    assert stats['points'] == stats['positive_W'] == 238 and stats['centers_inside_rectangle'] == 33
    out = ROOT / 'analysis/multiplayer-shading/weather-point-inherited-control-inspection.json'
    disasm = out.with_name('weather-point-inherited-control-inspection-disasm.txt')
    disasm.write_text(''.join(disassembly))
    report = dict(authority=__doc__, original_executable_sha256=PIN,
        entry_RAM_sha256=ENTRY, return_RAM_sha256=RETURN, packet_sha256=manifest['packet']['sha256'],
        packet_manifest_sha256=sha((directory / 'manifest.json').read_bytes()),
        original_execution_count=0, GPU_operations=0, live_guest_access=False,
        literal_code_regions=regions, disassembly_sha256=sha(disasm.read_bytes()),
        pinned_xemu=dict(commit=COMMIT, tree_sha256=sha((primary / 'tree.json').read_bytes()),
                         git_blob_verified_files=primary_files), banks=banks,
        inherited_sampler=dict(stage=3, cache_address='00404F50', unchanged=True,
            indices0_through8=dict(address_U=stage[0], address_V=stage[1], address_P=stage[2],
                mag_filter=stage[3], min_filter=stage[4], mip_filter=stage[5],
                LOD_bias_word=f'{stage[6]:08X}', max_mip_level=stage[7], max_anisotropy=stage[8]),
            SDK_derived_words=dict(method1BC8=f'{address_word:08X}', method1BCC=f'{control_word:08X}',
                                   method1BD4=f'{filter_word:08X}'),
            encoded_LOD_clamps=dict(minimum=(control_word & 0x3ffc0000) >> 18,
                maximum=(control_word & 0x3ffc0) >> 6, resource_levels=4,
                pinned_xemu_effective_minimum=0, pinned_xemu_effective_maximum=3,
                source='pgraph/texture.c207..210 and281..306 clamp encoded mip range to actual four-level resource.'),
            decoded='U/V/P repeat, linear magnification, linear minification with linear mip interpolation; zero bias, minimum mip0, actual four-level BC3 resource retained.',
            source='Complete3FFE00:3FFE3B..3FFE6D address;3FFE82..3FFEEB filter branch;3FFF19..3FFF7D LOD/filter encoding;3FFF96..3FFFB2 methods.',
            min_filter_table=dict(address='004039C4', raw_hex=table_raw.hex(), selected_index=8, selected_word='00060000'),
            address_extra_table=dict(address='00407620', raw_hex=lookup_raw.hex(), selected_index=stage[28]),
            live_PGRAPH_state_captured=False, emitted_in_weather_span=False),
        inherited_color_mask=dict(SDK_address='004075A4', title_mirror_address='004B8308',
            method='0358', word=f'{mask:08X}', RGB=True, alpha=False, unchanged=True,
            source='1E971..1E981 pairs40358 command with4075A4 cache; literal43E6F0 method table maps cache index67 to40358.',
            emitted_in_weather_span=False, live_PGRAPH_state_captured=False),
        inherited_depth_bias=dict(factor_address='004075CC', bias_address='004075D0',
            factor_word='80000000', bias_word='80000000', numeric_factor=-0.0, numeric_bias=-0.0,
            point_line_fill_enable_addresses=['004075D4', '004075D8', '004075DC'], enable_words=['00000000'] * 3,
            unchanged=True, emitted_in_weather_span=False, live_PGRAPH_state_captured=False,
            source='3F59B0:3F59D3 FCHS followedby0384/0388 command+cache stores;0330/0334/0338 enable stores.',
            point_uniform_primary='psh.c1711 applies polygonoffset only forprimitive>=TRIANGLES;actualPOINTS leavesdepthOffset/depthFactor uniforms positivezero at1727..1756.'),
        inherited_stencil=dict(SDK_address='004076D8', title_mirror_address='004B843C', enabled=False,
            word='00000000', unchanged=True, emitted_in_weather_span=False, live_PGRAPH_state_captured=False,
            source='Complete3F5590:3F559C caches suppliedenable;3F55EC..3F5628 emits032C0 forfalse irrespectivezeta pointer.',
            native_admission='Carry this supplied SDK stencil state explicitly; unsupported enabled-stencil input requires rejection, not silent disable.'),
        explicit_weather_methods=explicit,
        explicit_weather_method_counts={key: len(methods[key]) for key in explicit},
        explicit_depth=dict(depth_test=True, depth_write=False, compare='LEQUAL',
            W_perspective=True, clip_cull=True, clip_min_word='437FFFFF', clip_max_word='4B7FFFFF',
            clip_min=struct.unpack('<f', struct.pack('<I', 0x437fffff))[0],
            clip_max=struct.unpack('<f', struct.pack('<I', 0x4b7fffff))[0],
            source='391F0:39371..393A1 depth-write/compare;3F54F0 emits030C and1D78;3F6390 emits0290;3F6190 emits0394/0398.',
            point_fragment_primary='psh.c1004..1036: point W depth isvtxPos0.w; nonpositive/NaN becomesFLOAT_MAX;psh.c1058..1064 discards outside clip range under CULL.'),
        VS_diagnostic=dict(authority='Explicit host float32 multiply/add/divide evaluation of authoredVS22 using actual source v0/constants. No post-VS capture, hostGPU or NV2A precision claim.',
            arithmetic='DPH has implicit1 and excludesv0.w; left-associated separately-rounded float32 dot, reciprocal1/W, separately-rounded MUL/MAD. Exact shader compiler dot/contraction may differ.',
            stats=stats, rows=diagnostic,
            immediate_color_counts=dict(collections.Counter(f'{v["sdk_v1_word"]:08X}' for v in packet['emitted'])),
            visible_pixels_proved=False,
            behind_camera_note='No nonpositive W occurs in this supplied packet diagnostic. Negative world coordinates and offscreen centers are distinct; hypothetical negative W is not an actual238-row case.'),
        point_primary=dict(programmable='vsh.c300..311 overridesoPts only when point_params_enable=false; enabled authoredoPts passes togl_PointSize at412. SDK maximum64 is not applied to programmableoPts there.',
            no_point_geometry='geom.c90..91 returns false for point geometry stage.',
            screen_bridge='vsh-prog.c757..770 truncates screen XY to1/16, clamps W awayzero/inf, then undoes divide for hostclip; no invented W discard in native source.',
            sprite_coordinate='psh.c1078..1081 assigns stage3vec4(gl_PointCoord,1,1); actual hostGL coordinate orientation is not captured by CPU banks.',
            Vulkan_backend='vk/instance.c490..497 requests geometrypointsize feature but notlargePoints;vk/draw.c949 explicitly marks pointsize FIXME. This is not a supported programmable-size cap rule.'),
        Vulkan_primary=dict(rasterization='https://docs.vulkan.org/spec/latest/chapters/primsrast.html',
            features='https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceFeatures.html',
            limits='https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceLimits.html',
            requirements='PointSize must bepositive; only1 is supported withoutlargePoints; written size is clamped to implementationpointSizeRange and gradations arepointSizeGranularity. Device limits must be reported by the isolated pass, not inferred from SDK maximum64.'),
        unresolved=['Live PGRAPH inherited sampler/color/window-clip register bank and previous command consumption are not exported.',
            'Window clip type/rectangles are absent from appended weather span; captured source rectangle does not establish PGRAPH window clip bank.',
            'Actual framebuffer depth contents and depth-tested visible pixels, host point limits/rounding/coordinate orientation, and physical NV2A coverage remain separate.',
            'Diagnostic centers inside source rectangle are candidate centers, not a raster visibility result.'],
        tool_sha256=sha(Path(__file__).read_bytes()))
    out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(report=str(out.relative_to(ROOT)), sampler=report['inherited_sampler']['SDK_derived_words'],
                         color_mask=f'{mask:08X}', VS_diagnostic=stats, original_execution=0), indent=2))


if __name__ == '__main__':
    main()
