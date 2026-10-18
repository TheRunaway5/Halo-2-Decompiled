"""Read actual paired surface/cap vertex and constant buffers from a native GPU capture.

No source data, shader or graphics state is replaced. Run in bundled qrenderdoc
--script with PSTENCIL_SURFACE_CAPTURE_RECEIPT and PSTENCIL_SURFACE_OUT.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import traceback
import renderdoc as rd


def plain(value, depth=0):
    if isinstance(value, float) and not math.isfinite(value):
        return str(value)
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, rd.ResourceId) or depth > 9:
        return str(value)
    if isinstance(value, (bytes, bytearray)):
        return dict(bytes=len(value), sha256=hashlib.sha256(value).hexdigest())
    if isinstance(value, (list, tuple)) or type(value).__name__.startswith('rdcarray'):
        return [plain(v, depth + 1) for v in value]
    return {k: plain(getattr(value, k), depth + 1) for k in dir(value)
            if not k.startswith('_') and k not in ('this', 'thisown')
            and not callable(getattr(value, k))}


def actions(rows):
    for row in rows:
        yield row
        yield from actions(row.children)


out = Path(os.environ['PSTENCIL_SURFACE_OUT'])
out.mkdir(parents=True, exist_ok=False)
file = controller = None
try:
    receipt = json.loads(Path(os.environ['PSTENCIL_SURFACE_CAPTURE_RECEIPT']).read_text())
    file = rd.OpenCaptureFile()
    assert file.OpenFile(receipt['capture'], '', None) == rd.ResultCode.Succeeded
    status, controller = file.OpenCapture(rd.ReplayOptions(), None)
    assert status == rd.ResultCode.Succeeded, str(status)
    all_draws = {a.eventId: a for a in actions(controller.GetRootActions()) if a.flags & rd.ActionFlags.Drawcall}
    report = dict(scope=__doc__, capture=receipt['capture'], events=[])
    for event in (830, 860, 1209, 1448, 1698, 1762):
        (out / 'progress.txt').write_text(f'Exporting actual event{event}\n')
        controller.SetFrameEvent(event, True)
        pipe = controller.GetPipelineState()
        vk = controller.GetVulkanPipelineState()
        action = all_draws[event]
        vertex = pipe.GetShaderReflection(rd.ShaderStage.Vertex)
        mesh = controller.GetPostVSData(0, 0, rd.MeshDataStage.VSOut)
        raw = controller.GetBufferData(mesh.vertexResourceId, mesh.vertexByteOffset,
                                      mesh.numIndices * mesh.vertexByteStride)
        (out / f'{event}-vsout.bin').write_bytes(bytes(raw))
        row = dict(event=event, mesh=plain(mesh), signature=plain(vertex.outputSignature),
                   viewport=plain(pipe.GetViewport(0)), depthStencil=plain(vk.depthStencil),
                   vertexBindings=plain(pipe.GetVBuffers()), constants={})
        for stage, name in ((rd.ShaderStage.Vertex, 'vertex'), (rd.ShaderStage.Fragment, 'fragment')):
            reflection = pipe.GetShaderReflection(stage)
            banks = []
            for index, block in enumerate(reflection.constantBlocks):
                binding = pipe.GetConstantBlock(stage, index, 0).descriptor
                contents = controller.GetCBufferVariableContents(pipe.GetGraphicsPipelineObject(),
                    pipe.GetShader(stage), stage, pipe.GetShaderEntryPoint(stage), index,
                    binding.resource, binding.byteOffset, binding.byteSize)
                banks.append(dict(definition=plain(block), contents=plain(contents)))
            row['constants'][name] = banks
        for index, binding in enumerate(pipe.GetVBuffers()):
            if binding.resourceId == rd.ResourceId.Null():
                continue
            raw = controller.GetBufferData(binding.resourceId,
                binding.byteOffset + action.vertexOffset * binding.byteStride,
                action.numIndices * binding.byteStride)
            (out / f'{event}-input-{index}.bin').write_bytes(bytes(raw))
        report['events'].append(row)
        (out / 'vertices.json').write_text(json.dumps(report, indent=2) + '\n')
    (out / 'progress.txt').write_text('Complete\n')
except BaseException:
    (out / 'error.txt').write_text(traceback.format_exc())
    if controller is not None:
        controller.Shutdown()
    if file is not None:
        file.Shutdown()
    os._exit(1)
controller.Shutdown()
file.Shutdown()
os._exit(0)
