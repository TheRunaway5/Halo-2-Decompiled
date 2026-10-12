"""Bounded, unchanged original20 offline replay export for the stencil-only diagnostic.

Run in the bundled qrenderdoc Python environment only with the hardware lease.
No shader replacement, guest I/O, or writes to prior evidence directories.
"""
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
OUT = ROOT / 'analysis/multiplayer-shading/source-sky-stencil-stage-reference'
HELPER = ROOT / 'analysis/multiplayer-shading/appearance-residual-replay/replay.py'
helpers = runpy.run_path(str(HELPER))
plain, blob, digest = (helpers[k] for k in ('plain', 'blob', 'file_digest'))


def stage_record(ctrl, pipe, stage, label, event):
    reflection = pipe.GetShaderReflection(stage)
    if reflection is None:
        return None
    item = dict(shader=str(pipe.GetShader(stage)), entryPoint=reflection.entryPoint,
        inputs=plain(reflection.inputSignature), outputs=plain(reflection.outputSignature),
        constantBlocks=plain(reflection.constantBlocks), blocks=[])
    filename = f'{event}-{label}.spvasm'
    (OUT / filename).write_text(ctrl.DisassembleShader(pipe.GetGraphicsPipelineObject(), reflection, ''))
    item['disassembly'] = filename
    if hasattr(reflection, 'rawBytes'):
        item['module'] = blob(OUT, f'{event}-{label}.spv', reflection.rawBytes)
    for slot, block in enumerate(reflection.constantBlocks):
        bound = pipe.GetConstantBlock(stage, slot, 0).descriptor
        item['blocks'].append(dict(name=block.name, slot=slot, descriptor=plain(bound),
            **blob(OUT, f'{event}-{label}-block{slot}.bin',
                ctrl.GetBufferData(bound.resource, bound.byteOffset, bound.byteSize))))
    return item


def mesh_record(ctrl, event, stage, label):
    mesh = ctrl.GetPostVSData(0, 0, stage)
    item = dict(description=plain(mesh))
    if mesh.vertexResourceId != rd.ResourceId.Null():
        item['vertices'] = blob(OUT, f'{event}-{label}-vertices.bin', ctrl.GetBufferData(
            mesh.vertexResourceId, mesh.vertexByteOffset, mesh.numIndices * mesh.vertexByteStride))
    if mesh.indexResourceId != rd.ResourceId.Null() and mesh.indexByteStride:
        item['indices'] = blob(OUT, f'{event}-{label}-indices.bin', ctrl.GetBufferData(
            mesh.indexResourceId, mesh.indexByteOffset, mesh.numIndices * mesh.indexByteStride))
    return item


def run():
    OUT.mkdir(parents=True, exist_ok=True)
    assert digest(CAPTURE) == EXPECTED
    report = dict(capture=str(CAPTURE.relative_to(ROOT)), capture_sha256=EXPECTED,
        script_sha256=digest(ROOT/'tools/export_source_sky_stencil_stage.py'),
        helper_sha256=digest(HELPER), guest_IO=False, shader_replacements=False,
        scope='Unchanged original20 Vulkan replay stage admission only; not native sky appearance.',
        status='opening', packets=[])
    cap = ctrl = None
    try:
        cap = rd.OpenCaptureFile()
        assert cap.OpenFile(str(CAPTURE), '', None) == rd.ResultCode.Succeeded
        result, ctrl = cap.OpenCapture(rd.ReplayOptions(), None)
        assert result == rd.ResultCode.Succeeded, str(result)
        actions = helpers['actions_of'](ctrl.GetRootActions())
        for event, count in ((8097, 74), (8102, 49), (8110, 4)):
            ctrl.SetFrameEvent(event, True)
            pipe, action, vk = ctrl.GetPipelineState(), actions[event], ctrl.GetVulkanPipelineState()
            assert action.numIndices == count
            packet = dict(event=event, action=dict(numIndices=count, baseVertex=action.baseVertex,
                indexOffset=action.indexOffset, vertexOffset=action.vertexOffset,
                numInstances=action.numInstances, flags=str(action.flags)),
                viewport=plain(pipe.GetViewport(0)), depth=plain(pipe.GetDepthTarget()),
                vulkan={name: plain(getattr(vk, name)) for name in ('rasterizer','depthStencil',
                    'viewportScissor','inputAssembly','currentPass')})
            for stage, label in ((rd.ShaderStage.Vertex, 'vertex'),
                                 (rd.ShaderStage.Geometry, 'geometry'),
                                 (rd.ShaderStage.Fragment, 'fragment')):
                packet[label] = stage_record(ctrl, pipe, stage, label, event)
            assert packet['geometry'] is not None, 'Required triangle geometry stage missing'
            packet['VSOut'] = mesh_record(ctrl, event, rd.MeshDataStage.VSOut, 'vsout')
            packet['GSOut'] = mesh_record(ctrl, event, rd.MeshDataStage.GSOut, 'gsout')
            ib = pipe.GetIBuffer()
            packet['source_indices'] = dict(binding=plain(ib), **blob(OUT, f'{event}-source-indices.bin',
                ctrl.GetBufferData(ib.resourceId, ib.byteOffset + action.indexOffset * ib.byteStride,
                    action.numIndices * ib.byteStride)))
            name = f'{event}-packet.json'
            (OUT/name).write_text(json.dumps(packet, indent=2)+'\n')
            report['packets'].append(dict(event=event, file=name, sha256=digest(OUT/name)))
        report['status'] = 'complete'
    except Exception:
        report['status'] = 'failed'
        report['error'] = traceback.format_exc()
    finally:
        if ctrl:
            ctrl.Shutdown()
        if cap:
            cap.Shutdown()
        report['capture_unchanged'] = digest(CAPTURE) == EXPECTED
        (OUT/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['status'] == 'complete' and report['capture_unchanged'] else 1


if __name__ == '__main__':
    os._exit(run())
