"""Read-only inventory of an actual pStencil Vulkan surface capture.

Run in bundled qrenderdoc --script with PSTENCIL_SURFACE_CAPTURE_RECEIPT and
PSTENCIL_SURFACE_OUT. Inputs and shaders are not replaced. The saved draw and
shader inventory supports subsequent pixel-history diagnostics.
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
    if isinstance(value, rd.ResourceId) or depth > 8:
        return str(value)
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
(out / 'progress.txt').write_text('Opening actual capture\n')
file = controller = None
try:
    receipt = json.loads(Path(os.environ['PSTENCIL_SURFACE_CAPTURE_RECEIPT']).read_text())
    capture = Path(receipt['capture'])
    file = rd.OpenCaptureFile()
    assert file.OpenFile(str(capture), '', None) == rd.ResultCode.Succeeded
    status, controller = file.OpenCapture(rd.ReplayOptions(), None)
    assert status == rd.ResultCode.Succeeded, str(status)
    all_actions = list(actions(controller.GetRootActions()))
    draws = [x for x in all_actions if x.flags & rd.ActionFlags.Drawcall]
    assert draws, 'Actual capture has no draw inventory'
    textures = list(controller.GetTextures())
    shaders, rows = {}, []
    for ordinal, action in enumerate(draws):
        controller.SetFrameEvent(action.eventId, False)
        pipe = controller.GetPipelineState()
        vk = controller.GetVulkanPipelineState()
        row = dict(event=action.eventId, count=action.numIndices,
                   instances=action.numInstances, flags=str(action.flags),
                   targets=plain(pipe.GetOutputTargets()), depth=plain(pipe.GetDepthTarget()),
                   viewport=plain(pipe.GetViewport(0)),
                   depthStencil=plain(vk.depthStencil), shaders={})
        for stage, name in ((rd.ShaderStage.Vertex, 'vertex'),
                            (rd.ShaderStage.Geometry, 'geometry'),
                            (rd.ShaderStage.Fragment, 'fragment')):
            reflection = pipe.GetShaderReflection(stage)
            if reflection is None:
                continue
            identity = str(pipe.GetShader(stage))
            if identity not in shaders:
                raw = bytes(reflection.rawBytes)
                digest = hashlib.sha256(raw).hexdigest()
                path = out / (digest + '.spv')
                if not path.exists():
                    path.write_bytes(raw)
                shaders[identity] = dict(sha256=digest, bytes=len(raw),
                    entry=reflection.entryPoint, encoding=str(reflection.encoding),
                    inputs=plain(reflection.inputSignature), outputs=plain(reflection.outputSignature))
            row['shaders'][name] = identity
        rows.append(row)
        if ordinal % 100 == 0:
            (out / 'progress.txt').write_text(f'Inventoried {ordinal + 1}/{len(draws)} actual draws\n')
    controller.SetFrameEvent(all_actions[-1].eventId, True)
    targets = []
    for texture in textures:
        if texture.width != 1920 or texture.height != 1080:
            continue
        uses = list(controller.GetUsage(texture.resourceId))
        color = [u.eventId for u in uses if u.usage == rd.ResourceUsage.ColorTarget]
        if not color:
            continue
        item = dict(resource=str(texture.resourceId), description=plain(texture), color_events=color)
        save = rd.TextureSave()
        save.resourceId = texture.resourceId
        save.destType = rd.FileType.PNG
        path = out / ('target-' + str(texture.resourceId).split('::')[-1] + '.png')
        result = controller.SaveTexture(save, str(path))
        item.update(image=path.name, save_result=str(result))
        targets.append(item)
    report = dict(scope=__doc__, status='complete', capture=str(capture),
                  native_image=receipt['native_image'],
                  actions=len(all_actions), draw_count=len(draws),
                  textures=len(textures), shaders=shaders, draws=rows, targets=targets)
    (out / 'inventory.json').write_text(json.dumps(report, indent=2) + '\n')
    (out / 'progress.txt').write_text(f'Complete: {len(draws)} draws, {len(shaders)} shaders, {len(targets)} targets\n')
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
