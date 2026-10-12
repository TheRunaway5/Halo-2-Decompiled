"""Standard immutable original20 moon8110 replay; no replacements or guest I/O."""
from pathlib import Path
import hashlib
import json
import os
import runpy
import traceback
import renderdoc as rd

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
CAPTURE = ROOT / 'analysis/multiplayer-shading/original-xemu/original_capture_20.rdc'
EXPECTED = 'd99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee'
OUT = ROOT / 'analysis/multiplayer-shading/original20-moon-stencil'
DEPENDENCY = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/replay.py'
helpers = runpy.run_path(str(DEPENDENCY))
plain = helpers['plain']
blob = helpers['blob']
digest = helpers['file_digest']


def run():
    OUT.mkdir(parents=True, exist_ok=True)
    assert digest(CAPTURE) == EXPECTED
    report = dict(capture=str(CAPTURE.relative_to(ROOT)), capture_sha256=EXPECTED,
        script_sha256=digest(ROOT/'tools/export_original20_moon_stencil.py'), helper_sha256=digest(DEPENDENCY),
        guest_IO=False, status='opening', snapshots=[],
        scope='Unchanged original20 offline Vulkan replay; moon8110 only. Not physical Xbox or Lockout stencil attribution.')
    cap = ctrl = None
    try:
        cap = rd.OpenCaptureFile()
        assert cap.OpenFile(str(CAPTURE), '', None) == rd.ResultCode.Succeeded
        result, ctrl = cap.OpenCapture(rd.ReplayOptions(), None)
        assert result == rd.ResultCode.Succeeded, str(result)
        actions = helpers['actions_of'](ctrl.GetRootActions())
        textures = {str(t.resourceId): t for t in ctrl.GetTextures()}
        ctrl.SetFrameEvent(8110, True)
        pipe = ctrl.GetPipelineState()
        action = actions[8110]
        assert action.numIndices == 4
        depth = pipe.GetDepthTarget().resource
        texture = textures[str(depth)]
        assert str(depth) == 'ResourceId::8601' and texture.format.Name() == 'D24S8'
        assert texture.width == 640 and texture.height == 480 and texture.msSamp == 1
        vk = ctrl.GetVulkanPipelineState()
        packet = dict(event=8110, action=dict(count=action.numIndices,
            flags=str(action.flags), baseVertex=action.baseVertex, indexOffset=action.indexOffset),
            viewport=plain(pipe.GetViewport(0)), vertex_inputs=plain(pipe.GetVertexInputs()),
            depth=plain(pipe.GetDepthTarget()), samplers=plain(pipe.GetSamplers(rd.ShaderStage.Fragment)),
            vulkan={name: plain(getattr(vk,name)) for name in ('rasterizer','depthStencil',
                'colorBlend','viewportScissor','currentPass','inputAssembly','vertexInput')},
            vertex=helpers['blocks'](ctrl,pipe,rd.ShaderStage.Vertex,OUT,8110),
            fragment=helpers['blocks'](ctrl,pipe,rd.ShaderStage.Fragment,OUT,8110),
            vertex_buffers=[], textures=[])
        buffers = {str(b.resourceId): b for b in ctrl.GetBuffers()}
        for slot, vb in enumerate(pipe.GetVBuffers()):
            if vb.resourceId == rd.ResourceId.Null():
                continue
            remaining = buffers[str(vb.resourceId)].length - vb.byteOffset
            size = min(vb.byteSize, remaining)
            packet['vertex_buffers'].append(dict(slot=slot, binding=plain(vb),
                **blob(OUT,f'8110-vb{slot}.bin',ctrl.GetBufferData(vb.resourceId,vb.byteOffset,size))))
        ib = pipe.GetIBuffer()
        packet['indices'] = dict(binding=plain(ib), **blob(OUT,'8110-indices.bin',
            ctrl.GetBufferData(ib.resourceId,ib.byteOffset+action.indexOffset*ib.byteStride,
                action.numIndices*ib.byteStride)))
        vs = ctrl.GetPostVSData(0,0,rd.MeshDataStage.VSOut)
        packet['postvs'] = dict(description=plain(vs), **blob(OUT,'8110-postvs.bin',
            ctrl.GetBufferData(vs.vertexResourceId,vs.vertexByteOffset,vs.numIndices*vs.vertexByteStride)))
        for used in pipe.GetReadOnlyResources(rd.ShaderStage.Fragment):
            t = textures.get(str(used.descriptor.resource))
            if t is None or used.access.staticallyUnused:
                continue
            item = dict(binding=plain(used), texture=plain(t), subresources=[])
            for mip in range(t.mips):
                for face in range(t.arraysize):
                    sub = rd.Subresource(); sub.mip=mip; sub.slice=face
                    item['subresources'].append(dict(mip=mip, slice=face,
                        **blob(OUT,f'8110-texture{str(t.resourceId).split("::")[-1]}-m{mip}-s{face}.bin',
                            ctrl.GetTextureData(t.resourceId,sub))))
            packet['textures'].append(item)
        (OUT/'8110-pipeline.json').write_text(json.dumps(packet,indent=2)+'\n')
        report['pipeline'] = '8110-pipeline.json'
        for label,event in (('before',8109),('after',8110)):
            ctrl.SetFrameEvent(event,True)
            snapshot = dict(phase=label, replay_inclusive_event=event,
                width=texture.width,height=texture.height,resource=str(depth),format=texture.format.Name(),
                **blob(OUT,f'8110-{label}-depth-d24s8.raw',ctrl.GetTextureData(depth,rd.Subresource())),
                pixel_controls=[])
            for x,y in ((0,0),(320,240),(241,85),(367,120),(100,100),(500,100),(100,400),(500,400),(639,479)):
                value=ctrl.PickPixel(depth,x,y,rd.Subresource(),rd.CompType.Typeless)
                snapshot['pixel_controls'].append(dict(x=x,y=y,floatValue=list(value.floatValue),uintValue=list(value.uintValue)))
            report['snapshots'].append(snapshot)
            (OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        report['status']='complete'
    except Exception:
        report['status']='failed';report['error']=traceback.format_exc()
    finally:
        if ctrl:ctrl.Shutdown()
        if cap:cap.Shutdown()
        report['capture_unchanged']=digest(CAPTURE)==EXPECTED
        (OUT/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['status']=='complete' and report['capture_unchanged'] else 1


if __name__=='__main__':
    try:
        result=run()
    except Exception:
        OUT.mkdir(parents=True,exist_ok=True)
        (OUT/'setup-error.txt').write_text(traceback.format_exc())
        result=1
    os._exit(result)
