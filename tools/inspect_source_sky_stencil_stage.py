"""CPU-only admission and typed fixtures from unchanged original20 stage exports."""
from pathlib import Path
import hashlib
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'analysis/multiplayer-shading/source-sky-stencil-stage-reference'
OUT = ROOT/'analysis/multiplayer-shading/source-sky-stencil-prototype'
OLD = ROOT/'analysis/multiplayer-shading/appearance-residual-replay/source'
DEPTH = ROOT/'analysis/multiplayer-shading/original20-sky-stencil-analysis'
GS_SHA = '20ac4499716ad849300a2230c91faed205ad0c609bc0779e22006ae88aa45e22'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def offsets(signature, std430):
    offset, result = 0, {}
    for item in signature:
        n = item['compCount']
        if std430:
            alignment = 4 if n == 1 else 8 if n == 2 else 16
            offset = (offset + alignment - 1)//alignment*alignment
        result[item['varName']] = offset
        offset += n*4
    return result, (offset+15)//16*16 if std430 else offset


def ordered_int(bits):
    return 0x80000000-bits if bits & 0x80000000 else bits+0x80000000


def run():
    OUT.mkdir(parents=True, exist_ok=True)
    report = dict(scope='Retained original20 VSOut strip, captured GS and minimal texture-independent stencil diagnostic admission.',
        GPU_executed=False, live_renderer_activated=False, geometry=[],
        boundary='VSOut and GSOut are unchanged shader replay exports, not a physical Xbox precision claim. No sky color or complete world appearance is rendered.')
    export = json.loads((SOURCE/'report.json').read_text())
    assert export['status'] == 'complete' and export['capture_unchanged']
    coverage = json.loads((SOURCE/'coverage-state.json').read_text())
    assert coverage['status'] == 'complete' and coverage['capture_unchanged']
    coverage_by_event = {x['event']:x for x in coverage['events']}
    for event, count, vertices in ((8097,74,37),(8102,49,25),(8110,4,4)):
        packet = json.loads((SOURCE/f'{event}-packet.json').read_text())
        gs = (SOURCE/f'{event}-geometry.spvasm').read_text()
        fs = (SOURCE/f'{event}-fragment.spvasm').read_text()
        assert sha(SOURCE/f'{event}-geometry.spv') == GS_SHA
        assert '*v = {0, 1, 2};' in gs and 'ExecutionMode(main, Triangles);' in gs
        assert 'ExecutionMode(main, OutputTriangleStrip);' in gs and 'ExecutionMode(main, OutputVertices(3));' in gs
        assert not packet['geometry']['blocks']
        # Full textual main CFG contains one region-only demotion; all other
        # branches are finite-depth handling. Alpha is written, never tested.
        assert fs.count('DemoteToHelperInvocation();') == 1
        assert not re.search(r'\b(Kill|TerminateInvocation|Discard)\(', fs)
        gate = fs[fs.index('*clipContained = false;'):fs.index('float4 _143 = *gl_FragCoord;')]
        assert 'clipRegion' in gate and 'DemoteToHelperInvocation();' in gate
        assert 'texSamp' not in gate and 'alphaRef' not in gate
        assert 'FClamp(_268, _272, _274)' in fs
        assert 'Floor(' in fs and '/ 16777216.0000' in fs
        vo, stride = offsets(packet['vertex']['outputs'], True)
        go, gstride = offsets(packet['geometry']['outputs'], False)
        assert stride == 240 and vo['gl_Position'] == 0 and vo['v_vtxPos0'] == 176
        assert gstride == 204 and [go[k] for k in ('gl_Position','vtxPos0','vtxPos1','vtxPos2','triMZ')] == [0,152,168,184,200]
        assert packet['VSOut']['description']['vertexByteStride'] == stride
        assert packet['GSOut']['description']['vertexByteStride'] == gstride
        assert packet['GSOut']['description']['numIndices'] == (count-2)*3
        assert packet['VSOut']['description']['topology'] == 6 and packet['GSOut']['description']['topology'] == 5
        assert not packet['VSOut']['description']['flipY']
        indices = (SOURCE/f'{event}-vsout-indices.bin').read_bytes()
        assert len(indices) == count*4 and indices == (SOURCE/f'{event}-source-indices.bin').read_bytes()
        iv = struct.unpack('<'+'I'*count, indices)
        assert max(iv) == vertices-1 and min(iv) == 0
        raw = (SOURCE/f'{event}-vsout-vertices.bin').read_bytes()
        assert len(raw) == vertices*stride
        old = OLD if event != 8110 else ROOT/'analysis/multiplayer-shading/original20-moon-stencil'
        assert raw == (old/f'{event}-postvs.bin').read_bytes()
        assert indices == (old/f'{event}-indices.bin').read_bytes()
        ps = (SOURCE/f'{event}-fragment-block0.bin').read_bytes()
        assert ps == (old/f'{event}-fragment-block0.bin').read_bytes()
        clip = struct.unpack_from('<4f',ps,272)
        factor = struct.unpack_from('<2f',ps,832)
        scale = struct.unpack_from('<2i',ps,864)
        regions = struct.unpack_from('<32i',ps,288)
        assert clip == (0.0,16777215.0,255.99998474121094,16777215.0)
        assert factor == (0.0,0.0) and scale == (1,1) and regions == (0,0,641,481)*8
        uniform = ps[272:416]+ps[832:840]+ps[864:872]
        typed = b''.join(raw[i*stride:i*stride+16]+raw[i*stride+176:i*stride+192] for i in range(vertices))
        # Original source shader input W values and clip positions are copied
        # exactly. No fitted screen positions or generated coverage mask.
        fixture = b'H2SKST01'+struct.pack('<7I',event,640,480,vertices,count,0,0)+uniform+typed+indices
        target = OUT/f'{event}-retained-strip.fixture'
        target.write_bytes(fixture)
        raster = packet['vulkan']['rasterizer']; ds = packet['vulkan']['depthStencil']
        cov = coverage_by_event[event]
        assert cov['multisample']['rasterSamples'] == 1 and cov['multisample']['sampleMask'] == 0xffffffff
        assert not cov['multisample']['sampleShadingEnable'] and not cov['multisample']['sampleLocations']['customLocations']
        assert not cov['colorBlend']['alphaToCoverageEnable'] and not cov['colorBlend']['alphaToOneEnable']
        # isPassing=false is an inactive default when there is no buffer.
        assert cov['conditionalRendering']['bufferId'] == 'ResourceId::0'
        assert raster['depthClampEnable'] and not raster['depthClipEnable'] and raster['cullMode'] == 0
        assert ds['depthTestEnable'] and not ds['depthWriteEnable'] and ds['depthFunction'] == 3
        assert ds['stencilTestEnable']
        for name in ('frontFace','backFace'):
            assert ds[name] == dict(compareMask=255,depthFailOperation=0,failOperation=0,function=1,passOperation=2,reference=255,writeMask=255)
        before = (DEPTH/f'{event}-before-packed-d24s8.bin').read_bytes()
        after = (DEPTH/f'{event}-after-packed-d24s8.bin').read_bytes()
        assert len(before) == len(after) == 640*480*4
        bw, aw = struct.unpack('<307200I',before),struct.unpack('<307200I',after)
        assert all((a&0xffffff)==(b&0xffffff) for a,b in zip(aw,bw))
        changed = sum(a!=b for a,b in zip(aw,bw))
        assert changed == (462 if event==8097 else 0)
        # GS replay packed output is a control for topology and precision.
        # RenderDoc rewrites VS into compute for VSOut; GSOut uses graphics
        # transform feedback. Record the actual discrepancies, not an assumed
        # equality. Captured incoming order is even(i,i+1,i+2), odd(i,i+2,i+1).
        gsraw = (SOURCE/f'{event}-gsout-vertices.bin').read_bytes()
        assert len(gsraw) == (count-2)*3*204
        for tri in range(count-2):
            first = gsraw[tri*3*204+152:tri*3*204+204]
            for corner in (1,2):
                assert gsraw[(tri*3+corner)*204+152:(tri*3+corner)*204+204] == first
            for row in range(3):
                for corner in range(3):
                    assert gsraw[(tri*3+corner)*204+12:(tri*3+corner)*204+16] == gsraw[(tri*3+row)*204+152+corner*16+12:(tri*3+row)*204+152+corner*16+16]
        gstyped = b''.join(gsraw[i*204:i*204+16]+gsraw[i*204+152:i*204+204]
                            for i in range((count-2)*3))
        gstarget = OUT/f'{event}-retained-gsout.fixture'
        gstarget.write_bytes(b'H2SKGS01'+struct.pack('<7I',event,640,480,(count-2)*3,0,0,0)+uniform+gstyped)
        checks = differing = max_ulp = 0
        for tri in range(count-2):
            order = [iv[tri],iv[tri+1],iv[tri+2]] if tri%2==0 else [iv[tri],iv[tri+2],iv[tri+1]]
            for corner, index in enumerate(order):
                actual = gsraw[tri*3*204+corner*16+152:tri*3*204+corner*16+168]
                retained = raw[index*240+176:index*240+192]
                for a,b in zip(struct.unpack('<4I',actual),struct.unpack('<4I',retained)):
                    checks += 1
                    if a != b:
                        differing += 1;max_ulp=max(max_ulp,abs(ordered_int(a)-ordered_int(b)))
        report['geometry'].append(dict(event=event,source_indices=count,retained_vertices=vertices,
            fixture=str(target.relative_to(ROOT)),fixture_sha256=sha(target),
            passthrough_fixture=str(gstarget.relative_to(ROOT)),passthrough_fixture_sha256=sha(gstarget),
            vertex_offsets=vo,vertex_stride=stride,geometry_offsets=go,geometry_stride=gstride,
            captured_geometry_module_sha256=GS_SHA,geometry_uniform_blocks=0,rotations=[0,0],
            source_uniform_offsets=dict(clipRange=272,clipRegion=288,depthFactor=832,depthOffset=836,surfaceScale=864),
            clip_range=clip,depth_factor_offset=factor,surface_scale=scale,regions=list(regions),
            fragment_demotions=1,alpha_or_texture_discard_dependence=False,
            coverage=dict(raster_samples=1,sample_mask=0xffffffff,custom_sample_locations=False,
                sample_shading=False,alpha_to_coverage=False,alpha_to_one=False,conditional_buffer=None),
            topology='Source triangle strip; captured GS receives triangles and emits 3-vertex strips',
            GSOut_control=dict(triangles=count-2,window_components_compared=checks,
                components_differing_from_compute_VSOut=differing,max_component_ULP_difference=max_ulp,
                incoming_order_even=[0,1,2],incoming_order_odd=[0,2,1]),
            original_stencil_changes=changed,original_depth_changes=0))
        report['geometry'][-1]['original_far_depth_pixels'] = sum((x&0xffffff)==0xffffff for x in bw)
        report['geometry'][-1]['changes_at_far_depth'] = sum(a!=b and (b&0xffffff)==0xffffff for a,b in zip(aw,bw))
    (OUT/'source-admission.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Three original20 stages admitted; retained strip fixtures generated. No native GPU execution.')


if __name__ == '__main__':
    run()
